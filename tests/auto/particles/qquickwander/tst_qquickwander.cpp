// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
#include <private/qabstractanimation_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickwander : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickwander() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void test_basic();
};

void tst_qquickwander::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickwander::test_basic()
{
    QQuickView* view = createView(testFileUrl("basic.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    //Since Wander is random perturbations, the compromise between stability and actual testing is to hope that one of
    //the 500 was randomly changed from 0.0 in velocity
    bool vxChanged = false;
    bool vyChanged = false;
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
        if (d->t == -1)
            continue; //Particle data unused

        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
        if (d->vx != 0.0f)
            vxChanged = true;
        if (d->vy != 0.0f)
            vyChanged = true;
    }
    delete view;
    QVERIFY(vxChanged);
    QVERIFY(vyChanged);
}

QTEST_MAIN(tst_qquickwander);

#include "tst_qquickwander.moc"
