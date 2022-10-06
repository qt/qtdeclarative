// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
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

QTEST_MAIN(tst_qquickparticlesystem);

#include "tst_qquickparticlesystem.moc"
