// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qmath.h>
#include <QtTest/QtTest>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
#include <private/qabstractanimation_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickellipseextruder : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickellipseextruder() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void test_basic();
private:
    bool inCircle(qreal x, qreal y, qreal r, bool borderOnly=false);
};

void tst_qquickellipseextruder::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

bool tst_qquickellipseextruder::inCircle(qreal x, qreal y, qreal r, bool borderOnly)
{
    x -= r;
    y -= r;
    if (myFuzzyCompare(x,0) && myFuzzyCompare(y,0))
        return !borderOnly;
    qreal mag = qSqrt(x*x + y*y);
    if (borderOnly)
        return myFuzzyCompare(mag, r); //Need myFuzzyCompare for smaller Epsilon than qFuzzyCompare
    else
        return mag - EPSILON < r;
}

void tst_qquickellipseextruder::test_basic()
{
    QQuickView* view = createView(testFileUrl("basic.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    //Filled
    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
        if (d->t == -1)
            continue; //Particle data unused

        QVERIFY(inCircle(d->x, d->y, 160, false));
        QCOMPARE(d->vx, 0.f);
        QCOMPARE(d->vy, 0.f);
        QCOMPARE(d->ax, 0.f);
        QCOMPARE(d->ay, 0.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }

    //Just border
    QCOMPARE(system->groupData[1]->size(), 500);
    for (QQuickParticleData *d : std::as_const(system->groupData[1]->data)) {
        if (d->t == -1)
            continue; //Particle data unused

        QVERIFY(inCircle(d->x, d->y, 160, true));
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
}

QTEST_MAIN(tst_qquickellipseextruder);

#include "tst_qquickellipseextruder.moc"
