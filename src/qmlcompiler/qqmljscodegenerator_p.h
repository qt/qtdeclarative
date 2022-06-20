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

#ifndef QQMLJSCODEGENERATOR_P_H
#define QQMLJSCODEGENERATOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qqmljscompiler_p.h>
#include <private/qqmljstypepropagator_p.h>
#include <private/qqmljstyperesolver_p.h>

#include <private/qv4bytecodehandler_p.h>
#include <private/qv4codegen_p.h>

#include <QtCore/qstring.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QQmlJSCodeGenerator : public QQmlJSCompilePass
{
public:
    QQmlJSCodeGenerator(const QV4::Compiler::Context *compilerContext,
                       const QV4::Compiler::JSUnitGenerator *unitGenerator,
                       const QQmlJSTypeResolver *typeResolver,
                       QQmlJSLogger *logger, const QStringList &sourceCodeLines);
    ~QQmlJSCodeGenerator() = default;

    QQmlJSAotFunction run(const Function *function, const InstructionAnnotations *annotations,
                          QQmlJS::DiagnosticMessage *error);

protected:
    enum class JumpMode { None, Conditional, Unconditional };

    class Section
    {
    public:
        Section &operator+=(const QString &code) { m_code += code; return *this; }
        Section &operator+=(const QChar &code) { m_code += code; return *this; }

        bool addReadRegister(const QString &name)
        {
            if (m_readRegisters.contains(name))
                return false;
            m_readRegisters.append(name);
            return true;
        }

        void setWriteRegister(const QString &name) { m_writeRegister = name; }
        void setHasSideEffects(bool hasSideEffects) { m_hasSideEffects = hasSideEffects; }
        void setLabel(const QString &label) { m_label = label; }
        void setJump(const QString &target, JumpMode mode)
        {
            m_jumpTarget = target;
            m_jumpMode = mode;
        }

        bool readsRegister(const QString &name) const { return m_readRegisters.contains(name); }

        QString code() const { return m_code; }
        QString writeRegister() const { return m_writeRegister; }
        QStringList readRegisters() const { return m_readRegisters; }
        bool hasSideEffects() const { return m_hasSideEffects; }

        QString label() const { return m_label; }
        JumpMode jumpMode() const { return m_jumpMode; }
        QString jumpTarget() const { return m_jumpTarget; }

    private:
        QString m_code;
        QString m_writeRegister;
        QString m_label;
        QStringList m_readRegisters;
        QString m_jumpTarget;
        JumpMode m_jumpMode = JumpMode::None;
        bool m_hasSideEffects = false;
    };

    struct CodegenState : public State
    {
        QString accumulatorVariableIn;
        QString accumulatorVariableOut;
    };

    virtual QString metaObject(const QQmlJSScope::ConstPtr &objectType);

    void generate_Ret() override;
    void generate_Debug() override;
    void generate_LoadConst(int index) override;
    void generate_LoadZero() override;
    void generate_LoadTrue() override;
    void generate_LoadFalse() override;
    void generate_LoadNull() override;
    void generate_LoadUndefined() override;
    void generate_LoadInt(int value) override;
    void generate_MoveConst(int constIndex, int destTemp) override;
    void generate_LoadReg(int reg) override;
    void generate_StoreReg(int reg) override;
    void generate_MoveReg(int srcReg, int destReg) override;
    void generate_LoadImport(int index) override;
    void generate_LoadLocal(int index) override;
    void generate_StoreLocal(int index) override;
    void generate_LoadScopedLocal(int scope, int index) override;
    void generate_StoreScopedLocal(int scope, int index) override;
    void generate_LoadRuntimeString(int stringId) override;
    void generate_MoveRegExp(int regExpId, int destReg) override;
    void generate_LoadClosure(int value) override;
    void generate_LoadName(int nameIndex) override;
    void generate_LoadGlobalLookup(int index) override;
    void generate_LoadQmlContextPropertyLookup(int index) override;
    void generate_StoreNameSloppy(int nameIndex) override;
    void generate_StoreNameStrict(int name) override;
    void generate_LoadElement(int base) override;
    void generate_StoreElement(int base, int index) override;
    void generate_LoadProperty(int nameIndex) override;
    void generate_LoadOptionalProperty(int name, int offset) override;
    void generate_GetLookup(int index) override;
    void generate_GetOptionalLookup(int index, int offset) override;
    void generate_StoreProperty(int name, int baseReg) override;
    void generate_SetLookup(int index, int base) override;
    void generate_LoadSuperProperty(int property) override;
    void generate_StoreSuperProperty(int property) override;
    void generate_Yield() override;
    void generate_YieldStar() override;
    void generate_Resume(int) override;

    void generate_CallValue(int name, int argc, int argv) override;
    void generate_CallWithReceiver(int name, int thisObject, int argc, int argv) override;
    void generate_CallProperty(int name, int base, int argc, int argv) override;
    void generate_CallPropertyLookup(int lookupIndex, int base, int argc, int argv) override;
    void generate_CallElement(int base, int index, int argc, int argv) override;
    void generate_CallName(int name, int argc, int argv) override;
    void generate_CallPossiblyDirectEval(int argc, int argv) override;
    void generate_CallGlobalLookup(int index, int argc, int argv) override;
    void generate_CallQmlContextPropertyLookup(int index, int argc, int argv) override;
    void generate_CallWithSpread(int func, int thisObject, int argc, int argv) override;
    void generate_TailCall(int func, int thisObject, int argc, int argv) override;
    void generate_Construct(int func, int argc, int argv) override;
    void generate_ConstructWithSpread(int func, int argc, int argv) override;
    void generate_SetUnwindHandler(int offset) override;
    void generate_UnwindDispatch() override;
    void generate_UnwindToLabel(int level, int offset) override;
    void generate_DeadTemporalZoneCheck(int name) override;
    void generate_ThrowException() override;
    void generate_GetException() override;
    void generate_SetException() override;
    void generate_CreateCallContext() override;
    void generate_PushCatchContext(int index, int name) override;
    void generate_PushWithContext() override;
    void generate_PushBlockContext(int index) override;
    void generate_CloneBlockContext() override;
    void generate_PushScriptContext(int index) override;
    void generate_PopScriptContext() override;
    void generate_PopContext() override;
    void generate_GetIterator(int iterator) override;
    void generate_IteratorNext(int value, int done) override;
    void generate_IteratorNextForYieldStar(int iterator, int object) override;
    void generate_IteratorClose(int done) override;
    void generate_DestructureRestElement() override;
    void generate_DeleteProperty(int base, int index) override;
    void generate_DeleteName(int name) override;
    void generate_TypeofName(int name) override;
    void generate_TypeofValue() override;
    void generate_DeclareVar(int varName, int isDeletable) override;
    void generate_DefineArray(int argc, int args) override;
    void generate_DefineObjectLiteral(int internalClassId, int argc, int args) override;
    void generate_CreateClass(int classIndex, int heritage, int computedNames) override;
    void generate_CreateMappedArgumentsObject() override;
    void generate_CreateUnmappedArgumentsObject() override;
    void generate_CreateRestParameter(int argIndex) override;
    void generate_ConvertThisToObject() override;
    void generate_LoadSuperConstructor() override;
    void generate_ToObject() override;
    void generate_Jump(int offset) override;
    void generate_JumpTrue(int offset) override;
    void generate_JumpFalse(int offset) override;
    void generate_JumpNoException(int offset) override;
    void generate_JumpNotUndefined(int offset) override;
    void generate_CheckException() override;
    void generate_CmpEqNull() override;
    void generate_CmpNeNull() override;
    void generate_CmpEqInt(int lhs) override;
    void generate_CmpNeInt(int lhs) override;
    void generate_CmpEq(int lhs) override;
    void generate_CmpNe(int lhs) override;
    void generate_CmpGt(int lhs) override;
    void generate_CmpGe(int lhs) override;
    void generate_CmpLt(int lhs) override;
    void generate_CmpLe(int lhs) override;
    void generate_CmpStrictEqual(int lhs) override;
    void generate_CmpStrictNotEqual(int lhs) override;
    void generate_CmpIn(int lhs) override;
    void generate_CmpInstanceOf(int lhs) override;
    void generate_As(int lhs) override;
    void generate_UNot() override;
    void generate_UPlus() override;
    void generate_UMinus() override;
    void generate_UCompl() override;
    void generate_Increment() override;
    void generate_Decrement() override;
    void generate_Add(int lhs) override;
    void generate_BitAnd(int lhs) override;
    void generate_BitOr(int lhs) override;
    void generate_BitXor(int lhs) override;
    void generate_UShr(int lhs) override;
    void generate_Shr(int lhs) override;
    void generate_Shl(int lhs) override;
    void generate_BitAndConst(int rhs) override;
    void generate_BitOrConst(int rhs) override;
    void generate_BitXorConst(int rhs) override;
    void generate_UShrConst(int rhs) override;
    void generate_ShrConst(int value) override;
    void generate_ShlConst(int rhs) override;
    void generate_Exp(int lhs) override;
    void generate_Mul(int lhs) override;
    void generate_Div(int lhs) override;
    void generate_Mod(int lhs) override;
    void generate_Sub(int lhs) override;
    void generate_InitializeBlockDeadTemporalZone(int firstReg, int count) override;
    void generate_ThrowOnNullOrUndefined() override;
    void generate_GetTemplateObject(int index) override;

    Verdict startInstruction(QV4::Moth::Instr::Type) override;
    void endInstruction(QV4::Moth::Instr::Type) override;

    const QString &use(const QString &variable)
    {
        m_body.addReadRegister(variable);
        return variable;
    }

    void addInclude(const QString &include)
    {
        Q_ASSERT(!include.isEmpty());
        m_includes.append(include);
    }

    QString conversion(const QQmlJSRegisterContent &from,
                       const QQmlJSRegisterContent &to,
                       const QString &variable) const
    {
        return conversion(from.storedType(), to.storedType(), variable);
    }

    QString conversion(const QQmlJSScope::ConstPtr &from,
                       const QQmlJSScope::ConstPtr &to,
                       const QString &variable) const;

    QString errorReturnValue() const;
    void reject(const QString &thing);

    QString metaTypeFromType(const QQmlJSScope::ConstPtr &type) const;
    QString metaTypeFromName(const QQmlJSScope::ConstPtr &type) const;
    QString contentPointer(const QQmlJSRegisterContent &content, const QString &var);
    QString contentType(const QQmlJSRegisterContent &content, const QString &var);

    void generateSetInstructionPointer();
    void generateLookup(const QString &lookup, const QString &initialization,
                        const QString &resultPreparation = QString());
    QString getLookupPreparation(
            const QQmlJSRegisterContent &content, const QString &var, int lookup);
    QString setLookupPreparation(
            const QQmlJSRegisterContent &content, const QString &arg, int lookup);
    void generateEnumLookup(int index);

    QString registerVariable(int index) const;
    QQmlJSRegisterContent registerType(int index) const;

    Section m_body;
    CodegenState m_state;

private:
    enum class ReadMode { NoRead, SelfRead, Preserve };
    using RequiredRegisters = QList<QHash<QString, ReadMode>>;

    struct BasicBlock
    {
        int beginSection = -1;
        int endSection = -1;

        QString label;
        QString jumpTarget;
        QQmlJSCodeGenerator::JumpMode jumpMode = QQmlJSCodeGenerator::JumpMode::None;

        QList<int> previousBlocks;
        int jumpTargetBlock = -1;
    };

    void generateExceptionCheck();
    void generateEqualityOperation(int lhs, const QString &function, bool invert);
    void generateCompareOperation(int lhs, const QString &cppOperator);
    void generateArithmeticOperation(int lhs, const QString &cppOperator);
    void generateJumpCodeWithTypeConversions(int relativeOffset, JumpMode mode);
    void generateMoveOutVar(const QString &outVar);
    void generateTypeLookup(int index);
    void rejectIfNonQObjectOut(const QString &error);

    QString eqIntExpression(int lhsConst);
    QString argumentsList(int argc, int argv, QString *outVar);
    QString castTargetName(const QQmlJSScope::ConstPtr &type) const;

    void protectAccumulator();

    QList<BasicBlock> findBasicBlocks(const QList<Section> &sections);
    RequiredRegisters dropPreserveCycles(
            const QList<BasicBlock> &basicBlocks, const RequiredRegisters &requiredRegisters);
    void eliminateDeadStores();

    bool inlineMathMethod(const QString &name, int argc, int argv);
    QQmlJSScope::ConstPtr mathObject() const
    {
        return m_typeResolver->jsGlobalObject()->property(u"Math"_qs).type();
    }

    bool isArgument(int registerIndex) const
    {
        return registerIndex >= QV4::CallData::OffsetCount && registerIndex < firstRegisterIndex();
    }

    int firstRegisterIndex() const
    {
        return QV4::CallData::OffsetCount + m_function->argumentTypes.count();
    }

    int nextJSLine(uint line) const;
    void nextSection()
    {
        if (!m_body.code().isEmpty())
            m_sections.append(std::move(m_body));
        m_body = Section();
    }

    QStringList m_sourceCodeLines;

    // map from instruction offset to sequential label number
    QHash<int, QString> m_labels;
    QList<Section> m_sections;

    const QV4::Compiler::Context *m_context = nullptr;
    const InstructionAnnotations *m_annotations = nullptr;

    int m_lastLineNumberUsed = -1;
    bool m_skipUntilNextLabel = false;

    QStringList m_includes;
    QHash<int, QHash<QQmlJSScope::ConstPtr, QString>> m_registerVariables;
};

QT_END_NAMESPACE

#endif // QQMLJSCODEGENERATOR_P_H
