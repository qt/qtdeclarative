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
    F(Debug)
#endif

#define FOR_EACH_MOTH_INSTR(F) \
    F(Ret) \
    MOTH_DEBUG_INSTR(F) \
    F(LoadConst) \
    F(LoadZero) \
    F(LoadTrue) \
    F(LoadFalse) \
    F(LoadNull) \
    F(LoadUndefined) \
    F(LoadInt) \
    F(MoveConst) \
    F(LoadReg) \
    F(StoreReg) \
    F(MoveReg) \
    F(LoadScopedLocal) \
    F(StoreScopedLocal) \
    F(LoadRuntimeString) \
    F(LoadRegExp) \
    F(LoadClosure) \
    F(LoadName) \
    F(LoadGlobalLookup) \
    F(StoreNameSloppy) \
    F(StoreNameStrict) \
    F(LoadElement) \
    F(LoadElementA) \
    F(StoreElement) \
    F(LoadProperty) \
    F(LoadPropertyA) \
    F(GetLookup) \
    F(GetLookupA) \
    F(StoreProperty) \
    F(SetLookup) \
    F(StoreScopeObjectProperty) \
    F(StoreContextObjectProperty) \
    F(LoadScopeObjectProperty) \
    F(LoadContextObjectProperty) \
    F(LoadIdObject) \
    F(CallValue) \
    F(CallProperty) \
    F(CallPropertyLookup) \
    F(CallElement) \
    F(CallName) \
    F(CallPossiblyDirectEval) \
    F(CallGlobalLookup) \
    F(SetExceptionHandler) \
    F(ThrowException) \
    F(GetException) \
    F(SetException) \
    F(UnwindException) \
    F(PushCatchContext) \
    F(PushWithContext) \
    F(PopContext) \
    F(ForeachIteratorObject) \
    F(ForeachNextPropertyName) \
    F(DeleteMember) \
    F(DeleteSubscript) \
    F(DeleteName) \
    F(TypeofName) \
    F(TypeofValue) \
    F(DeclareVar) \
    F(DefineArray) \
    F(DefineObjectLiteral) \
    F(CreateMappedArgumentsObject) \
    F(CreateUnmappedArgumentsObject) \
    F(ConvertThisToObject) \
    F(Construct) \
    F(Jump) \
    F(JumpEq) \
    F(JumpNe) \
    F(CmpJmpEqNull) \
    F(CmpJmpNeNull) \
    F(CmpJmpEqInt) \
    F(CmpJmpNeInt) \
    F(CmpJmpEq) \
    F(CmpJmpNe) \
    F(CmpJmpGt) \
    F(CmpJmpGe) \
    F(CmpJmpLt) \
    F(CmpJmpLe) \
    F(JumpStrictEqual) \
    F(JumpStrictNotEqual) \
    F(JumpStrictEqualStackSlotInt) \
    F(JumpStrictNotEqualStackSlotInt) \
    F(UNot) \
    F(UPlus) \
    F(UMinus) \
    F(UCompl) \
    F(Increment) \
    F(Decrement) \
    F(Binop) \
    F(Add) \
    F(BitAnd) \
    F(BitOr) \
    F(BitXor) \
    F(Shr) \
    F(Shl) \
    F(BitAndConst) \
    F(BitOrConst) \
    F(BitXorConst) \
    F(ShrConst) \
    F(ShlConst) \
    F(Mul) \
    F(Sub) \
    F(BinopContext) \
    F(LoadQmlContext) \
    F(LoadQmlImportedScripts) \
    F(LoadQmlSingleton)

#if defined(Q_CC_GNU) && (!defined(Q_CC_INTEL) || __INTEL_COMPILER >= 1200)
#  define MOTH_THREADED_INTERPRETER
#endif

#define MOTH_INSTR_ALIGN_MASK (Q_ALIGNOF(QV4::Moth::Instr) - 1)

#define MOTH_INSTR_HEADER union { Instr::Type instructionType; quint64 _dummy; };

#define MOTH_INSTR_ENUM(I)  I,
#define MOTH_INSTR_SIZE(I) ((sizeof(QV4::Moth::Instr::instr_##I) + MOTH_INSTR_ALIGN_MASK) & ~MOTH_INSTR_ALIGN_MASK)


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
void dumpBytecode(const char *bytecode, int len, int nLocals, int nFormals, int startLine = 1, const QVector<int> &lineNumberMapping = QVector<int>());
inline void dumpBytecode(const QByteArray &bytecode, int nLocals, int nFormals, int startLine = 1, const QVector<int> &lineNumberMapping = QVector<int>()) {
    dumpBytecode(bytecode.constData(), bytecode.length(), nLocals, nFormals, startLine, lineNumberMapping);
}

union Instr
{
    enum class Type {
        FOR_EACH_MOTH_INSTR(MOTH_INSTR_ENUM)
        LastInstruction
    };

    struct instr_Common {
        MOTH_INSTR_HEADER
    };
    struct instr_Ret {
        MOTH_INSTR_HEADER
    };

#ifndef QT_NO_QML_DEBUGGING
    struct instr_Debug {
        MOTH_INSTR_HEADER
    };
#endif // QT_NO_QML_DEBUGGING

    struct instr_LoadConst {
        MOTH_INSTR_HEADER
        int index;
    };
    struct instr_LoadZero {
        MOTH_INSTR_HEADER
    };
    struct instr_LoadTrue {
        MOTH_INSTR_HEADER
    };
    struct instr_LoadFalse {
        MOTH_INSTR_HEADER
    };
    struct instr_LoadNull {
        MOTH_INSTR_HEADER
    };
    struct instr_LoadUndefined {
        MOTH_INSTR_HEADER
    };
    struct instr_LoadInt {
        MOTH_INSTR_HEADER
        int value;
    };
    struct instr_MoveConst {
        MOTH_INSTR_HEADER
        int constIndex;
        StackSlot destTemp;
    };
    struct instr_LoadReg {
        MOTH_INSTR_HEADER
        StackSlot reg;
    };
    struct instr_StoreReg {
        MOTH_INSTR_HEADER
        StackSlot reg;
    };
    struct instr_MoveReg {
        MOTH_INSTR_HEADER
        StackSlot srcReg;
        StackSlot destReg;
    };
    struct instr_LoadScopedLocal {
        MOTH_INSTR_HEADER
        int scope;
        int index;
    };
    struct instr_StoreScopedLocal {
        MOTH_INSTR_HEADER
        int scope;
        int index;
    };
    struct instr_LoadRuntimeString {
        MOTH_INSTR_HEADER
        int stringId;
    };
    struct instr_LoadRegExp {
        MOTH_INSTR_HEADER
        int regExpId;
    };
    struct instr_LoadClosure {
        MOTH_INSTR_HEADER
        int value;
    };
    struct instr_LoadName {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_LoadGlobalLookup {
        MOTH_INSTR_HEADER
        int index;
    };
    struct instr_StoreNameSloppy {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_StoreNameStrict {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_LoadProperty {
        MOTH_INSTR_HEADER
        int name;
        StackSlot base;
    };
    struct instr_LoadPropertyA {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_GetLookup {
        MOTH_INSTR_HEADER
        int index;
        StackSlot base;
    };
    struct instr_GetLookupA {
        MOTH_INSTR_HEADER
        int index;
    };
    struct instr_LoadScopeObjectProperty {
        MOTH_INSTR_HEADER
        int propertyIndex;
        StackSlot base;
        bool captureRequired;
    };
    struct instr_LoadContextObjectProperty {
        MOTH_INSTR_HEADER
        int propertyIndex;
        StackSlot base;
        bool captureRequired;
    };
    struct instr_LoadIdObject {
        MOTH_INSTR_HEADER
        int index;
        StackSlot base;
    };
    struct instr_StoreProperty {
        MOTH_INSTR_HEADER
        int name;
        StackSlot base;
    };
    struct instr_SetLookup {
        MOTH_INSTR_HEADER
        int index;
        StackSlot base;
    };
    struct instr_StoreScopeObjectProperty {
        MOTH_INSTR_HEADER
        StackSlot base;
        int propertyIndex;
    };
    struct instr_StoreContextObjectProperty {
        MOTH_INSTR_HEADER
        StackSlot base;
        int propertyIndex;
    };
    struct instr_LoadElement {
        MOTH_INSTR_HEADER
        StackSlot base;
        StackSlot index;
    };
    struct instr_LoadElementA {
        MOTH_INSTR_HEADER
        StackSlot base;
    };
    struct instr_StoreElement {
        MOTH_INSTR_HEADER
        StackSlot base;
        StackSlot index;
    };
    struct instr_CallValue {
        MOTH_INSTR_HEADER
        StackSlot callData;
    };
    struct instr_CallProperty {
        MOTH_INSTR_HEADER
        int name;
        StackSlot callData;
        StackSlot base;
    };
    struct instr_CallPropertyLookup {
        MOTH_INSTR_HEADER
        int lookupIndex;
        StackSlot callData;
        StackSlot base;
    };
    struct instr_CallElement {
        MOTH_INSTR_HEADER
        StackSlot base;
        StackSlot index;
        StackSlot callData;
    };
    struct instr_CallName {
        MOTH_INSTR_HEADER
        int name;
        StackSlot callData;
    };
    struct instr_CallPossiblyDirectEval {
        MOTH_INSTR_HEADER
        StackSlot callData;
    };
    struct instr_CallGlobalLookup {
        MOTH_INSTR_HEADER
        int index;
        StackSlot callData;
    };
    struct instr_SetExceptionHandler {
        MOTH_INSTR_HEADER
        int offset;
    };
    struct instr_ThrowException {
        MOTH_INSTR_HEADER
    };
    struct instr_GetException {
        MOTH_INSTR_HEADER
    };
    struct instr_SetException {
        MOTH_INSTR_HEADER
    };
    struct instr_UnwindException {
        MOTH_INSTR_HEADER
    };
    struct instr_PushCatchContext {
        MOTH_INSTR_HEADER
        int name;
        StackSlot reg;
    };
    struct instr_PushWithContext {
        MOTH_INSTR_HEADER
        StackSlot reg;
    };
    struct instr_PopContext {
        MOTH_INSTR_HEADER
        StackSlot reg;
    };
    struct instr_ForeachIteratorObject {
        MOTH_INSTR_HEADER
    };
    struct instr_ForeachNextPropertyName {
        MOTH_INSTR_HEADER
    };
    struct instr_DeleteMember {
        MOTH_INSTR_HEADER
        int member;
        StackSlot base;
    };
    struct instr_DeleteSubscript {
        MOTH_INSTR_HEADER
        StackSlot base;
        StackSlot index;
    };
    struct instr_DeleteName {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_TypeofName {
        MOTH_INSTR_HEADER
        int name;
    };
    struct instr_TypeofValue {
        MOTH_INSTR_HEADER
    };
    struct instr_DeclareVar {
        MOTH_INSTR_HEADER
        int varName;
        bool isDeletable;
    };
    struct instr_DefineArray {
        MOTH_INSTR_HEADER
        uint argc;
        StackSlot args;
    };
    struct instr_DefineObjectLiteral {
        MOTH_INSTR_HEADER
        int internalClassId;
        int arrayValueCount;
        int arrayGetterSetterCountAndFlags; // 30 bits for count, 1 bit for needsSparseArray boolean
        StackSlot args;
    };
    struct instr_CreateMappedArgumentsObject {
        MOTH_INSTR_HEADER
    };
    struct instr_CreateUnmappedArgumentsObject {
        MOTH_INSTR_HEADER
    };
    struct instr_ConvertThisToObject {
        MOTH_INSTR_HEADER
    };
    struct instr_Construct {
        MOTH_INSTR_HEADER
        StackSlot callData;
        StackSlot func;
    };
    struct instr_Jump {
        MOTH_INSTR_HEADER
        int offset;
    };
    struct instr_JumpEq {
        MOTH_INSTR_HEADER
        int offset;
    };
    struct instr_JumpNe {
        MOTH_INSTR_HEADER
        int offset;
    };
    struct instr_CmpJmpEqNull {
        MOTH_INSTR_HEADER
        int offset;
    };
    struct instr_CmpJmpNeNull {
        MOTH_INSTR_HEADER
        int offset;
    };
    struct instr_CmpJmpEqInt {
        MOTH_INSTR_HEADER
        int lhs;
        int offset;
    };
    struct instr_CmpJmpNeInt {
        MOTH_INSTR_HEADER
        int lhs;
        int offset;
    };
    struct instr_CmpJmpEq {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        int offset;
    };
    struct instr_CmpJmpNe {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        int offset;
    };
    struct instr_CmpJmpGt {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        int offset;
    };
    struct instr_CmpJmpGe {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        int offset;
    };
    struct instr_CmpJmpLt {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        int offset;
    };
    struct instr_CmpJmpLe {
        MOTH_INSTR_HEADER
        StackSlot lhs;
        int offset;
    };
    struct instr_JumpStrictEqual {
        MOTH_INSTR_HEADER
        int offset;
        StackSlot lhs;
    };
    struct instr_JumpStrictNotEqual {
        MOTH_INSTR_HEADER
        int offset;
        StackSlot lhs;
    };
    struct instr_JumpStrictEqualStackSlotInt {
        MOTH_INSTR_HEADER
        int offset;
        StackSlot lhs;
        int rhs;
    };
    struct instr_JumpStrictNotEqualStackSlotInt {
        MOTH_INSTR_HEADER
        int offset;
        StackSlot lhs;
        int rhs;
    };
    struct instr_UNot {
        MOTH_INSTR_HEADER
    };
    struct instr_UPlus {
        MOTH_INSTR_HEADER
    };
    struct instr_UMinus {
        MOTH_INSTR_HEADER
    };
    struct instr_UCompl {
        MOTH_INSTR_HEADER
    };
    struct instr_Increment {
        MOTH_INSTR_HEADER
    };
    struct instr_Decrement {
        MOTH_INSTR_HEADER
    };
    struct instr_Binop {
        MOTH_INSTR_HEADER
        int alu; // QV4::Runtime::RuntimeMethods enum value
        StackSlot lhs;
    };
    struct instr_Add {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_BitAnd {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_BitOr {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_BitXor {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_Shr {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_Shl {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_BitAndConst {
        MOTH_INSTR_HEADER
        int rhs;
    };
    struct instr_BitOrConst {
        MOTH_INSTR_HEADER
        int rhs;
    };
    struct instr_BitXorConst {
        MOTH_INSTR_HEADER
        int rhs;
    };
    struct instr_ShrConst {
        MOTH_INSTR_HEADER
        int rhs;
    };
    struct instr_ShlConst {
        MOTH_INSTR_HEADER
        int rhs;
    };
    struct instr_Mul {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_Sub {
        MOTH_INSTR_HEADER
        StackSlot lhs;
    };
    struct instr_BinopContext {
        MOTH_INSTR_HEADER
        uint alu; // offset inside the runtime methods
        StackSlot lhs;
    };
    struct instr_LoadQmlContext {
        MOTH_INSTR_HEADER
        StackSlot result;
    };
    struct instr_LoadQmlImportedScripts {
        MOTH_INSTR_HEADER
        StackSlot result;
    };
    struct instr_LoadQmlSingleton {
        MOTH_INSTR_HEADER
        int name;
    };

    instr_Common Common;
    instr_Ret Ret;
    instr_Debug Debug;
    instr_LoadConst LoadConst;
    instr_LoadZero LoadZero;
    instr_LoadTrue LoadTrue;
    instr_LoadFalse LoadFalse;
    instr_LoadNull LoadNull;
    instr_LoadUndefined LoadUndefined;
    instr_LoadInt LoadInt;
    instr_MoveConst MoveConst;
    instr_LoadReg LoadReg;
    instr_StoreReg StoreReg;
    instr_MoveReg MoveReg;
    instr_LoadScopedLocal LoadScopedLocal;
    instr_StoreScopedLocal StoreScopedLocal;
    instr_LoadRuntimeString LoadRuntimeString;
    instr_LoadRegExp LoadRegExp;
    instr_LoadClosure LoadClosure;
    instr_LoadName LoadName;
    instr_LoadGlobalLookup LoadGlobalLookup;
    instr_StoreNameSloppy StoreNameSloppy;
    instr_StoreNameStrict StoreNameStrict;
    instr_LoadElement LoadElement;
    instr_LoadElementA LoadElementA;
    instr_StoreElement StoreElement;
    instr_LoadProperty LoadProperty;
    instr_LoadPropertyA LoadPropertyA;
    instr_GetLookup GetLookup;
    instr_GetLookupA GetLookupA;
    instr_LoadScopeObjectProperty LoadScopeObjectProperty;
    instr_LoadContextObjectProperty LoadContextObjectProperty;
    instr_LoadIdObject LoadIdObject;
    instr_StoreProperty StoreProperty;
    instr_SetLookup SetLookup;
    instr_StoreScopeObjectProperty StoreScopeObjectProperty;
    instr_StoreContextObjectProperty StoreContextObjectProperty;
    instr_CallValue CallValue;
    instr_CallProperty CallProperty;
    instr_CallPropertyLookup CallPropertyLookup;
    instr_CallElement CallElement;
    instr_CallName CallName;
    instr_CallPossiblyDirectEval CallPossiblyDirectEval;
    instr_CallGlobalLookup CallGlobalLookup;
    instr_ThrowException ThrowException;
    instr_GetException GetException;
    instr_SetException SetException;
    instr_SetExceptionHandler SetExceptionHandler;
    instr_UnwindException UnwindException;
    instr_PushCatchContext PushCatchContext;
    instr_PushWithContext PushWithContext;
    instr_PopContext PopContext;
    instr_ForeachIteratorObject ForeachIteratorObject;
    instr_ForeachNextPropertyName ForeachNextPropertyName;
    instr_DeleteMember DeleteMember;
    instr_DeleteSubscript DeleteSubscript;
    instr_DeleteName DeleteName;
    instr_TypeofName TypeofName;
    instr_TypeofValue TypeofValue;
    instr_DeclareVar DeclareVar;
    instr_DefineArray DefineArray;
    instr_DefineObjectLiteral DefineObjectLiteral;
    instr_CreateMappedArgumentsObject CreateMappedArgumentsObject;
    instr_CreateUnmappedArgumentsObject CreateUnmappedArgumentsObject;
    instr_ConvertThisToObject ConvertThisToObject;
    instr_Construct Construct;
    instr_Jump Jump;
    instr_JumpEq JumpEq;
    instr_JumpNe JumpNe;
    instr_CmpJmpEqNull CmpJmpEqNull;
    instr_CmpJmpNeNull CmpJmpNeNull;
    instr_CmpJmpEqInt CmpJmpEqInt;
    instr_CmpJmpNeInt CmpJmpNeInt;
    instr_CmpJmpEq CmpJmpEq;
    instr_CmpJmpNe CmpJmpNe;
    instr_CmpJmpGt CmpJmpGt;
    instr_CmpJmpGe CmpJmpGe;
    instr_CmpJmpLt CmpJmpLt;
    instr_CmpJmpLe CmpJmpLe;
    instr_JumpStrictEqual JumpStrictEqual;
    instr_JumpStrictNotEqual JumpStrictNotEqual;
    instr_JumpStrictEqualStackSlotInt JumpStrictEqualStackSlotInt;
    instr_JumpStrictNotEqualStackSlotInt JumpStrictNotEqualStackSlotInt;
    instr_UNot UNot;
    instr_UPlus UPlus;
    instr_UMinus UMinus;
    instr_UCompl UCompl;
    instr_Increment Increment;
    instr_Decrement Decrement;
    instr_Binop Binop;
    instr_Add Add;
    instr_BitAnd BitAnd;
    instr_BitOr BitOr;
    instr_BitXor BitXor;
    instr_Shr Shr;
    instr_Shl Shl;
    instr_BitAndConst BitAndConst;
    instr_BitOrConst BitOrConst;
    instr_BitXorConst BitXorConst;
    instr_ShrConst ShrConst;
    instr_ShlConst ShlConst;
    instr_Mul Mul;
    instr_Sub Sub;
    instr_BinopContext BinopContext;
    instr_LoadQmlContext LoadQmlContext;
    instr_LoadQmlImportedScripts LoadQmlImportedScripts;
    instr_LoadQmlSingleton LoadQmlSingleton;

    static int size(Type type);
};

template<int N>
struct InstrMeta {
};

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wuninitialized")
#define MOTH_INSTR_META_TEMPLATE(I) \
    template<> struct InstrMeta<(int)Instr::Type::I> { \
        enum { Size = MOTH_INSTR_SIZE(I) }; \
        typedef Instr::instr_##I DataType; \
        static const DataType &data(const Instr &instr) { return instr.I; } \
        static void setData(Instr &instr, const DataType &v) { instr.I = v; } \
        static void setDataNoCommon(Instr &instr, const DataType &v) \
        { memcpy(reinterpret_cast<char *>(&instr.I) + sizeof(Instr::instr_Common), \
                 reinterpret_cast<const char *>(&v) + sizeof(Instr::instr_Common), \
                 Size - sizeof(Instr::instr_Common)); } \
    };
FOR_EACH_MOTH_INSTR(MOTH_INSTR_META_TEMPLATE);
#undef MOTH_INSTR_META_TEMPLATE
QT_WARNING_POP

template<int InstrType>
class InstrData : public InstrMeta<InstrType>::DataType
{
};

struct Instruction {
#define MOTH_INSTR_DATA_TYPEDEF(I) typedef InstrData<(int)Instr::Type::I> I;
FOR_EACH_MOTH_INSTR(MOTH_INSTR_DATA_TYPEDEF)
#undef MOTH_INSTR_DATA_TYPEDEF
private:
    Instruction();
};

} // namespace Moth
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4INSTR_MOTH_P_H
