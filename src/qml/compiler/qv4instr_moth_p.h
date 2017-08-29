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

#define INSTRUCTION(op, name, nargs, ...) \
    op##_INSTRUCTION(name, nargs, __VA_ARGS__)

/* for all jump instructions, the offset has to come last, to simplify the job of the bytecode generator */
#define INSTR_Ret(op) INSTRUCTION(op, Ret, 0)
#define INSTR_Debug(op) INSTRUCTION(op, Debug, 0)
#define INSTR_LoadConst(op) INSTRUCTION(op, LoadConst, 1, index)
#define INSTR_LoadZero(op) INSTRUCTION(op, LoadZero, 0)
#define INSTR_LoadTrue(op) INSTRUCTION(op, LoadTrue, 0)
#define INSTR_LoadFalse(op) INSTRUCTION(op, LoadFalse, 0)
#define INSTR_LoadNull(op) INSTRUCTION(op, LoadNull, 0)
#define INSTR_LoadUndefined(op) INSTRUCTION(op, LoadUndefined, 0)
#define INSTR_LoadInt(op) INSTRUCTION(op, LoadInt, 1, value)
#define INSTR_MoveConst(op) INSTRUCTION(op, MoveConst, 2, constIndex, destTemp)
#define INSTR_LoadReg(op) INSTRUCTION(op, LoadReg, 1, reg)
#define INSTR_StoreReg(op) INSTRUCTION(op, StoreReg, 1, reg)
#define INSTR_MoveReg(op) INSTRUCTION(op, MoveReg, 2, srcReg, destReg)
#define INSTR_LoadLocal(op) INSTRUCTION(op, LoadLocal, 1, index)
#define INSTR_StoreLocal(op) INSTRUCTION(op, StoreLocal, 1, index)
#define INSTR_LoadScopedLocal(op) INSTRUCTION(op, LoadScopedLocal, 2, scope, index)
#define INSTR_StoreScopedLocal(op) INSTRUCTION(op, StoreScopedLocal, 2, scope, index)
#define INSTR_LoadRuntimeString(op) INSTRUCTION(op, LoadRuntimeString, 1, stringId)
#define INSTR_LoadRegExp(op) INSTRUCTION(op, LoadRegExp, 1, regExpId)
#define INSTR_LoadClosure(op) INSTRUCTION(op, LoadClosure, 1, value)
#define INSTR_LoadName(op) INSTRUCTION(op, LoadName, 1, name)
#define INSTR_LoadGlobalLookup(op) INSTRUCTION(op, LoadGlobalLookup, 1, index)
#define INSTR_StoreNameSloppy(op) INSTRUCTION(op, StoreNameSloppy, 1, name)
#define INSTR_StoreNameStrict(op) INSTRUCTION(op, StoreNameStrict, 1, name)
#define INSTR_LoadProperty(op) INSTRUCTION(op, LoadProperty, 2, name, base)
#define INSTR_LoadPropertyA(op) INSTRUCTION(op, LoadPropertyA, 1, name)
#define INSTR_GetLookup(op) INSTRUCTION(op, GetLookup, 2, index, base)
#define INSTR_GetLookupA(op) INSTRUCTION(op, GetLookupA, 1, index)
#define INSTR_LoadScopeObjectProperty(op) INSTRUCTION(op, LoadScopeObjectProperty, 3, propertyIndex, base, captureRequired)
#define INSTR_LoadContextObjectProperty(op) INSTRUCTION(op, LoadContextObjectProperty, 3, propertyIndex, base, captureRequired)
#define INSTR_LoadIdObject(op) INSTRUCTION(op, LoadIdObject, 2, index, base)
#define INSTR_StoreProperty(op) INSTRUCTION(op, StoreProperty, 2, name, base)
#define INSTR_SetLookup(op) INSTRUCTION(op, SetLookup, 2, index, base)
#define INSTR_StoreScopeObjectProperty(op) INSTRUCTION(op, StoreScopeObjectProperty, 2, base, propertyIndex)
#define INSTR_StoreContextObjectProperty(op) INSTRUCTION(op, StoreContextObjectProperty, 2, base, propertyIndex)
#define INSTR_LoadElement(op) INSTRUCTION(op, LoadElement, 2, base, index)
#define INSTR_LoadElementA(op) INSTRUCTION(op, LoadElementA, 1, base)
#define INSTR_StoreElement(op) INSTRUCTION(op, StoreElement, 2, base, index)
#define INSTR_CallValue(op) INSTRUCTION(op, CallValue, 1, callData)
#define INSTR_CallProperty(op) INSTRUCTION(op, CallProperty, 3, name, callData, base)
#define INSTR_CallPropertyLookup(op) INSTRUCTION(op, CallPropertyLookup, 3, lookupIndex, callData, base)
#define INSTR_CallElement(op) INSTRUCTION(op, CallElement, 3, base, index, callData)
#define INSTR_CallName(op) INSTRUCTION(op, CallName, 2, name, callData)
#define INSTR_CallPossiblyDirectEval(op) INSTRUCTION(op, CallPossiblyDirectEval, 1, callData)
#define INSTR_CallGlobalLookup(op) INSTRUCTION(op, CallGlobalLookup, 2, index, callData)
#define INSTR_SetExceptionHandler(op) INSTRUCTION(op, SetExceptionHandler, 1, offset)
#define INSTR_ThrowException(op) INSTRUCTION(op, ThrowException, 0)
#define INSTR_GetException(op) INSTRUCTION(op, GetException, 0)
#define INSTR_SetException(op) INSTRUCTION(op, SetException, 0)
#define INSTR_PushCatchContext(op) INSTRUCTION(op, PushCatchContext, 2, name, reg)
#define INSTR_PushWithContext(op) INSTRUCTION(op, PushWithContext, 1, reg)
#define INSTR_PopContext(op) INSTRUCTION(op, PopContext, 1, reg)
#define INSTR_ForeachIteratorObject(op) INSTRUCTION(op, ForeachIteratorObject, 0)
#define INSTR_ForeachNextPropertyName(op) INSTRUCTION(op, ForeachNextPropertyName, 0)
#define INSTR_DeleteMember(op) INSTRUCTION(op, DeleteMember, 2, member, base)
#define INSTR_DeleteSubscript(op) INSTRUCTION(op, DeleteSubscript, 2, base, index)
#define INSTR_DeleteName(op) INSTRUCTION(op, DeleteName, 1, name)
#define INSTR_TypeofName(op) INSTRUCTION(op, TypeofName, 1, name)
#define INSTR_TypeofValue(op) INSTRUCTION(op, TypeofValue, 0)
#define INSTR_DeclareVar(op) INSTRUCTION(op, DeclareVar, 2, varName, isDeletable)
#define INSTR_DefineArray(op) INSTRUCTION(op, DefineArray, 2, argc, args)
// arrayGetterSetterCountAndFlags contains 30 bits for count, 1 bit for needsSparseArray boolean
#define INSTR_DefineObjectLiteral(op) INSTRUCTION(op, DefineObjectLiteral, 4, internalClassId, arrayValueCount, arrayGetterSetterCountAndFlags, args)
#define INSTR_CreateMappedArgumentsObject(op) INSTRUCTION(op, CreateMappedArgumentsObject, 0)
#define INSTR_CreateUnmappedArgumentsObject(op) INSTRUCTION(op, CreateUnmappedArgumentsObject, 0)
#define INSTR_ConvertThisToObject(op) INSTRUCTION(op, ConvertThisToObject, 0)
#define INSTR_Construct(op) INSTRUCTION(op, Construct, 2, callData, func)
#define INSTR_Jump(op) INSTRUCTION(op, Jump, 1, offset)
#define INSTR_JumpEq(op) INSTRUCTION(op, JumpEq, 1, offset)
#define INSTR_JumpNe(op) INSTRUCTION(op, JumpNe, 1, offset)
#define INSTR_CmpJmpEqNull(op) INSTRUCTION(op, CmpJmpEqNull, 1, offset)
#define INSTR_CmpJmpNeNull(op) INSTRUCTION(op, CmpJmpNeNull, 1, offset)
#define INSTR_CmpJmpEqInt(op) INSTRUCTION(op, CmpJmpEqInt, 2, lhs, offset)
#define INSTR_CmpJmpNeInt(op) INSTRUCTION(op, CmpJmpNeInt, 2, lhs, offset)
#define INSTR_CmpJmpEq(op) INSTRUCTION(op, CmpJmpEq, 2, lhs, offset)
#define INSTR_CmpJmpNe(op) INSTRUCTION(op, CmpJmpNe, 2, lhs, offset)
#define INSTR_CmpJmpGt(op) INSTRUCTION(op, CmpJmpGt, 2, lhs, offset)
#define INSTR_CmpJmpGe(op) INSTRUCTION(op, CmpJmpGe, 2, lhs, offset)
#define INSTR_CmpJmpLt(op) INSTRUCTION(op, CmpJmpLt, 2, lhs, offset)
#define INSTR_CmpJmpLe(op) INSTRUCTION(op, CmpJmpLe, 2, lhs, offset)
#define INSTR_JumpStrictEqual(op) INSTRUCTION(op, JumpStrictEqual, 2, lhs, offset)
#define INSTR_JumpStrictNotEqual(op) INSTRUCTION(op, JumpStrictNotEqual, 2, lhs, offset)
#define INSTR_JumpStrictEqualStackSlotInt(op) INSTRUCTION(op, JumpStrictEqualStackSlotInt, 3, lhs, rhs, offset)
#define INSTR_JumpStrictNotEqualStackSlotInt(op) INSTRUCTION(op, JumpStrictNotEqualStackSlotInt, 3, lhs, rhs, offset)
#define INSTR_UNot(op) INSTRUCTION(op, UNot, 0)
#define INSTR_UPlus(op) INSTRUCTION(op, UPlus, 0)
#define INSTR_UMinus(op) INSTRUCTION(op, UMinus, 0)
#define INSTR_UCompl(op) INSTRUCTION(op, UCompl, 0)
#define INSTR_Increment(op) INSTRUCTION(op, Increment, 0)
#define INSTR_Decrement(op) INSTRUCTION(op, Decrement, 0)
// alu is QV4::Runtime::RuntimeMethods enum value
#define INSTR_Binop(op) INSTRUCTION(op, Binop, 2, alu, lhs)
#define INSTR_Add(op) INSTRUCTION(op, Add, 1, lhs)
#define INSTR_BitAnd(op) INSTRUCTION(op, BitAnd, 1, lhs)
#define INSTR_BitOr(op) INSTRUCTION(op, BitOr, 1, lhs)
#define INSTR_BitXor(op) INSTRUCTION(op, BitXor, 1, lhs)
#define INSTR_Shr(op) INSTRUCTION(op, Shr, 1, lhs)
#define INSTR_Shl(op) INSTRUCTION(op, Shl, 1, lhs)
#define INSTR_BitAndConst(op) INSTRUCTION(op, BitAndConst, 1, rhs)
#define INSTR_BitOrConst(op) INSTRUCTION(op, BitOrConst, 1, rhs)
#define INSTR_BitXorConst(op) INSTRUCTION(op, BitXorConst, 1, rhs)
#define INSTR_ShrConst(op) INSTRUCTION(op, ShrConst, 1, rhs)
#define INSTR_ShlConst(op) INSTRUCTION(op, ShlConst, 1, rhs)
#define INSTR_Mul(op) INSTRUCTION(op, Mul, 1, lhs)
#define INSTR_Sub(op) INSTRUCTION(op, Sub, 1, lhs)
// alu is offset inside the runtime methods
#define INSTR_BinopContext(op) INSTRUCTION(op, BinopContext, 2, alu, lhs)
#define INSTR_LoadQmlContext(op) INSTRUCTION(op, LoadQmlContext, 1, result)
#define INSTR_LoadQmlImportedScripts(op) INSTRUCTION(op, LoadQmlImportedScripts, 1, result)
#define INSTR_LoadQmlSingleton(op) INSTRUCTION(op, LoadQmlSingleton, 1, name)


#define FOR_EACH_MOTH_INSTR(F) \
    F(Ret) \
    F(Debug) \
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
    F(LoadLocal) \
    F(StoreLocal) \
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
#define MOTH_NUM_INSTRUCTIONS() (static_cast<int>(Moth::Instr::Type::LoadQmlSingleton) + 1)

#if defined(Q_CC_GNU) && (!defined(Q_CC_INTEL) || __INTEL_COMPILER >= 1200)
#  define MOTH_COMPUTED_GOTO
#endif

#define MOTH_INSTR_ALIGN_MASK (Q_ALIGNOF(QV4::Moth::Instr) - 1)

#define MOTH_INSTR_ENUM(I)  I,
#define MOTH_INSTR_SIZE(I) ((sizeof(QV4::Moth::Instr::instr_##I) + MOTH_INSTR_ALIGN_MASK) & ~MOTH_INSTR_ALIGN_MASK)

#define MOTH_DEFINE_ARGS(nargs, ...) \
    MOTH_DEFINE_ARGS##nargs(__VA_ARGS__)

#define MOTH_DEFINE_ARGS0(dummy)
#define MOTH_DEFINE_ARGS1(arg) \
    int arg;
#define MOTH_DEFINE_ARGS2(arg1, arg2) \
    int arg1; \
    int arg2;
#define MOTH_DEFINE_ARGS3(arg1, arg2, arg3) \
    int arg1; \
    int arg2; \
    int arg3;
#define MOTH_DEFINE_ARGS4(arg1, arg2, arg3, arg4) \
    int arg1; \
    int arg2; \
    int arg3; \
    int arg4;

#define MOTH_COLLECT_ENUMS(instr) \
    INSTR_##instr(MOTH_GET_ENUM)
#define MOTH_GET_ENUM_INSTRUCTION(name, ...) \
    name,

#define MOTH_EMIT_STRUCTS(instr) \
    INSTR_##instr(MOTH_EMIT_STRUCT)
#define MOTH_EMIT_STRUCT_INSTRUCTION(name, nargs, ...) \
    struct instr_##name { \
        MOTH_DEFINE_ARGS(nargs, __VA_ARGS__) \
    };

#define MOTH_EMIT_INSTR_MEMBERS(instr) \
    INSTR_##instr(MOTH_EMIT_INSTR_MEMBER)
#define MOTH_EMIT_INSTR_MEMBER_INSTRUCTION(name, nargs, ...) \
    instr_##name name;

#define MOTH_COLLECT_NARGS(instr) \
    INSTR_##instr(MOTH_COLLECT_ARG_COUNT)
#define MOTH_COLLECT_ARG_COUNT_INSTRUCTION(name, nargs, ...) \
    nargs,

#define MOTH_DECODE_ARG(arg, type, nargs, offset) \
    arg = reinterpret_cast<const type *>(code)[-nargs + offset];
#define MOTH_ADJUST_CODE(type, nargs) \
    code += static_cast<quintptr>(nargs*sizeof(type) + 1)

#define MOTH_DECODE_INSTRUCTION(name, nargs, ...) \
        MOTH_DEFINE_ARGS(nargs, __VA_ARGS__) \
    op_int_##name: \
        MOTH_ADJUST_CODE(int, nargs); \
        MOTH_DECODE_ARGS(name, int, nargs, __VA_ARGS__) \
        goto op_main_##name; \
    op_char_##name: \
        MOTH_ADJUST_CODE(char, nargs); \
        MOTH_DECODE_ARGS(name, char, nargs, __VA_ARGS__) \
    op_main_##name: \
        ; \

#define MOTH_DECODE_ARGS(name, type, nargs, ...) \
    MOTH_DECODE_ARGS##nargs(name, type, nargs, __VA_ARGS__)

#define MOTH_DECODE_ARGS0(name, type, nargs, dummy)
#define MOTH_DECODE_ARGS1(name, type, nargs, arg) \
        MOTH_DECODE_ARG(arg, type, nargs, 0);
#define MOTH_DECODE_ARGS2(name, type, nargs, arg1, arg2) \
        MOTH_DECODE_ARGS1(name, type, nargs, arg1); \
        MOTH_DECODE_ARG(arg2, type, nargs, 1);
#define MOTH_DECODE_ARGS3(name, type, nargs, arg1, arg2, arg3) \
        MOTH_DECODE_ARGS2(name, type, nargs, arg1, arg2); \
        MOTH_DECODE_ARG(arg3, type, nargs, 2);
#define MOTH_DECODE_ARGS4(name, type, nargs, arg1, arg2, arg3, arg4) \
        MOTH_DECODE_ARGS3(name, type, nargs, arg1, arg2, arg3); \
        MOTH_DECODE_ARG(arg4, type, nargs, 3);

#ifdef MOTH_COMPUTED_GOTO
/* collect jump labels */
#define COLLECT_LABELS(instr) \
    INSTR_##instr(GET_LABEL)
#define GET_LABEL_INSTRUCTION(name, ...) \
    &&op_char_##name,
#define COLLECT_LABELS_WIDE(instr) \
    INSTR_##instr(GET_LABEL_WIDE)
#define GET_LABEL_WIDE_INSTRUCTION(name, ...) \
    &&op_int_##name,

#define MOTH_JUMP_TABLE \
    static const void *jumpTable[] = { \
        FOR_EACH_MOTH_INSTR(COLLECT_LABELS) \
        FOR_EACH_MOTH_INSTR(COLLECT_LABELS_WIDE) \
    };

#define MOTH_DISPATCH() \
    goto *jumpTable[*reinterpret_cast<const uchar *>(code)];
#else
#define MOTH_JUMP_TABLE

#define MOTH_INSTR_CASE_AND_JUMP(instr) \
    INSTR_##instr(GET_CASE_AND_JUMP)
#define GET_CASE_AND_JUMP_INSTRUCTION(name, ...) \
    case Instr::Type::name: goto op_char_##name;
#define MOTH_INSTR_CASE_AND_JUMP_WIDE(instr) \
    INSTR_##instr(GET_CASE_AND_JUMP_WIDE)
#define GET_CASE_AND_JUMP_WIDE_INSTRUCTION(name, ...) \
    case Instr::Type::name: goto op_int_##name;

#define MOTH_DISPATCH() \
    switch (static_cast<Instr::Type>(*code)) { \
        FOR_EACH_MOTH_INSTR(MOTH_INSTR_CASE_AND_JUMP) \
    }
#endif

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
    operator int() const { return index; }

    static QString dump(int reg, int nFormals) {
        StackSlot t;
        t.index = reg;
        return t.dump(nFormals);
    }
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
    };

    static const int argumentCount[];

    FOR_EACH_MOTH_INSTR(MOTH_EMIT_STRUCTS)

    FOR_EACH_MOTH_INSTR(MOTH_EMIT_INSTR_MEMBERS)

    static int size(Type type);
};

Q_STATIC_ASSERT(MOTH_NUM_INSTRUCTIONS() < 128);

template<int N>
struct InstrMeta {
};

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wuninitialized")
#define MOTH_INSTR_META_TEMPLATE(I) \
    template<> struct InstrMeta<int(Instr::Type::I)> { \
        enum { Size = MOTH_INSTR_SIZE(I) }; \
        typedef Instr::instr_##I DataType; \
        static const DataType &data(const Instr &instr) { return instr.I; } \
        static void setData(Instr &instr, const DataType &v) \
        { memcpy(reinterpret_cast<char *>(&instr.I), \
                 reinterpret_cast<const char *>(&v), \
                 Size); } \
    };
FOR_EACH_MOTH_INSTR(MOTH_INSTR_META_TEMPLATE);
#undef MOTH_INSTR_META_TEMPLATE
QT_WARNING_POP

template<int InstrType>
class InstrData : public InstrMeta<InstrType>::DataType
{
};

struct Instruction {
#define MOTH_INSTR_DATA_TYPEDEF(I) typedef InstrData<int(Instr::Type::I)> I;
FOR_EACH_MOTH_INSTR(MOTH_INSTR_DATA_TYPEDEF)
#undef MOTH_INSTR_DATA_TYPEDEF
private:
    Instruction();
};

} // namespace Moth
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4INSTR_MOTH_P_H
