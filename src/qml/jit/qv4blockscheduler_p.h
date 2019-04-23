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

#ifndef QV4BLOCKSCHEDULER_P_H
#define QV4BLOCKSCHEDULER_P_H

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

#include <QtCore/qstack.h>

#include "qv4mi_p.h"
#include "qv4util_p.h"

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

class DominatorTree;
class LoopInfo;

// High-level algorithm:
//  0. start with the first node (the start node) of a function
//  1. emit the node
//  2. add all outgoing edges that are not yet emitted to the postponed stack
//  3. When the postponed stack is empty, pop a stack from the loop stack. If that is empty too,
//     we're done.
//  4. pop a node from the postponed stack, and check if it can be scheduled:
//     a. if all incoming edges are scheduled, go to 4.
//     b. if an incoming edge is unscheduled, but it's a back-edge (an edge in a loop that jumps
//        back to the start of the loop), ignore it
//     c. if there is any unscheduled edge that is not a back-edge, ignore this node, and go to 4.
//  5. if this node is the start of a loop, push the postponed stack on the loop stack.
//  6. go back to 1.
//
// The postponing action in step 2 will put the node into its containing group. The case where this
// is important is when a (labeled) continue or a (labeled) break statement occur in a loop: the
// outgoing edge points to a node that is not part of the current loop (and possibly not of the
// parent loop).
//
// Linear scan register allocation benefits greatly from short life-time intervals with few holes
// (see for example section 4 (Lifetime Analysis) of [Wimmer1]). This algorithm makes sure that the
// blocks of a group are scheduled together, with no non-loop blocks in between. This applies
// recursively for nested loops. It also schedules groups of if-then-else-endif blocks together for
// the same reason.
class BlockScheduler
{
    const DominatorTree &dominatorTree;
    const LoopInfo &loopInfo;

    struct WorkForGroup
    {
        MIBlock *group;
        QStack<MIBlock *> postponed;

        WorkForGroup(MIBlock *group = nullptr) : group(group) {}
    };
    WorkForGroup currentGroup;
    QStack<WorkForGroup> postponedGroups;
    std::vector<MIBlock *> sequence;

    class ProcessedBlocks
    {
        BitVector processed;

    public:
        ProcessedBlocks(MIFunction *function)
            : processed(int(function->blockCount()), false)
        {}

        bool alreadyProcessed(MIBlock *bb) const
        {
            Q_ASSERT(bb);

            return processed.at(bb->index());
        }

        void markAsProcessed(MIBlock *bb)
        {
            processed.setBit(bb->index());
        }
    } emitted;
    QHash<MIBlock *, MIBlock *> loopsStartEnd;

    bool checkCandidate(MIBlock *candidate);
    MIBlock *pickNext();
    void emitBlock(MIBlock *bb);
    void schedule(MIBlock *functionEntryPoint);
    void postpone(MIBlock *bb);
    void dump() const;

public:
    BlockScheduler(const DominatorTree &dominatorTree, const LoopInfo &loopInfo);

    const std::vector<MIBlock *> &scheduledBlockSequence() const
    { return sequence; }

    QHash<MIBlock *, MIBlock *> loopEndsByStartBlock() const
    { return loopsStartEnd; }
};


} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4BLOCKSCHEDULER_P_H
