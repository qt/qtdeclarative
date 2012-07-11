/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QtTest>
#include "../../shared/util.h"
#include <QtQuick/qquickview.h>
#include <private/qabstractanimation_p.h>
#include <private/qquickanimatedsprite_p.h>

class tst_qquickanimatedsprite : public QQmlDataTest
{
    Q_OBJECT
public:
    tst_qquickanimatedsprite(){}

private slots:
    void initTestCase();
    void test_properties();
    void test_frameChangedSignal();
};

void tst_qquickanimatedsprite::initTestCase()
{
    QQmlDataTest::initTestCase();
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qquickanimatedsprite::test_properties()
{
    QQuickView *window = new QQuickView(0);

    window->setSource(testFileUrl("basic.qml"));
    window->show();
    QTest::qWaitForWindowShown(window);

    QVERIFY(window->rootObject());
    QQuickAnimatedSprite* sprite = window->rootObject()->findChild<QQuickAnimatedSprite*>("sprite");
    QVERIFY(sprite);

    QVERIFY(sprite->running());
    QVERIFY(!sprite->paused());
    QVERIFY(sprite->interpolate());
    QCOMPARE(sprite->loops(), 3);

    sprite->setRunning(false);
    QVERIFY(!sprite->running());
    sprite->setInterpolate(false);
    QVERIFY(!sprite->interpolate());

    delete window;
}

void tst_qquickanimatedsprite::test_frameChangedSignal()
{
    QQuickView *window = new QQuickView(0);

    window->setSource(testFileUrl("frameChange.qml"));
    window->show();
    QTest::qWaitForWindowShown(window);

    QVERIFY(window->rootObject());
    QQuickAnimatedSprite* sprite = window->rootObject()->findChild<QQuickAnimatedSprite*>("sprite");
    QVERIFY(sprite);

    QVERIFY(!sprite->running());
    QVERIFY(!sprite->paused());
    QCOMPARE(sprite->loops(), 3);
    QCOMPARE(sprite->frameCount(), 6);

    QSignalSpy frameChangedSpy(sprite, SIGNAL(currentFrameChanged(int)));
    sprite->setRunning(true);
    QTRY_COMPARE(frameChangedSpy.count(), 3*6);
    QTRY_VERIFY(!sprite->running());

    delete window;
}

QTEST_MAIN(tst_qquickanimatedsprite)

#include "tst_qquickanimatedsprite.moc"
