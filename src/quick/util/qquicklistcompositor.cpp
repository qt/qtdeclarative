/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquicklistcompositor_p.h"

#include <QtCore/qvarlengtharray.h>

//#define QT_QML_VERIFY_MINIMAL
//#define QT_QML_VERIFY_INTEGRITY

QT_BEGIN_NAMESPACE

#ifdef QT_QML_VERIFY_MINIMAL
#define QT_QML_VERIFY_INTEGRITY
static bool qt_verifyMinimal(
        const QQuickListCompositor::iterator &begin,
        const QQuickListCompositor::iterator &end)
{
    bool minimal = true;
    int index = 0;

    for (const QQuickListCompositor::Range *range = begin->next; range != *end; range = range->next, ++index) {
        if (range->previous->list == range->list
                && range->previous->flags == (range->flags & ~QQuickListCompositor::AppendFlag)
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

#ifdef QT_QML_VERIFY_INTEGRITY
static bool qt_printInfo(const QQuickListCompositor &compositor)
{
    qWarning() << compositor;
    return true;
}

static bool qt_verifyIntegrity(
        const QQuickListCompositor::iterator &begin,
        const QQuickListCompositor::iterator &end,
        const QQuickListCompositor::iterator &cachedIt)
{
    bool valid = true;

    int index = 0;
    QQuickListCompositor::iterator it;
    for (it = begin; *it != *end; *it = it->next) {
        if (it->count == 0 && !it->append()) {
            qWarning() << index << "Empty non-append range";
            valid = false;
        }
        if (it->count < 0) {
            qWarning() << index << "Negative count";
            valid = false;
        }
        if (it->list && it->flags != QQuickListCompositor::CacheFlag && it->index < 0) {
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
                            << QQuickListCompositor::Group(i)
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

#if defined(QT_QML_VERIFY_MINIMAL)
#   define QT_QML_VERIFY_LISTCOMPOSITOR Q_ASSERT(!(!(qt_verifyIntegrity(iterator(m_ranges.next, 0, Default, m_groupCount), m_end, m_cacheIt) \
            && qt_verifyMinimal(iterator(m_ranges.next, 0, Default, m_groupCount), m_end)) \
            && qt_printInfo(*this)));
#elif defined(QT_QML_VERIFY_INTEGRITY)
#   define QT_QML_VERIFY_LISTCOMPOSITOR Q_ASSERT(!(!qt_verifyIntegrity(iterator(m_ranges.next, 0, Default, m_groupCount), m_end, m_cacheIt) \
            && qt_printInfo(*this)));
#else
#   define QT_QML_VERIFY_LISTCOMPOSITOR
#endif

//#define QT_QML_TRACE_LISTCOMPOSITOR(args) qDebug() << m_end.index[1] << m_end.index[0] << Q_FUNC_INFO args;
#define QT_QML_TRACE_LISTCOMPOSITOR(args)

QQuickListCompositor::iterator &QQuickListCompositor::iterator::operator +=(int difference)
{
    // Update all indexes to the start of the range.
    decrementIndexes(offset);

    // If the iterator group isn't a member of the current range ignore the current offset.
    if (!(range->flags & groupFlag))
        offset = 0;

    offset += difference;

    // Iterate backwards looking for a range with a positive offset.
    while (offset <= 0 && range->previous->flags) {
        range = range->previous;
        if (range->flags & groupFlag)
            offset += range->count;
        decrementIndexes(range->count);
    }

    // Iterate forwards looking for the first range which contains both the offset and the
    // iterator group.
    while (range->flags && (offset >= range->count || !(range->flags & groupFlag))) {
        if (range->flags & groupFlag)
            offset -= range->count;
        incrementIndexes(range->count);
        range = range->next;
    }

    // Update all the indexes to inclue the remaining offset.
    incrementIndexes(offset);

    return *this;
}

QQuickListCompositor::insert_iterator &QQuickListCompositor::insert_iterator::operator +=(int difference)
{
    iterator::operator +=(difference);

    // If the previous range contains the append flag move the iterator to the tail of the previous
    // range so that appended appear after the insert position.
    if (offset == 0 && range->previous->append()) {
        range = range->previous;
        offset = range->inGroup() ? range->count : 0;
    }

    return *this;
}

QQuickListCompositor::QQuickListCompositor()
    : m_end(m_ranges.next, 0, Default, 2)
    , m_cacheIt(m_end)
    , m_groupCount(2)
    , m_defaultFlags(PrependFlag | DefaultFlag)
    , m_removeFlags(AppendFlag | PrependFlag | GroupMask)
{
}

QQuickListCompositor::~QQuickListCompositor()
{
    for (Range *next, *range = m_ranges.next; range != &m_ranges; range = next) {
        next = range->next;
        delete range;
    }
}

inline QQuickListCompositor::Range *QQuickListCompositor::insert(
        Range *before, void *list, int index, int count, uint flags)
{
    return new Range(before, list, index, count, flags);
}

inline QQuickListCompositor::Range *QQuickListCompositor::erase(
        Range *range)
{
    Range *next = range->next;
    next->previous = range->previous;
    next->previous->next = range->next;
    delete range;
    return next;
}

void QQuickListCompositor::setGroupCount(int count)
{
    m_groupCount = count;
    m_end = iterator(&m_ranges, 0, Default, m_groupCount);
    m_cacheIt = m_end;
}

int QQuickListCompositor::count(Group group) const
{
    return m_end.index[group];
}

QQuickListCompositor::iterator QQuickListCompositor::find(Group group, int index)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< group << index)
    Q_ASSERT(index >=0 && index < count(group));
    if (m_cacheIt == m_end) {
        m_cacheIt = iterator(m_ranges.next, 0, group, m_groupCount);
        m_cacheIt += index;
    } else {
        const int offset = index - m_cacheIt.index[group];
        m_cacheIt.setGroup(group);
        m_cacheIt += offset;
    }
    Q_ASSERT(m_cacheIt.index[group] == index);
    Q_ASSERT(m_cacheIt->inGroup(group));
    QT_QML_VERIFY_LISTCOMPOSITOR
    return m_cacheIt;
}

QQuickListCompositor::iterator QQuickListCompositor::find(Group group, int index) const
{
    return const_cast<QQuickListCompositor *>(this)->find(group, index);
}

QQuickListCompositor::insert_iterator QQuickListCompositor::findInsertPosition(Group group, int index)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< group << index)
    Q_ASSERT(index >=0 && index <= count(group));
    insert_iterator it;
    if (m_cacheIt == m_end) {
        it = iterator(m_ranges.next, 0, group, m_groupCount);
        it += index;
    } else {
        const int offset = index - m_cacheIt.index[group];
        it = m_cacheIt;
        it.setGroup(group);
        it += offset;
    }
    Q_ASSERT(it.index[group] == index);
    return it;
}

void QQuickListCompositor::append(
        void *list, int index, int count, uint flags, QVector<Insert> *inserts)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< list << index << count << flags)
    insert(m_end, list, index, count, flags, inserts);
}

void QQuickListCompositor::insert(
        Group group, int before, void *list, int index, int count, uint flags, QVector<Insert> *inserts)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< group << before << list << index << count << flags)
    insert(findInsertPosition(group, before), list, index, count, flags, inserts);
}

QQuickListCompositor::iterator QQuickListCompositor::insert(
        iterator before, void *list, int index, int count, uint flags, QVector<Insert> *inserts)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< before << list << index << count << flags)
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
    QT_QML_VERIFY_LISTCOMPOSITOR
    return before;
}

void QQuickListCompositor::setFlags(
        Group fromGroup, int from, int count, Group group, int flags, QVector<Insert> *inserts)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< fromGroup << from << count << group << flags)
    setFlags(find(fromGroup, from), count, group, flags, inserts);
}

void QQuickListCompositor::setFlags(
        iterator from, int count, Group group, uint flags, QVector<Insert> *inserts)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< from << count << flags)
    if (!flags || !count)
        return;

    if (from != group) {
        from.incrementIndexes(from->count - from.offset);
        from.offset = 0;
        *from = from->next;
    } else if (from.offset > 0) {
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

        const uint insertFlags = ~from->flags & flags;
        const uint setFlags = (from->flags | flags) & ~AppendFlag;
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
    QT_QML_VERIFY_LISTCOMPOSITOR
}

void QQuickListCompositor::clearFlags(
        Group fromGroup, int from, int count, Group group, uint flags, QVector<Remove> *removes)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< fromGroup << from << count << group << flags)
    clearFlags(find(fromGroup, from), count, group, flags, removes);
}

void QQuickListCompositor::clearFlags(
        iterator from, int count, Group group, uint flags, QVector<Remove> *removes)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< from << count << flags)
    if (!flags || !count)
        return;

    const bool clearCache = flags & CacheFlag;

    if (from != group) {
        from.incrementIndexes(from->count - from.offset);
        from.offset = 0;
        *from = from->next;
    } else if (from.offset > 0) {
        *from = insert(*from, from->list, from->index, from.offset, from->flags & ~AppendFlag)->next;
        from->index += from.offset;
        from->count -= from.offset;
        from.offset = 0;
    }

    for (; count > 0; *from = from->next) {
        if (from != group) {
            from.incrementIndexes(from->count);
            continue;
        }
        const int difference = qMin(count, from->count);
        count -= difference;

        const uint removeFlags = from->flags & flags & ~(AppendFlag | PrependFlag);
        const uint clearedFlags = from->flags & ~(flags | AppendFlag | UnresolvedFlag);
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
    QT_QML_VERIFY_LISTCOMPOSITOR
}

bool QQuickListCompositor::verifyMoveTo(
        Group fromGroup, int from, Group toGroup, int to, int count, Group group) const
{
    if (group != toGroup) {
        // determine how many items from the destination group intersect with the source group.
        iterator fromIt = find(fromGroup, from);

        int intersectingCount = 0;

        for (; count > 0; *fromIt = fromIt->next) {
            if (*fromIt == &m_ranges)
                return false;
            if (!fromIt->inGroup(group))
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

/*!
    \internal

    Moves \a count items belonging to \a moveGroup from the index \a from in \a fromGroup
    to the index \a to in \a toGroup.

    If \a removes and \a inserts are not null they will be populated with per group notifications
    of the items moved.
 */

void QQuickListCompositor::move(
        Group fromGroup,
        int from,
        Group toGroup,
        int to,
        int count,
        Group moveGroup,
        QVector<Remove> *removes,
        QVector<Insert> *inserts)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< fromGroup << from << toGroup << to << count)
    Q_ASSERT(count > 0);
    Q_ASSERT(from >=0);
    Q_ASSERT(verifyMoveTo(fromGroup, from, toGroup, to, count, moveGroup));

    // Find the position of the first item to move.
    iterator fromIt = find(fromGroup, from);

    if (fromIt != moveGroup) {
        // If the range at the from index doesn't contain items from the move group; skip
        // to the next range.
        fromIt.incrementIndexes(fromIt->count - fromIt.offset);
        fromIt.offset = 0;
        *fromIt = fromIt->next;
    } else if (fromIt.offset > 0) {
        // If the range at the from index contains items from the move group and the index isn't
        // at the start of the range; split the range at the index and move the iterator to start
        // of the second range.
        *fromIt = insert(
                *fromIt, fromIt->list, fromIt->index, fromIt.offset, fromIt->flags & ~AppendFlag)->next;
        fromIt->index += fromIt.offset;
        fromIt->count -= fromIt.offset;
        fromIt.offset = 0;
    }

    // Remove count items belonging to the move group from the list.
    Range movedFlags;
    for (int moveId = 0; count > 0;) {
        if (fromIt != moveGroup) {
            // Skip ranges not containing items from the move group.
            fromIt.incrementIndexes(fromIt->count);
            *fromIt = fromIt->next;
            continue;
        }
        int difference = qMin(count, fromIt->count);

        // Create a new static range containing the moved items from an existing range.
        new Range(
                &movedFlags,
                fromIt->list,
                fromIt->index,
                difference,
                fromIt->flags & ~(PrependFlag | AppendFlag));
        // Remove moved items from the count, the existing range, and a remove notification.
        if (removes)
            removes->append(Remove(fromIt, difference, fromIt->flags, moveId++));
        count -= difference;
        fromIt->count -= difference;

        // If the existing range contains the prepend flag replace the removed items with
        // a placeholder range for new items inserted into the source model.
        int removeIndex = fromIt->index;
        if (fromIt->prepend()
                && fromIt->previous != &m_ranges
                && fromIt->previous->flags == PrependFlag
                && fromIt->previous->list == fromIt->list
                && fromIt->previous->end() == fromIt->index) {
            // Grow the previous range instead of creating a new one if possible.
            fromIt->previous->count += difference;
        } else if (fromIt->prepend()) {
            *fromIt = insert(*fromIt, fromIt->list, removeIndex, difference, PrependFlag)->next;
        }
        fromIt->index += difference;

        if (fromIt->count == 0) {
            // If the existing range has no items remaining; remove it from the list.
            if (fromIt->append())
                fromIt->previous->flags |= AppendFlag;
            *fromIt = erase(*fromIt);

            // If the ranges before and after the removed range can be joined, do so.
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

    // Try and join the range following the removed items to the range preceding it.
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

    // Find the destination position of the move.
    insert_iterator toIt = fromIt;
    toIt.setGroup(toGroup);

    const int difference = to - toIt.index[toGroup];
    toIt += difference;

    // If the insert position is part way through a range; split it and move the iterator to the
    // start of the second range.
    if (toIt.offset > 0) {
        *toIt = insert(*toIt, toIt->list, toIt->index, toIt.offset, toIt->flags & ~AppendFlag)->next;
        toIt->index += toIt.offset;
        toIt->count -= toIt.offset;
        toIt.offset = 0;
    }

    // Insert the moved ranges before the insert iterator, growing the previous range if that
    // is an option.
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

    // Try and join the range after the inserted ranges to the last range inserted.
    if (*toIt != m_ranges.next
            && toIt->previous->list == toIt->list
            && (!toIt->list || (toIt->previous->end() == toIt->index && toIt->previous->flags == (toIt->flags & ~AppendFlag)))) {
        toIt.offset = toIt->previous->count;
        toIt->previous->count += toIt->count;
        toIt->previous->flags = toIt->flags;
        *toIt = erase(*toIt)->previous;
    }
    // Create insert notification for the ranges moved.
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

    QT_QML_VERIFY_LISTCOMPOSITOR
}

void QQuickListCompositor::clear()
{
    QT_QML_TRACE_LISTCOMPOSITOR( )
    for (Range *range = m_ranges.next; range != &m_ranges; range = erase(range)) {}
    m_end = iterator(m_ranges.next, 0, Default, m_groupCount);
    m_cacheIt = m_end;
}

void QQuickListCompositor::listItemsInserted(
        QVector<Insert> *translatedInsertions,
        void *list,
        const QVector<QQuickChangeSet::Insert> &insertions,
        const QVector<MovedFlags> *movedFlags)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< list << insertions)
    for (iterator it(m_ranges.next, 0, Default, m_groupCount); *it != &m_ranges; *it = it->next) {
        if (it->list != list || it->flags == CacheFlag) {
            it.incrementIndexes(it->count);
            continue;
        } else if (it->flags & MovedFlag) {
            it->flags &= ~MovedFlag;
            it.incrementIndexes(it->count);
            continue;
        }
        foreach (const QQuickChangeSet::Insert &insertion, insertions) {
            int offset = insertion.index - it->index;
            if ((offset > 0 && offset < it->count)
                    || (offset == 0 && it->prepend())
                    || (offset == it->count && it->append())) {
                if (it->prepend()) {
                    uint flags = m_defaultFlags;
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
    QT_QML_VERIFY_LISTCOMPOSITOR
}

void QQuickListCompositor::listItemsInserted(
        void *list, int index, int count, QVector<Insert> *translatedInsertions)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< list << index << count)
    Q_ASSERT(count > 0);

    QVector<QQuickChangeSet::Insert> insertions;
    insertions.append(QQuickChangeSet::Insert(index, count));

    listItemsInserted(translatedInsertions, list, insertions);
}

void QQuickListCompositor::listItemsRemoved(
        QVector<Remove> *translatedRemovals,
        void *list,
        QVector<QQuickChangeSet::Remove> *removals,
        QVector<QQuickChangeSet::Insert> *insertions,
        QVector<MovedFlags> *movedFlags, int moveId)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< list << *removals)

    for (iterator it(m_ranges.next, 0, Default, m_groupCount); *it != &m_ranges; *it = it->next) {
        if (it->list != list || it->flags == CacheFlag) {
            it.incrementIndexes(it->count);
            continue;
        }
        bool removed = false;
        for (QVector<QQuickChangeSet::Remove>::iterator removal = removals->begin();
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
                    QVector<QQuickChangeSet::Insert>::iterator insertion = insertions->begin();
                    for (; insertion != insertions->end() && insertion->moveId != removal->moveId;
                            ++insertion) {}
                    Q_ASSERT(insertion != insertions->end());
                    Q_ASSERT(insertion->count == removal->count);

                    if (relativeIndex < 0) {
                        int splitMoveId = ++moveId;
                        removal = removals->insert(removal, QQuickChangeSet::Remove(
                                removal->index, -relativeIndex, splitMoveId));
                        ++removal;
                        removal->count -= -relativeIndex;
                        insertion = insertions->insert(insertion, QQuickChangeSet::Insert(
                                insertion->index, -relativeIndex, splitMoveId));
                        ++insertion;
                        insertion->index += -relativeIndex;
                        insertion->count -= -relativeIndex;
                    }

                    if (it->prepend()) {
                        removeFlags |= it->flags & CacheFlag;
                        translatedRemoval.moveId = ++moveId;
                        movedFlags->append(MovedFlags(moveId, it->flags & ~AppendFlag));

                        if (removeCount < removal->count) {
                            removal = removals->insert(removal, QQuickChangeSet::Remove(
                                    removal->index, removeCount, translatedRemoval.moveId));
                            ++removal;
                            insertion = insertions->insert(insertion, QQuickChangeSet::Insert(
                                    insertion->index, removeCount, translatedRemoval.moveId));
                            ++insertion;

                            removal->count -= removeCount;
                            insertion->index += removeCount;
                            insertion->count -= removeCount;
                        } else {
                            removal->moveId = translatedRemoval.moveId;
                            insertion->moveId = translatedRemoval.moveId;
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
                    it.decrementIndexes(it->previous->count);
                    it->previous->count += it->count;
                    it->previous->flags = it->flags;
                    *it = erase(*it)->previous;
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
    QT_QML_VERIFY_LISTCOMPOSITOR
}

void QQuickListCompositor::listItemsRemoved(
        void *list, int index, int count, QVector<Remove> *translatedRemovals)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< list << index << count)
    Q_ASSERT(count >= 0);

    QVector<QQuickChangeSet::Remove> removals;
    removals.append(QQuickChangeSet::Remove(index, count));
    listItemsRemoved(translatedRemovals, list, &removals, 0, 0, 0);
}

void QQuickListCompositor::listItemsMoved(
        void *list,
        int from,
        int to,
        int count,
        QVector<Remove> *translatedRemovals,
        QVector<Insert> *translatedInsertions)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< list << from << to << count)
    Q_ASSERT(count >= 0);

    QVector<QQuickChangeSet::Remove> removals;
    QVector<QQuickChangeSet::Insert> insertions;
    QVector<MovedFlags> movedFlags;
    removals.append(QQuickChangeSet::Remove(from, count, 0));
    insertions.append(QQuickChangeSet::Insert(to, count, 0));

    listItemsRemoved(translatedRemovals, list, &removals, &insertions, &movedFlags, 0);
    listItemsInserted(translatedInsertions, list, insertions, &movedFlags);
}

void QQuickListCompositor::listItemsChanged(
        QVector<Change> *translatedChanges,
        void *list,
        const QVector<QQuickChangeSet::Change> &changes)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< list << changes)
    for (iterator it(m_ranges.next, 0, Default, m_groupCount); *it != &m_ranges; *it = it->next) {
        if (it->list != list || it->flags == CacheFlag) {
            it.incrementIndexes(it->count);
            continue;
        } else if (!it->inGroup()) {
            continue;
        }
        foreach (const QQuickChangeSet::Change &change, changes) {
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

void QQuickListCompositor::listItemsChanged(
        void *list, int index, int count, QVector<Change> *translatedChanges)
{
    QT_QML_TRACE_LISTCOMPOSITOR(<< list << index << count)
    Q_ASSERT(count >= 0);
    QVector<QQuickChangeSet::Change> changes;
    changes.append(QQuickChangeSet::Change(index, count));
    listItemsChanged(translatedChanges, list, changes);
}

void QQuickListCompositor::transition(
        Group from,
        Group to,
        QVector<QQuickChangeSet::Remove> *removes,
        QVector<QQuickChangeSet::Insert> *inserts)
{
    int removeCount = 0;
    for (iterator it(m_ranges.next, 0, Default, m_groupCount); *it != &m_ranges; *it = it->next) {
        if (it == from && it != to) {
            removes->append(QQuickChangeSet::Remove(it.index[from]- removeCount, it->count));
            removeCount += it->count;
        } else if (it != from && it == to) {
            inserts->append(QQuickChangeSet::Insert(it.index[to], it->count));
        }
        it.incrementIndexes(it->count);
    }
}

QDebug operator <<(QDebug debug, const QQuickListCompositor::Group &group)
{
    switch (group) {
    case QQuickListCompositor::Cache:   return debug << "Cache";
    case QQuickListCompositor::Default: return debug << "Default";
    default: return (debug.nospace() << "Group" << int(group)).space();
    }

}

QDebug operator <<(QDebug debug, const QQuickListCompositor::Range &range)
{
    (debug.nospace()
            << "Range("
            << range.list) << ' '
            << range.index << ' '
            << range.count << ' '
            << (range.isUnresolved() ? 'U' : '0')
            << (range.append() ? 'A' : '0')
            << (range.prepend() ? 'P' : '0');
    for (int i = QQuickListCompositor::MaximumGroupCount - 1; i >= 2; --i)
        debug << (range.inGroup(i) ? '1' : '0');
    return (debug
            << (range.inGroup(QQuickListCompositor::Default) ? 'D' : '0')
            << (range.inGroup(QQuickListCompositor::Cache) ? 'C' : '0'));
}

static void qt_print_indexes(QDebug &debug, int count, const int *indexes)
{
    for (int i = count - 1; i >= 0; --i)
        debug << indexes[i];
}

QDebug operator <<(QDebug debug, const QQuickListCompositor::iterator &it)
{
    (debug.nospace() << "iterator(" << it.group).space() << "offset:" << it.offset;
    qt_print_indexes(debug, it.groupCount, it.index);
    return ((debug << **it).nospace() << ')').space();
}

static QDebug qt_print_change(QDebug debug, const char *name, const QQuickListCompositor::Change &change)
{
    debug.nospace() << name << '(' << change.moveId << ' ' << change.count << ' ';
    for (int i = QQuickListCompositor::MaximumGroupCount - 1; i >= 2; --i)
        debug << (change.inGroup(i) ? '1' : '0');
    debug << (change.inGroup(QQuickListCompositor::Default) ? 'D' : '0')
            << (change.inGroup(QQuickListCompositor::Cache) ? 'C' : '0');
    int i = QQuickListCompositor::MaximumGroupCount - 1;
    for (; i >= 0 && !change.inGroup(i); --i) {}
    for (; i >= 0; --i)
        debug << ' ' << change.index[i];
    return (debug << ')').maybeSpace();
}

QDebug operator <<(QDebug debug, const QQuickListCompositor::Change &change)
{
    return qt_print_change(debug, "Change", change);
}

QDebug operator <<(QDebug debug, const QQuickListCompositor::Remove &remove)
{
    return qt_print_change(debug, "Remove", remove);
}

QDebug operator <<(QDebug debug, const QQuickListCompositor::Insert &insert)
{
    return qt_print_change(debug, "Insert", insert);
}

QDebug operator <<(QDebug debug, const QQuickListCompositor &list)
{
    int indexes[QQuickListCompositor::MaximumGroupCount];
    for (int i = 0; i < QQuickListCompositor::MaximumGroupCount; ++i)
        indexes[i] = 0;
    debug.nospace() << "QQuickListCompositor(";
    qt_print_indexes(debug, list.m_groupCount, list.m_end.index);
    for (QQuickListCompositor::Range *range = list.m_ranges.next; range != &list.m_ranges; range = range->next) {
        (debug << '\n').space();
        qt_print_indexes(debug, list.m_groupCount, indexes);
        debug << ' ' << *range;

        for (int i = 0; i < list.m_groupCount; ++i) {
            if (range->inGroup(i))
                indexes[i] += range->count;
        }
    }
    return (debug << ')').maybeSpace();
}

QT_END_NAMESPACE
