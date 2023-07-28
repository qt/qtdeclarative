// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsbasicblocks_p.h"

#include <QtQml/private/qv4instr_moth_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

DEFINE_BOOL_CONFIG_OPTION(qv4DumpBasicBlocks, QV4_DUMP_BASIC_BLOCKS)

void QQmlJSBasicBlocks::dumpBasicBlocks()
{
    qDebug().noquote() << "=== Basic Blocks for \"%1\""_L1.arg(m_context->name);
    for (const auto &[blockOffset, block] : m_basicBlocks) {
        QDebug debug = qDebug().nospace();
        debug << "Block " << blockOffset << ":\n";
        debug << "  jumpOrigins[" << block.jumpOrigins.size() << "]: ";
        for (auto origin : block.jumpOrigins) {
            debug << origin << ", ";
        }
        debug << "\n  readRegisters[" << block.readRegisters.size() << "]: ";
        for (auto reg : block.readRegisters) {
            debug << reg << ", ";
        }
        debug << "\n  readTypes[" << block.readTypes.size() << "]: ";
        for (const auto &type : block.readTypes) {
            debug << type->augmentedInternalName() << ", ";
        }
        debug << "\n  jumpTarget: " << block.jumpTarget;
        debug << "\n  jumpIsUnConditional: " << block.jumpIsUnconditional;
    }
    qDebug() << "\n";
}

void QQmlJSBasicBlocks::dumpDOTGraph()
{
    QString output;
    QTextStream s{ &output };
    s << "=== Basic Blocks Graph in DOT format for \"%1\" (spaces are encoded as"
         " &#160; to preserve formatting)\n"_L1.arg(m_context->name);
    s << "digraph BasicBlocks {\n"_L1;

    std::map<int, BasicBlock> blocks{ m_basicBlocks.begin(), m_basicBlocks.end() };
    for (const auto &[blockOffset, block] : blocks) {
        for (int originOffset : block.jumpOrigins) {
            int originBlockOffset;
            auto originBlockIt = blocks.find(originOffset);
            bool isBackEdge = false;
            if (originBlockIt != blocks.end()) {
                originBlockOffset = originOffset;
                isBackEdge = originOffset > blockOffset
                        && originBlockIt->second.jumpIsUnconditional;
            } else {
                originBlockOffset = std::prev(blocks.lower_bound(originOffset))->first;
            }
            s << "    %1 -> %2%3\n"_L1.arg(QString::number(originBlockOffset))
                            .arg(QString::number(blockOffset))
                            .arg(isBackEdge ? " [color=blue]"_L1 : ""_L1);
        }
    }

    for (const auto &[blockOffset, block] : blocks) {
        int beginOffset = std::max(0, blockOffset);
        auto nextBlockIt = blocks.lower_bound(blockOffset + 1);
        int nextBlockOffset = nextBlockIt == blocks.end() ? m_context->code.size() : nextBlockIt->first;
        QString dump = QV4::Moth::dumpBytecode(
                m_context->code.constData(), m_context->code.size(), m_context->locals.size(),
                m_context->formals->length(), beginOffset, nextBlockOffset - 1,
                m_context->lineAndStatementNumberMapping);
        dump = dump.replace(" "_L1, "&#160;"_L1); // prevent collapse of extra whitespace for formatting
        dump = dump.replace("\n"_L1, "\\l"_L1); // new line + left aligned
        s << "    %1 [shape=record, fontname=\"Monospace\", label=\"{Block %1: | %2}\"]\n"_L1
                        .arg(QString::number(blockOffset))
                        .arg(dump);
    }
    s << "}\n"_L1;

    // Have unique names to prevent overwriting of functions with the same name (eg. anonymous functions).
    static int functionCount = 0;
    static const auto dumpFolderPath = qEnvironmentVariable("QV4_DUMP_BASIC_BLOCKS");

    QString expressionName = m_context->name == ""_L1
            ? "anonymous"_L1
            : QString(m_context->name).replace(" "_L1, "_"_L1);
    QString fileName = "function"_L1 + QString::number(functionCount++) + "_"_L1 + expressionName + ".gv"_L1;
    QFile dumpFile(dumpFolderPath + (dumpFolderPath.endsWith("/"_L1) ? ""_L1 : "/"_L1) + fileName);

    if (dumpFolderPath == "-"_L1 || dumpFolderPath == "1"_L1 || dumpFolderPath == "true"_L1) {
        qDebug().noquote() << output;
    } else {
        if (!dumpFile.open(QIODeviceBase::Truncate | QIODevice::WriteOnly)) {
            qDebug() << "Error: Could not open file to dump the basic blocks into";
        } else {
            dumpFile.write(("//"_L1 + output).toLatin1().toStdString().c_str());
            dumpFile.close();
        }
    }
}

template<typename Container>
void deduplicate(Container &container)
{
    std::sort(container.begin(), container.end());
    auto erase = std::unique(container.begin(), container.end());
    container.erase(erase, container.end());
}

QQmlJSCompilePass::InstructionAnnotations QQmlJSBasicBlocks::run(
        const Function *function,
        const InstructionAnnotations &annotations,
        QQmlJS::DiagnosticMessage *error)
{
    m_function = function;
    m_annotations = annotations;
    m_error = error;

    for (int i = 0, end = function->argumentTypes.size(); i != end; ++i) {
        InstructionAnnotation annotation;
        annotation.changedRegisterIndex = FirstArgument + i;
        annotation.changedRegister = function->argumentTypes[i];
        m_annotations[-annotation.changedRegisterIndex] = annotation;
    }

    for (int i = 0, end = function->registerTypes.size(); i != end; ++i) {
        InstructionAnnotation annotation;
        annotation.changedRegisterIndex = firstRegisterIndex() + i;
        annotation.changedRegister = function->registerTypes[i];
        m_annotations[-annotation.changedRegisterIndex] = annotation;
    }

    m_basicBlocks.insert_or_assign(m_annotations.begin().key(), BasicBlock());

    const QByteArray byteCode = function->code;
    decode(byteCode.constData(), static_cast<uint>(byteCode.size()));
    if (m_hadBackJumps) {
        // We may have missed some connections between basic blocks if there were back jumps.
        // Fill them in via a second pass.

        // We also need to re-calculate the jump targets then because the basic block boundaries
        // may have shifted.
        for (auto it = m_basicBlocks.begin(), end = m_basicBlocks.end(); it != end; ++it) {
            it->second.jumpTarget = -1;
            it->second.jumpIsUnconditional = false;
        }

        m_skipUntilNextLabel = false;

        reset();
        decode(byteCode.constData(), static_cast<uint>(byteCode.size()));
        for (auto it = m_basicBlocks.begin(), end = m_basicBlocks.end(); it != end; ++it)
            deduplicate(it->second.jumpOrigins);
    }

    populateBasicBlocks();
    populateReaderLocations();
    adjustTypes();

    if (qv4DumpBasicBlocks()) {
        dumpBasicBlocks();
        dumpDOTGraph();
    }

    return std::move(m_annotations);
}

QV4::Moth::ByteCodeHandler::Verdict QQmlJSBasicBlocks::startInstruction(QV4::Moth::Instr::Type type)
{
    auto it = m_basicBlocks.find(currentInstructionOffset());
    if (it != m_basicBlocks.end()) {
        m_skipUntilNextLabel = false;
    } else if (m_skipUntilNextLabel && !instructionManipulatesContext(type)) {
        return SkipInstruction;
    }

    return ProcessInstruction;
}

void QQmlJSBasicBlocks::endInstruction(QV4::Moth::Instr::Type)
{
    if (m_skipUntilNextLabel)
        return;
    auto it = m_basicBlocks.find(nextInstructionOffset());
    if (it != m_basicBlocks.end())
        it->second.jumpOrigins.append(currentInstructionOffset());
}

void QQmlJSBasicBlocks::generate_Jump(int offset)
{
    processJump(offset, Unconditional);
}

void QQmlJSBasicBlocks::generate_JumpTrue(int offset)
{
    processJump(offset, Conditional);
}

void QQmlJSBasicBlocks::generate_JumpFalse(int offset)
{
    processJump(offset, Conditional);
}

void QQmlJSBasicBlocks::generate_JumpNoException(int offset)
{
    processJump(offset, Conditional);
}

void QQmlJSBasicBlocks::generate_JumpNotUndefined(int offset)
{
    processJump(offset, Conditional);
}

void QQmlJSBasicBlocks::generate_Ret()
{
    m_skipUntilNextLabel = true;
}

void QQmlJSBasicBlocks::generate_ThrowException()
{
    m_skipUntilNextLabel = true;
}

void QQmlJSBasicBlocks::generate_DefineArray(int argc, int argv)
{
    if (argc == 0)
        return; // empty array/list, nothing to do

    m_objectAndArrayDefinitions.append({
        currentInstructionOffset(), ObjectOrArrayDefinition::arrayClassId, argc, argv
    });
}

void QQmlJSBasicBlocks::generate_DefineObjectLiteral(int internalClassId, int argc, int argv)
{
    if (argc == 0)
        return;

    m_objectAndArrayDefinitions.append({ currentInstructionOffset(), internalClassId, argc, argv });
}

void QQmlJSBasicBlocks::processJump(int offset, JumpMode mode)
{
    if (offset < 0)
        m_hadBackJumps = true;
    const int jumpTarget = absoluteOffset(offset);
    Q_ASSERT(!m_basicBlocks.isEmpty());
    auto currentBlock = m_basicBlocks.lower_bound(currentInstructionOffset());
    if (currentBlock == m_basicBlocks.end() || currentBlock->first != currentInstructionOffset())
        --currentBlock;
    currentBlock->second.jumpTarget = jumpTarget;
    currentBlock->second.jumpIsUnconditional = (mode == Unconditional);
    m_basicBlocks[jumpTarget].jumpOrigins.append(currentInstructionOffset());
    if (mode == Unconditional)
        m_skipUntilNextLabel = true;
    else
        m_basicBlocks.insert(nextInstructionOffset(), BasicBlock());
}

template<typename ContainerA, typename ContainerB>
static bool containsAny(const ContainerA &container, const ContainerB &elements)
{
    for (const auto &element : elements) {
        if (container.contains(element))
            return true;
    }
    return false;
}

template<typename ContainerA, typename ContainerB>
static bool containsAll(const ContainerA &container, const ContainerB &elements)
{
    for (const auto &element : elements) {
        if (!container.contains(element))
            return false;
    }
    return true;
}

template<class Key, class T, class Compare = std::less<Key>,
         class KeyContainer = QList<Key>, class MappedContainer = QList<T>>
class NewFlatMap
{
public:
    using OriginalFlatMap = QFlatMap<Key, T, Compare, KeyContainer, MappedContainer>;

    void appendOrdered(const typename OriginalFlatMap::iterator &i) {
        keys.append(i.key());
        values.append(i.value());
    }

    OriginalFlatMap take() {
        OriginalFlatMap result(Qt::OrderedUniqueRange, std::move(keys), std::move(values));
        keys.clear();
        values.clear();
        return result;
    }

private:
    typename OriginalFlatMap::key_container_type keys;
    typename OriginalFlatMap::mapped_container_type values;
};

struct PendingBlock
{
    QList<int> conversions;
    int start = -1;
    bool registerActive = false;
};

void QQmlJSBasicBlocks::populateReaderLocations()
{
    using NewInstructionAnnotations = NewFlatMap<int, InstructionAnnotation>;

    bool erasedReaders = false;
    auto eraseDeadStore = [&](const InstructionAnnotations::iterator &it) {
        auto reader = m_readerLocations.find(it.key());
        if (reader != m_readerLocations.end()
                && (reader->typeReaders.isEmpty()
                    || reader->registerReadersAndConversions.isEmpty())) {

            if (it->second.isRename) {
                // If it's a rename, it doesn't "own" its output type. The type may
                // still be read elsewhere, even if this register isn't. However, we're
                // not interested in the variant or any other details of the register.
                // Therefore just delete it.
                it->second.changedRegisterIndex = InvalidRegister;
                it->second.changedRegister = QQmlJSRegisterContent();
            } else {
                // void the output, rather than deleting it. We still need its variant.
                bool adjusted = m_typeResolver->adjustTrackedType(
                            it->second.changedRegister.storedType(),
                            m_typeResolver->voidType());
                Q_ASSERT(adjusted); // Can always convert to void

                adjusted = m_typeResolver->adjustTrackedType(
                            m_typeResolver->containedType(it->second.changedRegister),
                            m_typeResolver->voidType());
                Q_ASSERT(adjusted); // Can always convert to void
            }
            m_readerLocations.erase(reader);

            // If it's not a label and has no side effects, we can drop the instruction.
            if (!it->second.hasSideEffects) {
                if (!it->second.readRegisters.isEmpty()) {
                    it->second.readRegisters.clear();
                    erasedReaders = true;
                }
                if (m_basicBlocks.find(it.key()) == m_basicBlocks.end())
                    return true;
            }
        }
        return false;
    };

    NewInstructionAnnotations newAnnotations;
    for (auto writeIt = m_annotations.begin(), writeEnd = m_annotations.end();
         writeIt != writeEnd; ++writeIt) {
        const int writtenRegister = writeIt->second.changedRegisterIndex;
        if (writtenRegister == InvalidRegister) {
            newAnnotations.appendOrdered(writeIt);
            continue;
        }

        RegisterAccess &access = m_readerLocations[writeIt.key()];
        access.trackedRegister = writtenRegister;
        if (writeIt->second.changedRegister.isConversion()) {
            // If it's a conversion, we have to check for all readers of the conversion origins.
            // This happens at jump targets where different types are merged. A StoreReg or similar
            // instruction must be optimized out if none of the types it can hold is read anymore.
            access.trackedTypes = writeIt->second.changedRegister.conversionOrigins();
        } else {
            access.trackedTypes.append(
                    m_typeResolver->trackedContainedType(writeIt->second.changedRegister));
        }

        auto blockIt = m_basicBlocks.lower_bound(writeIt.key());
        if (blockIt == m_basicBlocks.end() || blockIt->first != writeIt.key())
            --blockIt;

        QList<PendingBlock> blocks = { { {}, blockIt->first, true } };
        QHash<int, PendingBlock> processedBlocks;
        bool isFirstBlock = true;

        while (!blocks.isEmpty()) {
            const PendingBlock block = blocks.takeLast();

            // We can re-enter the first block from the beginning.
            // We will then find any reads before the write we're currently examining.
            if (!isFirstBlock)
                processedBlocks.insert(block.start, block);

            auto nextBlock = m_basicBlocks.find(block.start);
            auto currentBlock = nextBlock++;
            bool registerActive = block.registerActive;
            QList<int> conversions = block.conversions;

            const auto blockEnd = (nextBlock == m_basicBlocks.end())
                    ? m_annotations.end()
                    : m_annotations.find(nextBlock->first);

            auto blockInstr = isFirstBlock
                    ? (writeIt + 1)
                    : m_annotations.find(currentBlock->first);
            for (; blockInstr != blockEnd; ++blockInstr) {
                if (registerActive
                        && blockInstr->second.typeConversions.contains(writtenRegister)) {
                    conversions.append(blockInstr.key());
                }

                for (auto readIt = blockInstr->second.readRegisters.constBegin(),
                     end = blockInstr->second.readRegisters.constEnd();
                     readIt != end; ++readIt) {
                    if (!blockInstr->second.isRename && containsAny(
                                readIt->second.content.conversionOrigins(), access.trackedTypes)) {
                        Q_ASSERT(readIt->second.content.isConversion());
                        Q_ASSERT(readIt->second.content.conversionResult());
                        access.typeReaders[blockInstr.key()]
                                = readIt->second.content.conversionResult();
                    }
                    if (registerActive && readIt->first == writtenRegister)
                        access.registerReadersAndConversions[blockInstr.key()] = conversions;
                }

                if (blockInstr->second.changedRegisterIndex == writtenRegister) {
                    conversions.clear();
                    registerActive = false;
                }
            }

            auto scheduleBlock = [&](int blockStart) {
                // If we find that an already processed block has the register activated by this jump,
                // we need to re-evaluate it. We also need to propagate any newly found conversions.
                const auto processed = processedBlocks.find(blockStart);
                if (processed == processedBlocks.end())
                    blocks.append({conversions, blockStart, registerActive});
                else if (registerActive && !processed->registerActive)
                    blocks.append({conversions, blockStart, registerActive});
                else if (!containsAll(processed->conversions, conversions))
                    blocks.append({processed->conversions + conversions, blockStart, registerActive});
            };

            if (!currentBlock->second.jumpIsUnconditional && nextBlock != m_basicBlocks.end())
                scheduleBlock(nextBlock->first);

            const int jumpTarget = currentBlock->second.jumpTarget;
            if (jumpTarget != -1)
                scheduleBlock(jumpTarget);

            if (isFirstBlock)
                isFirstBlock = false;
        }

        if (!eraseDeadStore(writeIt))
            newAnnotations.appendOrdered(writeIt);
    }
    m_annotations = newAnnotations.take();

    while (erasedReaders) {
        erasedReaders = false;

        for (auto it = m_annotations.begin(), end = m_annotations.end(); it != end; ++it) {
            InstructionAnnotation &instruction = it->second;
            if (instruction.changedRegisterIndex < InvalidRegister) {
                newAnnotations.appendOrdered(it);
                continue;
            }

            auto readers = m_readerLocations.find(it.key());
            if (readers != m_readerLocations.end()) {
                for (auto typeIt = readers->typeReaders.begin();
                     typeIt != readers->typeReaders.end();) {
                    if (m_annotations.contains(typeIt.key()))
                        ++typeIt;
                    else
                        typeIt = readers->typeReaders.erase(typeIt);
                }

                for (auto registerIt = readers->registerReadersAndConversions.begin();
                     registerIt != readers->registerReadersAndConversions.end();) {
                    if (m_annotations.contains(registerIt.key()))
                        ++registerIt;
                    else
                        registerIt = readers->registerReadersAndConversions.erase(registerIt);
                }
            }

            if (!eraseDeadStore(it))
                newAnnotations.appendOrdered(it);
        }

        m_annotations = newAnnotations.take();
    }
}

bool QQmlJSBasicBlocks::canMove(int instructionOffset, const RegisterAccess &access) const
{
    if (access.registerReadersAndConversions.size() != 1)
        return false;
    const auto basicBlockForInstruction = [this](int instruction) {
        auto block = m_basicBlocks.lower_bound(instruction);
        if (block == m_basicBlocks.end() || block.key() == instruction)
            return block;
        Q_ASSERT(block.key() > instruction);
        if (block == m_basicBlocks.begin())
            return m_basicBlocks.end();
        return --block;
    };
    return basicBlockForInstruction(instructionOffset)
            == basicBlockForInstruction(access.registerReadersAndConversions.begin().key());
}

static QString adjustErrorMessage(
        const QQmlJSScope::ConstPtr &origin, const QQmlJSScope::ConstPtr &conversion) {
    return QLatin1String("Cannot convert from ")
            + origin->internalName() + QLatin1String(" to ") + conversion->internalName();
}

static QString adjustErrorMessage(
        const QQmlJSScope::ConstPtr &origin, const QList<QQmlJSScope::ConstPtr> &conversions) {
    if (conversions.size() == 1)
        return adjustErrorMessage(origin, conversions[0]);

    QString types;
    for (const QQmlJSScope::ConstPtr &type : conversions) {
        if (!types.isEmpty())
            types += QLatin1String(", ");
        types += type->internalName();
    }
    return QLatin1String("Cannot convert from ")
            + origin->internalName() + QLatin1String(" to union of ") + types;
}

void QQmlJSBasicBlocks::adjustTypes()
{
    using NewVirtualRegisters = NewFlatMap<int, VirtualRegister>;

    QHash<int, QList<int>> liveConversions;
    QHash<int, QList<int>> movableReads;

    const auto handleRegisterReadersAndConversions
            = [&](QHash<int, RegisterAccess>::const_iterator it) {
        for (auto conversions = it->registerReadersAndConversions.constBegin(),
             end = it->registerReadersAndConversions.constEnd(); conversions != end;
             ++conversions) {
            if (conversions->isEmpty() && canMove(it.key(), it.value()))
                movableReads[conversions.key()].append(it->trackedRegister);
            for (int conversion : *conversions)
                liveConversions[conversion].append(it->trackedRegister);
        }
    };

    const auto transformRegister = [&](const QQmlJSRegisterContent &content) {
        const QQmlJSScope::ConstPtr conversion
                = m_typeResolver->storedType(m_typeResolver->containedType(content));
        if (!m_typeResolver->adjustTrackedType(content.storedType(), conversion))
            setError(adjustErrorMessage(content.storedType(), conversion));
    };

    // Handle the array definitions first.
    // Changing the array type changes the expected element types.
    auto adjustArray = [&](int instructionOffset) {
        auto it = m_readerLocations.find(instructionOffset);
        if (it == m_readerLocations.end())
            return;

        const InstructionAnnotation &annotation = m_annotations[instructionOffset];

        Q_ASSERT(it->trackedTypes.size() == 1);
        Q_ASSERT(it->trackedTypes[0] == m_typeResolver->containedType(annotation.changedRegister));
        Q_ASSERT(!annotation.readRegisters.isEmpty());

        if (!m_typeResolver->adjustTrackedType(it->trackedTypes[0], it->typeReaders.values()))
            setError(adjustErrorMessage(it->trackedTypes[0], it->typeReaders.values()));

        // Now we don't adjust the type we store, but rather the type we expect to read. We
        // can do this because we've tracked the read type when we defined the array in
        // QQmlJSTypePropagator.
        if (QQmlJSScope::ConstPtr valueType = it->trackedTypes[0]->valueType()) {
            const QQmlJSRegisterContent content = annotation.readRegisters.begin().value().content;
            const QQmlJSScope::ConstPtr contained = m_typeResolver->containedType(content);
            if (!m_typeResolver->adjustTrackedType(contained, valueType))
                setError(adjustErrorMessage(contained, valueType));

            // We still need to adjust the stored type, too.
            transformRegister(content);
        }

        handleRegisterReadersAndConversions(it);
        m_readerLocations.erase(it);
    };

    // Handle the object definitions.
    // Changing the object type changes the expected property types.
    const auto adjustObject = [&](const ObjectOrArrayDefinition &object) {
        auto it = m_readerLocations.find(object.instructionOffset);
        if (it == m_readerLocations.end())
            return;

        const InstructionAnnotation &annotation = m_annotations[object.instructionOffset];

        Q_ASSERT(it->trackedTypes.size() == 1);
        Q_ASSERT(it->trackedTypes[0] == m_typeResolver->containedType(annotation.changedRegister));
        Q_ASSERT(!annotation.readRegisters.isEmpty());

        if (!m_typeResolver->adjustTrackedType(it->trackedTypes[0], it->typeReaders.values()))
            setError(adjustErrorMessage(it->trackedTypes[0], it->typeReaders.values()));

        QQmlJSScope::ConstPtr resultType = it->trackedTypes[0];

        const int classSize = m_jsUnitGenerator->jsClassSize(object.internalClassId);
        Q_ASSERT(object.argc >= classSize);

        for (int i = 0; i < classSize; ++i) {
            // Now we don't adjust the type we store, but rather the types we expect to read. We
            // can do this because we've tracked the read types when we defined the object in
            // QQmlJSTypePropagator.

            const QString propName = m_jsUnitGenerator->jsClassMember(object.internalClassId, i);
            const QQmlJSMetaProperty property = resultType->property(propName);
            if (!property.isValid()) {
                setError(
                        resultType->internalName()
                        + QLatin1String(" has no property called ")
                        + propName);
                continue;
            }
            const QQmlJSScope::ConstPtr propType = property.type();
            if (propType.isNull()) {
                setError(QLatin1String("Cannot resolve type of property ") + propName);
                continue;
            }
            const QQmlJSRegisterContent content = annotation.readRegisters[object.argv + i].content;
            const QQmlJSScope::ConstPtr contained = m_typeResolver->containedType(content);
            if (!m_typeResolver->adjustTrackedType(contained, propType))
                setError(adjustErrorMessage(contained, propType));

            // We still need to adjust the stored type, too.
            transformRegister(content);
        }

        // The others cannot be adjusted. We don't know their names, yet.
        // But we might still be able to use the variants.
    };

    // Iterate in reverse so that we can have nested lists and objects and the types are propagated
    // from the outer lists/objects to the inner ones.
    for (auto it = m_objectAndArrayDefinitions.crbegin(), end = m_objectAndArrayDefinitions.crend();
         it != end; ++it) {
        if (it->internalClassId != -1)
            adjustObject(*it);
        else
            adjustArray(it->instructionOffset);
    }

    for (auto it = m_readerLocations.begin(), end = m_readerLocations.end(); it != end; ++it) {
        handleRegisterReadersAndConversions(it);

        // There is always one first occurrence of any tracked type. Conversions don't change
        // the type.
        if (it->trackedTypes.size() != 1)
            continue;

        if (!m_typeResolver->adjustTrackedType(it->trackedTypes[0], it->typeReaders.values()))
            setError(adjustErrorMessage(it->trackedTypes[0], it->typeReaders.values()));
    }



    NewVirtualRegisters newRegisters;
    for (auto i = m_annotations.begin(), iEnd = m_annotations.end(); i != iEnd; ++i) {
        if (i->second.changedRegisterIndex != InvalidRegister)
            transformRegister(i->second.changedRegister);

        for (auto conversion = i->second.typeConversions.begin(),
             conversionEnd = i->second.typeConversions.end(); conversion != conversionEnd;
             ++conversion) {
            if (!liveConversions[i.key()].contains(conversion.key()))
                continue;

            QQmlJSScope::ConstPtr newResult;
            const auto content = conversion->second.content;
            if (content.isConversion()) {
                QQmlJSScope::ConstPtr conversionResult = content.conversionResult();
                const auto conversionOrigins = content.conversionOrigins();
                for (const auto &origin : conversionOrigins)
                    newResult = m_typeResolver->merge(newResult, origin);
                if (!m_typeResolver->adjustTrackedType(conversionResult, newResult))
                    setError(adjustErrorMessage(conversionResult, newResult));
            }
            transformRegister(content);
            newRegisters.appendOrdered(conversion);
        }
        i->second.typeConversions = newRegisters.take();

        for (int movable : std::as_const(movableReads[i.key()]))
            i->second.readRegisters[movable].canMove = true;
    }
}

void QQmlJSBasicBlocks::populateBasicBlocks()
{
    for (auto blockNext = m_basicBlocks.begin(), blockEnd = m_basicBlocks.end();
         blockNext != blockEnd;) {

        const auto blockIt = blockNext++;
        BasicBlock &block = blockIt->second;
        QList<QQmlJSScope::ConstPtr> writtenTypes;
        QList<int> writtenRegisters;

        const auto instrEnd = (blockNext == blockEnd)
                ? m_annotations.end()
                : m_annotations.find(blockNext->first);
        for (auto instrIt = m_annotations.find(blockIt->first); instrIt != instrEnd; ++instrIt) {
            const InstructionAnnotation &instruction = instrIt->second;
            for (auto it = instruction.readRegisters.begin(), end = instruction.readRegisters.end();
                 it != end; ++it) {
                if (!instruction.isRename) {
                    Q_ASSERT(it->second.content.isConversion());
                    for (const QQmlJSScope::ConstPtr &origin :
                         it->second.content.conversionOrigins()) {
                        if (!writtenTypes.contains(origin))
                            block.readTypes.append(origin);
                    }
                }
                if (!writtenRegisters.contains(it->first))
                    block.readRegisters.append(it->first);
            }

            // If it's just a renaming, the type has existed in a different register before.
            if (instruction.changedRegisterIndex != InvalidRegister) {
                if (!instruction.isRename) {
                    writtenTypes.append(m_typeResolver->trackedContainedType(
                                            instruction.changedRegister));
                }
                writtenRegisters.append(instruction.changedRegisterIndex);
            }
        }

        deduplicate(block.readTypes);
        deduplicate(block.readRegisters);
    }
}

QT_END_NAMESPACE
