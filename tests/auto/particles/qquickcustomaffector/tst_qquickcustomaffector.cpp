// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
#include <private/qabstractanimation_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickcustomaffector : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickcustomaffector() : QQmlDataTest(QT_QMLTEST_DATADIR) {}

private slots:
    void initTestCase() override;
    void test_basic();
    void test_move();
    void test_affectedSignal();
};

void tst_qquickcustomaffector::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickcustomaffector::test_basic()
{
    QQuickView* view = createView(testFileUrl("basic.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
        if (d->t == -1)
            continue; //Particle data unused
        //in CI the whole simulation often happens at once, so dead particles end up missing out
        if (!d->stillAlive(system))
            continue; //parameters no longer get set once you die

        QCOMPARE(d->x, 100.f);
        QCOMPARE(d->y, 100.f);
        QCOMPARE(d->vx, 100.f);
        QCOMPARE(d->vy, 100.f);
        QCOMPARE(d->ax, 100.f);
        QCOMPARE(d->ay, 100.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 100.f);
        QCOMPARE(d->endSize, 100.f);
        QCOMPARE(d->autoRotate, (uchar)1);
        QCOMPARE(d->color.r, (uchar)0);
        QCOMPARE(d->color.g, (uchar)255);
        QCOMPARE(d->color.b, (uchar)0);
        QCOMPARE(d->color.a, (uchar)0);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }
    delete view;
}

void tst_qquickcustomaffector::test_move()
{
    QQuickView* view = createView(testFileUrl("move.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QVERIFY(extremelyFuzzyCompare(system->groupData[0]->size(), 500, 10));
    for (QQuickParticleData *d : std::as_const(system->groupData[0]->data)) {
        if (d->t == -1)
            continue; //Particle data unused
        if (!d->stillAlive(system))
            continue; //parameters no longer get set once you die

        QVERIFY2(myFuzzyCompare(d->curX(system), 50.0), QByteArray::number(d->curX(system)));
        QVERIFY2(myFuzzyCompare(d->curY(system), 50.0), QByteArray::number(d->curY(system)));
        QVERIFY2(myFuzzyCompare(d->curVX(system), 50.0), QByteArray::number(d->curVX(system)));
        QVERIFY2(myFuzzyCompare(d->curVY(system), 50.0), QByteArray::number(d->curVY(system)));
        QVERIFY2(myFuzzyCompare(d->curAX(), 50.0), QByteArray::number(d->curAX()));
        QVERIFY2(myFuzzyCompare(d->curAY(), 50.0), QByteArray::number(d->curAY()));
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 32.f);
        QCOMPARE(d->endSize, 32.f);
        QVERIFY2(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)),
                 QString::fromLatin1("%1 <= %2 / 1000").arg(d->t).arg(system->timeInt).toUtf8());
    }
    delete view;
}

void tst_qquickcustomaffector::test_affectedSignal()
{
    QQuickView* view = createView(testFileUrl("affectedSignal.qml"), 600);
    QQuickParticleSystem* system = view->rootObject()->findChild<QQuickParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QCOMPARE(system->property("resultX1").toInt(), 0);
    QCOMPARE(system->property("resultY1").toInt(), 100);
    QCOMPARE(system->property("resultX2").toInt(), 1234);
    QCOMPARE(system->property("resultY2").toInt(), 1234);
    delete view;
}

QTEST_MAIN(tst_qquickcustomaffector);

#include "tst_qquickcustomaffector.moc"
