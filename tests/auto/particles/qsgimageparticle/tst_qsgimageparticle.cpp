/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include "../shared/particlestestsshared.h"
#include <private/qsgparticlesystem_p.h>
#include <private/qabstractanimation_p.h>

const double CONV_FACTOR = 0.017453292519943295;//Degrees to radians

class tst_qsgimageparticle : public QObject
{
    Q_OBJECT
public:
    tst_qsgimageparticle();

private slots:
    void test_basic();
    void test_colored();
    void test_deformed();
    void test_tabled();
    void test_sprite();
};

tst_qsgimageparticle::tst_qsgimageparticle()
{
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qsgimageparticle::test_basic()
{
    QSGView* view = createView(QCoreApplication::applicationDirPath() + "/data/basic.qml", 600);
    QSGParticleSystem* system = view->rootObject()->findChild<QSGParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QCOMPARE(system->groupData[0]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[0]->data) {
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
        QCOMPARE(d->rotationSpeed, 0.0f);
        QCOMPARE(d->autoRotate, 0.0f);
        QCOMPARE(d->animX, 0.0f);
        QCOMPARE(d->animY, 0.0f);
        QCOMPARE(d->animWidth, 1.0f);
        QCOMPARE(d->animHeight, 1.0f);
        QCOMPARE(d->frameDuration, 1.0f);
        QCOMPARE(d->frameCount, 1.0f);
        QCOMPARE(d->animT, -1.0f);
    }
}


void tst_qsgimageparticle::test_colored()
{
    QSGView* view = createView(QCoreApplication::applicationDirPath() + "/data/colored.qml", 600);
    QSGParticleSystem* system = view->rootObject()->findChild<QSGParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QCOMPARE(system->groupData[0]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[0]->data) {
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
        QCOMPARE(d->rotationSpeed, 0.0f);
        QCOMPARE(d->autoRotate, 0.0f);
        QCOMPARE(d->animX, 0.0f);
        QCOMPARE(d->animY, 0.0f);
        QCOMPARE(d->animWidth, 1.0f);
        QCOMPARE(d->animHeight, 1.0f);
        QCOMPARE(d->frameDuration, 1.0f);
        QCOMPARE(d->frameCount, 1.0f);
        QCOMPARE(d->animT, -1.0f);
    }
}


void tst_qsgimageparticle::test_deformed()
{
    QSGView* view = createView(QCoreApplication::applicationDirPath() + "/data/deformed.qml", 600);
    QSGParticleSystem* system = view->rootObject()->findChild<QSGParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QCOMPARE(system->groupData[0]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[0]->data) {
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
        QCOMPARE(d->rotation, 90.0f * (float)CONV_FACTOR);
        QCOMPARE(d->rotationSpeed, 90.0f * (float)CONV_FACTOR);
        QCOMPARE(d->autoRotate, 1.0f);
        QCOMPARE(d->animX, 0.0f);
        QCOMPARE(d->animY, 0.0f);
        QCOMPARE(d->animWidth, 1.0f);
        QCOMPARE(d->animHeight, 1.0f);
        QCOMPARE(d->frameDuration, 1.0f);
        QCOMPARE(d->frameCount, 1.0f);
        QCOMPARE(d->animT, -1.0f);
    }
}


void tst_qsgimageparticle::test_tabled()
{
    QSGView* view = createView(QCoreApplication::applicationDirPath() + "/data/tabled.qml", 600);
    QSGParticleSystem* system = view->rootObject()->findChild<QSGParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QCOMPARE(system->groupData[0]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[0]->data) {
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
        QCOMPARE(d->rotationSpeed, 0.0f);
        QCOMPARE(d->autoRotate, 0.0f);
        QCOMPARE(d->animX, 0.0f);
        QCOMPARE(d->animY, 0.0f);
        QCOMPARE(d->animWidth, 1.0f);
        QCOMPARE(d->animHeight, 1.0f);
        QCOMPARE(d->frameDuration, 1.0f);
        QCOMPARE(d->frameCount, 1.0f);
        QCOMPARE(d->animT, -1.0f);
        //TODO: This performance level doesn't alter particleData, but goes straight to shaders. Find something to test
    }
}


void tst_qsgimageparticle::test_sprite()
{
    QSGView* view = createView(QCoreApplication::applicationDirPath() + "/data/sprite.qml", 600);
    QSGParticleSystem* system = view->rootObject()->findChild<QSGParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QCOMPARE(system->groupData[0]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[0]->data) {
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
        QCOMPARE(d->rotationSpeed, 0.0f);
        QCOMPARE(d->autoRotate, 0.0f);
        QVERIFY(myFuzzyCompare(d->frameDuration, 120.f));
        QCOMPARE(d->frameCount, 6.0f);
        QVERIFY(d->animT > 0.0f);
        QCOMPARE(d->animX, 0.0f);
        QCOMPARE(d->animY, 0.0f);
        QCOMPARE(d->animWidth, 31.0f);
        QCOMPARE(d->animHeight, 30.0f);
    }
}

QTEST_MAIN(tst_qsgimageparticle);

#include "tst_qsgimageparticle.moc"
