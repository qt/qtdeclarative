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

#include <QtTest/QtTest>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/private/qdeclarativepath_p.h>

#include "../../../shared/util.h"

#ifdef Q_OS_SYMBIAN
// In Symbian OS test data is located in applications private dir
#define SRCDIR "."
#endif

class tst_QDeclarativePath : public QObject
{
    Q_OBJECT
public:
    tst_QDeclarativePath() {}

private slots:
    void arc();
    void catmullromCurve();
    void svg();
};

void tst_QDeclarativePath::arc()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/arc.qml"));
    QDeclarativePath *obj = qobject_cast<QDeclarativePath*>(c.create());
    QVERIFY(obj != 0);

    QCOMPARE(obj->startX(), 0.);
    QCOMPARE(obj->startY(), 0.);

    QDeclarativeListReference list(obj, "pathElements");
    QCOMPARE(list.count(), 1);

    QDeclarativePathArc* arc = qobject_cast<QDeclarativePathArc*>(list.at(0));
    QVERIFY(arc != 0);
    QCOMPARE(arc->x(), 100.);
    QCOMPARE(arc->y(), 100.);
    QCOMPARE(arc->radiusX(), 100.);
    QCOMPARE(arc->radiusY(), 100.);
    QCOMPARE(arc->useLargeArc(), false);
    QCOMPARE(arc->direction(), QDeclarativePathArc::Clockwise);

    QPainterPath path = obj->path();
    QVERIFY(path != QPainterPath());

    QPointF pos = obj->pointAt(0);
    QCOMPARE(pos, QPointF(0,0));
    pos = obj->pointAt(.25);
    QCOMPARE(pos.toPoint(), QPoint(39,8));  //fuzzy compare
    pos = obj->pointAt(.75);
    QCOMPARE(pos.toPoint(), QPoint(92,61)); //fuzzy compare
    pos = obj->pointAt(1);
    QCOMPARE(pos, QPointF(100,100));
}

void tst_QDeclarativePath::catmullromCurve()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/curve.qml"));
    QDeclarativePath *obj = qobject_cast<QDeclarativePath*>(c.create());
    QVERIFY(obj != 0);

    QCOMPARE(obj->startX(), 0.);
    QCOMPARE(obj->startY(), 0.);

    QDeclarativeListReference list(obj, "pathElements");
    QCOMPARE(list.count(), 3);

    QDeclarativePathCatmullRomCurve* arc = qobject_cast<QDeclarativePathCatmullRomCurve*>(list.at(0));
//    QVERIFY(arc != 0);
//    QCOMPARE(arc->x(), 100.);
//    QCOMPARE(arc->y(), 100.);
//    QCOMPARE(arc->radiusX(), 100.);
//    QCOMPARE(arc->radiusY(), 100.);
//    QCOMPARE(arc->useLargeArc(), false);
//    QCOMPARE(arc->direction(), QDeclarativePathArc::Clockwise);

    QPainterPath path = obj->path();
    QVERIFY(path != QPainterPath());

    QPointF pos = obj->pointAt(0);
    QCOMPARE(pos, QPointF(0,0));
    pos = obj->pointAt(.25);
    QCOMPARE(pos.toPoint(), QPoint(63,26));  //fuzzy compare
    pos = obj->pointAt(.75);
    QCOMPARE(pos.toPoint(), QPoint(51,105)); //fuzzy compare
    pos = obj->pointAt(1);
    QCOMPARE(pos, QPointF(100,150));
}

void tst_QDeclarativePath::svg()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent c(&engine, QUrl::fromLocalFile(SRCDIR "/data/svg.qml"));
    QDeclarativePath *obj = qobject_cast<QDeclarativePath*>(c.create());
    QVERIFY(obj != 0);

    QCOMPARE(obj->startX(), 0.);
    QCOMPARE(obj->startY(), 0.);

    QDeclarativeListReference list(obj, "pathElements");
    QCOMPARE(list.count(), 1);

    QDeclarativePathSvg* svg = qobject_cast<QDeclarativePathSvg*>(list.at(0));
    QVERIFY(svg != 0);
    QCOMPARE(svg->path(), QLatin1String("M200,300 Q400,50 600,300 T1000,300"));

    QPainterPath path = obj->path();
    QVERIFY(path != QPainterPath());

    QPointF pos = obj->pointAt(0);
    QCOMPARE(pos, QPointF(200,300));
    pos = obj->pointAt(.25);
    QCOMPARE(pos.toPoint(), QPoint(400,175));  //fuzzy compare
    pos = obj->pointAt(.75);
    QCOMPARE(pos.toPoint(), QPoint(800,425)); //fuzzy compare
    pos = obj->pointAt(1);
    QCOMPARE(pos, QPointF(1000,300));
}


QTEST_MAIN(tst_QDeclarativePath)

#include "tst_qdeclarativepath.moc"
