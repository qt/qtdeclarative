// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
#include <private/qabstractanimation_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickfriction : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickfriction() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void test_basic();
    void test_threshold();
};

void tst_qquickfriction::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickfriction::test_basic()
{
    QQuickView* view = createView(testFileUrl("basic.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    //Default is just slowed a little
    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
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
    for (QQuickParticleData *d : std::as_const(system->groupData[1]->data)) {
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
    QQuickView* view = createView(testFileUrl("threshold.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    //Velocity capped at 50, but it might take a frame or two to get there
    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
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
