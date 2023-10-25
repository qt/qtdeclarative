// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>
#include <QtQuick>
#include <QtQuickControls2/qquickstyle.h>
#include <QStyleHints>

class tst_PressAndHold : public QObject
{
    Q_OBJECT

public:
    tst_PressAndHold();

private slots:
    void initTestCase();
    void cleanupTestCase();

    void pressAndHold_data();
    void pressAndHold();

    void keepSelection_data();
    void keepSelection();
};

tst_PressAndHold::tst_PressAndHold()
{
    QQuickStyle::setStyle("Basic");
}

void tst_PressAndHold::initTestCase()
{
    QGuiApplication::styleHints()->setMousePressAndHoldInterval(100);
}

void tst_PressAndHold::cleanupTestCase()
{
    QGuiApplication::styleHints()->setMousePressAndHoldInterval(-1);
}

void tst_PressAndHold::pressAndHold_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<QByteArray>("signal");
    QTest::addColumn<bool>("acceptsRightMouseButton");

    QTest::newRow("Button") << QByteArray("import QtQuick.Controls; Button { text: 'Button' }") << QByteArray(SIGNAL(pressAndHold())) << false;
    QTest::newRow("SwipeDelegate") << QByteArray("import QtQuick.Controls; SwipeDelegate { text: 'SwipeDelegate' }") << QByteArray(SIGNAL(pressAndHold())) << false;
    QTest::newRow("TextField") << QByteArray("import QtQuick.Controls; TextField { text: 'TextField' }") << QByteArray(SIGNAL(pressAndHold(QQuickMouseEvent*))) << true;
    QTest::newRow("TextArea") << QByteArray("import QtQuick.Controls; TextArea { text: 'TextArea' }") << QByteArray(SIGNAL(pressAndHold(QQuickMouseEvent*))) << true;
}

void tst_PressAndHold::pressAndHold()
{
    QFETCH(QByteArray, data);
    QFETCH(QByteArray, signal);
    QFETCH(bool, acceptsRightMouseButton);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(data, QUrl());

    QScopedPointer<QObject> control(component.create());
    QScopedPointer<QObject> waitControl(component.create());
    QVERIFY(!control.isNull() && !waitControl.isNull());

    QSignalSpy spy(control.data(), signal);
    QSignalSpy waitSpy(waitControl.data(), signal);
    QVERIFY(spy.isValid() && waitSpy.isValid());

    int startDragDistance = QGuiApplication::styleHints()->startDragDistance();
    QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent press2(QEvent::MouseButtonPress, QPointF(), QPointF(),
                       Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QMouseEvent move(QEvent::MouseMove, QPointF(2 * startDragDistance, 2 * startDragDistance), QPointF(),
                     Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    // pressAndHold() emitted
    QGuiApplication::sendEvent(control.data(), &press);
    QTRY_COMPARE(spy.size(), 1);
    QGuiApplication::sendEvent(control.data(), &release);
    QCOMPARE(spy.size(), 1);
    spy.clear();

    // pressAndHold() canceled by release
    QGuiApplication::sendEvent(control.data(), &press);
    QGuiApplication::processEvents();
    QGuiApplication::sendEvent(control.data(), &release);
    QCOMPARE(spy.size(), 0);

    // pressAndHold() canceled by move
    QGuiApplication::sendEvent(control.data(), &press);
    QGuiApplication::sendEvent(control.data(), &move); // cancels pressAndHold()
    QGuiApplication::sendEvent(waitControl.data(), &press);
    // by the time the second control emits pressAndHold(), we can reliably
    // assume that the first control would have emitted pressAndHold() if it
    // wasn't canceled as appropriate by the move event above
    QTRY_COMPARE(waitSpy.size(), 1);
    QCOMPARE(spy.size(), 0);
    QGuiApplication::sendEvent(control.data(), &release);
    QGuiApplication::sendEvent(waitControl.data(), &release);
    QCOMPARE(waitSpy.size(), 1);
    QCOMPARE(spy.size(), 0);
    waitSpy.clear();

    // pressAndHold() canceled by 2nd press
    if (acceptsRightMouseButton) {
        QGuiApplication::sendEvent(control.data(), &press);
        QGuiApplication::sendEvent(control.data(), &press2); // cancels pressAndHold()
        QGuiApplication::sendEvent(waitControl.data(), &press);
        // by the time the second control emits pressAndHold(), we can reliably
        // assume that the first control would have emitted pressAndHold() if it
        // wasn't canceled as appropriate by the 2nd press event above
        QTRY_COMPARE(waitSpy.size(), 1);
        QCOMPARE(spy.size(), 0);
        QGuiApplication::sendEvent(control.data(), &release);
        QGuiApplication::sendEvent(waitControl.data(), &release);
        QCOMPARE(waitSpy.size(), 1);
        QCOMPARE(spy.size(), 0);
        waitSpy.clear();
    }
}

void tst_PressAndHold::keepSelection_data()
{
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("TextField") << QByteArray("import QtQuick.Controls; TextField { text: 'TextField' }");
    QTest::newRow("TextArea") << QByteArray("import QtQuick.Controls; TextArea { text: 'TextArea' }");
}

void tst_PressAndHold::keepSelection()
{
    QFETCH(QByteArray, data);

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(data, QUrl());

    QScopedPointer<QObject> control(component.create());
    QScopedPointer<QObject> waitControl(component.create());
    QVERIFY(!control.isNull() && !waitControl.isNull());

    QSignalSpy spy(control.data(), SIGNAL(pressAndHold(QQuickMouseEvent*)));
    QSignalSpy waitSpy(waitControl.data(), SIGNAL(pressAndHold(QQuickMouseEvent*)));
    QVERIFY(spy.isValid() && waitSpy.isValid());

    QMouseEvent press(QEvent::MouseButtonPress, QPointF(), QPointF(),
                      Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QMouseEvent press2(QEvent::MouseButtonPress, QPointF(), QPointF(),
                       Qt::RightButton, Qt::RightButton, Qt::NoModifier);
    QMouseEvent release(QEvent::MouseButtonRelease, QPointF(), QPointF(),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    QVERIFY(!control->property("text").toString().isEmpty());
    QVERIFY(QMetaObject::invokeMethod(control.data(), "selectAll"));
    QCOMPARE(control->property("selectedText"), control->property("text"));

    // pressAndHold() emitted => selection remains
    QGuiApplication::sendEvent(control.data(), &press);
    QTRY_COMPARE(spy.size(), 1);
    QGuiApplication::sendEvent(control.data(), &release);
    QCOMPARE(spy.size(), 1);
    QCOMPARE(control->property("selectedText"), control->property("text"));
    spy.clear();

    // pressAndHold() canceled by release => selection cleared
    QGuiApplication::sendEvent(control.data(), &press);
    QGuiApplication::processEvents();
    QGuiApplication::sendEvent(control.data(), &release);
    QCOMPARE(spy.size(), 0);
    QVERIFY(control->property("selectedText").toString().isEmpty());

    QVERIFY(QMetaObject::invokeMethod(control.data(), "selectAll"));
    QCOMPARE(control->property("selectedText"), control->property("text"));

    // pressAndHold() canceled by 2nd press => selection cleared
    QGuiApplication::sendEvent(control.data(), &press);
    QGuiApplication::sendEvent(control.data(), &press2); // cancels pressAndHold()
    QGuiApplication::sendEvent(waitControl.data(), &press);
    // by the time the second control emits pressAndHold(), we can reliably
    // assume that the first control would have emitted pressAndHold() if it
    // wasn't canceled as appropriate by the move event above
    QTRY_COMPARE(waitSpy.size(), 1);
    QCOMPARE(spy.size(), 0);
    QGuiApplication::sendEvent(control.data(), &release);
    QGuiApplication::sendEvent(waitControl.data(), &release);
    QCOMPARE(waitSpy.size(), 1);
    QCOMPARE(spy.size(), 0);
    QVERIFY(control->property("selectedText").toString().isEmpty());
    waitSpy.clear();
}

QTEST_MAIN(tst_PressAndHold)

#include "tst_pressandhold.moc"
