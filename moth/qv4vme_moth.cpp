#include "qv4vme_moth_p.h"
#include "qv4instr_moth_p.h"
#include "qmljs_value.h"
#include "debugging.h"

#include <iostream>

#include <alloca.h>

#ifdef DO_TRACE_INSTR
#  define TRACE_INSTR(I) fprintf(stderr, "executing a %s\n", #I);
#  define TRACE(n, str, ...) { char buf[4096]; snprintf(buf, 4096, str, __VA_ARGS__); fprintf(stderr, "    %s : %s\n", #n, buf); }
#else
#  define TRACE_INSTR(I)
#  define TRACE(n, str, ...)
#endif // DO_TRACE_INSTR

using namespace QQmlJS;
using namespace QQmlJS::Moth;

#define MOTH_BEGIN_INSTR_COMMON(I) { \
    const InstrMeta<(int)Instr::I>::DataType &instr = InstrMeta<(int)Instr::I>::data(*genericInstr); \
    code += InstrMeta<(int)Instr::I>::Size; \
    Q_UNUSED(instr); \
    TRACE_INSTR(I)

#ifdef MOTH_THREADED_INTERPRETER

#  define MOTH_BEGIN_INSTR(I) op_##I: \
    MOTH_BEGIN_INSTR_COMMON(I)

#  define MOTH_NEXT_INSTR(I) { \
    genericInstr = reinterpret_cast<const Instr *>(code); \
    goto *genericInstr->common.code; \
    }

#  define MOTH_END_INSTR(I) } \
    genericInstr = reinterpret_cast<const Instr *>(code); \
    goto *genericInstr->common.code; \

#else

#  define MOTH_BEGIN_INSTR(I) \
    case Instr::I: \
    MOTH_BEGIN_INSTR_COMMON(I)

#  define MOTH_NEXT_INSTR(I) { \
    break; \
    }

#  define MOTH_END_INSTR(I) } \
    break;

#endif

static inline VM::Value *tempValue(QQmlJS::VM::ExecutionContext *context, VM::Value* stack, int index)
{
#ifdef DO_TRACE_INSTR
    const char *kind;
    int pos;
    if (index < 0) {
        kind = "arg";
        pos = -index - 1;
    } else if (index < (int) context->variableCount()) {
        kind = "local";
        pos = index;
    } else {
        kind = "temp";
        pos = index - context->variableCount();
    }
    fprintf(stderr, "    tempValue: index = %d : %s = %d\n",
          index, kind, pos);
#endif // DO_TRACE_INSTR

    if (index < 0) {
        const int arg = -index - 1;

        Q_ASSERT(arg >= 0);
        Q_ASSERT((unsigned) arg < context->argumentCount);
        Q_ASSERT(context->arguments);

        return context->arguments + arg;
    } else if (index < (int) context->variableCount()) {
        Q_ASSERT(index >= 0);
        Q_ASSERT(context->locals);

        return context->locals + index;
    } else {
        int off = index - context->variableCount();

        Q_ASSERT(off >= 0);

        return stack + off;
    }
}

class FunctionState: public Debugging::FunctionState
{
public:
    FunctionState(QQmlJS::VM::ExecutionContext *context, const uchar **code)
        : Debugging::FunctionState(context)
        , stack(0)
        , stackSize(0)
        , code(code)
    {}

    virtual VM::Value *temp(unsigned idx) { return stack + idx; }

    void setStack(VM::Value *stack, unsigned stackSize)
    { this->stack = stack; this->stackSize = stackSize; }

private:
    VM::Value *stack;
    unsigned stackSize;
    const uchar **code;
};

#define TEMP(index) *tempValue(context, stack, index)

VM::Value VME::operator()(QQmlJS::VM::ExecutionContext *context, const uchar *code
#ifdef MOTH_THREADED_INTERPRETER
        , void ***storeJumpTable
#endif
        )
{
#ifdef DO_TRACE_INSTR
    qDebug("Starting VME with context=%p and code=%p", context, code);
#endif // DO_TRACE_INSTR

#ifdef MOTH_THREADED_INTERPRETER
    if (storeJumpTable) {
#define MOTH_INSTR_ADDR(I, FMT) &&op_##I,
        static void *jumpTable[] = {
            FOR_EACH_MOTH_INSTR(MOTH_INSTR_ADDR)
        };
#undef MOTH_INSTR_ADDR
        *storeJumpTable = jumpTable;
        return VM::Value::undefinedValue();
    }
#endif

    VM::Value *stack = 0;
    unsigned stackSize = 0;
    FunctionState state(context, &code);

#ifdef MOTH_THREADED_INTERPRETER
    const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
    goto *genericInstr->common.code;
#else
    for (;;) {
        const Instr *genericInstr = reinterpret_cast<const Instr *>(code);
        switch (genericInstr->common.instructionType) {
#endif

    MOTH_BEGIN_INSTR(MoveTemp)
        VM::Value tmp = TEMP(instr.fromTempIndex);
        TEMP(instr.toTempIndex) = tmp;
    MOTH_END_INSTR(MoveTemp)

    MOTH_BEGIN_INSTR(LoadValue)
        TEMP(instr.targetTempIndex) = instr.value;
    MOTH_END_INSTR(LoadValue)

    MOTH_BEGIN_INSTR(LoadClosure)
        VM::Value c = __qmljs_init_closure(instr.value, context);
        TEMP(instr.targetTempIndex) = c;
    MOTH_END_INSTR(LoadClosure)

    MOTH_BEGIN_INSTR(LoadName)
        TRACE(inline, "property name = %s", instr.name->toQString().toUtf8().constData());
        VM::Value val = __qmljs_get_activation_property(context, instr.name);
        TEMP(instr.targetTempIndex) = val;
    MOTH_END_INSTR(LoadName)

    MOTH_BEGIN_INSTR(StoreName)
        TRACE(inline, "property name = %s", instr.name->toQString().toUtf8().constData());
        VM::Value source = instr.sourceIsTemp ? TEMP(instr.source.tempIndex) : instr.source.value;
        __qmljs_set_activation_property(context, instr.name, source);
    MOTH_END_INSTR(StoreName)

    MOTH_BEGIN_INSTR(LoadElement)
        TEMP(instr.targetTempIndex) = __qmljs_get_element(context, TEMP(instr.base), TEMP(instr.index));
    MOTH_END_INSTR(LoadElement)

    MOTH_BEGIN_INSTR(StoreElement)
        VM::Value source = instr.sourceIsTemp ? TEMP(instr.source.tempIndex) : instr.source.value;
        __qmljs_set_element(context, TEMP(instr.base), TEMP(instr.index), source);
    MOTH_END_INSTR(StoreElement)

    MOTH_BEGIN_INSTR(LoadProperty)
        TRACE(inline, "base temp = %d, property name = %s", instr.baseTemp, instr.name->toQString().toUtf8().constData());
        VM::Value base = TEMP(instr.baseTemp);
        TEMP(instr.targetTempIndex) = __qmljs_get_property(context, base, instr.name);
    MOTH_END_INSTR(LoadProperty)

    MOTH_BEGIN_INSTR(StoreProperty)
        TRACE(inline, "base temp = %d, property name = %s", instr.baseTemp, instr.name->toQString().toUtf8().constData());
        VM::Value base = TEMP(instr.baseTemp);
        VM::Value source = instr.sourceIsTemp ? TEMP(instr.source.tempIndex) : instr.source.value;
        __qmljs_set_property(context, base, instr.name, source);
    MOTH_END_INSTR(StoreProperty)

    MOTH_BEGIN_INSTR(Push)
        TRACE(inline, "stack size: %u", instr.value);
        stackSize = instr.value;
        stack = static_cast<VM::Value *>(alloca(stackSize * sizeof(VM::Value)));
        state.setStack(stack, stackSize);
    MOTH_END_INSTR(Push)

    MOTH_BEGIN_INSTR(CallValue)
#ifdef DO_TRACE_INSTR
        if (Debugging::Debugger *debugger = context->engine->debugger) {
            if (VM::FunctionObject *o = (TEMP(instr.destIndex)).asFunctionObject()) {
                if (Debugging::FunctionDebugInfo *info = debugger->debugInfo(o)) {
                    QString n = debugger->name(o);
                    std::cerr << "*** Call to \"" << (n.isNull() ? "<no name>" : qPrintable(n)) << "\" defined @" << info->startLine << ":" << info->startColumn << std::endl;
                }
            }
        }
#endif // DO_TRACE_INSTR
        int argStart = instr.args - context->variableCount();
        TRACE(Call, "value index = %d, argStart = %d, argc = %d, result temp index = %d", instr.destIndex, argStart, instr.argc, instr.targetTempIndex);
        VM::Value *args = stack + argStart;
        VM::Value result = __qmljs_call_value(context, VM::Value::undefinedValue(), TEMP(instr.destIndex), args, instr.argc);
        TEMP(instr.targetTempIndex) = result;
    MOTH_END_INSTR(CallValue)

    MOTH_BEGIN_INSTR(CallProperty)
        int argStart = instr.args - context->variableCount();
    // TODO: change this assert everywhere to include a minimum
    // TODO: the args calculation is duplicate code, fix that
        VM::Value *args = stack + argStart;
        VM::Value base = TEMP(instr.baseTemp);
        TEMP(instr.targetTempIndex) = __qmljs_call_property(context, base, instr.name, args, instr.argc);
    MOTH_END_INSTR(CallProperty)

    MOTH_BEGIN_INSTR(CallActivationProperty)
        int argStart = instr.args - context->variableCount();
    // TODO: change this assert everywhere to include a minimum
    // TODO: the args calculation is duplicate code, fix that
        VM::Value *args = stack + argStart;
        TEMP(instr.targetTempIndex) = __qmljs_call_activation_property(context, instr.name, args, instr.argc);
    MOTH_END_INSTR(CallActivationProperty)

    MOTH_BEGIN_INSTR(CallBuiltin)
        // TODO: split this into separate instructions
        switch (instr.builtin) {
        case Instr::instr_callBuiltin::builtin_throw:
            TRACE(builtin_throw, "Throwing now...%s", "");
            __qmljs_builtin_throw(TEMP(instr.argTemp), context);
            break;
        case Instr::instr_callBuiltin::builtin_create_exception_handler: {
            TRACE(builtin_create_exception_handler, "%s", "");
            void *buf = __qmljs_create_exception_handler(context);
            // The targetTempIndex is the only value we need from the instr to
            // continue execution when an exception is caught.
            int targetTempIndex = instr.targetTempIndex;
            int didThrow = setjmp(* static_cast<jmp_buf *>(buf));
            // Two ways to come here: after a create, or after a throw.
            if (didThrow)
                // At this point, the interpreter state can be anything but
                // valid, so first restore the state. This includes all relevant
                // locals.
                restoreState(context, targetTempIndex, code);
            else
                // Save the state and any variables we need when catching an
                // exception, so we can restore the state at that point.
                saveState(context, targetTempIndex, code);
            TEMP(targetTempIndex) = VM::Value::fromInt32(didThrow);
        } break;
        case Instr::instr_callBuiltin::builtin_delete_exception_handler:
            TRACE(builtin_delete_exception_handler, "%s", "");
            __qmljs_delete_exception_handler(context);
            break;
        case Instr::instr_callBuiltin::builtin_get_exception:
            TEMP(instr.targetTempIndex) = __qmljs_get_exception(context);
            break;
        case Instr::instr_callBuiltin::builtin_push_with:
            __qmljs_builtin_push_with(TEMP(instr.argTemp), context);
            break;
        case Instr::instr_callBuiltin::builtin_pop_with:
            __qmljs_builtin_pop_with(context);
            break;
        default:
            assert(!"TODO!");
            Q_UNREACHABLE();
        }
    MOTH_END_INSTR(CallBuiltin)

    MOTH_BEGIN_INSTR(CallBuiltinForeachIteratorObject)
        VM::Value &obj = TEMP(instr.argTemp);
        VM::Value it = __qmljs_foreach_iterator_object(obj, context);
        TEMP(instr.targetTempIndex) = it;
    MOTH_END_INSTR(CallBuiltinForeachIteratorObject)

    MOTH_BEGIN_INSTR(CallBuiltinForeachNextPropertyName)
        VM::Value &iter = TEMP(instr.argTemp);
        VM::Value val = __qmljs_foreach_next_property_name(iter);
        TEMP(instr.targetTempIndex) = val;
    MOTH_END_INSTR(CallBuiltinForeachNextPropertyName)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteMember)
        TEMP(instr.targetTempIndex) = __qmljs_delete_member(context, TEMP(instr.base), instr.member);
    MOTH_END_INSTR(CallBuiltinDeleteMember)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteSubscript)
        TEMP(instr.targetTempIndex) = __qmljs_delete_subscript(context, TEMP(instr.base), TEMP(instr.index));
    MOTH_END_INSTR(CallBuiltinDeleteSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinDeleteName)
        TEMP(instr.targetTempIndex) = __qmljs_delete_name(context, instr.name);
    MOTH_END_INSTR(CallBuiltinDeleteName)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofMember)
        TEMP(instr.targetTempIndex) = __qmljs_builtin_typeof_member(TEMP(instr.base), instr.member, context);
    MOTH_END_INSTR(CallBuiltinTypeofMember)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofSubscript)
        TEMP(instr.targetTempIndex) = __qmljs_builtin_typeof_element(TEMP(instr.base), TEMP(instr.index), context);
    MOTH_END_INSTR(CallBuiltinTypeofSubscript)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofName)
        TEMP(instr.targetTempIndex) = __qmljs_builtin_typeof_name(instr.name, context);
    MOTH_END_INSTR(CallBuiltinTypeofName)

    MOTH_BEGIN_INSTR(CallBuiltinTypeofValue)
        TEMP(instr.targetTempIndex) = __qmljs_builtin_typeof(TEMP(instr.tempIndex), context);
    MOTH_END_INSTR(CallBuiltinTypeofValue)

    MOTH_BEGIN_INSTR(CallBuiltinDeclareVar)
        __qmljs_builtin_declare_var(context, instr.isDeletable, instr.varName);
    MOTH_END_INSTR(CallBuiltinDeclareVar)

    MOTH_BEGIN_INSTR(CallBuiltinDefineGetterSetter)
        __qmljs_builtin_define_getter_setter(TEMP(instr.objectTemp), instr.name, TEMP(instr.getterTemp), TEMP(instr.setterTemp), context);
    MOTH_END_INSTR(CallBuiltinDefineGetterSetter)

    MOTH_BEGIN_INSTR(CallBuiltinDefineProperty)
        __qmljs_builtin_define_property(TEMP(instr.objectTemp), instr.name, TEMP(instr.valueTemp), context);
    MOTH_END_INSTR(CallBuiltinDefineProperty)

    MOTH_BEGIN_INSTR(CreateValue)
        int argStart = instr.args - context->variableCount();
        VM::Value *args = stack + argStart;
        TEMP(instr.targetTempIndex) = __qmljs_construct_value(context, TEMP(instr.func), args, instr.argc);
    MOTH_END_INSTR(CreateValue)

    MOTH_BEGIN_INSTR(CreateProperty)
        int argStart = instr.args - context->variableCount();
        VM::Value *args = stack + argStart;
        TEMP(instr.targetTempIndex) = __qmljs_construct_property(context, TEMP(instr.base), instr.name, args, instr.argc);
    MOTH_END_INSTR(CreateProperty)

    MOTH_BEGIN_INSTR(CreateActivationProperty)
        TRACE(inline, "property name = %s, argc = %d", instr.name->toQString().toUtf8().constData(), instr.argc);
        int argStart = instr.args - context->variableCount();
        VM::Value *args = stack + argStart;
        TEMP(instr.targetTempIndex) = __qmljs_construct_activation_property(context, instr.name, args, instr.argc);
    MOTH_END_INSTR(CreateActivationProperty)

    MOTH_BEGIN_INSTR(Jump)
        code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(Jump)

    MOTH_BEGIN_INSTR(CJump)
        if (__qmljs_to_boolean(TEMP(instr.tempIndex), context))
            code = ((uchar *)&instr.offset) + instr.offset;
    MOTH_END_INSTR(CJump)

    MOTH_BEGIN_INSTR(Unop)
        TEMP(instr.targetTempIndex) = instr.alu(TEMP(instr.e), context);
    MOTH_END_INSTR(Unop)

    MOTH_BEGIN_INSTR(Binop)
        VM::Value lhs = instr.lhsIsTemp ? TEMP(instr.lhs.tempIndex) : instr.lhs.value;
        VM::Value rhs = instr.rhsIsTemp ? TEMP(instr.rhs.tempIndex) : instr.rhs.value;
        TEMP(instr.targetTempIndex) = instr.alu(lhs, rhs, context);
    MOTH_END_INSTR(Binop)

    MOTH_BEGIN_INSTR(Ret)
        VM::Value result = TEMP(instr.tempIndex);
//        TRACE(Ret, "returning value %s", result.toString(context)->toQString().toUtf8().constData());
        return result;
    MOTH_END_INSTR(Ret)

    MOTH_BEGIN_INSTR(LoadThis)
        TEMP(instr.targetTempIndex) = __qmljs_get_thisObject(context);
    MOTH_END_INSTR(LoadThis)

    MOTH_BEGIN_INSTR(InplaceElementOp)
        VM::Value source = instr.sourceIsTemp ? TEMP(instr.source.tempIndex) : instr.source.value;
        instr.alu(TEMP(instr.targetBase),
                  TEMP(instr.targetIndex),
                  source,
                  context);
    MOTH_END_INSTR(InplaceElementOp)

    MOTH_BEGIN_INSTR(InplaceMemberOp)
        VM::Value source = instr.sourceIsTemp ? TEMP(instr.source.tempIndex) : instr.source.value;
        instr.alu(source,
                  TEMP(instr.targetBase),
                  instr.targetMember,
                  context);
    MOTH_END_INSTR(InplaceMemberOp)

    MOTH_BEGIN_INSTR(InplaceNameOp)
        TRACE(name, "%s", instr.targetName->toQString().toUtf8().constData());
        VM::Value source = instr.sourceIsTemp ? TEMP(instr.source.tempIndex) : instr.source.value;
        instr.alu(source,
                  instr.targetName,
                  context);
    MOTH_END_INSTR(InplaceNameOp)

#ifdef MOTH_THREADED_INTERPRETER
    // nothing to do
#else
        default:
            qFatal("QQmlJS::Moth::VME: Internal error - unknown instruction %d", genericInstr->common.instructionType);
            break;
        }
    }
#endif

}

#ifdef MOTH_THREADED_INTERPRETER
void **VME::instructionJumpTable()
{
    static void **jumpTable = 0;
    if (!jumpTable) {
        VME dummy;
        dummy(0, 0, &jumpTable);
    }
    return jumpTable;
}
#endif

VM::Value VME::exec(VM::ExecutionContext *ctxt, const uchar *code)
{
    VME vme;
    return vme(ctxt, code);
}

void VME::restoreState(VM::ExecutionContext *context, int &targetTempIndex, const uchar *&code)
{
    VM::ExecutionEngine::ExceptionHandler &handler = context->engine->unwindStack.last();
    targetTempIndex = handler.targetTempIndex;
    code = handler.code;
}

void VME::saveState(VM::ExecutionContext *context, int targetTempIndex, const uchar *code)
{
    VM::ExecutionEngine::ExceptionHandler &handler = context->engine->unwindStack.last();
    handler.targetTempIndex = targetTempIndex;
    handler.code = code;
}
