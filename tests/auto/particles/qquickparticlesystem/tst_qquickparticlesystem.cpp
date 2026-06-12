// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/QTest>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
#include <private/qquickparticleemitter_p.h>
#include <private/qquickparticlepainter_p.h>
#include <private/qquickparticleaffector_p.h>
#include <private/qabstractanimation_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickparticlesystem : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickparticlesystem() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void test_basic();
    void test_affectorscrash();
    void emitter_setSystemToNewSystem();
    void emitter_setSystemToNull();
    void emitter_setSystemFromNull();
    void emitter_destroyWhileSystemRunning();
    void emitter_setGroupWhileAttached();
    void emitter_reparentToDifferentSystem();
    void painter_setSystemToNewSystem();
    void painter_setSystemToNull();
    void painter_destroyWhileSystemRunning();
    void painter_setGroupsWhileAttached();
    void affector_setSystemToNewSystem();
    void affector_setSystemToNull();
    void affector_destroyWhileSystemRunning();
    void affector_setGroupsWhileAttached();
    void moveAllToDifferentSystem();
    void systemDestroyedWhileComponentsAlive();
    void emitter_setSystemAfterComponentComplete();
};

void tst_qquickparticlesystem::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickparticlesystem::test_basic()
{
    QQuickView* view = createView(testFileUrl("basic.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    int stillAlive = 0;
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
        if (d->t == -1)
            continue; //Particle data unused

        if (d->stillAlive(system))
            stillAlive++;
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
    }
    delete view;
    QVERIFY(extremelyFuzzyCompare(stillAlive, 500, 5));//Small simulation variance is permissible.
}
void tst_qquickparticlesystem::test_affectorscrash()
{
    QScopedPointer<QQuickView> view (createView(testFileUrl("crashaffectors.qml"), 600));

    // This should have crashed by now
}

void tst_qquickparticlesystem::emitter_setSystemToNewSystem()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("emitterswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *systemB = view->rootObject()->findChild<QQuickParticleSystem *>("systemB");
    auto *emitter = view->rootObject()->findChild<QQuickParticleEmitter *>("emitter");
    QVERIFY(systemA);
    QVERIFY(systemB);
    QVERIFY(emitter);

    ensureAnimTime(600, systemA->m_animation);

    // Emitter is initially in systemA
    QCOMPARE(emitter->system(), systemA);

    // Move emitter to systemB — this must not crash
    emitter->setSystem(systemB);
    QCOMPARE(emitter->system(), systemB);

    // Let both systems run a bit more
    ensureAnimTime(1200, systemA->m_animation);
    ensureAnimTime(600, systemB->m_animation);

    // systemB should now have particles since the emitter is there
    const auto withParticles =
            std::find_if(systemB->groupData.cbegin(), systemB->groupData.cend(),
                         [systemB](const QQuickParticleGroupData *gd) {
                             return std::find_if(gd->data.cbegin(), gd->data.cend(),
                                                 [systemB](const QQuickParticleData *d) {
                                                     return d->t != -1 && d->stillAlive(systemB);
                                                 })
                                     != gd->data.cend();
                         });

    QCOMPARE_NE(withParticles, systemB->groupData.cend());
}

void tst_qquickparticlesystem::emitter_setSystemToNull()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("emitterswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *emitter = view->rootObject()->findChild<QQuickParticleEmitter *>("emitter");
    QVERIFY(systemA);
    QVERIFY(emitter);

    ensureAnimTime(600, systemA->m_animation);
    QCOMPARE(emitter->system(), systemA);

    // Set system to null — must not crash
    emitter->setSystem(nullptr);
    QCOMPARE(emitter->system(), nullptr);

    // Let the system continue running without the emitter
    ensureAnimTime(1200, systemA->m_animation);
}

void tst_qquickparticlesystem::emitter_setSystemFromNull()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("emitterswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *systemB = view->rootObject()->findChild<QQuickParticleSystem *>("systemB");
    auto *emitter = view->rootObject()->findChild<QQuickParticleEmitter *>("emitter");
    QVERIFY(systemA);
    QVERIFY(systemB);
    QVERIFY(emitter);

    ensureAnimTime(600, systemA->m_animation);

    // Detach
    emitter->setSystem(nullptr);
    QCOMPARE(emitter->system(), nullptr);

    ensureAnimTime(900, systemA->m_animation);

    // Now attach to systemB from null — must not crash
    emitter->setSystem(systemB);
    QCOMPARE(emitter->system(), systemB);

    ensureAnimTime(600, systemB->m_animation);
}

void tst_qquickparticlesystem::emitter_destroyWhileSystemRunning()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("emitterswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *emitter = view->rootObject()->findChild<QQuickParticleEmitter *>("emitter");
    QVERIFY(systemA);
    QVERIFY(emitter);

    ensureAnimTime(600, systemA->m_animation);

    // Delete the emitter while the system is running — must not crash
    delete emitter;

    // Let system continue
    ensureAnimTime(1200, systemA->m_animation);
}

void tst_qquickparticlesystem::emitter_setGroupWhileAttached()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("changegroup.qml"), 600));
    QVERIFY(view);

    auto *system = view->rootObject()->findChild<QQuickParticleSystem *>("system");
    auto *emitter = view->rootObject()->findChild<QQuickParticleEmitter *>("emitter");
    QVERIFY(system);
    QVERIFY(emitter);

    ensureAnimTime(600, system->m_animation);

    // Change the group while running — must not crash or produce invalid state
    emitter->setGroup(QStringLiteral("groupB"));

    ensureAnimTime(1200, system->m_animation);

    // Change back
    emitter->setGroup(QStringLiteral("groupA"));

    ensureAnimTime(1800, system->m_animation);
}

void tst_qquickparticlesystem::emitter_reparentToDifferentSystem()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("reparentemitter.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *systemB = view->rootObject()->findChild<QQuickParticleSystem *>("systemB");
    auto *emitter = view->rootObject()->findChild<QQuickParticleEmitter *>("emitter");
    auto *containerB = view->rootObject()->findChild<QQuickItem *>("containerB");
    QVERIFY(systemA);
    QVERIFY(systemB);
    QVERIFY(emitter);
    QVERIFY(containerB);

    ensureAnimTime(600, systemA->m_animation);

    // Reparent the emitter to be under systemB's hierarchy — must not crash
    emitter->setParentItem(containerB);

    // The emitter should ideally update its system, but even if it doesn't,
    // it must not crash.
    ensureAnimTime(1200, systemA->m_animation);
    ensureAnimTime(600, systemB->m_animation);
}

void tst_qquickparticlesystem::painter_setSystemToNewSystem()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("painterswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *systemB = view->rootObject()->findChild<QQuickParticleSystem *>("systemB");
    auto *painter = view->rootObject()->findChild<QQuickParticlePainter *>("painter");
    QVERIFY(systemA);
    QVERIFY(systemB);
    QVERIFY(painter);

    ensureAnimTime(600, systemA->m_animation);
    QCOMPARE(painter->system(), systemA);

    // Move painter to systemB — must not crash
    painter->setSystem(systemB);
    QCOMPARE(painter->system(), systemB);

    ensureAnimTime(1200, systemA->m_animation);
    ensureAnimTime(600, systemB->m_animation);
}

void tst_qquickparticlesystem::painter_setSystemToNull()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("painterswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *painter = view->rootObject()->findChild<QQuickParticlePainter *>("painter");
    QVERIFY(systemA);
    QVERIFY(painter);

    ensureAnimTime(600, systemA->m_animation);

    // Set system to null — must not crash
    painter->setSystem(nullptr);
    QCOMPARE(painter->system(), nullptr);

    ensureAnimTime(1200, systemA->m_animation);
}

void tst_qquickparticlesystem::painter_destroyWhileSystemRunning()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("painterswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *painter = view->rootObject()->findChild<QQuickParticlePainter *>("painter");
    QVERIFY(systemA);
    QVERIFY(painter);

    ensureAnimTime(600, systemA->m_animation);

    delete painter;

    ensureAnimTime(1200, systemA->m_animation);
}

void tst_qquickparticlesystem::painter_setGroupsWhileAttached()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("painterswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *painter = view->rootObject()->findChild<QQuickParticlePainter *>("painter");
    QVERIFY(systemA);
    QVERIFY(painter);

    ensureAnimTime(600, systemA->m_animation);

    // Change groups while running — must not crash
    painter->setGroups(QStringList{QStringLiteral("newGroup")});

    ensureAnimTime(1200, systemA->m_animation);

    // Change to empty (default group)
    painter->setGroups(QStringList{});

    ensureAnimTime(1800, systemA->m_animation);
}

void tst_qquickparticlesystem::affector_setSystemToNewSystem()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("affectorswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *systemB = view->rootObject()->findChild<QQuickParticleSystem *>("systemB");
    auto *affector = view->rootObject()->findChild<QQuickParticleAffector *>("affector");
    QVERIFY(systemA);
    QVERIFY(systemB);
    QVERIFY(affector);

    ensureAnimTime(600, systemA->m_animation);
    QCOMPARE(affector->system(), systemA);

    // Move affector to systemB — must not crash
    affector->setSystem(systemB);
    QCOMPARE(affector->system(), systemB);

    ensureAnimTime(1200, systemA->m_animation);
    ensureAnimTime(600, systemB->m_animation);
}

void tst_qquickparticlesystem::affector_setSystemToNull()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("affectorswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *affector = view->rootObject()->findChild<QQuickParticleAffector *>("affector");
    QVERIFY(systemA);
    QVERIFY(affector);

    ensureAnimTime(600, systemA->m_animation);

    affector->setSystem(nullptr);
    QCOMPARE(affector->system(), nullptr);

    ensureAnimTime(1200, systemA->m_animation);
}

void tst_qquickparticlesystem::affector_destroyWhileSystemRunning()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("affectorswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *affector = view->rootObject()->findChild<QQuickParticleAffector *>("affector");
    QVERIFY(systemA);
    QVERIFY(affector);

    ensureAnimTime(600, systemA->m_animation);

    delete affector;

    ensureAnimTime(1200, systemA->m_animation);
}

void tst_qquickparticlesystem::affector_setGroupsWhileAttached()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("affectorswitch.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *affector = view->rootObject()->findChild<QQuickParticleAffector *>("affector");
    QVERIFY(systemA);
    QVERIFY(affector);

    ensureAnimTime(600, systemA->m_animation);

    // Change groups while running — must not crash
    affector->setGroups(QStringList{QStringLiteral("newGroup")});

    ensureAnimTime(1200, systemA->m_animation);

    // Change to empty (all groups)
    affector->setGroups(QStringList{});

    ensureAnimTime(1800, systemA->m_animation);
}

void tst_qquickparticlesystem::moveAllToDifferentSystem()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("moveall.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *systemB = view->rootObject()->findChild<QQuickParticleSystem *>("systemB");
    auto *emitter = view->rootObject()->findChild<QQuickParticleEmitter *>("emitter");
    auto *painter = view->rootObject()->findChild<QQuickParticlePainter *>("painter");
    auto *affector = view->rootObject()->findChild<QQuickParticleAffector *>("affector");
    QVERIFY(systemA);
    QVERIFY(systemB);
    QVERIFY(emitter);
    QVERIFY(painter);
    QVERIFY(affector);

    ensureAnimTime(600, systemA->m_animation);

    // Move all components to systemB simultaneously — must not crash
    emitter->setSystem(systemB);
    painter->setSystem(systemB);
    affector->setSystem(systemB);

    QCOMPARE(emitter->system(), systemB);
    QCOMPARE(painter->system(), systemB);
    QCOMPARE(affector->system(), systemB);

    ensureAnimTime(1200, systemA->m_animation);
    ensureAnimTime(600, systemB->m_animation);
}

void tst_qquickparticlesystem::systemDestroyedWhileComponentsAlive()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("systemdestroy.qml"), 600));
    QVERIFY(view);

    auto *systemA = view->rootObject()->findChild<QQuickParticleSystem *>("systemA");
    auto *emitter = view->rootObject()->findChild<QQuickParticleEmitter *>("emitter");
    auto *painter = view->rootObject()->findChild<QQuickParticlePainter *>("painter");
    auto *affector = view->rootObject()->findChild<QQuickParticleAffector *>("affector");
    QVERIFY(systemA);
    QVERIFY(emitter);
    QVERIFY(painter);
    QVERIFY(affector);

    ensureAnimTime(600, systemA->m_animation);

    // Delete the system while components still reference it — must not crash.
    // After the fix, components should have their system pointer nulled out.
    // For now we just verify the deletion doesn't crash.
    delete systemA;
}

void tst_qquickparticlesystem::emitter_setSystemAfterComponentComplete()
{
    QScopedPointer<QQuickView> view(createView(testFileUrl("emitterlateattach.qml"), 0));
    QVERIFY(view);

    auto *system = view->rootObject()->findChild<QQuickParticleSystem *>("system");
    auto *emitter = view->rootObject()->findChild<QQuickParticleEmitter *>("emitter");
    QVERIFY(system);
    QVERIFY(emitter);

    // Component is already complete; emitter has no system yet
    QCOMPARE(emitter->system(), nullptr);
    QCOMPARE(system->count(), 0);

    // Assigning a system after componentComplete must call finishRegisteringParticleEmitter().
    // That calls emitterAdded(), which updates system->count() synchronously. Without it,
    // the emitter lands in m_emitters but particleCount is never updated, leaving
    // system->count() at 0 even as auto-allocation silently emits particles.
    emitter->setSystem(system);
    QCOMPARE(emitter->system(), system);
    QCOMPARE(system->count(), emitter->particleCount());
}

QTEST_MAIN(tst_qquickparticlesystem);

#include "tst_qquickparticlesystem.moc"
