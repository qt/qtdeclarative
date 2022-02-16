/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "qqmljsbasicblocks_p.h"

QT_BEGIN_NAMESPACE

template<typename Container>
void deduplicate(Container &container)
{
    std::sort(container.begin(), container.end());
    auto erase = std::unique(container.begin(), container.end());
    container.erase(erase, container.end());
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

QQmlJSCompilePass::InstructionAnnotations QQmlJSBasicBlocks::run(
        const Function *function,
        const InstructionAnnotations &annotations)
{
    m_annotations = annotations;
    m_basicBlocks.insert_or_assign(0, BasicBlock());

    const QByteArray byteCode = function->code;
    decode(byteCode.constData(), static_cast<uint>(byteCode.length()));
    if (m_hadBackJumps) {
        // We may have missed some connections between basic blocks if there were back jumps.
        // Fill them in via a second pass.
        reset();
        decode(byteCode.constData(), static_cast<uint>(byteCode.length()));
        for (auto it = m_basicBlocks.begin(), end = m_basicBlocks.end(); it != end; ++it)
            deduplicate(it->second.jumpOrigins);
    }

    populateBasicBlocks();
    populateReaderLocations();
    adjustTypes();
    return std::move(m_annotations);
}

QV4::Moth::ByteCodeHandler::Verdict QQmlJSBasicBlocks::startInstruction(QV4::Moth::Instr::Type type)
{
    auto it = m_basicBlocks.find(currentInstructionOffset());
    if (it != m_basicBlocks.end()) {
        if (!m_skipUntilNextLabel && it != m_basicBlocks.begin())
            it->second.jumpOrigins.append((--it)->first);
        m_skipUntilNextLabel = false;
    } else if (m_skipUntilNextLabel && !instructionManipulatesContext(type)) {
        return SkipInstruction;
    }

    return ProcessInstruction;
}

void QQmlJSBasicBlocks::endInstruction(QV4::Moth::Instr::Type)
{
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
        m_basicBlocks[nextInstructionOffset()].jumpOrigins.append(currentInstructionOffset());
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

void QQmlJSBasicBlocks::populateReaderLocations()
{
    using NewInstructionAnnotations = NewFlatMap<int, InstructionAnnotation>;

    bool erasedReaders = false;
    auto eraseDeadStore = [&](const InstructionAnnotations::iterator &it) {
        auto reader = m_readerLocations.find(it.key());
        if (reader != m_readerLocations.end() && reader->readers.isEmpty()) {

            // void the output, rather than deleting it. We still need its variant.
            m_typeResolver->adjustTrackedType(
                        it->second.changedRegister.storedType(),
                        m_typeResolver->voidType());
            m_typeResolver->adjustTrackedType(
                        m_typeResolver->containedType(it->second.changedRegister),
                        m_typeResolver->voidType());
            m_readerLocations.erase(reader);

            // If it's not a label and has no side effects, we can drop the instruction.
            if (!it->second.hasSideEffects && m_basicBlocks.find(it.key()) == m_basicBlocks.end()) {
                if (!it->second.readRegisters.isEmpty()) {
                    it->second.readRegisters.clear();
                    erasedReaders = true;
                }
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

        QList<int> blocks = { blockIt->first };
        QList<int> processedBlocks;
        bool isFirstBlock = true;

        while (!blocks.isEmpty()) {
            auto nextBlock = m_basicBlocks.find(blocks.takeLast());
            auto currentBlock = nextBlock++;

            if (isFirstBlock
                    || containsAny(currentBlock->second.readRegisters, access.trackedTypes)) {
                const auto blockEnd = (nextBlock == m_basicBlocks.end())
                        ? m_annotations.end()
                        : m_annotations.find(nextBlock->first);

                auto blockInstr = isFirstBlock
                        ? (writeIt + 1)
                        : m_annotations.find(currentBlock->first);
                for (; blockInstr != blockEnd; ++blockInstr) {
                    for (auto readIt = blockInstr->second.readRegisters.constBegin(),
                         end = blockInstr->second.readRegisters.constEnd();
                         readIt != end; ++readIt) {
                        Q_ASSERT(readIt->second.isConversion());
                        if (containsAny(readIt->second.conversionOrigins(), access.trackedTypes))
                            access.readers[blockInstr.key()] = readIt->second.conversionResult();
                    }
                }
            }

            if (!currentBlock->second.jumpIsUnconditional && nextBlock != m_basicBlocks.end()
                    && !processedBlocks.contains(nextBlock->first)) {
                blocks.append(nextBlock->first);
            }

            const int jumpTarget = currentBlock->second.jumpTarget;
            if (jumpTarget != -1 && !processedBlocks.contains(jumpTarget))
                blocks.append(jumpTarget);

            // We can re-enter the first block from the beginning.
            // We will then find any reads before the write we're currently examining.
            if (isFirstBlock)
                isFirstBlock = false;
            else
                processedBlocks.append(currentBlock->first);
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
                for (auto it = readers->readers.begin(); it != readers->readers.end();) {
                    if (m_annotations.contains(it.key()))
                        ++it;
                    else
                        it = readers->readers.erase(it);
                }
            }

            if (!eraseDeadStore(it))
                newAnnotations.appendOrdered(it);
        }

        m_annotations = newAnnotations.take();
    }
}

void QQmlJSBasicBlocks::adjustTypes()
{
    using NewVirtualRegisters = NewFlatMap<int, QQmlJSRegisterContent>;

    for (auto it = m_readerLocations.begin(), end = m_readerLocations.end(); it != end; ++it) {
        // There is always one first occurrence of any tracked type. Conversions don't change
        // the type.
        if (it->trackedTypes.length() != 1)
            continue;

        m_typeResolver->adjustTrackedType(it->trackedTypes[0], it->readers.values());
    }

    const auto transformRegister = [&](QQmlJSRegisterContent &content) {
        m_typeResolver->adjustTrackedType(
                    content.storedType(),
                    m_typeResolver->storedType(m_typeResolver->containedType(content)));
    };

    NewVirtualRegisters newRegisters;
    for (auto i = m_annotations.begin(), iEnd = m_annotations.end(); i != iEnd; ++i) {
        if (i->second.changedRegisterIndex != InvalidRegister)
            transformRegister(i->second.changedRegister);
        for (auto content : i->second.typeConversions) {
            QQmlJSScope::ConstPtr conversionResult = content.second.conversionResult();
            const auto conversionOrigins = content.second.conversionOrigins();
            QQmlJSScope::ConstPtr newResult;
            for (const auto &origin : conversionOrigins)
                newResult = m_typeResolver->merge(newResult, origin);
            m_typeResolver->adjustTrackedType(conversionResult, newResult);
            transformRegister(content.second);
        }
        i->second.typeConversions = newRegisters.take();
    }
}

void QQmlJSBasicBlocks::populateBasicBlocks()
{
    for (auto blockNext = m_basicBlocks.begin(), blockEnd = m_basicBlocks.end();
         blockNext != blockEnd;) {

        const auto blockIt = blockNext++;
        BasicBlock &block = blockIt->second;
        QList<QQmlJSScope::ConstPtr> writtenRegisters;

        const auto instrEnd = (blockNext == blockEnd)
                ? m_annotations.end()
                : m_annotations.find(blockNext->first);
        for (auto instrIt = m_annotations.find(blockIt->first); instrIt != instrEnd; ++instrIt) {
            const InstructionAnnotation &instruction = instrIt->second;
            for (auto it = instruction.readRegisters.begin(), end = instruction.readRegisters.end();
                 it != end; ++it) {
                Q_ASSERT(it->second.isConversion());
                for (const QQmlJSScope::ConstPtr &origin : it->second.conversionOrigins()) {
                    if (!writtenRegisters.contains(origin))
                        block.readRegisters.append(origin);
                }
            }

            // If it's just a renaming, the type has existed in a different register before.
            if (instruction.changedRegisterIndex != InvalidRegister && !instruction.isRename) {
                writtenRegisters.append(m_typeResolver->trackedContainedType(
                                            instruction.changedRegister));
            }
        }

        deduplicate(block.readRegisters);
    }
}

QT_END_NAMESPACE
