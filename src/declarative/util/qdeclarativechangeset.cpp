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

#include "qdeclarativechangeset_p.h"

void QDeclarativeChangeSet::insertInsert(int start, int end)
{
    const int count = end - start;

    // Moved signals.
    QVector<Move>::iterator move = m_moves.begin();
    for (; move != m_moves.end() && start >= move->maximum(); ++move) {}
    for (; move != m_moves.end() && end >= move->minimum(); ++move) {
        if (start <= move->tstart) {
            move->tstart += count;
            move->tend += count;
        } else if (start < move->tend) {
            int relativeStart = start - move->tstart;

            move = m_moves.insert(move, Move(
                    move->fstart + count, move->fstart + count + relativeStart, move->tstart));
            ++move;
            move->fstart += relativeStart;
            move->tstart += count + relativeStart;
            move->tend += count;

            start -= relativeStart;
            end -= relativeStart;
        } else {
            start -= move->count();
            end -= move->count();
        }

        if (start <= move->fstart) {
            move->fstart += count;
            move->fend += count;
        } else if (start < move->tstart) {
            start += move->count();
            end += move->count();
        }

    }
    for (; move != m_moves.end(); ++move) {
        move->fstart += count;
        move->fend += count;
        move->tstart += count;
        move->tend += count;
    }

    // Inserted signals.
    QVector<Insert>::iterator insert = m_inserts.begin();
    for (; insert != m_inserts.end(); ++insert) {
        if (start < insert->start) {
            insert = m_inserts.insert(insert, Insert(start, end));
            break;
        } else if (start <= insert->end) {
            insert->end += count;
            break;
        }
    }
    if (insert == m_inserts.end()) {
        m_inserts.append(Insert(start, end));
    } else for (++insert; insert != m_inserts.end(); ++insert) {
        insert->start += count;
        insert->end += count;
    }


    // Changed signals.
    QVector<Change>::iterator change = m_changes.begin();
    for (; change != m_changes.end() && start != change->start && start < change->end; ++change) {
        if (start > change->start) {
            int relativeStart = start - change->start;
            change = m_changes.insert(change, Change(change->start, change->start + relativeStart));
            ++change;
            change->start += count + relativeStart;
            change->end += count - relativeStart;
            break;
        }
    }
    for (; change != m_changes.end(); ++change) {
        change->start += count;
        change->end += count;
    }
}

void QDeclarativeChangeSet::insertRemove(int start, int end)
{
    // Changed Signals.
    QVector<Change>::iterator change = m_changes.begin();
    for (; change != m_changes.end() && start >= change->end; ++change) {}
    for (; change != m_changes.end() && end < change->start; ++change) {
        const int removeCount = qMin(change->end, end) - qMax(change->start, start);
        change->end -= removeCount;
        if (change->start == change->end) {
            change = m_changes.erase(change);
        } else if (start < change->start) {
            change->start = start;
        }
    }
    const int count = end - start;
    for (; change != m_changes.end(); ++change) {
        change->start -= count;
        change->end -= count;
    }

    QVector<Remove> removeChanges;

    // Moved signals.
    QVector<Move>::iterator move = m_moves.begin();
    for (; move != m_moves.end() && start >= move->maximum(); ++move) {}
    for (; move != m_moves.end() && end >= move->minimum(); ++move) {
        if (move->fstart < move->tstart) {
            if (start < move->fstart) {
                const int difference = move->fstart - start;
                move->fend -= difference;
                move->fstart = start;
                move->tstart -= difference;
                move->tend -= difference;

                removeChanges.append(Remove(start, start + difference));
                end -= difference;
            }
            if (end < move->tstart) {
                move->tstart -= end - start;
                move->tend -= end - start;
            } else if (start < move->tend) {
                const int difference = qMin(move->tend, end) - move->tstart;
                removeChanges.append(Remove(
                        move->fstart , move->fstart + difference));
                end -= difference;

                move->fend -= difference;
                move->tstart -= end - start;
                move->tend -=  end - start + difference;
            }
            start += move->count();
            end += move->count();
        } else {
            if (start < move->tend) {
                const int offset = qMax(0, start - move->tstart);
                const int difference = qMin(move->tend, end) - qMax(move->tstart, start);

                removeChanges.append(Remove(
                        move->fstart + offset, move->fstart + offset + difference));
                start -= offset;
                end -= offset + difference;

                move->fend -= difference;
                move->tstart = start;
                move->tend = start + move->fend - move->fstart;
            } else {
                start -= move->count();
                end -= move->count();
            }

            move->fstart -= end - start;
            move->fend -= end - start;

            if (start > move->fstart) {
                const int offset = start - move->fstart;
                const int difference = qMin(move->fend, end) - start;
                removeChanges.append(Remove(
                        move->fstart + end - start + offset + difference ,
                        move->fend + end - start + offset));
                end -= offset;
                move->fstart += offset;
                move->fend += offset;
            }
        }

        if (move->tstart == move->tend || move->fstart == move->tstart) {
            move = m_moves.erase(move);
            --move;
        }
    }
    for (; move != m_moves.end(); ++move) {
        move->fstart -= count;
        move->fend -= count;
        move->tstart -= count;
        move->tend -= count;
    }

    if (start != end)
        removeChanges.append(Remove(start, end));

    foreach (const Remove &r, removeChanges) {
        int start = r.start;
        int end = r.end;

        QVector<Insert>::iterator insert = m_inserts.end() - 1;
        for (const int count = end - start; insert != m_inserts.begin() - 1 && insert->start >= end; --insert) {
                insert->start -= count;
                insert->end -= count;
        }
        for (; insert != m_inserts.begin() - 1 && insert->end > start; --insert) {
            const int removeCount = qMin(insert->end, end) - qMax(insert->start, start);
            insert->end -= removeCount;
            if (insert->start == insert->end) {
                insert = m_inserts.erase(insert);
            } else if (start < insert->start) {
                insert->end -= insert->start - start;
                insert->start = start;
            } else {
                start -= insert->count();
                end -= insert->count();
            }
            end -= removeCount;
            if (start == end)
                 return;
         }
        // Adjust the index to compensate for any inserts prior to the remove position..
        for (; insert != m_inserts.begin() - 1; --insert) {
            start -= insert->count();
            end -= insert->count();
        }

        // Removed signals.
        QVector<Remove>::iterator remove = m_removes.begin();
        for (; remove != m_removes.end(); ++remove) {
            if (end < remove->start) {
                remove = m_removes.insert(remove, Remove(start, end));
                break;
            } else if (start <= remove->start) {
                remove->end += end - remove->start;
                remove->start = start;

                QVector<Remove>::iterator rbegin = remove;
                QVector<Remove>::iterator rend = ++rbegin;
                for (; rend != m_removes.end() && rend->start <= remove->end; ++rend)
                    remove->end += rend->count();
                if (rbegin != rend) {
                    remove = m_removes.erase(rbegin, rend);
                }
                break;
            }
        }
        if (remove != m_removes.end()) {
            const int count = end - start;
            for (++remove; remove != m_removes.end(); ++remove) {
                remove->start -= count;
                remove->end -= count;
            }
        } else {
            m_removes.append(Remove(start, end));
        }
    }
}

void QDeclarativeChangeSet::insertMove(int start, int end, int to)
{
    QVector<Insert> insertChanges;
    QVector<Move> moveChanges;

    int fStart = start;
    int fTo = to;
    int fEnd = end;
    int &bStart = fTo;
    int bEnd = to + end - start;

    if (start > to) {
        qSwap(fStart, bStart);
        qSwap(fEnd, bEnd);
    }

    // Inserted signals.
    QVector<Insert>::iterator insert = m_inserts.begin();
    if (start < to) {
        for (; insert != m_inserts.end() && fStart >= insert->end; ++insert) {}
        for (; insert != m_inserts.end() && fEnd > insert->start; ++insert) {
            const int removeCount = qMin(insert->end, fEnd) - qMax(insert->start, fStart);
            const int relativeStart = fStart - insert->start;
            const int relativeEnd = qMax(0, fEnd - insert->end);

            insert->end -= removeCount;
            if (insert->start == insert->end) {
                insert = m_inserts.erase(insert);
                --insert;
            }

            if (relativeStart < 0) {
                moveChanges.append(Move(fStart, fStart - relativeStart, fTo + relativeEnd));
                fTo -= relativeStart;
            }

            fTo += removeCount;
            insertChanges.append(Insert(bEnd - removeCount, bEnd));
        }
    } else {
        for (; insert != m_inserts.end() && bStart >= insert->end; ++insert) {}
        for (; insert != m_inserts.end() && bEnd > insert->start; ++insert) {
            const int removeCount = qMin(insert->end, bEnd) - qMax(insert->start, bStart);
            const int relativeStart = bStart - insert->start;

            insert->start += removeCount;
            if (insert->start == insert->end) {
                insert->start = fStart;
                insert->end = insert->start + removeCount;
            } else {
                insert = m_inserts.insert(insert, Insert(fStart, fStart + removeCount));
                ++insert;
            }
            if (relativeStart < 0) {
                moveChanges.append(Move(fStart, fStart - relativeStart, fTo + removeCount));
                fStart -= relativeStart;
                fTo -= relativeStart;
            }
            fStart += removeCount;
            fTo += removeCount;
        }
    }

    if (fTo != bEnd)
        moveChanges.append(Move(fStart, fStart + bEnd - fTo, fTo));

    QVector<Insert>::iterator it = insertChanges.begin();
    for (insert = m_inserts.begin(); it != insertChanges.end() && insert != m_inserts.end();++insert) {
        if (it->start < insert->start) {
            insert = m_inserts.insert(insert, *it);
            ++it;
        } else if (it->start <= insert->end) {
            insert->end += it->count();
            ++it;
        }
    }
    for (; it != insertChanges.end(); ++it)
        m_inserts.append(*it);

    // Insert queued moved signals ordered by destination position.
    QVector<Move>::iterator move = m_moves.begin();
    if (start > to) {
        for (QVector<Move>::iterator it = moveChanges.begin(); it != moveChanges.end(); ++it) {
            it->fend += it->tstart - it->fstart;
            it->tend -=it->tstart - it->fstart;
            qSwap(it->fstart, it->tstart);
            for (; move != m_moves.end() && it->to >= qMin(move->fstart, move->tstart); ++move) {}
            move = m_moves.insert(move, *it);
        }
    } else {
        for (QVector<Move>::iterator it = moveChanges.begin(); it != moveChanges.end(); ++it) {
            for (; move != m_moves.end() && it->start >= qMin(move->fstart, move->tstart); ++move) {}
            move = m_moves.insert(move, *it);
        }
    }
}

void QDeclarativeChangeSet::insertChange(int start, int end)
{
    QVector<Change> filteredChanges;

    // Inserted signals (don't emit change signals on new items).
    QVector<Insert>::iterator insert = m_inserts.begin();
    for (; insert != m_inserts.end() && start >= insert->end; ++insert) {}
    for (; insert != m_inserts.end() && end > insert->start; ++insert) {
        if (start < insert->start)
            filteredChanges.append(Change(start, insert->start));
        start = insert->end;
    }
    if (start < end)
        filteredChanges.append(Change(start, end));

    // Find the union of the existing and filtered sets of change signals.
    QVector<Change>::iterator change = m_changes.begin();
    for (QVector<Change>::iterator it = filteredChanges.begin(); it != filteredChanges.end(); ++it) {
        for (; change != m_changes.end() && change->end < it->start; ++change) {}
        if (change == m_changes.end() || change->start > it->end) {
            change = m_changes.insert(change, *it);
        } else {
            if (it->start < change->start)
                change->start = it->start;

            if (it->end > change->end) {
                change->end = it->end;
                QVector<Change>::iterator rbegin = change;
                QVector<Change>::iterator rend = ++rbegin;
                for (; rend != m_changes.end() && rend->start <= change->end; ++rend) {
                    if (rend->end > change->end)
                        change->end = rend->end;
                }
                if (rbegin != rend) {
                    change = m_changes.erase(rbegin, rend);
                    --change;
                }
            }
        }
    }

}

QDebug operator <<(QDebug debug, const QDeclarativeChangeSet &set)
{
    foreach (const QDeclarativeChangeSet::Remove &remove, set.removes())
        debug.nospace() << "QDeclarativeChangeSet::Remove(" << remove.start << "," << remove.end << ")";
    foreach (const QDeclarativeChangeSet::Insert &insert, set.inserts())
        debug.nospace() << "QDeclarativeChangeSet::Insert(" << insert.start << "," << insert.end << ")";
    foreach (const QDeclarativeChangeSet::Move &move, set.moves())
        debug.nospace() << "QDeclarativeChangeSet::Move(" << move.start << "," << move.end << "," << move.to << ")";
    foreach (const QDeclarativeChangeSet::Change &change, set.changes())
        debug.nospace() << "QDeclarativeChangeSet::Change(" << change.start << "," << change.end << ")";
    return debug;
}

