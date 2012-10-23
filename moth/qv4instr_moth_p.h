#ifndef QV4INSTR_MOTH_P_H
#define QV4INSTR_MOTH_P_H

#include <QtCore/qglobal.h>
#include "qmljs_objects.h"

#define FOR_EACH_MOTH_INSTR(F) \
    F(Ret, ret) \
    F(LoadUndefined, common) \
    F(LoadNull, common) \
    F(LoadFalse, common) \
    F(LoadTrue, common) \
    F(LoadNumber, loadNumber) \
    F(LoadString, loadString) \
    F(LoadClosure, loadClosure) \
    F(StoreTemp, storeTemp) \
    F(LoadTemp, loadTemp) \
    F(MoveTemp, moveTemp) \
    F(LoadName, loadName) \
    F(Push, push) \
    F(Call, call) \
    F(Jump, jump) \
    F(CJump, jump) \
    F(Binop, binop) \
    F(ActivateProperty, activateProperty)

#if defined(Q_CC_GNU) && (!defined(Q_CC_INTEL) || __INTEL_COMPILER >= 1200)
#  define MOTH_THREADED_INTERPRETER
#endif

#define MOTH_INSTR_ALIGN_MASK (Q_ALIGNOF(QQmlJS::Moth::Instr) - 1)

#ifdef MOTH_THREADED_INTERPRETER
#  define MOTH_INSTR_HEADER void *code;
#else
#  define MOTH_INSTR_HEADER quint8 instructionType;
#endif

#define MOTH_INSTR_ENUM(I, FMT)  I,
#define MOTH_INSTR_SIZE(I, FMT) ((sizeof(QQmlJS::Moth::Instr::instr_##FMT) + MOTH_INSTR_ALIGN_MASK) & ~MOTH_INSTR_ALIGN_MASK)


namespace QQmlJS {
namespace Moth {

union Instr
{
    enum Type {
        FOR_EACH_MOTH_INSTR(MOTH_INSTR_ENUM)
    };

    struct instr_common {
        MOTH_INSTR_HEADER
    };
    struct instr_ret {
        MOTH_INSTR_HEADER
        int tempIndex;
    }; 
    struct instr_storeTemp {
        MOTH_INSTR_HEADER
        int tempIndex;
    };
    struct instr_loadTemp {
        MOTH_INSTR_HEADER
        int tempIndex;
    };
    struct instr_moveTemp {
        MOTH_INSTR_HEADER
        int fromTempIndex;
        int toTempIndex;
    };
    struct instr_loadNumber {
        MOTH_INSTR_HEADER
        double value;
    };
    struct instr_loadString {
        MOTH_INSTR_HEADER
        VM::String *value;
    }; 
    struct instr_loadClosure {
        MOTH_INSTR_HEADER
        IR::Function *value;
    };
    struct instr_loadName {
        MOTH_INSTR_HEADER
        VM::String *value;
    };
    struct instr_push {
        MOTH_INSTR_HEADER
        quint32 value;
    };
    struct instr_call {
        MOTH_INSTR_HEADER
        quint32 argc;
        quint32 args;
    };
    struct instr_jump {
        MOTH_INSTR_HEADER
        ptrdiff_t offset; 
    };
    struct instr_binop {
        MOTH_INSTR_HEADER
        int lhsTempIndex;
        int rhsTempIndex;
        VM::Value (*alu)(const VM::Value , const VM::Value, VM::Context *);
    };
    struct instr_activateProperty {
        MOTH_INSTR_HEADER
        VM::String *propName;
    };

    instr_common common;
    instr_ret ret;
    instr_storeTemp storeTemp;
    instr_loadTemp loadTemp;
    instr_moveTemp moveTemp;
    instr_loadNumber loadNumber;
    instr_loadString loadString;
    instr_loadClosure loadClosure;
    instr_loadName loadName;
    instr_push push;
    instr_call call;
    instr_jump jump;
    instr_binop binop;
    instr_activateProperty activateProperty;

    static int size(Type type);
};

template<int N>
struct InstrMeta {
};

#define MOTH_INSTR_META_TEMPLATE(I, FMT) \
    template<> struct InstrMeta<(int)Instr::I> { \
        enum { Size = MOTH_INSTR_SIZE(I, FMT) }; \
        typedef Instr::instr_##FMT DataType; \
        static const DataType &data(const Instr &instr) { return instr.FMT; } \
        static void setData(Instr &instr, const DataType &v) { instr.FMT = v; } \
    }; 
FOR_EACH_MOTH_INSTR(MOTH_INSTR_META_TEMPLATE);
#undef MOTH_INSTR_META_TEMPLATE

template<int InstrType>
class InstrData : public InstrMeta<InstrType>::DataType
{
};

} // namespace Moth
} // namespace QQmlJS

#endif // QV4INSTR_MOTH_P_H
