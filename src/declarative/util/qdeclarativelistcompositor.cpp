/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "qdeclarativelistcompositor_p.h"

#include <QtCore/qvarlengtharray.h>

//#define QT_DECLARATIVE_VERIFY_MINIMAL
//#define QT_DECLARATIVE_VERIFY_INTEGRITY

QT_BEGIN_NAMESPACE

#ifdef QT_DECLARATIVE_VERIFY_MINIMAL
#define QT_DECLARATIVE_VERIFY_INTEGRITY
static bool qt_verifyMinimal(
        const QDeclarativeListCompositor::iterator &begin,
        const QDeclarativeListCompositor::iterator &end)
{
    bool minimal = true;
    int index = 0;

    for (const QDeclarativeListCompositor::Range *range = begin->next; range != *end; range = range->next, ++index) {
        if (range->previous->list == range->list
                && range->previous->flags == (range->flags & ~QDeclarativeListCompositor::AppendFlag)
                && range->previous->end() == range->index) {
            qWarning() << index << "Consecutive ranges";
            qWarning() << *range->previous;
            qWarning() << *range;
            minimal = false;
        }
    }

    return minimal;
}

#endif

#ifdef QT_DECLARATIVE_VERIFY_INTEGRITY
static bool qt_printInfo(const QDeclarativeListCompositor &compositor)
{
    qWarning() << compositor;
    return true;
}

static bool qt_verifyIntegrity(
        const QDeclarativeListCompositor::iterator &begin,
        const QDeclarativeListCompositor::iterator &end,
        const QDeclarativeListCompositor::iterator &cachedIt)
{
    bool valid = true;

    int index = 0;
    QDeclarativeListCompositor::iterator it;
    for (it = begin; *it != *end; *it = it->next) {
        if (it->count == 0 && !it->append()) {
            qWarning() << index << "Empty non-append range";
            valid = false;
        }
        if (it->count < 0) {
            qWarning() << index << "Negative count";
            valid = false;
        }
        if (it->list && it->flags != QDeclarativeListCompositor::CacheFlag && it->index < 0) {
            qWarning() << index <<"Negative index";
            valid = false;
        }
        if (it->previous->next != it.range) {
            qWarning() << index << "broken list: it->previous->next != it.range";
            valid = false;
        }
        if (it->next->previous != it.range) {
            qWarning() << index << "broken list: it->next->previous != it.range";
            valid = false;
        }
        if (*it == *cachedIt) {
            for (int i = 0; i < end.groupCount; ++i) {
                int groupIndex = it.index[i];
                if (cachedIt->flags & (1 << i))
                    groupIndex += cachedIt.offset;
                if (groupIndex != cachedIt.index[i]) {
                    qWarning() << index
                            << "invalid cached index"
                            << QDeclarativeListCompositor::Group(i)
                            << "Expected:"
                            << groupIndex
                            << "Actual"
                            << cachedIt.index[i]
                            << cachedIt;
                    valid = false;
                }
            }
        }
        it.incrementIndexes(it->count);
        ++index;
    }

    for (int i = 0; i < end.groupCount; ++i) {
        if (end.index[i] != it.index[i]) {
            qWarning() << "Group" << i << "count invalid. Expected:" << end.index[i] << "Actual:" << it.index[i];
            valid = false;
        }
    }
    return valid;
}
#endif

#if defined(QT_DECLARATIVE_VERIFY_MINIMAL)
#   define QT_DECLARATIVE_VERIFY_LISTCOMPOSITOR Q_ASSERT(!(!(qt_verifyIntegrity(iterator(m_ranges.next, 0, Default, m_groupCount), m_end, m_cacheIt) \
            && qt_verifyMinimal(iterator(m_ranges.next, 0, Default, m_groupCount), m_end)) \
            && qt_printInfo(*this)));
#elif defined(QT_DECLARATIVE_VERIFY_INTEGRITY)
#   define QT_DECLARATIVE_VERIFY_LISTCOMPOSITOR Q_ASSERT(!(!qt_verifyIntegrity(iterator(m_ranges.next, 0, Default, m_groupCount), m_end, m_cacheIt) \
            && qt_printInfo(*this)));
#else
#   define QT_DECLARATIVE_VERIFY_LISTCOMPOSITOR
#endif

//#define QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(args) qDebug() << m_end.index[1] << m_end.index[0] << Q_FUNC_INFO args;
#define QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(args)

QDeclarativeListCompositor::iterator &QDeclarativeListCompositor::iterator::operator +=(int difference)
{
    Q_ASSERT(difference >= 0);
    while (!(range->flags & groupFlag)) {
        incrementIndexes(range->count - offset);
        offset = 0;
        range = range->next;
    }
    decrementIndexes(offset);
    offset += difference;
    while (offset >= range->count || !(range->flags & groupFlag)) {
        if (range->flags & groupFlag)
            offset -= range->count;
        incrementIndexes(range->count);
        range = range->next;
    }
    incrementIndexes(offset);
    return *this;
}

QDeclarativeListCompositor::iterator &QDeclarativeListCompositor::iterator::operator -=(int difference)
{
    Q_ASSERT(difference >= 0);
    while (!(range->flags & groupFlag)) {
        decrementIndexes(offset);
        range = range->previous;
        offset = range->count;
    }
    decrementIndexes(offset);
    offset -= difference;
    while (offset < 0) {
        range = range->previous;
        if (range->flags & groupFlag)
            offset += range->count;
        decrementIndexes(range->count);
    }
    incrementIndexes(offset);
    return *this;
}

QDeclarativeListCompositor::insert_iterator &QDeclarativeListCompositor::insert_iterator::operator +=(int difference)
{
    Q_ASSERT(difference >= 0);
    while (!(range->flags & groupFlag) && (range->flags & (GroupMask | CacheFlag))) {
        incrementIndexes(range->count - offset);
        offset = 0;
        range = range->next;
    }
    decrementIndexes(offset);
    offset += difference;
    while (offset > range->count
            || (offset == range->count && !range->append() && offset > 0)
            || (!(range->flags & groupFlag) && offset > 0)) {
        if (range->flags & groupFlag)
            offset -= range->count;
        incrementIndexes(range->count);
        range = range->next;
    }
    incrementIndexes(offset);
    return *this;
}

QDeclarativeListCompositor::insert_iterator &QDeclarativeListCompositor::insert_iterator::operator -=(int difference)
{
    Q_ASSERT(difference >= 0);
    while (!(range->flags & groupFlag) && (range->flags & (GroupMask | CacheFlag))) {
        decrementIndexes(offset);
        range = range->previous;
        offset = range->count;
    }
    decrementIndexes(offset);
    offset -= difference;
    while (offset < 0) {
        range = range->previous;
        if (range->flags & groupFlag)
            offset += range->count;
        decrementIndexes(range->count);
    }
    incrementIndexes(offset);
    for (Range *previous = range->previous; offset == 0 && previous->prepend(); previous = previous->previous) {
        if (previous->append() && previous->inGroup()) {
            offset = previous->count;
            range = previous;
        } else if (!previous->inGroup()) {
            break;
        }
    }

    return *this;
}

QDeclarativeListCompositor::QDeclarativeListCompositor()
    : m_end(m_ranges.next, 0, Default, 2)
    , m_cacheIt(m_end)
    , m_groupCount(2)
    , m_defaultFlags(PrependFlag | DefaultFlag)
    , m_removeFlags(AppendFlag | PrependFlag | GroupMask)
{
}

QDeclarativeListCompositor::~QDeclarativeListCompositor()
{
    for (Range *next, *range = m_ranges.next; range != &m_ranges; range = next) {
        next = range->next;
        delete range;
    }
}

inline QDeclarativeListCompositor::Range *QDeclarativeListCompositor::insert(
        Range *before, void *list, int index, int count, int flags)
{
    return new Range(before, list, index, count, flags);
}

inline QDeclarativeListCompositor::Range *QDeclarativeListCompositor::erase(
        Range *range)
{
    Range *next = range->next;
    next->previous = range->previous;
    next->previous->next = range->next;
    delete range;
    return next;
}

void QDeclarativeListCompositor::setGroupCount(int count)
{
    m_groupCount = count;
    m_end = iterator(&m_ranges, 0, Default, m_groupCount);
    m_cacheIt = m_end;
}

int QDeclarativeListCompositor::count(Group group) const
{
    return m_end.index[group];
}

QDeclarativeListCompositor::iterator QDeclarativeListCompositor::find(Group group, int index)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< group << index)
    Q_ASSERT(index >=0 && index < count(group));
    if (m_cacheIt == m_end) {
        m_cacheIt = iterator(m_ranges.next, 0, group, m_groupCount);
        m_cacheIt += index;
    } else {
        const int offset = index - m_cacheIt.index[group];
        m_cacheIt.setGroup(group);
        if (offset > 0) {
            m_cacheIt += offset;
        } else if (offset < 0) {
            m_cacheIt -= -offset;
        } else if (offset == 0) {
            m_cacheIt -= 0;
            m_cacheIt += 0;
        }
    }
    Q_ASSERT(m_cacheIt.index[group] == index);
    Q_ASSERT(m_cacheIt->inGroup(group));
    QT_DECLARATIVE_VERIFY_LISTCOMPOSITOR
    return m_cacheIt;
}

QDeclarativeListCompositor::iterator QDeclarativeListCompositor::find(Group group, int index) const
{
    return const_cast<QDeclarativeListCompositor *>(this)->find(group, index);
}

QDeclarativeListCompositor::insert_iterator QDeclarativeListCompositor::findInsertPosition(Group group, int index)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< group << index)
    Q_ASSERT(index >=0 && index <= count(group));
    insert_iterator it;
    if (m_cacheIt == m_end) {
        it = iterator(m_ranges.next, 0, group, m_groupCount);
        it += index;
    } else {
        const int offset = index - m_cacheIt.index[group];
        it = m_cacheIt;
        it.setGroup(group);
        if (offset > 0) {
            it += offset;
        } else if (offset < 0) {
            it -= -offset;
        } else if (offset == 0) {
            it -= 0;
            it += 0;
        }
    }
    Q_ASSERT(it.index[group] == index);
    return it;
}

QDeclarativeListCompositor::iterator QDeclarativeListCompositor::begin(Group group)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< group)
    m_cacheIt = iterator(m_ranges.next, 0, group, m_groupCount);
    m_cacheIt += 0;
    return m_cacheIt;
}

void QDeclarativeListCompositor::append(
        void *list, int index, int count, int flags, QVector<Insert> *inserts)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< list << index << count << flags)
    insert(m_end, list, index, count, flags, inserts);
}

void QDeclarativeListCompositor::insert(
        Group group, int before, void *list, int index, int count, int flags, QVector<Insert> *inserts)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< group << before << list << index << count << flags)
    insert(findInsertPosition(group, before), list, index, count, flags, inserts);
}

QDeclarativeListCompositor::iterator QDeclarativeListCompositor::insert(
        iterator before, void *list, int index, int count, int flags, QVector<Insert> *inserts)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< before << list << index << count << flags)
    if (inserts) {
        inserts->append(Insert(before, count, flags & GroupMask));
    }
    if (before.offset > 0) {
        *before = insert(
                *before, before->list, before->index, before.offset, before->flags & ~AppendFlag)->next;
        before->index += before.offset;
        before->count -= before.offset;
        before.offset = 0;
    }

    if (!(flags & AppendFlag) && *before != m_ranges.next
            && before->previous->list == list
            && before->previous->flags == flags
            && (!list || before->previous->end() == index)) {
        before->previous->count += count;
        before.incrementIndexes(count, flags);
    } else {
        *before = insert(*before, list, index, count, flags);
        before.offset = 0;
    }

    if (!(flags & AppendFlag) && before->next != &m_ranges
            && before->list == before->next->list
            && before->flags == before->next->flags
            && (!list || before->end() == before->next->index)) {
        before->next->index = before->index;
        before->next->count += before->count;
        *before = erase(*before);
    }

    m_end.incrementIndexes(count, flags);
    m_cacheIt = before;
    QT_DECLARATIVE_VERIFY_LISTCOMPOSITOR
    return before;
}

void QDeclarativeListCompositor::setFlags(
        Group group, int index, int count, int flags, QVector<Insert> *inserts)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< group << index << count << flags)
    setFlags(find(group, index), count, flags, inserts);
}

void QDeclarativeListCompositor::setFlags(
        iterator from, int count, int flags, QVector<Insert> *inserts)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< from << count << flags)
    if (!flags || !count)
        return;

    if (from.offset > 0) {
        *from = insert(*from, from->list, from->index, from.offset, from->flags & ~AppendFlag)->next;
        from->index += from.offset;
        from->count -= from.offset;
        from.offset = 0;
    }

    for (; count > 0; *from = from->next) {
        if (from != from.group) {
            from.incrementIndexes(from->count);
            continue;
        }
        const int difference = qMin(count, from->count);
        count -= difference;

        const int insertFlags = ~from->flags & flags;
        const int setFlags = (from->flags | flags) & ~AppendFlag;
        if (insertFlags && inserts)
            inserts->append(Insert(from, difference, insertFlags | (from->flags & CacheFlag)));
        m_end.incrementIndexes(difference, insertFlags);
        from.incrementIndexes(difference, setFlags);

        if (from->previous != &m_ranges
                && from->previous->list == from->list
                && (!from->list || from->previous->end() == from->index)
                && from->previous->flags == setFlags) {
            from->previous->count += difference;
            from->index += difference;
            from->count -= difference;
            if (from->count == 0) {
                if (from->append())
                    from->previous->flags |= AppendFlag;
                *from = erase(*from)->previous;
                continue;
            } else {
                break;
            }
        } else if (!insertFlags) {
            from.incrementIndexes(from->count - difference);
            continue;
        } else if (difference < from->count) {
            *from = insert(*from, from->list, from->index, difference, setFlags)->next;
            from->index += difference;
            from->count -= difference;
        } else {
            from->flags |= flags;
            continue;
        }
        from.incrementIndexes(from->count);
    }

    if (from->previous != &m_ranges
            && from->previous->list == from->list
            && (!from->list || from->previous->end() == from->index)
            && from->previous->flags == (from->flags & ~AppendFlag)) {
        from.offset = from->previous->count;
        from->previous->count += from->count;
        from->previous->flags = from->flags;
        *from = erase(*from)->previous;
    }
    m_cacheIt = from;
    QT_DECLARATIVE_VERIFY_LISTCOMPOSITOR
}

void QDeclarativeListCompositor::clearFlags(
        Group group, int index, int count, int flags, QVector<Remove> *removes)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< group << index << count << flags)
    clearFlags(find(group, index), count, flags, removes);
}

void QDeclarativeListCompositor::clearFlags(
        iterator from, int count, int flags, QVector<Remove> *removes)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< from << count << flags)
    if (!flags || !count)
        return;

    const bool clearCache = flags & CacheFlag;

    if (from.offset > 0) {
        *from = insert(*from, from->list, from->index, from.offset, from->flags & ~AppendFlag)->next;
        from->index += from.offset;
        from->count -= from.offset;
        from.offset = 0;
    }

    for (; count > 0; *from = from->next) {
        if (from != from.group) {
            from.incrementIndexes(from->count);
            continue;
        }
        const int difference = qMin(count, from->count);
        count -= difference;

        const int removeFlags = from->flags & flags & ~(AppendFlag | PrependFlag);
        const int clearedFlags = from->flags & ~(flags | AppendFlag);
        if (removeFlags && removes) {
            const int maskedFlags = clearCache
                    ? (removeFlags & ~CacheFlag)
                    : (removeFlags | (from->flags & CacheFlag));
            if (maskedFlags)
                removes->append(Remove(from, difference, maskedFlags));
        }
        m_end.decrementIndexes(difference, removeFlags);
        from.incrementIndexes(difference, clearedFlags);

        if (from->previous != &m_ranges
                && from->previous->list == from->list
                && (!from->list || clearedFlags == CacheFlag || from->previous->end() == from->index)
                && from->previous->flags == clearedFlags) {
            from->previous->count += difference;
            from->index += difference;
            from->count -= difference;
            if (from->count == 0) {
                if (from->append())
                    from->previous->flags |= AppendFlag;
                *from = erase(*from)->previous;
            } else {
                from.incrementIndexes(from->count);
            }
        } else if (difference < from->count) {
            if (clearedFlags)
                *from = insert(*from, from->list, from->index, difference, clearedFlags)->next;
            from->index += difference;
            from->count -= difference;
            from.incrementIndexes(from->count);
        } else if (clearedFlags) {
            from->flags &= ~flags;
        } else {
            *from = erase(*from)->previous;
        }
    }

    if (*from != &m_ranges && from->previous != &m_ranges
            && from->previous->list == from->list
            && (!from->list || from->previous->end() == from->index)
            && from->previous->flags == (from->flags & ~AppendFlag)) {
        from.offset = from->previous->count;
        from->previous->count += from->count;
        from->previous->flags = from->flags;
        *from = erase(*from)->previous;
    }
    m_cacheIt = from;
    QT_DECLARATIVE_VERIFY_LISTCOMPOSITOR
}

void QDeclarativeListCompositor::removeList(void *list, QVector<Remove> *removes, bool destroyed)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< list << destroyed)
    for (iterator it(m_ranges.next, 0, Default, m_groupCount); *it != &m_ranges; *it = it->next) {
        if (it->list == list) {
            const int flags = it->flags & (GroupMask | CacheFlag);
            if (flags) {
                removes->append(Remove(it, it->count, flags));
                m_end.decrementIndexes(it->count, flags);
            }
            if (destroyed)
                it->list = 0;
            if (it->inCache()) {
                it->flags = CacheFlag;
                it.cacheIndex += it->count;
            } else {
                *it = erase(*it)->previous;
            }
        } else {
            it.incrementIndexes(it->count);
        }
    }
    m_cacheIt = m_end;
    QT_DECLARATIVE_VERIFY_LISTCOMPOSITOR
}

bool QDeclarativeListCompositor::verifyMoveTo(
        Group fromGroup, int from, Group toGroup, int to, int count) const
{
    if (fromGroup != toGroup) {
        // determine how many items from the destination group intersect with the source group.
        iterator fromIt = find(fromGroup, from);

        int intersectingCount = 0;

        for (; count > 0; *fromIt = fromIt->next) {
            if (*fromIt == &m_ranges)
                return false;
            if (!fromIt->inGroup(fromGroup))
                continue;
            if (fromIt->inGroup(toGroup))
                intersectingCount += qMin(count, fromIt->count - fromIt.offset);
            count -= fromIt->count - fromIt.offset;
            fromIt.offset = 0;
        }
        count = intersectingCount;
    }

    return to >= 0 && to + count <= m_end.index[toGroup];
}

void QDeclarativeListCompositor::move(
        Group fromGroup,
        int from,
        Group toGroup,
        int to,
        int count,
        QVector<Remove> *removes,
        QVector<Insert> *inserts)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< fromGroup << from << toGroup << to << count)
    Q_ASSERT(count != 0);
    Q_ASSERT(from >=0 && from + count <= m_end.index[toGroup]);
    Q_ASSERT(verifyMoveTo(fromGroup, from, toGroup, to, count));

    iterator fromIt = find(fromGroup, from);
    if (fromIt.offset > 0) {
        *fromIt = insert(
                *fromIt, fromIt->list, fromIt->index, fromIt.offset, fromIt->flags & ~AppendFlag)->next;
        fromIt->index += fromIt.offset;
        fromIt->count -= fromIt.offset;
        fromIt.offset = 0;
    }

    Range movedFlags;
    for (int moveId = 0; count > 0;) {
        if (fromIt != fromIt.group) {
            fromIt.incrementIndexes(fromIt->count);
            *fromIt = fromIt->next;
            continue;
        }
        int difference = qMin(count, fromIt->count);

        new Range(
                &movedFlags,
                fromIt->list,
                fromIt->index,
                difference,
                fromIt->flags & ~(PrependFlag | AppendFlag));
        if (removes)
            removes->append(Remove(fromIt, difference, fromIt->flags, moveId++));
        count -= difference;
        fromIt->count -= difference;

        int removeIndex = fromIt->index;
        if (fromIt->prepend()
                && fromIt->previous != &m_ranges
                && fromIt->previous->flags == PrependFlag
                && fromIt->previous->list == fromIt->list
                && fromIt->previous->end() == fromIt->index) {
            fromIt->previous->count += difference;
        } else if (fromIt->prepend()) {
            *fromIt = insert(*fromIt, fromIt->list, removeIndex, difference, PrependFlag)->next;
        }
        fromIt->index += difference;

        if (fromIt->count == 0) {
            if (fromIt->append())
                fromIt->previous->flags |= AppendFlag;
            *fromIt = erase(*fromIt);

            if (*fromIt != m_ranges.next && fromIt->flags == PrependFlag
                    && fromIt->previous != &m_ranges
                    && fromIt->previous->flags == PrependFlag
                    && fromIt->previous->list == fromIt->list
                    && fromIt->previous->end() == fromIt->index) {
                fromIt.incrementIndexes(fromIt->count);
                fromIt->previous->count += fromIt->count;
                *fromIt = erase(*fromIt);
            }
        } else if (count > 0) {
            *fromIt = fromIt->next;
        }
    }

    if (*fromIt != m_ranges.next
            && *fromIt != &m_ranges
            && fromIt->previous->list == fromIt->list
            && (!fromIt->list || fromIt->previous->end() == fromIt->index)
            && fromIt->previous->flags == (fromIt->flags & ~AppendFlag)) {
        if (fromIt == fromIt.group)
            fromIt.offset = fromIt->previous->count;
        fromIt.offset = fromIt->previous->count;
        fromIt->previous->count += fromIt->count;
        fromIt->previous->flags = fromIt->flags;
        *fromIt = erase(*fromIt)->previous;
    }

    insert_iterator toIt = fromIt;
    toIt.setGroup(toGroup);
    const int difference = to - toIt.index[toGroup];
    if (difference > 0)
        toIt += difference;
    else
        toIt -= -difference;

    if (toIt.offset > 0) {
        *toIt = insert(*toIt, toIt->list, toIt->index, toIt.offset, toIt->flags & ~AppendFlag)->next;
        toIt->index += toIt.offset;
        toIt->count -= toIt.offset;
        toIt.offset = 0;
    }

    for (Range *range = movedFlags.previous; range != &movedFlags; range = range->previous) {
        if (*toIt != &m_ranges
                && range->list == toIt->list
                && (!range->list || range->end() == toIt->index)
                && range->flags == (toIt->flags & ~AppendFlag)) {
            toIt->index -= range->count;
            toIt->count += range->count;
        } else {
            *toIt = insert(*toIt, range->list, range->index, range->count, range->flags);
        }
    }

    if (*toIt != m_ranges.next
            && toIt->previous->list == toIt->list
            && (!toIt->list || (toIt->previous->end() == toIt->index && toIt->previous->flags == (toIt->flags & ~AppendFlag)))) {
        toIt.offset = toIt->previous->count;
        toIt->previous->count += toIt->count;
        toIt->previous->flags = toIt->flags;
        *toIt = erase(*toIt)->previous;
    }
    Insert insert(toIt, 0, 0, 0);
    for (Range *next, *range = movedFlags.next; range != &movedFlags; range = next) {
        insert.count = range->count;
        insert.flags = range->flags;
        if (inserts)
            inserts->append(insert);
        for (int i = 0; i < m_groupCount; ++i) {
            if (insert.inGroup(i))
                insert.index[i] += range->count;
        }
        ++insert.moveId;
        next = range->next;
        delete range;
    }

    m_cacheIt = toIt;
    QT_DECLARATIVE_VERIFY_LISTCOMPOSITOR
}

void QDeclarativeListCompositor::clear()
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR( )
    for (Range *range = m_ranges.next; range != &m_ranges; range = erase(range)) {}
    m_end = iterator(m_ranges.next, 0, Default, m_groupCount);
    m_cacheIt = m_end;
}

void QDeclarativeListCompositor::listItemsInserted(
        QVector<Insert> *translatedInsertions,
        void *list,
        const QVector<QDeclarativeChangeSet::Insert> &insertions,
        const QVector<MovedFlags> *movedFlags)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< list << insertions)
    for (iterator it(m_ranges.next, 0, Default, m_groupCount); *it != &m_ranges; *it = it->next) {
        if (it->list != list || it->flags == CacheFlag) {
            it.incrementIndexes(it->count);
            continue;
        } else if (it->flags & MovedFlag) {
            it->flags &= ~MovedFlag;
            it.incrementIndexes(it->count);
            continue;
        }
        foreach (const QDeclarativeChangeSet::Insert &insertion, insertions) {
            int offset = insertion.index - it->index;
            if ((offset > 0 && offset < it->count)
                    || (offset == 0 && it->prepend())
                    || (offset == it->count && it->append())) {
                if (it->prepend()) {
                    int flags = m_defaultFlags;
                    if (insertion.isMove()) {
                        for (QVector<MovedFlags>::const_iterator move = movedFlags->begin();
                                move != movedFlags->end();
                                ++move) {
                            if (move->moveId == insertion.moveId) {
                                flags = move->flags;
                                break;
                            }
                        }
                    }
                    if (flags & ~(AppendFlag | PrependFlag)) {
                        Insert translatedInsert(it, insertion.count, flags, insertion.moveId);
                        for (int i = 0; i < m_groupCount; ++i) {
                            if (it->inGroup(i))
                                translatedInsert.index[i] += offset;
                        }
                        translatedInsertions->append(translatedInsert);
                    }
                    if ((it->flags & ~AppendFlag) == flags) {
                        it->count += insertion.count;
                    } else if (offset == 0
                            && it->previous != &m_ranges
                            && it->previous->list == list
                            && it->previous->end() == insertion.index
                            && it->previous->flags == flags) {
                        it->previous->count += insertion.count;
                        it->index += insertion.count;
                        it.incrementIndexes(insertion.count);
                    } else {
                        if (offset > 0) {
                            it.incrementIndexes(offset);
                            *it = insert(*it, it->list, it->index, offset, it->flags & ~AppendFlag)->next;
                        }
                        *it = insert(*it, it->list, insertion.index, insertion.count, flags)->next;
                        it.incrementIndexes(insertion.count, flags);
                        it->index += offset + insertion.count;
                        it->count -= offset;
                    }
                    m_end.incrementIndexes(insertion.count, flags);
                } else {
                    if (offset > 0) {
                        *it = insert(*it, it->list, it->index, offset, it->flags)->next;
                        it->index += offset;
                        it->count -= offset;
                    }
                    it->index += insertion.count;
                }
            } else if (offset <= 0) {
                it->index += insertion.count;
            }
        }
        it.incrementIndexes(it->count);
    }
    m_cacheIt = m_end;
    QT_DECLARATIVE_VERIFY_LISTCOMPOSITOR
}

void QDeclarativeListCompositor::listItemsInserted(
        void *list, int index, int count, QVector<Insert> *translatedInsertions)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< list << index << count)
    Q_ASSERT(count > 0);

    QVector<QDeclarativeChangeSet::Insert> insertions;
    insertions.append(QDeclarativeChangeSet::Insert(index, count));

    listItemsInserted(translatedInsertions, list, insertions);
}

void QDeclarativeListCompositor::listItemsRemoved(
        QVector<Remove> *translatedRemovals,
        void *list,
        QVector<QDeclarativeChangeSet::Remove> *removals,
        QVector<QDeclarativeChangeSet::Insert> *insertions,
        QVector<MovedFlags> *movedFlags, int moveId)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< list << *removals)

    for (iterator it(m_ranges.next, 0, Default, m_groupCount);
            *it != &m_ranges && !removals->isEmpty();
            *it = it->next) {
        if (it->list != list || it->flags == CacheFlag) {
            it.incrementIndexes(it->count);
            continue;
        }
        bool removed = false;
        for (QVector<QDeclarativeChangeSet::Remove>::iterator removal = removals->begin();
                !removed && removal != removals->end();
                ++removal) {
            int relativeIndex = removal->index - it->index;
            int itemsRemoved = removal->count;
            if (relativeIndex + removal->count > 0 && relativeIndex < it->count) {
                const int offset = qMax(0, relativeIndex);
                int removeCount = qMin(it->count, relativeIndex + removal->count) - offset;
                it->count -= removeCount;
                int removeFlags = it->flags & m_removeFlags;
                Remove translatedRemoval(it, removeCount, it->flags);
                for (int i = 0; i < m_groupCount; ++i) {
                    if (it->inGroup(i))
                        translatedRemoval.index[i] += offset;
                }
                if (removal->isMove()) {
                    QVector<QDeclarativeChangeSet::Insert>::iterator insertion = insertions->begin();
                    for (; insertion != insertions->end() && insertion->moveId != removal->moveId;
                            ++insertion) {}
                    Q_ASSERT(insertion != insertions->end());
                    Q_ASSERT(insertion->count == removal->count);

                    if (relativeIndex < 0) {
                        int splitMoveId = ++moveId;
                        removal = removals->insert(removal, QDeclarativeChangeSet::Remove(
                                removal->index, -relativeIndex, splitMoveId));
                        ++removal;
                        removal->count -= -relativeIndex;
                        insertion = insertions->insert(insertion, QDeclarativeChangeSet::Insert(
                                insertion->index, -relativeIndex, splitMoveId));
                        ++insertion;
                        insertion->index += -relativeIndex;
                        insertion->count -= -relativeIndex;
                    }

                    if (it->prepend()) {
                        removeFlags |= it->flags & CacheFlag;
                        translatedRemoval.moveId = ++moveId;
                        movedFlags->append(MovedFlags(moveId, it->flags & ~AppendFlag));

                        removal = removals->insert(removal, QDeclarativeChangeSet::Remove(
                                removal->index, removeCount, translatedRemoval.moveId));
                        ++removal;
                        insertion = insertions->insert(insertion, QDeclarativeChangeSet::Insert(
                                insertion->index, removeCount, translatedRemoval.moveId));
                        ++insertion;

                        removal->count -= removeCount;
                        insertion->index += removeCount;
                        insertion->count -= removeCount;
                        if (removal->count == 0) {
                            removal = removals->erase(removal);
                            insertion = insertions->erase(insertion);
                            --removal;
                            --insertion;
                        }
                    } else {
                        if (offset > 0) {
                            *it = insert(*it, it->list, it->index, offset, it->flags & ~AppendFlag)->next;
                            it->index += offset;
                            it->count -= offset;
                            it.incrementIndexes(offset);
                        }
                        if (it->previous != &m_ranges
                                && it->previous->list == it->list
                                && it->end() == insertion->index
                                && it->previous->flags == (it->flags | MovedFlag)) {
                            it->previous->count += removeCount;
                        } else {
                            *it = insert(*it, it->list, insertion->index, removeCount, it->flags | MovedFlag)->next;
                        }
                        translatedRemoval.flags = 0;
                        removeFlags = 0;
                    }
                } else if (it->inCache()) {
                    if (offset > 0) {
                        *it = insert(*it, it->list, it->index, offset, it->flags & ~AppendFlag)->next;
                        it->index += offset;
                        it->count -= offset;
                        it.incrementIndexes(offset);
                    }
                    if (it->previous != &m_ranges
                            && it->previous->list == it->list
                            && it->previous->flags == CacheFlag) {
                        it->previous->count += removeCount;
                    } else {
                        *it = insert(*it, it->list, -1, removeCount, CacheFlag)->next;
                    }
                    it.index[Cache] += removeCount;
                }
                if (removeFlags & GroupMask)
                    translatedRemovals->append(translatedRemoval);
                m_end.decrementIndexes(removeCount, removeFlags);
                if (it->count == 0 && !it->append()) {
                    *it = erase(*it)->previous;
                    removed = true;
                } else if (relativeIndex <= 0) {
                    it->index = removal->index;
                }
            } else if (relativeIndex < 0) {
                it->index -= itemsRemoved;

                if (it->previous != &m_ranges
                        && it->previous->list == it->list
                        && it->previous->end() == it->index
                        && it->previous->flags == (it->flags & ~AppendFlag)) {
                    it->previous->count += it->count;
                    it->previous->flags = it->flags;
                    it.incrementIndexes(it->count);
                    *it = erase(*it);
                    removed = true;
                }
            }
        }
        if (it->flags == CacheFlag && it->next->flags == CacheFlag && it->next->list == it->list) {
            it.index[Cache] += it->next->count;
            it->count += it->next->count;
            erase(it->next);
        } else if (!removed) {
            it.incrementIndexes(it->count);
        }
    }
    m_cacheIt = m_end;
    QT_DECLARATIVE_VERIFY_LISTCOMPOSITOR
}

void QDeclarativeListCompositor::listItemsRemoved(
        void *list, int index, int count, QVector<Remove> *translatedRemovals)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< list << index << count)
    Q_ASSERT(count >= 0);

    QVector<QDeclarativeChangeSet::Remove> removals;
    removals.append(QDeclarativeChangeSet::Remove(index, count));
    listItemsRemoved(translatedRemovals, list, &removals, 0, 0, 0);
}

void QDeclarativeListCompositor::listItemsMoved(
        void *list,
        int from,
        int to,
        int count,
        QVector<Remove> *translatedRemovals,
        QVector<Insert> *translatedInsertions)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< list << from << to << count)
    Q_ASSERT(count >= 0);

    QVector<QDeclarativeChangeSet::Remove> removals;
    QVector<QDeclarativeChangeSet::Insert> insertions;
    QVector<MovedFlags> movedFlags;
    removals.append(QDeclarativeChangeSet::Remove(from, count, 0));
    insertions.append(QDeclarativeChangeSet::Insert(to, count, 0));

    listItemsRemoved(translatedRemovals, list, &removals, &insertions, &movedFlags, 0);
    listItemsInserted(translatedInsertions, list, insertions, &movedFlags);
}

void QDeclarativeListCompositor::listItemsChanged(
        QVector<Change> *translatedChanges,
        void *list,
        const QVector<QDeclarativeChangeSet::Change> &changes)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< list << changes)
    for (iterator it(m_ranges.next, 0, Default, m_groupCount); *it != &m_ranges; *it = it->next) {
        if (it->list != list || it->flags == CacheFlag) {
            it.incrementIndexes(it->count);
            continue;
        } else if (!it->inGroup()) {
            continue;
        }
        foreach (const QDeclarativeChangeSet::Change &change, changes) {
            const int offset = change.index - it->index;
            if (offset + change.count > 0 && offset < it->count) {
                const int changeOffset = qMax(0, offset);
                const int changeCount = qMin(it->count, offset + change.count) - changeOffset;

                Change translatedChange(it, changeCount, it->flags);
                for (int i = 0; i < m_groupCount; ++i) {
                    if (it->inGroup(i))
                        translatedChange.index[i] += changeOffset;
                }
                translatedChanges->append(translatedChange);
            }
        }
        it.incrementIndexes(it->count);
    }
}

void QDeclarativeListCompositor::listItemsChanged(
        void *list, int index, int count, QVector<Change> *translatedChanges)
{
    QT_DECLARATIVE_TRACE_LISTCOMPOSITOR(<< list << index << count)
    Q_ASSERT(count >= 0);
    QVector<QDeclarativeChangeSet::Change> changes;
    changes.append(QDeclarativeChangeSet::Change(index, count));
    listItemsChanged(translatedChanges, list, changes);
}

void QDeclarativeListCompositor::listChanged(
        void *list,
        const QDeclarativeChangeSet &changeSet,
        QVector<Remove> *translatedRemovals,
        QVector<Insert> *translatedInsertions,
        QVector<Change> *translatedChanges)
{
    QVector<QDeclarativeChangeSet::Remove> removals = changeSet.removes();
    QVector<QDeclarativeChangeSet::Insert> insertions = changeSet.inserts();
    QVector<MovedFlags> movedFlags;
    listItemsRemoved(translatedRemovals, list, &removals, &insertions, &movedFlags, changeSet.moveCounter());
    listItemsInserted(translatedInsertions, list, insertions, &movedFlags);
    listItemsChanged(translatedChanges, list, changeSet.changes());
}

void QDeclarativeListCompositor::transition(
        Group from,
        Group to,
        QVector<QDeclarativeChangeSet::Remove> *removes,
        QVector<QDeclarativeChangeSet::Insert> *inserts)
{
    int removeCount = 0;
    for (iterator it(m_ranges.next, 0, Default, m_groupCount); *it != &m_ranges; *it = it->next) {
        if (it == from && it != to) {
            removes->append(QDeclarativeChangeSet::Remove(it.index[from]- removeCount, it->count));
            removeCount += it->count;
        } else if (it != from && it == to) {
            inserts->append(QDeclarativeChangeSet::Insert(it.index[to], it->count));
        }
        it.incrementIndexes(it->count);
    }
}

QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::Group &group)
{
    switch (group) {
    case QDeclarativeListCompositor::Cache:   return debug << "Cache";
    case QDeclarativeListCompositor::Default: return debug << "Default";
    default: return (debug.nospace() << "Group" << int(group)).space();
    }

}

QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::Range &range)
{
    (debug.nospace()
            << "Range("
            << range.list) << " "
            << range.index << " "
            << range.count << " "
            << (range.append() ? "A" : "0")
            << (range.prepend() ? "P" : "0");
    for (int i = QDeclarativeListCompositor::MaximumGroupCount - 1; i >= 2; --i)
        debug << (range.inGroup(i) ? "1" : "0");
    return (debug
            << (range.inGroup(QDeclarativeListCompositor::Default) ? "D" : "0")
            << (range.inGroup(QDeclarativeListCompositor::Cache) ? "C" : "0"));
}

static void qt_print_indexes(QDebug &debug, int count, const int *indexes)
{
    for (int i = count - 1; i >= 0; --i)
        debug << indexes[i];
}

QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::iterator &it)
{
    (debug.nospace() << "iterator(" << it.group).space() << "offset:" << it.offset;
    qt_print_indexes(debug, it.groupCount, it.index);
    return ((debug << **it).nospace() << ")").space();
}

static QDebug qt_print_change(QDebug debug, const char *name, const QDeclarativeListCompositor::Change &change)
{
    debug.nospace() << name << "(" << change.moveId << " " << change.count << " ";
    for (int i = QDeclarativeListCompositor::MaximumGroupCount - 1; i >= 2; --i)
        debug << (change.inGroup(i) ? "1" : "0");
    debug << (change.inGroup(QDeclarativeListCompositor::Default) ? "D" : "0")
            << (change.inGroup(QDeclarativeListCompositor::Cache) ? "C" : "0");
    int i = QDeclarativeListCompositor::MaximumGroupCount - 1;
    for (; i >= 0 && !change.inGroup(i); --i) {}
    for (; i >= 0; --i)
        debug << " " << change.index[i];
    return (debug << ")").maybeSpace();
}

QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::Change &change)
{
    return qt_print_change(debug, "Change", change);
}

QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::Remove &remove)
{
    return qt_print_change(debug, "Remove", remove);
}

QDebug operator <<(QDebug debug, const QDeclarativeListCompositor::Insert &insert)
{
    return qt_print_change(debug, "Insert", insert);
}

QDebug operator <<(QDebug debug, const QDeclarativeListCompositor &list)
{
    int indexes[QDeclarativeListCompositor::MaximumGroupCount];
    for (int i = 0; i < QDeclarativeListCompositor::MaximumGroupCount; ++i)
        indexes[i] = 0;
    debug.nospace() << "QDeclarativeListCompositor(";
    qt_print_indexes(debug, list.m_groupCount, list.m_end.index);
    for (QDeclarativeListCompositor::Range *range = list.m_ranges.next; range != &list.m_ranges; range = range->next) {
        (debug << "\n").space();
        qt_print_indexes(debug, list.m_groupCount, indexes);
        debug << " " << *range;

        for (int i = 0; i < list.m_groupCount; ++i) {
            if (range->inGroup(i))
                indexes[i] += range->count;
        }
    }
    return (debug << ")").maybeSpace();
}

QT_END_NAMESPACE
