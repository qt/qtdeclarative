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

#include "qqmljscodegenerator_p.h"

#include <private/qqmljstypepropagator_p.h>

#include <private/qqmlirbuilder_p.h>
#include <private/qqmljsscope_p.h>
#include <private/qqmljsutils_p.h>
#include <private/qv4compilerscanfunctions_p.h>

#include <QtCore/qdir.h>
#include <QtCore/qfileinfo.h>

QT_BEGIN_NAMESPACE

/*!
 * \internal
 * \class QQmlJSCodeGenerator
 *
 * This is a final compile pass that generates C++ code from a function and the
 * annotations produced by previous passes. Such annotations are produced by
 * QQmlJSTypePropagator, and possibly amended by other passes.
 */

#define BYTECODE_UNIMPLEMENTED() Q_ASSERT_X(false, Q_FUNC_INFO, "not implemented");

#define INJECT_TRACE_INFO(function) \
    static const bool injectTraceInfo = true; \
    if (injectTraceInfo) { \
        m_body += u"// "_qs + QStringLiteral(#function) + u'\n'; \
    }

QString QQmlJSCodeGenerator::castTargetName(const QQmlJSScope::ConstPtr &type) const
{
    return type->augmentedInternalName();
}

QQmlJSCodeGenerator::QQmlJSCodeGenerator(const QV4::Compiler::Context *compilerContext,
        const QV4::Compiler::JSUnitGenerator *unitGenerator,
        const QQmlJSTypeResolver *typeResolver,
        QQmlJSLogger *logger, const QStringList &sourceCodeLines)
    : QQmlJSCompilePass(unitGenerator, typeResolver, logger)
    , m_sourceCodeLines(sourceCodeLines)
    , m_context(compilerContext)
{}

QString QQmlJSCodeGenerator::metaTypeFromType(const QQmlJSScope::ConstPtr &type) const
{
    return u"QMetaType::fromType<"_qs + type->augmentedInternalName() + u">()"_qs;
}

QString QQmlJSCodeGenerator::metaTypeFromName(const QQmlJSScope::ConstPtr &type) const
{
    return u"QMetaType::fromName(\""_qs
            + QString::fromUtf8(QMetaObject::normalizedType(type->augmentedInternalName().toUtf8()))
            + u"\")"_qs;
}

QString QQmlJSCodeGenerator::metaObject(const QQmlJSScope::ConstPtr &objectType)
{
    if (!objectType->isComposite()) {
        if (objectType->internalName() == u"QObject"_qs
                || objectType->internalName() == u"QQmlComponent"_qs) {
            return u'&' + objectType->internalName() + u"::staticMetaObject"_qs;
        }
        return metaTypeFromName(objectType) + u".metaObject()"_qs;
    }

    reject(u"retrieving the metaObject of a composite type without using an instance."_qs);
    return QString();
}

QQmlJSAotFunction QQmlJSCodeGenerator::run(
        const Function *function, const InstructionAnnotations *annotations,
        QQmlJS::DiagnosticMessage *error)
{
    m_annotations = annotations;
    m_function = function;
    m_error = error;

    QSet<QString> registerNames;
    const auto defineRegisterVariable = [&](
            int registerIndex, const QQmlJSScope::ConstPtr &seenType) {
        // Don't generate any variables for registers that are initialized with undefined.
        if (seenType.isNull() || seenType == m_typeResolver->voidType())
            return;

        auto &typesForRegisters = m_registerVariables[registerIndex];
        if (!typesForRegisters.contains(seenType)) {
            QString variableName = u"r%1"_qs.arg(registerIndex);
            if (registerNames.contains(variableName))
                variableName += u'_' + QString::number(typesForRegisters.count());
            registerNames.insert(variableName);
            typesForRegisters[seenType] = variableName;
        }
    };

    for (const InstructionAnnotation &annotation : *m_annotations) {
        for (auto regIt = annotation.registers.constBegin(),
             regEnd = annotation.registers.constEnd();
             regIt != regEnd;
             ++regIt) {
            defineRegisterVariable(regIt.key(), regIt.value().storedType());
        }

        for (auto regIt = annotation.expectedTargetTypesBeforeJump.constBegin(),
             regEnd = annotation.expectedTargetTypesBeforeJump.constEnd();
             regIt != regEnd;
             ++regIt) {
            defineRegisterVariable(regIt.key(), regIt.value().storedType());
        }
    }

    // ensure we have m_labels for loops
    for (const auto loopLabel : m_context->labelInfo)
        m_labels.insert(loopLabel, u"label_%1"_qs.arg(m_labels.count()));

    const QByteArray byteCode = function->code;
    decode(byteCode.constData(), static_cast<uint>(byteCode.length()));
    eliminateDeadStores();

    QQmlJSAotFunction result;
    result.includes.swap(m_includes);

    for (const auto &registerTypes : qAsConst(m_registerVariables)) {
        for (auto registerTypeIt = registerTypes.constBegin(), end = registerTypes.constEnd();
             registerTypeIt != end; ++registerTypeIt) {

            const QQmlJSScope::ConstPtr storedType = registerTypeIt.key();
            result.code += storedType->internalName();

            if (storedType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
                result.code += u" *"_qs;
            else
                result.code += u' ';

            result.code += registerTypeIt.value();
            result.code += u";\n"_qs;
        }
    }

    for (const Section &section : m_sections)
        result.code += section.code();

    for (const QQmlJSScope::ConstPtr &argType : qAsConst(function->argumentTypes)) {
        if (argType) {
            result.argumentTypes.append(argType->augmentedInternalName());
        } else {
            result.argumentTypes.append(u"void"_qs);
        }
    }

    if (function->returnType) {
        result.returnType = function->returnType->internalName();
        if (function->returnType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            result.returnType += u'*';
    } else {
        result.returnType = u"void"_qs;
    }

    return result;
}

QList<QQmlJSCodeGenerator::BasicBlock> QQmlJSCodeGenerator::findBasicBlocks(
        const QList<QQmlJSCodeGenerator::Section> &sections)
{
    using JumpMode = QQmlJSCodeGenerator::JumpMode;

    QList<BasicBlock> basicBlocks;
    BasicBlock currentBlock;
    currentBlock.beginSection = 0;
    for (int i = 0, end = sections.length(); i != end; ++i) {
        const QQmlJSCodeGenerator::Section &section = sections[i];
        const QString label = section.label();
        if (!label.isEmpty() || currentBlock.jumpMode != JumpMode::None) {
            if (currentBlock.beginSection != i) {
                currentBlock.endSection = i;
                basicBlocks.append(currentBlock);
                currentBlock.beginSection = i;
            }
            currentBlock.label = label;
        }
        currentBlock.jumpMode = section.jumpMode();
        currentBlock.jumpTarget = section.jumpTarget();
    }
    currentBlock.endSection = sections.length();
    basicBlocks.append(currentBlock);

    BasicBlock *prevBlock = nullptr;
    for (int i = 0, end = basicBlocks.length(); i != end; ++i) {
        BasicBlock *block = &basicBlocks[i];
        if (prevBlock && prevBlock->jumpMode != JumpMode::Unconditional)
            block->previousBlocks.append(i);
        if (!block->label.isEmpty()) {
            for (int j = 0, end = basicBlocks.length(); j != end; ++j) {
                BasicBlock *jumpFrom = &basicBlocks[j];
                if (jumpFrom->jumpMode == JumpMode::None || jumpFrom->jumpTarget != block->label)
                    continue;
                jumpFrom->jumpTargetBlock = i;
                block->previousBlocks.append(j);
            }
        }
    }

    return basicBlocks;
}

QQmlJSCodeGenerator::RequiredRegisters QQmlJSCodeGenerator::dropPreserveCycles(
        const QList<QQmlJSCodeGenerator::BasicBlock> &basicBlocks,
        const QQmlJSCodeGenerator::RequiredRegisters &requiredRegisters)
{
    RequiredRegisters result(requiredRegisters.length());
    for (int i = 0, blocksEnd = basicBlocks.length(); i != blocksEnd; ++i) {
        const BasicBlock &block = basicBlocks[i];
        const QHash<QString, ReadMode> &registers = requiredRegisters[i];
        QHash<QString, ReadMode> &resultRegisters = result[i];
        for (auto r = registers.begin(), registersEnd = registers.end(); r != registersEnd; ++r) {
            const QString variable = r.key();
            switch (*r) {
            case ReadMode::NoRead:
                resultRegisters[variable] = ReadMode::NoRead;
                continue;
            case ReadMode::Preserve:
                // Convert Preserve into NoRead and update below
                if (!resultRegisters.contains(variable))
                    resultRegisters[variable] = ReadMode::NoRead;
                continue;
            case ReadMode::SelfRead:
                resultRegisters[variable] = ReadMode::SelfRead;
                break;
            }

            QList<int> blocksToCheck;
            blocksToCheck.append(block.previousBlocks);

            while (!blocksToCheck.isEmpty()) {
                const int currentIndex = blocksToCheck.takeFirst();
                if (requiredRegisters[currentIndex][variable] == ReadMode::Preserve) {
                    result[currentIndex][variable] = ReadMode::Preserve;
                    blocksToCheck.append(basicBlocks[currentIndex].previousBlocks);
                }
            }
        }
    }
    return result;
}

void QQmlJSCodeGenerator::eliminateDeadStores()
{
    const QList<BasicBlock> basicBlocks = findBasicBlocks(m_sections);

    QList<int> toErase;
    bool foundUnknownBlock = false;
    RequiredRegisters requiredRegisters(basicBlocks.length());
    do {
        toErase.clear();
        foundUnknownBlock = false;
        for (auto &registerTypes : m_registerVariables) {
            for (auto registerTypeIt = registerTypes.constBegin();
                 registerTypeIt != registerTypes.constEnd();) {

                const QString variable = registerTypeIt.value();

                // Don't declare any variables for registers that contain undefined.
                if (variable.isEmpty()) {
                    registerTypeIt = registerTypes.erase(registerTypeIt);
                    continue;
                }

                bool usedOnce = false;
                ReadMode inUse = ReadMode::NoRead;
                int basicBlockIndex = basicBlocks.length() - 1;
                for (int i = m_sections.length() - 1; i >= 0; --i) {
                    Section &section = m_sections[i];
                    const BasicBlock *block = &basicBlocks[basicBlockIndex];
                    if (block->beginSection > i) {
                        requiredRegisters[basicBlockIndex][variable] = inUse;
                        block = &basicBlocks[--basicBlockIndex];
                        if (block->jumpMode == JumpMode::Unconditional)
                            inUse = ReadMode::NoRead;
                        else if (inUse == ReadMode::SelfRead)
                            inUse = ReadMode::Preserve;

                        if (block->jumpMode != JumpMode::None) {
                            QHash<QString, ReadMode> *blockRegisters
                                    = &requiredRegisters[block->jumpTargetBlock];
                            auto req = blockRegisters->find(variable);
                            if (req == blockRegisters->end()) {
                                foundUnknownBlock = true;
                                inUse = ReadMode::Preserve;
                            } else if (*req != ReadMode::NoRead) {
                                inUse = ReadMode::Preserve;
                            }
                        }
                    }

                    if (section.writeRegister() == variable) {
                        if (inUse == ReadMode::NoRead && !section.hasSideEffects()) {
                            toErase.append(i);
                        } else {
                            usedOnce = true;

                            // We can read and write the same register in one instruction.
                            // See Increment and Decrement
                            inUse = section.readsRegister(variable)
                                    ? ReadMode::SelfRead
                                    : ReadMode::NoRead;
                        }
                    } else if (section.readsRegister(variable)) {
                        inUse = ReadMode::SelfRead;
                        usedOnce = true;
                    }
                }
                requiredRegisters[0][variable] = inUse;

                if (!usedOnce) {
                    registerTypeIt = registerTypes.erase(registerTypeIt);
                    continue;
                }

                ++registerTypeIt;
            }
        }

        // Sort the offsets in reverse order so that we can pop from the back
        std::sort(toErase.begin(), toErase.end());
        int eraseIndex = toErase.length() - 1;
        for (int i = m_sections.length() - 1; i >= 0 && eraseIndex >= 0; --i) {
            if (i != toErase[eraseIndex])
                continue;
            Section &section = m_sections[i];
            QString code = section.code();
            section = Section();

            // Comment out the section.
            code.replace(u'\n', u"\n// "_qs);
            if (!code.startsWith(u"// "_qs) && !code.startsWith(u'\n'))
                code.prepend(u"// "_qs);

            // Make sure we end with a newline so that we don't comment out the next section.
            if (code.endsWith(u"\n// "))
                code.chop(3);
            else if (!code.endsWith(u'\n'))
                code.append(u'\n');

            section += code;
            --eraseIndex;
        }

        if (foundUnknownBlock)
            requiredRegisters = dropPreserveCycles(basicBlocks, requiredRegisters);

    } while (!toErase.isEmpty() || foundUnknownBlock);
}

QString QQmlJSCodeGenerator::errorReturnValue() const
{
    if (auto ret = m_function->returnType) {
        return ret->accessSemantics() == QQmlJSScope::AccessSemantics::Reference
                ? conversion(m_typeResolver->voidType(), ret, QString()) // produces nullptr
                : ret->internalName() + u"()"_qs;
    }
    return QString();
}

void QQmlJSCodeGenerator::generate_Ret()
{
    INJECT_TRACE_INFO(generate_Ret);

    m_body.setWriteRegister(QString());
    m_body.setHasSideEffects(true);

    if (m_function->returnType) {
        const QString signalUndefined = u"aotContext->setReturnValueUndefined();\n"_qs;
        if (!m_state.accumulatorVariableIn.isEmpty()) {
            const QString in = use(m_state.accumulatorVariableIn);
            if (m_state.accumulatorIn.storedType() == m_typeResolver->varType()) {
                m_body += u"if (!"_qs + in + u".isValid())\n"_qs;
                m_body += u"    "_qs + signalUndefined;
            } else if (m_state.accumulatorIn.storedType() == m_typeResolver->jsPrimitiveType()) {
                m_body += u"if ("_qs + in
                        + u".type() == QJSPrimitiveValue::Undefined)\n"_qs;
                m_body += u"    "_qs + signalUndefined;
            } else if (m_state.accumulatorIn.storedType() == m_typeResolver->jsValueType()) {
                m_body += u"if ("_qs + in + u".isUndefined())\n"_qs;
                m_body += u"    "_qs + signalUndefined;
            }
            m_body += u"return "_qs
                    + conversion(m_state.accumulatorIn.storedType(), m_function->returnType, in);
        } else if (m_function->returnType != m_typeResolver->voidType()
                   && m_function->returnType->internalName() != u"void"_qs) {
            if (m_function->returnType->internalName().trimmed().endsWith(u'*')
                || m_function->returnType->internalName().trimmed().endsWith(u'&')) {
                setError(u"Not all paths return a value"_qs);
                return;
            }

            m_body += signalUndefined;
            m_body += u"return "_qs + m_function->returnType->internalName() + u"()"_qs;
        }
    } else {
        m_body += u"return"_qs;
    }

    m_body += u";\n"_qs;
    m_skipUntilNextLabel = true;
}

void QQmlJSCodeGenerator::generate_Debug()
{
    BYTECODE_UNIMPLEMENTED();
}

static QString toNumericString(double value)
{
    if (value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max()) {
        const int i = value;
        if (i == value)
            return QString::number(i);
    }

    switch (qFpClassify(value)) {
    case FP_INFINITE: {
        const QString inf = u"std::numeric_limits<double>::infinity()"_qs;
        return std::signbit(value) ? (u'-' + inf) : inf;
    }
    case FP_NAN:
        return u"std::numeric_limits<double>::quiet_NaN()"_qs;
    case FP_ZERO:
        return std::signbit(value) ? u"-0.0"_qs : u"0"_qs;
    default:
        break;
    }

    return QString::number(value, 'f', std::numeric_limits<double>::max_digits10);
}

void QQmlJSCodeGenerator::generate_LoadConst(int index)
{
    INJECT_TRACE_INFO(generate_LoadConst);

    auto encodedConst = m_jsUnitGenerator->constant(index);
    double value = QV4::StaticValue::fromReturnedValue(encodedConst).doubleValue();
    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_qs;
    m_body += toNumericString(value); // ### handle other types
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_LoadZero()
{
    INJECT_TRACE_INFO(generate_LoadZero);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = 0"_qs;
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_LoadTrue()
{
    INJECT_TRACE_INFO(generate_LoadTrue);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = true"_qs;
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_LoadFalse()
{
    INJECT_TRACE_INFO(generate_LoadFalse);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = false"_qs;
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_LoadNull()
{
    INJECT_TRACE_INFO(generate_LoadNull);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = QJSPrimitiveNull()"_qs;
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_LoadUndefined()
{
    INJECT_TRACE_INFO(generate_LoadUndefined);
    m_body += m_state.accumulatorVariableOut + u" = "_qs + conversion(
                m_typeResolver->jsPrimitiveType(), m_state.accumulatorOut.storedType(),
                u"QJSPrimitiveValue()"_qs) + u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_LoadInt(int value)
{
    INJECT_TRACE_INFO(generate_LoadInt);

    Q_ASSERT(m_typeResolver->registerContains(m_state.accumulatorOut, m_typeResolver->intType()));
    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_qs;
    m_body += QString::number(value);
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_MoveConst(int constIndex, int destTemp)
{
    INJECT_TRACE_INFO(generate_MoveConst);

    auto var = registerVariable(destTemp);
    if (var.isEmpty())
        return; // Do not load 'undefined'

    m_body.setWriteRegister(var);
    const auto v4Value = QV4::StaticValue::fromReturnedValue(
                m_jsUnitGenerator->constant(constIndex));

    if (v4Value.isNull()) {
        const auto type = registerType(destTemp).storedType();
        m_body += var + u" = "_qs;
        if (type == m_typeResolver->jsPrimitiveType()) {
            m_body += u"QJSPrimitiveNull()"_qs;
        } else if (type == m_typeResolver->jsValueType()) {
            m_body += u"QJSValue(QJSValue::NullValue)"_qs;
        } else if (type == m_typeResolver->varType()) {
            m_body += u"QVariant::fromValue<std::nullptr_t>(nullptr)"_qs;
        } else if (type->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
            m_body += u"nullptr"_qs;
        } else {
            setError(u"Cannot load null into %1"_qs.arg(m_state.accumulatorOut.descriptiveName()));
        }

        m_body += u";\n"_qs;
        return;
    }

    double value = 0.0f;
    if (v4Value.isInteger() || v4Value.isBoolean())
        value = v4Value.int_32();
    else if (v4Value.isDouble())
        value = v4Value.doubleValue();

    m_body += var;
    m_body += u" = "_qs;
    m_body += toNumericString(value);
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_LoadReg(int reg)
{
    INJECT_TRACE_INFO(generate_LoadReg);

    // We can't emit any code yet for loading undefined...
    // See also generate_LoadUndefined()
    if (m_typeResolver->registerContains(m_state.accumulatorOut, m_typeResolver->voidType()))
        return;
    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_qs;
    m_body += use(registerVariable(reg));
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_StoreReg(int reg)
{
    INJECT_TRACE_INFO(generate_StoreReg);
    Q_ASSERT(m_state.accumulatorIn.isValid());

    if (isArgument(reg))
        reject(u"writing into a function argument"_qs);

    const QString var = registerVariable(reg);
    m_body.setWriteRegister(var);
    if (var.isEmpty())
        return; // don't store "undefined"
    m_body += var;
    m_body += u" = "_qs;
    m_body += conversion(m_state.accumulatorIn, registerType(reg),
                         use(m_state.accumulatorVariableIn));
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_MoveReg(int srcReg, int destReg)
{
    INJECT_TRACE_INFO(generate_MoveReg);

    const QString destRegName = registerVariable(destReg);
    m_body.setWriteRegister(destRegName);
    if (destRegName.isEmpty())
        return; // don't store things we cannot store.
    m_body += destRegName;
    m_body += u" = "_qs;
    m_body += use(registerVariable(srcReg));
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_LoadImport(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadLocal(int index)
{
    Q_UNUSED(index);
    reject(u"LoadLocal"_qs);
}

void QQmlJSCodeGenerator::generate_StoreLocal(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadScopedLocal(int scope, int index)
{
    Q_UNUSED(scope)
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_StoreScopedLocal(int scope, int index)
{
    Q_UNUSED(scope)
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadRuntimeString(int stringId)
{
    INJECT_TRACE_INFO(generate_LoadRuntimeString);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_qs;
    m_body += QQmlJSUtils::toLiteral(m_jsUnitGenerator->stringForIndex(stringId));
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_MoveRegExp(int regExpId, int destReg)
{
    Q_UNUSED(regExpId)
    Q_UNUSED(destReg)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadClosure(int value)
{
    Q_UNUSED(value)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadName(int nameIndex)
{
    Q_UNUSED(nameIndex)
    reject(u"LoadName"_qs);
}

void QQmlJSCodeGenerator::generate_LoadGlobalLookup(int index)
{
    INJECT_TRACE_INFO(generate_LoadGlobalLookup);

    const QString lookup = u"aotContext->loadGlobalLookup("_qs + QString::number(index)
            + u", &"_qs + m_state.accumulatorVariableOut + u", "_qs
            + metaTypeFromType(m_state.accumulatorOut.storedType()) + u')';
    const QString initialization = u"aotContext->initLoadGlobalLookup("_qs
            + QString::number(index) + u')';
    generateLookup(lookup, initialization);
}

void QQmlJSCodeGenerator::generate_LoadQmlContextPropertyLookup(int index)
{
    INJECT_TRACE_INFO(generate_LoadQmlContextPropertyLookup);

    if (m_state.accumulatorVariableOut.isEmpty())
        return;

    const int nameIndex = m_jsUnitGenerator->lookupNameIndex(index);
    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    if (m_state.accumulatorOut.variant() == QQmlJSRegisterContent::JavaScriptGlobal) {
        m_body += m_state.accumulatorVariableOut + u" = "_qs
                + conversion(
                    m_typeResolver->jsValueType(), m_state.accumulatorOut.storedType(),
                    u"aotContext->javaScriptGlobalProperty("_qs + QString::number(nameIndex) + u")")
                + u";\n"_qs;
        return;
    }

    const QString indexString = QString::number(index);
    if (m_state.accumulatorOut.variant() == QQmlJSRegisterContent::ObjectById) {
        const QString lookup = u"aotContext->loadContextIdLookup("_qs
                + indexString + u", "_qs
                + contentPointer(m_state.accumulatorOut, m_state.accumulatorVariableOut) + u')';
        const QString initialization = u"aotContext->initLoadContextIdLookup("_qs
                + indexString + u')';
        generateLookup(lookup, initialization);
        return;
    }

    const bool isProperty = m_state.accumulatorOut.isProperty();
    const QQmlJSScope::ConstPtr scope = m_state.accumulatorOut.scopeType();
    const QQmlJSScope::ConstPtr stored = m_state.accumulatorOut.storedType();
    if (isProperty) {
        m_body += u"{\n"_qs;

        const auto lookupType = contentType(m_state.accumulatorOut, m_state.accumulatorVariableOut);

        const QString lookup = u"aotContext->loadScopeObjectPropertyLookup("_qs
                + indexString + u", "_qs
                + contentPointer(m_state.accumulatorOut, m_state.accumulatorVariableOut) + u')';
        const QString initialization
                = u"aotContext->initLoadScopeObjectPropertyLookup("_qs
                + indexString + u", "_qs
                + lookupType + u')';
        const QString preparation = getLookupPreparation(
                    m_state.accumulatorOut, m_state.accumulatorVariableOut, index);

        generateLookup(lookup, initialization, preparation);
        m_body += u"}\n"_qs;
    } else if (m_state.accumulatorOut.isType() || m_state.accumulatorOut.isImportNamespace()) {
        generateTypeLookup(index);
    } else {
        reject(u"lookup of %1"_qs.arg(m_state.accumulatorOut.descriptiveName()));
    }
}

void QQmlJSCodeGenerator::generate_StoreNameSloppy(int nameIndex)
{
    INJECT_TRACE_INFO(generate_StoreNameSloppy);

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());

    const QString name = m_jsUnitGenerator->stringForIndex(nameIndex);
    const QQmlJSRegisterContent specific = m_typeResolver->scopedType(m_function->qmlScope, name);
    const QQmlJSRegisterContent type = specific.storedIn(
                m_typeResolver->genericType(specific.storedType()));
    Q_ASSERT(type.isProperty());

    switch (type.variant()) {
    case QQmlJSRegisterContent::ScopeProperty:
    case QQmlJSRegisterContent::ExtensionScopeProperty: {
        if (type.property().type() != m_typeResolver->containedType(m_state.accumulatorIn)) {
            m_body += u"{\n"_qs;
            m_body += u"auto converted = "_qs
                    + conversion(m_state.accumulatorIn, type, use(m_state.accumulatorVariableIn))
                    + u";\n"_qs;
            m_body += u"aotContext->storeNameSloppy("_qs + QString::number(nameIndex)
                    + u", "_qs + contentPointer(type, u"converted"_qs)
                    + u", "_qs + contentType(type, u"converted"_qs) + u')';
            m_body += u";\n"_qs;
            m_body += u"}\n"_qs;
        } else {
            m_body += u"aotContext->storeNameSloppy("_qs + QString::number(nameIndex)
                    + u", "_qs
                    + contentPointer(m_state.accumulatorIn, use(m_state.accumulatorVariableIn))
                    + u", "_qs
                    + contentType(m_state.accumulatorIn, use(m_state.accumulatorVariableIn)) + u')';
            m_body += u";\n"_qs;
        }
        break;
    }
    case QQmlJSRegisterContent::ScopeMethod:
    case QQmlJSRegisterContent::ExtensionScopeMethod:
        reject(u"assignment to scope method"_qs);
        break;
    default:
        Q_UNREACHABLE();
    }
}

void QQmlJSCodeGenerator::generate_StoreNameStrict(int name)
{
    Q_UNUSED(name)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadElement(int base)
{
    INJECT_TRACE_INFO(generate_LoadElement);

    const QQmlJSRegisterContent baseType = registerType(base);

    if (!m_typeResolver->isNumeric(m_state.accumulatorIn) || !baseType.isList()) {
        reject(u"LoadElement with non-list base type or non-numeric arguments"_qs);
        return;
    }

    if (baseType.storedType() != m_typeResolver->listPropertyType()) {
        reject(u"indirect LoadElement"_qs);
        return;
    }

    const QString baseName = use(registerVariable(base));
    const QString indexName = use(m_state.accumulatorVariableIn);

    const QString voidAssignment = u"    "_qs + m_state.accumulatorVariableOut + u" = "_qs +
            conversion(m_typeResolver->globalType(m_typeResolver->voidType()),
                       m_state.accumulatorOut, QString()) + u";\n"_qs;

    if (!m_typeResolver->isIntegral(m_state.accumulatorIn)) {
        m_body += u"if (!QJSNumberCoercion::isInteger("_qs + indexName + u"))\n"_qs
                + voidAssignment
                + u"else "_qs;
    }
    // Our QQmlListProperty only keeps plain QObject*.
    const auto valueType = m_typeResolver->valueType(baseType);
    const auto elementType = m_typeResolver->globalType(
                m_typeResolver->genericType(m_typeResolver->containedType(valueType)));

    m_body += u"if ("_qs + indexName + u" >= 0 && "_qs + indexName
            + u" < "_qs + baseName + u".count(&"_qs + baseName
            + u"))\n"_qs;
    m_body += u"    "_qs + m_state.accumulatorVariableOut + u" = "_qs +
            conversion(elementType, m_state.accumulatorOut,
                       baseName + u".at(&"_qs + baseName + u", "_qs
                       + indexName + u')') + u";\n"_qs;
    m_body += u"else\n"_qs
            + voidAssignment;
}

void QQmlJSCodeGenerator::generate_StoreElement(int base, int index)
{
    INJECT_TRACE_INFO(generate_StoreElement);

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());

    const QQmlJSRegisterContent baseType = registerType(base);
    const QQmlJSRegisterContent indexType = registerType(index);

    if (!m_typeResolver->isNumeric(registerType(index)) || !baseType.isList()) {
        reject(u"StoreElement with non-list base type or non-numeric arguments"_qs);
        return;
    }

    if (baseType.storedType() != m_typeResolver->listPropertyType()) {
        reject(u"indirect StoreElement"_qs);
        return;
    }

    const QString baseName = use(registerVariable(base));
    const QString indexName = use(registerVariable(index));

    const auto valueType = m_typeResolver->valueType(baseType);
    const auto elementType = m_typeResolver->globalType(m_typeResolver->genericType(
                                          m_typeResolver->containedType(valueType)));

    m_body += u"if ("_qs;
    if (!m_typeResolver->isIntegral(indexType))
        m_body += u"QJSNumberCoercion::isInteger("_qs + indexName + u") && "_qs;
    m_body += indexName + u" >= 0 && "_qs
            + indexName + u" < "_qs + baseName + u".count(&"_qs + baseName
            + u"))\n"_qs;
    m_body += u"    "_qs + baseName + u".replace(&"_qs + baseName
            + u", "_qs + indexName + u", "_qs;
    m_body += conversion(m_state.accumulatorIn, elementType, use(m_state.accumulatorVariableIn))
            + u");\n"_qs;
}

void QQmlJSCodeGenerator::generate_LoadProperty(int nameIndex)
{
    Q_UNUSED(nameIndex)
    reject(u"LoadProperty"_qs);
}

void QQmlJSCodeGenerator::generate_LoadOptionalProperty(int name, int offset)
{
    Q_UNUSED(name)
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generateEnumLookup(int index)
{
    const QString enumMember = m_state.accumulatorOut.enumMember();

    // If we're referring to the type, there's nothing to do.
    if (enumMember.isEmpty())
        return;

    // If the metaenum has the value, just use it and skip all the rest.
    const QQmlJSMetaEnum metaEnum = m_state.accumulatorOut.enumeration();
    if (metaEnum.hasValues()) {
        m_body += m_state.accumulatorVariableOut + u" = "_qs
                + QString::number(metaEnum.value(enumMember));
        m_body += u";\n"_qs;
        return;
    }

    const QQmlJSScope::ConstPtr scopeType = m_state.accumulatorOut.scopeType();

    // Otherwise we would have found an enum with values.
    Q_ASSERT(!scopeType->isComposite());

    const QString enumName = metaEnum.isFlag() ? metaEnum.alias() : metaEnum.name();
    const QString lookup = u"aotContext->getEnumLookup("_qs + QString::number(index)
            + u", &"_qs + m_state.accumulatorVariableOut + u')';
    const QString initialization = u"aotContext->initGetEnumLookup("_qs
            + QString::number(index) + u", "_qs + metaObject(scopeType)
            + u", \""_qs + enumName + u"\", \""_qs + enumMember
            + u"\")"_qs;
    generateLookup(lookup, initialization);
}

void QQmlJSCodeGenerator::generateTypeLookup(int index)
{
    const QString indexString = QString::number(index);
    const QString namespaceString
            = m_state.accumulatorIn.isImportNamespace()
                ? QString::number(m_state.accumulatorIn.importNamespace())
                : u"QQmlPrivate::AOTCompiledContext::InvalidStringId"_qs;

    switch (m_state.accumulatorOut.variant()) {
    case QQmlJSRegisterContent::Singleton: {
        rejectIfNonQObjectOut(u"non-QObject singleton type"_qs);
        const QString lookup = u"aotContext->loadSingletonLookup("_qs + indexString
                + u", &"_qs + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadSingletonLookup("_qs + indexString
                + u", "_qs + namespaceString + u')';
        generateLookup(lookup, initialization);
        break;
    }
    case QQmlJSRegisterContent::ScopeModulePrefix:
        m_body += m_state.accumulatorVariableOut + u" = aotContext->qmlScopeObject;\n"_qs;
        break;
    case QQmlJSRegisterContent::ScopeAttached: {
        rejectIfNonQObjectOut(u"non-QObject attached type"_qs);
        const QString lookup = u"aotContext->loadAttachedLookup("_qs + indexString
                + u", aotContext->qmlScopeObject, &"_qs + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadAttachedLookup("_qs + indexString
                + u", "_qs + namespaceString + u", aotContext->qmlScopeObject)"_qs;
        generateLookup(lookup, initialization);
        break;
    }
    case QQmlJSRegisterContent::Script:
        reject(u"script lookup"_qs);
        break;
    case QQmlJSRegisterContent::MetaType: {
        if (m_state.accumulatorOut.storedType() != m_typeResolver->metaObjectType()) {
            // TODO: Can we trigger this somehow?
            //       It might be impossible, but we better be safe here.
            reject(u"meta-object stored in different type"_qs);
        }
        const QString lookup = u"aotContext->loadTypeLookup("_qs + indexString
                + u", &"_qs + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadTypeLookup("_qs + indexString
                + u", "_qs + namespaceString + u")"_qs;
        generateLookup(lookup, initialization);
        break;
    }
    default:
        Q_UNREACHABLE();
    }
}

void QQmlJSCodeGenerator::rejectIfNonQObjectOut(const QString &error)
{
    if (m_state.accumulatorOut.storedType()->accessSemantics()
        != QQmlJSScope::AccessSemantics::Reference) {
        reject(error);
    }
}

void QQmlJSCodeGenerator::generate_GetLookup(int index)
{
    INJECT_TRACE_INFO(generate_GetLookup);

    if (m_state.accumulatorOut.isMethod()) {
        reject(u"lookup of function property."_qs);
        return;
    }

    if (m_state.accumulatorOut.isEnumeration()) {
        generateEnumLookup(index);
        return;
    }

    if (m_state.accumulatorOut.isImportNamespace()) {
        m_body.setWriteRegister(QString());
        return; // Nothing to do. We've resolved the prefix already.
    }

    const QString indexString = QString::number(index);
    const QString namespaceString = m_state.accumulatorIn.isImportNamespace()
            ? QString::number(m_state.accumulatorIn.importNamespace())
            : u"QQmlPrivate::AOTCompiledContext::InvalidStringId"_qs;
    const auto storedType = m_state.accumulatorIn.storedType();
    const bool isReferenceType
            = (storedType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference);
    switch (m_state.accumulatorOut.variant()) {
    case QQmlJSRegisterContent::ObjectAttached: {
        if (!isReferenceType) {
            // This can happen on incomplete type information. We contextually know that the
            // type must be a QObject, but we cannot construct the inheritance chain. Then we
            // store it in a generic type. Technically we could even convert it to QObject*, but
            // that would be expensive.
            reject(u"attached object for non-QObject type"_qs);
        }
        rejectIfNonQObjectOut(u"non-QObject attached type"_qs);

        const QString lookup = u"aotContext->loadAttachedLookup("_qs + indexString
                + u", "_qs + use(m_state.accumulatorVariableIn)
                + u", &"_qs + m_state.accumulatorVariableOut + u')';
        const QString initialization = u"aotContext->initLoadAttachedLookup("_qs
                + indexString + u", "_qs + namespaceString + u", "_qs
                + use(m_state.accumulatorVariableIn) + u')';
        generateLookup(lookup, initialization);
        return;
    }
    case QQmlJSRegisterContent::ScopeAttached:
    case QQmlJSRegisterContent::Singleton:
    case QQmlJSRegisterContent::MetaType:
        generateTypeLookup(index);
        return;
    default:
        break;
    }

    Q_ASSERT(m_state.accumulatorOut.isProperty());

    const QQmlJSScope::ConstPtr out = m_state.accumulatorOut.storedType();
    if (isReferenceType) {
        m_body += u"{\n"_qs;
        protectAccumulator();
        const QString lookup = u"aotContext->getObjectLookup("_qs + indexString
                + u", "_qs + use(m_state.accumulatorVariableIn) + u", "_qs
                + contentPointer(m_state.accumulatorOut, m_state.accumulatorVariableOut) + u')';
        const QString initialization = u"aotContext->initGetObjectLookup("_qs
                + indexString + u", "_qs + use(m_state.accumulatorVariableIn)
                + u", "_qs + contentType(m_state.accumulatorOut, m_state.accumulatorVariableOut)
                + u')';
        const QString preparation = getLookupPreparation(
                    m_state.accumulatorOut, m_state.accumulatorVariableOut, index);
        generateLookup(lookup, initialization, preparation);
        m_body += u"}\n"_qs;
    } else if ((storedType == m_typeResolver->stringType()
                || storedType->accessSemantics() == QQmlJSScope::AccessSemantics::Sequence)
               && m_jsUnitGenerator->lookupName(index) == u"length"_qs) {
        // Special-cased the same way as in QQmlJSTypeResolver::memberType()
        m_body += m_state.accumulatorVariableOut + u" = "_qs + use(m_state.accumulatorVariableIn)
                + u".count("_qs;
        if (storedType == m_typeResolver->listPropertyType())
            m_body += u'&' + use(m_state.accumulatorVariableIn);
        m_body += u')' + u";\n"_qs;
    } else if (storedType == m_typeResolver->jsValueType()) {
        reject(u"lookup in QJSValue"_qs);
    } else {
        m_body += u"{\n"_qs;
        protectAccumulator();
        const QString lookup = u"aotContext->getValueLookup("_qs + indexString
                + u", "_qs + contentPointer(m_state.accumulatorIn,
                                            use(m_state.accumulatorVariableIn))
                + u", "_qs + contentPointer(m_state.accumulatorOut, m_state.accumulatorVariableOut)
                + u')';
        const QString initialization = u"aotContext->initGetValueLookup("_qs
                + indexString + u", "_qs
                + metaObject(m_state.accumulatorOut.scopeType()) + u", "_qs
                + contentType(m_state.accumulatorOut, m_state.accumulatorVariableOut) + u')';
        const QString preparation = getLookupPreparation(
                    m_state.accumulatorOut, m_state.accumulatorVariableOut, index);
        generateLookup(lookup, initialization, preparation);
        m_body += u"}\n"_qs;
    }
}

void QQmlJSCodeGenerator::generate_GetOptionalLookup(int index, int offset)
{
    Q_UNUSED(index)
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_StoreProperty(int nameIndex, int baseReg)
{
    Q_UNUSED(nameIndex)
    Q_UNUSED(baseReg)
    reject(u"StoreProperty"_qs);
}

QString QQmlJSCodeGenerator::setLookupPreparation(
        const QQmlJSRegisterContent &content, const QString &arg, int lookup)
{
    const QQmlJSScope::ConstPtr stored = content.storedType();
    if (m_typeResolver->containedType(content) == stored) {
        return QString();
    } else if (stored == m_typeResolver->varType()) {
        return u"const QMetaType argType = aotContext->lookupResultMetaType("_qs
                + QString::number(lookup) + u");\n"_qs
                + u"if (argType.isValid())\n    "_qs + arg + u".convert(argType)";
    }
    // TODO: We could make sure they're compatible, for example QObject pointers.
    return QString();
}


void QQmlJSCodeGenerator::generate_SetLookup(int index, int baseReg)
{
    INJECT_TRACE_INFO(generate_SetLookup);

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());

    const QString indexString = QString::number(index);
    const QQmlJSScope::ConstPtr valueType = m_state.accumulatorIn.storedType();
    const QQmlJSRegisterContent callBase = registerType(baseReg);
    const QQmlJSScope::ConstPtr objectType = callBase.storedType();
    const QQmlJSRegisterContent specific = m_typeResolver->memberType(
                callBase, m_jsUnitGenerator->lookupName(index));
    const QQmlJSRegisterContent property = specific.storedIn(
                m_typeResolver->genericType(specific.storedType()));

    const QString object = use(registerVariable(baseReg));
    m_body += u"{\n"_qs;
    QString variableIn;
    QString variableInType;
    QString preparation;
    QString argType;
    if (property != m_state.accumulatorIn) {
        m_body += u"auto converted = "_qs
                + conversion(m_state.accumulatorIn, property, use(m_state.accumulatorVariableIn))
                + u";\n"_qs;
        variableIn = contentPointer(property, u"converted"_qs);
        variableInType = contentType(property, u"converted"_qs);
        preparation = setLookupPreparation(property, u"converted"_qs, index);
        if (preparation.isEmpty())
            argType = contentType(property, u"converted"_qs);
        else
            argType = u"argType"_qs;
    } else {
        variableIn = contentPointer(property, use(m_state.accumulatorVariableIn));
        variableInType = contentType(property, use(m_state.accumulatorVariableIn));
        argType = variableInType;
    }

    switch (objectType->accessSemantics()) {
    case QQmlJSScope::AccessSemantics::Reference: {
        const QString lookup = u"aotContext->setObjectLookup("_qs + indexString
                + u", "_qs + object + u", "_qs + variableIn + u')';
        const QString initialization = u"aotContext->initSetObjectLookup("_qs
                + indexString + u", "_qs + object + u", "_qs + argType + u')';
        generateLookup(lookup, initialization, preparation);
        break;
    }
    case QQmlJSScope::AccessSemantics::Sequence: {
        const QString propertyName = m_jsUnitGenerator->lookupName(index);
        if (propertyName != u"length"_qs) {
            reject(u"setting non-length property on a sequence type"_qs);
            break;
        }

        if (objectType != m_typeResolver->listPropertyType()) {
            reject(u"SetLookup on sequence types (because of missing write-back)"_qs);
            break;
        }

        // We can resize without write back on a list property because it's actually a reference.
        m_body += u"const int begin = "_qs + object + u".count(&" + object + u");\n"_qs;
        m_body += u"const int end = "_qs
                + (variableIn.startsWith(u'&') ? variableIn.mid(1) : (u'*' + variableIn))
                + u";\n"_qs;
        m_body += u"for (int i = begin; i < end; ++i)\n"_qs;
        m_body += u"    "_qs + object + u".append(&"_qs + object + u", nullptr);\n"_qs;
        m_body += u"for (int i = begin; i > end; --i)\n"_qs;
        m_body += u"    "_qs + object + u".removeLast(&"_qs + object + u')'
                + u";\n"_qs;
        break;
    }
    case QQmlJSScope::AccessSemantics::Value: {
        const QString propertyName = m_jsUnitGenerator->lookupName(index);
        const QQmlJSRegisterContent specific = m_typeResolver->memberType(callBase, propertyName);
        Q_ASSERT(specific.isProperty());
        const QQmlJSRegisterContent property = specific.storedIn(
                    m_typeResolver->genericType(specific.storedType()));

        const QString lookup = u"aotContext->setValueLookup("_qs + indexString
                + u", "_qs + contentPointer(registerType(baseReg), object)
                + u", "_qs + variableIn + u')';
        const QString initialization = u"aotContext->initSetValueLookup("_qs
                + indexString + u", "_qs + metaObject(property.scopeType())
                + u", "_qs + contentType(registerType(baseReg), object) + u')';

        generateLookup(lookup, initialization, preparation);
        reject(u"SetLookup on value types (because of missing write-back)"_qs);
        break;
    }
    case QQmlJSScope::AccessSemantics::None:
        Q_UNREACHABLE();
        break;
    }

    m_body += u"}\n"_qs;
}

void QQmlJSCodeGenerator::generate_LoadSuperProperty(int property)
{
    Q_UNUSED(property)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_StoreSuperProperty(int property)
{
    Q_UNUSED(property)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Yield()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_YieldStar()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Resume(int)
{
    BYTECODE_UNIMPLEMENTED();
}

QString QQmlJSCodeGenerator::argumentsList(int argc, int argv, QString *outVar)
{
    QString types;
    QString args;
    QString conversions;

    if (m_typeResolver->containedType(m_state.accumulatorOut) == m_typeResolver->voidType()) {
        types = u"QMetaType()"_qs;
        args = u"nullptr"_qs;
    } else {
        *outVar = u"callResult"_qs;
        const QQmlJSScope::ConstPtr outType = m_state.accumulatorOut.storedType();
        m_body += outType->internalName();
        if (outType->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            m_body += u" *"_qs;
        else
            m_body += u' ';
        m_body += *outVar + u";\n";

        types = metaTypeFromType(m_state.accumulatorOut.storedType());
        args = u'&' + *outVar;
    }

    for (int i = 0; i < argc; ++i) {
        const QQmlJSScope::ConstPtr type = registerType(argv + i).storedType();
        const QString var = use(registerVariable(argv + i));
        if (type == m_typeResolver->jsPrimitiveType()) {
            QString argName = u"arg"_qs + QString::number(i);
            conversions += u"QVariant "_qs + argName + u" = "_qs
                    + conversion(type, m_typeResolver->varType(), var) + u";\n"_qs;
            args += u", "_qs + argName + u".data()"_qs;
            types += u", "_qs + argName + u".metaType()"_qs;
        } else if (type == m_typeResolver->varType()) {
            args += u", "_qs + var + u".data()"_qs;
            types += u", "_qs + var + u".metaType()"_qs;
        } else {
            args += u", &"_qs + var;
            types += u", "_qs + metaTypeFromType(type);
        }
    }
    return conversions
            + u"void *args[] = { "_qs + args + u" };\n"_qs
            + u"const QMetaType types[] = { "_qs + types + u" };\n"_qs;
}

void QQmlJSCodeGenerator::generateMoveOutVar(const QString &outVar)
{
    // Generate a new section to set m_state.accumulatorVariableOut,
    // so that m_state.accumulatorVariableOut can be optimized away.

    nextSection();
    m_body.setWriteRegister(m_state.accumulatorVariableOut);
    m_body += m_state.accumulatorVariableOut + u" = "_qs;

    if (outVar.isEmpty())
        m_body += u"{};\n"_qs;
    else
        m_body += u"std::move(" + outVar + u");\n";

    nextSection();
    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());
}

void QQmlJSCodeGenerator::generate_CallValue(int name, int argc, int argv)
{
    Q_UNUSED(name)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CallWithReceiver(int name, int thisObject, int argc, int argv)
{
    Q_UNUSED(name)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CallProperty(int nameIndex, int baseReg, int argc, int argv)
{
    Q_UNUSED(nameIndex);
    Q_UNUSED(baseReg);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    reject(u"CallProperty"_qs);
}

bool QQmlJSCodeGenerator::inlineMathMethod(const QString &name, int argc, int argv)
{
    addInclude(u"cmath"_qs);
    addInclude(u"limits"_qs);
    addInclude(u"qalgorithms.h"_qs);
    addInclude(u"qrandom.h"_qs);

    if (m_state.accumulatorOut.storedType() != m_typeResolver->realType())
        return false;

    m_body += u"{\n"_qs;
    for (int i = 0; i < argc; ++i) {
        m_body += u"const double arg%1 = "_qs.arg(i + 1) + conversion(
                        registerType(argv + i).storedType(),
                        m_typeResolver->realType(), use(registerVariable(argv + i)))
                + u";\n"_qs;
    }

    const QString qNaN = u"std::numeric_limits<double>::quiet_NaN()"_qs;
    const QString inf = u"std::numeric_limits<double>::infinity()"_qs;
    m_body += m_state.accumulatorVariableOut + u" = "_qs;

    if (name == u"abs" && argc == 1) {
        m_body += u"(qIsNull(arg1) ? 0 : (arg1 < 0.0 ? -arg1 : arg1))"_qs;
    } else if (name == u"acos"_qs && argc == 1) {
        m_body += u"arg1 > 1.0 ? %1 : std::acos(arg1)"_qs.arg(qNaN);
    } else if (name == u"acosh"_qs && argc == 1) {
        m_body += u"arg1 < 1.0 ? %1 : std::acosh(arg1)"_qs.arg(qNaN);
    } else if (name == u"asin"_qs && argc == 1) {
        m_body += u"arg1 > 1.0 ? %1 : std::asin(arg1)"_qs.arg(qNaN);
    } else if (name == u"asinh"_qs && argc == 1) {
        m_body += u"qIsNull(arg1) ? arg1 : std::asinh(arg1)"_qs;
    } else if (name == u"atan"_qs && argc == 1) {
        m_body += u"qIsNull(arg1) ? arg1 : std::atan(arg1)"_qs;
    } else if (name == u"atanh"_qs && argc == 1) {
        m_body += u"qIsNull(arg1) ? arg1 : std::atanh(arg1)"_qs;
    } else if (name == u"atan2"_qs) {
        // TODO: complicated
        return false;
    } else if (name == u"cbrt"_qs && argc == 1) {
        m_body += u"std::cbrt(arg1)"_qs;
    } else if (name == u"ceil"_qs && argc == 1) {
        m_body += u"(arg1 < 0.0 && arg1 > -1.0) ? std::copysign(0.0, -1.0) : std::ceil(arg1)"_qs;
    } else if (name == u"clz32"_qs && argc == 1) {
        m_body += u"qint32(qCountLeadingZeroBits(quint32(QJSNumberCoercion::toInteger(arg1))))"_qs;
    } else if (name == u"cos"_qs && argc == 1) {
        m_body += u"std::cos(arg1)"_qs;
    } else if (name == u"cosh"_qs && argc == 1) {
        m_body += u"std::cosh(arg1)"_qs;
    } else if (name == u"exp"_qs && argc == 1) {
        m_body += u"std::isinf(arg1) "
                "? (std::copysign(1.0, arg1) == -1 ? 0.0 : %1) "
                ": std::exp(arg1)"_qs.arg(inf);
    } else if (name == u"expm1"_qs) {
        // TODO: complicated
        return false;
    } else if (name == u"floor"_qs && argc == 1) {
        m_body += u"std::floor(arg1)"_qs;
    } else if (name == u"fround"_qs && argc == 1) {
        m_body += u"(std::isnan(arg1) || std::isinf(arg1) || qIsNull(arg1)) "
                "? arg1 "
                ": double(float(arg1))"_qs;
    } else if (name == u"hypot"_qs) {
        // TODO: complicated
        return false;
    } else if (name == u"imul"_qs && argc == 2) {
        m_body += u"qint32(quint32(QJSNumberCoercion::toInteger(arg1)) "
                "* quint32(QJSNumberCoercion::toInteger(arg2)))"_qs;
    } else if (name == u"log"_qs && argc == 1) {
        m_body += u"arg1 < 0.0 ? %1 : std::log(arg1)"_qs.arg(qNaN);
    } else if (name == u"log10"_qs && argc == 1) {
        m_body += u"arg1 < 0.0 ? %1 : std::log10(arg1)"_qs.arg(qNaN);
    } else if (name == u"log1p"_qs && argc == 1) {
        m_body += u"arg1 < -1.0 ? %1 : std::log1p(arg1)"_qs.arg(qNaN);
    } else if (name == u"log2"_qs && argc == 1) {
        m_body += u"arg1 < -0.0 ? %1 : std::log2(arg1)"_qs.arg(qNaN);
    } else if (name == u"max"_qs && argc == 2) {
        m_body += u"(qIsNull(arg2) && qIsNull(arg1) && std::copysign(1.0, arg2) == 1) "
                "? arg2 "
                ": ((arg2 > arg1 || std::isnan(arg2)) ? arg2 : arg1)"_qs;
    } else if (name == u"min"_qs && argc == 2) {
        m_body += u"(qIsNull(arg2) && qIsNull(arg1) && std::copysign(1.0, arg2) == -1) "
                "? arg2 "
                ": ((arg2 < arg1 || std::isnan(arg2)) ? arg2 : arg1)"_qs;
    } else if (name == u"pow"_qs) {
        // TODO: complicated
        return false;
    } else if (name == u"random"_qs && argc == 0) {
        m_body += u"QRandomGenerator::global()->generateDouble()"_qs;
    } else if (name == u"round"_qs && argc == 1) {
        m_body += u"std::isfinite(arg1) "
                "? ((arg1 < 0.5 && arg1 >= -0.5) "
                    "? std::copysign(0.0, arg1) "
                    ": std::floor(arg1 + 0.5)) "
                ": arg1"_qs;
    } else if (name == u"sign"_qs && argc == 1) {
        m_body += u"std::isnan(arg1) "
                "? %1 "
                ": (qIsNull(arg1) "
                    "? arg1 "
                    ": (std::signbit(arg1) ? -1.0 : 1.0))"_qs.arg(qNaN);
    } else if (name == u"sin"_qs && argc == 1) {
        m_body += u"qIsNull(arg1) ? arg1 : std::sin(arg1)"_qs;
    } else if (name == u"sinh"_qs && argc == 1) {
        m_body += u"qIsNull(arg1) ? arg1 : std::sinh(arg1)"_qs;
    } else if (name == u"sqrt"_qs && argc == 1) {
        m_body += u"std::sqrt(arg1)"_qs;
    } else if (name == u"tan"_qs && argc == 1) {
        m_body += u"qIsNull(arg1) ? arg1 : std::tan(arg1)"_qs;
    } else if (name == u"tanh"_qs && argc == 1) {
        m_body += u"qIsNull(arg1) ? arg1 : std::tanh(arg1)"_qs;
    } else if (name == u"trunc"_qs && argc == 1) {
        m_body += u"std::trunc(arg1)"_qs;
    } else {
        return false;
    }

    m_body += u";\n"_qs;
    m_body += u"}\n"_qs;
    return true;
}

void QQmlJSCodeGenerator::generate_CallPropertyLookup(int index, int base, int argc, int argv)
{
    INJECT_TRACE_INFO(generate_CallPropertyLookup);

    if (m_state.accumulatorOut.variant() == QQmlJSRegisterContent::JavaScriptReturnValue)
        reject(u"call to untyped JavaScript function"_qs);

    const QQmlJSRegisterContent baseType = registerType(base);
    if (baseType.storedType()->accessSemantics() != QQmlJSScope::AccessSemantics::Reference) {
        const QString name = m_jsUnitGenerator->stringForIndex(
                    m_jsUnitGenerator->lookupNameIndex(index));
        if (m_typeResolver->containedType(baseType) == mathObject()) {
            if (inlineMathMethod(name, argc, argv))
                return;
        }

        reject(u"call to property '%1' of %2"_qs.arg(name, baseType.descriptiveName()));
    }

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());
    const QString indexString = QString::number(index);

    m_body += u"{\n"_qs;

    QString outVar;
    m_body += argumentsList(argc, argv, &outVar);
    const QString lookup = u"aotContext->callObjectPropertyLookup("_qs + indexString
            + u", "_qs + use(registerVariable(base))
            + u", args, types, "_qs + QString::number(argc) + u')';
    const QString initialization = u"aotContext->initCallObjectPropertyLookup("_qs
            + indexString + u')';
    generateLookup(lookup, initialization);
    generateMoveOutVar(outVar);

    m_body += u"}\n"_qs;
}

void QQmlJSCodeGenerator::generate_CallElement(int base, int index, int argc, int argv)
{
    Q_UNUSED(base)
    Q_UNUSED(index)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CallName(int name, int argc, int argv)
{
    Q_UNUSED(name);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    reject(u"CallName"_qs);
}

void QQmlJSCodeGenerator::generate_CallPossiblyDirectEval(int argc, int argv)
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CallGlobalLookup(int index, int argc, int argv)
{
    Q_UNUSED(index);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    reject(u"CallGlobalLookup"_qs);
}

void QQmlJSCodeGenerator::generate_CallQmlContextPropertyLookup(int index, int argc, int argv)
{
    INJECT_TRACE_INFO(generate_CallQmlContextPropertyLookup);

    if (m_state.accumulatorOut.variant() == QQmlJSRegisterContent::JavaScriptReturnValue)
        reject(u"call to untyped JavaScript function"_qs);

    m_body.setHasSideEffects(true);
    const QString indexString = QString::number(index);

    m_body += u"{\n"_qs;
    QString outVar;
    m_body += argumentsList(argc, argv, &outVar);
    const QString lookup = u"aotContext->callQmlContextPropertyLookup("_qs + indexString
            + u", args, types, "_qs + QString::number(argc) + u')';
    const QString initialization = u"aotContext->initCallQmlContextPropertyLookup("_qs
            + indexString + u')';
    generateLookup(lookup, initialization);
    generateMoveOutVar(outVar);

    m_body += u"}\n"_qs;
}

void QQmlJSCodeGenerator::generate_CallWithSpread(int func, int thisObject, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_TailCall(int func, int thisObject, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(thisObject)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Construct(int func, int argc, int argv)
{
    Q_UNUSED(func);
    Q_UNUSED(argc);
    Q_UNUSED(argv);
    reject(u"Construct"_qs);
}

void QQmlJSCodeGenerator::generate_ConstructWithSpread(int func, int argc, int argv)
{
    Q_UNUSED(func)
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_SetUnwindHandler(int offset)
{
    Q_UNUSED(offset)
    reject(u"SetUnwindHandlerh"_qs);
}

void QQmlJSCodeGenerator::generate_UnwindDispatch()
{
    reject(u"UnwindDispatch"_qs);
}

void QQmlJSCodeGenerator::generate_UnwindToLabel(int level, int offset)
{
    Q_UNUSED(level)
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DeadTemporalZoneCheck(int name)
{
    Q_UNUSED(name)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_ThrowException()
{
    INJECT_TRACE_INFO(generate_ThrowException);

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());

    generateSetInstructionPointer();
    m_body += u"aotContext->engine->throwError("_qs
        + conversion(m_state.accumulatorIn, m_typeResolver->globalType(
                         m_typeResolver->jsValueType()),
                     use(m_state.accumulatorVariableIn)) + u");\n"_qs;
    m_body += u"return "_qs + errorReturnValue() + u";\n"_qs;
    m_skipUntilNextLabel = true;
}

void QQmlJSCodeGenerator::generate_GetException()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_SetException()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateCallContext()
{
    INJECT_TRACE_INFO(generate_CreateCallContext);

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());
    m_body += u"{\n"_qs;
}

void QQmlJSCodeGenerator::generate_PushCatchContext(int index, int nameIndex)
{
    Q_UNUSED(index)
    Q_UNUSED(nameIndex)
    reject(u"PushCatchContext"_qs);
}

void QQmlJSCodeGenerator::generate_PushWithContext()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PushBlockContext(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CloneBlockContext()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PushScriptContext(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PopScriptContext()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_PopContext()
{
    INJECT_TRACE_INFO(generate_PopContext);

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());
    // Add a semicolon before the closing brace, in case there was a bare label before it.
    m_body += u";}\n"_qs;
}

void QQmlJSCodeGenerator::generate_GetIterator(int iterator)
{
    Q_UNUSED(iterator)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_IteratorNext(int value, int done)
{
    Q_UNUSED(value)
    Q_UNUSED(done)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_IteratorNextForYieldStar(int iterator, int object)
{
    Q_UNUSED(iterator)
    Q_UNUSED(object)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_IteratorClose(int done)
{
    Q_UNUSED(done)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DestructureRestElement()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DeleteProperty(int base, int index)
{
    Q_UNUSED(base)
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DeleteName(int name)
{
    Q_UNUSED(name)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_TypeofName(int name)
{
    Q_UNUSED(name);
    reject(u"TypeofName"_qs);
}

void QQmlJSCodeGenerator::generate_TypeofValue()
{
    reject(u"TypeofValue"_qs);
}

void QQmlJSCodeGenerator::generate_DeclareVar(int varName, int isDeletable)
{
    Q_UNUSED(varName)
    Q_UNUSED(isDeletable)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_DefineArray(int argc, int args)
{
    Q_UNUSED(argc);
    Q_UNUSED(args);
    reject(u"DefineArray"_qs);
}

void QQmlJSCodeGenerator::generate_DefineObjectLiteral(int internalClassId, int argc, int args)
{
    Q_UNUSED(internalClassId)
    Q_UNUSED(argc)
    Q_UNUSED(args)
    reject(u"DefineObjectLiteral"_qs);
}

void QQmlJSCodeGenerator::generate_CreateClass(int classIndex, int heritage, int computedNames)
{
    Q_UNUSED(classIndex)
    Q_UNUSED(heritage)
    Q_UNUSED(computedNames)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateMappedArgumentsObject()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateUnmappedArgumentsObject()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CreateRestParameter(int argIndex)
{
    Q_UNUSED(argIndex)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_ConvertThisToObject()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_LoadSuperConstructor()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_ToObject()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Jump(int offset)
{
    INJECT_TRACE_INFO(generate_Jump);

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());
    generateJumpCodeWithTypeConversions(offset, JumpMode::Unconditional);
    m_body += u";\n"_qs;
    m_skipUntilNextLabel = true;
}

void QQmlJSCodeGenerator::generate_JumpTrue(int offset)
{
    INJECT_TRACE_INFO(generate_JumpTrue);

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());

    m_body += u"if ("_qs;
    m_body += conversion(m_state.accumulatorIn.storedType(), m_typeResolver->boolType(),
                       use(m_state.accumulatorVariableIn));
    m_body += u") "_qs;
    generateJumpCodeWithTypeConversions(offset, JumpMode::Conditional);
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_JumpFalse(int offset)
{
    INJECT_TRACE_INFO(generate_JumpFalse);

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());

    m_body += u"if (!"_qs;
    m_body += conversion(m_state.accumulatorIn.storedType(), m_typeResolver->boolType(),
                       use(m_state.accumulatorVariableIn));
    m_body += u") "_qs;
    generateJumpCodeWithTypeConversions(offset, JumpMode::Conditional);
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_JumpNoException(int offset)
{
    INJECT_TRACE_INFO(generate_JumpNoException);

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());

    m_body += u"if (!context->engine->hasException()) "_qs;
    generateJumpCodeWithTypeConversions(offset, JumpMode::Conditional);
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_JumpNotUndefined(int offset)
{
    Q_UNUSED(offset)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_CheckException()
{
    INJECT_TRACE_INFO(generate_CheckException);

    m_body.setHasSideEffects(true);
    m_body.setWriteRegister(QString());

    generateExceptionCheck();
}

void QQmlJSCodeGenerator::generate_CmpEqNull()
{
    INJECT_TRACE_INFO(generate_CmlEqNull);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = QJSPrimitiveValue(QJSPrimitiveNull()).equals("_qs;
    m_body += conversion(m_state.accumulatorIn.storedType(), m_typeResolver->jsPrimitiveType(),
                       use(m_state.accumulatorVariableIn));
    m_body += u')';
    m_body += u";\n"_qs;

}

void QQmlJSCodeGenerator::generate_CmpNeNull()
{
    INJECT_TRACE_INFO(generate_CmlNeNull);

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = !QJSPrimitiveValue(QJSPrimitiveNull()).equals("_qs;
    m_body += conversion(m_state.accumulatorIn.storedType(), m_typeResolver->jsPrimitiveType(),
                       use(m_state.accumulatorVariableIn));
    m_body += u')';
    m_body += u";\n"_qs;
}

QString QQmlJSCodeGenerator::eqIntExpression(int lhsConst)
{
    if (m_state.accumulatorIn.storedType() == m_typeResolver->intType())
        return QString::number(lhsConst) + u" == "_qs + use(m_state.accumulatorVariableIn);

    if (m_state.accumulatorIn.storedType() == m_typeResolver->boolType()) {
        return QString::number(lhsConst) + u" == "_qs
                + conversion(m_state.accumulatorIn.storedType(), m_typeResolver->intType(),
                             use(m_state.accumulatorVariableIn));
    }

    if (m_typeResolver->isNumeric(m_state.accumulatorIn)) {
        return conversion(m_typeResolver->intType(), m_typeResolver->realType(),
                           QString::number(lhsConst)) + u" == "_qs
                + conversion(m_state.accumulatorIn.storedType(), m_typeResolver->realType(),
                             use(m_state.accumulatorVariableIn));
    }

    QString result;
    result += conversion(m_typeResolver->intType(), m_typeResolver->jsPrimitiveType(),
                       QString::number(lhsConst));
    result += u".equals("_qs;
    result += conversion(m_state.accumulatorIn.storedType(), m_typeResolver->jsPrimitiveType(),
                       use(m_state.accumulatorVariableIn));
    result += u')';
    return result;
}

QString QQmlJSCodeGenerator::getLookupPreparation(
        const QQmlJSRegisterContent &content, const QString &var, int lookup)
{
    const QQmlJSScope::ConstPtr stored = content.storedType();
    if (m_typeResolver->containedType(content) == stored) {
        return QString();
    } else if (stored == m_typeResolver->varType()) {
        return var + u" = QVariant(aotContext->lookupResultMetaType("_qs
                + QString::number(lookup) + u"))"_qs;
    }
    // TODO: We could make sure they're compatible, for example QObject pointers.
    return QString();
}

QString QQmlJSCodeGenerator::contentPointer(const QQmlJSRegisterContent &content, const QString &var)
{
    const QQmlJSScope::ConstPtr stored = content.storedType();
    if (m_typeResolver->containedType(content) == stored)
        return u'&' + var;
    else if (stored == m_typeResolver->varType())
        return var + u".data()"_qs;
    else if (stored->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
        return u'&' + var;
    else
        reject(u"content pointer of non-QVariant wrapper type "_qs + content.descriptiveName());
    return QString();
}

QString QQmlJSCodeGenerator::contentType(const QQmlJSRegisterContent &content, const QString &var)
{
    const QQmlJSScope::ConstPtr stored = content.storedType();
    const QQmlJSScope::ConstPtr contained = QQmlJSScope::nonCompositeBaseType(
                m_typeResolver->containedType(content));
    if (contained == stored)
        return metaTypeFromType(stored);
    else if (stored == m_typeResolver->varType())
        return var + u".metaType()"_qs; // We expect the QVariant to be initialized
    else if (stored->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
        return metaTypeFromName(contained);
    else
        reject(u"content type of non-QVariant wrapper type "_qs + content.descriptiveName());
    return QString();
}

void QQmlJSCodeGenerator::generate_CmpEqInt(int lhsConst)
{
    INJECT_TRACE_INFO(generate_CmpEqInt);

    m_body += m_state.accumulatorVariableOut + u" = "_qs + eqIntExpression(lhsConst)
            + u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_CmpNeInt(int lhsConst)
{
    INJECT_TRACE_INFO(generate_CmpNeInt);

    m_body += m_state.accumulatorVariableOut + u" = !("_qs + eqIntExpression(lhsConst)
            + u");\n"_qs;
}

void QQmlJSCodeGenerator::generate_CmpEq(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpEq);
    generateEqualityOperation(lhs, u"equals"_qs, false);
}

void QQmlJSCodeGenerator::generate_CmpNe(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpNe);
    generateEqualityOperation(lhs, u"equals"_qs, true);
}

void QQmlJSCodeGenerator::generate_CmpGt(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpGt);
    generateCompareOperation(lhs, u">"_qs);
}

void QQmlJSCodeGenerator::generate_CmpGe(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpGe);
    generateCompareOperation(lhs, u">="_qs);
}

void QQmlJSCodeGenerator::generate_CmpLt(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpLt);
    generateCompareOperation(lhs, u"<"_qs);
}

void QQmlJSCodeGenerator::generate_CmpLe(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpLe);
    generateCompareOperation(lhs, u"<="_qs);
}

void QQmlJSCodeGenerator::generate_CmpStrictEqual(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpStrictEqual);
    generateEqualityOperation(lhs, u"strictlyEquals"_qs, false);
}

void QQmlJSCodeGenerator::generate_CmpStrictNotEqual(int lhs)
{
    INJECT_TRACE_INFO(generate_CmpStrictNotEqual);
    generateEqualityOperation(lhs, u"strictlyEquals"_qs, true);
}

void QQmlJSCodeGenerator::generate_CmpIn(int lhs)
{
    Q_UNUSED(lhs)
    reject(u"CmpIn"_qs);
}

void QQmlJSCodeGenerator::generate_CmpInstanceOf(int lhs)
{
    Q_UNUSED(lhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_As(int lhs)
{
    INJECT_TRACE_INFO(generate_As);

    const QString input = use(registerVariable(lhs));
    const QQmlJSScope::ConstPtr contained = m_typeResolver->containedType(m_state.accumulatorOut);

    m_body += m_state.accumulatorVariableOut + u" = "_qs;
    if (m_state.accumulatorIn.storedType() == m_typeResolver->metaObjectType()
            && contained->isComposite()) {
        m_body += conversion(
                    m_typeResolver->genericType(contained), m_state.accumulatorOut.storedType(),
                    use(m_state.accumulatorVariableIn) + u"->cast("_qs + input + u')');
    } else {
        m_body += conversion(
                    m_typeResolver->genericType(contained), m_state.accumulatorOut.storedType(),
                    u'(' + metaObject(contained) + u")->cast("_qs + input + u')');
    }
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_UNot()
{
    INJECT_TRACE_INFO(generate_UNot);
    m_body += m_state.accumulatorVariableOut;
    m_body += u" = !"_qs;
    m_body += conversion(m_state.accumulatorIn.storedType(), m_typeResolver->boolType(),
                       use(m_state.accumulatorVariableIn));
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_UPlus()
{
    INJECT_TRACE_INFO(generate_UPlus);
    m_body += m_state.accumulatorVariableOut;
    m_body += u"= +"_qs;
    m_body += conversion(m_state.accumulatorIn, m_state.accumulatorOut,
                         use(m_state.accumulatorVariableIn));
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_UMinus()
{
    INJECT_TRACE_INFO(generate_UMinus);
    m_body += m_state.accumulatorVariableOut;
    m_body += u"= -"_qs;
    m_body += conversion(m_state.accumulatorIn, m_state.accumulatorOut,
                         use(m_state.accumulatorVariableIn));
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_UCompl()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Increment()
{
    INJECT_TRACE_INFO(generate_Increment);
    if (m_state.accumulatorVariableIn != m_state.accumulatorVariableOut) {
        m_body += m_state.accumulatorVariableOut + u" = "_qs
            + conversion(m_state.accumulatorIn, m_state.accumulatorOut,
                         use(m_state.accumulatorVariableIn)) + u"; "_qs;
        // No line break, to allow removal of the whole thing
    }
    m_body += u"++"_qs + use(m_state.accumulatorVariableOut) + u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_Decrement()
{
    INJECT_TRACE_INFO(generate_Decrement);
    if (m_state.accumulatorVariableIn != m_state.accumulatorVariableOut) {
        m_body += m_state.accumulatorVariableOut + u" = "_qs
            + conversion(m_state.accumulatorIn, m_state.accumulatorOut,
                         use(m_state.accumulatorVariableIn)) + u"; "_qs;
        // No line break, to allow removal of the whole thing
    }
    m_body += u"--"_qs + use(m_state.accumulatorVariableOut) + u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_Add(int lhs)
{
    INJECT_TRACE_INFO(generate_Add);
    generateArithmeticOperation(lhs, u"+"_qs);
}

void QQmlJSCodeGenerator::generate_BitAnd(int lhs)
{
    Q_UNUSED(lhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_BitOr(int lhs)
{
    Q_UNUSED(lhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_BitXor(int lhs)
{
    Q_UNUSED(lhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_UShr(int lhs)
{
    Q_UNUSED(lhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Shr(int lhs)
{
    Q_UNUSED(lhs);
    reject(u"Shr"_qs);
}

void QQmlJSCodeGenerator::generate_Shl(int lhs)
{
    Q_UNUSED(lhs);
    reject(u"Shl"_qs);
}

void QQmlJSCodeGenerator::generate_BitAndConst(int rhs)
{
    Q_UNUSED(rhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_BitOrConst(int rhs)
{
    Q_UNUSED(rhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_BitXorConst(int rhs)
{
    Q_UNUSED(rhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_UShrConst(int rhs)
{
    Q_UNUSED(rhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_ShrConst(int value)
{
    Q_UNUSED(value);
    reject(u"ShrConst"_qs);
}

void QQmlJSCodeGenerator::generate_ShlConst(int value)
{
    Q_UNUSED(value);
    reject(u"ShlConst"_qs);
}

void QQmlJSCodeGenerator::generate_Exp(int lhs)
{
    Q_UNUSED(lhs)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_Mul(int lhs)
{
    INJECT_TRACE_INFO(generate_Mul);
    generateArithmeticOperation(lhs, u"*"_qs);
}

void QQmlJSCodeGenerator::generate_Div(int lhs)
{
    INJECT_TRACE_INFO(generate_Div);
    generateArithmeticOperation(lhs, u"/"_qs);
}

void QQmlJSCodeGenerator::generate_Mod(int lhs)
{
    INJECT_TRACE_INFO(generate_Mod);

    const auto lhsVar = conversion(
                registerType(lhs).storedType(), m_typeResolver->jsPrimitiveType(),
                use(registerVariable(lhs)));
    const auto rhsVar = conversion(
                m_state.accumulatorIn.storedType(), m_typeResolver->jsPrimitiveType(),
                use(m_state.accumulatorVariableIn));
    Q_ASSERT(!lhsVar.isEmpty());
    Q_ASSERT(!rhsVar.isEmpty());

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_qs;
    m_body += conversion(m_typeResolver->jsPrimitiveType(), m_state.accumulatorOut.storedType(),
                       u'(' + lhsVar + u" % "_qs + rhsVar + u')');
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generate_Sub(int lhs)
{
    INJECT_TRACE_INFO(generate_Sub);
    generateArithmeticOperation(lhs, u"-"_qs);
}

void QQmlJSCodeGenerator::generate_InitializeBlockDeadTemporalZone(int firstReg, int count)
{
    Q_UNUSED(firstReg)
    Q_UNUSED(count)
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_ThrowOnNullOrUndefined()
{
    BYTECODE_UNIMPLEMENTED();
}

void QQmlJSCodeGenerator::generate_GetTemplateObject(int index)
{
    Q_UNUSED(index)
    BYTECODE_UNIMPLEMENTED();
}

static bool instructionManipulatesContext(QV4::Moth::Instr::Type type)
{
    using Type = QV4::Moth::Instr::Type;
    switch (type) {
    case Type::PopContext:
    case Type::PopScriptContext:
    case Type::CreateCallContext:
    case Type::CreateCallContext_Wide:
    case Type::PushCatchContext:
    case Type::PushCatchContext_Wide:
    case Type::PushWithContext:
    case Type::PushWithContext_Wide:
    case Type::PushBlockContext:
    case Type::PushBlockContext_Wide:
    case Type::CloneBlockContext:
    case Type::CloneBlockContext_Wide:
    case Type::PushScriptContext:
    case Type::PushScriptContext_Wide:
        return true;
    default:
        break;
    }
    return false;
}

QV4::Moth::ByteCodeHandler::Verdict QQmlJSCodeGenerator::startInstruction(
        QV4::Moth::Instr::Type type)
{
    m_state.State::operator=(nextStateFromAnnotations(m_state, *m_annotations));
    m_state.accumulatorVariableIn = m_registerVariables.value(Accumulator)
            .value(m_state.accumulatorIn.storedType());
    Q_ASSERT(!m_state.accumulatorIn.isValid() || !m_state.accumulatorVariableIn.isEmpty());

    auto labelIt = m_labels.constFind(currentInstructionOffset());
    if (labelIt != m_labels.constEnd()) {
        nextSection();
        m_body.setHasSideEffects(true);
        m_body.setLabel(*labelIt);
        m_body += *labelIt + u":;\n"_qs;
        m_skipUntilNextLabel = false;
    } else if (m_skipUntilNextLabel && !instructionManipulatesContext(type)) {
        return SkipInstruction;
    }

    nextSection();

    m_state.accumulatorVariableOut = registerVariable(QQmlJSTypePropagator::Accumulator);
    if (!m_state.accumulatorVariableOut.isEmpty())
        m_body.setWriteRegister(m_state.accumulatorVariableOut);

    // If the accumulator type is valid, we want an accumulator variable.
    // If not, we don't want one.
    // If the stored type is void, we don't need a variable, but we want to transport the type
    // information for any enum access or similar.
    Q_ASSERT((m_state.accumulatorOut.isValid()
              && m_state.accumulatorOut.storedType() != m_typeResolver->voidType())
             || m_state.accumulatorVariableOut.isEmpty());
    Q_ASSERT(!m_state.accumulatorOut.isValid()
             || m_state.accumulatorOut.storedType() == m_typeResolver->voidType()
             || !m_state.accumulatorVariableOut.isEmpty());

    const int currentLine = currentSourceLocation().startLine;
    if (currentLine != m_lastLineNumberUsed) {
        const int nextLine = nextJSLine(currentLine);
        for (auto line = currentLine - 1; line < nextLine - 1; ++line) {
            m_body += u"// "_qs;
            m_body += m_sourceCodeLines.value(line).trimmed();
            m_body += u'\n';
        }
        m_lastLineNumberUsed = currentLine;
    }
    return ProcessInstruction;
}

void QQmlJSCodeGenerator::endInstruction(QV4::Moth::Instr::Type)
{
    generateJumpCodeWithTypeConversions(0, JumpMode::None);
    nextSection();
}

void QQmlJSCodeGenerator::generateSetInstructionPointer()
{
    m_body += u"aotContext->setInstructionPointer("_qs
        + QString::number(nextInstructionOffset()) + u");\n"_qs;
}

void QQmlJSCodeGenerator::generateExceptionCheck()
{
    m_body += u"if (aotContext->engine->hasError())\n"_qs;
    m_body += u"    return "_qs + errorReturnValue() + u";\n"_qs;
}

void QQmlJSCodeGenerator::generateEqualityOperation(int lhs, const QString &function, bool invert)
{
    const QQmlJSRegisterContent lhsContent = registerType(lhs);
    auto isComparable = [&]() {
        if (m_typeResolver->isPrimitive(lhsContent)
                && m_typeResolver->isPrimitive(m_state.accumulatorIn)) {
            return true;
        }
        if (m_typeResolver->isNumeric(lhsContent) && m_state.accumulatorIn.isEnumeration())
            return true;
        if (m_typeResolver->isNumeric(m_state.accumulatorIn) && lhsContent.isEnumeration())
            return true;
        return false;
    };

    if (!isComparable())
        reject(u"equality comparison on non-primitive types"_qs);

    const QQmlJSScope::ConstPtr lhsType = lhsContent.storedType();
    const QQmlJSScope::ConstPtr rhsType = m_state.accumulatorIn.storedType();

    m_body += m_state.accumulatorVariableOut + u" = "_qs;

    const auto primitive = m_typeResolver->jsPrimitiveType();
    if (lhsType == rhsType && lhsType != primitive) {
        m_body += use(registerVariable(lhs));
        m_body += (invert ? u" != "_qs : u" == "_qs);
        m_body += use(m_state.accumulatorVariableIn);
    } else {
        if (invert)
            m_body += u'!';
        m_body += conversion(registerType(lhs).storedType(), primitive, use(registerVariable(lhs)));
        m_body += u'.';
        m_body += function;
        m_body += u'(';
        m_body += conversion(m_state.accumulatorIn.storedType(), primitive,
                             use(m_state.accumulatorVariableIn));
        m_body += u')';
    }
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generateCompareOperation(int lhs, const QString &cppOperator)
{
    m_body += m_state.accumulatorVariableOut;

    const auto lhsType = registerType(lhs);
    const QQmlJSScope::ConstPtr compareType =
            m_typeResolver->isNumeric(lhsType) && m_typeResolver->isNumeric(m_state.accumulatorIn)
                ? m_typeResolver->merge(lhsType, m_state.accumulatorIn).storedType()
                : m_typeResolver->jsPrimitiveType();
    m_body += u" = "_qs;
    m_body += conversion(registerType(lhs).storedType(), compareType, use(registerVariable(lhs)));
    m_body += u' ';
    m_body += cppOperator;
    m_body += u' ';
    m_body += conversion(m_state.accumulatorIn.storedType(), compareType,
                         use(m_state.accumulatorVariableIn));
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generateArithmeticOperation(int lhs, const QString &cppOperator)
{
    const auto lhsVar = conversion(registerType(lhs), m_state.accumulatorOut,
                                   use(registerVariable(lhs)));
    const auto rhsVar = conversion(m_state.accumulatorIn, m_state.accumulatorOut,
                                   use(m_state.accumulatorVariableIn));
    Q_ASSERT(!lhsVar.isEmpty());
    Q_ASSERT(!rhsVar.isEmpty());

    m_body += m_state.accumulatorVariableOut;
    m_body += u" = "_qs;
    m_body += lhsVar;
    m_body += u' ';
    m_body += cppOperator;
    m_body += u' ';
    m_body += rhsVar;
    m_body += u";\n"_qs;
}

void QQmlJSCodeGenerator::generateLookup(const QString &lookup, const QString &initialization,
                                        const QString &resultPreparation)
{
    if (!resultPreparation.isEmpty())
        m_body += resultPreparation + u";\n"_qs;
    m_body += u"while (!"_qs + lookup + u") {\n"_qs;
    generateSetInstructionPointer();
    m_body += initialization + u";\n"_qs;
    generateExceptionCheck();
    if (!resultPreparation.isEmpty())
        m_body += resultPreparation + u";\n"_qs;
    m_body += u"}\n"_qs;
}

void QQmlJSCodeGenerator::protectAccumulator()
{
    // If both m_state.accumulatorIn and m_state.accumulatorOut are QVariant, we will need to
    // prepare the output QVariant, and afterwards use the input variant. Therefore we need to move
    // the input out of the way first.
    if (m_state.accumulatorVariableIn == m_state.accumulatorVariableOut
            && m_state.accumulatorOut.storedType() == m_typeResolver->varType()) {
        m_state.accumulatorVariableIn = use(m_state.accumulatorVariableIn) + u"_moved"_qs;
        m_body += u"QVariant "_qs + m_state.accumulatorVariableIn
                + u" = std::move("_qs + m_state.accumulatorVariableOut + u");\n"_qs;
    }
}

void QQmlJSCodeGenerator::generateJumpCodeWithTypeConversions(
        int relativeOffset, JumpMode mode)
{
    nextSection();
    m_body.setHasSideEffects(true);
    m_body += u"{\n"_qs;
    int absoluteOffset =nextInstructionOffset() + relativeOffset;

    const auto annotation = m_annotations->constFind(absoluteOffset);
    if (annotation != m_annotations->constEnd()) {
        const auto &currentTypes = (*m_annotations)[currentInstructionOffset()].registers;
        const auto &conversions = annotation->expectedTargetTypesBeforeJump;

        for (auto regIt = conversions.constBegin(), regEnd = conversions.constEnd();
             regIt != regEnd; ++regIt) {
            int registerIndex = regIt.key();
            const QQmlJSRegisterContent targetType = regIt.value();
            if (!targetType.isValid() || !currentTypes.contains(registerIndex))
                continue;
            const QQmlJSRegisterContent currentType = currentTypes.value(registerIndex);
            if (!currentType.isValid() || currentType == targetType)
                continue;
            Q_ASSERT(m_registerVariables.contains(registerIndex));
            const auto &currentRegisterVariables = m_registerVariables[registerIndex];
            const auto variable = currentRegisterVariables.constFind(targetType.storedType());
            const QString oldVar = registerVariable(registerIndex);
            if (variable == currentRegisterVariables.end() || *variable == oldVar)
                continue;

            nextSection();
            m_body.setWriteRegister(*variable);
            m_body += *variable;
            m_body += u" = "_qs;
            m_body += conversion(currentTypes.value(registerIndex), targetType, use(oldVar));
            m_body += u";\n"_qs;
        }
    }

    nextSection();
    m_body.setHasSideEffects(true);
    if (relativeOffset) {
        auto labelIt = m_labels.find(absoluteOffset);
        if (labelIt == m_labels.end())
            labelIt = m_labels.insert(absoluteOffset, u"label_%1"_qs.arg(m_labels.count()));
        m_body.setJump(*labelIt, mode);
        m_body += u"    goto "_qs + *labelIt + u";\n"_qs;
    }
    m_body += u"}\n"_qs;
}

QString QQmlJSCodeGenerator::registerVariable(int index) const
{
    if (index >= QV4::CallData::OffsetCount && index < firstRegisterIndex()) {
        const int argumentIndex = index - QV4::CallData::OffsetCount;
        return u"*static_cast<"_qs
                + castTargetName(m_function->argumentTypes[argumentIndex])
                + u"*>(argumentsPtr["_qs + QString::number(argumentIndex) + u"])"_qs;
    }
    return m_registerVariables.value(index).value(registerType(index).storedType());
}

QQmlJSRegisterContent QQmlJSCodeGenerator::registerType(int index) const
{
    if (index >= QV4::CallData::OffsetCount && index < firstRegisterIndex()) {
        return m_typeResolver->globalType(
                    m_function->argumentTypes[index - QV4::CallData::OffsetCount]);
    }
    return m_state.registers[index];
}

QString QQmlJSCodeGenerator::conversion(const QQmlJSScope::ConstPtr &from,
                                       const QQmlJSScope::ConstPtr &to,
                                       const QString &variable) const
{
    // TODO: most values can be moved, which is much more efficient with the common types.
    //       add a move(from, to, variable) function that implements the moves.
    Q_ASSERT(!to->isComposite()); // We cannot directly convert to composites.

    const auto jsValueType = m_typeResolver->jsValueType();
    const auto varType = m_typeResolver->varType();
    const auto jsPrimitiveType = m_typeResolver->jsPrimitiveType();
    const auto boolType = m_typeResolver->boolType();

    auto zeroBoolOrNumeric = [&](const QQmlJSScope::ConstPtr &to) {
        if (to == boolType)
            return u"false"_qs;
        if (to == m_typeResolver->intType())
            return u"0"_qs;
        if (to == m_typeResolver->floatType())
            return u"0.0f"_qs;
        if (to == m_typeResolver->realType())
            return u"0.0"_qs;
        return QString();
    };

    if (from == m_typeResolver->voidType()) {
        if (to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            return u"static_cast<"_qs + to->internalName() + u" *>(nullptr)"_qs;
        const QString zero = zeroBoolOrNumeric(to);
        if (!zero.isEmpty())
            return zero;
        if (to == m_typeResolver->stringType())
            return QQmlJSUtils::toLiteral(u"undefined"_qs);
        if (from == to)
            return variable;
        // Anything else is just the default constructed type.
        return to->augmentedInternalName() + u"()"_qs;
    }

    if (from == to)
        return variable;

    if (from->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
        if (to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
            // Compare internalName here. The same C++ type can be exposed muliple times in
            // different QML types. However, the C++ names have to be unique. We can always
            // static_cast to those.

            for (QQmlJSScope::ConstPtr base = from; base; base = base->baseType()) {
                // We still have to cast as other execution paths may result in different types.
                if (base->internalName() == to->internalName())
                    return u"static_cast<"_qs + to->internalName() + u" *>("_qs + variable + u')';
            }
            for (QQmlJSScope::ConstPtr base = to; base; base = base->baseType()) {
                if (base->internalName() == from->internalName())
                    return u"static_cast<"_qs + to->internalName() + u" *>("_qs + variable + u')';
            }
        } else if (to == m_typeResolver->boolType()) {
            return u'(' + variable + u" != nullptr)"_qs;
        }
    }

    auto isJsValue = [&](const QQmlJSScope::ConstPtr &candidate) {
        return candidate == jsValueType || candidate->isScript();
    };

    if (isJsValue(from) && isJsValue(to))
        return variable;

    const auto isBoolOrNumber = [&](const QQmlJSScope::ConstPtr &type) {
        return m_typeResolver->isNumeric(m_typeResolver->globalType(type))
                || type == m_typeResolver->boolType()
                || type->scopeType() ==  QQmlJSScope::EnumScope;
    };

    if (from == m_typeResolver->realType() && to == m_typeResolver->intType())
        return u"QJSNumberCoercion::toInteger("_qs + variable + u')';

    if (isBoolOrNumber(from) && isBoolOrNumber(to))
        return to->internalName() + u'(' + variable + u')';

    if (from == jsPrimitiveType) {
        if (to == m_typeResolver->realType())
            return variable + u".toDouble()"_qs;
        if (to == boolType)
            return variable + u".toBoolean()"_qs;
        if (to == m_typeResolver->intType())
            return variable + u".toInteger()"_qs;
        if (to == m_typeResolver->stringType())
            return variable + u".toString()"_qs;
        if (to == jsValueType)
            return u"QJSValue(QJSPrimitiveValue("_qs + variable + u"))"_qs;
        if (to == varType)
            return variable + u".toVariant()"_qs;
        if (to->accessSemantics() == QQmlJSScope::AccessSemantics::Reference)
            return u"static_cast<"_qs + to->internalName() + u" *>(nullptr)"_qs;
    }

    if (isJsValue(from)) {
        if (to == jsPrimitiveType)
            return variable + u".toPrimitive()"_qs;
        if (to == varType)
            return variable + u".toVariant(QJSValue::RetainJSObjects)"_qs;
        return u"qjsvalue_cast<"_qs + castTargetName(to) + u">("_qs + variable + u')';
    }

    if (to == jsPrimitiveType)
        return u"QJSPrimitiveValue("_qs + variable + u')';

    if (to == jsValueType)
        return u"aotContext->engine->toScriptValue("_qs + variable + u')';

    if (from == varType) {
        if (to == m_typeResolver->listPropertyType())
            return u"QQmlListReference("_qs + variable + u", aotContext->qmlEngine())"_qs;
        return u"aotContext->engine->fromVariant<"_qs + castTargetName(to) + u">("_qs
                + variable + u')';
    }

    if (to == varType)
        return u"QVariant::fromValue("_qs + variable + u')';

    if (from == m_typeResolver->urlType() && to == m_typeResolver->stringType())
        return variable + u".toString()"_qs;

    if (from == m_typeResolver->stringType() && to == m_typeResolver->urlType())
        return u"QUrl("_qs + variable + u')';

    const auto retrieveFromPrimitive = [&](const QQmlJSScope::ConstPtr &type) {
        if (type == m_typeResolver->boolType())
            return u".toBoolean()"_qs;
        if (type == m_typeResolver->intType())
            return u".toInteger()"_qs;
        if (type == m_typeResolver->realType())
            return u".toDouble()"_qs;
        if (type == m_typeResolver->stringType())
            return u".toString()"_qs;
        return QString();
    };

    const auto fitsIntoPrimitive = [&](const QQmlJSScope::ConstPtr &type) {
        return !retrieveFromPrimitive(type).isEmpty() || type == m_typeResolver->floatType();
    };

    if (fitsIntoPrimitive(from)) {
        const QString retrieve = retrieveFromPrimitive(to);
        if (!retrieve.isEmpty())
            return u"QJSPrimitiveValue("_qs + variable + u')' + retrieve;
    }

    // TODO: more efficient string conversions, possibly others

    return u"aotContext->engine->fromScriptValue<"_qs + castTargetName(to)
            + u">(aotContext->engine->toScriptValue("_qs + variable + u"))"_qs;
}

int QQmlJSCodeGenerator::nextJSLine(uint line) const
{
    auto findLine = [](uint line, const QV4::CompiledData::CodeOffsetToLine &entry) {
        return entry.line > line;
    };
    const auto codeToLine
        = std::upper_bound(m_context->lineNumberMapping.constBegin(),
                           m_context->lineNumberMapping.constEnd(),
                           line,
                           findLine);
    bool bNoNextLine = m_context->lineNumberMapping.constEnd() == codeToLine;

    return static_cast<int>(bNoNextLine ? -1 : codeToLine->line);
}

void QQmlJSCodeGenerator::reject(const QString &thing)
{
    setError(u"Cannot generate efficient code for %1"_qs.arg(thing));
}

QT_END_NAMESPACE
