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

#include "qv4loopinfo_p.h"
#include "qv4domtree_p.h"

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace IR {

Q_LOGGING_CATEGORY(lcLoopinfo, "qt.v4.ir.loopinfo")

void LoopInfo::detectLoops()
{
    blockInfos.resize(dt.function()->blockCount());

    std::vector<MIBlock *> backedges;
    backedges.reserve(4);

    const auto order = dt.calculateDFNodeIterOrder();
    for (MIBlock *bb : order) {
        if (bb->isDeoptBlock())
            continue;

        backedges.clear();

        for (MIBlock *pred : bb->inEdges()) {
            if (bb == pred || dt.dominates(bb->index(), pred->index()))
                backedges.push_back(pred);
        }

        if (!backedges.empty())
            subLoop(bb, backedges);
    }

    collectLoopExits();

    dump();
}

void LoopInfo::collectLoopExits()
{
    for (MIBlock::Index i = 0, ei = MIBlock::Index(blockInfos.size()); i != ei; ++i) {
        BlockInfo &bi = blockInfos[i];
        MIBlock *currentBlock = dt.function()->block(i);
        if (bi.isLoopHeader) {
            for (MIBlock *outEdge : currentBlock->outEdges()) {
                if (outEdge != currentBlock && !inLoopOrSubLoop(outEdge, currentBlock))
                    bi.loopExits.push_back(outEdge);
            }
        }
        if (MIBlock *containingLoop = bi.loopHeader) {
            BlockInfo &loopInfo = blockInfos[containingLoop->index()];
            for (MIBlock *outEdge : currentBlock->outEdges()) {
                if (outEdge != containingLoop && !inLoopOrSubLoop(outEdge, containingLoop))
                    loopInfo.loopExits.push_back(outEdge);
            }
        }
    }
}

bool LoopInfo::inLoopOrSubLoop(MIBlock *block, MIBlock *loopHeader) const
{
    const BlockInfo &bi = blockInfos[block->index()];
    MIBlock *loopHeaderForBlock = bi.loopHeader;
    if (loopHeaderForBlock == nullptr)
        return false; // block is not in any loop

    while (loopHeader) {
        if (loopHeader == loopHeaderForBlock)
            return true;
        // look into the parent loop of loopHeader to see if block is contained there
        loopHeader = blockInfos[loopHeader->index()].loopHeader;
    }

    return false;
}

void LoopInfo::subLoop(MIBlock *loopHead, const std::vector<MIBlock *> &backedges)
{
    blockInfos[loopHead->index()].isLoopHeader = true;

    std::vector<MIBlock *> worklist;
    worklist.reserve(backedges.size() + 8);
    worklist.insert(worklist.end(), backedges.begin(), backedges.end());
    while (!worklist.empty()) {
        MIBlock *predIt = worklist.back();
        worklist.pop_back();

        MIBlock *subloop = blockInfos[predIt->index()].loopHeader;
        if (subloop) {
            // This is a discovered block. Find its outermost discovered loop.
            while (MIBlock *parentLoop = blockInfos[subloop->index()].loopHeader)
                subloop = parentLoop;

            // If it is already discovered to be a subloop of this loop, continue.
            if (subloop == loopHead)
                continue;

            // Yay, it's a subloop of this loop.
            blockInfos[subloop->index()].loopHeader = loopHead;
            predIt = subloop;

            // Add all predecessors of the subloop header to the worklist, as long as
            // those predecessors are not in the current subloop. It might be the case
            // that they are in other loops, which we will then add as a subloop to the
            // current loop.
            for (MIBlock *predIn : predIt->inEdges())
                if (blockInfos[predIn->index()].loopHeader != subloop)
                    worklist.push_back(predIn);
        } else {
            if (predIt == loopHead)
                continue;

            // This is an undiscovered block. Map it to the current loop.
            blockInfos[predIt->index()].loopHeader = loopHead;

            // Add all incoming edges to the worklist.
            for (MIBlock *bb : predIt->inEdges())
                worklist.push_back(bb);
        }
    }
}

void LoopInfo::dump() const
{
    if (!lcLoopinfo().isDebugEnabled())
        return;

    QString s = QStringLiteral("Loop information:\n");
    for (size_t i = 0, ei = blockInfos.size(); i != ei; ++i) {
        const BlockInfo &bi = blockInfos[i];
        s += QStringLiteral("    %1 : is loop header: %2, contained in loop header's loop: ")
                .arg(i).arg(bi.isLoopHeader ? QLatin1String("yes") : QLatin1String("no"));
        if (bi.loopHeader)
            s += QString::number(bi.loopHeader->index());
        else
            s += QLatin1String("<none>");
        if (bi.isLoopHeader) {
            s += QStringLiteral(", loop exits: ");
            if (bi.loopExits.empty()) {
                s += QLatin1String("<none>");
            } else {
                bool first = true;
                for (MIBlock *exit : bi.loopExits) {
                    if (first)
                        first = false;
                    else
                        s += QStringLiteral(", ");
                    s += QString::number(exit->index());
                }
            }
        }
        s += QLatin1Char('\n');
    }
    qCDebug(lcLoopinfo).noquote().nospace() << s;
}

} // IR namespace
} // QV4 namespace
QT_END_NAMESPACE
