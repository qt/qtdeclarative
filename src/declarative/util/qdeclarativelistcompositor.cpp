/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
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

bool qt_verifyIntegrity(const QLinkedList<QDeclarativeCompositeRange> &ranges, int absoluteCount, int internalCount)
{
    bool valid = true;
    int actualAbsoluteCount = 0;
    int actualInternalCount = 0;

    int index = 0;
    foreach (const QDeclarativeCompositeRange &range, ranges) {
        if (range.null()) {
            if (range.internal()) {
                qWarning() << index << "Null Internal Range";
                valid = false;
            }
        } else {
            actualAbsoluteCount += range.count;
            if (range.internal())
                actualInternalCount += range.count;
        }
        ++index;
    }

    if (actualAbsoluteCount != absoluteCount) {
        qWarning() << "Absolute count invalid" << absoluteCount << actualAbsoluteCount;
        valid = false;
    }
    if (actualInternalCount != internalCount) {
        qWarning() << "Internal count invalid" << internalCount << actualInternalCount;
        valid = false;
    }

    return valid;
}

QDeclarativeListCompositor::QDeclarativeListCompositor(int internalCount)
    : absoluteCount(internalCount)
    , internalCount(internalCount)
{
}

QDeclarativeListCompositor::~QDeclarativeListCompositor()
{
}

QDeclarativeListCompositor::iterator QDeclarativeListCompositor::insert(iterator before, const QDeclarativeCompositeRange &range)
{
    rangeCreated(range.list);
    return ++ranges.insert(before, range);
}

QDeclarativeListCompositor::iterator QDeclarativeListCompositor::erase(iterator range)
{
    rangeDestroyed(range->list);
    return ranges.erase(range);
}

int QDeclarativeListCompositor::count() const
{
    return absoluteCount;
}

QDeclarativeCompositeRange QDeclarativeListCompositor::at(int index, int *offset, int *internalIndex) const
{
    Q_ASSERT(index >=0 && index < absoluteCount);
    *internalIndex = 0;
    for (const_iterator range = ranges.begin(); range != ranges.end(); ++range) {
        if (range->null())
            continue;
        if (index < range->count) {
            *offset = index;
            if (range->internal())
                *internalIndex += index;
            return *range;
        }
        if (range->internal())
            *internalIndex += range->count;
        index -= range->count;
    }
    return QDeclarativeCompositeRange(0, 0, 0, 0);
}

void QDeclarativeListCompositor::appendList(void *list, int start, int count, bool grow)
{
    absoluteCount += count;
    if (!ranges.isEmpty()
            && ranges.last().list == list
            && !ranges.last().null()
            && !ranges.last().prepend()
            && !grow
            && ranges.last().index + ranges.last().count == start) {
        ranges.last().count += count;
    } else {
        insert(ranges.end(), QDeclarativeCompositeRange(list, start, count, grow ? (Prepend | Append) : 0));
    }
}

bool QDeclarativeListCompositor::appendData(const void *data)
{
    if (insertInternalData(internalCount, data)) {
        if (!ranges.isEmpty() && !ranges.last().list)
            ranges.last().count += 1;
        else
            ranges.insert(ranges.end(), QDeclarativeCompositeRange(0, -1, 1, Internal));
        absoluteCount += 1;
        internalCount += 1;
        return true;
    } else {
        return false;
    }
}

QDeclarativeListCompositor::iterator QDeclarativeListCompositor::insertIntoRange(
    iterator range, int insertAt, const QDeclarativeCompositeRange &insertRange)
{
    if (insertAt > 0) {
        range = insert(range, QDeclarativeCompositeRange(
                range->list, range->index, insertAt, range->flags & ~Append));
    }
    range = ++ranges.insert(range, insertRange);
    range->index += insertAt;
    range->count -= insertAt;
    return range;
}

void QDeclarativeListCompositor::insertList(int index, void *list, int start, int count, bool grow)
{
    Q_ASSERT(index >=0 && index <= absoluteCount);
    QDeclarativeCompositeRange insertRange(list, start, count, grow ? (Prepend | Append) : 0);
    absoluteCount += count;
    for (iterator range = ranges.begin(); range != ranges.end(); ++range) {
        if (range->null())
            continue;
        if (index < range->count || (index == range->count && range->append())) {
            insertIntoRange(range, index, insertRange);
            return;
        }
        index -= range->count;
    }
    insert(ranges.end(), insertRange);
}

bool QDeclarativeListCompositor::insertData(int index, const void *data)
{
    Q_ASSERT(index >=0 && index <= absoluteCount);

    int internalIndex = 0;
    int relativeIndex = index;
    for (iterator range = ranges.begin(); range != ranges.end(); ++range) {
        if (range->null())
            continue;
        if (!range->list && relativeIndex <= range->count) {
            if (!insertInternalData(internalIndex + relativeIndex, data))
                return false;
            range->count += 1;
            internalCount += 1;
            absoluteCount += 1;
            return true;
        } else if (relativeIndex < range->count || (relativeIndex == range->count && range->append())) {
            if (range->internal())
                internalIndex += relativeIndex;
            if (!insertInternalData(internalIndex, data))
                return false;
            range = insertIntoRange(range, relativeIndex, QDeclarativeCompositeRange(
                    0, -1, 1, Internal));
            internalCount += 1;
            absoluteCount += 1;
            return true;
        }
        if (range->internal())
            internalIndex += range->count;
        relativeIndex -= range->count;
    }
    if (!insertInternalData(internalIndex, data))
        return false;
    insert(ranges.end(), QDeclarativeCompositeRange(0, -1, 1, Internal));
    internalCount += 1;
    absoluteCount += 1;
    return true;
}

bool QDeclarativeListCompositor::replaceAt(int index, const void *data)
{
    Q_ASSERT(index >=0 && index < absoluteCount);
    int internalIndex = 0;
    int relativeIndex = index;
    iterator range = ranges.begin();
    while (range != ranges.end()) {
        if (range->null()) {
            ++range;
            continue;
        }
        if (relativeIndex < range->count) {
            internalIndex += relativeIndex;

            if (range->internal()) {
                replaceInternalData(internalIndex, data);
                return true;
            } else if (!insertInternalData(internalIndex, data)) {
                return false;
            }

            range->count -= 1;
            internalCount += 1;

            int removeIndex = range->index + relativeIndex;

            if (range->count == 0) {
                range->flags |= Internal;
                range->count = 1;
            } else {
                if (relativeIndex > 0) {
                    int splitOffset = relativeIndex;
                    if (range->list)
                        splitOffset += 1;
                    if (splitOffset < range->count) {
                        range = insert(range, QDeclarativeCompositeRange(
                                range->list, range->index, relativeIndex, range->flags & ~Append));
                        range->index += splitOffset;
                        range->count -= splitOffset;
                    } else {
                        range->index += relativeIndex;
                    }
                }
                range = insert(range, QDeclarativeCompositeRange(
                        range->list, removeIndex, 1, (range->flags | Internal) & ~Append));
            }
            return true;
        }
        relativeIndex -= range->count;
        if (range->internal())
            internalIndex += range->count;
        ++range;
    }
    return false;
}

void QDeclarativeListCompositor::removeAt(int index, int count)
{
    Q_ASSERT(index >=0 && index + count <= absoluteCount);
    int internalIndex = 0;
    int internalRemoveCount = 0;
    int relativeIndex = index;
    iterator range = ranges.begin();
    while (range != ranges.end() && count > 0) {
        if (range->null()) {
            ++range;
            continue;
        }
        if (relativeIndex < range->count) {
            int removeCount = qMin(count, range->count - relativeIndex);
            range->count -= removeCount;
            count -= removeCount;
            absoluteCount -= removeCount;

            int removeIndex = range->index + relativeIndex;
            if (range->internal())
                internalCount -= removeCount;

            if (range->count == 0) {
                if (range->append()) {
                    range = insert(range, QDeclarativeCompositeRange(
                            range->list, range->index, removeCount, (range->flags & ~Append) | Null));
                    range->index += removeCount;
                } else if (range->prepend()) {
                    range->flags |= Null;
                    range->count = removeCount;
                } else {
                    range = erase(range);
                    continue;
                }
            } else {
                if (relativeIndex > 0) {
                    int splitOffset = relativeIndex;
                    if (range->list)
                        splitOffset += removeCount;
                    if (splitOffset < range->count) {
                        Q_ASSERT(count == 0);
                        range = insert(range, QDeclarativeCompositeRange(
                                range->list, range->index, relativeIndex, range->flags & ~Append));
                        range->index += splitOffset;
                        range->count -= splitOffset;
                    } else {
                        range->index += relativeIndex;
                        relativeIndex = 0;
                    }
                } else if (range->list) {
                    range->index += removeCount;
                }
                if (range->prepend()) {
                    range = insert(range, QDeclarativeCompositeRange(
                            range->list, removeIndex, removeCount, Null | Prepend));
                }
            }
            ++range;
            continue;
        }
        relativeIndex -= range->count;
        if (range->internal())
            internalIndex += range->count;
        ++range;
    }

    if (internalRemoveCount > 0)
        removeInternalData(internalIndex, internalRemoveCount);
}

void QDeclarativeListCompositor::removeList(void *list, QVector<QDeclarativeChangeSet::Remove> *changes)
{
    int absoluteIndex = 0;
    int internalIndex = 0;
    iterator range = ranges.begin();
    while (range != ranges.end()) {
        if (range->list == list) {
            if (!range->null()) {
                absoluteCount -= range->count;
                changes->append(QDeclarativeChangeSet::Remove(absoluteIndex, absoluteIndex + range->count));
                if (range->internal())
                    removeInternalData(internalIndex, range->count);
            }
            range = erase(range);
            continue;
        }
        if (!range->null()) {
            absoluteIndex += range->count;
            if (range->internal())
                internalIndex += range->count;
        }
        ++range;
    }
}

void QDeclarativeListCompositor::backwardToForwardMove(int &from, int &to, int &count)
{
    int tfrom = from;
    int tto = to;
    from = tto;
    to = tto + count;
    count = tfrom - tto;
}

void QDeclarativeListCompositor::forwardToBackwardMove(int &from, int &to, int &count)
{
    int tfrom = from;
    int tto = to;
    from = tto;
    to = tto + count;
    count = tfrom - tto;
}

void QDeclarativeListCompositor::move(int from, int to, int count)
{
    Q_ASSERT(from != to || count == 0);
    Q_ASSERT(from >=0 && from + count <= absoluteCount);
    Q_ASSERT(to >=0 && to <= absoluteCount);

    const bool backward = from > to;

    int internalTo = 0;
    int internalMoveCount = 0;
    iterator insertPos;
    iterator range = ranges.begin();

    if (backward) {
        for (int relativeTo = to; range != ranges.end(); ++range) {
            if (range->null())
                continue;
            if (relativeTo < range->count || (relativeTo == range->count && range->append())) {
                if (relativeTo > 0) {
                    range = insert(range, QDeclarativeCompositeRange(
                            range->list, range->index, relativeTo, range->flags & ~Append));
                    range->index += relativeTo;
                    range->count -= relativeTo;
                    if (range->internal())
                        internalTo += relativeTo;
                }
                insertPos = range;
                break;
            }
            if (range->internal())
                internalTo += range->count;
            relativeTo -= range->count;
        }
    }

    int internalFrom = internalTo;
    for (int relativeFrom = backward ? from - to : from; range != ranges.end(); ++range) {
        if (range->null())
            continue;
        if (relativeFrom < range->count) {
            if (relativeFrom > 0) {
                QDeclarativeCompositeRange insertRange(
                        range->list, range->index, relativeFrom, range->flags & ~Append);
                if (backward && range == insertPos) {
                    rangeCreated(range->list);
                    range = ranges.insert(range, insertRange);
                    insertPos = range;
                    ++range;
                } else {
                    range = insert(range, insertRange);
                }
                range->index += relativeFrom;
                range->count -= relativeFrom;
                if (range->internal())
                    internalFrom += relativeFrom;
            }
            break;
        }
        if (range->internal())
            internalFrom += range->count;
        relativeFrom -= range->count;
    }

    QVarLengthArray<QDeclarativeCompositeRange, 20> movedRanges;

    do {
        if (range->null()) {
            ++range;
            continue;
        }
        QDeclarativeCompositeRange movedRange = *range;
        if (range->count > count || (range->count == count && range->append())) {
            insert(range + 1, QDeclarativeCompositeRange(
                    range->list, range->index + count, range->count - count, range->flags));
            movedRange.count = count;
        }
        if (range->prepend()) {
            movedRange.flags &= ~(Prepend | Append);
            range->count = count;
            range->flags |= Null;
            range->flags &= ~(Internal | Append);
            ++range;
        } else {
            range = erase(range);
        }
        if (movedRange.internal())
            internalMoveCount += movedRange.count;
        count -= movedRange.count;
        movedRanges.append(movedRange);
    } while (count > 0);

    if (!backward) {
        insertPos = ranges.end();
        internalTo = internalFrom;
        for (int relativeTo = to - from; range != ranges.end(); ++range) {
            if (range->null())
                continue;
            if (relativeTo < range->count || (relativeTo == range->count && range->append())) {
                if (relativeTo > 0) {
                    int insertIndex = range->index;
                    if (range->internal())
                        insertIndex -= internalMoveCount;
                    range = insert(range, QDeclarativeCompositeRange(
                            range->list, insertIndex, relativeTo, range->flags & ~Append));
                }
                range->index += relativeTo;
                range->count -= relativeTo;
                if (range->internal())
                    internalTo += relativeTo;
                insertPos = range;
                break;
            }
            if (range->internal())
                internalTo = range->count;
            relativeTo -= range->count;
        }
    }

    if (internalMoveCount > 0)
        moveInternalData(internalFrom, internalTo, internalMoveCount);

    for (int i = 0; i < movedRanges.count(); ++i)
        insertPos = ++ranges.insert(insertPos, movedRanges.at(i));

    Q_ASSERT(qt_verifyIntegrity(ranges, absoluteCount, internalCount));
}

bool QDeclarativeListCompositor::merge(int from, int to)
{
    int internalIndex = 0;
    int internalFrom = 0;
    int internalTo = 0;

    int relativeFrom = from;
    int relativeTo = to;

    iterator fromRange = ranges.end();
    iterator toRange = ranges.end();

    if (from > to)
        qSwap(relativeFrom, relativeTo);

    for (iterator range = ranges.begin(); range != ranges.end(); ++range) {
        if (range->null())
            continue;
        if (relativeFrom < range->count) {
            fromRange = range;
            internalFrom = internalIndex;
            if (range->internal())
                internalFrom += relativeFrom;
            relativeTo -= from - relativeFrom;
            internalIndex -= 1;
            break;
        }
        if (range->internal())
            internalIndex += range->count;
        relativeFrom -= range->count;
    }
    for (iterator range = fromRange; range != ranges.end(); ++range) {
        if (range->null())
            continue;
        if (relativeTo < range->count) {
            toRange = range;
            internalTo = internalIndex;
            if (range->internal())
                internalTo += relativeTo;
            break;
        }
        if (range->internal())
            internalIndex += range->count;
        relativeTo -= range->count;
    }

    if (from > to) {
        qSwap(relativeFrom, relativeTo);
        qSwap(internalFrom, internalTo);
        qSwap(fromRange, toRange);
    }

    if (fromRange == ranges.end()
            || toRange == ranges.end()
            || fromRange->list
            || !toRange->list
            || toRange->internal()) {
        return false;
    }

    fromRange->count -= 1;

    if (fromRange->count == 0)
        ranges.erase(fromRange);

    if (relativeTo > 0) {
        toRange = ++ranges.insert(toRange, QDeclarativeCompositeRange(
                toRange->list, toRange->index, relativeTo, toRange->flags & ~Append));
        toRange->index += relativeTo;
        toRange->count -= relativeTo;
    }

    if (toRange->count == 1 && !toRange->append()) {
        toRange->flags |= Internal;
    } else {
        toRange = ++ranges.insert(toRange, QDeclarativeCompositeRange(
                toRange->list, toRange->index, 1, (toRange->flags | Internal) & ~Append));
        toRange->index += 1;
        toRange->count -= 1;
    }

    if (internalFrom != internalTo)
        moveInternalData(internalFrom, internalTo, 1);

    absoluteCount -= 1;

    return true;
}

void QDeclarativeListCompositor::clear()
{
    for (iterator range = ranges.begin(); range != ranges.end(); range = erase(range)) {}
    ranges.clear();
    removeInternalData(0, internalCount);
    absoluteCount = 0;
    internalCount = 0;
}

int QDeclarativeListCompositor::absoluteIndexOf(int internalIndex) const
{
    int absoluteIndex = 0;
    for (const_iterator range = ranges.begin(); range != ranges.end(); ++range) {
        if (range->null())
            continue;
        if (range->internal()) {
            if (internalIndex < range->count)
                return absoluteIndex + internalIndex;
            internalIndex -= range->count;
        }
        absoluteIndex += range->count;
    }
    return -1;
}

int QDeclarativeListCompositor::absoluteIndexOf(void *list, int index, int from) const
{
    Q_ASSERT(list);
    int absoluteIndex = 0;
    for (const_iterator range = ranges.begin(); range != ranges.end(); ++range) {
        if (range->null())
            continue;
        if (range->list == list
                && absoluteIndex + range->count > from
                && index >= range->index
                && index < range->index + range->count) {
            return absoluteIndex + index - range->index;
        }
        absoluteIndex += range->count;
    }
    return -1;
}

void QDeclarativeListCompositor::listItemsInserted(void *list, int start, int end, QVector<QDeclarativeChangeSet::Insert> *changes)
{
    int index = start;
    int count = end - start;

    int absoluteIndex = 0;
    iterator range = ranges.begin();
    while (range != ranges.end()) {
        if (range->list == list) {
            int relativeIndex = index - range->index;
            if ((relativeIndex > 0 && relativeIndex < range->count)
                    || (relativeIndex == 0 && range->prepend())
                    || (relativeIndex == range->count && range->append())) {
                if (range->flags & (Null | Internal)) {
                    range = insertIntoRange(range, relativeIndex, QDeclarativeCompositeRange(
                            range->list, index, count, Prepend));
                    range->index += count;
                } else {
                    range->count += count;
                }
                absoluteCount += count;
                *changes << QDeclarativeChangeSet::Insert(
                        absoluteIndex + relativeIndex,
                        absoluteIndex + relativeIndex + count);
            } else if (relativeIndex <= 0) {
                range->index += count;
            }
        }
        if (!range->null())
            absoluteIndex += range->count;
        ++range;
    }
}

void QDeclarativeListCompositor::listItemsRemoved(void *list, int start, int end, QVector<QDeclarativeChangeSet::Remove> *changes)
{
    int index = start;
    int count = end - start;
    int internalIndex = 0;
    int absoluteIndex = 0;
    iterator range = ranges.begin();
    while (range != ranges.end()) {
        if (range->list == list) {
            int relativeIndex = index - range->index;
            if (relativeIndex + count > 0 && relativeIndex < range->count) {
                const int removeOffset = qMax(0, relativeIndex);
                int removeCount = qMin(range->count, relativeIndex + count) - removeOffset;
                range->count -= removeCount;

                if (!range->null()) {
                    absoluteCount -= removeCount;
                    *changes << QDeclarativeChangeSet::Remove(
                            absoluteIndex + removeOffset,
                            absoluteIndex + removeOffset + removeCount);

                    if (range->internal()) {
                        internalIndex += removeOffset;
                        removeInternalData(internalIndex, removeCount);
                    }
                }
                if (range->count == 0 && !range->append()) {
                    range = erase(range);
                    continue;
                } else if (relativeIndex <= 0) {
                    range->index += relativeIndex;
                }

            } else if (relativeIndex < 0) {
                range->index -= count;
            }
        }
        if (!range->null()) {
            absoluteIndex += range->count;
            if (range->internal())
                internalIndex += range->count;
        }
          ++range;
    }
}

QDeclarativeListCompositor::iterator QDeclarativeListCompositor::listItemsMovedForward(
        void *list,
        int from,
        int to,
        int count,
        QVector<QDeclarativeChangeSet::Move> *changes,
        iterator range,
        int &absoluteIndex,
        int &internalIndex)
{
    const bool outer = range == ranges.begin();
    QVarLengthArray<Move, 20> moves;
    while (range != ranges.end()) {
        if (range->list != list) {
            if (!range->null()) {
                absoluteIndex += range->count;
                if (range->internal())
                    internalIndex += range->count;
            }
            ++range;
            continue;
        }

        if (range->prepend() && !moves.isEmpty() && range->index == 0)
            range = listItemsMovedForward(
                    list,
                    from,
                    to,
                    count,
                    changes,
                    range,
                    absoluteIndex,
                    internalIndex);

        int relativeFrom = from - range->index;
        if (relativeFrom + count > 0 && relativeFrom < range->count) {
            if (range->prepend()) {
                int moveCount = qMin(range->count, count + qMin(0, relativeFrom));
                range->count -= moveCount;
                range->index += qMin(0, relativeFrom);

                if (!range->null()) {
                    if (range->internal()) {
                        moves.append(Move(changes->count(), internalIndex + relativeFrom));
                    } else {
                        moves.append(Move(changes->count()));
                    }
                    changes->append(QDeclarativeChangeSet::Move(
                            absoluteIndex + qMax(0, relativeFrom),
                            absoluteIndex + qMax(0, relativeFrom) + moveCount,
                            INT_MAX));
                } else {
                    moves.append(Move::nullMove(moveCount));
                }

                if (range->count == 0) {
                    range = erase(range);
                    continue;
                }
            } else {
                absoluteIndex += range->count;
                if (relativeFrom > 0) {
                    range = insert(range, QDeclarativeCompositeRange(
                            range->list, range->index, relativeFrom, range->flags & ~(Null | Prepend | Append)));
                    range->index += relativeFrom;
                    range->count -= relativeFrom;
                    relativeFrom = 0;
                }
                int moveCount = qMin(range->count, count + relativeFrom);
                if (range->count < moveCount) {
                    range = insert(range, QDeclarativeCompositeRange(
                            range->list, range->index, moveCount, range->flags & ~(Null | Prepend | Append)));
                    range->index += moveCount;
                    range->count -= moveCount;
                } else {
                    range->index = to - relativeFrom;
                }
                ++range;
                continue;
            }
        }
        if (range->index >= from + count && range->index < to + count)
            range->index -= count;
        int relativeTo = to - range->index;
        if ((range->prepend() && relativeTo >= 0 && relativeTo <= range->count)
                || (range->append() && relativeTo == range->count)
                || (range->prepend() && range->index <= from && relativeTo == range->count)) {

            int moveTo = absoluteIndex + relativeTo;
            for (int i = 0; i < moves.count(); ++i) {
                const Move &move = moves.at(i);
                const int moveCount = move.null() ? move.count() : changes->at(move.changeIndex).count();
                if (!move.null()) {
                    (*changes)[move.changeIndex].tstart = moveTo;
                    (*changes)[move.changeIndex].tend = moveTo + moveCount;
                }
                if (move.null() == range->null()) {
                    range->count += moveCount;
                    relativeTo += moveCount;
                } else {
                    if (move.internal()) {
                        internalIndex += relativeTo;
                        moveInternalData(move.internalIndex, internalIndex, moveCount);
                        internalIndex += moveCount;
                    }
                    if (relativeTo > 0) {
                        range = insert(range, QDeclarativeCompositeRange(
                                range->list, range->index, relativeTo, range->flags & ~Append));
                        range->index += relativeTo;
                        range->count -= relativeTo;
                        absoluteIndex += relativeTo;
                        relativeTo = 0;
                    }
                    range = insert(range, QDeclarativeCompositeRange(
                            range->list, range->index, moveCount, move.null() ? (Null | Prepend) : Prepend));
                    absoluteIndex += moveCount;
                    range->index += moveCount;
                }
                moveTo += moveCount;
            }
            moves.clear();
            if (!outer)
                return ++range;
        }
        if (!range->null()) {
            absoluteIndex += range->count;
            if (range->internal())
                internalIndex += range->count;
        }
        ++range;
    }
    return range;
}

void QDeclarativeListCompositor::listItemsMoved(
    void *list, int start, int end, int to, QVector<QDeclarativeChangeSet::Move> *changes)
{
    int from = start;
    int count = end - start;

    Q_ASSERT(from != to);
    Q_ASSERT(count != 0);

    const bool reversed = from > to;
    if (reversed)
        backwardToForwardMove(from, to, count);

    int absoluteIndex = 0;
    int internalIndex = 0;
    listItemsMovedForward(
            list,
            from,
            to,
            count,
            changes,
            ranges.begin(),
            absoluteIndex,
            internalIndex);

    if (reversed) {
        for (QVector<QDeclarativeChangeSet::Move>::iterator it = changes->begin(); it != changes->end(); ++it) {
            int from = it->start;
            int to = it->to;
            int count = it->count();
            forwardToBackwardMove(from, to, count);
            *it = QDeclarativeChangeSet::Move(from, from + count, to);
        }
    }

    Q_ASSERT(qt_verifyIntegrity(ranges, absoluteCount, internalCount));
}

void QDeclarativeListCompositor::listItemsChanged(void *list, int start, int end, QVector<QDeclarativeChangeSet::Change> *changes)
{
    int index = start;
    int count = end - start;

    int absoluteIndex = 0;
    foreach (const QDeclarativeCompositeRange &range, ranges) {
        if (range.null())
            continue;
        if (range.list == list) {
            int relativeIndex = index - range.index;
            if (relativeIndex + range.count > 0 && relativeIndex < range.count) {
                const int changeOffset = qMax(0, relativeIndex);
                int changeCount = qMin(range.count, relativeIndex + count) - changeOffset;
                int changeIndex = absoluteIndex + changeOffset;
                 *changes << QDeclarativeChangeSet::Change(changeIndex, changeIndex + changeCount);
            }
        }
        absoluteIndex += range.count;
    }
}

void QDeclarativeListCompositor::compress()
{
}

QDebug operator <<(QDebug debug, const QDeclarativeListCompositor &list)
{
    int index = 0;
    int internalIndex = 0;
    debug.nospace() << "QDeclarativeListCompositor(" << list.absoluteCount;
    foreach (const QDeclarativeCompositeRange &range, list.ranges) {
        (((debug.space()
                << "\n" << index << internalIndex << "\t(").nospace()
                << range.list).space()
                << range.index
                << range.count
                << "null:" << range.null()
                << "prepend:" << range.prepend()
                << "append:" << range.append()
                << "internal:").nospace() << range.internal() << ")";
        if (!range.null()) {
            index += range.count;
            if (range.internal())
                internalIndex += range.count;
        }
    }
    return (debug.nospace() << ")").maybeSpace();
}
