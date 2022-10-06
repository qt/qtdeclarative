// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <qmath.h>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
#include <private/qabstractanimation_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickangleddirection : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickangleddirection() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void test_basic();
};

void tst_qquickangleddirection::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickangleddirection::test_basic()
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
        QVERIFY(qFuzzyCompare(d->vx, 353.55339f));
        QVERIFY(qFuzzyCompare(d->vy, 353.55339f));
        QVERIFY(d->ax > 0.f);
        QVERIFY(d->ax < 500.f);
        QVERIFY(d->ay > 0.f);
        QVERIFY(d->ay < 500.f);
        QVERIFY(qSqrt(d->ax*d->ax + d->ay*d->ay) < 500.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }
    delete view;
}

QTEST_MAIN(tst_qquickangleddirection);

#include "tst_qquickangleddirection.moc"
