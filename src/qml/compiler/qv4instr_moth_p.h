/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV4INSTR_MOTH_P_H
#define QV4INSTR_MOTH_P_H

#include <QtCore/qglobal.h>
#include <private/qv4value_def_p.h>
#include <private/qv4function_p.h>
#include <private/qv4runtime_p.h>

QT_BEGIN_NAMESPACE

#define FOR_EACH_MOTH_INSTR(F) \
    F(Ret, ret) \
    F(LoadValue, loadValue) \
    F(LoadRuntimeString, loadRuntimeString) \
    F(LoadRegExp, loadRegExp) \
    F(LoadClosure, loadClosure) \
    F(MoveTemp, moveTemp) \
    F(SwapTemps, swapTemps) \
    F(LoadName, loadName) \
    F(StoreName, storeName) \
    F(LoadElement, loadElement) \
    F(StoreElement, storeElement) \
    F(LoadProperty, loadProperty) \
    F(StoreProperty, storeProperty) \
    F(Push, push) \
    F(EnterTry, enterTry) \
    F(CallValue, callValue) \
    F(CallProperty, callProperty) \
    F(CallElement, callElement) \
    F(CallActivationProperty, callActivationProperty) \
    F(CallBuiltinThrow, callBuiltinThrow) \
    F(CallBuiltinFinishTry, callBuiltinFinishTry) \
    F(CallBuiltinPushScope, callBuiltinPushScope) \
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
    F(CallBuiltinDeclareVar, callBuiltinDeclareVar) \
    F(CallBuiltinDefineGetterSetter, callBuiltinDefineGetterSetter) \
    F(CallBuiltinDefineProperty, callBuiltinDefineProperty) \
    F(CallBuiltinDefineArray, callBuiltinDefineArray) \
    F(CallBuiltinDefineObjectLiteral, callBuiltinDefineObjectLiteral) \
    F(CallBuiltinSetupArgumentsObject, callBuiltinSetupArgumentsObject) \
    F(CreateValue, createValue) \
    F(CreateProperty, createProperty) \
    F(CreateActivationProperty, createActivationProperty) \
    F(Jump, jump) \
    F(CJump, cjump) \
    F(Unop, unop) \
    F(Binop, binop) \
    F(BinopContext, binopContext) \
    F(AddNumberParams, addNumberParams) \
    F(MulNumberParams, mulNumberParams) \
    F(SubNumberParams, subNumberParams) \
    F(LoadThis, loadThis)

#if defined(Q_CC_GNU) && (!defined(Q_CC_INTEL) || __INTEL_COMPILER >= 1200)
#  define MOTH_THREADED_INTERPRETER
#endif

#define MOTH_INSTR_ALIGN_MASK (Q_ALIGNOF(QQmlJS::Moth::Instr) - 1)

#ifdef MOTH_THREADED_INTERPRETER
#  define MOTH_INSTR_HEADER void *code; \
                            unsigned int breakPoint : 1;
#else
#  define MOTH_INSTR_HEADER quint8 instructionType; \
                            unsigned int breakPoint : 1;
#endif

#define MOTH_INSTR_ENUM(I, FMT)  I,
#define MOTH_INSTR_SIZE(I, FMT) ((sizeof(QQmlJS::Moth::Instr::instr_##FMT) + MOTH_INSTR_ALIGN_MASK) & ~MOTH_INSTR_ALIGN_MASK)


namespace QQmlJS {
namespace Moth {

struct Param {
    enum {
        ValueType    = 0,
        ArgumentType = 1,
        LocalType    = 2,
        TempType     = 3,
        ScopedLocalType  = 4
    };
    QV4::Primitive value;
    unsigned type  : 3;
    unsigned scope : 29;
    unsigned index;

    bool isValue() const { return type == ValueType; }
    bool isArgument() const { return type == ArgumentType; }
    bool isLocal() const { return type == LocalType; }
    bool isTemp() const { return type == TempType; }
    bool isScopedLocal() const { return type == ScopedLocalType; }

    static Param createValue(const QV4::Primitive &v)
    {
        Param p;
        p.type = ValueType;
        p.scope = 0;
        p.value = v;
        return p;
    }

    static Param createArgument(unsigned idx, uint scope)
    {
        Param p;
        p.type = ArgumentType;
        p.scope = scope;
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

    inline bool operator==(const Param &other) const
    { return type == other.type && scope == other.scope && index == other.index; }

    inline bool operator!=(const Param &other) const
    { return !(*this == other); }
};

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
        Param result;
    }; 
    struct instr_loadValue {
        MOTH_INSTR_HEADER
        Param value;
        Param result;
    };
    struct instr_loadRuntimeString {
        MOTH_INSTR_HEADER
        int stringId;
        Param result;
    };
    struct instr_loadRegExp {
        MOTH_INSTR_HEADER
        int regExpId;
        Param result;
    };
    struct instr_moveTemp {
        MOTH_INSTR_HEADER
        Param source;
        Param result;
    };
    struct instr_swapTemps {
        MOTH_INSTR_HEADER
        Param left;
        Param right;
    };
    struct instr_loadClosure {
        MOTH_INSTR_HEADER
        int value;
        Param result;
    };
    struct instr_loadName {
        MOTH_INSTR_HEADER
        int name;
        Param result;
    };
    struct instr_storeName {
        MOTH_INSTR_HEADER
        int name;
        Param source;
    };
    struct instr_loadProperty {
        MOTH_INSTR_HEADER
        int name;
        Param base;
        Param result;
    };
    struct instr_storeProperty {
        MOTH_INSTR_HEADER
        int name;
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
    struct instr_enterTry {
        MOTH_INSTR_HEADER
        ptrdiff_t tryOffset;
        ptrdiff_t catchOffset;
        int exceptionVarName;
        Param exceptionVar;
    };
    struct instr_callValue {
        MOTH_INSTR_HEADER
        quint32 argc;
        quint32 callData;
        Param dest;
        Param result;
    };
    struct instr_callProperty {
        MOTH_INSTR_HEADER
        int name;
        quint32 argc;
        quint32 callData;
        Param base;
        Param result;
    };
    struct instr_callElement {
        MOTH_INSTR_HEADER
        Param base;
        Param index;
        quint32 argc;
        quint32 callData;
        Param result;
    };
    struct instr_callActivationProperty {
        MOTH_INSTR_HEADER
        int name;
        quint32 argc;
        quint32 callData;
        Param result;
    };
    struct instr_callBuiltinThrow {
        MOTH_INSTR_HEADER
        Param arg;
    };
    struct instr_callBuiltinFinishTry {
        MOTH_INSTR_HEADER
    };
    struct instr_callBuiltinPushScope {
        MOTH_INSTR_HEADER
        Param arg;
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
        int member;
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
        int name;
        Param result;
    };
    struct instr_callBuiltinTypeofMember {
        MOTH_INSTR_HEADER
        int member;
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
        int name;
        Param result;
    };
    struct instr_callBuiltinTypeofValue {
        MOTH_INSTR_HEADER
        Param value;
        Param result;
    };
    struct instr_callBuiltinDeclareVar {
        MOTH_INSTR_HEADER
        int varName;
        bool isDeletable;
    };
    struct instr_callBuiltinDefineGetterSetter {
        MOTH_INSTR_HEADER
        int name;
        Param object;
        Param getter;
        Param setter;
    };
    struct instr_callBuiltinDefineProperty {
        MOTH_INSTR_HEADER
        int name;
        Param object;
        Param value;
    };
    struct instr_callBuiltinDefineArray {
        MOTH_INSTR_HEADER
        quint32 argc;
        quint32 args;
        Param result;
    };
    struct instr_callBuiltinDefineObjectLiteral {
        MOTH_INSTR_HEADER
        int internalClassId;
        quint32 args;
        Param result;
    };
    struct instr_callBuiltinSetupArgumentsObject {
        MOTH_INSTR_HEADER
        Param result;
    };
    struct instr_createValue {
        MOTH_INSTR_HEADER
        quint32 argc;
        quint32 callData;
        Param func;
        Param result;
    };
    struct instr_createProperty {
        MOTH_INSTR_HEADER
        int name;
        quint32 argc;
        quint32 callData;
        Param base;
        Param result;
    };
    struct instr_createActivationProperty {
        MOTH_INSTR_HEADER
        int name;
        quint32 argc;
        quint32 callData;
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
        bool invert;
    };
    struct instr_unop {
        MOTH_INSTR_HEADER
        QV4::UnaryOpName alu;
        Param source;
        Param result;
    };
    struct instr_binop {
        MOTH_INSTR_HEADER
        QV4::BinOp alu;
        Param lhs;
        Param rhs;
        Param result;
    };
    struct instr_binopContext {
        MOTH_INSTR_HEADER
        QV4::BinOpContext alu;
        Param lhs;
        Param rhs;
        Param result;
    };
    struct instr_addNumberParams {
        MOTH_INSTR_HEADER
        Param lhs;
        Param rhs;
        Param result;
    };
    struct instr_mulNumberParams {
        MOTH_INSTR_HEADER
        Param lhs;
        Param rhs;
        Param result;
    };
    struct instr_subNumberParams {
        MOTH_INSTR_HEADER
        Param lhs;
        Param rhs;
        Param result;
    };
    struct instr_loadThis {
        MOTH_INSTR_HEADER
        Param result;
    };

    instr_common common;
    instr_ret ret;
    instr_loadValue loadValue;
    instr_loadRuntimeString loadRuntimeString;
    instr_loadRegExp loadRegExp;
    instr_moveTemp moveTemp;
    instr_swapTemps swapTemps;
    instr_loadClosure loadClosure;
    instr_loadName loadName;
    instr_storeName storeName;
    instr_loadElement loadElement;
    instr_storeElement storeElement;
    instr_loadProperty loadProperty;
    instr_storeProperty storeProperty;
    instr_push push;
    instr_enterTry enterTry;
    instr_callValue callValue;
    instr_callProperty callProperty;
    instr_callElement callElement;
    instr_callActivationProperty callActivationProperty;
    instr_callBuiltinThrow callBuiltinThrow;
    instr_callBuiltinFinishTry callBuiltinFinishTry;
    instr_callBuiltinPushScope callBuiltinPushScope;
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
    instr_callBuiltinDeclareVar callBuiltinDeclareVar;
    instr_callBuiltinDefineGetterSetter callBuiltinDefineGetterSetter;
    instr_callBuiltinDefineProperty callBuiltinDefineProperty;
    instr_callBuiltinDefineArray callBuiltinDefineArray;
    instr_callBuiltinDefineObjectLiteral callBuiltinDefineObjectLiteral;
    instr_callBuiltinSetupArgumentsObject callBuiltinSetupArgumentsObject;
    instr_createValue createValue;
    instr_createProperty createProperty;
    instr_createActivationProperty createActivationProperty;
    instr_jump jump;
    instr_cjump cjump;
    instr_unop unop;
    instr_binop binop;
    instr_binopContext binopContext;
    instr_addNumberParams addNumberParams;
    instr_mulNumberParams mulNumberParams;
    instr_subNumberParams subNumberParams;
    instr_loadThis loadThis;

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

QT_END_NAMESPACE

#endif // QV4INSTR_MOTH_P_H
