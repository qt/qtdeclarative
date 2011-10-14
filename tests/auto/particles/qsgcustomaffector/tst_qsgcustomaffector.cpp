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

class tst_qsgcustomaffector : public QObject
{
    Q_OBJECT
public:
    tst_qsgcustomaffector();

private slots:
    void test_basic();
};

tst_qsgcustomaffector::tst_qsgcustomaffector()
{
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qsgcustomaffector::test_basic()
{
    QQuickView* view = createView(QCoreApplication::applicationDirPath() + "/data/basic.qml", 600);
    QSGParticleSystem* system = view->rootObject()->findChild<QSGParticleSystem*>("system");
    ensureAnimTime(600, system->m_animation);

    QCOMPARE(system->groupData[0]->size(), 500);
    foreach (QSGParticleData *d, system->groupData[0]->data) {
        if (d->t == -1)
            continue; //Particle data unused

        QCOMPARE(d->x, 100.f);
        QCOMPARE(d->y, 100.f);
        QCOMPARE(d->vx, 100.f);
        QCOMPARE(d->vy, 100.f);
        QCOMPARE(d->ax, 100.f);
        QCOMPARE(d->ay, 100.f);
        QCOMPARE(d->lifeSpan, 0.5f);
        QCOMPARE(d->size, 100.f);
        QCOMPARE(d->endSize, 100.f);
        QVERIFY(myFuzzyLEQ(d->t, ((qreal)system->timeInt/1000.0)));
    }
}

QTEST_MAIN(tst_qsgcustomaffector);

#include "tst_qsgcustomaffector.moc"
