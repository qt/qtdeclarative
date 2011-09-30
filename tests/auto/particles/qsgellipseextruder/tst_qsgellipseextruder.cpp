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

#include <qmath.h>
#include <QtTest/QtTest>
#include "../shared/particlestestsshared.h"
#include <private/qsgparticlesystem_p.h>

class tst_qsgellipseextruder : public QObject
{
    Q_OBJECT
public:
    tst_qsgellipseextruder();

private slots:
    void test_basic();
private:
    bool inCircle(qreal x, qreal y, qreal r, bool borderOnly=false);
};

tst_qsgellipseextruder::tst_qsgellipseextruder()
{
}

bool tst_qsgellipseextruder::inCircle(qreal x, qreal y, qreal r, bool borderOnly)
{
    x -= r;
    y -= r;
    if (myFuzzyCompare(x,0) && myFuzzyCompare(y,0))
        return !borderOnly;
    qreal mag = qSqrt(x*x + y*y);
    if (borderOnly)
        return myFuzzyCompare(mag, r); //Need myFuzzyCompare for smaller Epsilon than qFuzzyCompare
    else
        return mag - EPSILON < r;
}

void tst_qsgellipseextruder::test_basic()
{
    QSGView* view = createView(SRCDIR "/data/basic.qml", 600);
    QSGParticleSystem* system = view->rootObject()->findChild<QSGParticleSystem*>("system");

    //Filled
    QCOMPARE(system->groupData[0]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[0]->data) {
        QVERIFY(inCircle(d->x, d->y, 160, false));
        QCOMPARE(d->vx, 0.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(d->t <= ((qreal)system->timeInt/1000.0));
    }
    //Just border
    QCOMPARE(system->groupData[1]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[1]->data) {
        QVERIFY(inCircle(d->x, d->y, 160, true));
        QCOMPARE(d->vx, 0.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(d->t <= ((qreal)system->timeInt/1000.0));
    }
}

QTEST_MAIN(tst_qsgellipseextruder);

#include "tst_qsgellipseextruder.moc"
