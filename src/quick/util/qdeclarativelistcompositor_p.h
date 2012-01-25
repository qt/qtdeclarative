/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDECLARATIVELISTCOMPOSITOR_P_H
#define QDECLARATIVELISTCOMPOSITOR_P_H

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

#include <QtCore/qglobal.h>
#include <QtCore/qvector.h>

#include <private/qdeclarativechangeset_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QDeclarativeListCompositor
{
public:
    enum { MinimumGroupCount = 3, MaximumGroupCount = 11 };

    enum Group
    {
        Cache   = 0,
        Default = 1,
        Persisted = 2
    };

    enum Flag
    {
        CacheFlag       = 1 << Cache,
        DefaultFlag     = 1 << Default,
        PersistedFlag   = 1 << Persisted,
        PrependFlag     = 0x10000000,
        AppendFlag      = 0x20000000,
        UnresolvedFlag  = 0x40000000,
        MovedFlag       = 0x80000000,
        GroupMask       = ~(PrependFlag | AppendFlag | UnresolvedFlag | MovedFlag | CacheFlag)
    };

    class Range
    {
    public:
        Range() : next(this), previous(this), list(0), index(0), count(0), flags(0) {}
        Range(Range *next, void *list, int index, int count, uint flags)
            : next(next), previous(next->previous), list(list), index(index), count(count), flags(flags) {
            next->previous = this; previous->next = this; }

        Range *next;
        Range *previous;
        void *list;
        int index;
        int count;
        uint flags;

        inline int start() const { return index; }
        inline int end() const { return index + count; }

        inline int groups() const { return flags & GroupMask; }

        inline bool inGroup() const { return flags & GroupMask; }
        inline bool inCache() const { return flags & CacheFlag; }
        inline bool inGroup(int group) const { return flags & (1 << group); }
        inline bool isUnresolved() const { return flags & UnresolvedFlag; }

        inline bool prepend() const { return flags & PrependFlag; }
        inline bool append() const { return flags & AppendFlag; }
    };

    class Q_AUTOTEST_EXPORT iterator
    {
    public:
        inline iterator();
        inline iterator(const iterator &it);
        inline iterator(Range *range, int offset, Group group, int groupCount);
        inline ~iterator() {}

        bool operator ==(const iterator &it) const { return range == it.range && offset == it.offset; }
        bool operator !=(const iterator &it) const { return range != it.range || offset != it.offset; }

        bool operator ==(Group group) const { return range->flags & (1 << group); }
        bool operator !=(Group group) const { return !(range->flags & (1 << group)); }

        Range *&operator *() { return range; }
        Range * const &operator *() const { return range; }
        Range *operator ->() { return range; }
        const Range *operator ->() const { return range; }

        iterator &operator +=(int difference);
        iterator &operator -=(int difference);

        template<typename T> T *list() const { return static_cast<T *>(range->list); }
        int modelIndex() const { return range->index + offset; }

        void incrementIndexes(int difference) { incrementIndexes(difference, range->flags); }
        void decrementIndexes(int difference) { decrementIndexes(difference, range->flags); }

        inline void incrementIndexes(int difference, uint flags);
        inline void decrementIndexes(int difference, uint flags);

        void setGroup(Group g) { group = g; groupFlag = 1 << g; }

        Range *range;
        int offset;
        Group group;
        int groupFlag;
        int groupCount;
        union {
            struct {
                int cacheIndex;
            };
            int index[MaximumGroupCount];
        };
    };

    class Q_AUTOTEST_EXPORT insert_iterator : public iterator
    {
    public:
        inline insert_iterator() {}
        inline insert_iterator(const iterator &it) : iterator(it) {}
        inline insert_iterator(Range *, int, Group, int);
        inline ~insert_iterator() {}

        insert_iterator &operator +=(int difference);
        insert_iterator &operator -=(int difference);
    };

    struct Change
    {
        inline Change() {}
        inline Change(iterator it, int count, uint flags, int moveId = -1);
        int count;
        uint flags;
        int moveId;
        union {
            struct {
                int cacheIndex;
            };
            int index[MaximumGroupCount];
        };

        inline bool isMove() const { return moveId >= 0; }
        inline bool inCache() const { return flags & CacheFlag; }
        inline bool inGroup() const { return flags & GroupMask; }
        inline bool inGroup(int group) const { return flags & (CacheFlag << group); }

        inline int groups() const { return flags & GroupMask; }
    };

    struct Insert : public Change
    {
        Insert() {}
        Insert(iterator it, int count, uint flags, int moveId = -1)
            : Change(it, count, flags, moveId) {}
    };

    struct Remove : public Change
    {
        Remove() {}
        Remove(iterator it, int count, uint flags, int moveId = -1)
            : Change(it, count, flags, moveId) {}
    };

    QDeclarativeListCompositor();
    ~QDeclarativeListCompositor();

    int defaultGroups() const { return m_defaultFlags & ~PrependFlag; }
    void setDefaultGroups(int groups) { m_defaultFlags = groups | PrependFlag; }
    void setDefaultGroup(Group group) { m_defaultFlags |= (1 << group); }
    void clearDefaultGroup(Group group) { m_defaultFlags &= ~(1 << group); }
    void setRemoveGroups(int groups) { m_removeFlags = PrependFlag | AppendFlag | groups; }
    void setGroupCount(int count);

    int count(Group group) const;
    iterator find(Group group, int index);
    iterator find(Group group, int index) const;
    insert_iterator findInsertPosition(Group group, int index);

    iterator begin(Group group);
    const iterator &end() { return m_end; }

    void append(void *list, int index, int count, uint flags, QVector<Insert> *inserts = 0);
    void insert(Group group, int before, void *list, int index, int count, uint flags, QVector<Insert> *inserts = 0);
    iterator insert(iterator before, void *list, int index, int count, uint flags, QVector<Insert> *inserts = 0);

    void setFlags(Group fromGroup, int from, int count, Group group, int flags, QVector<Insert> *inserts = 0);
    void setFlags(iterator from, int count, Group group, uint flags, QVector<Insert> *inserts = 0);
    void setFlags(Group fromGroup, int from, int count, uint flags, QVector<Insert> *inserts = 0) {
        setFlags(fromGroup, from, count, fromGroup, flags, inserts); }
    void setFlags(iterator from, int count, uint flags, QVector<Insert> *inserts = 0) {
        setFlags(from, count, from.group, flags, inserts); }

    void clearFlags(Group fromGroup, int from, int count, Group group, uint flags, QVector<Remove> *removals = 0);
    void clearFlags(iterator from, int count, Group group, uint flags, QVector<Remove> *removals = 0);
    void clearFlags(Group fromGroup, int from, int count, uint flags, QVector<Remove> *removals = 0) {
        clearFlags(fromGroup, from, count, fromGroup, flags, removals); }
    void clearFlags(iterator from, int count, uint flags, QVector<Remove> *removals = 0) {
        clearFlags(from, count, from.group, flags, removals); }

    void removeList(void *list, QVector<Remove> *removals, bool destroyed);

    bool verifyMoveTo(Group fromGroup, int from, Group toGroup, int to, int count, Group group) const;

    void move(
            Group fromGroup,
            int from,
            Group toGroup,
            int to,
            int count,
            Group group,
            QVector<Remove> *removals = 0,
            QVector<Insert> *inserts = 0);
    void clear();

    void listItemsInserted(void *list, int index, int count, QVector<Insert> *inserts);
    void listItemsRemoved(void *list, int index, int count, QVector<Remove> *removals);
    void listItemsMoved(void *list, int from, int to, int count, QVector<Remove> *removals, QVector<Insert> *inserts);
    void listItemsChanged(void *list, int index, int count, QVector<Change> *changes);
    void listChanged(
            void *list,
            const QDeclarativeChangeSet &changeSet,
            QVector<Remove> *removals,
            QVector<Insert> *inserts,
            QVector<Change> *changes);

    void transition(
            Group from,
            Group to,
            QVector<QDeclarativeChangeSet::Remove> *removes,
            QVector<QDeclarativeChangeSet::Insert> *inserts);

private:
    Range m_ranges;
    iterator m_end;
    iterator m_cacheIt;
    int m_groupCount;
    int m_defaultFlags;
    int m_removeFlags;

    inline Range *insert(Range *before, void *list, int index, int count, uint flags);
    inline Range *erase(Range *range);

    struct MovedFlags
    {
        MovedFlags() {}
        MovedFlags(int moveId, uint flags) : moveId(moveId), flags(flags) {}

        int moveId;
        uint flags;
    };

    void listItemsRemoved(
            QVector<Remove> *translatedRemovals,
            void *list,
            QVector<QDeclarativeChangeSet::Remove> *removals,
            QVector<QDeclarativeChangeSet::Insert> *insertions = 0,
            QVector<MovedFlags> *movedFlags = 0,
            int moveId = 0);
    void listItemsInserted(
            QVector<Insert> *translatedInsertions,
            void *list,
            const QVector<QDeclarativeChangeSet::Insert> &insertions,
            const QVector<MovedFlags> *movedFlags = 0);
    void listItemsChanged(
            QVector<Change> *translatedChanges,
            void *list,
            const QVector<QDeclarativeChangeSet::Change> &changes);

    friend Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeListCompositor &list);
};

inline QDeclarativeListCompositor::iterator::iterator()
    : range(0), offset(0), group(Default), groupCount(0) {}
inline QDeclarativeListCompositor::iterator::iterator(const iterator &it)
    : range(it.range)
    , offset(it.offset)
    , group(it.group)
    , groupFlag(it.groupFlag)
    , groupCount(it.groupCount)
{
    for (int i = 0; i < groupCount; ++i)
        index[i] = it.index[i];
}

inline QDeclarativeListCompositor::iterator::iterator(
        Range *range, int offset, Group group, int groupCount)
    : range(range)
    , offset(offset)
    , group(group)
    , groupFlag(1 << group)
    , groupCount(groupCount)
{
    for (int i = 0; i < groupCount; ++i)
        index[i] = 0;
}

inline void QDeclarativeListCompositor::iterator::incrementIndexes(int difference, uint flags)
{
    for (int i = 0; i < groupCount; ++i) {
        if (flags & (1 << i))
            index[i] += difference;
    }
}

inline void QDeclarativeListCompositor::iterator::decrementIndexes(int difference, uint flags)
{
    for (int i = 0; i < groupCount; ++i) {
        if (flags & (1 << i))
            index[i] -= difference;
    }
}

inline QDeclarativeListCompositor::insert_iterator::insert_iterator(
        Range *range, int offset, Group group, int groupCount)
    : iterator(range, offset, group, groupCount) {}

inline QDeclarativeListCompositor::Change::Change(iterator it, int count, uint flags, int moveId)
    : count(count), flags(flags), moveId(moveId)
{
    for (int i = 0; i < MaximumGroupCount; ++i)
        index[i] = it.index[i];
}

Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::Group &group);
Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::Range &range);
Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::iterator &it);
Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::Change &change);
Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::Remove &remove);
Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::Insert &insert);
Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeListCompositor &list);

QT_END_NAMESPACE

#endif
