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
    F(CallElement, callElement) \
    F(CallActivationProperty, callActivationProperty) \
    F(CallBuiltinThrow, callBuiltinThrow) \
    F(CallBuiltinCreateExceptionHandler, callBuiltinCreateExceptionHandler) \
    F(CallBuiltinFinishTry, callBuiltinFinishTry) \
    F(CallBuiltinGetException, callBuiltinGetException) \
    F(CallBuiltinPushScope, callBuiltinPushScope) \
    F(CallBuiltinPushCatchScope, callBuiltinPushCatchScope) \
    F(CallBuiltinPopScope, callBuiltinPopScope) \
    F(CallBuiltinForeachIteratorObject, callBuiltinForeachIteratorObject) \
    F(CallBuiltinForeachNextPropertyName, callBuiltinForeachNextPropertyName) \
    F(CallBuiltinDeleteMember, callBuiltinDeleteMember) \
    F(CallBuiltinDeleteSubscript, callBuiltinDeleteSubscript) \
    F(CallBuiltinDeleteName, callBuiltinDeleteName) \
    F(CallBuiltinTypeofMember, callBuiltinTypeofMember) \
    F(CallBuiltinTypeofSubscript, callBuiltinTypeofSubscript) \
    F(CallBuiltinTypeofName, callBuiltinTypeofName) \
    F(CallBuiltinTypeofValue, callBuiltinTypeofValue) \
    F(CallBuiltinPostIncMember, callBuiltinPostIncMember) \
    F(CallBuiltinPostIncSubscript, callBuiltinPostIncSubscript) \
    F(CallBuiltinPostIncName, callBuiltinPostIncName) \
    F(CallBuiltinPostIncValue, callBuiltinPostIncValue) \
    F(CallBuiltinPostDecMember, callBuiltinPostDecMember) \
    F(CallBuiltinPostDecSubscript, callBuiltinPostDecSubscript) \
    F(CallBuiltinPostDecName, callBuiltinPostDecName) \
    F(CallBuiltinPostDecValue, callBuiltinPostDecValue) \
    F(CallBuiltinDeclareVar, callBuiltinDeclareVar) \
    F(CallBuiltinDefineGetterSetter, callBuiltinDefineGetterSetter) \
    F(CallBuiltinDefineProperty, callBuiltinDefineProperty) \
    F(CallBuiltinDefineArray, callBuiltinDefineArray) \
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
    struct Param {
        enum {
            ValueType    = 0,
            ArgumentType = 1,
            LocalType    = 2,
            TempType     = 3,
            ScopedLocalType  = 4
        };
        VM::Value value;
        unsigned type  : 3;
        unsigned scope : 29;
        unsigned index;

        bool isValue() const { return type == ValueType; }
        bool isArgument() const { return type == ArgumentType; }
        bool isLocal() const { return type == LocalType; }
        bool isTemp() const { return type == TempType; }
        bool isScopedLocal() const { return type == ScopedLocalType; }

        static Param createValue(const VM::Value &v)
        {
            Param p;
            p.type = ValueType;
            p.scope = 0;
            p.value = v;
            return p;
        }

        static Param createArgument(unsigned idx)
        {
            Param p;
            p.type = ArgumentType;
            p.scope = 0;
            p.index = idx;
            return p;
        }

        static Param createLocal(unsigned idx)
        {
            Param p;
            p.type = LocalType;
            p.scope = 0;
            p.index = idx;
            return p;
        }

        static Param createTemp(unsigned idx)
        {
            Param p;
            p.type = TempType;
            p.scope = 0;
            p.index = idx;
            return p;
        }

        static Param createScopedLocal(unsigned idx, uint scope)
        {
            Param p;
            p.type = ScopedLocalType;
            p.scope = scope;
            p.index = idx;
            return p;
        }
    };

    enum Type {
        FOR_EACH_MOTH_INSTR(MOTH_INSTR_ENUM)
    };

    struct instr_common {
        MOTH_INSTR_HEADER
    };
    struct instr_ret {
        MOTH_INSTR_HEADER
        Param result;
    }; 
    struct instr_loadValue {
        MOTH_INSTR_HEADER
        Param value;
        Param result;
    };
    struct instr_moveTemp {
        MOTH_INSTR_HEADER
        Param source;
        Param result;
    };
    struct instr_loadClosure {
        MOTH_INSTR_HEADER
        VM::Function *value;
        Param result;
    };
    struct instr_loadName {
        MOTH_INSTR_HEADER
        VM::String *name;
        Param result;
    };
    struct instr_storeName {
        MOTH_INSTR_HEADER
        VM::String *name;
        Param source;
    };
    struct instr_loadProperty {
        MOTH_INSTR_HEADER
        VM::String *name;
        Param base;
        Param result;
    };
    struct instr_storeProperty {
        MOTH_INSTR_HEADER
        VM::String *name;
        Param base;
        Param source;
    };
    struct instr_loadElement {
        MOTH_INSTR_HEADER
        Param base;
        Param index;
        Param result;
    };
    struct instr_storeElement {
        MOTH_INSTR_HEADER
        Param base;
        Param index;
        Param source;
    };
    struct instr_push {
        MOTH_INSTR_HEADER
        quint32 value;
    };
    struct instr_callValue {
        MOTH_INSTR_HEADER
        quint32 argc;
        quint32 args;
        Param dest;
        Param result;
    };
    struct instr_callProperty {
        MOTH_INSTR_HEADER
        VM::String *name;
        quint32 argc;
        quint32 args;
        Param base;
        Param result;
    };
    struct instr_callElement {
        MOTH_INSTR_HEADER
        Param base;
        Param index;
        quint32 argc;
        quint32 args;
        Param result;
    };
    struct instr_callActivationProperty {
        MOTH_INSTR_HEADER
        VM::String *name;
        quint32 argc;
        quint32 args;
        Param result;
    };
    struct instr_callBuiltinThrow {
        MOTH_INSTR_HEADER
        Param arg;
    };
    struct instr_callBuiltinCreateExceptionHandler {
        MOTH_INSTR_HEADER
        Param result;
    };
    struct instr_callBuiltinFinishTry {
        MOTH_INSTR_HEADER
    };
    struct instr_callBuiltinGetException {
        MOTH_INSTR_HEADER
        Param result;
    };
    struct instr_callBuiltinPushScope {
        MOTH_INSTR_HEADER
        Param arg;
    };
    struct instr_callBuiltinPushCatchScope {
        MOTH_INSTR_HEADER
        VM::String *varName;
    };
    struct instr_callBuiltinPopScope {
        MOTH_INSTR_HEADER
    };
    struct instr_callBuiltinForeachIteratorObject {
        MOTH_INSTR_HEADER
        Param arg;
        Param result;
    };
    struct instr_callBuiltinForeachNextPropertyName {
        MOTH_INSTR_HEADER
        Param arg;
        Param result;
    };
    struct instr_callBuiltinDeleteMember {
        MOTH_INSTR_HEADER
        VM::String *member;
        Param base;
        Param result;
    };
    struct instr_callBuiltinDeleteSubscript {
        MOTH_INSTR_HEADER
        Param base;
        Param index;
        Param result;
    };
    struct instr_callBuiltinDeleteName {
        MOTH_INSTR_HEADER
        VM::String *name;
        Param result;
    };
    struct instr_callBuiltinTypeofMember {
        MOTH_INSTR_HEADER
        VM::String *member;
        Param base;
        Param result;
    };
    struct instr_callBuiltinTypeofSubscript {
        MOTH_INSTR_HEADER
        Param base;
        Param index;
        Param result;
    };
    struct instr_callBuiltinTypeofName {
        MOTH_INSTR_HEADER
        VM::String *name;
        Param result;
    };
    struct instr_callBuiltinTypeofValue {
        MOTH_INSTR_HEADER
        Param value;
        Param result;
    };
    struct instr_callBuiltinPostIncMember {
        MOTH_INSTR_HEADER
        Param base;
        VM::String *member;
        Param result;
    };
    struct instr_callBuiltinPostIncSubscript {
        MOTH_INSTR_HEADER
        Param base;
        Param index;
        Param result;
    };
    struct instr_callBuiltinPostIncName {
        MOTH_INSTR_HEADER
        VM::String *name;
        Param result;
    };
    struct instr_callBuiltinPostIncValue {
        MOTH_INSTR_HEADER
        Param value;
        Param result;
    };
    struct instr_callBuiltinPostDecMember {
        MOTH_INSTR_HEADER
        Param base;
        VM::String *member;
        Param result;
    };
    struct instr_callBuiltinPostDecSubscript {
        MOTH_INSTR_HEADER
        Param base;
        Param index;
        Param result;
    };
    struct instr_callBuiltinPostDecName {
        MOTH_INSTR_HEADER
        VM::String *name;
        Param result;
    };
    struct instr_callBuiltinPostDecValue {
        MOTH_INSTR_HEADER
        Param value;
        Param result;
    };
    struct instr_callBuiltinDeclareVar {
        MOTH_INSTR_HEADER
        VM::String *varName;
        bool isDeletable;
    };
    struct instr_callBuiltinDefineGetterSetter {
        MOTH_INSTR_HEADER
        VM::String *name;
        Param object;
        Param getter;
        Param setter;
    };
    struct instr_callBuiltinDefineProperty {
        MOTH_INSTR_HEADER
        VM::String *name;
        Param object;
        Param value;
    };
    struct instr_callBuiltinDefineArray {
        MOTH_INSTR_HEADER
        quint32 argc;
        quint32 args;
        Param result;
    };
    struct instr_createValue {
        MOTH_INSTR_HEADER
        quint32 argc;
        quint32 args;
        Param func;
        Param result;
    };
    struct instr_createProperty {
        MOTH_INSTR_HEADER
        VM::String *name;
        quint32 argc;
        quint32 args;
        Param base;
        Param result;
    };
    struct instr_createActivationProperty {
        MOTH_INSTR_HEADER
        VM::String *name;
        quint32 argc;
        quint32 args;
        Param result;
    };
    struct instr_jump {
        MOTH_INSTR_HEADER
        ptrdiff_t offset; 
    };
    struct instr_cjump {
        MOTH_INSTR_HEADER
        ptrdiff_t offset;
        Param condition;
    };
    struct instr_unop {
        MOTH_INSTR_HEADER
        VM::UnaryOpName alu;
        Param source;
        Param result;
    };
    struct instr_binop {
        MOTH_INSTR_HEADER
        VM::BinOp alu;
        Param lhs;
        Param rhs;
        Param result;
    };
    struct instr_loadThis {
        MOTH_INSTR_HEADER
        Param result;
    };
    struct instr_inplaceElementOp {
        MOTH_INSTR_HEADER
        VM::InplaceBinOpElement alu;
        Param base;
        Param index;
        Param source;
    };
    struct instr_inplaceMemberOp {
        MOTH_INSTR_HEADER
        VM::InplaceBinOpMember alu;
        VM::String *member;
        Param base;
        Param source;
    };
    struct instr_inplaceNameOp {
        MOTH_INSTR_HEADER
        VM::InplaceBinOpName alu;
        VM::String *name;
        Param source;
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
    instr_callElement callElement;
    instr_callActivationProperty callActivationProperty;
    instr_callBuiltinThrow callBuiltinThrow;
    instr_callBuiltinCreateExceptionHandler callBuiltinCreateExceptionHandler;
    instr_callBuiltinFinishTry callBuiltinFinishTry;
    instr_callBuiltinGetException callBuiltinGetException;
    instr_callBuiltinPushScope callBuiltinPushScope;
    instr_callBuiltinPushCatchScope callBuiltinPushCatchScope;
    instr_callBuiltinPopScope callBuiltinPopScope;
    instr_callBuiltinForeachIteratorObject callBuiltinForeachIteratorObject;
    instr_callBuiltinForeachNextPropertyName callBuiltinForeachNextPropertyName;
    instr_callBuiltinDeleteMember callBuiltinDeleteMember;
    instr_callBuiltinDeleteSubscript callBuiltinDeleteSubscript;
    instr_callBuiltinDeleteName callBuiltinDeleteName;
    instr_callBuiltinTypeofMember callBuiltinTypeofMember;
    instr_callBuiltinTypeofSubscript callBuiltinTypeofSubscript;
    instr_callBuiltinTypeofName callBuiltinTypeofName;
    instr_callBuiltinTypeofValue callBuiltinTypeofValue;
    instr_callBuiltinPostIncMember callBuiltinPostIncMember;
    instr_callBuiltinPostIncSubscript callBuiltinPostIncSubscript;
    instr_callBuiltinPostIncName callBuiltinPostIncName;
    instr_callBuiltinPostIncValue callBuiltinPostIncValue;
    instr_callBuiltinPostDecMember callBuiltinPostDecMember;
    instr_callBuiltinPostDecSubscript callBuiltinPostDecSubscript;
    instr_callBuiltinPostDecName callBuiltinPostDecName;
    instr_callBuiltinPostDecValue callBuiltinPostDecValue;
    instr_callBuiltinDeclareVar callBuiltinDeclareVar;
    instr_callBuiltinDefineGetterSetter callBuiltinDefineGetterSetter;
    instr_callBuiltinDefineProperty callBuiltinDefineProperty;
    instr_callBuiltinDefineArray callBuiltinDefineArray;
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
