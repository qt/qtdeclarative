
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
InstructionSelection::InstructionSelection(IR::Module *module, uchar *buffer)
    : _module(module)
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
    _code = _codePtr;
    _code = (uchar *) ((size_t(_code) + 15) & ~15);
    function->code = (void (*)(VM::Context *)) _code;
    _codePtr = _code;

    int locals = function->tempCount * sizeof(Value);
    locals = (locals + 15) & ~15;

    amd64_push_reg(_codePtr, AMD64_RBP);
    amd64_push_reg(_codePtr, AMD64_R14);
    amd64_push_reg(_codePtr, AMD64_R15);

    amd64_mov_reg_reg(_codePtr, AMD64_RBP, AMD64_RSP, 8);
    amd64_mov_reg_reg(_codePtr, AMD64_R14, AMD64_RDI, 8);
    amd64_alu_reg_imm(_codePtr, X86_SUB, AMD64_RSP, locals);

    amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_R15, AMD64_R15);

    foreach (IR::BasicBlock *block, function->basicBlocks) {
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
}

String *InstructionSelection::identifier(const QString &s)
{
    String *&id = _identifiers[s];
    if (! id)
        id = new String(s);
    return id;
}

int InstructionSelection::tempOffset(IR::Temp *t)
{
    return sizeof(Value) * (t->index - 1);
}

void InstructionSelection::loadTempAddress(int reg, IR::Temp *t)
{
    amd64_lea_membase(_codePtr, reg, AMD64_RSP, sizeof(Value) * (t->index - 1));
}

void InstructionSelection::callActivationProperty(IR::Call *call, IR::Temp *result)
{
    int argc = 0;
    for (IR::ExprList *it = call->args; it; it = it->next)
        ++argc;

    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
    amd64_alu_reg_reg(_codePtr, X86_XOR, AMD64_RSI, AMD64_RSI);
    amd64_mov_reg_imm(_codePtr, AMD64_RDX, argc);
    amd64_call_code(_codePtr, __qmljs_new_context);

    amd64_mov_reg_reg(_codePtr, AMD64_R15, AMD64_RAX, 8);

    argc = 0;
    for (IR::ExprList *it = call->args; it; it = it->next) {
        IR::Temp *t = it->expr->asTemp();
        Q_ASSERT(t != 0);
        amd64_mov_reg_membase(_codePtr, AMD64_RAX, AMD64_R15, offsetof(Context, arguments), 8);
        amd64_lea_membase(_codePtr, AMD64_RDI, AMD64_RAX, argc * sizeof(Value));
        loadTempAddress(AMD64_RSI, t);
        amd64_call_code(_codePtr, __qmljs_copy);
        ++argc;
    }

    String *id = identifier(*call->base->asName()->id);
    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R15, 8);
    loadTempAddress(AMD64_RSI, result);
    amd64_mov_reg_imm(_codePtr, AMD64_RDX, id);
    amd64_call_code(_codePtr, __qmljs_call_activation_property);
    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R15, 8);
    amd64_call_code(_codePtr, __qmljs_dispose_context);
}

void InstructionSelection::visitExp(IR::Exp *)
{
//    Q_UNIMPLEMENTED();
//    assert(!"TODO");
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
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, new String(*str->value));
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
            }
        } else if (IR::Temp *t = s->target->asTemp()) {
            if (IR::Name *n = s->source->asName()) {
                String *propertyName = identifier(*n->id);
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                loadTempAddress(AMD64_RSI, t);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, propertyName);
                amd64_call_code(_codePtr, __qmljs_get_activation_property);
                return;
            } else if (IR::Const *c = s->source->asConst()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                loadTempAddress(AMD64_RSI, t);

                switch (c->type) {
                case IR::BoolType:
                    amd64_mov_reg_imm(_codePtr, AMD64_RDX, c->value != 0);
                    amd64_call_code(_codePtr, __qmljs_init_boolean);
                    break;

                case IR::NumberType:
                    amd64_mov_reg_imm(_codePtr, AMD64_RAX, &c->value);
                    amd64_movsd_reg_regp(_codePtr, X86_XMM0, AMD64_RAX);
                    amd64_call_code(_codePtr, __qmljs_init_number);
                    break;

                default:
                    Q_UNIMPLEMENTED();
                    assert(!"TODO");
                }
                return;
            } else if (IR::Temp *t2 = s->source->asTemp()) {
                loadTempAddress(AMD64_RDI, t);
                loadTempAddress(AMD64_RSI, t2);
                amd64_call_code(_codePtr, __qmljs_copy);
                return;
            } else if (IR::String *str = s->source->asString()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                loadTempAddress(AMD64_RSI, t);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, new String(*str->value));
                amd64_call_code(_codePtr, __qmljs_init_string);
                return;
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
                    return;
                }
            } else if (IR::Call *c = s->source->asCall()) {
                if (c->base->asName()) {
                    callActivationProperty(c, t);
                    return;
                }
            }
        }
    } else {
        // inplace assignment, e.g. x += 1, ++x, ...
    }
    Q_UNIMPLEMENTED();
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
        amd64_call_code(_codePtr, __qmljs_to_boolean);
        amd64_alu_reg_imm_size(_codePtr, X86_CMP, X86_EAX, 0, 4);
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

