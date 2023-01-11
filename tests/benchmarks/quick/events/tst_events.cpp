// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <qtest.h>
#include <QtQuick>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QDebug>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>

class TestView : public QQuickView
{
public:
    void handleEvent(QEvent *ev) { event(ev); }
};


class tst_events : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_events();

private slots:
    void mousePressRelease();
    void mouseMove();
    void touchToMousePressRelease();
    void touchToMousePressMove();

public slots:
    void initTestCase() override {
        QQmlDataTest::initTestCase();
        window.setBaseSize(QSize(400, 400));
        window.setSource(testFileUrl("mouseevent.qml"));
        window.show();
        QVERIFY(QTest::qWaitForWindowExposed(&window));
    }

private:
    TestView window;
};

tst_events::tst_events()
    : QQmlDataTest(QT_QMLTEST_DATADIR)
{
}

void tst_events::mousePressRelease()
{
    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mouseArea");
    QCOMPARE(mouseArea->isPressed(), false);

    const QPoint localPos(100, 100);
    const QPoint globalPos = window.mapToGlobal(localPos);
    QBENCHMARK {
        QMouseEvent pressEvent(QEvent::MouseButtonPress, localPos, globalPos, Qt::LeftButton, Qt::LeftButton, {});
        window.handleEvent(&pressEvent);
        QCOMPARE(mouseArea->isPressed(), true);
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, localPos, globalPos, Qt::LeftButton, Qt::LeftButton, {});
        window.handleEvent(&releaseEvent);
    }
    QCOMPARE(mouseArea->isPressed(), false);
}

void tst_events::mouseMove()
{
    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mouseArea");
    const QPoint localPos1(100, 100);
    const QPoint globalPos1 = window.mapToGlobal(localPos1);
    const QPoint localPos2(101, 100);
    const QPoint globalPos2 = window.mapToGlobal(localPos2);
    QCOMPARE(mouseArea->isPressed(), false);

    QMouseEvent pressEvent(QEvent::MouseButtonPress, localPos1, globalPos1, Qt::LeftButton, Qt::LeftButton, {});
    window.handleEvent(&pressEvent);
    QCOMPARE(mouseArea->isPressed(), true);
    QMouseEvent moveEvent1(QEvent::MouseMove, localPos2, globalPos2, Qt::LeftButton, Qt::LeftButton, {});
    QMouseEvent moveEvent2(QEvent::MouseMove, localPos1, globalPos1, Qt::LeftButton, Qt::LeftButton, {});
    QBENCHMARK {
        window.handleEvent(&moveEvent1);
        window.handleEvent(&moveEvent2);
    }
    QCOMPARE(mouseArea->isPressed(), true);
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, localPos1, globalPos1, Qt::LeftButton, Qt::LeftButton, {});
    window.handleEvent(&releaseEvent);
    QCOMPARE(mouseArea->isPressed(), false);
}

void tst_events::touchToMousePressRelease()
{
    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mouseArea");
    QCOMPARE(mouseArea->isPressed(), false);

    auto device = QTest::createTouchDevice();
    auto p = QPoint(80, 80);

    QBENCHMARK {
        QTest::touchEvent(&window, device).press(0, p, &window).commit();
        QCOMPARE(mouseArea->isPressed(), true);
        QTest::touchEvent(&window, device).release(0, p, &window).commit();
    }
    QCOMPARE(mouseArea->isPressed(), false);
}

void tst_events::touchToMousePressMove()
{
    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mouseArea");
    QCOMPARE(mouseArea->isPressed(), false);

    auto device = QTest::createTouchDevice();
    auto p = QPoint(80, 80);
    auto p2 = QPoint(81, 80);

    QTest::touchEvent(&window, device).press(0, p, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(mouseArea->isPressed(), true);

    QBENCHMARK {
        QTest::touchEvent(&window, device).move(0, p, &window).commit();
        QCOMPARE(mouseArea->isPressed(), true);
        QTest::touchEvent(&window, device).move(0, p2, &window).commit();
    }
    QCOMPARE(mouseArea->isPressed(), true);
    QTest::touchEvent(&window, device).release(0, p, &window).commit();
    QQuickTouchUtils::flush(&window);
    QCOMPARE(mouseArea->isPressed(), false);
}

QTEST_MAIN(tst_events)
#include "tst_events.moc"
