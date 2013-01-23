#ifndef QV4INSTR_MOTH_P_H
#define QV4INSTR_MOTH_P_H

#include <QtCore/qglobal.h>
#include "qv4object.h"

#define FOR_EACH_MOTH_INSTR(F) \
    F(Ret, ret) \
    F(LoadValue, loadValue) \
    F(LoadClosure, loadClosure) \
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
    F(CallBuiltinForeachIteratorObject, callBuiltinForeachIteratorObject) \
    F(CallBuiltinForeachNextPropertyName, callBuiltinForeachNextPropertyName) \
    F(CallBuiltinDeleteMember, callBuiltinDeleteMember) \
    F(CallBuiltinDeleteSubscript, callBuiltinDeleteSubscript) \
    F(CallBuiltinDeleteName, callBuiltinDeleteName) \
    F(CallBuiltinTypeofMember, callBuiltinTypeofMember) \
    F(CallBuiltinTypeofSubscript, callBuiltinTypeofSubscript) \
    F(CallBuiltinTypeofName, callBuiltinTypeofName) \
    F(CallBuiltinTypeofValue, callBuiltinTypeofValue) \
    F(CallBuiltinDeclareVar, callBuiltinDeclareVar) \
    F(CallBuiltinDefineGetterSetter, callBuiltinDefineGetterSetter) \
    F(CallBuiltinDefineProperty, callBuiltinDefineProperty) \
    F(CreateValue, createValue) \
    F(CreateProperty, createProperty) \
    F(CreateActivationProperty, createActivationProperty) \
    F(Jump, jump) \
    F(CJump, cjump) \
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
        int tempIndex;
    };
    struct instr_ret {
        MOTH_INSTR_HEADER
        int tempIndex;
    }; 
    struct instr_loadValue {
        MOTH_INSTR_HEADER
        int targetTempIndex;
        VM::Value value;
    };
    struct instr_moveTemp {
        MOTH_INSTR_HEADER
        int fromTempIndex;
        int toTempIndex;
    };
    struct instr_loadClosure {
        MOTH_INSTR_HEADER
        VM::Function *value;
        int targetTempIndex;
    };
    struct instr_loadName {
        MOTH_INSTR_HEADER
        VM::String *name;
        int targetTempIndex;
    };
    struct instr_storeName {
        MOTH_INSTR_HEADER
        VM::String *name;
        ValueOrTemp source;
        unsigned sourceIsTemp:1;
    };
    struct instr_loadProperty {
        MOTH_INSTR_HEADER
        int baseTemp;
        int targetTempIndex;
        VM::String *name;
    };
    struct instr_storeProperty {
        MOTH_INSTR_HEADER
        int baseTemp;
        VM::String *name;
        ValueOrTemp source;
        unsigned sourceIsTemp:1;
    };
    struct instr_loadElement {
        MOTH_INSTR_HEADER
        int base;
        int index;
        int targetTempIndex;
    };
    struct instr_storeElement {
        MOTH_INSTR_HEADER
        int base;
        int index;
        ValueOrTemp source;
        unsigned sourceIsTemp:1;
    };
    struct instr_push {
        MOTH_INSTR_HEADER
        quint32 value;
    };
    struct instr_callValue {
        MOTH_INSTR_HEADER
        quint32 argc;
        quint32 args;
        int destIndex;
        int targetTempIndex;
    };
    struct instr_callProperty {
        MOTH_INSTR_HEADER
        VM::String *name;
        int baseTemp;
        quint32 argc;
        quint32 args;
        int targetTempIndex;
    };
    struct instr_callBuiltin {
        MOTH_INSTR_HEADER
        enum {
            builtin_throw,
            builtin_rethrow,
            builtin_create_exception_handler,
            builtin_delete_exception_handler,
            builtin_get_exception,
            builtin_push_with,
            builtin_pop_with
        } builtin;
        int argTemp;
        int targetTempIndex;
    };
    struct instr_callBuiltinForeachIteratorObject {
        MOTH_INSTR_HEADER
        int argTemp;
        int targetTempIndex;
    };
    struct instr_callBuiltinForeachNextPropertyName {
        MOTH_INSTR_HEADER
        int argTemp;
        int targetTempIndex;
    };
    struct instr_callBuiltinDeleteMember {
        MOTH_INSTR_HEADER
        int base;
        VM::String *member;
        int targetTempIndex;
    };
    struct instr_callBuiltinDeleteSubscript {
        MOTH_INSTR_HEADER
        int base;
        int index;
        int targetTempIndex;
    };
    struct instr_callBuiltinDeleteName {
        MOTH_INSTR_HEADER
        VM::String *name;
        int targetTempIndex;
    };
    struct instr_callBuiltinTypeofMember {
        MOTH_INSTR_HEADER
        int base;
        VM::String *member;
        int targetTempIndex;
    };
    struct instr_callBuiltinTypeofSubscript {
        MOTH_INSTR_HEADER
        int base;
        int index;
        int targetTempIndex;
    };
    struct instr_callBuiltinTypeofName {
        MOTH_INSTR_HEADER
        VM::String *name;
        int targetTempIndex;
    };
    struct instr_callBuiltinTypeofValue {
        MOTH_INSTR_HEADER
        int tempIndex;
        int targetTempIndex;
    };
    struct instr_callBuiltinDeclareVar {
        MOTH_INSTR_HEADER
        bool isDeletable;
        VM::String *varName;
    };
    struct instr_callBuiltinDefineGetterSetter {
        MOTH_INSTR_HEADER
        int objectTemp;
        VM::String *name;
        int getterTemp;
        int setterTemp;
    };
    struct instr_callBuiltinDefineProperty {
        MOTH_INSTR_HEADER
        int objectTemp;
        VM::String *name;
        int valueTemp;
    };
    struct instr_createValue {
        MOTH_INSTR_HEADER
        int func;
        quint32 argc;
        quint32 args;
        int targetTempIndex;
    };
    struct instr_createProperty {
        MOTH_INSTR_HEADER
        int base;
        VM::String *name;
        quint32 argc;
        quint32 args;
        int targetTempIndex;
    };
    struct instr_createActivationProperty {
        MOTH_INSTR_HEADER
        VM::String *name;
        quint32 argc;
        quint32 args;
        int targetTempIndex;
    };
    struct instr_jump {
        MOTH_INSTR_HEADER
        ptrdiff_t offset; 
    };
    struct instr_cjump {
        MOTH_INSTR_HEADER
        ptrdiff_t offset;
        int tempIndex;
    };
    struct instr_unop {
        MOTH_INSTR_HEADER
        VM::Value (*alu)(const VM::Value value, VM::ExecutionContext *ctx);
        int e;
        int targetTempIndex;
    };
    struct instr_binop {
        MOTH_INSTR_HEADER
        VM::Value (*alu)(const VM::Value , const VM::Value, VM::ExecutionContext *);
        int targetTempIndex;
        ValueOrTemp lhs;
        ValueOrTemp rhs;
        unsigned lhsIsTemp:1;
        unsigned rhsIsTemp:1;
    };
    struct instr_loadThis {
        MOTH_INSTR_HEADER
        int targetTempIndex;
    };
    struct instr_inplaceElementOp {
        MOTH_INSTR_HEADER
        void (*alu)(VM::Value, VM::Value, VM::Value, VM::ExecutionContext *);
        int targetBase;
        int targetIndex;
        ValueOrTemp source;
        unsigned sourceIsTemp:1;
    };
    struct instr_inplaceMemberOp {
        MOTH_INSTR_HEADER
        void (*alu)(VM::Value, VM::Value, VM::String *, VM::ExecutionContext *);
        int targetBase;
        VM::String *targetMember;
        ValueOrTemp source;
        unsigned sourceIsTemp:1;
    };
    struct instr_inplaceNameOp {
        MOTH_INSTR_HEADER
        void (*alu)(VM::Value, VM::String *, VM::ExecutionContext *);
        VM::String *targetName;
        ValueOrTemp source;
        unsigned sourceIsTemp:1;
    };

    instr_common common;
    instr_ret ret;
    instr_loadValue loadValue;
    instr_moveTemp moveTemp;
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
    instr_callBuiltinForeachIteratorObject callBuiltinForeachIteratorObject;
    instr_callBuiltinForeachNextPropertyName callBuiltinForeachNextPropertyName;
    instr_callBuiltinDeleteMember callBuiltinDeleteMember;
    instr_callBuiltinDeleteSubscript callBuiltinDeleteSubscript;
    instr_callBuiltinDeleteName callBuiltinDeleteName;
    instr_callBuiltinTypeofMember callBuiltinTypeofMember;
    instr_callBuiltinTypeofSubscript callBuiltinTypeofSubscript;
    instr_callBuiltinTypeofName callBuiltinTypeofName;
    instr_callBuiltinTypeofValue callBuiltinTypeofValue;
    instr_callBuiltinDeclareVar callBuiltinDeclareVar;
    instr_callBuiltinDefineGetterSetter callBuiltinDefineGetterSetter;
    instr_callBuiltinDefineProperty callBuiltinDefineProperty;
    instr_createValue createValue;
    instr_createProperty createProperty;
    instr_createActivationProperty createActivationProperty;
    instr_jump jump;
    instr_cjump cjump;
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
