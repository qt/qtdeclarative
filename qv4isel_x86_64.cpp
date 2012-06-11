
#include "qv4isel_x86_64_p.h"
#include "qmljs_runtime.h"
#include "qmljs_objects.h"

#define TARGET_AMD64
#define g_assert assert
typedef quint64 guint64;
typedef qint64 gint64;
typedef uchar guint8;
typedef uint guint32;
typedef void *gpointer;
#include "asm/amd64-codegen.h"

#include <sys/mman.h>
#include <iostream>
#include <cassert>

#ifndef NO_UDIS86
#  include <udis86.h>
#endif

#define qmljs_call_code(ptr, code) \
    do { \
        amd64_mov_reg_imm(ptr, AMD64_RAX, code); \
        amd64_call_reg(ptr, AMD64_RAX); \
    } while (0)

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
    _patches.clear();
    _addrs.clear();

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
        IR::BasicBlock *block = it.key();
        uchar *target = _addrs.value(block);
        assert(target != 0);
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
        qmljs_call_code(_codePtr, __qmljs_copy);
    }

    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8); // load the context

    if (result)
        loadTempAddress(AMD64_RSI, result);
    else
        amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_RSI, AMD64_RSI);

    if (baseName->id) {
        amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*baseName->id));
        amd64_lea_membase(_codePtr, AMD64_RCX, AMD64_RSP, 0);
        amd64_mov_reg_imm(_codePtr, AMD64_R8, argc);
        qmljs_call_code(_codePtr, __qmljs_call_activation_property);
    } else {
        switch (baseName->builtin) {
        case IR::Name::builtin_invalid:
            Q_UNREACHABLE();
            break;
        case IR::Name::builtin_typeof:
            amd64_lea_membase(_codePtr, AMD64_RDX, AMD64_RSP, 0);
            amd64_mov_reg_imm(_codePtr, AMD64_RCX, argc);
            qmljs_call_code(_codePtr, __qmljs_builtin_typeof);
            break;
        case IR::Name::builtin_delete:
            Q_UNREACHABLE();
            break;
        case IR::Name::builtin_throw:
            amd64_lea_membase(_codePtr, AMD64_RDX, AMD64_RSP, 0);
            amd64_mov_reg_imm(_codePtr, AMD64_RCX, argc);
            qmljs_call_code(_codePtr, __qmljs_builtin_throw);
            break;
        case IR::Name::builtin_rethrow:
            amd64_lea_membase(_codePtr, AMD64_RDX, AMD64_RSP, 0);
            amd64_mov_reg_imm(_codePtr, AMD64_RCX, argc);
            qmljs_call_code(_codePtr, __qmljs_builtin_rethrow);
            return; // we need to return to avoid checking the exceptions
        }
    }

    checkExceptions();
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
        qmljs_call_code(_codePtr, __qmljs_copy);
    }

    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8); // load the context

    if (result)
        loadTempAddress(AMD64_RSI, result);
    else
        amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_RSI, AMD64_RSI);

    amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_RDX, AMD64_RDX);
    loadTempAddress(AMD64_RCX, baseTemp);
    amd64_lea_membase(_codePtr, AMD64_R8, AMD64_RSP, 0);
    amd64_mov_reg_imm(_codePtr, AMD64_R9, argc);
    qmljs_call_code(_codePtr, __qmljs_call_value);

    checkExceptions();
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
        qmljs_call_code(_codePtr, __qmljs_copy);
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
    qmljs_call_code(_codePtr, __qmljs_call_property);

    checkExceptions();
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
        qmljs_call_code(_codePtr, __qmljs_copy);
    }

    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8); // load the context

    if (result)
        loadTempAddress(AMD64_RSI, result);
    else
        amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_RSI, AMD64_RSI);

    amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*baseName->id));
    amd64_lea_membase(_codePtr, AMD64_RCX, AMD64_RSP, 0);
    amd64_mov_reg_imm(_codePtr, AMD64_R8, argc);
    qmljs_call_code(_codePtr, __qmljs_construct_activation_property);

    checkExceptions();
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
        qmljs_call_code(_codePtr, __qmljs_copy);
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
    qmljs_call_code(_codePtr, __qmljs_construct_property);

    checkExceptions();
}

void InstructionSelection::constructValue(IR::New *call, IR::Temp *result)
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
        qmljs_call_code(_codePtr, __qmljs_copy);
    }

    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8); // load the context

    if (result)
        loadTempAddress(AMD64_RSI, result);
    else
        amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_RSI, AMD64_RSI);

    loadTempAddress(AMD64_RDX, baseTemp);
    amd64_lea_membase(_codePtr, AMD64_RCX, AMD64_RSP, 0);
    amd64_mov_reg_imm(_codePtr, AMD64_R8, argc);
    qmljs_call_code(_codePtr, __qmljs_construct_value);
}

void InstructionSelection::checkExceptions()
{
    amd64_mov_reg_membase(_codePtr, AMD64_RAX, AMD64_R14, offsetof(Context, hasUncaughtException), 4);
    amd64_alu_reg_imm_size(_codePtr, X86_CMP, AMD64_RAX, 1, 1);
    _patches[_function->handlersBlock].append(_codePtr);
    x86_branch32(_codePtr, X86_CC_E, 0, 1);
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
                    qmljs_call_code(_codePtr, __qmljs_set_activation_property_boolean);
                    break;

                case IR::NumberType:
                    amd64_mov_reg_imm(_codePtr, AMD64_RAX, &c->value);
                    amd64_movsd_reg_regp(_codePtr, AMD64_XMM0, AMD64_RAX);
                    qmljs_call_code(_codePtr, __qmljs_set_activation_property_number);
                    break;

                default:
                    Q_UNIMPLEMENTED();
                    assert(!"TODO");
                }
            } else if (IR::String *str = s->source->asString()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, propertyName);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, _engine->newString(*str->value));
                qmljs_call_code(_codePtr, __qmljs_set_activation_property_string);
            } else if (IR::Temp *t = s->source->asTemp()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, propertyName);
                loadTempAddress(AMD64_RDX, t);
                qmljs_call_code(_codePtr, __qmljs_set_activation_property);
            } else if (IR::Name *other = s->source->asName()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, propertyName);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*other->id));
                qmljs_call_code(_codePtr, __qmljs_copy_activation_property);
            } else if (IR::Closure *clos = s->source->asClosure()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, propertyName);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, clos->value);
                qmljs_call_code(_codePtr, __qmljs_set_activation_property_closure);
            } else {
                Q_UNIMPLEMENTED();
                assert(!"TODO");
            }

            checkExceptions();
            return;
        } else if (IR::Temp *t = s->target->asTemp()) {
            if (IR::Name *n = s->source->asName()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                loadTempAddress(AMD64_RSI, t);
                if (*n->id == QStringLiteral("this")) { // ### `this' should be a builtin.
                    qmljs_call_code(_codePtr, __qmljs_get_thisObject);
                } else {
                    String *propertyName = identifier(*n->id);
                    amd64_mov_reg_imm(_codePtr, AMD64_RDX, propertyName);
                    qmljs_call_code(_codePtr, __qmljs_get_activation_property);
                    checkExceptions();
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
                    amd64_movsd_reg_regp(_codePtr, AMD64_XMM0, AMD64_RAX);
                    amd64_mov_membase_imm(_codePtr, AMD64_RSI, 0, NUMBER_TYPE, 4);
                    amd64_movsd_membase_reg(_codePtr, AMD64_RSI, offsetof(Value, numberValue), AMD64_XMM0);
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
                qmljs_call_code(_codePtr, __qmljs_init_string);
                return;
            } else if (IR::Closure *clos = s->source->asClosure()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                loadTempAddress(AMD64_RSI, t);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, clos->value);
                qmljs_call_code(_codePtr, __qmljs_init_closure);
                return;
            } else if (IR::New *ctor = s->source->asNew()) {
                if (ctor->base->asName()) {
                    constructActivationProperty(ctor, t);
                    return;
                } else if (ctor->base->asMember()) {
                    constructProperty(ctor, t);
                    return;
                } else if (ctor->base->asTemp()) {
                    constructValue(ctor, t);
                    return;
                }
            } else if (IR::Member *m = s->source->asMember()) {
                //__qmljs_get_property(ctx, result, object, name);
                if (IR::Temp *base = m->base->asTemp()) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, t);
                    loadTempAddress(AMD64_RDX, base);
                    amd64_mov_reg_imm(_codePtr, AMD64_RCX, identifier(*m->name));
                    qmljs_call_code(_codePtr, __qmljs_get_property);
                    checkExceptions();
                    return;
                }
                assert(!"wip");
                return;
            } else if (IR::Subscript *ss = s->source->asSubscript()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                loadTempAddress(AMD64_RSI, t);
                loadTempAddress(AMD64_RDX, ss->base->asTemp());
                loadTempAddress(AMD64_RCX, ss->index->asTemp());
                qmljs_call_code(_codePtr, __qmljs_get_element);
                checkExceptions();
                return;
            } else if (IR::Unop *u = s->source->asUnop()) {
                if (IR::Temp *e = u->expr->asTemp()) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, t);
                    loadTempAddress(AMD64_RDX, e);
                    void (*op)(Context *, Value *, const Value *) = 0;
                    switch (u->op) {
                    case IR::OpIfTrue: assert(!"unreachable"); break;
                    case IR::OpNot: op = __qmljs_not; break;
                    case IR::OpUMinus: op = __qmljs_uminus; break;
                    case IR::OpUPlus: op = __qmljs_uplus; break;
                    case IR::OpCompl: op = __qmljs_compl; break;
                    default: assert(!"unreachable"); break;
                    } // switch
                    qmljs_call_code(_codePtr, op);
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
                        x86_branch8(_codePtr, X86_CC_NE, 0, 0);
                        amd64_alu_membase_imm_size(_codePtr, X86_CMP, AMD64_RCX, 0, NUMBER_TYPE, 4);
                        label2 = _codePtr;
                        x86_branch8(_codePtr, X86_CC_NE, 0, 0);
                        amd64_movsd_reg_membase(_codePtr, AMD64_XMM0, AMD64_RDX, offsetof(Value, numberValue));
                        amd64_movsd_reg_membase(_codePtr, AMD64_XMM1, AMD64_RCX, offsetof(Value, numberValue));
                        switch (b->op) {
                        case IR::OpAdd:
                            amd64_sse_addsd_reg_reg(_codePtr, AMD64_XMM0, AMD64_XMM1);
                            break;
                        case IR::OpSub:
                            amd64_sse_subsd_reg_reg(_codePtr, AMD64_XMM0, AMD64_XMM1);
                            break;
                        case IR::OpMul:
                            amd64_sse_mulsd_reg_reg(_codePtr, AMD64_XMM0, AMD64_XMM1);
                            break;
                        case IR::OpDiv:
                            amd64_sse_divsd_reg_reg(_codePtr, AMD64_XMM0, AMD64_XMM1);
                            break;
                        default:
                            Q_UNREACHABLE();
                        } // switch

                        amd64_mov_membase_imm(_codePtr, AMD64_RSI, 0, NUMBER_TYPE, 4);
                        amd64_movsd_membase_reg(_codePtr, AMD64_RSI, offsetof(Value, numberValue), AMD64_XMM0);
                        label3 = _codePtr;
                        x86_jump32(_codePtr, 0);
                    }


                    if (label1 && label2) {
                        amd64_patch(label1, _codePtr);
                        amd64_patch(label2, _codePtr);
                    }

                    void (*op)(Context *, Value *, const Value *, const Value *) = 0;

                    switch ((IR::AluOp) b->op) {
                    case IR::OpInvalid:
                    case IR::OpIfTrue:
                    case IR::OpNot:
                    case IR::OpUMinus:
                    case IR::OpUPlus:
                    case IR::OpCompl:
                        assert(!"unreachable");
                        break;

                    case IR::OpBitAnd: op = __qmljs_bit_and; break;
                    case IR::OpBitOr: op = __qmljs_bit_or; break;
                    case IR::OpBitXor: op = __qmljs_bit_xor; break;
                    case IR::OpAdd: op = __qmljs_add; break;
                    case IR::OpSub: op = __qmljs_sub; break;
                    case IR::OpMul: op = __qmljs_mul; break;
                    case IR::OpDiv: op = __qmljs_div; break;
                    case IR::OpMod: op = __qmljs_mod; break;
                    case IR::OpLShift: op = __qmljs_shl; break;
                    case IR::OpRShift: op = __qmljs_shr; break;
                    case IR::OpURShift: op = __qmljs_ushr; break;
                    case IR::OpGt: op = __qmljs_gt; break;
                    case IR::OpLt: op = __qmljs_lt; break;
                    case IR::OpGe: op = __qmljs_ge; break;
                    case IR::OpLe: op = __qmljs_le; break;
                    case IR::OpEqual: op = __qmljs_eq; break;
                    case IR::OpNotEqual: op = __qmljs_ne; break;
                    case IR::OpStrictEqual: op = __qmljs_se; break;
                    case IR::OpStrictNotEqual: op = __qmljs_sne; break;
                    case IR::OpInstanceof: op = __qmljs_instanceof; break;
                    case IR::OpIn: op = __qmljs_in; break;

                    case IR::OpAnd:
                    case IR::OpOr:
                        assert(!"unreachable");
                        break;
                    }
                    qmljs_call_code(_codePtr, op);
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
                    amd64_movsd_reg_regp(_codePtr, AMD64_XMM0, AMD64_RAX);
                    qmljs_call_code(_codePtr, __qmljs_set_property_number);
                } else if (IR::String *str = s->source->asString()) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, base);
                    amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*m->name));
                    amd64_mov_reg_imm(_codePtr, AMD64_RCX, _engine->newString(*str->value));
                    qmljs_call_code(_codePtr, __qmljs_set_property_string);
                } else if (IR::Temp *t = s->source->asTemp()) {
                    // __qmljs_set_property(ctx, object, name, value);
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, base);
                    amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*m->name));
                    loadTempAddress(AMD64_RCX, t);
                    qmljs_call_code(_codePtr, __qmljs_set_property);
                } else if (IR::Closure *clos = s->source->asClosure()) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, base);
                    amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*m->name));
                    amd64_mov_reg_imm(_codePtr, AMD64_RCX, clos->value);
                    qmljs_call_code(_codePtr, __qmljs_set_property_closure);
                } else {
                    Q_UNIMPLEMENTED();
                    assert(!"TODO");
                }
                checkExceptions();
                return;
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (IR::Temp *t2 = s->source->asTemp()) {
                loadTempAddress(AMD64_RSI, ss->base->asTemp());
                loadTempAddress(AMD64_RDX, ss->index->asTemp());
                loadTempAddress(AMD64_RCX, t2);
                qmljs_call_code(_codePtr, __qmljs_set_element);
            } else if (IR::Const *c = s->source->asConst()) {
                if (c->type == IR::NumberType) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    loadTempAddress(AMD64_RSI, ss->base->asTemp());
                    loadTempAddress(AMD64_RDX, ss->index->asTemp());
                    amd64_mov_reg_imm(_codePtr, AMD64_RAX, &c->value);
                    amd64_movsd_reg_regp(_codePtr, AMD64_XMM0, AMD64_RAX);
                    qmljs_call_code(_codePtr, __qmljs_set_element_number);
                } else {
                    s->dump(qout, IR::Stmt::MIR);
                    qout << endl;
                    Q_UNIMPLEMENTED();
                    assert(!"TODO");
                }
            } else {
                Q_UNIMPLEMENTED();
                assert(!"TODO");
            }
            checkExceptions();
            return;
        }
    } else {
        // inplace assignment, e.g. x += 1, ++x, ...
        if (IR::Temp *t = s->target->asTemp()) {
            if (IR::Temp *t2 = s->source->asTemp()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                loadTempAddress(AMD64_RSI, t);
                amd64_mov_reg_reg(_codePtr, AMD64_RDX, AMD64_RSI, 8);
                loadTempAddress(AMD64_RCX, t2);
                void (*op)(Context *, Value *, const Value *, const Value *) = 0;
                switch (s->op) {
                case IR::OpBitAnd: op = __qmljs_bit_and; break;
                case IR::OpBitOr: op = __qmljs_bit_or; break;
                case IR::OpBitXor: op = __qmljs_bit_xor; break;
                case IR::OpAdd: op = __qmljs_add; break;
                case IR::OpSub: op = __qmljs_sub; break;
                case IR::OpMul: op = __qmljs_mul; break;
                case IR::OpDiv: op = __qmljs_div; break;
                case IR::OpMod: op = __qmljs_mod; break;
                case IR::OpLShift: op = __qmljs_shl; break;
                case IR::OpRShift: op = __qmljs_shr; break;
                case IR::OpURShift: op = __qmljs_ushr; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                qmljs_call_code(_codePtr, op);
                return;
            }
        } else if (IR::Name *n = s->target->asName()) {
            if (IR::Temp *t = s->source->asTemp()) {
                void (*op)(Context *, String *, Value *) = 0;
                switch (s->op) {
                case IR::OpBitAnd: op = __qmljs_inplace_bit_and_name; break;
                case IR::OpBitOr: op = __qmljs_inplace_bit_or_name; break;
                case IR::OpBitXor: op = __qmljs_inplace_bit_xor_name; break;
                case IR::OpAdd: op = __qmljs_inplace_add_name; break;
                case IR::OpSub: op = __qmljs_inplace_sub_name; break;
                case IR::OpMul: op = __qmljs_inplace_mul_name; break;
                case IR::OpDiv: op = __qmljs_inplace_div_name; break;
                case IR::OpMod: op = __qmljs_inplace_mod_name; break;
                case IR::OpLShift: op = __qmljs_inplace_shl_name; break;
                case IR::OpRShift: op = __qmljs_inplace_shr_name; break;
                case IR::OpURShift: op = __qmljs_inplace_ushr_name; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, identifier(*n->id));
                loadTempAddress(AMD64_RDX, t);
                qmljs_call_code(_codePtr, op);
                checkExceptions();
                return;
            }
        } else if (IR::Subscript *ss = s->target->asSubscript()) {
            if (IR::Temp *t = s->source->asTemp()) {
                void (*op)(Context *, Value *, Value *, Value *) = 0;
                switch (s->op) {
                case IR::OpBitAnd: op = __qmljs_inplace_bit_and_element; break;
                case IR::OpBitOr: op = __qmljs_inplace_bit_or_element; break;
                case IR::OpBitXor: op = __qmljs_inplace_bit_xor_element; break;
                case IR::OpAdd: op = __qmljs_inplace_add_element; break;
                case IR::OpSub: op = __qmljs_inplace_sub_element; break;
                case IR::OpMul: op = __qmljs_inplace_mul_element; break;
                case IR::OpDiv: op = __qmljs_inplace_div_element; break;
                case IR::OpMod: op = __qmljs_inplace_mod_element; break;
                case IR::OpLShift: op = __qmljs_inplace_shl_element; break;
                case IR::OpRShift: op = __qmljs_inplace_shr_element; break;
                case IR::OpURShift: op = __qmljs_inplace_ushr_element; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                loadTempAddress(AMD64_RSI, ss->base->asTemp());
                loadTempAddress(AMD64_RDX, ss->index->asTemp());
                loadTempAddress(AMD64_RCX, t);
                qmljs_call_code(_codePtr, op);
                checkExceptions();
                return;
            }
        } else if (IR::Member *m = s->target->asMember()) {
            if (IR::Temp *t = s->source->asTemp()) {
                void (*op)(Context *, Value *, String *, Value *) = 0;
                switch (s->op) {
                case IR::OpBitAnd: op = __qmljs_inplace_bit_and_member; break;
                case IR::OpBitOr: op = __qmljs_inplace_bit_or_member; break;
                case IR::OpBitXor: op = __qmljs_inplace_bit_xor_member; break;
                case IR::OpAdd: op = __qmljs_inplace_add_member; break;
                case IR::OpSub: op = __qmljs_inplace_sub_member; break;
                case IR::OpMul: op = __qmljs_inplace_mul_member; break;
                case IR::OpDiv: op = __qmljs_inplace_div_member; break;
                case IR::OpMod: op = __qmljs_inplace_mod_member; break;
                case IR::OpLShift: op = __qmljs_inplace_shl_member; break;
                case IR::OpRShift: op = __qmljs_inplace_shr_member; break;
                case IR::OpURShift: op = __qmljs_inplace_ushr_member; break;
                default:
                    Q_UNREACHABLE();
                    break;
                }

                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                loadTempAddress(AMD64_RSI, m->base->asTemp());
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, identifier(*m->name));
                loadTempAddress(AMD64_RCX, t);
                qmljs_call_code(_codePtr, op);
                checkExceptions();
                return;
            }
        }
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
        x86_jump32(_codePtr, 0);
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
        x86_branch8(_codePtr, X86_CC_NE, 0, 0);

        amd64_mov_reg_membase(_codePtr, AMD64_RAX, AMD64_RSI, offsetof(Value, booleanValue), 1);

        uchar *label2 = _codePtr;
        x86_jump8(_codePtr, 0);

        amd64_patch(label1, _codePtr);
        qmljs_call_code(_codePtr, __qmljs_to_boolean);

        amd64_patch(label2, _codePtr);
        amd64_mov_reg_imm_size(_codePtr, AMD64_RDX, 1, 1);
        amd64_alu_reg8_reg8(_codePtr, X86_CMP, AMD64_RAX, AMD64_RDX, 0, 0);
        _patches[s->iftrue].append(_codePtr);
        x86_branch32(_codePtr, X86_CC_E, 0, 1);

        if (_block->index + 1 != s->iffalse->index) {
            _patches[s->iffalse].append(_codePtr);
            x86_jump32(_codePtr, 0);
        }
        return;
    } else if (IR::Binop *b = s->cond->asBinop()) {
        IR::Temp *l = b->left->asTemp();
        IR::Temp *r = b->right->asTemp();
        if (l && r) {
            loadTempAddress(AMD64_RSI, l);
            loadTempAddress(AMD64_RDX, r);

            uchar *label1 = 0, *label2 = 0, *label3 = 0;
            if (b->op != IR::OpInstanceof && b->op != IR::OpIn) {
                amd64_alu_membase_imm_size(_codePtr, X86_CMP, AMD64_RSI, 0, NUMBER_TYPE, 4);
                label1 = _codePtr;
                x86_branch8(_codePtr, X86_CC_NE, 0, 0);
                amd64_alu_membase_imm_size(_codePtr, X86_CMP, AMD64_RDX, 0, NUMBER_TYPE, 4);
                label2 = _codePtr;
                x86_branch8(_codePtr, X86_CC_NE, 0, 0);
                amd64_movsd_reg_membase(_codePtr, AMD64_XMM0, AMD64_RSI, offsetof(Value, numberValue));
                amd64_movsd_reg_membase(_codePtr, AMD64_XMM1, AMD64_RDX, offsetof(Value, numberValue));

                int op;
                switch (b->op) {
                default: Q_UNREACHABLE(); assert(!"todo"); break;
                case IR::OpGt: op = X86_CC_GT; break;
                case IR::OpLt: op = X86_CC_LT; break;
                case IR::OpGe: op = X86_CC_GE; break;
                case IR::OpLe: op = X86_CC_LE; break;
                case IR::OpEqual: op = X86_CC_EQ; break;
                case IR::OpNotEqual: op = X86_CC_NE; break;
                case IR::OpStrictEqual: op = X86_CC_EQ; break;
                case IR::OpStrictNotEqual: op = X86_CC_NE; break;
                }

                amd64_sse_ucomisd_reg_reg(_codePtr, AMD64_XMM0, AMD64_XMM1);
                amd64_set_reg_size(_codePtr, op, AMD64_RAX, 0, 1);

                label3 = _codePtr;
                x86_jump32(_codePtr, 0);
            }

            if (label1 && label2) {
                amd64_patch(label1, _codePtr);
                amd64_patch(label2, _codePtr);
            }

            amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);

            bool (*op)(Context *, const Value *, const Value *);
            switch (b->op) {
            default: Q_UNREACHABLE(); assert(!"todo"); break;
            case IR::OpGt: op = __qmljs_cmp_gt; break;
            case IR::OpLt: op = __qmljs_cmp_lt; break;
            case IR::OpGe: op = __qmljs_cmp_ge; break;
            case IR::OpLe: op = __qmljs_cmp_le; break;
            case IR::OpEqual: op = __qmljs_cmp_eq; break;
            case IR::OpNotEqual: op = __qmljs_cmp_ne; break;
            case IR::OpStrictEqual: op = __qmljs_cmp_se; break;
            case IR::OpStrictNotEqual: op = __qmljs_cmp_sne; break;
            case IR::OpInstanceof: op = __qmljs_cmp_instanceof; break;
            case IR::OpIn: op = __qmljs_cmp_in; break;
            } // switch

            qmljs_call_code(_codePtr, op);

            if (label3)
                amd64_patch(label3, _codePtr);

            x86_mov_reg_imm(_codePtr, X86_EDX, 1);
            x86_alu_reg8_reg8(_codePtr, X86_CMP, X86_EAX, X86_EDX, 0, 0);

            _patches[s->iftrue].append(_codePtr);
            x86_branch32(_codePtr, X86_CC_E, 0, 1);

            if (_block->index + 1 != s->iffalse->index) {
                _patches[s->iffalse].append(_codePtr);
                x86_jump32(_codePtr, 0);
            }

            return;
        } else {
            assert(!"wip");
        }
    }

    Q_UNIMPLEMENTED();
    assert(!"TODO");
}

void InstructionSelection::visitRet(IR::Ret *s)
{
    if (IR::Temp *t = s->expr->asTemp()) {
        amd64_lea_membase(_codePtr, AMD64_RDI, AMD64_R14, offsetof(Context, result));
        loadTempAddress(AMD64_RSI, t);
        qmljs_call_code(_codePtr, __qmljs_copy);
        return;
    }
    Q_UNIMPLEMENTED();
    Q_UNUSED(s);
}

