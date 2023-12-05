// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSCodeGenerator : public QQmlJSCompilePass
{
public:
    QQmlJSCodeGenerator(const QV4::Compiler::Context *compilerContext,
                       const QV4::Compiler::JSUnitGenerator *unitGenerator,
                       const QQmlJSTypeResolver *typeResolver,
                       QQmlJSLogger *logger);
    ~QQmlJSCodeGenerator() = default;

    QQmlJSAotFunction run(const Function *function, const InstructionAnnotations *annotations,
                          QQmlJS::DiagnosticMessage *error, bool basicBlocksValidationFailed);

protected:
    struct CodegenState : public State
    {
        QString accumulatorVariableIn;
        QString accumulatorVariableOut;
    };

    // This is an RAII helper we can use to automatically convert the result of "inflexible"
    // operations to the desired type. For example GetLookup can only retrieve the type of
    // the property we're looking up. If we want to store a different type, we need to convert.
    struct Q_QMLCOMPILER_PRIVATE_EXPORT AccumulatorConverter
    {
        Q_DISABLE_COPY_MOVE(AccumulatorConverter);
        AccumulatorConverter(QQmlJSCodeGenerator *generator);
        ~AccumulatorConverter();

    private:
        const QQmlJSRegisterContent accumulatorOut;
        const QString accumulatorVariableIn;
        const QString accumulatorVariableOut;
        QQmlJSCodeGenerator *generator = nullptr;
    };

    virtual QString metaObject(const QQmlJSScope::ConstPtr &objectType);
    virtual QString metaType(const QQmlJSScope::ConstPtr &type);

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
    void generate_IteratorNext(int value, int offset) override;
    void generate_IteratorNextForYieldStar(int iterator, int object, int offset) override;
    void generate_IteratorClose() override;
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

    void addInclude(const QString &include)
    {
        Q_ASSERT(!include.isEmpty());
        m_includes.append(include);
    }

    QString conversion(const QQmlJSRegisterContent &from,
                       const QQmlJSRegisterContent &to,
                       const QString &variable);

    QString conversion(const QQmlJSScope::ConstPtr &from,
                       const QQmlJSRegisterContent &to,
                       const QString &variable)
    {
        const QQmlJSScope::ConstPtr contained = m_typeResolver->containedType(to);
        if (m_typeResolver->equals(to.storedType(), contained)
                || m_typeResolver->isNumeric(to.storedType())
                || to.storedType()->isReferenceType()
                || m_typeResolver->equals(from, contained)) {
            // If:
            // * the output is not actually wrapped at all, or
            // * the output is a number (as there are no internals to a number)
            // * the output is a QObject pointer, or
            // * we merely wrap the value into a new container,
            // we can convert by stored type.
            return convertStored(from, to.storedType(), variable);
        } else {
            return convertContained(m_typeResolver->globalType(from), to, variable);
        }
    }

    QString convertStored(const QQmlJSScope::ConstPtr &from,
                          const QQmlJSScope::ConstPtr &to,
                          const QString &variable);

    QString convertContained(const QQmlJSRegisterContent &from,
                             const QQmlJSRegisterContent &to,
                             const QString &variable);

    QString errorReturnValue();
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
    QString lookupVariable(int lookupIndex) const;
    QString consumedRegisterVariable(int index) const;
    QString consumedAccumulatorVariableIn() const;

    QString changedRegisterVariable() const;
    QQmlJSRegisterContent registerType(int index) const;
    QQmlJSRegisterContent lookupType(int lookupIndex) const;
    bool shouldMoveRegister(int index) const;

    QString m_body;
    CodegenState m_state;

    void resetState() { m_state = CodegenState(); }

private:
    void generateExceptionCheck();

    void generateEqualityOperation(
            const QQmlJSRegisterContent &lhsContent, const QString &lhsName,
            const QString &function, bool invert) {
        generateEqualityOperation(
                lhsContent, m_state.accumulatorIn(), lhsName, m_state.accumulatorVariableIn,
                function, invert);
    }

    void generateEqualityOperation(
            const QQmlJSRegisterContent &lhsContent, const QQmlJSRegisterContent &rhsContent,
            const QString &lhsName, const QString &rhsName, const QString &function, bool invert);
    void generateCompareOperation(int lhs, const QString &cppOperator);
    void generateArithmeticOperation(int lhs, const QString &cppOperator);
    void generateShiftOperation(int lhs, const QString &cppOperator);
    void generateArithmeticOperation(
            const QString &lhs, const QString &rhs, const QString &cppOperator);
    void generateArithmeticConstOperation(int lhsConst, const QString &cppOperator);
    void generateJumpCodeWithTypeConversions(int relativeOffset);
    void generateUnaryOperation(const QString &cppOperator);
    void generateInPlaceOperation(const QString &cppOperator);
    void generateMoveOutVar(const QString &outVar);
    void generateTypeLookup(int index);
    void generateVariantEqualityComparison(
            const QQmlJSRegisterContent &nonStorable, const QString &registerName, bool invert);
    void generateVariantEqualityComparison(
            const QQmlJSRegisterContent &storableContent, const QString &typedRegisterName,
            const QString &varRegisterName, bool invert);
    void generateArrayInitializer(int argc, int argv);
    void generateWriteBack(int registerIndex);
    void rejectIfNonQObjectOut(const QString &error);
    void rejectIfBadArray();


    QString eqIntExpression(int lhsConst);
    QString argumentsList(int argc, int argv, QString *outVar);
    QString castTargetName(const QQmlJSScope::ConstPtr &type) const;

    bool inlineStringMethod(const QString &name, int base, int argc, int argv);
    bool inlineTranslateMethod(const QString &name, int argc, int argv);
    bool inlineMathMethod(const QString &name, int argc, int argv);
    bool inlineConsoleMethod(const QString &name, int argc, int argv);
    bool inlineArrayMethod(const QString &name, int base, int argc, int argv);

    void generate_GetLookupHelper(int index);

    QQmlJSScope::ConstPtr mathObject() const
    {
        using namespace Qt::StringLiterals;
        return m_typeResolver->jsGlobalObject()->property(u"Math"_s).type();
    }

    QQmlJSScope::ConstPtr consoleObject() const
    {
        using namespace Qt::StringLiterals;
        return m_typeResolver->jsGlobalObject()->property(u"console"_s).type();
    }

    QString resolveValueTypeContentPointer(
            const QQmlJSScope::ConstPtr &required, const QQmlJSRegisterContent &actual,
            const QString &variable, const QString &errorMessage);
    QString resolveQObjectPointer(
            const QQmlJSScope::ConstPtr &required, const QQmlJSRegisterContent &actual,
            const QString &variable, const QString &errorMessage);
    bool generateContentPointerCheck(
            const QQmlJSScope::ConstPtr &required, const QQmlJSRegisterContent &actual,
            const QString &variable, const QString &errorMessage);

    // map from instruction offset to sequential label number
    QHash<int, QString> m_labels;

    const QV4::Compiler::Context *m_context = nullptr;
    const InstructionAnnotations *m_annotations = nullptr;

    bool m_skipUntilNextLabel = false;

    QStringList m_includes;

    struct RegisterVariablesKey
    {
        QString internalName;
        int registerIndex = -1;
        int lookupIndex = QQmlJSRegisterContent::InvalidLookupIndex;

    private:
        friend size_t qHash(const RegisterVariablesKey &key, size_t seed = 0) noexcept
        {
            return qHashMulti(seed, key.internalName, key.registerIndex, key.lookupIndex);
        }

        friend bool operator==(
                const RegisterVariablesKey &lhs, const RegisterVariablesKey &rhs) noexcept
        {
            return lhs.registerIndex == rhs.registerIndex
                    && lhs.lookupIndex == rhs.lookupIndex
                    && lhs.internalName == rhs.internalName;
        }

        friend bool operator!=(
                const RegisterVariablesKey &lhs, const RegisterVariablesKey &rhs) noexcept
        {
            return !(lhs == rhs);
        }
    };

    struct RegisterVariablesValue
    {
        QString variableName;
        QQmlJSScope::ConstPtr storedType;
        int numTracked = 0;
    };

    QHash<RegisterVariablesKey, RegisterVariablesValue> m_registerVariables;
};

QT_END_NAMESPACE

#endif // QQMLJSCODEGENERATOR_P_H
