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
    F(StoreName, storeName) \
    F(LoadElement, loadElement) \
    F(StoreElement, storeElement) \
    F(LoadProperty, loadProperty) \
    F(StoreProperty, storeProperty) \
    F(Push, push) \
    F(CallValue, callValue) \
    F(CallProperty, callProperty) \
    F(CallBuiltin, callBuiltin) \
    F(CreateValue, createValue) \
    F(CreateProperty, createProperty) \
    F(CreateActivationProperty, createActivationProperty) \
    F(Jump, jump) \
    F(CJump, jump) \
    F(Unop, unop) \
    F(Binop, binop) \
    F(LoadThis, loadThis) \
    F(InplaceElementOp, inplaceElementOp) \
    F(InplaceMemberOp, inplaceMemberOp) \
    F(InplaceNameOp, inplaceNameOp)

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
    union ValueOrTemp {
        VM::Value value;
        int tempIndex;
    };

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
        VM::String *name;
    };
    struct instr_storeName {
        MOTH_INSTR_HEADER
        VM::String *name;
    };
    struct instr_loadProperty {
        MOTH_INSTR_HEADER
        int baseTemp;
        VM::String *name;
    };
    struct instr_storeProperty {
        MOTH_INSTR_HEADER
        int baseTemp;
        VM::String *name;
    };
    struct instr_loadElement {
        MOTH_INSTR_HEADER
        int base;
        int index;
    };
    struct instr_storeElement {
        MOTH_INSTR_HEADER
        int base;
        int index;
    };
    struct instr_push {
        MOTH_INSTR_HEADER
        quint32 value;
    };
    struct instr_callValue {
        MOTH_INSTR_HEADER
        quint32 argc;
        quint32 args;
    };
    struct instr_callProperty {
        MOTH_INSTR_HEADER
        VM::String *name;
        quint32 argc;
        quint32 args;
    };
    struct instr_callBuiltin {
        MOTH_INSTR_HEADER
        enum {
            builtin_typeof,
            builtin_throw,
            builtin_create_exception_handler,
            builtin_delete_exception_handler,
            builtin_get_exception
        } builtin;
        quint32 argc;
        quint32 args;
    };
    struct instr_createValue {
        MOTH_INSTR_HEADER
        int func;
        quint32 argc;
        quint32 args;
    };
    struct instr_createProperty {
        MOTH_INSTR_HEADER
        int base;
        VM::String *name;
        quint32 argc;
        quint32 args;
    };
    struct instr_createActivationProperty {
        MOTH_INSTR_HEADER
        VM::String *name;
        quint32 argc;
        quint32 args;
    };
    struct instr_jump {
        MOTH_INSTR_HEADER
        ptrdiff_t offset; 
    };
    struct instr_unop {
        MOTH_INSTR_HEADER
        int e;
        VM::Value (*alu)(const VM::Value value, VM::Context *ctx);
    };
    struct instr_binop {
        MOTH_INSTR_HEADER
        ValueOrTemp lhs;
        ValueOrTemp rhs;
        unsigned lhsIsTemp:1;
        unsigned rhsIsTemp:1;
        VM::Value (*alu)(const VM::Value , const VM::Value, VM::Context *);
    };
    struct instr_loadThis {
        MOTH_INSTR_HEADER
    };
    struct instr_inplaceElementOp {
        MOTH_INSTR_HEADER
        void (*alu)(VM::Value, VM::Value, VM::Value, VM::Context *);
        int targetBase;
        int targetIndex;
        int source;
    };
    struct instr_inplaceMemberOp {
        MOTH_INSTR_HEADER
        void (*alu)(VM::Value, VM::Value, VM::String *, VM::Context *);
        int targetBase;
        VM::String *targetMember;
        int source;
    };
    struct instr_inplaceNameOp {
        MOTH_INSTR_HEADER
        void (*alu)(VM::Value, VM::String *, VM::Context *);
        VM::String *targetName;
        int source;
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
    instr_storeName storeName;
    instr_loadElement loadElement;
    instr_storeElement storeElement;
    instr_loadProperty loadProperty;
    instr_storeProperty storeProperty;
    instr_push push;
    instr_callValue callValue;
    instr_callProperty callProperty;
    instr_callBuiltin callBuiltin;
    instr_createValue createValue;
    instr_createProperty createProperty;
    instr_createActivationProperty createActivationProperty;
    instr_jump jump;
    instr_unop unop;
    instr_binop binop;
    instr_loadThis loadThis;
    instr_inplaceElementOp inplaceElementOp;
    instr_inplaceMemberOp inplaceMemberOp;
    instr_inplaceNameOp inplaceNameOp;

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
