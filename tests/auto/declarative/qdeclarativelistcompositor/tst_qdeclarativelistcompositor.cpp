/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
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
#include <qtest.h>
#include <private/qdeclarativelistcompositor_p.h>

template<typename T, int N> int lengthOf(const T (&)[N]) { return N; }

typedef QDeclarativeListCompositor C;

class tst_qdeclarativelistcompositor : public QObject
{
    Q_OBJECT

    enum {
        VisibleFlag   = 0x04,
        SelectionFlag = 0x08
    };

    static const C::Group Visible = C::Group(2);
    static const C::Group Selection = C::Group(3);

private slots:
    void insert();
    void clearFlags();
    void setFlags();
    void move();
    void clear();
    void listItemsInserted();
    void listItemsRemoved();
    void listItemsMoved();
    void listItemsChanged();
};

void tst_qdeclarativelistcompositor::insert()
{
    QDeclarativeListCompositor compositor;
    compositor.setGroupCount(4);
    compositor.setDefaultGroups(VisibleFlag | C::DefaultFlag);

    C::iterator it;

    int listA; int *a = &listA;
    int listB; int *b = &listB;
    int listC; int *c = &listC;

    {
        compositor.append(a, 0, 12, C::AppendFlag | C::PrependFlag | C::DefaultFlag);
        const int indexes[] = {0,1,2,3,4,5,6,7,8,9,10,11};
        const int *lists[]  = {a,a,a,a,a,a,a,a,a,a, a, a};
        QCOMPARE(compositor.count(C::Default), lengthOf(indexes));
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    } {
        compositor.append(b, 4, 4, C::DefaultFlag);
        const int indexes[] = {0,1,2,3,4,5,6,7,8,9,10,11,4,5,6,7};
        const int *lists[]  = {a,a,a,a,a,a,a,a,a,a, a, a,b,b,b,b};
        QCOMPARE(compositor.count(C::Default), lengthOf(indexes));
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    } { // Insert at end.
        compositor.insert(
                C::Default, 16, c, 2, 2, C::DefaultFlag);
        const int indexes[] = {0,1,2,3,4,5,6,7,8,9,10,11,4,5,6,7,2,3};
        const int *lists[]  = {a,a,a,a,a,a,a,a,a,a, a, a,b,b,b,b,c,c};
        QCOMPARE(compositor.count(C::Default), lengthOf(indexes));
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    } { // Insert at start
        compositor.insert(
                C::Default, 0, c, 6, 4, C::DefaultFlag);
        const int indexes[] = {6,7,8,9,0,1,2,3,4,5,6,7,8,9,10,11,4,5,6,7,2,3};
        const int *lists[]  = {c,c,c,c,a,a,a,a,a,a,a,a,a,a, a, a,b,b,b,b,c,c};
        QCOMPARE(compositor.count(C::Default), lengthOf(indexes));
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    } { // Insert after static range.
        compositor.insert(
                C::Default, 4, b, 0, 8, C::AppendFlag | C::PrependFlag | C::DefaultFlag);
        const int indexes[] = {6,7,8,9,0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,8,9,10,11,4,5,6,7,2,3};
        const int *lists[]  = {c,c,c,c,b,b,b,b,b,b,b,b,a,a,a,a,a,a,a,a,a,a, a, a,b,b,b,b,c,c};
        QCOMPARE(compositor.count(C::Default), lengthOf(indexes));
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    } { // Insert at end of dynamic range.
        compositor.insert(
                C::Default, 12, c, 0, 4, C::AppendFlag | C::PrependFlag | C::DefaultFlag);
        const int indexes[] = {6,7,8,9,0,1,2,3,4,5,6,7,0,1,2,3,0,1,2,3,4,5,6,7,8,9,10,11,4,5,6,7,2,3};
        const int *lists[]  = {c,c,c,c,b,b,b,b,b,b,b,b,c,c,c,c,a,a,a,a,a,a,a,a,a,a, a, a,b,b,b,b,c,c};
        QCOMPARE(compositor.count(C::Default), lengthOf(indexes));
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    } { // Insert into range.
        compositor.insert(
                C::Default, 8, c, 0, 4, C::AppendFlag | C::PrependFlag | C::DefaultFlag);
        const int indexes[] = {6,7,8,9,0,1,2,3,0,1,2,3,4,5,6,7,0,1,2,3,0,1,2,3,4,5,6,7,8,9,10,11,4,5,6,7,2,3};
        const int *lists[]  = {c,c,c,c,b,b,b,b,c,c,c,c,b,b,b,b,c,c,c,c,a,a,a,a,a,a,a,a,a,a, a, a,b,b,b,b,c,c};
        QCOMPARE(compositor.count(C::Default), lengthOf(indexes));
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    }
}

void tst_qdeclarativelistcompositor::clearFlags()
{
    QDeclarativeListCompositor compositor;
    compositor.setGroupCount(4);
    compositor.setDefaultGroups(VisibleFlag | C::DefaultFlag);
    int listA;

    QVector<C::Remove> removes;
    compositor.append(&listA, 0, 12, C::PrependFlag | C::DefaultFlag | VisibleFlag | C::CacheFlag | SelectionFlag);
    compositor.append(0, 0, 4, C::DefaultFlag | VisibleFlag | C::CacheFlag | SelectionFlag);
    QCOMPARE(compositor.count(C::Default), 16);
    QCOMPARE(compositor.count(Visible), 16);
    QCOMPARE(compositor.count(C::Cache), 16);
    QCOMPARE(compositor.count(Selection), 16);

    compositor.clearFlags(C::Default, 2, 2, SelectionFlag, &removes);
    QCOMPARE(removes.count(), 1);
    QCOMPARE(removes.at(0).index[C::Default], 2);
    QCOMPARE(removes.at(0).index[Visible], 2);
    QCOMPARE(removes.at(0).index[C::Cache], 2);
    QCOMPARE(removes.at(0).index[Selection], 2);
    QCOMPARE(removes.at(0).count, 2);
    QCOMPARE(removes.at(0).flags, SelectionFlag | C::CacheFlag);
    QCOMPARE(compositor.count(C::Default), 16);
    QCOMPARE(compositor.count(Visible), 16);
    QCOMPARE(compositor.count(C::Cache), 16);
    QCOMPARE(compositor.count(Selection), 14);
    QCOMPARE(compositor.find(C::Default, 1)->flags, C::PrependFlag | C::DefaultFlag | VisibleFlag | C::CacheFlag | SelectionFlag);
    QCOMPARE(compositor.find(C::Default, 2)->flags, C::PrependFlag | C::DefaultFlag | VisibleFlag | C::CacheFlag);
    QCOMPARE(compositor.find(C::Default, 3)->flags, C::PrependFlag | C::DefaultFlag | VisibleFlag | C::CacheFlag);
    QCOMPARE(compositor.find(C::Default, 4)->flags, C::PrependFlag | C::DefaultFlag | VisibleFlag | C::CacheFlag | SelectionFlag);

    removes.clear();
    compositor.clearFlags(Selection, 1, 2, VisibleFlag, &removes);
    QCOMPARE(removes.count(), 2);
    QCOMPARE(removes.at(0).index[C::Default], 1);
    QCOMPARE(removes.at(0).index[Visible], 1);
    QCOMPARE(removes.at(0).index[C::Cache], 1);
    QCOMPARE(removes.at(0).index[Selection], 1);
    QCOMPARE(removes.at(0).count, 1);
    QCOMPARE(removes.at(0).flags, VisibleFlag | C::CacheFlag);
    QCOMPARE(removes.at(1).index[C::Default], 4);
    QCOMPARE(removes.at(1).index[Visible], 3);
    QCOMPARE(removes.at(1).index[C::Cache], 4);
    QCOMPARE(removes.at(1).index[Selection], 2);
    QCOMPARE(removes.at(1).count, 1);
    QCOMPARE(removes.at(1).flags, VisibleFlag | C::CacheFlag);
    QCOMPARE(compositor.count(C::Default), 16);
    QCOMPARE(compositor.count(Visible), 14);
    QCOMPARE(compositor.count(C::Cache), 16);
    QCOMPARE(compositor.count(Selection), 14);
    QCOMPARE(compositor.find(C::Default, 1)->flags, C::PrependFlag | C::DefaultFlag | C::CacheFlag | SelectionFlag);
    QCOMPARE(compositor.find(C::Default, 2)->flags, C::PrependFlag | C::DefaultFlag | VisibleFlag | C::CacheFlag);
    QCOMPARE(compositor.find(C::Default, 3)->flags, C::PrependFlag | C::DefaultFlag | VisibleFlag | C::CacheFlag);
    QCOMPARE(compositor.find(C::Default, 4)->flags, C::PrependFlag | C::DefaultFlag | C::CacheFlag | SelectionFlag);

    removes.clear();
    compositor.clearFlags(C::Default, 13, 1, C::PrependFlag | C::DefaultFlag | VisibleFlag| SelectionFlag, &removes);
    QCOMPARE(removes.count(), 1);
    QCOMPARE(removes.at(0).index[C::Default], 13);
    QCOMPARE(removes.at(0).index[Visible], 11);
    QCOMPARE(removes.at(0).index[C::Cache], 13);
    QCOMPARE(removes.at(0).index[Selection], 11);
    QCOMPARE(removes.at(0).count, 1);
    QCOMPARE(removes.at(0).flags, C::DefaultFlag | VisibleFlag | C::CacheFlag | SelectionFlag);
    QCOMPARE(compositor.count(C::Default), 15);
    QCOMPARE(compositor.count(Visible), 13);
    QCOMPARE(compositor.count(C::Cache), 16);
    QCOMPARE(compositor.count(Selection), 13);
    QCOMPARE(compositor.find(C::Default, 11)->flags, C::PrependFlag | C::DefaultFlag | VisibleFlag | C::CacheFlag | SelectionFlag);
    QCOMPARE(compositor.find(C::Cache, 11)->flags, C::PrependFlag | C::DefaultFlag | VisibleFlag | C::CacheFlag | SelectionFlag);
    QCOMPARE(compositor.find(C::Default, 12)->flags, C::DefaultFlag | VisibleFlag | C::CacheFlag | SelectionFlag);
    QCOMPARE(compositor.find(C::Cache, 12)->flags, C::DefaultFlag | VisibleFlag | C::CacheFlag | SelectionFlag);
    QCOMPARE(compositor.find(C::Cache, 13)->flags, int(C::CacheFlag));
    QCOMPARE(compositor.find(C::Default, 13)->flags, C::DefaultFlag | VisibleFlag | C::CacheFlag | SelectionFlag);
    QCOMPARE(compositor.find(C::Cache, 14)->flags, C::DefaultFlag | VisibleFlag | C::CacheFlag | SelectionFlag);

    removes.clear();
    compositor.clearFlags(C::Cache, 11, 4, C::CacheFlag);
    QCOMPARE(removes.count(), 0);
    QCOMPARE(compositor.count(C::Default), 15);
    QCOMPARE(compositor.count(Visible), 13);
    QCOMPARE(compositor.count(C::Cache), 12);
    QCOMPARE(compositor.count(Selection), 13);
    QCOMPARE(compositor.find(C::Default, 11)->flags, C::PrependFlag | C::DefaultFlag | VisibleFlag| SelectionFlag);
    QCOMPARE(compositor.find(C::Default, 12)->flags, C::DefaultFlag | VisibleFlag| SelectionFlag);
    QCOMPARE(compositor.find(C::Default, 13)->flags, C::DefaultFlag | VisibleFlag| SelectionFlag);

    removes.clear();
    compositor.clearFlags(C::Default, 11, 3, C::DefaultFlag | VisibleFlag| SelectionFlag, &removes);
    QCOMPARE(removes.count(), 2);
    QCOMPARE(removes.at(0).index[C::Default], 11);
    QCOMPARE(removes.at(0).index[Visible], 9);
    QCOMPARE(removes.at(0).index[C::Cache], 11);
    QCOMPARE(removes.at(0).index[Selection], 9);
    QCOMPARE(removes.at(0).count, 1);
    QCOMPARE(removes.at(0).flags, C::DefaultFlag | VisibleFlag| SelectionFlag);
    QCOMPARE(removes.at(1).index[C::Default], 11);
    QCOMPARE(removes.at(1).index[Visible], 9);
    QCOMPARE(removes.at(1).index[C::Cache], 11);
    QCOMPARE(removes.at(1).index[Selection], 9);
    QCOMPARE(removes.at(1).count, 2);
    QCOMPARE(removes.at(1).flags, C::DefaultFlag | VisibleFlag| SelectionFlag);
}

void tst_qdeclarativelistcompositor::setFlags()
{
    QDeclarativeListCompositor compositor;
    compositor.setGroupCount(4);
    compositor.setDefaultGroups(VisibleFlag | C::DefaultFlag);
    int listA;

    QVector<C::Insert> inserts;
    compositor.append(&listA, 0, 12, C::DefaultFlag);
    compositor.append(0, 0, 4, C::CacheFlag);
    QCOMPARE(compositor.count(C::Default), 12);
    QCOMPARE(compositor.count(Visible), 0);
    QCOMPARE(compositor.count(C::Cache), 4);
    QCOMPARE(compositor.count(Selection), 0);

    compositor.setFlags(C::Default, 2, 2, C::DefaultFlag, &inserts);
    QCOMPARE(inserts.count(), 0);
    QCOMPARE(compositor.count(C::Default), 12);
    QCOMPARE(compositor.count(Visible), 0);
    QCOMPARE(compositor.count(C::Cache), 4);
    QCOMPARE(compositor.count(Selection), 0);

    compositor.setFlags(C::Default, 2, 2, VisibleFlag, &inserts);
    QCOMPARE(inserts.count(), 1);
    QCOMPARE(inserts.at(0).index[C::Default], 2);
    QCOMPARE(inserts.at(0).index[Visible], 0);
    QCOMPARE(inserts.at(0).index[C::Cache], 0);
    QCOMPARE(inserts.at(0).index[Selection], 0);
    QCOMPARE(inserts.at(0).flags, int(VisibleFlag));
    QCOMPARE(compositor.find(C::Default, 1)->flags, int(C::DefaultFlag));
    QCOMPARE(compositor.find(C::Default, 2)->flags, C::DefaultFlag | VisibleFlag);
    QCOMPARE(compositor.find(C::Default, 3)->flags, C::DefaultFlag | VisibleFlag);
    QCOMPARE(compositor.count(C::Default), 12);
    QCOMPARE(compositor.count(Visible), 2);
    QCOMPARE(compositor.count(C::Cache), 4);
    QCOMPARE(compositor.count(Selection), 0);

    inserts.clear();
    compositor.setFlags(C::Default, 6, 2, VisibleFlag);
    compositor.setFlags(Visible, 1, 2, SelectionFlag | C::CacheFlag, &inserts);
    QCOMPARE(inserts.count(), 2);
    QCOMPARE(inserts.at(0).index[C::Default], 3);
    QCOMPARE(inserts.at(0).index[Visible], 1);
    QCOMPARE(inserts.at(0).index[C::Cache], 0);
    QCOMPARE(inserts.at(0).index[Selection], 0);
    QCOMPARE(inserts.at(0).flags, SelectionFlag | C::CacheFlag);
    QCOMPARE(inserts.at(1).index[C::Default], 6);
    QCOMPARE(inserts.at(1).index[Visible], 2);
    QCOMPARE(inserts.at(1).index[C::Cache], 1);
    QCOMPARE(inserts.at(1).index[Selection], 1);
    QCOMPARE(inserts.at(1).flags, SelectionFlag | C::CacheFlag);
    QCOMPARE(compositor.find(C::Default, 3)->flags, C::DefaultFlag | VisibleFlag| SelectionFlag | C::CacheFlag);
    QCOMPARE(compositor.find(C::Default, 4)->flags, int(C::DefaultFlag));
    QCOMPARE(compositor.find(C::Default, 5)->flags, int(C::DefaultFlag));
    QCOMPARE(compositor.find(C::Default, 6)->flags, C::DefaultFlag | VisibleFlag | SelectionFlag | C::CacheFlag);
    QCOMPARE(compositor.count(C::Default), 12);
    QCOMPARE(compositor.count(Visible), 4);
    QCOMPARE(compositor.count(C::Cache), 6);
    QCOMPARE(compositor.count(Selection), 2);

    inserts.clear();
    compositor.setFlags(C::Cache, 3, 1, VisibleFlag, &inserts);
    QCOMPARE(inserts.count(), 1);
    QCOMPARE(inserts.at(0).index[C::Default], 12);
    QCOMPARE(inserts.at(0).index[Visible], 4);
    QCOMPARE(inserts.at(0).index[C::Cache], 3);
    QCOMPARE(inserts.at(0).index[Selection], 2);
    QCOMPARE(inserts.at(0).flags, VisibleFlag | C::CacheFlag);
    QCOMPARE(compositor.find(C::Cache, 3)->flags, VisibleFlag | C::CacheFlag);
    QCOMPARE(compositor.count(C::Default), 12);
    QCOMPARE(compositor.count(Visible), 5);
    QCOMPARE(compositor.count(C::Cache), 6);
    QCOMPARE(compositor.count(Selection), 2);
}

void tst_qdeclarativelistcompositor::move()
{
    QDeclarativeListCompositor compositor;
    compositor.setGroupCount(4);
    compositor.setDefaultGroups(VisibleFlag | C::DefaultFlag);
    C::iterator it;

    int listA; const int *a = &listA;
    int listB; const int *b = &listB;
    int listC; const int *c = &listC;

    compositor.append(&listA, 0, 6, C::DefaultFlag);
    compositor.append(&listB, 0, 6, C::AppendFlag | C::PrependFlag | C::DefaultFlag);
    compositor.append(0, 0, 4, C::CacheFlag | C::DefaultFlag);
    compositor.append(&listC, 0, 6, C::AppendFlag | C::PrependFlag | C::DefaultFlag);
    compositor.setFlags(C::Default, 18, 2, C::CacheFlag);
    QCOMPARE(compositor.count(C::Default), 22);

    {   const int indexes[] = {0,1,2,3,4,5,0,1,2,3,4,5,0,1,2,3,0,1,2,3,4,5};
        const int *lists[]  = {a,a,a,a,a,a,b,b,b,b,b,b,0,0,0,0,c,c,c,c,c,c};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    }

    compositor.move(C::Default, 15, C::Default, 0, 1);
    QCOMPARE(compositor.count(C::Default), 22);
    {   const int indexes[] = {0,0,1,2,3,4,5,0,1,2,3,4,5,1,2,3,0,1,2,3,4,5};
        const int *lists[]  = {0,a,a,a,a,a,a,b,b,b,b,b,b,0,0,0,c,c,c,c,c,c};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    }

    compositor.move(C::Default, 15, C::Default, 1, 1);
    QCOMPARE(compositor.count(C::Default), 22);
    {   const int indexes[] = {0,1,0,1,2,3,4,5,0,1,2,3,4,5,2,3,0,1,2,3,4,5};
        const int *lists[]  = {0,0,a,a,a,a,a,a,b,b,b,b,b,b,0,0,c,c,c,c,c,c};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    }

    compositor.move(C::Default, 0, C::Default, 3, 2);
    QCOMPARE(compositor.count(C::Default), 22);
    {   const int indexes[] = {0,1,2,0,1,3,4,5,0,1,2,3,4,5,2,3,0,1,2,3,4,5};
        const int *lists[]  = {a,a,a,0,0,a,a,a,b,b,b,b,b,b,0,0,c,c,c,c,c,c};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    }

    compositor.move(C::Default, 7, C::Default, 1, 10);
    QCOMPARE(compositor.count(C::Default), 22);
    {   const int indexes[] = {0,5,0,1,2,3,4,5,0,1,0,1,2,2,3,3,4,1,2,3,4,5};
        const int *lists[]  = {a,a,b,b,b,b,b,b,0,0,c,a,a,0,0,a,a,c,c,c,c,c};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    }

    compositor.move(C::Default, 17, C::Default, 20, 2);
    QCOMPARE(compositor.count(C::Default), 22);
    {   const int indexes[] = {0,5,0,1,2,3,4,5,0,1,0,1,2,2,3,3,4,3,4,5,1,2};
        const int *lists[]  = {a,a,b,b,b,b,b,b,0,0,c,a,a,0,0,a,a,c,c,c,c,c};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            it = compositor.find(C::Default, i);
            QCOMPARE(it.list<int>(), lists[i]);
            if (lists[i]) QCOMPARE(it.modelIndex(), indexes[i]);
        }
    }
}

void tst_qdeclarativelistcompositor::clear()
{
    QDeclarativeListCompositor compositor;
    compositor.setGroupCount(4);
    compositor.setDefaultGroups(VisibleFlag | C::DefaultFlag);

    int listA;
    int listB;

    compositor.append(&listA, 0, 8, C::AppendFlag | C::PrependFlag | VisibleFlag | C::DefaultFlag);
    compositor.append(&listB, 4, 5,  VisibleFlag | C::DefaultFlag);
    compositor.append(0, 0, 3,  VisibleFlag | C::DefaultFlag | C::CacheFlag);

    QCOMPARE(compositor.count(C::Default), 16);
    QCOMPARE(compositor.count(Visible), 16);
    QCOMPARE(compositor.count(C::Cache), 3);

    compositor.clear();
    QCOMPARE(compositor.count(C::Default), 0);
    QCOMPARE(compositor.count(Visible), 0);
    QCOMPARE(compositor.count(C::Cache), 0);
}

void tst_qdeclarativelistcompositor::listItemsInserted()
{
    QDeclarativeListCompositor compositor;
    compositor.setGroupCount(4);
    compositor.setDefaultGroups(VisibleFlag | C::DefaultFlag);

    int listA;
    int listB;
    QVector<C::Insert> inserts;

    compositor.append(&listA, 0, 7, C::AppendFlag | C::PrependFlag | C::DefaultFlag);
    compositor.append(&listB, 0, 4, C::DefaultFlag);
    compositor.move(C::Default, 2, C::Default, 8, 3);
    QCOMPARE(compositor.count(C::Default), 11);
    {   const int indexes[] = {/*A*/0,1,5,6,/*B*/0,1,2,3,/*A*/2,3,4};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }

    compositor.listItemsInserted(&listA, 10, 2, &inserts);
    QCOMPARE(compositor.count(C::Default), 11);
    QCOMPARE(inserts.count(), 0);
    {   const int indexes[] = {/*A*/0,1,5,6,/*B*/0,1,2,3,/*A*/2,3,4};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }

    compositor.listItemsInserted(&listB, 10, 2, &inserts);
    QCOMPARE(compositor.count(C::Default), 11);
    QCOMPARE(inserts.count(), 0);
    {   const int indexes[] = {/*A*/0,1,5,6,/*B*/0,1,2,3,/*A*/2,3,4};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }

    compositor.listItemsInserted(&listA, 0, 2, &inserts);
    QCOMPARE(compositor.count(C::Default), 13);
    QCOMPARE(inserts.count(), 1);
    QCOMPARE(inserts.at(0).index[C::Default], 0); QCOMPARE(inserts.at(0).count, 2);
    {   const int indexes[] = {/*A*/0,1,2,3,7,8,/*B*/0,1,2,3,/*A*/4,5,6};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }

    inserts.clear();
    compositor.listItemsInserted(&listA, 5, 1, &inserts);
    QCOMPARE(compositor.count(C::Default), 14);
    QCOMPARE(inserts.count(), 1);
    QCOMPARE(inserts.at(0).index[C::Default], 4); QCOMPARE(inserts.at(0).count, 1);
    {   const int indexes[] = {/*A*/0,1,2,3,5,8,9,/*B*/0,1,2,3,/*A*/4,6,7};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }

    inserts.clear();
    compositor.listItemsInserted(&listA, 10, 2, &inserts);
    QCOMPARE(compositor.count(C::Default), 16);
    QCOMPARE(inserts.count(), 1);
    QCOMPARE(inserts.at(0).index[C::Default], 7); QCOMPARE(inserts.at(0).count, 2);
    {   const int indexes[] = {/*A*/0,1,2,3,5,8,9,10,11,/*B*/0,1,2,3,/*A*/4,6,7};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }
}

void tst_qdeclarativelistcompositor::listItemsRemoved()
{
    QDeclarativeListCompositor compositor;
    compositor.setGroupCount(4);
    compositor.setDefaultGroups(VisibleFlag | C::DefaultFlag);

    int listA;
    int listB;
    QVector<C::Remove> removes;

    compositor.append(&listA, 0, 7, C::AppendFlag | C::PrependFlag | C::DefaultFlag);
    compositor.append(&listB, 0, 4, C::DefaultFlag);
    compositor.move(C::Default, 2, C::Default, 8, 3);

    QCOMPARE(compositor.count(C::Default), 11);
    {   const int indexes[] = {/*A*/0,1,5,6,/*B*/0,1,2,3,/*A*/2,3,4};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }

    compositor.listItemsRemoved(&listA, 12, 2, &removes);
    QCOMPARE(compositor.count(C::Default), 11);
    QCOMPARE(removes.count(), 0);

    compositor.listItemsRemoved(&listB, 12, 2, &removes);
    QCOMPARE(compositor.count(C::Default), 11);
    QCOMPARE(removes.count(), 0);

    compositor.listItemsRemoved(&listA, 4, 3, &removes);
    QCOMPARE(compositor.count(C::Default), 8);
    QCOMPARE(removes.count(), 2);
    QCOMPARE(removes.at(0).index[C::Default], 2); QCOMPARE(removes.at(0).count, 2);
    QCOMPARE(removes.at(1).index[C::Default], 8); QCOMPARE(removes.at(1).count, 1);
    {   const int indexes[] = {/*A*/0,1,/*B*/0,1,2,3,/*A*/2,3};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }
}

void tst_qdeclarativelistcompositor::listItemsMoved()
{
    QDeclarativeListCompositor compositor;
    compositor.setGroupCount(4);
    compositor.setDefaultGroups(VisibleFlag | C::DefaultFlag);

    int listA;
    int listB;
    QVector<C::Remove> removes;
    QVector<C::Insert> inserts;

    compositor.append(&listA, 0, 7, C::AppendFlag | C::PrependFlag | C::DefaultFlag);
    {   const int indexes[] = {/*A*/0,1,2,3,4,5,6};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }

    removes.clear();
    inserts.clear();
    compositor.listItemsMoved(&listA, 4, 3, 1, &removes, &inserts);
    {   const int indexes[] = {/*A*/0,1,2,3,4,5,6};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
        const int from[]  = {4, 1};
        const int to[]    = {3, 1};

        QCOMPARE(removes.count(), lengthOf(from) / 2);
        for (int i = 0; i < lengthOf(from); i += 2) {
            QCOMPARE(removes.at(i).index[C::Default], from[i]);
            QCOMPARE(removes.at(i).count, from[i + 1]);
        }
        QCOMPARE(inserts.count(), lengthOf(to) / 2);
        for (int i = 0; i < lengthOf(to); i += 2) {
            QCOMPARE(inserts.at(i).index[C::Default], to[i]);
            QCOMPARE(inserts.at(i).count, to[i + 1]);
        }
    }

    compositor.append(&listB, 0, 4, C::DefaultFlag);
    compositor.move(C::Default, 2, C::Default, 8, 3);
    QCOMPARE(compositor.count(C::Default), 11);
    {   const int indexes[] = {/*A*/0,1,5,6,/*B*/0,1,2,3,/*A*/2,3,4};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }

    removes.clear();
    inserts.clear();
    compositor.listItemsMoved(&listA, 4, 1, 3, &removes, &inserts);
    {   const int indexes[] = {/*A*/0,2,3,4,/*B*/0,1,2,3,/*A*/5,6,1};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }

    removes.clear();
    inserts.clear();
    compositor.listItemsMoved(&listA, 0, 6, 1, &removes, &inserts);
    {   const int indexes[] = {/*A*/1,2,3,6,/*B*/0,1,2,3,/*A*/4,5,0};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }

    compositor.clear();
    compositor.append(&listA, 0, 8, C::AppendFlag | C::PrependFlag | C::DefaultFlag);
    for (int i = 0; i < 4; ++i)
        compositor.setFlags(C::Default, 0, 4, C::CacheFlag);
    removes.clear();
    inserts.clear();
    compositor.listItemsMoved(&listA, 6, 2, 1, &removes, &inserts);
}

void tst_qdeclarativelistcompositor::listItemsChanged()
{
    QDeclarativeListCompositor compositor;
    compositor.setGroupCount(4);
    compositor.setDefaultGroups(VisibleFlag | C::DefaultFlag);

    int listA;
    int listB;
    QVector<C::Change> changes;

    compositor.append(&listA, 0, 7, C::AppendFlag | C::PrependFlag | C::DefaultFlag);
    compositor.append(&listB, 0, 4, C::DefaultFlag);
    compositor.move(C::Default, 2, C::Default, 8, 3);

    QCOMPARE(compositor.count(C::Default), 11);
    {   const int indexes[] = {/*A*/0,1,5,6,/*B*/0,1,2,3,/*A*/2,3,4};
        for (int i = 0; i < lengthOf(indexes); ++i) {
            QCOMPARE(compositor.find(C::Default, i).modelIndex(), indexes[i]);
        }
    }

    compositor.listItemsChanged(&listA, 3, 4, &changes);
    QCOMPARE(changes.count(), 2);
    QCOMPARE(changes.at(0).index[C::Default], 2); QCOMPARE(changes.at(0).count, 2);
    QCOMPARE(changes.at(1).index[C::Default], 9); QCOMPARE(changes.at(0).count, 2);
}

QTEST_MAIN(tst_qdeclarativelistcompositor)

#include "tst_qdeclarativelistcompositor.moc"

