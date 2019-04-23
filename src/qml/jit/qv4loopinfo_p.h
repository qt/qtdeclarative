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

#ifndef QV4LOOPINFO_P_H
#define QV4LOOPINFO_P_H

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

#include "qv4mi_p.h"

#include <QHash>

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

class DominatorTree;

// Detect all (sub-)loops in a function.
//
// Doing loop detection on the CFG is better than relying on the statement information in
// order to mark loops. Although JavaScript only has natural loops, it can still be the case
// that something is not a loop even though a loop-like-statement is in the source. For
// example:
//    while (true) {
//      if (i > 0)
//        break;
//      else
//        break;
//    }
//
// Algorithm:
//  - do a DFS on the dominator tree, where for each node:
//    - collect all back-edges
//    - if there are back-edges, the node is a loop-header for a new loop, so:
//      - walk the CFG is reverse-direction, and for every node:
//        - if the node already belongs to a loop, we've found a nested loop:
//          - get the loop-header for the (outermost) nested loop
//          - add that loop-header to the current loop
//          - continue by walking all incoming edges that do not yet belong to the current loop
//        - if the node does not belong to a loop yet, add it to the current loop, and
//          go on with all incoming edges
//
// Loop-header detection by checking for back-edges is very straight forward: a back-edge is
// an incoming edge where the other node is dominated by the current node. Meaning: all
// execution paths that reach that other node have to go through the current node, that other
// node ends with a (conditional) jump back to the loop header.
//
// The exact order of the DFS on the dominator tree is not important. The only property has to
// be that a node is only visited when all the nodes it dominates have been visited before.
// The reason for the DFS is that for nested loops, the inner loop's loop-header is dominated
// by the outer loop's header. So, by visiting depth-first, sub-loops are identified before
// their containing loops, which makes nested-loop identification free. An added benefit is
// that the nodes for those sub-loops are only processed once.
//
// Note: independent loops that share the same header are merged together. For example, in
// the code snippet below, there are 2 back-edges into the loop-header, but only one single
// loop will be detected.
//    while (a) {
//      if (b)
//        continue;
//      else
//        continue;
//    }
class LoopInfo
{
    Q_DISABLE_COPY_MOVE(LoopInfo)

    struct BlockInfo
    {
        MIBlock *loopHeader = nullptr;
        bool isLoopHeader = false;
        std::vector<MIBlock *> loopExits;
    };

public:
    LoopInfo(const DominatorTree &dt)
        : dt(dt)
    {}

    ~LoopInfo() = default;

    void detectLoops();

    MIBlock *loopHeaderFor(MIBlock *bodyBlock) const
    { return blockInfos[bodyBlock->index()].loopHeader; }

    bool isLoopHeader(MIBlock *block) const
    { return blockInfos[block->index()].isLoopHeader; }

    const std::vector<MIBlock *> loopExitsForLoop(MIBlock *loopHeader) const
    { return blockInfos[loopHeader->index()].loopExits; }

private:
    void subLoop(MIBlock *loopHead, const std::vector<MIBlock *> &backedges);
    void collectLoopExits();
    bool inLoopOrSubLoop(MIBlock *block, MIBlock *loopHeader) const;

    void dump() const;

private:
    const DominatorTree &dt;
    std::vector<BlockInfo> blockInfos;
};

} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4LOOPINFO_P_H
