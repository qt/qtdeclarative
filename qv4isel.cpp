
#include "qv4isel_p.h"
#include "qmljs_runtime.h"
#include "qmljs_objects.h"

#define TARGET_AMD64
typedef quint64 guint64;
typedef qint64 gint64;
typedef uchar guint8;
typedef uint guint32;
typedef void *gpointer;
#include "amd64-codegen.h"

#include <sys/mman.h>
#include <iostream>
#include <cassert>

#ifndef NO_UDIS86
#  include <udis86.h>
#endif

using namespace QQmlJS;
using namespace QQmlJS::x86_64;
using namespace QQmlJS::VM;

namespace {
QTextStream qout(stdout, QIODevice::WriteOnly);
}

static inline void
amd64_patch (unsigned char* code, gpointer target)
{
    guint8 rex = 0;

#ifdef __native_client_codegen__
    code = amd64_skip_nops (code);
#endif
#if defined(__native_client_codegen__) && defined(__native_client__)
    if (nacl_is_code_address (code)) {
        /* For tail calls, code is patched after being installed */
        /* but not through the normal "patch callsite" method.   */
        unsigned char buf[kNaClAlignment];
        unsigned char *aligned_code = (uintptr_t)code & ~kNaClAlignmentMask;
        int ret;
        memcpy (buf, aligned_code, kNaClAlignment);
        /* Patch a temp buffer of bundle size, */
        /* then install to actual location.    */
        amd64_patch (buf + ((uintptr_t)code - (uintptr_t)aligned_code), target);
        ret = nacl_dyncode_modify (aligned_code, buf, kNaClAlignment);
        g_assert (ret == 0);
        return;
    }
    target = nacl_modify_patch_target (target);
#endif

    /* Skip REX */
    if ((code [0] >= 0x40) && (code [0] <= 0x4f)) {
        rex = code [0];
        code += 1;
    }

    if ((code [0] & 0xf8) == 0xb8) {
        /* amd64_set_reg_template */
        *(guint64*)(code + 1) = (guint64)target;
    }
    else if ((code [0] == 0x8b) && rex && x86_modrm_mod (code [1]) == 0 && x86_modrm_rm (code [1]) == 5) {
        /* mov 0(%rip), %dreg */
        *(guint32*)(code + 2) = (guint32)(guint64)target - 7;
    }
    else if ((code [0] == 0xff) && (code [1] == 0x15)) {
        /* call *<OFFSET>(%rip) */
        *(guint32*)(code + 2) = ((guint32)(guint64)target) - 7;
    }
    else if (code [0] == 0xe8) {
        /* call <DISP> */
        gint64 disp = (guint8*)target - (guint8*)code;
        assert (amd64_is_imm32 (disp));
        x86_patch (code, (unsigned char*)target);
    }
    else
        x86_patch (code, (unsigned char*)target);
}
InstructionSelection::InstructionSelection(VM::ExecutionEngine *engine, IR::Module *module, uchar *buffer)
    : _engine(engine)
    , _module(module)
    , _function(0)
    , _block(0)
    , _buffer(buffer)
    , _code(buffer)
    , _codePtr(buffer)
{
}

InstructionSelection::~InstructionSelection()
{
}

void InstructionSelection::operator()(IR::Function *function)
{
    qSwap(_function, function);

    _code = _codePtr;
    _code = (uchar *) ((size_t(_code) + 15) & ~15);
    _function->code = (void (*)(VM::Context *)) _code;
    _codePtr = _code;

    int locals = (_function->tempCount - _function->locals.size() + _function->maxNumberOfArguments) * sizeof(Value);
    locals = (locals + 15) & ~15;

    amd64_push_reg(_codePtr, AMD64_RBP);
    amd64_push_reg(_codePtr, AMD64_R14);
    amd64_push_reg(_codePtr, AMD64_R15);

    amd64_mov_reg_reg(_codePtr, AMD64_RBP, AMD64_RSP, 8);
    amd64_mov_reg_reg(_codePtr, AMD64_R14, AMD64_RDI, 8);
    amd64_alu_reg_imm(_codePtr, X86_SUB, AMD64_RSP, locals);

    amd64_mov_reg_membase(_codePtr, AMD64_R15, AMD64_R14, offsetof(Context, locals), 8);

    foreach (IR::BasicBlock *block, _function->basicBlocks) {
        _block = block;
        _addrs[block] = _codePtr;
        foreach (IR::Stmt *s, block->statements) {
            s->accept(this);
        }
    }

    QHashIterator<IR::BasicBlock *, QVector<uchar *> > it(_patches);
    while (it.hasNext()) {
        it.next();
        uchar *target = _addrs[it.key()];
        foreach (uchar *instr, it.value()) {
            amd64_patch(instr, target);
        }
    }

    amd64_alu_reg_imm(_codePtr, X86_ADD, AMD64_RSP, locals);
    amd64_pop_reg(_codePtr, AMD64_R15);
    amd64_pop_reg(_codePtr, AMD64_R14);
    amd64_pop_reg(_codePtr, AMD64_RBP);
    amd64_ret(_codePtr);

#ifndef NO_UDIS86
    static bool showCode = !qgetenv("SHOW_CODE").isNull();
    if (showCode) {
        printf("code size: %ld bytes\n", (_codePtr - _code));
        ud_t ud_obj;

        ud_init(&ud_obj);
        ud_set_input_buffer(&ud_obj, _code, _codePtr - _code);
        ud_set_mode(&ud_obj, 64);
        ud_set_syntax(&ud_obj, UD_SYN_ATT);

        while (ud_disassemble(&ud_obj)) {
            printf("\t%s\n", ud_insn_asm(&ud_obj));
        }
    }
#endif
    qSwap(_function, _function);
}

String *InstructionSelection::identifier(const QString &s)
{
    return _engine->identifier(s);
}

void InstructionSelection::loadTempAddress(int reg, IR::Temp *t)
{
    if (t->index < 0) {
        const int arg = -t->index - 1;
        amd64_mov_reg_membase(_codePtr, reg, AMD64_R14, offsetof(Context, arguments), 8);
        amd64_lea_membase(_codePtr, reg, reg, sizeof(Value) * arg);
    } else if (t->index < _function->locals.size()) {
        amd64_lea_membase(_codePtr, reg, AMD64_R15, sizeof(Value) * t->index);
    } else {
        amd64_lea_membase(_codePtr, reg, AMD64_RSP, sizeof(Value) * (_function->maxNumberOfArguments + t->index - _function->locals.size()));
    }
}

void InstructionSelection::callActivationProperty(IR::Call *call, IR::Temp *result)
{
    IR::Name *baseName = call->base->asName();
    assert(baseName != 0);

    int argc = 0;
    for (IR::ExprList *it = call->args; it; it = it->next) {
        ++argc;
    }

    int i = 0;
    for (IR::ExprList *it = call->args; it; it = it->next, ++i) {
        IR::Temp *arg = it->expr->asTemp();
        assert(arg != 0);
        amd64_lea_membase(_codePtr, AMD64_RDI, AMD64_RSP, sizeof(Value) * i);
        loadTempAddress(AMD64_RSI, arg);
        amd64_call_code(_codePtr, __qmljs_copy);
    }

    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8); // load the context

    if (result)
        loadTempAddress(AMD64_RSI, result);
    else
        amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_RSI, AMD64_RSI);

    amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*baseName->id));
    amd64_lea_membase(_codePtr, AMD64_RCX, AMD64_RSP, 0);
    amd64_mov_reg_imm(_codePtr, AMD64_R8, argc);
    amd64_call_code(_codePtr, __qmljs_call_activation_property);
}


void InstructionSelection::callValue(IR::Call *call, IR::Temp *result)
{
    IR::Temp *baseTemp = call->base->asTemp();
    assert(baseTemp != 0);

    int argc = 0;
    for (IR::ExprList *it = call->args; it; it = it->next) {
        ++argc;
    }

    int i = 0;
    for (IR::ExprList *it = call->args; it; it = it->next, ++i) {
        IR::Temp *arg = it->expr->asTemp();
        assert(arg != 0);
        amd64_lea_membase(_codePtr, AMD64_RDI, AMD64_RSP, sizeof(Value) * i);
        loadTempAddress(AMD64_RSI, arg);
        amd64_call_code(_codePtr, __qmljs_copy);
    }

    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8); // load the context

    if (result)
        loadTempAddress(AMD64_RSI, result);
    else
        amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_RSI, AMD64_RSI);

    loadTempAddress(AMD64_RDX, baseTemp);
    amd64_lea_membase(_codePtr, AMD64_RCX, AMD64_RSP, 0);
    amd64_mov_reg_imm(_codePtr, AMD64_R8, argc);
    amd64_call_code(_codePtr, __qmljs_call_value);
}

void InstructionSelection::callProperty(IR::Call *call, IR::Temp *result)
{
    IR::Member *member = call->base->asMember();
    assert(member != 0);
    assert(member->base->asTemp());

    int argc = 0;
    for (IR::ExprList *it = call->args; it; it = it->next) {
        ++argc;
    }

    int i = 0;
    for (IR::ExprList *it = call->args; it; it = it->next, ++i) {
        IR::Temp *arg = it->expr->asTemp();
        assert(arg != 0);
        amd64_lea_membase(_codePtr, AMD64_RDI, AMD64_RSP, sizeof(Value) * i);
        loadTempAddress(AMD64_RSI, arg);
        amd64_call_code(_codePtr, __qmljs_copy);
    }

    //__qmljs_call_property(ctx, result, base, name, args, argc);
    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8); // load the context

    if (result)
        loadTempAddress(AMD64_RSI, result);
    else
        amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_RSI, AMD64_RSI);

    loadTempAddress(AMD64_RDX, member->base->asTemp());
    amd64_mov_reg_imm(_codePtr, AMD64_RCX, identifier(*member->name));
    amd64_lea_membase(_codePtr, AMD64_R8, AMD64_RSP, 0);
    amd64_mov_reg_imm(_codePtr, AMD64_R9, argc);
    amd64_call_code(_codePtr, __qmljs_call_property);
}

void InstructionSelection::constructActivationProperty(IR::New *call, IR::Temp *result)
{
    IR::Name *baseName = call->base->asName();
    assert(baseName != 0);

    int argc = 0;
    for (IR::ExprList *it = call->args; it; it = it->next) {
        ++argc;
    }

    int i = 0;
    for (IR::ExprList *it = call->args; it; it = it->next, ++i) {
        IR::Temp *arg = it->expr->asTemp();
        assert(arg != 0);
        amd64_lea_membase(_codePtr, AMD64_RDI, AMD64_RSP, sizeof(Value) * i);
        loadTempAddress(AMD64_RSI, arg);
        amd64_call_code(_codePtr, __qmljs_copy);
    }

    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8); // load the context

    if (result)
        loadTempAddress(AMD64_RSI, result);
    else
        amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_RSI, AMD64_RSI);

    amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*baseName->id));
    amd64_lea_membase(_codePtr, AMD64_RCX, AMD64_RSP, 0);
    amd64_mov_reg_imm(_codePtr, AMD64_R8, argc);
    amd64_call_code(_codePtr, __qmljs_construct_activation_property);
}

void InstructionSelection::constructProperty(IR::New *call, IR::Temp *result)
{
    IR::Member *member = call->base->asMember();
    assert(member != 0);
    assert(member->base->asTemp());

    int argc = 0;
    for (IR::ExprList *it = call->args; it; it = it->next) {
        ++argc;
    }

    int i = 0;
    for (IR::ExprList *it = call->args; it; it = it->next, ++i) {
        IR::Temp *arg = it->expr->asTemp();
        assert(arg != 0);
        amd64_lea_membase(_codePtr, AMD64_RDI, AMD64_RSP, sizeof(Value) * i);
        loadTempAddress(AMD64_RSI, arg);
        amd64_call_code(_codePtr, __qmljs_copy);
    }

    //__qmljs_call_property(ctx, result, base, name, args, argc);
    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8); // load the context

    if (result)
        loadTempAddress(AMD64_RSI, result);
    else
        amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_RSI, AMD64_RSI);

    loadTempAddress(AMD64_RDX, member->base->asTemp());
    amd64_mov_reg_imm(_codePtr, AMD64_RCX, identifier(*member->name));
    amd64_lea_membase(_codePtr, AMD64_R8, AMD64_RSP, 0);
    amd64_mov_reg_imm(_codePtr, AMD64_R9, argc);
    amd64_call_code(_codePtr, __qmljs_construct_property);
}

void InstructionSelection::visitExp(IR::Exp *s)
{
    if (IR::Call *c = s->expr->asCall()) {
        if (c->base->asName()) {
            callActivationProperty(c, 0);
            return;
        } else if (c->base->asTemp()) {
            callValue(c, 0);
            return;
        } else if (c->base->asMember()) {
            callProperty(c, 0);
            return;
        }
    }
    Q_UNIMPLEMENTED();
    assert(!"TODO");
}

void InstructionSelection::visitEnter(IR::Enter *)
{
    Q_UNIMPLEMENTED();
    assert(!"TODO");
}

void InstructionSelection::visitLeave(IR::Leave *)
{
    Q_UNIMPLEMENTED();
    assert(!"TODO");
}

void InstructionSelection::visitMove(IR::Move *s)
{
    // %rdi, %rsi, %rdx, %rcx, %r8 and %r9
    if (s->op == IR::OpInvalid) {
        if (IR::Name *n = s->target->asName()) {
            String *propertyName = identifier(*n->id);

            if (IR::Const *c = s->source->asConst()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, propertyName);

                switch (c->type) {
                case IR::BoolType:
                    amd64_mov_reg_imm(_codePtr, AMD64_RDX, c->value != 0);
                    amd64_call_code(_codePtr, __qmljs_set_activation_property_boolean);
                    break;

                case IR::NumberType:
                    amd64_mov_reg_imm(_codePtr, AMD64_RAX, &c->value);
                    amd64_movsd_reg_regp(_codePtr, X86_XMM0, AMD64_RAX);
                    amd64_call_code(_codePtr, __qmljs_set_activation_property_number);
                    break;

                default:
                    Q_UNIMPLEMENTED();
                    assert(!"TODO");
                }
                return;
            } else if (IR::String *str = s->source->asString()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, propertyName);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, _engine->newString(*str->value));
                amd64_call_code(_codePtr, __qmljs_set_activation_property_string);
                return;
            } else if (IR::Temp *t = s->source->asTemp()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, propertyName);
                loadTempAddress(AMD64_RDX, t);
                amd64_call_code(_codePtr, __qmljs_set_activation_property);
                return;
            } else if (IR::Name *other = s->source->asName()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, propertyName);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*other->id));
                amd64_call_code(_codePtr, __qmljs_copy_activation_property);
                return;
            } else if (IR::Closure *clos = s->source->asClosure()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, propertyName);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, clos->value);
                amd64_call_code(_codePtr, __qmljs_set_activation_property_closure);
                return;
            }
        } else if (IR::Temp *t = s->target->asTemp()) {
            if (IR::Name *n = s->source->asName()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                loadTempAddress(AMD64_RSI, t);
                if (*n->id == QLatin1String("this")) { // ### `this' should be a builtin.
                    amd64_call_code(_codePtr, __qmljs_get_thisObject);
                } else {
                    String *propertyName = identifier(*n->id);
                    amd64_mov_reg_imm(_codePtr, AMD64_RDX, propertyName);
                    amd64_call_code(_codePtr, __qmljs_get_activation_property);
                }
                return;
            } else if (IR::Const *c = s->source->asConst()) {
                loadTempAddress(AMD64_RSI, t);

                switch (c->type) {
                case IR::NullType:
                    amd64_mov_membase_imm(_codePtr, AMD64_RSI, 0, NULL_TYPE, 4);
                    break;

                case IR::UndefinedType:
                    amd64_mov_membase_imm(_codePtr, AMD64_RSI, 0, UNDEFINED_TYPE, 4);
                    break;

                case IR::BoolType:
                    amd64_mov_membase_imm(_codePtr, AMD64_RSI, 0, BOOLEAN_TYPE, 4);
                    amd64_mov_membase_imm(_codePtr, AMD64_RSI, offsetof(Value, booleanValue), c->value != 0, 1);
                    break;

                case IR::NumberType:
                    amd64_mov_reg_imm(_codePtr, AMD64_RAX, &c->value);
                    amd64_movsd_reg_regp(_codePtr, X86_XMM0, AMD64_RAX);
                    amd64_mov_membase_imm(_codePtr, AMD64_RSI, 0, NUMBER_TYPE, 4);
                    amd64_movsd_membase_reg(_codePtr, AMD64_RSI, offsetof(Value, numberValue), X86_XMM0);
                    break;

                default:
                    Q_UNIMPLEMENTED();
                    assert(!"TODO");
                }
                return;
            } else if (IR::Temp *t2 = s->source->asTemp()) {
                loadTempAddress(AMD64_RDI, t);
                loadTempAddress(AMD64_RSI, t2);
                amd64_mov_reg_membase(_codePtr, AMD64_RAX, AMD64_RSI, 0, 4);
                amd64_mov_membase_reg(_codePtr, AMD64_RDI, 0, AMD64_RAX, 4);
                amd64_mov_reg_membase(_codePtr, AMD64_RAX, AMD64_RSI, offsetof(Value, numberValue), 8);
                amd64_mov_membase_reg(_codePtr, AMD64_RDI, offsetof(Value, numberValue), AMD64_RAX, 8);
                return;
            } else if (IR::String *str = s->source->asString()) {
                loadTempAddress(AMD64_RDI, t);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, _engine->newString(*str->value));
                amd64_call_code(_codePtr, __qmljs_init_string);
                return;
            } else if (IR::Closure *clos = s->source->asClosure()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                loadTempAddress(AMD64_RSI, t);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, clos->value);
                amd64_call_code(_codePtr, __qmljs_init_closure);
                return;
            } else if (IR::New *ctor = s->source->asNew()) {
                if (ctor->base->asName()) {
                    constructActivationProperty(ctor, t);
                    return;
                } else if (ctor->base->asMember()) {
                    constructProperty(ctor, t);
                    return;
                }
            } else if (IR::Member *m = s->source->asMember()) {
                //__qmljs_get_property(ctx, result, object, name);
                if (IR::Temp *base = m->base->asTemp()) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, t);
                    loadTempAddress(AMD64_RDX, base);
                    amd64_mov_reg_imm(_codePtr, AMD64_RCX, identifier(*m->name));
                    amd64_call_code(_codePtr, __qmljs_get_property);
                    return;
                }
                assert(!"todo");
            } else if (IR::Unop *u = s->source->asUnop()) {
                if (IR::Temp *e = u->expr->asTemp()) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, t);
                    loadTempAddress(AMD64_RDX, e);
                    void (*op)(Context *, Value *, const Value *) = 0;
                    switch (u->op) {
                    case QQmlJS::IR::OpIfTrue: assert(!"unreachable"); break;
                    case QQmlJS::IR::OpNot: op = __qmljs_not; break;
                    case QQmlJS::IR::OpUMinus: op = __qmljs_uminus; break;
                    case QQmlJS::IR::OpUPlus: op = __qmljs_uplus; break;
                    case QQmlJS::IR::OpCompl: op = __qmljs_compl; break;
                    default: assert(!"unreachable"); break;
                    } // switch
                    amd64_call_code(_codePtr, op);
                    return;
                }

            } else if (IR::Binop *b = s->source->asBinop()) {
                IR::Temp *l = b->left->asTemp();
                IR::Temp *r = b->right->asTemp();
                if (l && r) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, t);
                    loadTempAddress(AMD64_RDX, l);
                    loadTempAddress(AMD64_RCX, r);

                    uchar *label1 = 0, *label2 = 0, *label3 = 0;

                    if (b->op == IR::OpMul || b->op == IR::OpAdd || b->op == IR::OpSub || b->op == IR::OpDiv) {
                        amd64_alu_membase_imm_size(_codePtr, X86_CMP, AMD64_RDX, 0, NUMBER_TYPE, 4);
                        label1 = _codePtr;
                        amd64_branch8(_codePtr, X86_CC_NE, 0, 0);
                        amd64_alu_membase_imm_size(_codePtr, X86_CMP, AMD64_RCX, 0, NUMBER_TYPE, 4);
                        label2 = _codePtr;
                        amd64_branch8(_codePtr, X86_CC_NE, 0, 0);
                        amd64_movsd_reg_membase(_codePtr, X86_XMM0, AMD64_RDX, offsetof(Value, numberValue));
                        amd64_movsd_reg_membase(_codePtr, X86_XMM1, AMD64_RCX, offsetof(Value, numberValue));
                        switch (b->op) {
                        case IR::OpAdd:
                            amd64_sse_addsd_reg_reg(_codePtr, X86_XMM0, X86_XMM1);
                            break;
                        case IR::OpSub:
                            amd64_sse_subsd_reg_reg(_codePtr, X86_XMM0, X86_XMM1);
                            break;
                        case IR::OpMul:
                            amd64_sse_mulsd_reg_reg(_codePtr, X86_XMM0, X86_XMM1);
                            break;
                        case IR::OpDiv:
                            amd64_sse_divsd_reg_reg(_codePtr, X86_XMM0, X86_XMM1);
                            break;
                        default:
                            Q_UNREACHABLE();
                        } // switch

                        amd64_mov_membase_imm(_codePtr, AMD64_RSI, 0, NUMBER_TYPE, 4);
                        amd64_movsd_membase_reg(_codePtr, AMD64_RSI, offsetof(Value, numberValue), X86_XMM0);
                        label3 = _codePtr;
                        amd64_jump32(_codePtr, 0);
                    }

                    if (label1 && label2) {
                        amd64_patch(label1, _codePtr);
                        amd64_patch(label2, _codePtr);
                    }

                    void (*op)(Context *, Value *, const Value *, const Value *) = 0;

                    switch (b->op) {
                    case QQmlJS::IR::OpInvalid:
                    case QQmlJS::IR::OpIfTrue:
                    case QQmlJS::IR::OpNot:
                    case QQmlJS::IR::OpUMinus:
                    case QQmlJS::IR::OpUPlus:
                    case QQmlJS::IR::OpCompl:
                        assert(!"unreachable");
                        break;

                    case QQmlJS::IR::OpBitAnd: op = __qmljs_bit_and; break;
                    case QQmlJS::IR::OpBitOr: op = __qmljs_bit_or; break;
                    case QQmlJS::IR::OpBitXor: op = __qmljs_bit_xor; break;
                    case QQmlJS::IR::OpAdd: op = __qmljs_add; break;
                    case QQmlJS::IR::OpSub: op = __qmljs_sub; break;
                    case QQmlJS::IR::OpMul: op = __qmljs_mul; break;
                    case QQmlJS::IR::OpDiv: op = __qmljs_div; break;
                    case QQmlJS::IR::OpMod: op = __qmljs_mod; break;
                    case QQmlJS::IR::OpLShift: op = __qmljs_shl; break;
                    case QQmlJS::IR::OpRShift: op = __qmljs_shr; break;
                    case QQmlJS::IR::OpURShift: op = __qmljs_ushr; break;
                    case QQmlJS::IR::OpGt: op = __qmljs_gt; break;
                    case QQmlJS::IR::OpLt: op = __qmljs_lt; break;
                    case QQmlJS::IR::OpGe: op = __qmljs_ge; break;
                    case QQmlJS::IR::OpLe: op = __qmljs_le; break;
                    case QQmlJS::IR::OpEqual: op = __qmljs_eq; break;
                    case QQmlJS::IR::OpNotEqual: op = __qmljs_ne; break;
                    case QQmlJS::IR::OpStrictEqual: op = __qmljs_se; break;
                    case QQmlJS::IR::OpStrictNotEqual: op = __qmljs_sne; break;

                    case QQmlJS::IR::OpAnd:
                    case QQmlJS::IR::OpOr:
                        assert(!"unreachable");
                        break;
                    }
                    amd64_call_code(_codePtr, op);
                    if (label3)
                        amd64_patch(label3, _codePtr);
                    return;
                }
            } else if (IR::Call *c = s->source->asCall()) {
                if (c->base->asName()) {
                    callActivationProperty(c, t);
                    return;
                } else if (c->base->asMember()) {
                    callProperty(c, t);
                    return;
                } else if (c->base->asTemp()) {
                    callValue(c, t);
                    return;
                }
            }
        } else if (IR::Member *m = s->target->asMember()) {
            if (IR::Temp *base = m->base->asTemp()) {
                if (IR::Const *c = s->source->asConst()) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, base);
                    amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*m->name));
                    amd64_mov_reg_imm(_codePtr, AMD64_RAX, &c->value);
                    amd64_movsd_reg_regp(_codePtr, X86_XMM0, AMD64_RAX);
                    amd64_call_code(_codePtr, __qmljs_set_property_number);
                    return;
                } else if (IR::String *str = s->source->asString()) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, base);
                    amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*m->name));
                    amd64_mov_reg_imm(_codePtr, AMD64_RCX, _engine->newString(*str->value));
                    amd64_call_code(_codePtr, __qmljs_set_property_string);
                    return;
                } else if (IR::Temp *t = s->source->asTemp()) {
                    // __qmljs_set_property(ctx, object, name, value);
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, base);
                    amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*m->name));
                    loadTempAddress(AMD64_RCX, t);
                    amd64_call_code(_codePtr, __qmljs_set_property);
                    return;
                } else if (IR::Closure *clos = s->source->asClosure()) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, base);
                    amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*m->name));
                    amd64_mov_reg_imm(_codePtr, AMD64_RCX, clos->value);
                    amd64_call_code(_codePtr, __qmljs_set_property_closure);
                    return;
                }
            }
        }
    } else {
        // inplace assignment, e.g. x += 1, ++x, ...
    }

    Q_UNIMPLEMENTED();
    s->dump(qout, IR::Stmt::MIR);
    qout << endl;
    assert(!"TODO");
}

void InstructionSelection::visitJump(IR::Jump *s)
{
    if (_block->index + 1 != s->target->index) {
        _patches[s->target].append(_codePtr);
        amd64_jump32(_codePtr, 0);
    }
}

void InstructionSelection::visitCJump(IR::CJump *s)
{
    if (IR::Temp *t = s->cond->asTemp()) {
        amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
        loadTempAddress(AMD64_RSI, t);

        amd64_mov_reg_membase(_codePtr, AMD64_RAX, AMD64_RSI, 0, 4);
        amd64_alu_reg_imm(_codePtr, X86_CMP, AMD64_RAX, BOOLEAN_TYPE);

        uchar *label1 = _codePtr;
        amd64_branch32(_codePtr, X86_CC_NE, 0, 0);

        amd64_mov_reg_membase(_codePtr, AMD64_RAX, AMD64_RSI, offsetof(Value, booleanValue), 1);

        uchar *label2 = _codePtr;
        amd64_jump32(_codePtr, 0);

        amd64_patch(label1, _codePtr);
        amd64_call_code(_codePtr, __qmljs_to_boolean);

        amd64_patch(label2, _codePtr);
        amd64_alu_reg_imm_size(_codePtr, X86_CMP, AMD64_RAX, 0, 4);
        _patches[s->iftrue].append(_codePtr);
        amd64_branch32(_codePtr, X86_CC_NZ, 0, 1);

        if (_block->index + 1 != s->iffalse->index) {
            _patches[s->iffalse].append(_codePtr);
            amd64_jump32(_codePtr, 0);
        }
        return;
    }
    Q_UNIMPLEMENTED();
    assert(!"TODO");
}

void InstructionSelection::visitRet(IR::Ret *s)
{
    if (IR::Temp *t = s->expr->asTemp()) {
        amd64_lea_membase(_codePtr, AMD64_RDI, AMD64_R14, offsetof(Context, result));
        loadTempAddress(AMD64_RSI, t);
        amd64_call_code(_codePtr, __qmljs_copy);
        return;
    }
    Q_UNIMPLEMENTED();
    Q_UNUSED(s);
}

