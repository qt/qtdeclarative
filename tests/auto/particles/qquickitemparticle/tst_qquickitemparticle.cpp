// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
#include <private/qquickparticleemitter_p.h>
#include <private/qquickimage_p.h>
#include <private/qabstractanimation_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickitemparticle : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickitemparticle() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void test_basic();
    void test_deletion();
    void test_noDeletion();
    void test_takeGive();
    void test_noCrashOnReset();
    void test_noLeakWhenDeleted();
};

void tst_qquickitemparticle::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickitemparticle::test_basic()
{
    QQuickView* view = createView(testFileUrl("basic.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
        if (d->t == -1)
            continue; //Particle data unused

        QCOMPARE(d->x, 0.f);
        QCOMPARE(d->y, 0.f);
        QCOMPARE(d->vx, 0.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
        if (d->t > ((qreal)system->timeInt/1000.0) - 0.05)//Delegates appear between frames, may miss the first couple
            continue;
        if (d->t < ((qreal)system->timeInt/1000.0) - 0.45)//Delegates cleared on death
            continue;
        QVERIFY(d->delegate);
        QVERIFY(qobject_cast<QQuickImage*>(d->delegate));
    }
    delete view;
}

void tst_qquickitemparticle::test_deletion()
{
    QQuickView* view = createView(testFileUrl("managed.qml"), 500);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(500, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 100, 10));
    //qDebug() << system->property("acc").toInt(); Seems to be around +15 due to the one frame delay in cleanup compared to creation
    QVERIFY(extremelyFuzzyCompare(system->property("acc").toInt(), 100, 20));
    delete view;
}

void tst_qquickitemparticle::test_noDeletion()
{
    QQuickView* view = createView(testFileUrl("unmanaged.qml"), 500);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(500, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 100, 10));
    QVERIFY(extremelyFuzzyCompare(system->property("acc").toInt(), 100, 10));
    delete view;
}

void tst_qquickitemparticle::test_takeGive()
{
    QQuickView* view = createView(testFileUrl("takeGive.qml"), 500);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    QQuickParticleEmitter* emitter = view->rootObject()->findChild<QQuickParticleEmitter*>("emitter");
    QMetaObject::invokeMethod(view->rootObject(), "takeItems");
    emitter->burst(100);
    ensureAnimTime(1000, system->m_animation);
    QVERIFY(system->property("acc").toInt() == 100);
    QMetaObject::invokeMethod(view->rootObject(), "giveItems");
    QTRY_VERIFY(system->property("acc").toInt() == 0);
    QTRY_VERIFY(system->isEmpty() == true);
    delete view;
}

void tst_qquickitemparticle::test_noCrashOnReset()
{
    QQuickView* view = createView(testFileUrl("basic.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");

    for (int i = 0; i < 10; ++i) {
        ensureAnimTime(16, system->m_animation);
        system->reset();
    }

    delete view;
}

void tst_qquickitemparticle::test_noLeakWhenDeleted()
{
    QQuickView* view = createView(testFileUrl("loader.qml"), 500);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(100, system->m_animation);

    auto particles = std::as_const(system->groupData[0]->data);
    QVERIFY(!particles.isEmpty());

    QQuickParticleData* firstParticleData = particles.first();
    QPointer<QQuickItem> firstParticleDelegate = firstParticleData->delegate;
    QVERIFY(!firstParticleDelegate.isNull());

    QQuickItem* loader = view->rootObject()->findChild<QQuickItem*>("loader");
    loader->setProperty("active", false); //This should destroy the ParticleSystem, ItemParticle and Emitter

    QTest::qWait(1); //Process events to make sure the loader is properly unloaded
    QVERIFY(firstParticleDelegate.isNull()); //Delegates should be deleted

    delete view;
}

QTEST_MAIN(tst_qquickitemparticle);

#include "tst_qquickitemparticle.moc"
