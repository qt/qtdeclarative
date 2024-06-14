// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsoptimizations_p.h"
#include "qqmljsbasicblocks_p.h"
#include "qqmljsutils_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

QQmlJSCompilePass::BlocksAndAnnotations QQmlJSOptimizations::run(const Function *function)
{
    m_function = function;

    populateBasicBlocks();
    populateReaderLocations();
    removeDeadStoresUntilStable();
    adjustTypes();

    return { std::move(m_basicBlocks), std::move(m_annotations) };
}

struct PendingBlock
{
    QQmlJSOptimizations::Conversions conversions;
    int start = -1;
    bool registerActive = false;
};

template<typename ContainerA, typename ContainerB>
static bool containsAny(const ContainerA &container, const ContainerB &elements)
{
    for (const auto &element : elements) {
        if (container.contains(element))
            return true;
    }
    return false;
}

template<class Key, class T, class Compare = std::less<Key>,
         class KeyContainer = QList<Key>, class MappedContainer = QList<T>>
class NewFlatMap
{
public:
    using OriginalFlatMap = QFlatMap<Key, T, Compare, KeyContainer, MappedContainer>;

    void appendOrdered(const typename OriginalFlatMap::iterator &i)
    {
        keys.append(i.key());
        values.append(i.value());
    }

    OriginalFlatMap take()
    {
        OriginalFlatMap result(Qt::OrderedUniqueRange, std::move(keys), std::move(values));
        keys.clear();
        values.clear();
        return result;
    }

private:
    typename OriginalFlatMap::key_container_type keys;
    typename OriginalFlatMap::mapped_container_type values;
};

void QQmlJSOptimizations::populateReaderLocations()
{
    for (auto writeIt = m_annotations.begin(), writeEnd = m_annotations.end();
         writeIt != writeEnd; ++writeIt) {
        const int writtenRegister = writeIt->second.changedRegisterIndex;

        // Instructions that don't write can't be dead stores, no need to populate reader locations
        if (writtenRegister == InvalidRegister)
            continue;

        RegisterAccess &access = m_readerLocations[writeIt.key()];
        access.trackedRegister = writtenRegister;
        if (writeIt->second.changedRegister.isConversion()) {
            // If it's a conversion, we have to check for all readers of the conversion origins.
            // This happens at jump targets where different types are merged. A StoreReg or similar
            // instruction must be optimized out if none of the types it can hold is read anymore.
            access.trackedTypes.clear();
            const auto origins = writeIt->second.changedRegister.conversionOrigins();
            for (const QQmlJSRegisterContent &origin : origins)
                access.trackedTypes.append(origin.containedType());
        } else {
            access.trackedTypes.append(
                    m_typeResolver->trackedContainedType(writeIt->second.changedRegister));
            Q_ASSERT(!access.trackedTypes.last().isNull());
        }

        auto blockIt = QQmlJSBasicBlocks::basicBlockForInstruction(m_basicBlocks, writeIt.key());
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
            Conversions conversions = block.conversions;

            const auto blockEnd = (nextBlock == m_basicBlocks.end())
                    ? m_annotations.end()
                    : m_annotations.find(nextBlock->first);

            auto blockInstr = isFirstBlock
                    ? (writeIt + 1)
                    : m_annotations.find(currentBlock->first);
            for (; blockInstr != blockEnd; ++blockInstr) {
                if (registerActive
                        && blockInstr->second.typeConversions.contains(writtenRegister)) {
                    conversions.insert(blockInstr.key());
                }

                for (auto readIt = blockInstr->second.readRegisters.constBegin(),
                     end = blockInstr->second.readRegisters.constEnd();
                     readIt != end; ++readIt) {
                    if (!blockInstr->second.isRename) {
                        const QList<QQmlJSRegisterContent> conversionOrigins
                                = readIt->second.content.conversionOrigins();
                        for (const QQmlJSRegisterContent &origin : conversionOrigins) {
                            if (!access.trackedTypes.contains(origin.containedType()))
                                continue;

                            Q_ASSERT(readIt->second.content.isConversion());
                            Q_ASSERT(readIt->second.content.conversionResult());
                            access.typeReaders[blockInstr.key()]
                                    = readIt->second.content.conversionResult();
                            break;
                        }
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
                if (processed == processedBlocks.end()) {
                    blocks.append({conversions, blockStart, registerActive});
                } else if (registerActive && !processed->registerActive) {
                    blocks.append({conversions, blockStart, registerActive});
                } else {
                    Conversions merged = processed->conversions;
                    merged.unite(conversions);

                    if (merged.size() > processed->conversions.size())
                        blocks.append({std::move(merged), blockStart, registerActive});
                }
            };

            if (!currentBlock->second.jumpIsUnconditional && nextBlock != m_basicBlocks.end())
                scheduleBlock(nextBlock->first);

            const int jumpTarget = currentBlock->second.jumpTarget;
            if (jumpTarget != -1)
                scheduleBlock(jumpTarget);

            if (isFirstBlock)
                isFirstBlock = false;
        }
    }
}

bool QQmlJSOptimizations::eraseDeadStore(const InstructionAnnotations::iterator &it,
                                         bool &erasedReaders)
{
    auto reader = m_readerLocations.find(it.key());
    if (reader != m_readerLocations.end()
        && (reader->typeReaders.isEmpty() || reader->registerReadersAndConversions.isEmpty())) {

        if (it->second.isRename) {
            // If it's a rename, it doesn't "own" its output type. The type may
            // still be read elsewhere, even if this register isn't. However, we're
            // not interested in the variant or any other details of the register.
            // Therefore just delete it.
            it->second.changedRegisterIndex = InvalidRegister;
            it->second.changedRegister = QQmlJSRegisterContent();
        } else {
            // void the output, rather than deleting it. We still need its variant.
            const bool adjusted = m_typeResolver->adjustTrackedType(
                    it->second.changedRegister.containedType(), m_typeResolver->voidType());
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
}

void QQmlJSOptimizations::removeDeadStoresUntilStable()
{
    using NewInstructionAnnotations = NewFlatMap<int, InstructionAnnotation>;
    NewInstructionAnnotations newAnnotations;

    bool erasedReaders = true;
    while (erasedReaders) {
        erasedReaders = false;

        for (auto it = m_annotations.begin(), end = m_annotations.end(); it != end; ++it) {
            InstructionAnnotation &instruction = it->second;

            // Don't touch the function prolog instructions
            if (instruction.changedRegisterIndex < InvalidRegister) {
                newAnnotations.appendOrdered(it);
                continue;
            }

            removeReadsFromErasedInstructions(it);

            if (!eraseDeadStore(it, erasedReaders))
                newAnnotations.appendOrdered(it);
        }

        m_annotations = newAnnotations.take();
    }
}

void QQmlJSOptimizations::removeReadsFromErasedInstructions(
        const QFlatMap<int, InstructionAnnotation>::const_iterator &it)
{
    auto readers = m_readerLocations.find(it.key());
    if (readers == m_readerLocations.end())
        return;

    for (auto typeIt = readers->typeReaders.begin(); typeIt != readers->typeReaders.end();) {
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

bool QQmlJSOptimizations::canMove(int instructionOffset,
                                  const QQmlJSOptimizations::RegisterAccess &access) const
{
    if (access.registerReadersAndConversions.size() != 1)
        return false;
    return QQmlJSBasicBlocks::basicBlockForInstruction(m_basicBlocks, instructionOffset)
            == QQmlJSBasicBlocks::basicBlockForInstruction(m_basicBlocks, access.registerReadersAndConversions.begin().key());
}

QList<QQmlJSCompilePass::ObjectOrArrayDefinition>
QQmlJSBasicBlocks::objectAndArrayDefinitions() const
{
    return m_objectAndArrayDefinitions;
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

void QQmlJSOptimizations::adjustTypes()
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

    // Handle the array definitions first.
    // Changing the array type changes the expected element types.
    auto adjustArray = [&](int instructionOffset, int mode) {
        auto it = m_readerLocations.find(instructionOffset);
        if (it == m_readerLocations.end())
            return;

        const InstructionAnnotation &annotation = m_annotations[instructionOffset];
        if (annotation.readRegisters.isEmpty())
            return;

        Q_ASSERT(it->trackedTypes.size() == 1);
        Q_ASSERT(it->trackedTypes[0] == annotation.changedRegister.containedType());

        if (it->trackedTypes[0]->accessSemantics() != QQmlJSScope::AccessSemantics::Sequence)
            return; // Constructed something else.

        if (!m_typeResolver->adjustTrackedType(it->trackedTypes[0], it->typeReaders.values()))
            addError(adjustErrorMessage(it->trackedTypes[0], it->typeReaders.values()));

        // Now we don't adjust the type we store, but rather the type we expect to read. We
        // can do this because we've tracked the read type when we defined the array in
        // QQmlJSTypePropagator.
        if (QQmlJSScope::ConstPtr valueType = it->trackedTypes[0]->valueType()) {
            const QQmlJSRegisterContent content = annotation.readRegisters.begin().value().content;
            const QQmlJSScope::ConstPtr contained = content.containedType();

            // If it's the 1-arg Array ctor, and the argument is a number, that's special.
            if (mode != ObjectOrArrayDefinition::ArrayConstruct1ArgId
                    || !m_typeResolver->equals(contained, m_typeResolver->realType())) {
                if (!m_typeResolver->adjustTrackedType(contained, valueType))
                    addError(adjustErrorMessage(contained, valueType));
            }
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
        QQmlJSScope::ConstPtr resultType = it->trackedTypes[0];

        Q_ASSERT(resultType == annotation.changedRegister.containedType());
        Q_ASSERT(!annotation.readRegisters.isEmpty());

        if (!m_typeResolver->adjustTrackedType(resultType, it->typeReaders.values()))
            addError(adjustErrorMessage(resultType, it->typeReaders.values()));

        if (m_typeResolver->equals(resultType, m_typeResolver->varType())
                || m_typeResolver->equals(resultType, m_typeResolver->variantMapType())) {
            // It's all variant anyway
            return;
        }

        const int classSize = m_jsUnitGenerator->jsClassSize(object.internalClassId);
        Q_ASSERT(object.argc >= classSize);

        for (int i = 0; i < classSize; ++i) {
            // Now we don't adjust the type we store, but rather the types we expect to read. We
            // can do this because we've tracked the read types when we defined the object in
            // QQmlJSTypePropagator.

            const QString propName = m_jsUnitGenerator->jsClassMember(object.internalClassId, i);
            const QQmlJSMetaProperty property = resultType->property(propName);
            if (!property.isValid()) {
                addError(resultType->internalName() + QLatin1String(" has no property called ")
                         + propName);
                continue;
            }
            const QQmlJSScope::ConstPtr propType = property.type();
            if (propType.isNull()) {
                addError(QLatin1String("Cannot resolve type of property ") + propName);
                continue;
            }
            const QQmlJSRegisterContent content = annotation.readRegisters[object.argv + i].content;
            const QQmlJSScope::ConstPtr contained = content.containedType();
            if (!m_typeResolver->adjustTrackedType(contained, propType))
                addError(adjustErrorMessage(contained, propType));
        }

        // The others cannot be adjusted. We don't know their names, yet.
        // But we might still be able to use the variants.
    };

    // Iterate in reverse so that we can have nested lists and objects and the types are propagated
    // from the outer lists/objects to the inner ones.
    for (auto it = m_objectAndArrayDefinitions.crbegin(), end = m_objectAndArrayDefinitions.crend();
         it != end; ++it) {
        switch (it->internalClassId) {
        case ObjectOrArrayDefinition::ArrayClassId:
        case ObjectOrArrayDefinition::ArrayConstruct1ArgId:
            adjustArray(it->instructionOffset, it->internalClassId);
            break;
        default:
            adjustObject(*it);
            break;
        }
    }

    for (auto it = m_readerLocations.begin(), end = m_readerLocations.end(); it != end; ++it) {
        handleRegisterReadersAndConversions(it);

        // There is always one first occurrence of any tracked type. Conversions don't change
        // the type.
        if (it->trackedTypes.size() != 1)
            continue;

        // Don't adjust renamed values. We only adjust the originals.
        const int writeLocation = it.key();
        if (writeLocation >= 0 && m_annotations[writeLocation].isRename)
            continue;

        if (!m_typeResolver->adjustTrackedType(it->trackedTypes[0], it->typeReaders.values()))
            addError(adjustErrorMessage(it->trackedTypes[0], it->typeReaders.values()));
    }


    NewVirtualRegisters newRegisters;
    for (auto i = m_annotations.begin(), iEnd = m_annotations.end(); i != iEnd; ++i) {
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
                    newResult = m_typeResolver->merge(newResult, origin.containedType());
                if (!m_typeResolver->adjustTrackedType(conversionResult, newResult))
                    addError(adjustErrorMessage(conversionResult, newResult));
            }
            newRegisters.appendOrdered(conversion);
        }
        i->second.typeConversions = newRegisters.take();

        for (int movable : std::as_const(movableReads[i.key()]))
            i->second.readRegisters[movable].canMove = true;
    }
}

void QQmlJSOptimizations::populateBasicBlocks()
{
    for (auto blockNext = m_basicBlocks.begin(), blockEnd = m_basicBlocks.end();
         blockNext != blockEnd;) {

        const auto blockIt = blockNext++;
        BasicBlock &block = blockIt->second;
        QList<QQmlJSScope::ConstPtr> writtenTypes;
        QList<int> writtenRegisters;

        const auto instrEnd = (blockNext == blockEnd) ? m_annotations.end()
                                                      : m_annotations.find(blockNext->first);
        for (auto instrIt = m_annotations.find(blockIt->first); instrIt != instrEnd; ++instrIt) {
            const InstructionAnnotation &instruction = instrIt->second;
            for (auto it = instruction.readRegisters.begin(), end = instruction.readRegisters.end();
                 it != end; ++it) {
                Q_ASSERT(instruction.isRename || it->second.content.isConversion());
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

        QQmlJSUtils::deduplicate(block.readRegisters);
    }
}


QT_END_NAMESPACE
