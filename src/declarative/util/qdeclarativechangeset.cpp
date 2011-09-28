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

#include "qdeclarativechangeset_p.h"

QT_BEGIN_NAMESPACE

QDeclarativeChangeSet::QDeclarativeChangeSet()
    : m_moveCounter(0)
{
}

QDeclarativeChangeSet::QDeclarativeChangeSet(const QDeclarativeChangeSet &changeSet)
    : m_removes(changeSet.m_removes)
    , m_inserts(changeSet.m_inserts)
    , m_changes(changeSet.m_changes)
    , m_moveCounter(changeSet.m_moveCounter)
{
}

QDeclarativeChangeSet::~QDeclarativeChangeSet()
{
}

QDeclarativeChangeSet &QDeclarativeChangeSet::operator =(const QDeclarativeChangeSet &changeSet)
{
    m_removes = changeSet.m_removes;
    m_inserts = changeSet.m_inserts;
    m_changes = changeSet.m_changes;
    m_moveCounter = changeSet.m_moveCounter;
    return *this;
}

void QDeclarativeChangeSet::insert(int index, int count)
{
    applyInsertions(QVector<Insert>() << Insert(index, count));
}

void QDeclarativeChangeSet::remove(int index, int count)
{
    QVector<Insert> i;
    applyRemovals(QVector<Remove>() << Remove(index, count), i);
}

void QDeclarativeChangeSet::move(int from, int to, int count)
{
    apply(QVector<Remove>() << Remove(from, count, -2), QVector<Insert>() << Insert(to, count, -2));
}

void QDeclarativeChangeSet::change(int index, int count)
{
    applyChanges(QVector<Change>() << Change(index, count));
}

void QDeclarativeChangeSet::apply(const QDeclarativeChangeSet &changeSet)
{
    apply(changeSet.m_removes, changeSet.m_inserts, changeSet.m_changes);
}

void QDeclarativeChangeSet::apply(const QVector<Remove> &removals)
{
    QVector<Remove> r = removals;
    QVector<Insert> i;
    applyRemovals(r, i);
}

void QDeclarativeChangeSet::apply(const QVector<Insert> &insertions)
{
    QVector<Insert> i = insertions;
    applyInsertions(i);
}

void QDeclarativeChangeSet::apply(const QVector<Change> &changes)
{
    QVector<Change> c = changes;
    applyChanges(c);
}

void QDeclarativeChangeSet::apply(const QVector<Remove> &removals, const QVector<Insert> &insertions, const QVector<Change> &changes)
{
    QVector<Remove> r = removals;
    QVector<Insert> i = insertions;
    QVector<Change> c = changes;
    applyRemovals(r, i);
    applyInsertions(i);
    applyChanges(c);
}

void QDeclarativeChangeSet::applyRemovals(QVector<Remove> &removals, QVector<Insert> &insertions)
{
    int removeCount = 0;
    int insertCount = 0;
    QVector<Insert>::iterator insert = m_inserts.begin();
    QVector<Change>::iterator change = m_changes.begin();
    QVector<Remove>::iterator rit = removals.begin();
    for (; rit != removals.end(); ++rit) {
        int index = rit->index + removeCount;
        int count = rit->count;

        QVector<Insert>::iterator iit = insertions.begin();
        for (; rit->moveId != -1 && iit != insertions.end() && iit->moveId != rit->moveId; ++iit) {}

        for (QVector<Remove>::iterator nrit = rit + 1; nrit != removals.end(); nrit = rit + 1) {
            if (nrit->index != rit->index || (rit->moveId == -1) != (nrit->moveId == -1))
                break;
            if (nrit->moveId != -1) {
                QVector<Insert>::iterator niit = iit + 1;
                if (niit->moveId != nrit->moveId || niit->index != iit->index + iit->count)
                    break;
                niit->index = iit->index;
                niit->count += iit->count;
                iit = insertions.erase(iit);
            }
            nrit->count += rit->count;
            rit = removals.erase(rit);
        }

        for (; change != m_changes.end() && change->end() < rit->index; ++change) {}
        for (; change != m_changes.end() && change->index > rit->end(); ++change) {
            change->count -= qMin(change->end(), rit->end()) - qMax(change->index, rit->index);
            if (change->count == 0) {
                change = m_changes.erase(change);
            } else if (rit->index < change->index) {
                change->index = rit->index;
            }
        }
        for (; insert != m_inserts.end() && insert->end() <= index; ++insert) {
            insertCount += insert->count;
            insert->index -= removeCount;
        }
        for (; insert != m_inserts.end() && insert->index < index + count; ++insert) {
            const int offset = insert->index - index;
            const int difference = qMin(insert->end(), index + count) - qMax(insert->index, index);
            const int moveId = rit->moveId != -1 ? m_moveCounter++ : -1;
            if (insert->moveId != -1) {
                QVector<Remove>::iterator remove = m_removes.begin();
                for (; remove != m_removes.end() && remove->moveId != insert->moveId; ++remove) {}
                Q_ASSERT(remove != m_removes.end());
                const int offset = index - insert->index;
                if (rit->moveId != -1 && offset < 0) {
                    const int moveId = m_moveCounter++;
                    iit = insertions.insert(iit, Insert(iit->index, -offset, moveId));
                    ++iit;
                    iit->index += -offset;
                    iit->count -= -offset;
                    rit = removals.insert(rit, Remove(rit->index, -offset, moveId));
                    ++rit;
                    rit->count -= -offset;
                }

                if (offset > 0) {
                    const int moveId = m_moveCounter++;
                    insert = m_inserts.insert(insert, Insert(insert->index, offset, moveId));
                    ++insert;
                    insert->index += offset;
                    insert->count -= offset;
                    remove = m_removes.insert(remove, Remove(remove->index, offset, moveId));
                    ++remove;
                    remove->count -= offset;
                    rit->index -= offset;
                    index += offset;
                    count -= offset;
                }

                if (remove->count == difference) {
                    remove->moveId = moveId;
                } else {
                    remove = m_removes.insert(remove, Remove(remove->index, difference, moveId));
                    ++remove;
                    remove->count -= difference;
                }
            } else if (rit->moveId != -1 && offset > 0) {
                const int moveId = m_moveCounter++;
                iit = insertions.insert(iit, Insert(iit->index, offset, moveId));
                ++iit;
                iit->index += offset;
                iit->count -= offset;
                rit = removals.insert(rit, Remove(rit->index, offset, moveId));
                ++rit;
                rit->count -= offset;
                index += offset;
                count -= offset;
            }

            if (rit->moveId != -1 && difference > 0) {
                iit = insertions.insert(iit, Insert(
                        iit->index, difference, insert->moveId != -1 ? moveId : -1));
                ++iit;
                iit->index += difference;
                iit->count -= difference;
            }

            insert->count -= difference;
            rit->count -= difference;
            if (insert->count == 0) {
                insert = m_inserts.erase(insert);
                --insert;
            } else if (index <= insert->index) {
                insert->index = rit->index;
            } else {
                rit->index -= insert->count;
            }
            index += difference;
            count -= difference;
            removeCount += difference;
        }
        rit->index -= insertCount;
        removeCount += rit->count;

        if (rit->count == 0) {
            if (rit->moveId != -1 && iit->count == 0)
                insertions.erase(iit);
            rit = removals.erase(rit);
            --rit;
        } else if (rit->moveId != -1) {
            const int moveId = m_moveCounter++;
            rit->moveId = moveId;
            iit->moveId = moveId;
        }
    }
    for (; change != m_changes.end(); ++change)
        change->index -= removeCount;
    for (; insert != m_inserts.end(); ++insert)
        insert->index -= removeCount;

    removeCount = 0;
    QVector<Remove>::iterator remove = m_removes.begin();
    for (rit = removals.begin(); rit != removals.end(); ++rit) {
        QVector<Insert>::iterator iit = insertions.begin();
        int index = rit->index + removeCount;
        for (; rit->moveId != -1 && iit != insertions.end() && iit->moveId != rit->moveId; ++iit) {}
        for (; remove != m_removes.end() && index > remove->index; ++remove)
            remove->index -= removeCount;
        while (remove != m_removes.end() && index + rit->count > remove->index) {
            int count = 0;
            const int offset = remove->index - index - removeCount;
            QVector<Remove>::iterator rend = remove;
            for (; rend != m_removes.end()
                    && rit->moveId == -1
                    && rend->moveId == -1
                    && rit->index + rit->count > rend->index; ++rend) {
                count += rend->count;
            }
            if (remove != rend) {
                const int difference = rend == m_removes.end() || rit->index + rit->count < rend->index - removeCount
                        ? rit->count
                        : offset;
                count += difference;

                index += difference;
                rit->count -= difference;
                removeCount += difference;

                remove->index = rit->index;
                remove->count = count;
                remove = m_removes.erase(++remove, rend);
            } else if (rit->moveId != -1) {
                if (offset > 0) {
                    const int moveId = m_moveCounter++;
                    iit = insertions.insert(iit, Insert(iit->index, offset, moveId));
                    ++iit;
                    iit->index += offset;
                    iit->count -= offset;
                    remove = m_removes.insert(remove, Remove(rit->index, offset, moveId));
                    ++remove;
                    rit->count -= offset;
                }
                remove->index = rit->index;
                index += offset;
                removeCount += offset;

                ++remove;
            } else {
                if (offset > 0) {
                    remove = m_removes.insert(remove, Remove(rit->index, offset));
                    ++remove;
                    rit->count -= offset;
                }
                remove->index = rit->index;
                index += offset;
                removeCount += offset;

                ++remove;
            }
            index += count;
            rit->count -= count;
        }

        if (rit->count > 0) {
            remove = m_removes.insert(remove, *rit);
            ++remove;
        }
        removeCount += rit->count;
    }
    for (; remove != m_removes.end(); ++remove)
        remove->index -= removeCount;
}

void QDeclarativeChangeSet::applyInsertions(QVector<Insert> &insertions)
{
    int insertCount = 0;
    QVector<Insert>::iterator insert = m_inserts.begin();
    QVector<Change>::iterator change = m_changes.begin();
    for (QVector<Insert>::iterator iit = insertions.begin(); iit != insertions.end(); ++iit) {
        int index = iit->index - insertCount;
        int count = iit->count;
        for (; change != m_changes.end() && change->index >= index; ++change)
            change->index += insertCount;
        if (change != m_changes.end() && change->index < index + count) {
                int offset = index - change->index;
                change = m_changes.insert(change, Change(change->index + insertCount, offset));
                ++change;
                change->index += count + offset;
                change->count -= offset;
        }
        for (; insert != m_inserts.end() && iit->index > insert->index + insert->count; ++insert)
            insert->index += insertCount;
        if (insert == m_inserts.end()) {
            insert = m_inserts.insert(insert, *iit);
            ++insert;
        } else {
            const int offset = index - insert->index;
            if (offset < 0 || (offset == 0 && (iit->moveId != -1 || insert->moveId != -1))) {
                insert = m_inserts.insert(insert, *iit);
                ++insert;
            } else if (iit->moveId == -1 && insert->moveId == -1) {
                insert->index -= iit->count;
                insert->count += iit->count;
            } else if (offset < insert->count) {
                const int moveId = insert->moveId != -1 ? m_moveCounter++ : -1;
                insert = m_inserts.insert(insert, Insert(insert->index + insertCount, offset, moveId));
                ++insert;
                insert->index += offset;
                insert->count -= offset;
                insert = m_inserts.insert(insert, *iit);
                ++insert;

                if (insert->moveId != -1) {
                    QVector<Remove>::iterator remove = m_removes.begin();
                    for (; remove != m_removes.end() && remove->moveId != insert->moveId; ++remove) {}
                    Q_ASSERT(remove != m_removes.end());
                    if (remove->count == offset) {
                        remove->moveId = moveId;
                    } else {
                        remove = m_removes.insert(remove, Remove(remove->index, offset, moveId));
                        ++remove;
                        remove->count -= offset;
                    }
                }
            } else {
                ++insert;
                insert = m_inserts.insert(insert, *iit);
                ++insert;
            }
            insertCount += iit->count;
        }
    }
    for (; change != m_changes.end(); ++change)
        change->index += insertCount;
    for (; insert != m_inserts.end(); ++insert)
        insert->index += insertCount;
}

void QDeclarativeChangeSet::applyChanges(QVector<Change> &changes)
{
    QVector<Insert>::iterator insert = m_inserts.begin();
    QVector<Change>::iterator change = m_changes.begin();
    for (QVector<Change>::iterator cit = changes.begin(); cit != changes.end(); ++cit) {
        for (; insert != m_inserts.end() && insert->end() < cit->index; ++insert) {}
        for (; insert != m_inserts.end() && insert->index < cit->end(); ++insert) {
            const int offset = insert->index - cit->index;
            const int count = cit->count + cit->index - insert->index - insert->count;
            if (offset == 0) {
                cit->index = insert->index + insert->count;
                cit->count = count;
            } else {
                cit = changes.insert(++cit, Change(insert->index + insert->count, count));
                --cit;
                cit->count = offset;
            }
        }

        for (; change != m_changes.end() && change->index + change->count < cit->index; ++change) {}
        if (change == m_changes.end() || change->index > cit->index + cit->count) {
            if (cit->count > 0) {
                change = m_changes.insert(change, *cit);
                ++change;
            }
        } else {
            if (cit->index < change->index) {
                change->count += change->index - cit->index;
                change->index = cit->index;
            }

            if (cit->index + cit->count > change->index + change->count) {
                change->count = cit->index + cit->count - change->index;
                QVector<Change>::iterator rbegin = change;
                QVector<Change>::iterator rend = ++rbegin;
                for (; rend != m_changes.end() && rend->index <= change->index + change->count; ++rend) {
                    if (rend->index + rend->count > change->index + change->count)
                        change->count = rend->index + rend->count - change->index;
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
    debug.nospace() << "QDeclarativeChangeSet(";
    foreach (const QDeclarativeChangeSet::Remove &remove, set.removes()) debug << remove;
    foreach (const QDeclarativeChangeSet::Insert &insert, set.inserts()) debug << insert;
    foreach (const QDeclarativeChangeSet::Change &change, set.changes()) debug << change;
    return debug.nospace() << ")";
}

QDebug operator <<(QDebug debug, const QDeclarativeChangeSet::Remove &remove)
{
    return (debug.nospace() << "Remove(" << remove.index << "," << remove.count << "," << remove.moveId << ")").space();
}

QDebug operator <<(QDebug debug, const QDeclarativeChangeSet::Insert &insert)
{
    return (debug.nospace() << "Insert(" << insert.index << "," << insert.count << "," << insert.moveId << ")").space();
}

QDebug operator <<(QDebug debug, const QDeclarativeChangeSet::Change &change)
{
    return (debug.nospace() << "Change(" << change.index << "," << change.count << ")").space();
}

QT_END_NAMESPACE

