/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QtCore/qloggingcategory.h>

#include "qv4schedulers_p.h"
#include "qv4util_p.h"
#include "qv4graph_p.h"
#include "qv4blockscheduler_p.h"
#include "qv4stackframe_p.h"

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace IR {

Q_LOGGING_CATEGORY(lcSched, "qt.v4.ir.scheduling")
Q_LOGGING_CATEGORY(lcDotCFG, "qt.v4.ir.scheduling.cfg")

static bool needsScheduling(Node *n)
{
    if (n->operation()->isConstant())
        return false;
    switch (n->opcode()) {
    case Meta::Function: Q_FALLTHROUGH();
    case Meta::CppFrame:
    case Meta::Phi:
    case Meta::EffectPhi:
        return false;
    default:
        return true;
    }
}

bool NodeScheduler::canStartBlock(Node *node) const
{
    switch (node->operation()->kind()) {
    case Meta::Start: Q_FALLTHROUGH();
    case Meta::IfTrue:
    case Meta::IfFalse:
    case Meta::Region:
    case Meta::HandleUnwind:
    case Meta::OnException:
        return true;

    default:
        return false;
    }
}

bool NodeScheduler::isControlFlowSplit(Node *node) const
{
    int nOutputs = node->operation()->controlOutputCount();
    if (nOutputs == 2) {
        // if there is a "missing" control output, it's for exception flow without unwinder
        int controlUses = 0;
        auto uses = node->uses();
        for (auto it = uses.begin(), eit = uses.end(); it != eit; ++it) {
            if (isLive(*it) && it.isUsedAsControl())
                ++controlUses;
        }
        return controlUses == 2;
    }
    return nOutputs > 2;
}

bool NodeScheduler::isBlockTerminator(Node *node) const
{
    switch (node->operation()->kind()) {
    case Meta::Branch: Q_FALLTHROUGH();
    case Meta::Jump:
    case Meta::Return:
    case Meta::TailCall:
    case Meta::UnwindDispatch:
    case Meta::End:
        return true;
    case Meta::Call:
        return isControlFlowSplit(node);
    default:
        return false;
    }
}

MIBlock *NodeScheduler::getCommonDominator(MIBlock *one, MIBlock *other) const
{
    MIBlock::Index a = one->index();
    MIBlock::Index b = other->index();

    while (a != b) {
        if (m_dominatorDepthForBlock[a] < m_dominatorDepthForBlock[b])
            b = m_domTree->immediateDominator(b);
        else
            a = m_domTree->immediateDominator(a);
    }

    return m_miFunction->block(a);
}

// For Nodes that end up inside loops, it'd be great if we can move (hoist) them out of the loop.
// To do that, we need a block that preceeds the loop. (So the block before the loop header.)
// This function calculates that hoist block if the original block is in a loop.
MIBlock *NodeScheduler::getHoistBlock(MIBlock *block) const
{
    if (m_loopInfo->isLoopHeader(block))
        return m_miFunction->block(m_domTree->immediateDominator(block->index()));

    // make the loop header a candidate:
    MIBlock *loopHeader = m_loopInfo->loopHeaderFor(block);
    if (loopHeader == nullptr)
        return nullptr; // block is not in a loop

    // And now the tricky part: block has to dominate all exits from the loop. If it does not do
    // that, it meanse that there is an exit from the loop that can be reached before block. In
    // that case, hoisting from "block" to "loopHeader" would mean there now is an extra calculation
    // that is not needed for a certain loop exit.
    for (MIBlock *outEdge : m_loopInfo->loopExitsForLoop(loopHeader)) {
        if (getCommonDominator(block, outEdge) != block)
            return nullptr;
    }

    return m_miFunction->block(m_domTree->immediateDominator(loopHeader->index()));
}

NodeScheduler::NodeScheduler(Function *irFunction)
    : m_irFunction(irFunction)
    , m_vregs(irFunction->graph()->nodeCount(), std::numeric_limits<unsigned>::max())
    , m_live(irFunction->graph(), /*collectUses =*/ false /* do explicitly NOT collect uses! */)
{
}

MIFunction *NodeScheduler::buildMIFunction()
{
    m_miFunction = new MIFunction(m_irFunction);

    // step 1: build the CFG
    auto roots = buildCFG();
    m_miFunction->renumberBlocks();
    m_miFunction->dump(QStringLiteral("CFG after renumbering"));

    Q_ASSERT(m_miFunction->block(MIFunction::StartBlockIndex)->index()
             == MIFunction::StartBlockIndex);
    Q_ASSERT(m_miFunction->block(MIFunction::StartBlockIndex)->instructions().front().opcode()
             == Meta::Start);

    // step 2: build the dominator tree
    if (lcDotCFG().isDebugEnabled())
        dumpDotCFG();
    m_domTree.reset(new DominatorTree(m_miFunction));
    m_dominatorDepthForBlock = m_domTree->calculateNodeDepths();

    // step 3: find loops
    m_loopInfo.reset(new LoopInfo(*m_domTree.data()));
    m_loopInfo->detectLoops();

    // step 4: schedule early
    scheduleEarly(roots);
    showNodesByBlock(QStringLiteral("nodes per block after early scheduling"));

    // step 5: schedule late
    scheduleLate(roots);
    showNodesByBlock(QStringLiteral("nodes per block after late scheduling"));

    // step 6: schedule instructions in each block
    scheduleNodesInBlock();

    m_miFunction->dump(QStringLiteral("MI before block scheduling"));

    // step 7: order the basic blocks in the CFG
    BlockScheduler blockScheduler(*m_domTree.data(), *m_loopInfo.data());
    m_miFunction->setBlockOrder(blockScheduler.scheduledBlockSequence());

    // we're done
    m_miFunction->renumberInstructions();
    m_miFunction->setVregCount(m_nextVReg);
    m_miFunction->dump(QStringLiteral("MI after scheduling"));
    return m_miFunction;
}

static Node *splitEdge(Function *irFunction, Node *node, unsigned inputIndex)
{
    Graph *g = irFunction->graph();
    Node *in = node->input(inputIndex);
    Node *region = g->createNode(g->opBuilder()->getRegion(1), &in, 1);
    Node *jump = g->createNode(g->opBuilder()->get<Meta::Jump>(), &region, 1);

    qCDebug(lcSched) << "splitting critical edge from node" << node->id()
                     << "to node" << node->input(inputIndex)->id()
                     << "by inserting jump node" << jump->id()
                     << "and region node" << region->id();

    node->replaceInput(inputIndex, jump);
    return jump;
}

// See Chapter 6.3.1 of https://scholarship.rice.edu/bitstream/handle/1911/96451/TR95-252.pdf for
// a description of the algorithm.
std::vector<Node *> NodeScheduler::buildCFG()
{
    std::vector<Node *> roots;
    roots.reserve(32);
    NodeWorkList todo(m_irFunction->graph());

    auto enqueueControlInputs = [this, &todo](Node *node) {
        for (unsigned i = 0, ei = node->operation()->controlInputCount(); i != ei; ++i) {
            const auto inputIndex = node->operation()->indexOfFirstControl() + i;
            Node *input = node->input(inputIndex);
            Q_ASSERT(input);
            if (node->operation()->kind() == Meta::Region
                    && node->operation()->controlInputCount() > 1
                    && isControlFlowSplit(input)) {
                // critical edge!
                input = splitEdge(m_irFunction, node, inputIndex);
                m_live.markReachable(input);
                m_live.markReachable(input->controlInput(0));
            }
            if (!isBlockTerminator(input)) {
                auto g = m_irFunction->graph();
                Node *jump = g->createNode(g->opBuilder()->get<Meta::Jump>(), &input, 1);
                node->replaceInput(inputIndex, jump);
                m_live.markReachable(jump);
                qCDebug(lcSched) << "inserting jump node" << jump->id()
                                 << "between node" << node->id()
                                 << "and node" << input->id();
                input = jump;
            }
            todo.enqueue(input);
        }
    };

    // create the CFG by scheduling control dependencies that start/end blocks:
    todo.enqueue(m_irFunction->graph()->endNode());
    while (Node *node = todo.dequeueNextNodeForVisiting()) {
        Q_ASSERT(isBlockTerminator(node));

        if (schedulerData(node)->minimumBlock)
            continue;

        MIBlock *b = m_miFunction->addBlock();

        qCDebug(lcSched) << "scheduling node" << node->id() << "as terminator for new block"
                         << b->index();
        b->instructions().push_front(createMIInstruction(node));
        placeFixed(node, b, Schedule);
        roots.push_back(node);

        if (Node *framestate = node->frameStateInput()) {
            placeFixed(framestate, b, DontSchedule);
            qCDebug(lcSched) << ".. also scheduling framestate dependency node" << node->id()
                             << "in block" << b->index();
        }

        if (node->opcode() == Meta::End) {
            enqueueControlInputs(node);
            continue;
        }

        while (true) {
            Node *controlDependency = node->controlInput(0);
            if (!controlDependency)
                break;
            if (todo.isVisited(controlDependency))
                break;
            if (schedulerData(controlDependency)->isFixed)
                break;

            if (controlDependency->opcode() == Meta::Start) {
                qCDebug(lcSched) << "placing start node" << controlDependency->id()
                                 << "in block" << b->index();
                handleStartNode(controlDependency, b);
                placeFixed(controlDependency, b, Schedule);
                roots.push_back(controlDependency);
                break; // we're done with this block
            }
            if (isBlockTerminator(controlDependency)) {
                qCDebug(lcSched) << "found terminator node" << controlDependency->id()
                                 << "for another block, so finish block" << b->index();
                Node *merge = m_irFunction->graph()->createNode(
                        m_irFunction->graph()->opBuilder()->getRegion(1), &controlDependency, 1);
                node->replaceInput(node->operation()->indexOfFirstControl(), merge);
                addBlockStart(roots, merge, b);
                placeFixed(merge, b, Schedule);
                m_live.markReachable(merge);
                todo.enqueue(controlDependency);
                break; // we're done with this block
            }
            if (canStartBlock(controlDependency)
                    || schedulerData(controlDependency->controlInput())->isFixed) {
                qCDebug(lcSched) << "found block start node" << controlDependency->id()
                                 << "for this block, so finish block" << b->index();
                addBlockStart(roots, controlDependency, b);
                placeFixed(controlDependency, b, Schedule);
                roots.push_back(controlDependency);
                enqueueControlInputs(controlDependency);
                break; // we're done with this block
            }
            qCDebug(lcSched) << "skipping node" << controlDependency->id();
            node = controlDependency;
        }
    }

    // link the edges of the MIBlocks, and add basic-block arguments:
    for (MIBlock *toBlock : m_miFunction->blocks()) {
        Q_ASSERT(!toBlock->instructions().empty());
        MIInstr &instr = toBlock->instructions().front();
        Node *toNode = instr.irNode();
        const auto opcode = toNode->operation()->kind();
        if (opcode == Meta::Region) {
            unsigned inputNr = 0;
            for (Node *input : toNode->inputs()) {
                MIBlock *fromBlock = schedulerData(input)->minimumBlock;
                fromBlock->addOutEdge(toBlock);
                toBlock->addInEdge(fromBlock);
                MIInstr &fromTerminator = fromBlock->instructions().back();
                if (fromTerminator.irNode()->opcode() == Meta::Jump ||
                        fromTerminator.irNode()->opcode() == Meta::UnwindDispatch) {
                    unsigned arg = 0;
                    for (const MIOperand &bbArg : toBlock->arguments()) {
                        fromTerminator.setOperand(arg++,
                                                  createMIOperand(bbArg.irNode()->input(inputNr)));
                    }
                }
                ++inputNr;
            }
        } else if (opcode == Meta::End) {
            for (Node *input : toNode->inputs()) {
                MIBlock *fromBlock = schedulerData(input)->minimumBlock;
                fromBlock->addOutEdge(toBlock);
                toBlock->addInEdge(fromBlock);
            }
        } else if (Node *fromNode = toNode->controlInput()) {
            MIBlock *fromBlock = schedulerData(fromNode)->minimumBlock;
            fromBlock->addOutEdge(toBlock);
            toBlock->addInEdge(fromBlock);
        }
    }

    m_irFunction->dump(QStringLiteral("graph after building CFG"));

    auto startBlock = schedulerData(m_irFunction->graph()->startNode())->minimumBlock;
    m_miFunction->setStartBlock(startBlock);

    if (lcSched().isDebugEnabled())
        m_miFunction->dump(QStringLiteral("control flow graph before renumbering"));
    m_miFunction->verifyCFG();

    return roots;
}

// See Chapter 6.3.3 of https://scholarship.rice.edu/bitstream/handle/1911/96451/TR95-252.pdf for
// a description of the algorithm.
void NodeScheduler::scheduleEarly(const std::vector<Node *> &roots)
{
    // scheduling one node might have the effect of queueing its dependencies
    NodeWorkList todo(m_irFunction->graph());
    for (Node *root : roots) {
        todo.enqueue(root);
        while (Node *node = todo.dequeueNextNodeForVisiting())
            scheduleEarly(node, todo);
    }
}

void NodeScheduler::scheduleEarly(Node *node, NodeWorkList &todo)
{
    qCDebug(lcSched) << "Scheduling node" << node->id() << "early...";

    SchedulerData *sd = schedulerData(node);

    if (sd->isFixed) {
        // Fixed nodes already know their schedule early position.
        qCDebug(lcSched) << ".. Fixed node" << node->id() << "is on minimum block"
                         << sd->minimumBlock->index()
                         << "which has dominator depth"
                         << m_dominatorDepthForBlock[sd->minimumBlock->index()];
    }

    for (Node *use : node->uses()) {
        if (isLive(use))
            propagateMinimumPosition(sd->minimumBlock, use, todo);
        else
            qCDebug(lcSched) << ".. Skipping node" << use->id() << "as it's not live";
    }
}

void NodeScheduler::propagateMinimumPosition(MIBlock *newMinimumPosition, Node *toNode,
                                             NodeWorkList &todo)
{
    Q_ASSERT(newMinimumPosition);

    SchedulerData *sd = schedulerData(toNode);
    if (sd->isFixed) // nothing to do
        return;

    MIBlock::Index minimumBlockIndex = sd->minimumBlock
            ? sd->minimumBlock->index()
            : MIFunction::StartBlockIndex;
    Q_ASSERT(m_domTree->insideSameDominatorChain(newMinimumPosition->index(), minimumBlockIndex));
    if (sd->minimumBlock == nullptr
            || m_dominatorDepthForBlock[newMinimumPosition->index()]
               > m_dominatorDepthForBlock[minimumBlockIndex]) {
        // ok, some input for toNode is scheduled *after* our current minimum depth, so we need
        // to adjust out minimal position. (This might involve rescheduling toNode's uses.)
        place(toNode, newMinimumPosition);
        todo.reEnqueue(toNode);
        qCDebug(lcSched)  << ".. Propagating minimum block" << sd->minimumBlock->index()
                          << "which has dominator depth"
                          << m_dominatorDepthForBlock[newMinimumPosition->index()]
                          << "to use node" << toNode->id();
    } else {
        qCDebug(lcSched) << ".. Minimum position" << newMinimumPosition->index()
                         << "is not better than" << minimumBlockIndex
                         << "for node" << toNode->id();
    }
}

// See Chapter 6.3.4 of https://scholarship.rice.edu/bitstream/handle/1911/96451/TR95-252.pdf for
// a description of the algorithm.
//
// There is one extra detail not described in the thesis mentioned above: loop hoisting. Before we
// place a node in the latest block that dominates all uses, we check if we accidentally sink it
// *into* a loop (meaning the latest block is inside a loop, where it is not if the earliest
// possible block would be chosen). If we detect that a nodes is going to sink into a loop, we walk
// the dominator path from the latest block up to the earliest block, and pick the first block that
// is in the same loop (if any) as the earlieast block.
//
// As noted in the thesis, this strategy might enlongen life times, which could be harmful for
// values that are simple to re-materialized or re-calculate.
void NodeScheduler::scheduleLate(const std::vector<Node *> &roots)
{
    NodeWorkList todo(m_irFunction->graph());
    for (Node *root : roots)
        todo.enqueue(root);

    while (Node *node = todo.dequeueNextNodeForVisiting())
        scheduleNodeLate(node, todo);
}

void NodeScheduler::scheduleNodeLate(Node *node, NodeWorkList &todo)
{
    if (!needsScheduling(node))
        return;
    qCDebug(lcSched) << "Scheduling node" << node->id() << "late...";

    auto sd = schedulerData(node);
    if (sd->unscheduledUses == SchedulerData::NotYetCalculated) {
        sd->unscheduledUses = 0;
        for (Node *use : node->uses()) {
            if (!isLive(use))
                continue;
            if (!needsScheduling(use))
                continue;
            if (schedulerData(use)->isFixed)
                continue;
            todo.enqueue(use);
            ++sd->unscheduledUses;
        }
    }

    if (sd->isFixed) {
        qCDebug(lcSched) << ".. it's fixed";
        enqueueInputs(node, todo);
        return;
    }

    if (sd->unscheduledUses) {
        qCDebug(lcSched).noquote() << ".. not all uses are fixed, postpone it."<< todo.status(node);
        return;
    }

    MIBlock *&minBlock = sd->minimumBlock;
    if (minBlock == nullptr)
        minBlock = m_miFunction->block(MIFunction::StartBlockIndex);
    MIBlock *commonUseDominator = commonDominatorOfUses(node);
    qCDebug(lcSched) << ".. common use dominator: block" << commonUseDominator->index();

    // the minBlock has to dominate the uses, *and* the common dominator of the uses.
    Q_ASSERT(minBlock->index() == commonUseDominator->index() ||
             m_domTree->dominates(minBlock->index(), commonUseDominator->index()));

    // we now found the deepest block, so use it as the target block:
    MIBlock *targetBlock = commonUseDominator;

    if (node->opcode() == Meta::FrameState) {
        // never hoist framestates: they're used (among other things) to keep their inputs alive, so
        // hoisting them out would end the life-time of those inputs prematurely
    } else {
        // but we want to prevent blocks sinking into loops unnecessary
        MIBlock *hoistBlock = getHoistBlock(targetBlock);
        while (hoistBlock
                    && m_dominatorDepthForBlock[hoistBlock->index()]
                       >= m_dominatorDepthForBlock[minBlock->index()]) {
            qCDebug(lcSched) << ".. hoisting node" << node->id() << "from block"
                             << targetBlock->index() << "to block" << hoistBlock->index();
            // ok, so there a) is a hoist block and b) it's deeper than the minimum block,
            // so lift it up one level ...
            targetBlock = hoistBlock;
            // ... and see if we can lift it one more level
            hoistBlock = getHoistBlock(targetBlock);
        }
    }

    qCDebug(lcSched) << ".. fixating it in block" << targetBlock->index()
                     << "where the minimum block was" << minBlock->index();

    placeFixed(node, targetBlock, DontSchedule);
    enqueueInputs(node, todo);
}

void NodeScheduler::enqueueInputs(Node *node, NodeWorkList &todo)
{
    for (Node *input : node->inputs()) {
        if (!input)
            continue;
        if (!needsScheduling(input))
            continue;
        if (!isLive(input))
            continue;
        auto sd = schedulerData(input);
        if (sd->isFixed)
            continue;
        qCDebug(lcSched).noquote() << "... enqueueing input node" << input->id()
                                   << todo.status(input);
        if (sd->unscheduledUses != SchedulerData::NotYetCalculated) {
            if (sd->unscheduledUses > 0)
                --sd->unscheduledUses;
            if (sd->unscheduledUses == 0)
                todo.reEnqueue(input);
        } else {
            todo.reEnqueue(input);
        }
    }
}

Node *NodeScheduler::firstNotFixedUse(Node *node)
{
    for (Node *use : node->uses()) {
        if (!isLive(use))
            continue;
        if (!schedulerData(use)->isFixed)
            return use;
    }
    return nullptr;
}

MIBlock *NodeScheduler::commonDominatorOfUses(Node *node)
{
    MIBlock *commonDominator = nullptr;
    for (auto useIt = node->uses().begin(), useEIt = node->uses().end(); useIt != useEIt; ++useIt) {
        Node *use = *useIt;
        if (!isLive(use))
            continue;
        // region nodes use other nodes through their control dependency. But those nodes should
        // already have been placed as block terminators before.
        Q_ASSERT(use->opcode() != Meta::Region);
        if (use->opcode() == Meta::Phi || use->opcode() == Meta::EffectPhi) {
            // find the predecessor block defining this input
            Node *region = use->controlInput(0);
            Node *input = region->controlInput(useIt.inputIndex());
            use = input;
        }
        auto minBlock = schedulerData(use)->minimumBlock;
        if (commonDominator == nullptr)
            commonDominator = minBlock;
        else
            commonDominator = getCommonDominator(commonDominator, minBlock);
    }
    return commonDominator;
}

void NodeScheduler::scheduleNodesInBlock()
{
    auto startBlock = m_miFunction->block(MIFunction::StartBlockIndex);
    for (Node *n : m_live.reachable()) {
        auto sd = schedulerData(n);
        if (!sd->minimumBlock)
            sd->minimumBlock = startBlock;
    }

    std::vector<std::vector<SchedulerData *>> nodesForBlock;
    nodesForBlock.resize(m_miFunction->blockCount());

    for (auto sd : m_schedulerData) {
        if (sd == nullptr)
            continue;
        if (!isLive(sd->node))
            continue;
        sd->unscheduledUses = 0;
        for (Node *use : sd->node->uses()) {
            if (!needsScheduling(use))
                continue;
            if (schedulerData(use)->isScheduledInBlock)
                continue;
            if (schedulerData(use)->minimumBlock == sd->minimumBlock)
                ++sd->unscheduledUses;
        }
        if (sd->unscheduledUses == 0)
            nodesForBlock[sd->minimumBlock->index()].push_back(sd);
    }

    NodeWorkList todo(m_irFunction->graph());
    for (MIBlock *b : m_miFunction->blocks()) {
        qCDebug(lcSched) << "Scheduling inside block" << b->index();
        MIInstr *insertionPoint = &b->instructions().back();
        todo.enqueue(insertionPoint->irNode());
        scheduleNodesInBlock(insertionPoint, b, todo);
        Q_ASSERT(todo.isEmpty());
        for (auto sd : nodesForBlock[b->index()]) {
            if (!sd->isScheduledInBlock)
                todo.enqueue(sd->node);
        }
        scheduleNodesInBlock(insertionPoint, b, todo);
        Q_ASSERT(todo.isEmpty());
        todo.reset();
    }
}

void NodeScheduler::scheduleNodesInBlock(MIInstr *&insertionPoint, MIBlock *b, NodeWorkList &todo)
{
    while (Node *n = todo.dequeueNextNodeForVisiting())
        scheduleNodeInBlock(n, insertionPoint, b, todo);
}

void NodeScheduler::scheduleNodeInBlock(Node *node, MIInstr *&insertionPoint, MIBlock *b,
                                        NodeWorkList &todo)
{
    Q_ASSERT(!node->isDead());

    if (!isLive(node))
        return;

    if (!needsScheduling(node))
        return;

    auto nodeData = schedulerData(node);
    if (nodeData->minimumBlock != b)
        return;

    const bool wasAlreadyScheduled = nodeData->isScheduledInBlock;
    if (!wasAlreadyScheduled) {
        if (nodeData->unscheduledUses)
            return;

        scheduleNodeNow(node, insertionPoint);
    }

    if (Node *framestate = node->frameStateInput())
        scheduleNodeInBlock(framestate, insertionPoint, b, todo);

    for (Node *input : node->inputs()) {
        if (!input)
            continue;
        if (!needsScheduling(input))
            continue;
        if (!isLive(input))
            continue;
        auto inputInfo = schedulerData(input);
        if (inputInfo->isScheduledInBlock)
            continue;
        Q_ASSERT(inputInfo->minimumBlock != nullptr);
        if (inputInfo->minimumBlock != b)
            continue;
        Q_ASSERT(!input->isDead());
        Q_ASSERT(inputInfo->unscheduledUses != SchedulerData::NotYetCalculated);
        if (!wasAlreadyScheduled && inputInfo->unscheduledUses > 0)
            --inputInfo->unscheduledUses;
        if (inputInfo->unscheduledUses == 0)
            todo.enqueue(input);
    }
}

void NodeScheduler::scheduleNodeNow(Node *node, MIInstr *&insertionPoint)
{
    qCDebug(lcSched) << ".. scheduling node" << node->id()
                     << "in block" << insertionPoint->parent()->index()
                     << "before node" << insertionPoint->irNode()->id();

    MIInstr *newInstr = createMIInstruction(node);
    newInstr->insertBefore(insertionPoint);
    insertionPoint = newInstr;
}

void NodeScheduler::place(Node *node, MIBlock *b)
{
    Q_ASSERT(!node->isDead());

    if (b == nullptr)
        return;

    schedulerData(node)->minimumBlock = b;
}

void NodeScheduler::placeFixed(Node *node, MIBlock *b, ScheduleOrNot markScheduled)
{
    place(node, b);
    auto sd = schedulerData(node);
    Q_ASSERT(!sd->isFixed);
    sd->isFixed = true;
    sd->isScheduledInBlock = markScheduled == Schedule;
}

unsigned NodeScheduler::vregForNode(Node *node)
{
    unsigned &vreg = m_vregs[unsigned(node->id())];
    if (vreg == std::numeric_limits<unsigned>::max())
        vreg = m_nextVReg++;
    return vreg;
}

void NodeScheduler::addBlockStart(std::vector<Node *> &roots, Node *startNode, MIBlock *block)
{
    block->instructions().insert(block->instructions().begin(), createMIInstruction(startNode));
    if (startNode->opcode() == Meta::Region) {
        for (Node *use : startNode->uses()) {
            if (use->opcode() == Meta::Phi && isLive(use)) {
                block->addArgument(MIOperand::createVirtualRegister(use, vregForNode(use)));
                placeFixed(use, block, Schedule);
                roots.push_back(use);
            } else if (use->opcode() == Meta::EffectPhi && isLive(use)) {
                placeFixed(use, block, Schedule);
                roots.push_back(use);
            }
        }
    }
}

void NodeScheduler::handleStartNode(Node *startNode, MIBlock *startBlock)
{
    startBlock->instructions().push_front(createMIInstruction(startNode));

    QVarLengthArray<Node *, 32> args;
    for (Node *use : startNode->uses()) {
        switch (use->opcode()) {
        case Meta::Engine: Q_FALLTHROUGH();
        case Meta::CppFrame:
        case Meta::Function:
            placeFixed(use, startBlock, Schedule);
            break;
        case Meta::Parameter: {
            auto param = ParameterPayload::get(*use->operation());
            int idx = int(param->parameterIndex());
            if (args.size() <= idx)
                args.resize(idx + 1);
            args[int(idx)] = use;
            placeFixed(use, startBlock, Schedule);
        }
            break;
        default:
            break;
        }
    }

    for (unsigned i = 0, ei = unsigned(args.size()); i != ei; ++i) {
        if (Node *arg = args.at(int(i)))
            startBlock->addArgument(MIOperand::createJSStackSlot(arg, i));
    }
}

static Node *firstControlOutput(Node *n)
{
    for (auto it = n->uses().begin(), eit = n->uses().end(); it != eit; ++it) {
        if (it.isUsedAsControl())
            return *it;
    }
    return nullptr;
}

MIInstr *NodeScheduler::createMIInstruction(Node *node)
{
    const auto opcode = node->operation()->kind();

    unsigned nArgs = 0;
    switch (opcode) {
    case Meta::UnwindDispatch: Q_FALLTHROUGH();
    case Meta::Jump: {
            Node *target = firstControlOutput(node);
            if (target->opcode() == Meta::Region) {
                for (Node *n : target->uses()) {
                    if (n->opcode() == Meta::Phi && isLive(n))
                        ++nArgs;
                }
            }
        }
        break;
    case Meta::Branch:
        nArgs = 1;
        break;
    case Meta::Return:
        nArgs = 1;
        break;
    default:
        nArgs = node->operation()->valueInputCount();
        break;
    }

    MIInstr *instr = MIInstr::create(m_irFunction->pool(), node, nArgs);
    for (unsigned i = 0, ei = node->operation()->valueInputCount(); i != ei; ++i)
        instr->setOperand(i, createMIOperand(node->input(i)));
    if (node->opcode() != Meta::Start && node->operation()->valueOutputCount() > 0)
        instr->setDestination(createMIOperand(node));

    schedulerData(node)->isScheduledInBlock = true;
    return instr;
}

MIOperand NodeScheduler::createMIOperand(Node *node)
{
    if (node->operation()->isConstant())
        return MIOperand::createConstant(node);

    auto opcode = node->operation()->kind();
    switch (opcode) {
    case Meta::Parameter:
        return MIOperand::createJSStackSlot(
                    node, unsigned(ParameterPayload::get(*node->operation())->parameterIndex()));
    case Meta::Engine:
        return MIOperand::createEngineRegister(node);
    case Meta::CppFrame:
        return MIOperand::createCppFrameRegister(node);
    case Meta::Function:
        return MIOperand::createFunction(node);
    default:
        if ((node->opcode() == Meta::Call
                    && CallPayload::get(*node->operation())->callee() == Meta::JSThisToObject)
               || node->opcode() == Meta::StoreThis) {
            return MIOperand::createJSStackSlot(node, CallData::This);
        } else {
            return MIOperand::createVirtualRegister(node, vregForNode(node));
        }
    }
}

void NodeScheduler::showNodesByBlock(const QString &description) const
{
    if (!lcSched().isDebugEnabled())
        return;

    qCDebug(lcSched) << description;

    for (MIBlock *b : m_miFunction->blocks()) {
        QString s;
        for (const SchedulerData *sd : m_schedulerData) {
            if (!sd)
                continue;
            if (!isLive(sd->node))
                continue;
            if (sd->minimumBlock == b) {
                if (!s.isEmpty())
                    s += QStringLiteral(", ");
                s += QStringLiteral("%1 (%2)").arg(QString::number(sd->node->id()),
                                                   sd->node->operation()->debugString());
            }
        }
        if (s.isEmpty())
            s = QStringLiteral("<<none>>");
        qCDebug(lcSched, "Nodes in block %u: %s", b->index(), s.toUtf8().constData());
    }
}

void NodeScheduler::dumpDotCFG() const
{
    QString out;
    out += QLatin1Char('\n');
    out += QStringLiteral("digraph{root=\"L%1\" label=\"Control Flow Graph\";"
                          "node[shape=circle];edge[dir=forward fontsize=10]\n")
            .arg(MIFunction::StartBlockIndex);
    for (MIBlock *src : m_miFunction->blocks()) {
        for (MIBlock *dst : src->outEdges()) {
            out += QStringLiteral("L%1->L%2\n").arg(QString::number(src->index()),
                                                    QString::number(dst->index()));
        }
    }
    out += QStringLiteral("}\n");
    qCDebug(lcDotCFG).nospace().noquote() << out;
}

} // IR namespace
} // QV4 namespace
QT_END_NAMESPACE
