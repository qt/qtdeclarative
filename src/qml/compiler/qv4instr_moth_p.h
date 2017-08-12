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
    F(LoadTrue, loadTrue) \
    F(LoadFalse, loadFalse) \
    F(LoadNull, loadNull) \
    F(LoadUndefined, loadUndefined) \
    F(LoadInt, loadInt) \
    F(MoveConst, moveConst) \
    F(LoadReg, loadReg) \
    F(StoreReg, storeReg) \
    F(MoveReg, moveReg) \
    F(LoadScopedLocal, loadScopedLocal) \
    F(StoreScopedLocal, storeScopedLocal) \
    F(LoadScopedArgument, loadScopedArgument) \
    F(StoreScopedArgument, storeScopedArgument) \
    F(LoadRuntimeString, loadRuntimeString) \
    F(LoadRegExp, loadRegExp) \
    F(LoadClosure, loadClosure) \
    F(LoadName, loadName) \
    F(LoadGlobalLookup, loadGlobalLookup) \
    F(StoreNameSloppy, storeNameSloppy) \
    F(StoreNameStrict, storeNameStrict) \
    F(LoadElement, loadElement) \
    F(LoadElementA, loadElementA) \
    F(StoreElement, storeElement) \
    F(LoadProperty, loadProperty) \
    F(LoadPropertyA, loadPropertyA) \
    F(GetLookup, getLookup) \
    F(GetLookupA, getLookupA) \
    F(StoreProperty, storeProperty) \
    F(SetLookup, setLookup) \
    F(StoreScopeObjectProperty, storeScopeObjectProperty) \
    F(StoreContextObjectProperty, storeContextObjectProperty) \
    F(LoadScopeObjectProperty, loadScopeObjectProperty) \
    F(LoadContextObjectProperty, loadContextObjectProperty) \
    F(LoadIdObject, loadIdObject) \
    F(CallValue, callValue) \
    F(CallProperty, callProperty) \
    F(CallPropertyLookup, callPropertyLookup) \
    F(CallElement, callElement) \
    F(CallName, callName) \
    F(CallGlobalLookup, callGlobalLookup) \
    F(SetExceptionHandler, setExceptionHandler) \
    F(ThrowException, throwException) \
    F(GetException, getException) \
    F(SetException, setException) \
    F(UnwindException, unwindException) \
    F(PushCatchContext, pushCatchContext) \
    F(PushWithContext, pushWithContext) \
    F(PopContext, popContext) \
    F(ForeachIteratorObject, foreachIteratorObject) \
    F(ForeachNextPropertyName, foreachNextPropertyName) \
    F(DeleteMember, deleteMember) \
    F(DeleteSubscript, deleteSubscript) \
    F(DeleteName, deleteName) \
    F(TypeofName, typeofName) \
    F(TypeofValue, typeofValue) \
    F(DeclareVar, declareVar) \
    F(DefineArray, defineArray) \
    F(DefineObjectLiteral, defineObjectLiteral) \
    F(CreateMappedArgumentsObject, createMappedArgumentsObject) \
    F(CreateUnmappedArgumentsObject, createUnmappedArgumentsObject) \
    F(ConvertThisToObject, convertThisToObject) \
    F(CreateValue, createValue) \
    F(CreateProperty, createProperty) \
    F(ConstructPropertyLookup, constructPropertyLookup) \
    F(CreateName, createName) \
    F(ConstructGlobalLookup, constructGlobalLookup) \
    F(Jump, jump) \
    F(JumpEq, jumpEq) \
    F(JumpNe, jumpNe) \
    F(CmpJmpEq, cmpJmpEq) \
    F(CmpJmpNe, cmpJmpNe) \
    F(CmpJmpGt, cmpJmpGt) \
    F(CmpJmpGe, cmpJmpGe) \
    F(CmpJmpLt, cmpJmpLt) \
    F(CmpJmpLe, cmpJmpLe) \
    F(JumpStrictEqual, jumpStrictEqual) \
    F(JumpStrictNotEqual, jumpStrictNotEqual) \
    F(JumpStrictEqualStackSlotInt, jumpStrictEqualStackSlotInt) \
    F(JumpStrictNotEqualStackSlotInt, jumpStrictNotEqualStackSlotInt) \
    F(UNot, unot) \
    F(UPlus, uplus) \
    F(UMinus, uminus) \
    F(UCompl, ucompl) \
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

class StackSlot {
    int index;

public:
    static StackSlot createRegister(int index) {
        Q_ASSERT(index >= 0);
        StackSlot t;
        t.index = index;
        return t;
    }

    static StackSlot createArgument(int nFormals, int index) {
        Q_ASSERT(index >= -1);
        StackSlot t;
        t.index = index - nFormals;
        return t;
    }

    bool isRegister() const { return index >= 0; }
    bool isArgument() const { return index < 0; }

    int argIndex() const {
        Q_ASSERT(isArgument());
        return -index - 1;
    }

    int stackSlot() const { return index; }

    QString dump(int nFormals) const {
        if (isRegister())
            return QStringLiteral("r%1").arg(index);

        if (nFormals + index == -1)
            return QStringLiteral("(this)");

        return QStringLiteral("a%1").arg(nFormals + index);
    }
};

inline bool operator==(const StackSlot &l, const StackSlot &r) { return l.stackSlot() == r.stackSlot(); }
inline bool operator!=(const StackSlot &l, const StackSlot &r) { return l.stackSlot() != r.stackSlot(); }

// When making changes to the instructions, make sure to bump QV4_DATA_STRUCTURE_VERSION in qv4compileddata_p.h

void dumpConstantTable(const Value *constants, uint count);
void dumpBytecode(const char *bytecode, int len, int nFormals);
inline void dumpBytecode(const QByteArray &bytecode, int nFormals) {
    dumpBytecode(bytecode.constData(), bytecode.length(), nFormals);
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
    struct instr_loadTrue {
        MOTH_INSTR_HEADER
    };
    struct instr_loadFalse {
        MOTH_INSTR_HEADER
    };
    struct instr_loadNull {
        MOTH_INSTR_HEADER
    };
    struct instr_loadUndefined {
        MOTH_INSTR_HEADER
    };
    struct instr_loadInt {
        MOTH_INSTR_HEADER
        int value;
    };
    struct instr_moveConst {
        MOTH_INSTR_HEADER
        int constIndex;
        StackSlot destTemp;
    };
    struct instr_loadReg {
        MOTH_INSTR_HEADER
        StackSlot reg;
    };
    struct instr_storeReg {
        MOTH_INSTR_HEADER
        StackSlot reg;
    };
    struct instr_moveReg {
        MOTH_INSTR_HEADER
        StackSlot srcReg;
        StackSlot destReg;
    };
    struct instr_loadScopedLocal {
        MOTH_INSTR_HEADER
        int scope;
        int index;
    };
    struct instr_storeScopedLocal {
        MOTH_INSTR_HEADER
        int scope;
        int index;
    };
    struct instr_loadScopedArgument {
        MOTH_INSTR_HEADER
        int scope;
        int index;
    };
    struct instr_storeScopedArgument {
        MOTH_INSTR_HEADER
        int scope;
        int index;
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
    struct instr_loadGlobalLookup {
        MOTH_INSTR_HEADER
        int index;
    };
    struct instr_storeNameSloppy {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_storeNameStrict {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_loadProperty {
        MOTH_INSTR_HEADER
        int name;
        StackSlot base;
    };
    struct instr_loadPropertyA {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_getLookup {
        MOTH_INSTR_HEADER
        int index;
        StackSlot base;
    };
    struct instr_getLookupA {
        MOTH_INSTR_HEADER
        int index;
    };
    struct instr_loadScopeObjectProperty {
        MOTH_INSTR_HEADER
        int propertyIndex;
        StackSlot base;
        bool captureRequired;
    };
    struct instr_loadContextObjectProperty {
        MOTH_INSTR_HEADER
        int propertyIndex;
        StackSlot base;
        bool captureRequired;
    };
    struct instr_loadIdObject {
        MOTH_INSTR_HEADER
        int index;
        StackSlot base;
    };
    struct instr_storeProperty {
        MOTH_INSTR_HEADER
        int name;
        StackSlot base;
    };
    struct instr_setLookup {
        MOTH_INSTR_HEADER
        int index;
        StackSlot base;
    };
    struct instr_storeScopeObjectProperty {
        MOTH_INSTR_HEADER
        StackSlot base;
        int propertyIndex;
    };
    struct instr_storeContextObjectProperty {
        MOTH_INSTR_HEADER
        StackSlot base;
        int propertyIndex;
    };
    struct instr_loadElement {
        MOTH_INSTR_HEADER
        StackSlot base;
        StackSlot index;
    };
    struct instr_loadElementA {
        MOTH_INSTR_HEADER
        StackSlot base;
    };
    struct instr_storeElement {
        MOTH_INSTR_HEADER
        StackSlot base;
        StackSlot index;
    };
    struct instr_callValue {
        MOTH_INSTR_HEADER
        StackSlot callData;
    };
    struct instr_callProperty {
        MOTH_INSTR_HEADER
        int name;
        StackSlot callData;
        StackSlot base;
    };
    struct instr_callPropertyLookup {
        MOTH_INSTR_HEADER
        int lookupIndex;
        StackSlot callData;
        StackSlot base;
    };
    struct instr_callElement {
        MOTH_INSTR_HEADER
        StackSlot base;
        StackSlot index;
        StackSlot callData;
    };
    struct instr_callName {
        MOTH_INSTR_HEADER
        int name;
        StackSlot callData;
    };
    struct instr_callGlobalLookup {
        MOTH_INSTR_HEADER
        int index;
        StackSlot callData;
    };
    struct instr_setExceptionHandler {
        MOTH_INSTR_HEADER
        qptrdiff offset;
    };
    struct instr_throwException {
        MOTH_INSTR_HEADER
    };
    struct instr_getException {
        MOTH_INSTR_HEADER
    };
    struct instr_setException {
        MOTH_INSTR_HEADER
    };
    struct instr_unwindException {
        MOTH_INSTR_HEADER
    };
    struct instr_pushCatchContext {
        MOTH_INSTR_HEADER
        int name;
        StackSlot reg;
    };
    struct instr_pushWithContext {
        MOTH_INSTR_HEADER
        StackSlot reg;
    };
    struct instr_popContext {
        MOTH_INSTR_HEADER
        StackSlot reg;
    };
    struct instr_foreachIteratorObject {
        MOTH_INSTR_HEADER
    };
    struct instr_foreachNextPropertyName {
        MOTH_INSTR_HEADER
    };
    struct instr_deleteMember {
        MOTH_INSTR_HEADER
        int member;
        StackSlot base;
    };
    struct instr_deleteSubscript {
        MOTH_INSTR_HEADER
        StackSlot base;
        StackSlot index;
    };
    struct instr_deleteName {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_typeofName {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_typeofValue {
        MOTH_INSTR_HEADER
    };
    struct instr_declareVar {
        MOTH_INSTR_HEADER
        int varName;
        bool isDeletable;
    };
    struct instr_defineArray {
        MOTH_INSTR_HEADER
        uint argc;
        StackSlot args;
    };
    struct instr_defineObjectLiteral {
        MOTH_INSTR_HEADER
        int internalClassId;
        int arrayValueCount;
        int arrayGetterSetterCountAndFlags; // 30 bits for count, 1 bit for needsSparseArray boolean
        StackSlot args;
    };
    struct instr_createMappedArgumentsObject {
        MOTH_INSTR_HEADER
    };
    struct instr_createUnmappedArgumentsObject {
        MOTH_INSTR_HEADER
    };
    struct instr_convertThisToObject {
        MOTH_INSTR_HEADER
    };
    struct instr_createValue {
        MOTH_INSTR_HEADER
        StackSlot callData;
        StackSlot func;
    };
    struct instr_createProperty {
        MOTH_INSTR_HEADER
        int name;
        int argc;
        StackSlot callData;
        StackSlot base;
    };
    struct instr_constructPropertyLookup {
        MOTH_INSTR_HEADER
        int index;
        int argc;
        StackSlot callData;
        StackSlot base;
    };
    struct instr_createName {
        MOTH_INSTR_HEADER
        int name;
        int argc;
        StackSlot callData;
    };
    struct instr_constructGlobalLookup {
        MOTH_INSTR_HEADER
        int index;
        int argc;
        StackSlot callData;
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
    struct instr_cmpJmpEq {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        ptrdiff_t offset;
    };
    struct instr_cmpJmpNe {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        ptrdiff_t offset;
    };
    struct instr_cmpJmpGt {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        ptrdiff_t offset;
    };
    struct instr_cmpJmpGe {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        ptrdiff_t offset;
    };
    struct instr_cmpJmpLt {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        ptrdiff_t offset;
    };
    struct instr_cmpJmpLe {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        ptrdiff_t offset;
    };
    struct instr_jumpStrictEqual {
        MOTH_INSTR_HEADER
        ptrdiff_t offset;
        StackSlot lhs;
    };
    struct instr_jumpStrictNotEqual {
        MOTH_INSTR_HEADER
        ptrdiff_t offset;
        StackSlot lhs;
    };
    struct instr_jumpStrictEqualStackSlotInt {
        MOTH_INSTR_HEADER
        ptrdiff_t offset;
        StackSlot lhs;
        int rhs;
    };
    struct instr_jumpStrictNotEqualStackSlotInt {
        MOTH_INSTR_HEADER
        ptrdiff_t offset;
        StackSlot lhs;
        int rhs;
    };
    struct instr_unot {
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
    struct instr_increment {
        MOTH_INSTR_HEADER
    };
    struct instr_decrement {
        MOTH_INSTR_HEADER
    };
    struct instr_binop {
        MOTH_INSTR_HEADER
        int alu; // QV4::Runtime::RuntimeMethods enum value
        StackSlot lhs;
    };
    struct instr_add {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_bitAnd {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_bitOr {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_bitXor {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_shr {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_shl {
        MOTH_INSTR_HEADER
        StackSlot lhs;
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
        StackSlot lhs;
    };
    struct instr_sub {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_binopContext {
        MOTH_INSTR_HEADER
        uint alu; // offset inside the runtime methods
        StackSlot lhs;
    };
    struct instr_loadQmlContext {
        MOTH_INSTR_HEADER
        StackSlot result;
    };
    struct instr_loadQmlImportedScripts {
        MOTH_INSTR_HEADER
        StackSlot result;
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
    instr_loadTrue loadTrue;
    instr_loadFalse loadFalse;
    instr_loadNull loadNull;
    instr_loadUndefined loadUndefined;
    instr_loadInt loadInt;
    instr_moveConst moveConst;
    instr_loadReg loadReg;
    instr_storeReg storeReg;
    instr_moveReg moveReg;
    instr_loadScopedLocal loadScopedLocal;
    instr_storeScopedLocal storeScopedLocal;
    instr_loadScopedArgument loadScopedArgument;
    instr_storeScopedArgument storeScopedArgument;
    instr_loadRuntimeString loadRuntimeString;
    instr_loadRegExp loadRegExp;
    instr_loadClosure loadClosure;
    instr_loadName loadName;
    instr_loadGlobalLookup loadGlobalLookup;
    instr_storeNameSloppy storeNameSloppy;
    instr_storeNameStrict storeNameStrict;
    instr_loadElement loadElement;
    instr_loadElementA loadElementA;
    instr_storeElement storeElement;
    instr_loadProperty loadProperty;
    instr_loadPropertyA loadPropertyA;
    instr_getLookup getLookup;
    instr_getLookupA getLookupA;
    instr_loadScopeObjectProperty loadScopeObjectProperty;
    instr_loadContextObjectProperty loadContextObjectProperty;
    instr_loadIdObject loadIdObject;
    instr_storeProperty storeProperty;
    instr_setLookup setLookup;
    instr_storeScopeObjectProperty storeScopeObjectProperty;
    instr_storeContextObjectProperty storeContextObjectProperty;
    instr_callValue callValue;
    instr_callProperty callProperty;
    instr_callPropertyLookup callPropertyLookup;
    instr_callElement callElement;
    instr_callName callName;
    instr_callGlobalLookup callGlobalLookup;
    instr_throwException throwException;
    instr_getException getException;
    instr_setException setException;
    instr_setExceptionHandler setExceptionHandler;
    instr_unwindException unwindException;
    instr_pushCatchContext pushCatchContext;
    instr_pushWithContext pushWithContext;
    instr_popContext popContext;
    instr_foreachIteratorObject foreachIteratorObject;
    instr_foreachNextPropertyName foreachNextPropertyName;
    instr_deleteMember deleteMember;
    instr_deleteSubscript deleteSubscript;
    instr_deleteName deleteName;
    instr_typeofName typeofName;
    instr_typeofValue typeofValue;
    instr_declareVar declareVar;
    instr_defineArray defineArray;
    instr_defineObjectLiteral defineObjectLiteral;
    instr_createMappedArgumentsObject createMappedArgumentsObject;
    instr_createUnmappedArgumentsObject createUnmappedArgumentsObject;
    instr_convertThisToObject convertThisToObject;
    instr_createValue createValue;
    instr_createProperty createProperty;
    instr_constructPropertyLookup constructPropertyLookup;
    instr_createName createName;
    instr_constructGlobalLookup constructGlobalLookup;
    instr_jump jump;
    instr_jumpEq jumpEq;
    instr_jumpNe jumpNe;
    instr_cmpJmpEq cmpJmpEq;
    instr_cmpJmpNe cmpJmpNe;
    instr_cmpJmpGt cmpJmpGt;
    instr_cmpJmpGe cmpJmpGe;
    instr_cmpJmpLt cmpJmpLt;
    instr_cmpJmpLe cmpJmpLe;
    instr_jumpStrictEqual jumpStrictEqual;
    instr_jumpStrictNotEqual jumpStrictNotEqual;
    instr_jumpStrictEqualStackSlotInt jumpStrictEqualStackSlotInt;
    instr_jumpStrictNotEqualStackSlotInt jumpStrictNotEqualStackSlotInt;
    instr_unot unot;
    instr_uplus uplus;
    instr_uminus uminus;
    instr_ucompl ucompl;
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
