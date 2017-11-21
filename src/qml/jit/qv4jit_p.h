/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QV4JIT_P_H
#define QV4JIT_P_H

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
#include <private/qv4function_p.h>
#include <private/qv4instr_moth_p.h>

//QT_REQUIRE_CONFIG(qml_jit);

#define JIT_DEFINE_ARGS(nargs, ...) \
    MOTH_EXPAND_FOR_MSVC(JIT_DEFINE_ARGS##nargs(__VA_ARGS__))

#define JIT_DEFINE_ARGS0()
#define JIT_DEFINE_ARGS1(arg) \
    int arg
#define JIT_DEFINE_ARGS2(arg1, arg2) \
    int arg1, \
    int arg2
#define JIT_DEFINE_ARGS3(arg1, arg2, arg3) \
    int arg1, \
    int arg2, \
    int arg3
#define JIT_DEFINE_ARGS4(arg1, arg2, arg3, arg4) \
    int arg1, \
    int arg2, \
    int arg3, \
    int arg4

#define JIT_DEFINE_VIRTUAL_BYTECODE_HANDLER_INSTRUCTION(name, nargs, ...) \
    virtual void generate_##name( \
    JIT_DEFINE_ARGS(nargs, __VA_ARGS__) \
    ) = 0;

#define JIT_DEFINE_VIRTUAL_BYTECODE_HANDLER(instr) \
    INSTR_##instr(JIT_DEFINE_VIRTUAL_BYTECODE_HANDLER)

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace JIT {

class Assembler;

class ByteCodeHandler
{
public:
    virtual ~ByteCodeHandler();

    void decode(const char *code, uint len);

    int instructionOffset() const { return _offset; }

protected:
    FOR_EACH_MOTH_INSTR(JIT_DEFINE_VIRTUAL_BYTECODE_HANDLER)

    virtual void startInstruction(Moth::Instr::Type instr) = 0;
    virtual void endInstruction(Moth::Instr::Type instr) = 0;

private:
    int _offset = 0;
};

#ifdef V4_ENABLE_JIT
class BaselineJIT final: public ByteCodeHandler
{
public:
    BaselineJIT(QV4::Function *);
    virtual ~BaselineJIT();

    void generate();

    void generate_Ret() Q_DECL_OVERRIDE;
    void generate_Debug() Q_DECL_OVERRIDE;
    void generate_LoadConst(int index) Q_DECL_OVERRIDE;
    void generate_LoadZero() Q_DECL_OVERRIDE;
    void generate_LoadTrue() Q_DECL_OVERRIDE;
    void generate_LoadFalse() Q_DECL_OVERRIDE;
    void generate_LoadNull() Q_DECL_OVERRIDE;
    void generate_LoadUndefined() Q_DECL_OVERRIDE;
    void generate_LoadInt(int value) Q_DECL_OVERRIDE;
    void generate_MoveConst(int constIndex, int destTemp) Q_DECL_OVERRIDE;
    void generate_LoadReg(int reg) Q_DECL_OVERRIDE;
    void generate_StoreReg(int reg) Q_DECL_OVERRIDE;
    void generate_MoveReg(int srcReg, int destReg) Q_DECL_OVERRIDE;
    void generate_LoadLocal(int index) Q_DECL_OVERRIDE;
    void generate_StoreLocal(int index) Q_DECL_OVERRIDE;
    void generate_LoadScopedLocal(int scope, int index) Q_DECL_OVERRIDE;
    void generate_StoreScopedLocal(int scope, int index) Q_DECL_OVERRIDE;
    void generate_LoadRuntimeString(int stringId) Q_DECL_OVERRIDE;
    void generate_MoveRegExp(int regExpId, int destReg) Q_DECL_OVERRIDE;
    void generate_LoadClosure(int value) Q_DECL_OVERRIDE;
    void generate_LoadName(int name) Q_DECL_OVERRIDE;
    void generate_LoadGlobalLookup(int index) Q_DECL_OVERRIDE;
    void generate_StoreNameSloppy(int name) Q_DECL_OVERRIDE;
    void generate_StoreNameStrict(int name) Q_DECL_OVERRIDE;
    void generate_LoadElement(int base, int index) Q_DECL_OVERRIDE;
    void generate_LoadElementA(int base) Q_DECL_OVERRIDE;
    void generate_StoreElement(int base, int index) Q_DECL_OVERRIDE;
    void generate_LoadProperty(int name, int base) Q_DECL_OVERRIDE;
    void generate_LoadPropertyA(int name) Q_DECL_OVERRIDE;
    void generate_GetLookup(int index, int base) Q_DECL_OVERRIDE;
    void generate_GetLookupA(int index) Q_DECL_OVERRIDE;
    void generate_StoreProperty(int name, int base) Q_DECL_OVERRIDE;
    void generate_SetLookup(int index, int base) Q_DECL_OVERRIDE;
    void generate_StoreScopeObjectProperty(int base,
                                           int propertyIndex) Q_DECL_OVERRIDE;
    void generate_StoreContextObjectProperty(int base,
                                             int propertyIndex) Q_DECL_OVERRIDE;
    void generate_LoadScopeObjectProperty(int propertyIndex, int base,
                                          int captureRequired) Q_DECL_OVERRIDE;
    void generate_LoadContextObjectProperty(int propertyIndex, int base,
                                            int captureRequired) Q_DECL_OVERRIDE;
    void generate_LoadIdObject(int index, int base) Q_DECL_OVERRIDE;
    void generate_CallValue(int name, int argc, int argv) Q_DECL_OVERRIDE;
    void generate_CallProperty(int name, int base, int argc, int argv) Q_DECL_OVERRIDE;
    void generate_CallPropertyLookup(int lookupIndex, int base, int argc, int argv) Q_DECL_OVERRIDE;
    void generate_CallElement(int base, int index, int argc, int argv) Q_DECL_OVERRIDE;
    void generate_CallName(int name, int argc, int argv) Q_DECL_OVERRIDE;
    void generate_CallPossiblyDirectEval(int argc, int argv) Q_DECL_OVERRIDE;
    void generate_CallGlobalLookup(int index, int argc, int argv) Q_DECL_OVERRIDE;
    void generate_SetExceptionHandler(int offset) Q_DECL_OVERRIDE;
    void generate_ThrowException() Q_DECL_OVERRIDE;
    void generate_GetException() Q_DECL_OVERRIDE;
    void generate_SetException() Q_DECL_OVERRIDE;
    void generate_CreateCallContext() Q_DECL_OVERRIDE;
    void generate_PushCatchContext(int name, int reg) Q_DECL_OVERRIDE;
    void generate_PushWithContext(int reg) Q_DECL_OVERRIDE;
    void generate_PopContext(int reg) Q_DECL_OVERRIDE;
    void generate_ForeachIteratorObject() Q_DECL_OVERRIDE;
    void generate_ForeachNextPropertyName() Q_DECL_OVERRIDE;
    void generate_DeleteMember(int member, int base) Q_DECL_OVERRIDE;
    void generate_DeleteSubscript(int base, int index) Q_DECL_OVERRIDE;
    void generate_DeleteName(int name) Q_DECL_OVERRIDE;
    void generate_TypeofName(int name) Q_DECL_OVERRIDE;
    void generate_TypeofValue() Q_DECL_OVERRIDE;
    void generate_DeclareVar(int varName, int isDeletable) Q_DECL_OVERRIDE;
    void generate_DefineArray(int argc, int args) Q_DECL_OVERRIDE;
    void generate_DefineObjectLiteral(int internalClassId, int arrayValueCount,
                                      int arrayGetterSetterCountAndFlags,
                                      int args) Q_DECL_OVERRIDE;
    void generate_CreateMappedArgumentsObject() Q_DECL_OVERRIDE;
    void generate_CreateUnmappedArgumentsObject() Q_DECL_OVERRIDE;
    void generate_ConvertThisToObject() Q_DECL_OVERRIDE;
    void generate_Construct(int func, int argc, int argv) Q_DECL_OVERRIDE;
    void generate_Jump(int offset) Q_DECL_OVERRIDE;
    void generate_JumpTrue(int offset) Q_DECL_OVERRIDE;
    void generate_JumpFalse(int offset) Q_DECL_OVERRIDE;
    void generate_CmpEqNull() Q_DECL_OVERRIDE;
    void generate_CmpNeNull() Q_DECL_OVERRIDE;
    void generate_CmpEqInt(int lhs) Q_DECL_OVERRIDE;
    void generate_CmpNeInt(int lhs) Q_DECL_OVERRIDE;
    void generate_CmpEq(int lhs) Q_DECL_OVERRIDE;
    void generate_CmpNe(int lhs) Q_DECL_OVERRIDE;
    void generate_CmpGt(int lhs) Q_DECL_OVERRIDE;
    void generate_CmpGe(int lhs) Q_DECL_OVERRIDE;
    void generate_CmpLt(int lhs) Q_DECL_OVERRIDE;
    void generate_CmpLe(int lhs) Q_DECL_OVERRIDE;
    void generate_CmpStrictEqual(int lhs) Q_DECL_OVERRIDE;
    void generate_CmpStrictNotEqual(int lhs) Q_DECL_OVERRIDE;
    void generate_CmpIn(int lhs) Q_DECL_OVERRIDE;
    void generate_CmpInstanceOf(int lhs) Q_DECL_OVERRIDE;
    void generate_JumpStrictEqualStackSlotInt(int lhs, int rhs,
                                              int offset) Q_DECL_OVERRIDE;
    void generate_JumpStrictNotEqualStackSlotInt(int lhs, int rhs,
                                                 int offset) Q_DECL_OVERRIDE;
    void generate_UNot() Q_DECL_OVERRIDE;
    void generate_UPlus() Q_DECL_OVERRIDE;
    void generate_UMinus() Q_DECL_OVERRIDE;
    void generate_UCompl() Q_DECL_OVERRIDE;
    void generate_Increment() Q_DECL_OVERRIDE;
    void generate_Decrement() Q_DECL_OVERRIDE;
    void generate_Add(int lhs) Q_DECL_OVERRIDE;
    void generate_BitAnd(int lhs) Q_DECL_OVERRIDE;
    void generate_BitOr(int lhs) Q_DECL_OVERRIDE;
    void generate_BitXor(int lhs) Q_DECL_OVERRIDE;
    void generate_UShr(int lhs) Q_DECL_OVERRIDE;
    void generate_Shr(int lhs) Q_DECL_OVERRIDE;
    void generate_Shl(int lhs) Q_DECL_OVERRIDE;
    void generate_BitAndConst(int rhs) Q_DECL_OVERRIDE;
    void generate_BitOrConst(int rhs) Q_DECL_OVERRIDE;
    void generate_BitXorConst(int rhs) Q_DECL_OVERRIDE;
    void generate_UShrConst(int rhs) Q_DECL_OVERRIDE;
    void generate_ShrConst(int rhs) Q_DECL_OVERRIDE;
    void generate_ShlConst(int rhs) Q_DECL_OVERRIDE;
    void generate_Mul(int lhs) Q_DECL_OVERRIDE;
    void generate_Div(int lhs) Q_DECL_OVERRIDE;
    void generate_Mod(int lhs) Q_DECL_OVERRIDE;
    void generate_Sub(int lhs) Q_DECL_OVERRIDE;
    void generate_LoadQmlContext(int result) Q_DECL_OVERRIDE;
    void generate_LoadQmlImportedScripts(int result) Q_DECL_OVERRIDE;
    void generate_LoadQmlSingleton(int name) Q_DECL_OVERRIDE;

    void startInstruction(Moth::Instr::Type instr) Q_DECL_OVERRIDE;
    void endInstruction(Moth::Instr::Type instr) Q_DECL_OVERRIDE;

protected:
    bool hasLabel() const
    { return std::find(labels.cbegin(), labels.cend(), instructionOffset()) != labels.cend(); }

private:
    void collectLabelsInBytecode();

private:
    QV4::Function *function;
    QScopedPointer<Assembler> as;
    std::vector<int> labels;
};
#endif // V4_ENABLE_JIT

} // namespace JIT
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4JIT_P_H
