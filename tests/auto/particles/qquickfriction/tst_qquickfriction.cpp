/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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
#include <private/qquickparticlesystem_p.h>
#include <private/qabstractanimation_p.h>

class tst_qquickfriction : public QObject
{
    Q_OBJECT
public:
    tst_qquickfriction();

private slots:
    void test_basic();
    void test_threshold();
};

tst_qquickfriction::tst_qquickfriction()
{
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickfriction::test_basic()
{
    QQuickView* view = createView(QCoreApplication::applicationDirPath() + "/data/basic.qml", 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    //Default is just slowed a little
    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    foreach (QQuickParticleData *d, system->groupData[0]->data) {
        if (d->t == -1)
            continue; //Particle data unused

        QVERIFY(d->vx < 100.f);
        QCOMPARE(d->y, 0.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }

    //Nondefault comes to a complete stop within the first half of its life
    QCOMPARE(system->groupData[1]->size(), 500);
    foreach (QQuickParticleData *d, system->groupData[1]->data) {
        if (d->t == -1)
            continue; //Particle data unused

        if (d->t > ((qreal)system->timeInt/1000.0) - 0.25)
            continue;
        QVERIFY(myFuzzyCompare(d->vx, 0.f));
        QCOMPARE(d->y, 200.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }
    delete view;
}

void tst_qquickfriction::test_threshold()
{
    QQuickView* view = createView(QCoreApplication::applicationDirPath() + "/data/threshold.qml", 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    //Speed capped at 50, but it might take a frame or two to get there
    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    foreach (QQuickParticleData *d, system->groupData[0]->data) {
        if (d->t == -1.0f)
            continue; //Particle data unused
        if (myFuzzyGEQ(d->t, ((qreal)system->timeInt/1000.0) - 0.1))
            continue; //Particle data too young

        QVERIFY(myFuzzyLEQ(d->vx, 50.f));
        QCOMPARE(d->y, 0.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }
    delete view;
}

QTEST_MAIN(tst_qquickfriction);

#include "tst_qquickfriction.moc"
