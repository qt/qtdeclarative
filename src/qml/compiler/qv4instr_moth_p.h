/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV4INSTR_MOTH_P_H
#define QV4INSTR_MOTH_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//
#include <private/qv4global_p.h>
#include <private/qv4value_p.h>
#include <private/qv4runtime_p.h>

#if !defined(V4_BOOTSTRAP)
QT_REQUIRE_CONFIG(qml_interpreter);
#endif

QT_BEGIN_NAMESPACE

#ifdef QT_NO_QML_DEBUGGER
#define MOTH_DEBUG_INSTR(F)
#else
#define MOTH_DEBUG_INSTR(F) \
    F(Line, line) \
    F(Debug, debug)
#endif

#define FOR_EACH_MOTH_INSTR(F) \
    F(Ret, ret) \
    MOTH_DEBUG_INSTR(F) \
    F(LoadConst, loadConst) \
    F(MoveConst, moveConst) \
    F(LoadReg, loadReg) \
    F(StoreReg, storeReg) \
    F(MoveReg, moveReg) \
    F(LoadLocal, loadLocal) \
    F(StoreLocal, storeLocal) \
    F(LoadArg, loadArg) \
    F(StoreArg, storeArg) \
    F(LoadScopedLocal, loadScopedLocal) \
    F(StoreScopedLocal, storeScopedLocal) \
    F(LoadScopedArg, loadScopedArg) \
    F(StoreScopedArg, storeScopedArg) \
    F(LoadRuntimeString, loadRuntimeString) \
    F(LoadRegExp, loadRegExp) \
    F(LoadClosure, loadClosure) \
    F(LoadName, loadName) \
    F(GetGlobalLookup, getGlobalLookup) \
    F(StoreName, storeName) \
    F(LoadElement, loadElement) \
    F(LoadElementLookup, loadElementLookup) \
    F(StoreElement, storeElement) \
    F(StoreElementLookup, storeElementLookup) \
    F(LoadProperty, loadProperty) \
    F(GetLookup, getLookup) \
    F(StoreProperty, storeProperty) \
    F(SetLookup, setLookup) \
    F(StoreScopeObjectProperty, storeScopeObjectProperty) \
    F(StoreContextObjectProperty, storeContextObjectProperty) \
    F(LoadScopeObjectProperty, loadScopeObjectProperty) \
    F(LoadContextObjectProperty, loadContextObjectProperty) \
    F(LoadIdObject, loadIdObject) \
    F(InitStackFrame, initStackFrame) \
    F(CallValue, callValue) \
    F(CallProperty, callProperty) \
    F(CallPropertyLookup, callPropertyLookup) \
    F(CallElement, callElement) \
    F(CallActivationProperty, callActivationProperty) \
    F(CallGlobalLookup, callGlobalLookup) \
    F(SetExceptionHandler, setExceptionHandler) \
    F(CallBuiltinThrow, callBuiltinThrow) \
    F(GetException, getException) \
    F(SetException, setException) \
    F(CallBuiltinUnwindException, callBuiltinUnwindException) \
    F(CallBuiltinPushCatchScope, callBuiltinPushCatchScope) \
    F(CallBuiltinPushScope, callBuiltinPushScope) \
    F(CallBuiltinPopScope, callBuiltinPopScope) \
    F(CallBuiltinForeachIteratorObject, callBuiltinForeachIteratorObject) \
    F(CallBuiltinForeachNextPropertyName, callBuiltinForeachNextPropertyName) \
    F(CallBuiltinDeleteMember, callBuiltinDeleteMember) \
    F(CallBuiltinDeleteSubscript, callBuiltinDeleteSubscript) \
    F(CallBuiltinDeleteName, callBuiltinDeleteName) \
    F(CallBuiltinTypeofName, callBuiltinTypeofName) \
    F(CallBuiltinTypeofValue, callBuiltinTypeofValue) \
    F(CallBuiltinDeclareVar, callBuiltinDeclareVar) \
    F(CallBuiltinDefineArray, callBuiltinDefineArray) \
    F(CallBuiltinDefineObjectLiteral, callBuiltinDefineObjectLiteral) \
    F(CallBuiltinSetupArgumentsObject, callBuiltinSetupArgumentsObject) \
    F(CallBuiltinConvertThisToObject, callBuiltinConvertThisToObject) \
    F(CreateValue, createValue) \
    F(CreateProperty, createProperty) \
    F(ConstructPropertyLookup, constructPropertyLookup) \
    F(CreateActivationProperty, createActivationProperty) \
    F(ConstructGlobalLookup, constructGlobalLookup) \
    F(Jump, jump) \
    F(JumpEq, jumpEq) \
    F(JumpNe, jumpNe) \
    F(JumpStrictEqual, jumpStrictEqual) \
    F(JumpStrictNotEqual, jumpStrictNotEqual) \
    F(UNot, unot) \
    F(UNotBool, unotBool) \
    F(UPlus, uplus) \
    F(UMinus, uminus) \
    F(UCompl, ucompl) \
    F(UComplInt, ucomplInt) \
    F(Increment, increment) \
    F(Decrement, decrement) \
    F(Binop, binop) \
    F(Add, add) \
    F(BitAnd, bitAnd) \
    F(BitOr, bitOr) \
    F(BitXor, bitXor) \
    F(Shr, shr) \
    F(Shl, shl) \
    F(BitAndConst, bitAndConst) \
    F(BitOrConst, bitOrConst) \
    F(BitXorConst, bitXorConst) \
    F(ShrConst, shrConst) \
    F(ShlConst, shlConst) \
    F(Mul, mul) \
    F(Sub, sub) \
    F(BinopContext, binopContext) \
    F(LoadThis, loadThis) \
    F(LoadQmlContext, loadQmlContext) \
    F(LoadQmlImportedScripts, loadQmlImportedScripts) \
    F(LoadQmlSingleton, loadQmlSingleton)

#if defined(Q_CC_GNU) && (!defined(Q_CC_INTEL) || __INTEL_COMPILER >= 1200)
#  define MOTH_THREADED_INTERPRETER
#endif

#define MOTH_INSTR_ALIGN_MASK (Q_ALIGNOF(QV4::Moth::Instr) - 1)

#define MOTH_INSTR_HEADER union { Instr::Type instructionType; quint64 _dummy; };

#define MOTH_INSTR_ENUM(I, FMT)  I,
#define MOTH_INSTR_SIZE(I, FMT) ((sizeof(QV4::Moth::Instr::instr_##FMT) + MOTH_INSTR_ALIGN_MASK) & ~MOTH_INSTR_ALIGN_MASK)


namespace QV4 {
namespace Moth {

struct Temp {
    int index;

    static Temp create(int index) {
        Temp t;
        t.index = index;
        return t;
    }
};

inline bool operator==(const Temp &l, const Temp &r) { return l.index == r.index; }
inline bool operator!=(const Temp &l, const Temp &r) { return l.index != r.index; }

// When making changes to the instructions, make sure to bump QV4_DATA_STRUCTURE_VERSION in qv4compileddata_p.h

void dumpConstantTable(const Value *constants, uint count);
void dumpBytecode(const char *bytecode, int len);
inline void dumpBytecode(const QByteArray &bytecode) {
    dumpBytecode(bytecode.constData(), bytecode.length());
}

union Instr
{
    enum Type {
        FOR_EACH_MOTH_INSTR(MOTH_INSTR_ENUM)
        LastInstruction
    };

    struct instr_common {
        MOTH_INSTR_HEADER
    };
    struct instr_ret {
        MOTH_INSTR_HEADER
        Temp result;
    };

#ifndef QT_NO_QML_DEBUGGING
    struct instr_line {
        MOTH_INSTR_HEADER
        qint32 lineNumber;
    };
    struct instr_debug {
        MOTH_INSTR_HEADER
        qint32 lineNumber;
    };
#endif // QT_NO_QML_DEBUGGING

    struct instr_loadConst {
        MOTH_INSTR_HEADER
        int index;
    };
    struct instr_moveConst {
        MOTH_INSTR_HEADER
        int constIndex;
        Temp destTemp;
    };
    struct instr_loadReg {
        MOTH_INSTR_HEADER
        Temp reg;
    };
    struct instr_storeReg {
        MOTH_INSTR_HEADER
        Temp reg;
    };
    struct instr_moveReg {
        MOTH_INSTR_HEADER
        Temp srcReg;
        Temp destReg;
    };
    struct instr_loadLocal {
        MOTH_INSTR_HEADER
        int index;
    };
    struct instr_storeLocal {
        MOTH_INSTR_HEADER
        int index;
    };
    struct instr_loadArg {
        MOTH_INSTR_HEADER
        int index;
    };
    struct instr_storeArg {
        MOTH_INSTR_HEADER
        int index;
    };
    struct instr_loadScopedLocal {
        MOTH_INSTR_HEADER
        unsigned scope : 12;
        unsigned index : 20;
    };
    struct instr_storeScopedLocal {
        MOTH_INSTR_HEADER
        unsigned scope : 12;
        unsigned index : 20;
    };
    struct instr_loadScopedArg {
        MOTH_INSTR_HEADER
        unsigned scope : 12;
        unsigned index : 20;
    };
    struct instr_storeScopedArg {
        MOTH_INSTR_HEADER
        unsigned scope : 12;
        unsigned index : 20;
    };
    struct instr_loadRuntimeString {
        MOTH_INSTR_HEADER
        int stringId;
    };
    struct instr_loadRegExp {
        MOTH_INSTR_HEADER
        int regExpId;
    };
    struct instr_loadClosure {
        MOTH_INSTR_HEADER
        int value;
    };
    struct instr_loadName {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_getGlobalLookup {
        MOTH_INSTR_HEADER
        int index;
    };
    struct instr_storeName {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_loadProperty {
        MOTH_INSTR_HEADER
        int name;
        Temp base;
    };
    struct instr_getLookup {
        MOTH_INSTR_HEADER
        int index;
        Temp base;
    };
    struct instr_loadScopeObjectProperty {
        MOTH_INSTR_HEADER
        int propertyIndex;
        Temp base;
        bool captureRequired;
    };
    struct instr_loadContextObjectProperty {
        MOTH_INSTR_HEADER
        int propertyIndex;
        Temp base;
        bool captureRequired;
    };
    struct instr_loadIdObject {
        MOTH_INSTR_HEADER
        int index;
        Temp base;
    };
    struct instr_storeProperty {
        MOTH_INSTR_HEADER
        int name;
        Temp base;
    };
    struct instr_setLookup {
        MOTH_INSTR_HEADER
        int index;
        Temp base;
    };
    struct instr_storeScopeObjectProperty {
        MOTH_INSTR_HEADER
        Temp base;
        int propertyIndex;
    };
    struct instr_storeContextObjectProperty {
        MOTH_INSTR_HEADER
        Temp base;
        int propertyIndex;
    };
    struct instr_loadElement {
        MOTH_INSTR_HEADER
        Temp base;
        Temp index;
    };
    struct instr_loadElementLookup {
        MOTH_INSTR_HEADER
        uint lookup;
        Temp base;
        Temp index;
    };
    struct instr_storeElement {
        MOTH_INSTR_HEADER
        Temp base;
        Temp index;
    };
    struct instr_storeElementLookup {
        MOTH_INSTR_HEADER
        uint lookup;
        Temp base;
        Temp index;
    };
    struct instr_initStackFrame {
        MOTH_INSTR_HEADER
        int value;
    };
    struct instr_callValue {
        MOTH_INSTR_HEADER
        Temp callData;
        Temp dest;
    };
    struct instr_callProperty {
        MOTH_INSTR_HEADER
        int name;
        Temp callData;
        Temp base;
    };
    struct instr_callPropertyLookup {
        MOTH_INSTR_HEADER
        int lookupIndex;
        int argc;
        Temp callData;
        Temp base;
    };
    struct instr_callElement {
        MOTH_INSTR_HEADER
        Temp base;
        Temp index;
        Temp callData;
    };
    struct instr_callActivationProperty {
        MOTH_INSTR_HEADER
        int name;
        Temp callData;
    };
    struct instr_callGlobalLookup {
        MOTH_INSTR_HEADER
        int index;
        Temp callData;
    };
    struct instr_setExceptionHandler {
        MOTH_INSTR_HEADER
        qptrdiff offset;
    };
    struct instr_callBuiltinThrow {
        MOTH_INSTR_HEADER
    };
    struct instr_getException {
        MOTH_INSTR_HEADER
    };
    struct instr_setException {
        MOTH_INSTR_HEADER
    };
    struct instr_callBuiltinUnwindException {
        MOTH_INSTR_HEADER
    };
    struct instr_callBuiltinPushCatchScope {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_callBuiltinPushScope {
        MOTH_INSTR_HEADER
    };
    struct instr_callBuiltinPopScope {
        MOTH_INSTR_HEADER
    };
    struct instr_callBuiltinForeachIteratorObject {
        MOTH_INSTR_HEADER
    };
    struct instr_callBuiltinForeachNextPropertyName {
        MOTH_INSTR_HEADER
    };
    struct instr_callBuiltinDeleteMember {
        MOTH_INSTR_HEADER
        int member;
        Temp base;
    };
    struct instr_callBuiltinDeleteSubscript {
        MOTH_INSTR_HEADER
        Temp base;
        Temp index;
    };
    struct instr_callBuiltinDeleteName {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_callBuiltinTypeofName {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_callBuiltinTypeofValue {
        MOTH_INSTR_HEADER
    };
    struct instr_callBuiltinDeclareVar {
        MOTH_INSTR_HEADER
        int varName;
        bool isDeletable;
    };
    struct instr_callBuiltinDefineArray {
        MOTH_INSTR_HEADER
        uint argc;
        Temp args;
    };
    struct instr_callBuiltinDefineObjectLiteral {
        MOTH_INSTR_HEADER
        int internalClassId;
        int arrayValueCount;
        int arrayGetterSetterCountAndFlags; // 30 bits for count, 1 bit for needsSparseArray boolean
        Temp args;
    };
    struct instr_callBuiltinSetupArgumentsObject {
        MOTH_INSTR_HEADER
    };
    struct instr_callBuiltinConvertThisToObject {
        MOTH_INSTR_HEADER
    };
    struct instr_createValue {
        MOTH_INSTR_HEADER
        Temp callData;
        Temp func;
    };
    struct instr_createProperty {
        MOTH_INSTR_HEADER
        int name;
        int argc;
        Temp callData;
        Temp base;
    };
    struct instr_constructPropertyLookup {
        MOTH_INSTR_HEADER
        int index;
        int argc;
        Temp callData;
        Temp base;
    };
    struct instr_createActivationProperty {
        MOTH_INSTR_HEADER
        int name;
        int argc;
        Temp callData;
    };
    struct instr_constructGlobalLookup {
        MOTH_INSTR_HEADER
        int index;
        int argc;
        Temp callData;
    };
    struct instr_jump {
        MOTH_INSTR_HEADER
        ptrdiff_t offset;
    };
    struct instr_jumpEq {
        MOTH_INSTR_HEADER
        ptrdiff_t offset;
    };
    struct instr_jumpNe {
        MOTH_INSTR_HEADER
        ptrdiff_t offset;
    };
    struct instr_jumpStrictEqual {
        MOTH_INSTR_HEADER
        ptrdiff_t offset;
        Temp lhs;
    };
    struct instr_jumpStrictNotEqual {
        MOTH_INSTR_HEADER
        ptrdiff_t offset;
        Temp lhs;
    };
    struct instr_unot {
        MOTH_INSTR_HEADER
    };
    struct instr_unotBool {
        MOTH_INSTR_HEADER
    };
    struct instr_uplus {
        MOTH_INSTR_HEADER
    };
    struct instr_uminus {
        MOTH_INSTR_HEADER
    };
    struct instr_ucompl {
        MOTH_INSTR_HEADER
    };
    struct instr_ucomplInt {
        MOTH_INSTR_HEADER
    };
    struct instr_increment {
        MOTH_INSTR_HEADER
    };
    struct instr_decrement {
        MOTH_INSTR_HEADER
    };
    struct instr_binop {
        MOTH_INSTR_HEADER
        int alu; // QV4::Runtime::RuntimeMethods enum value
        Temp lhs;
    };
    struct instr_add {
        MOTH_INSTR_HEADER
        Temp lhs;
    };
    struct instr_bitAnd {
        MOTH_INSTR_HEADER
        Temp lhs;
    };
    struct instr_bitOr {
        MOTH_INSTR_HEADER
        Temp lhs;
    };
    struct instr_bitXor {
        MOTH_INSTR_HEADER
        Temp lhs;
    };
    struct instr_shr {
        MOTH_INSTR_HEADER
        Temp lhs;
    };
    struct instr_shl {
        MOTH_INSTR_HEADER
        Temp lhs;
    };
    struct instr_bitAndConst {
        MOTH_INSTR_HEADER
        int rhs;
    };
    struct instr_bitOrConst {
        MOTH_INSTR_HEADER
        int rhs;
    };
    struct instr_bitXorConst {
        MOTH_INSTR_HEADER
        int rhs;
    };
    struct instr_shrConst {
        MOTH_INSTR_HEADER
        int rhs;
    };
    struct instr_shlConst {
        MOTH_INSTR_HEADER
        int rhs;
    };
    struct instr_mul {
        MOTH_INSTR_HEADER
        Temp lhs;
    };
    struct instr_sub {
        MOTH_INSTR_HEADER
        Temp lhs;
    };
    struct instr_binopContext {
        MOTH_INSTR_HEADER
        uint alu; // offset inside the runtime methods
        Temp lhs;
    };
    struct instr_loadThis {
        MOTH_INSTR_HEADER
    };
    struct instr_loadQmlContext {
        MOTH_INSTR_HEADER
        Temp result;
    };
    struct instr_loadQmlImportedScripts {
        MOTH_INSTR_HEADER
        Temp result;
    };
    struct instr_loadQmlSingleton {
        MOTH_INSTR_HEADER
        int name;
    };

    instr_common common;
    instr_ret ret;
    instr_line line;
    instr_debug debug;
    instr_loadConst loadConst;
    instr_moveConst moveConst;
    instr_loadReg loadReg;
    instr_storeReg storeReg;
    instr_moveReg moveReg;
    instr_loadLocal loadLocal;
    instr_storeLocal storeLocal;
    instr_loadArg loadArg;
    instr_storeArg storeArg;
    instr_loadScopedLocal loadScopedLocal;
    instr_storeScopedLocal storeScopedLocal;
    instr_loadScopedArg loadScopedArg;
    instr_storeScopedArg storeScopedArg;
    instr_loadRuntimeString loadRuntimeString;
    instr_loadRegExp loadRegExp;
    instr_loadClosure loadClosure;
    instr_loadName loadName;
    instr_getGlobalLookup getGlobalLookup;
    instr_storeName storeName;
    instr_loadElement loadElement;
    instr_loadElementLookup loadElementLookup;
    instr_storeElement storeElement;
    instr_storeElementLookup storeElementLookup;
    instr_loadProperty loadProperty;
    instr_getLookup getLookup;
    instr_loadScopeObjectProperty loadScopeObjectProperty;
    instr_loadContextObjectProperty loadContextObjectProperty;
    instr_loadIdObject loadIdObject;
    instr_storeProperty storeProperty;
    instr_setLookup setLookup;
    instr_storeScopeObjectProperty storeScopeObjectProperty;
    instr_storeContextObjectProperty storeContextObjectProperty;
    instr_initStackFrame initStackFrame;
    instr_callValue callValue;
    instr_callProperty callProperty;
    instr_callPropertyLookup callPropertyLookup;
    instr_callElement callElement;
    instr_callActivationProperty callActivationProperty;
    instr_callGlobalLookup callGlobalLookup;
    instr_callBuiltinThrow callBuiltinThrow;
    instr_getException getException;
    instr_setException setException;
    instr_setExceptionHandler setExceptionHandler;
    instr_callBuiltinUnwindException callBuiltinUnwindException;
    instr_callBuiltinPushCatchScope callBuiltinPushCatchScope;
    instr_callBuiltinPushScope callBuiltinPushScope;
    instr_callBuiltinPopScope callBuiltinPopScope;
    instr_callBuiltinForeachIteratorObject callBuiltinForeachIteratorObject;
    instr_callBuiltinForeachNextPropertyName callBuiltinForeachNextPropertyName;
    instr_callBuiltinDeleteMember callBuiltinDeleteMember;
    instr_callBuiltinDeleteSubscript callBuiltinDeleteSubscript;
    instr_callBuiltinDeleteName callBuiltinDeleteName;
    instr_callBuiltinTypeofName callBuiltinTypeofName;
    instr_callBuiltinTypeofValue callBuiltinTypeofValue;
    instr_callBuiltinDeclareVar callBuiltinDeclareVar;
    instr_callBuiltinDefineArray callBuiltinDefineArray;
    instr_callBuiltinDefineObjectLiteral callBuiltinDefineObjectLiteral;
    instr_callBuiltinSetupArgumentsObject callBuiltinSetupArgumentsObject;
    instr_callBuiltinConvertThisToObject callBuiltinConvertThisToObject;
    instr_createValue createValue;
    instr_createProperty createProperty;
    instr_constructPropertyLookup constructPropertyLookup;
    instr_createActivationProperty createActivationProperty;
    instr_constructGlobalLookup constructGlobalLookup;
    instr_jump jump;
    instr_jumpEq jumpEq;
    instr_jumpNe jumpNe;
    instr_jumpStrictEqual jumpStrictEqual;
    instr_jumpStrictNotEqual jumpStrictNotEqual;
    instr_unot unot;
    instr_unotBool unotBool;
    instr_uplus uplus;
    instr_uminus uminus;
    instr_ucompl ucompl;
    instr_ucomplInt ucomplInt;
    instr_increment increment;
    instr_decrement decrement;
    instr_binop binop;
    instr_add add;
    instr_bitAnd bitAnd;
    instr_bitOr bitOr;
    instr_bitXor bitXor;
    instr_shr shr;
    instr_shl shl;
    instr_bitAndConst bitAndConst;
    instr_bitOrConst bitOrConst;
    instr_bitXorConst bitXorConst;
    instr_shrConst shrConst;
    instr_shlConst shlConst;
    instr_mul mul;
    instr_sub sub;
    instr_binopContext binopContext;
    instr_loadThis loadThis;
    instr_loadQmlContext loadQmlContext;
    instr_loadQmlImportedScripts loadQmlImportedScripts;
    instr_loadQmlSingleton loadQmlSingleton;

    static int size(Type type);
};

template<int N>
struct InstrMeta {
};

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wuninitialized")
#define MOTH_INSTR_META_TEMPLATE(I, FMT) \
    template<> struct InstrMeta<(int)Instr::I> { \
        enum { Size = MOTH_INSTR_SIZE(I, FMT) }; \
        typedef Instr::instr_##FMT DataType; \
        static const DataType &data(const Instr &instr) { return instr.FMT; } \
        static void setData(Instr &instr, const DataType &v) { instr.FMT = v; } \
        static void setDataNoCommon(Instr &instr, const DataType &v) \
        { memcpy(reinterpret_cast<char *>(&instr.FMT) + sizeof(Instr::instr_common), \
                 reinterpret_cast<const char *>(&v) + sizeof(Instr::instr_common), \
                 Size - sizeof(Instr::instr_common)); } \
    };
FOR_EACH_MOTH_INSTR(MOTH_INSTR_META_TEMPLATE);
#undef MOTH_INSTR_META_TEMPLATE
QT_WARNING_POP

template<int InstrType>
class InstrData : public InstrMeta<InstrType>::DataType
{
};

struct Instruction {
#define MOTH_INSTR_DATA_TYPEDEF(I, FMT) typedef InstrData<Instr::I> I;
FOR_EACH_MOTH_INSTR(MOTH_INSTR_DATA_TYPEDEF)
#undef MOTH_INSTR_DATA_TYPEDEF
private:
    Instruction();
};

} // namespace Moth
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4INSTR_MOTH_P_H
