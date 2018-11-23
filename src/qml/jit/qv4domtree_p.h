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

#ifndef QV4DOMTREE_P_H
#define QV4DOMTREE_P_H

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
#include "qv4miblockset_p.h"

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

class DominatorTree
{
    Q_DISABLE_COPY_MOVE(DominatorTree)

public:
    DominatorTree(MIFunction *f);
    ~DominatorTree() = default;

    void dumpImmediateDominators() const;
    MIFunction *function() const
    { return m_function; }

    void setImmediateDominator(MIBlock::Index dominated, MIBlock::Index dominator);

    MIBlock::Index immediateDominator(MIBlock::Index blockIndex) const
    { return m_idom[blockIndex]; }

    bool dominates(MIBlock::Index dominator, MIBlock::Index dominated) const;

    bool insideSameDominatorChain(MIBlock::Index one, MIBlock::Index other) const
    { return one == other || dominates(one, other) || dominates(other, one); }

    std::vector<MIBlock *> calculateDFNodeIterOrder() const;

    std::vector<int> calculateNodeDepths() const;

private: // functions
    int calculateNodeDepth(MIBlock::Index nodeIdx, std::vector<int> &nodeDepths) const;
    void link(MIBlock::Index p, MIBlock::Index n);
    void calculateIDoms();
    void dfs(MIBlock::Index node);
    MIBlock::Index ancestorWithLowestSemi(MIBlock::Index v, std::vector<MIBlock::Index> &worklist);

private: // data
    struct Data {
        std::vector<unsigned> dfnum; // MIBlock index -> dfnum
        std::vector<MIBlock::Index> vertex;
        std::vector<MIBlock::Index> parent; // MIBlock index -> parent MIBlock index
        std::vector<MIBlock::Index> ancestor; // MIBlock index -> ancestor MIBlock index
        std::vector<MIBlock::Index> best; // MIBlock index -> best MIBlock index
        std::vector<MIBlock::Index> semi; // MIBlock index -> semi dominator MIBlock index
        std::vector<MIBlock::Index> samedom; // MIBlock index -> same dominator MIBlock index
        unsigned size = 0;
    };

    MIFunction *m_function;
    QScopedPointer<Data> m_data;
    std::vector<MIBlock::Index> m_idom; // MIBlock index -> immediate dominator MIBlock index
};

class DominatorFrontier
{
public:
    DominatorFrontier(const DominatorTree &domTree)
    { compute(domTree); }

    const MIBlockSet &operator[](MIBlock *n) const
    { return m_df[n->index()]; }

private: // functions
    void compute(const DominatorTree &domTree);
    void dump(MIFunction *function);

private: // data
    std::vector<MIBlockSet> m_df; // MIBlock index -> dominator frontier
};

} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4DOMTREE_P_H
