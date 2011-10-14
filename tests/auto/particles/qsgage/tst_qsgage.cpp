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
#include "../shared/particlestestsshared.h"
#include <private/qsgparticlesystem_p.h>
#include <private/qabstractanimation_p.h>

class tst_qsgage : public QObject
{
    Q_OBJECT
public:
    tst_qsgage();

private slots:
    void test_kill();
    void test_jump();
    void test_onceOff();
    void test_sustained();
};

tst_qsgage::tst_qsgage()
{
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qsgage::test_kill()
{
    QQuickView* view = createView(QCoreApplication::applicationDirPath() + "/data/kill.qml", 600);
    QSGParticleSystem* system = view->rootObject()->findChild<QSGParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QCOMPARE(system->groupData[0]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[0]->data) {
        if (d->t == -1)
            continue; //Particle data unused

        QCOMPARE(d->x, 0.f);
        QCOMPARE(d->y, 0.f);
        QCOMPARE(d->vx, 1000.f);
        QCOMPARE(d->vy, 1000.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(d->t <= ((qreal)system->timeInt/1000.0) - 0.5f + EPSILON);
    }
}

void tst_qsgage::test_jump()
{
    QQuickView* view = createView(QCoreApplication::applicationDirPath() + "/data/jump.qml", 600);
    QSGParticleSystem* system = view->rootObject()->findChild<QSGParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QCOMPARE(system->groupData[0]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[0]->data) {
        if (d->t == -1)
            continue; //Particle data unused

        //Allow for a small variance because jump is trying to simulate off wall time
        extremelyFuzzyCompare(d->x, -100.f, 5.0f);
        extremelyFuzzyCompare(d->y, -100.f, 5.0f);
        QCOMPARE(d->vx, 500.f);
        QCOMPARE(d->vy, 500.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(d->t <= ((qreal)system->timeInt/1000.0) - 0.4f + EPSILON);
    }
}

void tst_qsgage::test_onceOff()
{
    QQuickView* view = createView(QCoreApplication::applicationDirPath() + "/data/onceoff.qml", 600);
    QSGParticleSystem* system = view->rootObject()->findChild<QSGParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QCOMPARE(system->groupData[0]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[0]->data) {
        if (d->t == -1)
            continue; //Particle data unused

        QCOMPARE(d->x, 0.f);
        QCOMPARE(d->y, 0.f);
        QCOMPARE(d->vx, 500.f);
        QCOMPARE(d->vy, 500.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(d->t <= ((qreal)system->timeInt/1000.0) - 0.4f + EPSILON);
    }
}

void tst_qsgage::test_sustained()
{
    QQuickView* view = createView(QCoreApplication::applicationDirPath() + "/data/sustained.qml", 600);
    QSGParticleSystem* system = view->rootObject()->findChild<QSGParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);
    //TODO: Ensure some particles have lived to 0.4s point despite unified timer

    QCOMPARE(system->groupData[0]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[0]->data) {
        if (d->t == -1)
            continue; //Particle data unused

        QCOMPARE(d->x, 0.f);
        QCOMPARE(d->y, 0.f);
        QCOMPARE(d->vx, 500.f);
        QCOMPARE(d->vy, 500.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyCompare(d->t, ((qreal)system->timeInt/1000.0) - 0.4f));
    }
}

QTEST_MAIN(tst_qsgage);

#include "tst_qsgage.moc"
