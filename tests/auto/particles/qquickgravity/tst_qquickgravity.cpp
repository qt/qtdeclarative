// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
#include <private/qabstractanimation_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickgravity : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickgravity() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void test_basic();
};

void tst_qquickgravity::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickgravity::test_basic()
{
    QQuickView* view = createView(testFileUrl("basic.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    float mag = 707.10678f;
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
        if (d->t == -1 || !d->stillAlive(system))
            continue; //Particle data unused or dead

        float t = ((qreal)system->timeInt/1000.0) - d->t;
        QVERIFY(extremelyFuzzyCompare(d->vx, t*mag, 20.0f));
        QVERIFY(extremelyFuzzyCompare(d->vy, t*mag, 20.0f));
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }
    delete view;
}

QTEST_MAIN(tst_qquickgravity);

#include "tst_qquickgravity.moc"
