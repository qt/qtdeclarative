// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtCore/QtMath>
#include "../shared/particlestestsshared.h"
#include <private/qquickparticlesystem_p.h>
#include <private/qabstractanimation_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>

class tst_qquickimageparticle : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickimageparticle() : QQmlDataTest(QT_QMLTEST_DATADIR) {}
    ~tst_qquickimageparticle();

private slots:
    void initTestCase() override;
    void test_basic();
    void test_colored();
    void test_colorVariance();
    void test_deformed();
    void test_tabled();
    void test_sprite();
};

void tst_qquickimageparticle::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
    //QQuickImageParticle has several debug statements, with possible pointer dereferences
    qputenv("QML_PARTICLES_DEBUG","please");
}

tst_qquickimageparticle::~tst_qquickimageparticle()
{
    qputenv("QML_PARTICLES_DEBUG","");
}

void tst_qquickimageparticle::test_basic()
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
        QCOMPARE(d->color.r, (uchar)255);
        QCOMPARE(d->color.g, (uchar)255);
        QCOMPARE(d->color.b, (uchar)255);
        QCOMPARE(d->color.a, (uchar)255);
        QCOMPARE(d->xx, 1.0f);
        QCOMPARE(d->xy, 0.0f);
        QCOMPARE(d->yy, 1.0f);
        QCOMPARE(d->yx, 0.0f);
        QCOMPARE(d->rotation, 0.0f);
        QCOMPARE(d->rotationVelocity, 0.0f);
        QCOMPARE(d->autoRotate, (uchar)0);
        QCOMPARE(d->animX, 0.0f);
        QCOMPARE(d->animY, 0.0f);
        QCOMPARE(d->animWidth, 1.0f);
        QCOMPARE(d->animHeight, 1.0f);
        QCOMPARE(d->frameDuration, 1.0f);
        QCOMPARE(d->frameCount, 1.0f);
        QCOMPARE(d->animT, -1.0f);
    }
    delete view;
}


void tst_qquickimageparticle::test_colored()
{
    QQuickView* view = createView(testFileUrl("colored.qml"), 600);
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
        QCOMPARE(d->color.r, (uchar)003);
        QCOMPARE(d->color.g, (uchar)002);
        QCOMPARE(d->color.b, (uchar)001);
        QCOMPARE(d->color.a, (uchar)127);
        QCOMPARE(d->xx, 1.0f);
        QCOMPARE(d->xy, 0.0f);
        QCOMPARE(d->yy, 1.0f);
        QCOMPARE(d->yx, 0.0f);
        QCOMPARE(d->rotation, 0.0f);
        QCOMPARE(d->rotationVelocity, 0.0f);
        QCOMPARE(d->autoRotate, (uchar)0);
        QCOMPARE(d->animX, 0.0f);
        QCOMPARE(d->animY, 0.0f);
        QCOMPARE(d->animWidth, 1.0f);
        QCOMPARE(d->animHeight, 1.0f);
        QCOMPARE(d->frameDuration, 1.0f);
        QCOMPARE(d->frameCount, 1.0f);
        QCOMPARE(d->animT, -1.0f);
    }
    delete view;
}


void tst_qquickimageparticle::test_colorVariance()
{
    QQuickView* view = createView(testFileUrl("colorVariance.qml"), 600);
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
        QVERIFY(d->color.r < 128);
        QVERIFY(d->color.g < 64);
        QVERIFY(d->color.b < 32);
        QVERIFY(d->color.a >= 64);
        QVERIFY(d->color.a < 192);
        QCOMPARE(d->xx, 1.0f);
        QCOMPARE(d->xy, 0.0f);
        QCOMPARE(d->yy, 1.0f);
        QCOMPARE(d->yx, 0.0f);
        QCOMPARE(d->rotation, 0.0f);
        QCOMPARE(d->rotationVelocity, 0.0f);
        QCOMPARE(d->autoRotate, (uchar)0);
        QCOMPARE(d->animX, 0.0f);
        QCOMPARE(d->animY, 0.0f);
        QCOMPARE(d->animWidth, 1.0f);
        QCOMPARE(d->animHeight, 1.0f);
        QCOMPARE(d->frameDuration, 1.0f);
        QCOMPARE(d->frameCount, 1.0f);
        QCOMPARE(d->animT, -1.0f);
    }
    delete view;
}


void tst_qquickimageparticle::test_deformed()
{
    QQuickView* view = createView(testFileUrl("deformed.qml"), 600);
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
        QCOMPARE(d->color.r, (uchar)255);
        QCOMPARE(d->color.g, (uchar)255);
        QCOMPARE(d->color.b, (uchar)255);
        QCOMPARE(d->color.a, (uchar)255);
        QCOMPARE(d->xx, 0.5f);
        QCOMPARE(d->xy, 0.5f);
        QCOMPARE(d->yy, 0.5f);
        QCOMPARE(d->yx, 0.5f);
        QCOMPARE(d->rotation, qDegreesToRadians(90.0f));
        QCOMPARE(d->rotationVelocity, qDegreesToRadians(90.0f));
        QCOMPARE(d->autoRotate, (uchar)1);
        QCOMPARE(d->animX, 0.0f);
        QCOMPARE(d->animY, 0.0f);
        QCOMPARE(d->animWidth, 1.0f);
        QCOMPARE(d->animHeight, 1.0f);
        QCOMPARE(d->frameDuration, 1.0f);
        QCOMPARE(d->frameCount, 1.0f);
        QCOMPARE(d->animT, -1.0f);
    }
    delete view;
}


void tst_qquickimageparticle::test_tabled()
{
    #if defined(Q_OS_LINUX)
        QSKIP("Crashing on Ubuntu 22.04: QTBUG-107707");
    #else
        QQuickView* view = createView(testFileUrl("tabled.qml"), 600);
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
            QCOMPARE(d->color.r, (uchar)255);
            QCOMPARE(d->color.g, (uchar)255);
            QCOMPARE(d->color.b, (uchar)255);
            QCOMPARE(d->color.a, (uchar)255);
            QCOMPARE(d->xx, 1.0f);
            QCOMPARE(d->xy, 0.0f);
            QCOMPARE(d->yy, 1.0f);
            QCOMPARE(d->yx, 0.0f);
            QCOMPARE(d->rotation, 0.0f);
            QCOMPARE(d->rotationVelocity, 0.0f);
            QCOMPARE(d->autoRotate, (uchar)0);
            QCOMPARE(d->animX, 0.0f);
            QCOMPARE(d->animY, 0.0f);
            QCOMPARE(d->animWidth, 1.0f);
            QCOMPARE(d->animHeight, 1.0f);
            QCOMPARE(d->frameDuration, 1.0f);
            QCOMPARE(d->frameCount, 1.0f);
            QCOMPARE(d->animT, -1.0f);
            //TODO: This performance level doesn't alter particleData, but goes straight to shaders. Find something to test
        }
        delete view;
    #endif
}


void tst_qquickimageparticle::test_sprite()
{
    QQuickView* view = createView(testFileUrl("sprite.qml"), 600);
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
        QCOMPARE(d->color.r, (uchar)255);
        QCOMPARE(d->color.g, (uchar)255);
        QCOMPARE(d->color.b, (uchar)255);
        QCOMPARE(d->color.a, (uchar)255);
        QCOMPARE(d->xx, 1.0f);
        QCOMPARE(d->xy, 0.0f);
        QCOMPARE(d->yy, 1.0f);
        QCOMPARE(d->yx, 0.0f);
        QCOMPARE(d->rotation, 0.0f);
        QCOMPARE(d->rotationVelocity, 0.0f);
        QCOMPARE(d->autoRotate, (uchar)0);
        QVERIFY(myFuzzyCompare(d->frameDuration, 120.f));
        QCOMPARE(d->frameCount, 6.0f);
        QVERIFY(d->animT > 0.0f);
        QCOMPARE(d->animX, 0.0f);
        QCOMPARE(d->animY, 0.0f);
        QCOMPARE(d->animWidth, 31.0f);
        QCOMPARE(d->animHeight, 30.0f);
    }
    delete view;
}

QTEST_MAIN(tst_qquickimageparticle);

#include "tst_qquickimageparticle.moc"
