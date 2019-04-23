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

#include "qv4blockscheduler_p.h"
#include "qv4domtree_p.h"
#include "qv4loopinfo_p.h"

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace IR {

Q_LOGGING_CATEGORY(lcBlockScheduler, "qt.v4.ir.blockscheduler")

bool BlockScheduler::checkCandidate(MIBlock *candidate)
{
    Q_ASSERT(loopInfo.loopHeaderFor(candidate) == currentGroup.group);

    for (MIBlock *pred : candidate->inEdges()) {
        if (pred->isDeoptBlock())
            continue;

        if (emitted.alreadyProcessed(pred))
            continue;

        if (dominatorTree.dominates(candidate->index(), pred->index())) {
            // this is a loop, where there in
            //   -> candidate edge is the jump back to the top of the loop.
            continue;
        }

        if (pred == candidate)
            // this is a very tight loop, e.g.:
            //   L1: ...
            //       goto L1
            // This can happen when, for example, the basic-block merging gets rid of the empty
            // body block. In this case, we can safely schedule this block (if all other
            // incoming edges are either loop-back edges, or have been scheduled already).
            continue;

        return false; // an incoming edge that is not yet emitted, and is not a back-edge
    }

    if (loopInfo.isLoopHeader(candidate)) {
        // postpone everything, and schedule the loop first.
        postponedGroups.push(currentGroup);
        currentGroup = WorkForGroup(candidate);
    }

    return true;
}

MIBlock *BlockScheduler::pickNext()
{
    while (true) {
        while (currentGroup.postponed.isEmpty()) {
            if (postponedGroups.isEmpty())
                return nullptr;
            if (currentGroup.group) // record the first and the last node of a group
                loopsStartEnd[currentGroup.group] = sequence.back();
            currentGroup = postponedGroups.pop();
        }

        MIBlock *next = currentGroup.postponed.pop();
        if (checkCandidate(next))
            return next;
    }

    Q_UNREACHABLE();
    return nullptr;
}

void BlockScheduler::emitBlock(MIBlock *bb)
{
    if (emitted.alreadyProcessed(bb))
        return;

    sequence.push_back(bb);
    emitted.markAsProcessed(bb);
}

void BlockScheduler::schedule(MIBlock *functionEntryPoint)
{
    MIBlock *next = functionEntryPoint;

    while (next) {
        emitBlock(next);
        // postpone all outgoing edges, if they were not already processed
        QVarLengthArray<MIBlock *, 32> nonExceptionEdges;
        // first postpone all exception edges, so they will be processed last
        for (int i = next->outEdges().size(); i != 0; ) {
            --i;
            MIBlock *out = next->outEdges().at(i);
            if (emitted.alreadyProcessed(out))
                continue;
            if (out == nullptr)
                continue;
            if (out->instructions().front().opcode() == Meta::OnException)
                postpone(out);
            else
                nonExceptionEdges.append(out);
        }
        for (MIBlock *edge : nonExceptionEdges)
            postpone(edge);
        next = pickNext();
    }

    // finally schedule all de-optimization blocks at the end
    for (auto bb : dominatorTree.function()->blocks()) {
        if (bb->isDeoptBlock())
            emitBlock(bb);
    }
}

void BlockScheduler::postpone(MIBlock *bb)
{
    if (currentGroup.group == loopInfo.loopHeaderFor(bb)) {
        currentGroup.postponed.append(bb);
        return;
    }

    for (int i = postponedGroups.size(); i != 0; ) {
        --i;
        WorkForGroup &g = postponedGroups[i];
        if (g.group == loopInfo.loopHeaderFor(bb)) {
            g.postponed.append(bb);
            return;
        }
    }

    Q_UNREACHABLE();
}

void BlockScheduler::dump() const
{
    if (!lcBlockScheduler().isDebugEnabled())
        return;

    QString s = QStringLiteral("Scheduled blocks:\n");
    for (auto *bb : sequence) {
        s += QLatin1String("    L") + QString::number(bb->index());
        MIBlock *loopEnd = loopsStartEnd[bb];
        if (loopEnd)
            s += QLatin1String(", loop start, ends at L") + QString::number(loopEnd->index());
        s += QLatin1Char('\n');
    }
    qCDebug(lcBlockScheduler).noquote().nospace() << s;
}

BlockScheduler::BlockScheduler(const DominatorTree &dominatorTree, const LoopInfo &loopInfo)
    : dominatorTree(dominatorTree)
    , loopInfo(loopInfo)
    , sequence(0)
    , emitted(dominatorTree.function())
{
    schedule(dominatorTree.function()->blocks().front());

    dump();

    if (dominatorTree.function()->blockCount() != sequence.size()) {
        qFatal("The block scheduler did not schedule all blocks. This is most likely due to"
               "a non-natural loop.");
        // Usually caused by having an execution path that manages to skip over unwind handler
        // reset: any exception happening after will jump back to the unwind handler, and thereby
        // creating a loop that can be entered in 2 different ways.
    }
}

} // IR namespace
} // QV4 namespace
QT_END_NAMESPACE
