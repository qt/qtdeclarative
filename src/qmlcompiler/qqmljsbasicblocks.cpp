// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsbasicblocks_p.h"
#include "qqmljsutils_p.h"

#include <QtQml/private/qv4instr_moth_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

DEFINE_BOOL_CONFIG_OPTION(qv4DumpBasicBlocks, QV4_DUMP_BASIC_BLOCKS)
DEFINE_BOOL_CONFIG_OPTION(qv4ValidateBasicBlocks, QV4_VALIDATE_BASIC_BLOCKS)

void QQmlJSBasicBlocks::dumpBasicBlocks()
{
    qDebug().noquote() << "=== Basic Blocks for \"%1\""_L1.arg(m_context->name);
    for (const auto &[blockOffset, block] : m_basicBlocks) {
        QDebug debug = qDebug().nospace();
        debug << "Block " << (blockOffset < 0 ? "Function prolog"_L1 : QString::number(blockOffset))
              << ":\n";
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
        debug << "\n  isReturnBlock: " << block.isReturnBlock;
        debug << "\n  isThrowBlock: " << block.isThrowBlock;
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

    QFlatMap<int, BasicBlock> blocks{ m_basicBlocks };
    for (const auto &[blockOffset, block] : blocks) {
        for (int originOffset : block.jumpOrigins) {
            const auto originBlockIt = basicBlockForInstruction(blocks, originOffset);
            const auto isBackEdge = originOffset > blockOffset && originBlockIt->second.jumpIsUnconditional;
            s << "    %1 -> %2%3\n"_L1.arg(QString::number(originBlockIt.key()))
                            .arg(QString::number(blockOffset))
                            .arg(isBackEdge ? " [color=blue]"_L1 : ""_L1);
        }
    }

    for (const auto &[blockOffset, block] : blocks) {
        if (blockOffset < 0) {
            s << "    %1 [shape=record, fontname=\"Monospace\", label=\"Function Prolog\"]\n"_L1
                            .arg(QString::number(blockOffset));
            continue;
        }

        auto nextBlockIt = blocks.lower_bound(blockOffset + 1);
        int nextBlockOffset = nextBlockIt == blocks.end() ? m_context->code.size() : nextBlockIt->first;
        QString dump = QV4::Moth::dumpBytecode(
                m_context->code.constData(), m_context->code.size(), m_context->locals.size(),
                m_context->formals->length(), blockOffset, nextBlockOffset - 1,
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

QQmlJSCompilePass::BlocksAndAnnotations
QQmlJSBasicBlocks::run(const Function *function, QQmlJSAotCompiler::Flags compileFlags,
                       bool &basicBlocksValidationFailed)
{
    basicBlocksValidationFailed = false;

    m_function = function;

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

    // Insert the function prolog block followed by the first "real" block.
    m_basicBlocks.insert_or_assign(m_annotations.begin().key(), BasicBlock());
    BasicBlock zeroBlock;
    zeroBlock.jumpOrigins.append(m_basicBlocks.begin().key());
    m_basicBlocks.insert(0, zeroBlock);

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
            QQmlJSUtils::deduplicate(it->second.jumpOrigins);
    }

    if (compileFlags.testFlag(QQmlJSAotCompiler::ValidateBasicBlocks) || qv4ValidateBasicBlocks()) {
        if (auto validationResult = basicBlocksValidation(); !validationResult.success) {
            qDebug() << "Basic blocks validation failed: %1."_L1.arg(validationResult.errorMessage);
            basicBlocksValidationFailed = true;
        }
    }

    if (qv4DumpBasicBlocks()) {
        dumpBasicBlocks();
        dumpDOTGraph();
    }

    return { std::move(m_basicBlocks), std::move(m_annotations) };
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

void QQmlJSBasicBlocks::generate_IteratorNext(int value, int offset)
{
    Q_UNUSED(value);
    processJump(offset, Conditional);
}

void QQmlJSBasicBlocks::generate_GetOptionalLookup(int index, int offset)
{
    Q_UNUSED(index);
    processJump(offset, Conditional);
}

void QQmlJSBasicBlocks::generate_Ret()
{
    auto currentBlock = basicBlockForInstruction(m_basicBlocks, currentInstructionOffset());
    currentBlock.value().isReturnBlock = true;
    m_skipUntilNextLabel = true;
}

void QQmlJSBasicBlocks::generate_ThrowException()
{
    auto currentBlock = basicBlockForInstruction(m_basicBlocks, currentInstructionOffset());
    currentBlock.value().isThrowBlock = true;
    m_skipUntilNextLabel = true;
}

void QQmlJSBasicBlocks::generate_DefineArray(int argc, int argv)
{
    if (argc == 0)
        return; // empty array/list, nothing to do

    m_objectAndArrayDefinitions.append({
        currentInstructionOffset(), ObjectOrArrayDefinition::ArrayClassId, argc, argv
    });
}

void QQmlJSBasicBlocks::generate_DefineObjectLiteral(int internalClassId, int argc, int argv)
{
    if (argc == 0)
        return;

    m_objectAndArrayDefinitions.append({ currentInstructionOffset(), internalClassId, argc, argv });
}

void QQmlJSBasicBlocks::generate_Construct(int func, int argc, int argv)
{
    Q_UNUSED(func)
    if (argc == 0)
        return; // empty array/list, nothing to do

    m_objectAndArrayDefinitions.append({
        currentInstructionOffset(),
        argc == 1
            ? ObjectOrArrayDefinition::ArrayConstruct1ArgId
            : ObjectOrArrayDefinition::ArrayClassId,
        argc,
        argv
    });
}

void QQmlJSBasicBlocks::processJump(int offset, JumpMode mode)
{
    if (offset < 0)
        m_hadBackJumps = true;
    const int jumpTarget = absoluteOffset(offset);
    Q_ASSERT(!m_basicBlocks.isEmpty());
    auto currentBlock = basicBlockForInstruction(m_basicBlocks, currentInstructionOffset());
    currentBlock->second.jumpTarget = jumpTarget;
    currentBlock->second.jumpIsUnconditional = (mode == Unconditional);
    m_basicBlocks[jumpTarget].jumpOrigins.append(currentInstructionOffset());
    if (mode == Unconditional)
        m_skipUntilNextLabel = true;
    else
        m_basicBlocks.insert(nextInstructionOffset(), BasicBlock());
}

QQmlJSCompilePass::BasicBlocks::iterator QQmlJSBasicBlocks::basicBlockForInstruction(
        QFlatMap<int, BasicBlock> &container, int instructionOffset)
{
    auto block = container.lower_bound(instructionOffset);
    if (block == container.end() || block->first != instructionOffset)
        --block;
    return block;
}

QQmlJSCompilePass::BasicBlocks::const_iterator QQmlJSBasicBlocks::basicBlockForInstruction(
        const BasicBlocks &container, int instructionOffset)
{
    return basicBlockForInstruction(const_cast<BasicBlocks &>(container), instructionOffset);
}

QQmlJSBasicBlocks::BasicBlocksValidationResult QQmlJSBasicBlocks::basicBlocksValidation()
{
    if (m_basicBlocks.empty())
        return {};

    const QFlatMap<int, BasicBlock> blocks{ m_basicBlocks };
    QList<QFlatMap<int, BasicBlock>::const_iterator> returnOrThrowBlocks;
    for (auto it = blocks.cbegin(); it != blocks.cend(); ++it) {
        if (it.value().isReturnBlock || it.value().isThrowBlock)
            returnOrThrowBlocks.append(it);
    }

    // 1. Return blocks and throw blocks must not have a jump target
    for (const auto &it : returnOrThrowBlocks) {
        if (it.value().jumpTarget != -1)
            return { false, "Return or throw block jumps to somewhere"_L1 };
    }

    // 2. The basic blocks graph must be connected.
    QSet<int> visitedBlockOffsets;
    QList<QFlatMap<int, BasicBlock>::const_iterator> toVisit;
    toVisit.append(returnOrThrowBlocks);

    while (!toVisit.empty()) {
        const auto &[offset, block] = *toVisit.takeLast();
        visitedBlockOffsets.insert(offset);
        for (int originOffset : block.jumpOrigins) {
            const auto originBlock = basicBlockForInstruction(blocks, originOffset);
            if (visitedBlockOffsets.find(originBlock.key()) == visitedBlockOffsets.end()
                && !toVisit.contains(originBlock))
                toVisit.append(originBlock);
        }
    }

    if (visitedBlockOffsets.size() != blocks.size())
        return { false, "Basic blocks graph is not fully connected"_L1 };

    // 3. A block's jump target must be the first offset of a block.
    for (const auto &[blockOffset, block] : blocks) {
        auto target = block.jumpTarget;
        if (target != -1 && blocks.find(target) == blocks.end())
            return { false, "Invalid jump; target is not the start of a block"_L1 };
    }

    return {};
}

QT_END_NAMESPACE
