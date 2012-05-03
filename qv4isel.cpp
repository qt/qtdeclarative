
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

#ifndef NO_UDIS86
#  include <udis86.h>
#endif

using namespace QQmlJS;

static inline bool protect(const void *addr, size_t size)
{
    size_t pageSize = sysconf(_SC_PAGESIZE);
    size_t iaddr = reinterpret_cast<size_t>(addr);
    size_t roundAddr = iaddr & ~(pageSize - static_cast<size_t>(1));
    int mode = PROT_READ | PROT_WRITE | PROT_EXEC;
    return mprotect(reinterpret_cast<void*>(roundAddr), size + (iaddr - roundAddr), mode) == 0;
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
InstructionSelection::InstructionSelection(IR::Module *module)
    : _module(module)
    , _function(0)
    , _block(0)
    , _code(0)
    , _codePtr(0)
{
}

InstructionSelection::~InstructionSelection()
{
}

void InstructionSelection::visitFunction(IR::Function *function)
{
    uchar *code = (uchar *) malloc(getpagesize());
    Q_ASSERT(! (size_t(code) & 15));

    protect(code, getpagesize());

    uchar *codePtr = code;

    qSwap(_code, code);
    qSwap(_codePtr, codePtr);

    int locals = function->tempCount * sizeof(Value);
    locals = (locals + 15) & ~15;

    amd64_push_reg(_codePtr, AMD64_R14);
    amd64_mov_reg_reg(_codePtr, AMD64_R14, AMD64_RDI, 8);
    amd64_alu_reg_imm(_codePtr, X86_SUB, AMD64_RSP, locals);

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
    amd64_pop_reg(_codePtr, AMD64_R14);
    amd64_ret(_codePtr);

    qSwap(_codePtr, codePtr);
    qSwap(_code, code);

    static bool showCode = !qgetenv("SHOW_CODE").isNull();
    if (showCode) {
#ifndef NO_UDIS86
        ud_t ud_obj;

        ud_init(&ud_obj);
        ud_set_input_buffer(&ud_obj, code, codePtr - code);
        ud_set_mode(&ud_obj, 64);
        ud_set_syntax(&ud_obj, UD_SYN_INTEL);

        while (ud_disassemble(&ud_obj)) {
            printf("\t%s\n", ud_insn_asm(&ud_obj));
        }
#endif
    }

    void (*f)(Context *) = (void (*)(Context *)) code;

    Context *ctx = new (GC) Context;
    ctx->activation = Value::object(ctx, new (GC) ArgumentsObject);
    f(ctx);
    Value d;
    ctx->activation.objectValue->get(String::get(ctx, QLatin1String("d")), &d);
    __qmljs_to_string(ctx, &d, &d);
    qDebug() << qPrintable(d.stringValue->text());
}

String *InstructionSelection::identifier(const QString &s)
{
    String *&id = _identifiers[s];
    if (! id)
        id = new (GC) String(s);
    return id;
}

void InstructionSelection::visitExp(IR::Exp *s)
{
    //    if (IR::Call *c = s->expr->asCall()) {
    //        return;
    //    }
    Q_ASSERT(!"TODO");
}

void InstructionSelection::visitEnter(IR::Enter *)
{
    Q_ASSERT(!"TODO");
}

void InstructionSelection::visitLeave(IR::Leave *)
{
    Q_ASSERT(!"TODO");
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
                amd64_mov_reg_imm(_codePtr, AMD64_RAX, &c->value);
                amd64_movsd_reg_regp(_codePtr, X86_XMM0, AMD64_RAX);
                amd64_call_code(_codePtr, __qmljs_set_activation_property_number);
                return;
            } else if (IR::String *str = s->source->asString()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, propertyName);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, new (GC) String(*str->value));
                amd64_call_code(_codePtr, __qmljs_set_activation_property_string);
                return;
            } else if (IR::Temp *t = s->source->asTemp()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_mov_reg_imm(_codePtr, AMD64_RSI, propertyName);
                amd64_lea_membase(_codePtr, AMD64_RDX, AMD64_RSP, 8 + sizeof(Value) * (t->index - 1));
                amd64_call_code(_codePtr, __qmljs_set_activation_property);
                return;
            }
        } else if (IR::Temp *t = s->target->asTemp()) {
            const int offset = 8 + sizeof(Value) * (t->index - 1);
            if (IR::Name *n = s->source->asName()) {
                String *propertyName = identifier(*n->id);
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_lea_membase(_codePtr, AMD64_RSI, AMD64_RSP, offset);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, propertyName);
                amd64_call_code(_codePtr, __qmljs_get_activation_property);
                return;
            } else if (IR::Const *c = s->source->asConst()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_lea_membase(_codePtr, AMD64_RSI, AMD64_RSP, offset);
                amd64_mov_reg_imm(_codePtr, AMD64_RAX, &c->value);
                amd64_movsd_reg_regp(_codePtr, X86_XMM0, AMD64_RAX);
                amd64_call_code(_codePtr, __qmljs_init_number);
                return;
            } else if (IR::String *str = s->source->asString()) {
                amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                amd64_lea_membase(_codePtr, AMD64_RSI, AMD64_RSP, offset);
                amd64_mov_reg_imm(_codePtr, AMD64_RDX, new (GC) String(*str->value));
                amd64_call_code(_codePtr, __qmljs_init_string);
                return;
            } else if (IR::Binop *b = s->source->asBinop()) {
                IR::Temp *l = b->left->asTemp();
                IR::Temp *r = b->right->asTemp();
                if (l && r) {
                    amd64_mov_reg_reg(_codePtr, AMD64_RDI, AMD64_R14, 8);
                    amd64_lea_membase(_codePtr, AMD64_RSI, AMD64_RSP, offset);
                    amd64_lea_membase(_codePtr, AMD64_RDX, AMD64_RSP, 8 + sizeof(Value) * (l->index - 1));
                    amd64_lea_membase(_codePtr, AMD64_RCX, AMD64_RSP, 8 + sizeof(Value) * (r->index - 1));

                    void (*op)(Context *, Value *, const Value *, const Value *) = 0;

                    switch (b->op) {
                    case QQmlJS::IR::OpInvalid:
                    case QQmlJS::IR::OpIfTrue:
                    case QQmlJS::IR::OpNot:
                    case QQmlJS::IR::OpUMinus:
                    case QQmlJS::IR::OpUPlus:
                    case QQmlJS::IR::OpCompl:
                        Q_ASSERT(!"unreachable");
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
                        Q_ASSERT(!"unreachable");
                        break;
                    }
                    amd64_call_code(_codePtr, op);
                    return;
                }
            }
        }
    } else {
        // inplace assignment, e.g. x += 1, ++x, ...
    }
    Q_ASSERT(!"TODO");
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
        amd64_lea_membase(_codePtr, AMD64_RSI, AMD64_RSP, 8 + sizeof(Value) * (t->index - 1));
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
    Q_ASSERT(!"TODO");
}

void InstructionSelection::visitRet(IR::Ret *s)
{
    qWarning() << "TODO: RET";
    //Q_ASSERT(!"TODO");
}

