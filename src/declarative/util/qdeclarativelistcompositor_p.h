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
#include <QtCore/qlinkedlist.h>
#include <QtCore/qvector.h>

#include <private/qdeclarativechangeset_p.h>

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

class QDeclarativeCompositeRange
{
public:
    QDeclarativeCompositeRange() {}
    QDeclarativeCompositeRange(void *list, int index, int count, uint flags)
        : list(list), index(index), count(count), flags(flags) {}

    void *list;
    int index;
    int count;
    uint flags;

    inline bool null() const;
    inline bool prepend() const;
    inline bool append() const;
    inline bool internal() const;
};

class QDeclarativeListCompositor
{
public:
    enum Flag
    {
        Null     = 0x01,
        Prepend  = 0x02,
        Append   = 0x04,
        Internal = 0x08
    };

    QDeclarativeListCompositor(int internalCount);
    ~QDeclarativeListCompositor();

    int count() const;
    QDeclarativeCompositeRange at(int index, int *offset, int *internalIndex) const;

    void appendList(void *list, int start, int count, bool grow);
    bool appendData(const void *data);
    void insertList(int index, void *list, int start, int count, bool grow);
    bool insertData(int index, const void *data);
    int replaceAt(int index, const void *data);
    void removeAt(int index, int count);
    void removeList(void *list, QVector<QDeclarativeChangeSet::Remove> *changes);
    void move(int from, int to, int count);
    int merge(int from, int to);
    void clear();
    void clearData(int internalIndex, int count);

    int absoluteIndexOf(int internalIndex) const;
    int absoluteIndexOf(void *list, int index, int from = 0) const;

    void listItemsInserted(void *list, int start, int end, QVector<QDeclarativeChangeSet::Insert> *changes);
    void listItemsRemoved(void *list, int start, int end, QVector<QDeclarativeChangeSet::Remove> *changes);
    void listItemsMoved(void *list, int start, int end, int to, QVector<QDeclarativeChangeSet::Move> *changes);
    void listItemsChanged(void *list, int start, int end, QVector<QDeclarativeChangeSet::Change> *changes);

    void compress();

protected:
    virtual void rangeCreated(void *list) = 0;
    virtual void rangeDestroyed(void *list) = 0;

    virtual bool insertInternalData(int index, const void *data) = 0;
    virtual void replaceInternalData(int index, const void *data) = 0;
    virtual void removeInternalData(int index, int count) = 0;
    virtual void moveInternalData(int from, int to, int count) = 0;
    virtual bool mergeInternalData(int from, int to);
    virtual bool mergeInternalData(int internalIndex, void *list, int range);

private:
    int absoluteCount;
    int internalCount;

    QLinkedList<QDeclarativeCompositeRange> ranges;

    typedef QLinkedList<QDeclarativeCompositeRange>::iterator iterator;
    typedef QLinkedList<QDeclarativeCompositeRange>::const_iterator const_iterator;

    struct Move
    {
        Move() {}
        Move(int changeIndex, int internalIndex = -1)
            : changeIndex(changeIndex), internalIndex(internalIndex) {}
        static Move nullMove(int count) { return Move(-count); }

        bool null() const { return changeIndex < 0; }
        bool internal() const { return internalIndex >= 0; }
        int count() const { return -changeIndex; }

        int changeIndex;
        int internalIndex;
    };

    static void backwardToForwardMove(int &from, int &to, int &count);
    static void forwardToBackwardMove(int &from, int &to, int &count);

    iterator insert(iterator before, const QDeclarativeCompositeRange &range);
    iterator erase(iterator pos);

    iterator insertIntoRange(iterator range, int insertAt, const QDeclarativeCompositeRange &insertRange);
    iterator listItemsMovedForward(
            void *list,
            int from,
            int to,
            int count,
            QVector<QDeclarativeChangeSet::Move> *changes,
            iterator pos,
            int &absoluteIndex,
            int &internalIndex);
    friend QDebug operator <<(QDebug debug, const QDeclarativeListCompositor &list);
};

inline bool QDeclarativeCompositeRange::null() const { return flags & QDeclarativeListCompositor::Null; }
inline bool QDeclarativeCompositeRange::prepend() const { return flags & QDeclarativeListCompositor::Prepend; }
inline bool QDeclarativeCompositeRange::append() const { return flags & QDeclarativeListCompositor::Append; }
inline bool QDeclarativeCompositeRange::internal() const { return flags & QDeclarativeListCompositor::Internal; }

QDebug operator <<(QDebug debug, const QDeclarativeListCompositor &list);

QT_END_NAMESPACE

#endif
