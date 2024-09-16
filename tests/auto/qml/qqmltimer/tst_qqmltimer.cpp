// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <private/qabstractanimation_p.h>
#include <private/qqmltimer_p.h>

#include <QtQuick/qquickitem.h>

#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlengine.h>

#include <QtTest/qsignalspy.h>
#include <QtTest/qtest.h>

#include <QtCore/qpauseanimation.h>

void consistentWait(int ms)
{
    //Use animations for timing, because we enabled consistentTiming
    //This function will qWait for >= ms worth of consistent timing to elapse
    QPauseAnimation waitTimer(ms);
    waitTimer.start();
    while (waitTimer.state() == QAbstractAnimation::Running)
        QTest::qWait(20);
}

class tst_qqmltimer : public QObject
{
    Q_OBJECT
public:
    tst_qqmltimer();

private slots:
    void initTestCase();
    void notRepeating();
    void notRepeatingStart();
    void repeat();
    void noTriggerIfNotRunning();
    void triggeredOnStart();
    void triggeredOnStartRepeat();
    void changeDuration();
    void restart();
    void restartFromTriggered();
    void runningFromTriggered();
    void parentProperty();
    void stopWhenEventPosted();
    void restartWhenEventPosted();
};

class TimerHelper : public QObject
{
    Q_OBJECT
public:
    TimerHelper() { }

    int count = 0;

public slots:
    void timeout() {
        ++count;
    }
};

tst_qqmltimer::tst_qqmltimer() { }

void tst_qqmltimer::initTestCase()
{
    QUnifiedTimer::instance()->setConsistentTiming(true);
}

void tst_qqmltimer::notRepeating()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQml 2.0\nTimer { interval: 100; running: true }"), QUrl::fromLocalFile(""));
    std::unique_ptr<QObject> o { component.create() };
    QQmlTimer *timer = qobject_cast<QQmlTimer*>(o.get());
    QVERIFY(timer != nullptr);
    QVERIFY(timer->isRunning());
    QVERIFY(!timer->isRepeating());
    QCOMPARE(timer->interval(), 100);

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));


    consistentWait(200);
    QCOMPARE(helper.count, 1);
    consistentWait(200);
    QCOMPARE(helper.count, 1);
    QVERIFY(!timer->isRunning());
}

void tst_qqmltimer::notRepeatingStart()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQml 2.0\nTimer { interval: 100 }"), QUrl::fromLocalFile(""));
    std::unique_ptr<QQmlTimer> timer { qobject_cast<QQmlTimer*>(component.create()) };
    QVERIFY(timer.get());
    QVERIFY(!timer->isRunning());

    TimerHelper helper;
    connect(timer.get(), SIGNAL(triggered()), &helper, SLOT(timeout()));

    consistentWait(200);
    QCOMPARE(helper.count, 0);

    timer->start();
    consistentWait(200);
    QCOMPARE(helper.count, 1);
    consistentWait(200);
    QCOMPARE(helper.count, 1);
    QVERIFY(!timer->isRunning());
}

void tst_qqmltimer::repeat()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQml 2.0\nTimer { interval: 100; repeat: true; running: true }"), QUrl::fromLocalFile(""));
    std::unique_ptr<QQmlTimer> timer { qobject_cast<QQmlTimer*>(component.create()) };
    QVERIFY(timer);

    TimerHelper helper;
    connect(timer.get(), SIGNAL(triggered()), &helper, SLOT(timeout()));
    QCOMPARE(helper.count, 0);

    consistentWait(200);
    QVERIFY(helper.count > 0);
    int oldCount = helper.count;

    consistentWait(200);
    QVERIFY(helper.count > oldCount);
    QVERIFY(timer->isRunning());

    oldCount = helper.count;
    timer->stop();

    consistentWait(200);
    QCOMPARE(helper.count, oldCount);
    QVERIFY(!timer->isRunning());

    QSignalSpy spy(timer.get(), SIGNAL(repeatChanged()));

    timer->setRepeating(false);
    QVERIFY(!timer->isRepeating());
    QCOMPARE(spy.size(),1);

    timer->setRepeating(false);
    QCOMPARE(spy.size(),1);

    timer->setRepeating(true);
    QCOMPARE(spy.size(),2);
}

void tst_qqmltimer::triggeredOnStart()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQml 2.0\nTimer { interval: 100; running: true; triggeredOnStart: true }"), QUrl::fromLocalFile(""));
    std::unique_ptr<QQmlTimer> timer { qobject_cast<QQmlTimer*>(component.create()) };
    QVERIFY(timer);
    QVERIFY(timer->triggeredOnStart());

    TimerHelper helper;
    connect(timer.get(), SIGNAL(triggered()), &helper, SLOT(timeout()));
    consistentWait(1);
    QCOMPARE(helper.count, 1);
    consistentWait(200);
    QCOMPARE(helper.count, 2);
    consistentWait(200);
    QCOMPARE(helper.count, 2);
    QVERIFY(!timer->isRunning());

    QSignalSpy spy(timer.get(), SIGNAL(triggeredOnStartChanged()));

    timer->setTriggeredOnStart(false);
    QVERIFY(!timer->triggeredOnStart());
    QCOMPARE(spy.size(),1);

    timer->setTriggeredOnStart(false);
    QCOMPARE(spy.size(),1);

    timer->setTriggeredOnStart(true);
    QCOMPARE(spy.size(),2);
}

void tst_qqmltimer::triggeredOnStartRepeat()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQml 2.0\nTimer { interval: 100; running: true; triggeredOnStart: true; repeat: true }"), QUrl::fromLocalFile(""));
    std::unique_ptr<QQmlTimer> timer { qobject_cast<QQmlTimer*>(component.create()) };
    QVERIFY(timer);

    TimerHelper helper;
    connect(timer.get(), SIGNAL(triggered()), &helper, SLOT(timeout()));
    consistentWait(1);
    QCOMPARE(helper.count, 1);

    consistentWait(200);
    QVERIFY(helper.count > 1);
    int oldCount = helper.count;
    consistentWait(200);
    QVERIFY(helper.count > oldCount);
    QVERIFY(timer->isRunning());
}

void tst_qqmltimer::noTriggerIfNotRunning()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray(
        "import QtQml 2.0\n"
        "QtObject { property bool ok: true\n"
            "property Timer timer1: Timer { id: t1; interval: 100; repeat: true; running: true; onTriggered: if (!running) ok=false }"
            "property Timer timer2: Timer { interval: 10; running: true; onTriggered: t1.running=false }"
        "}"
    ), QUrl::fromLocalFile(""));
    std::unique_ptr<QObject> item { component.create() };
    QVERIFY(item);
    consistentWait(200);
    QCOMPARE(item->property("ok").toBool(), true);
}

void tst_qqmltimer::changeDuration()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQml 2.0\nTimer { interval: 200; repeat: true; running: true }"), QUrl::fromLocalFile(""));
    std::unique_ptr<QQmlTimer> timer { qobject_cast<QQmlTimer*>(component.create()) };
    QVERIFY(timer);

    TimerHelper helper;
    connect(timer.get(), SIGNAL(triggered()), &helper, SLOT(timeout()));
    QCOMPARE(helper.count, 0);

    consistentWait(500);
    QCOMPARE(helper.count, 2);

    timer->setInterval(500);

    consistentWait(600);
    QCOMPARE(helper.count, 3);
    QVERIFY(timer->isRunning());

    QSignalSpy spy(timer.get(), SIGNAL(intervalChanged()));

    timer->setInterval(200);
    QCOMPARE(timer->interval(), 200);
    QCOMPARE(spy.size(),1);

    timer->setInterval(200);
    QCOMPARE(spy.size(),1);

    timer->setInterval(300);
    QCOMPARE(spy.size(),2);
}

void tst_qqmltimer::restart()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQml 2.0\nTimer { interval: 1000; repeat: true; running: true }"), QUrl::fromLocalFile(""));
    std::unique_ptr<QQmlTimer> timer { qobject_cast<QQmlTimer*>(component.create()) };
    QVERIFY(timer);

    TimerHelper helper;
    connect(timer.get(), SIGNAL(triggered()), &helper, SLOT(timeout()));
    QCOMPARE(helper.count, 0);

    consistentWait(1200);
    QCOMPARE(helper.count, 1);

    consistentWait(500);

    QCOMPARE(helper.count, 1);
    timer->restart();
    QCOMPARE(helper.count, 1);

    consistentWait(1400);

    QCOMPARE(helper.count, 2);
    QVERIFY(timer->isRunning());
}

void tst_qqmltimer::restartFromTriggered()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQml 2.0\nTimer { "
                                    "interval: 500; "
                                    "repeat: false; "
                                    "running: true; "
                                    "onTriggered: restart()"
                                 " }"), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(component.create());
    QQmlTimer *timer = qobject_cast<QQmlTimer*>(object.data());
    QVERIFY(timer != nullptr);

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));
    QCOMPARE(helper.count, 0);

    consistentWait(600);
    QCOMPARE(helper.count, 1);
    QVERIFY(timer->isRunning());

    consistentWait(600);
    QCOMPARE(helper.count, 2);
    QVERIFY(timer->isRunning());
}

void tst_qqmltimer::runningFromTriggered()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQml 2.0\nTimer { "
                                    "property bool ok: false; "
                                    "interval: 500; "
                                    "repeat: false; "
                                    "running: true; "
                                    "onTriggered: { ok = !running; running = true }"
                                 " }"), QUrl::fromLocalFile(""));
    QScopedPointer<QObject> object(component.create());
    QQmlTimer *timer = qobject_cast<QQmlTimer*>(object.data());
    QVERIFY(timer != nullptr);

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));
    QCOMPARE(helper.count, 0);

    consistentWait(600);
    QCOMPARE(helper.count, 1);
    QVERIFY(timer->property("ok").toBool());
    QVERIFY(timer->isRunning());

    consistentWait(600);
    QCOMPARE(helper.count, 2);
    QVERIFY(timer->property("ok").toBool());
    QVERIFY(timer->isRunning());
}

void tst_qqmltimer::parentProperty()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQuick 2.0\nItem { Timer { objectName: \"timer\"; running: parent.visible } }"), QUrl::fromLocalFile(""));
    std::unique_ptr<QQuickItem> item { qobject_cast<QQuickItem*>(component.create()) };
    QVERIFY(item);
    QQmlTimer *timer = item->findChild<QQmlTimer*>("timer");
    QVERIFY(timer != nullptr);

    QVERIFY(timer->isRunning());
}

void tst_qqmltimer::stopWhenEventPosted()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQml 2.0\nTimer { interval: 200; running: true }"), QUrl::fromLocalFile(""));
    std::unique_ptr<QObject> o { component.create() };
    QQmlTimer *timer = qobject_cast<QQmlTimer*>(o.get());

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));
    QCOMPARE(helper.count, 0);

    // Use QThread::msleep() as QTest::qWait() always calls sendPostedEvents before
    // exiting, so we can't use it to stop between an event is posted and it is received.
    QThread::msleep(200);
    QCOMPARE(helper.count, 0);
    QVERIFY(timer->isRunning());
    timer->stop();
    QVERIFY(!timer->isRunning());

    consistentWait(300);
    QCOMPARE(helper.count, 0);
}


void tst_qqmltimer::restartWhenEventPosted()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QByteArray("import QtQml 2.0\nTimer { interval: 200; running: true }"), QUrl::fromLocalFile(""));
    std::unique_ptr<QObject> o { component.create() };
    QQmlTimer *timer = qobject_cast<QQmlTimer*>(o.get());

    TimerHelper helper;
    connect(timer, SIGNAL(triggered()), &helper, SLOT(timeout()));
    QCOMPARE(helper.count, 0);

    // Use QThread::msleep() as QTest::qWait() always calls sendPostedEvents before
    // exiting, so we can't use it to stop between an event is posted and it is received.
    QThread::msleep(200);
    QCOMPARE(helper.count, 0);
    timer->restart();

    consistentWait(100);
    QCOMPARE(helper.count, 0);
    QVERIFY(timer->isRunning());

    consistentWait(200);
    QCOMPARE(helper.count, 1);
}

QTEST_MAIN(tst_qqmltimer)

#include "tst_qqmltimer.moc"
