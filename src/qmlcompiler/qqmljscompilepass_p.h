/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLJSCOMPILEPASS_P_H
#define QQMLJSCOMPILEPASS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.


#include <private/qqmljslogger_p.h>
#include <private/qqmljsregistercontent_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qv4bytecodehandler_p.h>
#include <private/qv4compiler_p.h>

QT_BEGIN_NAMESPACE

class QQmlJSCompilePass : public QV4::Moth::ByteCodeHandler
{
    Q_DISABLE_COPY_MOVE(QQmlJSCompilePass)
public:
    enum RegisterShortcuts {
        Accumulator = QV4::CallData::Accumulator,
        FirstArgument = QV4::CallData::OffsetCount
    };

    using SourceLocationTable = QV4::Compiler::Context::SourceLocationTable;

    // map from register index to expected type
    using VirtualRegisters = QHash<int, QQmlJSRegisterContent>;

    struct InstructionAnnotation
    {
        VirtualRegisters registers;
        VirtualRegisters expectedTargetTypesBeforeJump;
    };

    using InstructionAnnotations = QHash<int, InstructionAnnotation>;

    struct Function
    {
        QQmlJSScopesById addressableScopes;
        QList<QQmlJSScope::ConstPtr> argumentTypes;
        QQmlJSScope::ConstPtr returnType;
        QQmlJSScope::ConstPtr qmlScope;
        QByteArray code;
        const SourceLocationTable *sourceLocations = nullptr;
        bool isSignalHandler = false;
        bool isQPropertyBinding = false;
    };

    struct State
    {
        VirtualRegisters registers;
        QQmlJSRegisterContent accumulatorIn;
        QQmlJSRegisterContent accumulatorOut;
    };

    QQmlJSCompilePass(const QV4::Compiler::JSUnitGenerator *jsUnitGenerator,
                      const QQmlJSTypeResolver *typeResolver, QQmlJSLogger *logger)
        : m_jsUnitGenerator(jsUnitGenerator)
        , m_typeResolver(typeResolver)
        , m_logger(logger)
    {}

protected:
    const QV4::Compiler::JSUnitGenerator *m_jsUnitGenerator = nullptr;
    const QQmlJSTypeResolver *m_typeResolver = nullptr;
    QQmlJSLogger *m_logger = nullptr;

    const Function *m_function = nullptr;
    QQmlJS::DiagnosticMessage *m_error = nullptr;

    State initialState(const Function *function, const QQmlJSTypeResolver *resolver)
    {
        State state;
        for (int i = 0; i < function->argumentTypes.count(); ++i) {
            state.registers[FirstArgument + i]
                    = resolver->globalType(function->argumentTypes.at(i));
        }
        return state;
    }

    State nextStateFromAnnotations(
            const State &oldState, const InstructionAnnotations &annotations)
    {
        State newState;

        // Usually the initial accumulator type is the output of the previous instruction, but ...
        newState.accumulatorIn = oldState.accumulatorOut;

        const auto instruction = annotations.constFind(currentInstructionOffset());
        if (instruction != annotations.constEnd()) {
            const auto target = instruction->expectedTargetTypesBeforeJump.constFind(Accumulator);
            if (target != instruction->expectedTargetTypesBeforeJump.constEnd()) {
                // ... the initial type of the accumulator is given in expectedTargetTypesBeforeJump
                // if the current instruction can be jumped to.
                newState.accumulatorIn = *target;
            }

            newState.registers = instruction->registers;
            newState.accumulatorOut = instruction->registers[Accumulator];
        } else {
            newState.registers = VirtualRegisters();
            newState.accumulatorOut = QQmlJSRegisterContent();
        }

        return newState;
    }

    QQmlJS::SourceLocation sourceLocation(int instructionOffset) const
    {
        Q_ASSERT(m_function);
        Q_ASSERT(m_function->sourceLocations);
        const auto &entries = m_function->sourceLocations->entries;
        auto item = std::lower_bound(entries.begin(), entries.end(), instructionOffset,
                                     [](auto entry, uint offset) { return entry.offset < offset; });

        Q_ASSERT(item != entries.end());
        return item->location;
    }

    QQmlJS::SourceLocation currentSourceLocation() const
    {
        return sourceLocation(currentInstructionOffset());
    }

    void setError(const QString &message, int instructionOffset)
    {
        Q_ASSERT(m_error);
        if (m_error->isValid())
            return;
        m_error->message = message;
        m_error->loc = sourceLocation(instructionOffset);
    }

    void setError(const QString &message)
    {
        setError(message, currentInstructionOffset());
    }

    // Stub out all the methods so that passes can choose to only implement part of them.
    void generate_Add(int) override {}
    void generate_As(int) override {}
    void generate_BitAnd(int) override {}
    void generate_BitAndConst(int) override {}
    void generate_BitOr(int) override {}
    void generate_BitOrConst(int) override {}
    void generate_BitXor(int) override {}
    void generate_BitXorConst(int) override {}
    void generate_CallElement(int, int, int, int) override {}
    void generate_CallGlobalLookup(int, int, int) override {}
    void generate_CallName(int, int, int) override {}
    void generate_CallPossiblyDirectEval(int, int) override {}
    void generate_CallProperty(int, int, int, int) override {}
    void generate_CallPropertyLookup(int, int, int, int) override {}
    void generate_CallQmlContextPropertyLookup(int, int, int) override {}
    void generate_CallValue(int, int, int) override {}
    void generate_CallWithReceiver(int, int, int, int) override {}
    void generate_CallWithSpread(int, int, int, int) override {}
    void generate_CheckException() override {}
    void generate_CloneBlockContext() override {}
    void generate_CmpEq(int) override {}
    void generate_CmpEqInt(int) override {}
    void generate_CmpEqNull() override {}
    void generate_CmpGe(int) override {}
    void generate_CmpGt(int) override {}
    void generate_CmpIn(int) override {}
    void generate_CmpInstanceOf(int) override {}
    void generate_CmpLe(int) override {}
    void generate_CmpLt(int) override {}
    void generate_CmpNe(int) override {}
    void generate_CmpNeInt(int) override {}
    void generate_CmpNeNull() override {}
    void generate_CmpStrictEqual(int) override {}
    void generate_CmpStrictNotEqual(int) override {}
    void generate_Construct(int, int, int) override {}
    void generate_ConstructWithSpread(int, int, int) override {}
    void generate_ConvertThisToObject() override {}
    void generate_CreateCallContext() override {}
    void generate_CreateClass(int, int, int) override {}
    void generate_CreateMappedArgumentsObject() override {}
    void generate_CreateRestParameter(int) override {}
    void generate_CreateUnmappedArgumentsObject() override {}
    void generate_DeadTemporalZoneCheck(int) override {}
    void generate_Debug() override {}
    void generate_DeclareVar(int, int) override {}
    void generate_Decrement() override {}
    void generate_DefineArray(int, int) override {}
    void generate_DefineObjectLiteral(int, int, int) override {}
    void generate_DeleteName(int) override {}
    void generate_DeleteProperty(int, int) override {}
    void generate_DestructureRestElement() override {}
    void generate_Div(int) override {}
    void generate_Exp(int) override {}
    void generate_GetException() override {}
    void generate_GetIterator(int) override {}
    void generate_GetLookup(int) override {}
    void generate_GetOptionalLookup(int, int) override {}
    void generate_GetTemplateObject(int) override {}
    void generate_Increment() override {}
    void generate_InitializeBlockDeadTemporalZone(int, int) override {}
    void generate_IteratorClose(int) override {}
    void generate_IteratorNext(int, int) override {}
    void generate_IteratorNextForYieldStar(int, int) override {}
    void generate_Jump(int) override {}
    void generate_JumpFalse(int) override {}
    void generate_JumpNoException(int) override {}
    void generate_JumpNotUndefined(int) override {}
    void generate_JumpTrue(int) override {}
    void generate_LoadClosure(int) override {}
    void generate_LoadConst(int) override {}
    void generate_LoadElement(int) override {}
    void generate_LoadFalse() override {}
    void generate_LoadGlobalLookup(int) override {}
    void generate_LoadImport(int) override {}
    void generate_LoadInt(int) override {}
    void generate_LoadLocal(int) override {}
    void generate_LoadName(int) override {}
    void generate_LoadNull() override {}
    void generate_LoadOptionalProperty(int, int) override {}
    void generate_LoadProperty(int) override {}
    void generate_LoadQmlContextPropertyLookup(int) override {}
    void generate_LoadReg(int) override {}
    void generate_LoadRuntimeString(int) override {}
    void generate_LoadScopedLocal(int, int) override {}
    void generate_LoadSuperConstructor() override {}
    void generate_LoadSuperProperty(int) override {}
    void generate_LoadTrue() override {}
    void generate_LoadUndefined() override {}
    void generate_LoadZero() override {}
    void generate_Mod(int) override {}
    void generate_MoveConst(int, int) override {}
    void generate_MoveReg(int, int) override {}
    void generate_MoveRegExp(int, int) override {}
    void generate_Mul(int) override {}
    void generate_PopContext() override {}
    void generate_PopScriptContext() override {}
    void generate_PushBlockContext(int) override {}
    void generate_PushCatchContext(int, int) override {}
    void generate_PushScriptContext(int) override {}
    void generate_PushWithContext() override {}
    void generate_Resume(int) override {}
    void generate_Ret() override {}
    void generate_SetException() override {}
    void generate_SetLookup(int, int) override {}
    void generate_SetUnwindHandler(int) override {}
    void generate_Shl(int) override {}
    void generate_ShlConst(int) override {}
    void generate_Shr(int) override {}
    void generate_ShrConst(int) override {}
    void generate_StoreElement(int, int) override {}
    void generate_StoreLocal(int) override {}
    void generate_StoreNameSloppy(int) override {}
    void generate_StoreNameStrict(int) override {}
    void generate_StoreProperty(int, int) override {}
    void generate_StoreReg(int) override {}
    void generate_StoreScopedLocal(int, int) override {}
    void generate_StoreSuperProperty(int) override {}
    void generate_Sub(int) override {}
    void generate_TailCall(int, int, int, int) override {}
    void generate_ThrowException() override {}
    void generate_ThrowOnNullOrUndefined() override {}
    void generate_ToObject() override {}
    void generate_TypeofName(int) override {}
    void generate_TypeofValue() override {}
    void generate_UCompl() override {}
    void generate_UMinus() override {}
    void generate_UNot() override {}
    void generate_UPlus() override {}
    void generate_UShr(int) override {}
    void generate_UShrConst(int) override {}
    void generate_UnwindDispatch() override {}
    void generate_UnwindToLabel(int, int) override {}
    void generate_Yield() override {}
    void generate_YieldStar() override {}
};

QT_END_NAMESPACE

#endif // QQMLJSCOMPILEPASS_P_H
