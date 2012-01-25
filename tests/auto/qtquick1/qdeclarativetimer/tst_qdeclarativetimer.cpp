/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QSignalSpy>
#include <qtest.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtQuick1/private/qdeclarativetimer_p.h>
#include <QtQuick1/qdeclarativeitem.h>
#include <QDebug>

class tst_qdeclarativetimer : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativetimer();

private slots:
    void notRepeating();
    void notRepeatingStart();
    void repeat();
    void noTriggerIfNotRunning();
    void triggeredOnStart();
    void triggeredOnStartRepeat();
    void changeDuration();
    void restart();
    void parentProperty();
};

class TimerHelper : public QObject
{
    Q_OBJECT
public:
    TimerHelper() : QObject(), count(0)
    {
    }

    int count;

public slots:
    void timeout() {
        ++count;
    }
};

#define TIMEOUT_TIMEOUT 200

tst_qdeclarativetimer::tst_qdeclarativetimer()
{
}

void tst_qdeclarativetimer::notRepeating()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 1.0\nTimer { interval: 100; running: true }"), QUrl::fromLocalFile(""));
    QDeclarative1Timer *timer = qobject_cast<QDeclarative1Timer*>(component.create());
    QVERIFY(timer != 0);
    QVERIFY(timer->isRunning());
    QVERIFY(!timer->isRepeating());
    QCOMPARE(timer->interval(), 100);

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));

    QTest::qWait(TIMEOUT_TIMEOUT);
    QCOMPARE(helper.count, 1);
    QTest::qWait(TIMEOUT_TIMEOUT);
    QCOMPARE(helper.count, 1);
    QVERIFY(timer->isRunning() == false);
}

void tst_qdeclarativetimer::notRepeatingStart()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 1.0\nTimer { interval: 100 }"), QUrl::fromLocalFile(""));
    QDeclarative1Timer *timer = qobject_cast<QDeclarative1Timer*>(component.create());
    QVERIFY(timer != 0);
    QVERIFY(!timer->isRunning());

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));

    QTest::qWait(TIMEOUT_TIMEOUT);
    QCOMPARE(helper.count, 0);

    timer->start();
    QTest::qWait(TIMEOUT_TIMEOUT);
    QCOMPARE(helper.count, 1);
    QTest::qWait(TIMEOUT_TIMEOUT);
    QCOMPARE(helper.count, 1);
    QVERIFY(timer->isRunning() == false);

    delete timer;
}

void tst_qdeclarativetimer::repeat()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 1.0\nTimer { interval: 100; repeat: true; running: true }"), QUrl::fromLocalFile(""));
    QDeclarative1Timer *timer = qobject_cast<QDeclarative1Timer*>(component.create());
    QVERIFY(timer != 0);

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));
    QCOMPARE(helper.count, 0);

    QTest::qWait(TIMEOUT_TIMEOUT);
    QVERIFY(helper.count > 0);
    int oldCount = helper.count;

    QTest::qWait(TIMEOUT_TIMEOUT);
    QVERIFY(helper.count > oldCount);
    QVERIFY(timer->isRunning());

    oldCount = helper.count;
    timer->stop();

    QTest::qWait(TIMEOUT_TIMEOUT);
    QVERIFY(helper.count == oldCount);
    QVERIFY(timer->isRunning() == false);

    QSignalSpy spy(timer, SIGNAL(repeatChanged()));

    timer->setRepeating(false);
    QVERIFY(!timer->isRepeating());
    QCOMPARE(spy.count(),1);

    timer->setRepeating(false);
    QCOMPARE(spy.count(),1);

    timer->setRepeating(true);
    QCOMPARE(spy.count(),2);

    delete timer;
}

void tst_qdeclarativetimer::triggeredOnStart()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 1.0\nTimer { interval: 100; running: true; triggeredOnStart: true }"), QUrl::fromLocalFile(""));
    QDeclarative1Timer *timer = qobject_cast<QDeclarative1Timer*>(component.create());
    QVERIFY(timer != 0);
    QVERIFY(timer->triggeredOnStart());

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));
    QTest::qWait(1);
    QCOMPARE(helper.count, 1);

    QTest::qWait(TIMEOUT_TIMEOUT);
    QCOMPARE(helper.count, 2);
    QTest::qWait(TIMEOUT_TIMEOUT);
    QCOMPARE(helper.count, 2);
    QVERIFY(timer->isRunning() == false);

    QSignalSpy spy(timer, SIGNAL(triggeredOnStartChanged()));

    timer->setTriggeredOnStart(false);
    QVERIFY(!timer->triggeredOnStart());
    QCOMPARE(spy.count(),1);

    timer->setTriggeredOnStart(false);
    QCOMPARE(spy.count(),1);

    timer->setTriggeredOnStart(true);
    QCOMPARE(spy.count(),2);

    delete timer;
}

void tst_qdeclarativetimer::triggeredOnStartRepeat()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 1.0\nTimer { interval: 100; running: true; triggeredOnStart: true; repeat: true }"), QUrl::fromLocalFile(""));
    QDeclarative1Timer *timer = qobject_cast<QDeclarative1Timer*>(component.create());
    QVERIFY(timer != 0);

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));
    QTest::qWait(1);
    QCOMPARE(helper.count, 1);

    QTest::qWait(TIMEOUT_TIMEOUT);
    QVERIFY(helper.count > 1);
    int oldCount = helper.count;
    QTest::qWait(TIMEOUT_TIMEOUT);
    QVERIFY(helper.count > oldCount);
    QVERIFY(timer->isRunning());

    delete timer;
}

void tst_qdeclarativetimer::noTriggerIfNotRunning()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray(
        "import QtQuick 1.0\n"
        "Item { property bool ok: true\n"
            "Timer { id: t1; interval: 100; repeat: true; running: true; onTriggered: if (!running) ok=false }"
            "Timer { interval: 10; running: true; onTriggered: t1.running=false }"
        "}"
    ), QUrl::fromLocalFile(""));
    QObject *item = component.create();
    QVERIFY(item != 0);
    QTest::qWait(TIMEOUT_TIMEOUT);
    QCOMPARE(item->property("ok").toBool(), true);

    delete item;
}

void tst_qdeclarativetimer::changeDuration()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 1.0\nTimer { interval: 200; repeat: true; running: true }"), QUrl::fromLocalFile(""));
    QDeclarative1Timer *timer = qobject_cast<QDeclarative1Timer*>(component.create());
    QVERIFY(timer != 0);

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));
    QCOMPARE(helper.count, 0);

    QTest::qWait(500);
    QCOMPARE(helper.count, 2);

    timer->setInterval(500);

    QTest::qWait(600);
    QCOMPARE(helper.count, 3);
    QVERIFY(timer->isRunning());

    QSignalSpy spy(timer, SIGNAL(intervalChanged()));

    timer->setInterval(200);
    QCOMPARE(timer->interval(), 200);
    QCOMPARE(spy.count(),1);

    timer->setInterval(200);
    QCOMPARE(spy.count(),1);

    timer->setInterval(300);
    QCOMPARE(spy.count(),2);

    delete timer;
}

void tst_qdeclarativetimer::restart()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 1.0\nTimer { interval: 500; repeat: true; running: true }"), QUrl::fromLocalFile(""));
    QDeclarative1Timer *timer = qobject_cast<QDeclarative1Timer*>(component.create());
    QVERIFY(timer != 0);

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));
    QCOMPARE(helper.count, 0);

    QTest::qWait(600);
    QCOMPARE(helper.count, 1);

    QTest::qWait(300);

    timer->restart();

    QTest::qWait(700);

    QCOMPARE(helper.count, 2);
    QVERIFY(timer->isRunning());

    delete timer;
}

void tst_qdeclarativetimer::parentProperty()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 1.0\nItem { Timer { objectName: \"timer\"; running: parent.visible } }"), QUrl::fromLocalFile(""));
    QDeclarativeItem *item = qobject_cast<QDeclarativeItem*>(component.create());
    QVERIFY(item != 0);
    QDeclarative1Timer *timer = item->findChild<QDeclarative1Timer*>("timer");
    QVERIFY(timer != 0);

    QVERIFY(timer->isRunning());

    delete timer;
}

QTEST_MAIN(tst_qdeclarativetimer)

#include "tst_qdeclarativetimer.moc"
