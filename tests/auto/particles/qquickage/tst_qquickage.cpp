// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
#include <private/qabstractanimation_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickage : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickage() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;

    void test_kill();
    void test_jump();
    void test_onceOff();
    void test_sustained();
};

void tst_qquickage::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickage::test_kill()
{
    QQuickView* view = createView(testFileUrl("kill.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
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
    delete view;
}

void tst_qquickage::test_jump()
{
    QQuickView* view = createView(testFileUrl("jump.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
        if (d->t == -1)
            continue; //Particle data unused

        //Allow for variance because jump is trying to simulate off wall time and things have emitted 'continuously' before first affect
        QVERIFY(d->x <= -50.f);
        QVERIFY(d->y <= -50.f);
        QCOMPARE(d->vx, 500.f);
        QCOMPARE(d->vy, 500.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(d->t <= ((qreal)system->timeInt/1000.0) - 0.4f + EPSILON);
    }
    delete view;
}

void tst_qquickage::test_onceOff()
{
    QQuickView* view = createView(testFileUrl("onceoff.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
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
    delete view;
}

void tst_qquickage::test_sustained()
{
    QQuickView* view = createView(testFileUrl("sustained.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);
    //TODO: Ensure some particles have lived to 0.4s point despite unified timer

    //Can't verify size, because particles never die. It will constantly grow.
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
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
    delete view;
}

QTEST_MAIN(tst_qquickage);

#include "tst_qquickage.moc"
