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

#ifndef QV4MIBLOCKSET_P_H
#define QV4MIBLOCKSET_P_H

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
#include "qv4util_p.h"

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

class MIBlockSet
{
    using Flags = BitVector;

    QVarLengthArray<MIBlock::Index, 8> blockNumbers;
    Flags *blockFlags = nullptr;
    MIFunction *function = nullptr;
    enum { MaxVectorCapacity = 8 };

public:
    class const_iterator;
    friend class const_iterator;

public:
    MIBlockSet(MIFunction *f = nullptr)
    {
        if (f)
            init(f);
    }

    MIBlockSet(MIBlockSet &&other) noexcept
    {
        std::swap(blockNumbers, other.blockNumbers);
        std::swap(blockFlags, other.blockFlags);
        std::swap(function, other.function);
    }

    MIBlockSet(const MIBlockSet &other)
        : function(other.function)
    {
        if (other.blockFlags)
            blockFlags = new Flags(*other.blockFlags);
        blockNumbers = other.blockNumbers;
    }

    MIBlockSet &operator=(const MIBlockSet &other)
    {
        if (blockFlags) {
            delete blockFlags;
            blockFlags = nullptr;
        }
        function = other.function;
        if (other.blockFlags)
            blockFlags = new Flags(*other.blockFlags);
        blockNumbers = other.blockNumbers;
        return *this;
    }

    MIBlockSet &operator=(MIBlockSet &&other) noexcept
    {
        if (&other != this) {
            std::swap(blockNumbers, other.blockNumbers);

            delete blockFlags;
            blockFlags = other.blockFlags;
            other.blockFlags = nullptr;

            function = other.function;
        }
        return *this;
    }

    ~MIBlockSet()
    {
        delete blockFlags;
    }

    void init(MIFunction *f)
    {
        Q_ASSERT(!function);
        Q_ASSERT(f);
        function = f;
    }

    bool empty() const;

    void insert(MIBlock *bb)
    {
        Q_ASSERT(function);

        if (blockFlags) {
            blockFlags->setBit(bb->index());
            return;
        }

        for (unsigned int blockNumber : qAsConst(blockNumbers)) {
            if (blockNumber == bb->index())
                return;
        }

        if (blockNumbers.size() == MaxVectorCapacity) {
            blockFlags = new Flags(int(function->blockCount()), false);
            for (unsigned int blockNumber : qAsConst(blockNumbers)) {
                blockFlags->setBit(int(blockNumber));
            }
            blockNumbers.clear();
            blockFlags->setBit(int(bb->index()));
        } else {
            blockNumbers.append(bb->index());
        }
    }

    void remove(MIBlock *bb)
    {
        Q_ASSERT(function);

        if (blockFlags) {
            blockFlags->clearBit(bb->index());
            return;
        }

        for (int i = 0; i < blockNumbers.size(); ++i) {
            if (blockNumbers[i] == bb->index()) {
                blockNumbers.remove(i);
                return;
            }
        }
    }

    const_iterator begin() const;
    const_iterator end() const;

    void collectValues(std::vector<MIBlock *> &bbs) const;

    bool contains(MIBlock *bb) const
    {
        Q_ASSERT(function);

        if (blockFlags)
            return blockFlags->at(bb->index());

        for (unsigned int blockNumber : blockNumbers) {
            if (blockNumber == bb->index())
                return true;
        }

        return false;
    }
};

class MIBlockSet::const_iterator
{
    const MIBlockSet &set;
    // ### These two members could go into a union, but clang won't compile
    //     (https://codereview.qt-project.org/#change,74259)
    QVarLengthArray<MIBlock::Index, 8>::const_iterator numberIt;
    MIBlock::Index flagIt;

    friend class MIBlockSet;
    const_iterator(const MIBlockSet &set, bool end)
        : set(set)
    {
        if (end || !set.function) {
            if (!set.blockFlags)
                numberIt = set.blockNumbers.end();
            else
                flagIt = set.blockFlags->size();
        } else {
            if (!set.blockFlags)
                numberIt = set.blockNumbers.begin();
            else
                findNextWithFlags(0);
        }
    }

    void findNextWithFlags(int start)
    {
        flagIt = MIBlock::Index(set.blockFlags->findNext(start, true, /*wrapAround = */false));
        Q_ASSERT(flagIt <= MIBlock::Index(set.blockFlags->size()));
    }

public:
    MIBlock *operator*() const
    {
        if (!set.blockFlags)
            return set.function->block(*numberIt);

        Q_ASSERT(flagIt <= set.function->blockCount());
        return set.function->block(flagIt);

    }

    bool operator==(const const_iterator &other) const
    {
        if (&set != &other.set)
            return false;
        if (!set.blockFlags)
            return numberIt == other.numberIt;
        return flagIt == other.flagIt;
    }

    bool operator!=(const const_iterator &other) const
    { return !(*this == other); }

    const_iterator &operator++()
    {
        if (!set.blockFlags)
            ++numberIt;
        else
            findNextWithFlags(flagIt + 1);

        return *this;
    }
};

inline bool MIBlockSet::empty() const
{ return begin() == end(); }

inline MIBlockSet::const_iterator MIBlockSet::begin() const
{ return const_iterator(*this, false); }

inline MIBlockSet::const_iterator MIBlockSet::end() const
{ return const_iterator(*this, true); }

inline void MIBlockSet::collectValues(std::vector<MIBlock *> &bbs) const
{
    Q_ASSERT(function);

    for (auto it : *this)
        bbs.push_back(it);
}

} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4MIBLOCKSET_P_H
