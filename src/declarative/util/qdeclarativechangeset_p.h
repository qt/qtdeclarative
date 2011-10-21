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

#ifndef QDECLARATIVECHANGESET_P_H
#define QDECLARATIVECHANGESET_P_H

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

#include <QtCore/qdebug.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

class Q_AUTOTEST_EXPORT QDeclarativeChangeSet
{
public:
    struct MoveKey
    {
        MoveKey() : moveId(-1), offset(0) {}
        MoveKey(int moveId, int offset) : moveId(moveId), offset(offset) {}
        int moveId;
        int offset;
    };

    struct Change
    {
        Change() : index(0), count(0), moveId(-1) {}
        Change(int index, int count) : index(index), count(count), moveId(-1) {}
        Change(int index, int count, int moveId) : index(index), count(count), moveId(moveId) {}

        int index;
        int count;
        int moveId;

        bool isMove() const { return moveId >= 0; }

        MoveKey moveKey(int index) const { return MoveKey(moveId, index - Change::index); }

        int start() const { return index; }
        int end() const { return index + count; }
    };


    struct Insert : public Change
    {
        Insert() {}
        Insert(int index, int count) : Change(index, count) {}
        Insert(int index, int count, int moveId) : Change(index, count, moveId) {}
    };

    struct Remove : public Change
    {
        Remove() {}
        Remove(int index, int count) : Change(index, count) {}
        Remove(int index, int count, int moveId) : Change(index, count, moveId) {}
    };

    QDeclarativeChangeSet();
    QDeclarativeChangeSet(const QDeclarativeChangeSet &changeSet);
    ~QDeclarativeChangeSet();

    QDeclarativeChangeSet &operator =(const QDeclarativeChangeSet &changeSet);

    const QVector<Remove> &removes() const { return m_removes; }
    const QVector<Insert> &inserts() const { return m_inserts; }
    const QVector<Change> &changes() const {return  m_changes; }

    void insert(int index, int count);
    void remove(int index, int count);
    void move(int from, int to, int count);
    void change(int index, int count);

    void apply(const QDeclarativeChangeSet &changeSet);
    void apply(const QVector<Remove> &removals);
    void apply(const QVector<Insert> &insertions);
    void apply(const QVector<Change> &changes);
    void apply(
            const QVector<Remove> &removals,
            const QVector<Insert> &insertions,
            const QVector<Change> &changes = QVector<Change>());

    bool isEmpty() const { return m_removes.empty() && m_inserts.empty() && m_changes.empty(); }

    void clear()
    {
        m_removes.clear();
        m_inserts.clear();
        m_changes.clear();
        m_moveCounter = 0;
        m_difference = 0;
    }

    int moveCounter() const { return m_moveCounter; }
    int difference() const { return m_difference; }

private:
    void applyRemovals(QVector<Remove> &removals, QVector<Insert> &insertions);
    void applyInsertions(QVector<Insert> &insertions);
    void applyChanges(QVector<Change> &changes);

    QVector<Remove> m_removes;
    QVector<Insert> m_inserts;
    QVector<Change> m_changes;
    int m_moveCounter;
    int m_difference;
};

inline uint qHash(const QDeclarativeChangeSet::MoveKey &key) { return qHash(qMakePair(key.moveId, key.offset)); }
inline bool operator ==(const QDeclarativeChangeSet::MoveKey &l, const QDeclarativeChangeSet::MoveKey &r) {
    return l.moveId == r.moveId && l.offset == r.offset; }

Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeChangeSet &change);
Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeChangeSet::Remove &remove);
Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeChangeSet::Insert &insert);
Q_AUTOTEST_EXPORT QDebug operator <<(QDebug debug, const QDeclarativeChangeSet::Change &change);

QT_END_NAMESPACE

#endif
