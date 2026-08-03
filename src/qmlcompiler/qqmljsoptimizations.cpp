// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljsoptimizations_p.h"
#include "qqmljsbasicblocks_p.h"
#include "qqmljsutils_p.h"

#include <QtCore/qhash.h>
#include <QtCore/qset.h>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

QQmlJSCompilePass::BlocksAndAnnotations QQmlJSOptimizations::run(const Function *function)
{
    m_function = function;

    populateBasicBlocks();
    populateReaderLocationsTrackedTypes();
    populateReaderLocationsReadersAndConversions();
    removeDeadStoresUntilStable();
    adjustTypes();

    return { std::move(m_basicBlocks), std::move(m_annotations) };
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

void QQmlJSOptimizations::populateReaderLocationsTrackedTypes()
{
    for (const auto &[offset, annotation] : m_annotations) {
        const int writtenRegister = annotation.changedRegisterIndex;

        // Instructions that don't write can't be dead stores, no need to populate reader locations
        if (writtenRegister == InvalidRegister)
            continue;

        RegisterAccess &access = m_readerLocations[offset];
        access.trackedRegister = writtenRegister;
        if (annotation.changedRegister.isConversion()) {
            // If it's a conversion, we have to check for all readers of the conversion origins.
            // This happens at jump targets where different types are merged. A StoreReg or similar
            // instruction must be optimized out if none of the types it can hold is read anymore.
            const auto &origins = annotation.changedRegister.conversionOrigins();
            for (QQmlJSRegisterContent origin : origins)
                access.trackedTypes.append(origin);
        } else {
            access.trackedTypes.append(annotation.changedRegister);
            Q_ASSERT(!access.trackedTypes.last().isNull());
        }
    }
}

// Every write that currently reaches this program point for this register, and the
// type-conversion instructions crossed since each one specifically. Almost always a
// single entry; more than one only at a genuine merge of differing reaching writes.
using RegisterState = QHash<int, QQmlJSOptimizations::Conversions>;

struct BlockState
{
    QHash<QQmlJSRegisterContent, QSet<int>> availableContent; // content -> writers reaching here
    QHash<int, RegisterState> registers; // register index -> reaching state
};

static bool mergeBlockState(BlockState &to, const BlockState &from)
{
    bool changed = false;

    for (auto contentIt = from.availableContent.constBegin(),
         contentEnd = from.availableContent.constEnd(); contentIt != contentEnd; ++contentIt) {
        QSet<int> &target = to.availableContent[contentIt.key()];
        const qsizetype before = target.size();
        target.unite(contentIt.value());
        if (target.size() > before)
            changed = true;
    }

    for (const auto &[index, registerState] : from.registers.asKeyValueRange()) {
        RegisterState &target = to.registers[index];
        for (const auto &[writeOffset, conversions] : registerState.asKeyValueRange()) {
            const auto existing = target.find(writeOffset);
            if (existing == target.end()) {
                target.insert(writeOffset, conversions);
                changed = true;
                continue;
            }
            const qsizetype before = existing.value().size();
            existing.value().unite(conversions);
            if (existing.value().size() > before)
                changed = true;
        }
    }

    return changed;
}

void QQmlJSOptimizations::populateReaderLocationsReadersAndConversions()
{
    QHash<int, BlockState> entryState; // block start -> state at block entry
    std::vector<int> pending;
    pending.reserve(m_basicBlocks.size());
    for (const auto &[blockStart, block] : m_basicBlocks)
        pending.push_back(blockStart);

    while (!pending.empty()) {
        const int blockStart = pending.back();
        pending.pop_back();

        const auto blockIt = m_basicBlocks.find(blockStart);
        auto nextBlockIt = blockIt;
        ++nextBlockIt;

        BlockState state = entryState.value(blockStart);

        auto instrIt = m_annotations.find(blockStart);
        const auto blockEnd = (nextBlockIt == m_basicBlocks.end())
                ? m_annotations.end()
                : m_annotations.find(nextBlockIt->first);

        for (; instrIt != blockEnd; ++instrIt) {
            const int key = instrIt.key();
            if (!instrIt->second.isRename) {
                for (const auto &read : instrIt->second.readRegisters.values()) {
                    const QQmlJSRegisterContent &content = read.content;
                    const auto recordRead = [&](const QQmlJSRegisterContent &r) {
                        const auto found = state.availableContent.constFind(r);
                        if (found != state.availableContent.constEnd()) {
                            for (int writerKey : found.value())
                                m_readerLocations[writerKey].typeReaders[key] = content;
                        }
                    };

                    if (content.isConversion()) {
                        Q_ASSERT(content.conversionResultType());
                        for (QQmlJSRegisterContent origin : content.conversionOrigins())
                            recordRead(origin);
                    } else {
                        recordRead(content);
                    }
                }
            }

            for (const int convRegister : instrIt->second.typeConversions.keys()) {
                const auto tracked = state.registers.find(convRegister);
                if (tracked != state.registers.end()) {
                    for (auto writerIt = tracked.value().begin(), writerEnd = tracked.value().end();
                         writerIt != writerEnd; ++writerIt) {
                        writerIt.value().insert(key);
                    }
                }
            }

            for (const int readRegister : instrIt->second.readRegisters.keys()) {
                const auto tracked = state.registers.constFind(readRegister);
                if (tracked != state.registers.constEnd()) {
                    for (const auto &[writerKey, value] : tracked.value().asKeyValueRange())
                        m_readerLocations[writerKey].registerReadersAndConversions[key] = value;
                }
            }

            // Record this instruction's write, once done reading above.
            if (instrIt->second.changedRegisterIndex != InvalidRegister) {
                const auto access = m_readerLocations.constFind(key);
                if (access != m_readerLocations.constEnd()) {
                    for (const QQmlJSRegisterContent &tracked : std::as_const(access->trackedTypes))
                        state.availableContent[tracked].insert(key);
                }

                // A write unconditionally shadows whatever reached this register before it
                RegisterState fresh;
                fresh.insert(key, Conversions{});
                state.registers[instrIt->second.changedRegisterIndex] = std::move(fresh);
            }
        }

        auto scheduleSuccessor = [&](int successorStart) {
            if (mergeBlockState(entryState[successorStart], state))
                pending.push_back(successorStart);
        };

        if (!blockIt->second.jumpIsUnconditional && nextBlockIt != m_basicBlocks.end())
            scheduleSuccessor(nextBlockIt->first);

        const int jumpTarget = blockIt->second.jumpTarget;
        if (jumpTarget != -1)
            scheduleSuccessor(jumpTarget);
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
            // We can't do this with certain QObjects because they still need tracking as
            // implicitly destructible by the garbage collector. We may be calling a factory
            // function and then forgetting the object after all.
            //
            // However, objects we need to track that way can only be produced through external
            // side effects (i.e. function calls).

            const QQmlJSScope::ConstPtr contained = it->second.changedRegister.containedType();
            if (!it->second.hasExternalSideEffects
                    || (!contained->isReferenceType()
                        && !m_typeResolver->canHold(contained, m_typeResolver->qObjectType()))) {
                // void the output, rather than deleting it. We still need its variant.
                const bool adjusted = m_typeResolver->adjustTrackedType(
                        it->second.changedRegister, m_typeResolver->voidType());
                Q_ASSERT(adjusted); // Can always convert to void
            }
        }
        m_readerLocations.erase(reader);

        // If it's not a label and has no side effects, we can drop the instruction.
        if (!it->second.hasInternalSideEffects) {
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
    if (access.typeReaders.size() != 1)
        return false;
    return QQmlJSBasicBlocks::constBasicBlockForInstruction(m_basicBlocks, instructionOffset)
            == QQmlJSBasicBlocks::constBasicBlockForInstruction(m_basicBlocks, access.typeReaders.begin().key());
}

QList<QQmlJSCompilePass::ObjectOrArrayDefinition>
QQmlJSBasicBlocks::objectAndArrayDefinitions() const
{
    return m_objectAndArrayDefinitions;
}

static QString adjustErrorMessage(
        QQmlJSRegisterContent origin, const QQmlJSScope::ConstPtr &conversion) {
    return QLatin1String("Cannot convert from ")
            + origin.containedType()->internalName() + QLatin1String(" to ")
            + conversion->internalName();
}

static QString adjustErrorMessage(
        QQmlJSRegisterContent origin, QQmlJSRegisterContent conversion) {
    return adjustErrorMessage(origin, conversion.containedType());
}

static QString adjustErrorMessage(
        QQmlJSRegisterContent origin, const QList<QQmlJSRegisterContent> &conversions) {
    if (conversions.size() == 1)
        return adjustErrorMessage(origin, conversions[0]);

    QString types;
    for (QQmlJSRegisterContent type : conversions) {
        if (!types.isEmpty())
            types += QLatin1String(", ");
        types += type.containedType()->internalName();
    }
    return QLatin1String("Cannot convert from ")
            + origin.containedType()->internalName() + QLatin1String(" to union of ") + types;
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
        auto it = m_readerLocations.constFind(instructionOffset);
        if (it == m_readerLocations.cend())
            return;

        const InstructionAnnotation &annotation = m_annotations[instructionOffset];
        if (annotation.readRegisters.isEmpty())
            return;

        Q_ASSERT(it->trackedTypes.size() == 1);
        Q_ASSERT(it->trackedTypes[0] == annotation.changedRegister);

        if (it->trackedTypes[0].containedType()->accessSemantics()
                != QQmlJSScope::AccessSemantics::Sequence) {
            return; // Constructed something else.
        }

        if (!m_typeResolver->adjustTrackedType(it->trackedTypes[0], it->typeReaders.values()))
            addError(adjustErrorMessage(it->trackedTypes[0], it->typeReaders.values()));

        // Now we don't adjust the type we store, but rather the type we expect to read. We
        // can do this because we've tracked the read type when we defined the array in
        // QQmlJSTypePropagator.
        if (const QQmlJSScope::ConstPtr elementType
                = it->trackedTypes[0].containedType()->elementType()) {
            const auto adjust = [&](const auto it) {
                const QQmlJSRegisterContent content = it.value().content;
                const QQmlJSScope::ConstPtr contained = content.containedType();
                if (!m_typeResolver->adjustTrackedType(content, elementType)) {
                    addError(adjustErrorMessage(content, elementType));
                    return false;
                }
                return true;
            };

            const auto &readRegisters = annotation.readRegisters;
            if (mode == ObjectOrArrayDefinition::ArrayConstruct1ArgId) {
                Q_ASSERT(readRegisters.size() == 1);
                const auto it = readRegisters.cbegin();
                if (it.value().content.containedType() != m_typeResolver->realType())
                    adjust(it);
            } else {
                for (auto it = readRegisters.cbegin(); it != readRegisters.cend(); ++it) {
                    if (!adjust(it))
                        break;
                }
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
        const QQmlJSRegisterContent resultType = it->trackedTypes[0];

        Q_ASSERT(resultType == annotation.changedRegister);
        Q_ASSERT(!annotation.readRegisters.isEmpty());

        if (!m_typeResolver->adjustTrackedType(resultType, it->typeReaders.values()))
            addError(adjustErrorMessage(resultType, it->typeReaders.values()));

        m_readerLocations.erase(it);

        if (resultType.contains(m_typeResolver->varType())
                || resultType.contains(m_typeResolver->variantMapType())
                || resultType.contains(m_typeResolver->jsValueType())) {
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
            const QQmlJSMetaProperty property = resultType.containedType()->property(propName);
            if (!property.isValid()) {
                addError(resultType.containedType()->internalName()
                         + QLatin1String(" has no property called ") + propName);
                continue;
            }
            const QQmlJSScope::ConstPtr propType = property.type();
            if (propType.isNull()) {
                addError(QLatin1String("Cannot resolve type of property ") + propName);
                continue;
            }
            const QQmlJSRegisterContent content = annotation.readRegisters[object.argv + i].content;
            if (!m_typeResolver->adjustTrackedType(content, propType))
                addError(adjustErrorMessage(content, propType));
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

    for (auto it = m_readerLocations.cbegin(), end = m_readerLocations.cend(); it != end; ++it) {
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
            if (content.isConversion() && !content.original().isValid()) {
                const auto &conversionOrigins = content.conversionOrigins();
                for (const auto &origin : conversionOrigins)
                    newResult = m_typeResolver->merge(newResult, origin.containedType());
                if (!m_typeResolver->adjustTrackedType(content, newResult))
                    addError(adjustErrorMessage(content, newResult));
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
                if (!writtenRegisters.contains(it->first))
                    block.readRegisters.append(it->first);
            }

            // If it's just a renaming, the type has existed in a different register before.
            if (instruction.changedRegisterIndex != InvalidRegister) {
                if (!instruction.isRename)
                    writtenTypes.append(instruction.changedRegister.containedType());
                writtenRegisters.append(instruction.changedRegisterIndex);
            }
        }

        QQmlJSUtils::deduplicate(block.readRegisters);
    }
}


QT_END_NAMESPACE
