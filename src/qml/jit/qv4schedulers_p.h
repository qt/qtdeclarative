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

#ifndef QV4SCHEDULER_P_H
#define QV4SCHEDULER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qv4global_p.h"
#include "qv4mi_p.h"
#include "qv4node_p.h"
#include "qv4domtree_p.h"
#include "qv4loopinfo_p.h"

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

// Node scheduling "flattens" the graph into basic blocks with an ordered list of instructions.
//
// The various steps are mentioned in buildMIFunction, but the general idea is described in
// https://scholarship.rice.edu/bitstream/handle/1911/96451/TR95-252.pdf in chapter 6.
class NodeScheduler final
{
    Q_DISABLE_COPY_MOVE(NodeScheduler)

    class SchedulerData final {
        Q_DISABLE_COPY_MOVE(SchedulerData)
    public:
        static SchedulerData *create(QQmlJS::MemoryPool *pool)
        { return pool->New<SchedulerData>(); }

        SchedulerData() = default;
        ~SchedulerData() = default;

        Node *node = nullptr;
        MIBlock *minimumBlock = nullptr;
        bool isFixed = false;
        bool isScheduledInBlock = false;
        static constexpr unsigned NotYetCalculated = std::numeric_limits<unsigned>::max();
        unsigned unscheduledUses = NotYetCalculated;
    };

public:
    NodeScheduler(Function *irFunction);
    ~NodeScheduler() = default;

    MIFunction *buildMIFunction();

private:
    std::vector<Node *> buildCFG();
    void scheduleEarly(const std::vector<Node *> &roots);
    void scheduleEarly(Node *node, NodeWorkList &todo);
    void propagateMinimumPosition(MIBlock *newMinimumPosition, Node *toNode, NodeWorkList &todo);
    void scheduleLate(const std::vector<Node *> &roots);
    void scheduleNodeLate(Node *node, NodeWorkList &todo);
    void enqueueInputs(Node *node, NodeWorkList &todo);
    Node *firstNotFixedUse(Node *node);
    MIBlock *commonDominatorOfUses(Node *node);
    void scheduleNodesInBlock();
    void scheduleNodesInBlock(MIInstr *&insertionPoint, MIBlock *b, NodeWorkList &todo);
    void scheduleNodeInBlock(Node *node, MIInstr *&insertionPoint, MIBlock *b, NodeWorkList &todo);
    void scheduleNodeNow(Node *node, MIInstr *&insertionPoint);

    void place(Node *node, MIBlock *b);
    enum ScheduleOrNot { DontSchedule, Schedule };
    void placeFixed(Node *node, MIBlock *b, ScheduleOrNot markScheduled);
    unsigned vregForNode(Node *node);
    void addBlockStart(std::vector<Node *> &roots, Node *startNode, MIBlock *block);
    void enqueueControlInputs(Node *node);
    void handleStartNode(Node *startNode, MIBlock *startBlock);
    MIInstr *createMIInstruction(Node *node);
    MIOperand createMIOperand(Node *node);
    SchedulerData *schedulerData(Node *n)
    {
        if (Q_UNLIKELY(m_schedulerData.size() <= n->id()))
            m_schedulerData.resize(n->id() + 8);
        SchedulerData *&sd = m_schedulerData[n->id()];
        if (Q_UNLIKELY(sd == nullptr)) {
            sd = SchedulerData::create(m_irFunction->pool());
            sd->node = n;
        }
        return sd;
    }
    bool isLive(Node *n) const
    { return m_live.isReachable(n->id()); }
    bool canStartBlock(Node *node) const;
    bool isControlFlowSplit(Node *node) const;
    bool isBlockTerminator(Node *node) const;
    MIBlock *getCommonDominator(MIBlock *one, MIBlock *other) const;
    MIBlock *getHoistBlock(MIBlock *block) const;

    void showNodesByBlock(const QString &description) const;

    void dumpDotCFG() const;

private:
    Function *m_irFunction = nullptr;
    MIFunction *m_miFunction = nullptr;
    QScopedPointer<LoopInfo> m_loopInfo;
    QScopedPointer<DominatorTree> m_domTree;
    std::vector<int> m_dominatorDepthForBlock;
    std::vector<unsigned> m_vregs;
    std::vector<SchedulerData *> m_schedulerData;
    NodeCollector m_live;
    unsigned m_nextVReg = 0;
};

} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4SCHEDULER_P_H
