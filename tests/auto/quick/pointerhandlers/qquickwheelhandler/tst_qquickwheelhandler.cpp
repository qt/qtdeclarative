/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtGui/QStyleHints>
#include <qpa/qwindowsysteminterface.h>
#include <private/qquickwheelhandler_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlcontext.h>
#include "../../../shared/util.h"
#include "../../shared/viewtestutil.h"

Q_LOGGING_CATEGORY(lcPointerTests, "qt.quick.pointer.tests")

class tst_QQuickWheelHandler: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickWheelHandler() { }

private slots:
    void singleHandler_data();
    void singleHandler();
    void nestedHandler_data();
    void nestedHandler();

private:
    void sendWheelEvent(QQuickView &window, QPoint pos, QPoint angleDelta,
                        QPoint pixelDelta = QPoint(), Qt::KeyboardModifiers modifiers = Qt::NoModifier,
                        Qt::ScrollPhase phase = Qt::NoScrollPhase, bool inverted = false);
};

void tst_QQuickWheelHandler::sendWheelEvent(QQuickView &window, QPoint pos, QPoint angleDelta,
                                            QPoint pixelDelta, Qt::KeyboardModifiers modifiers, Qt::ScrollPhase phase, bool inverted)
{
    QWheelEvent wheelEvent(pos, window.mapToGlobal(pos), pixelDelta, angleDelta,
                           Qt::NoButton, modifiers, phase, inverted);
    QGuiApplication::sendEvent(&window, &wheelEvent);
    qApp->processEvents();
}

void tst_QQuickWheelHandler::singleHandler_data()
{
    // handler properties
    QTest::addColumn<Qt::Orientation>("orientation");
    QTest::addColumn<bool>("invertible");
    QTest::addColumn<int>("rotationScale");
    QTest::addColumn<QString>("property");
    QTest::addColumn<qreal>("targetScaleMultiplier");
    QTest::addColumn<bool>("targetTransformAroundCursor");
    // event
    QTest::addColumn<QPoint>("eventPos");
    QTest::addColumn<QPoint>("eventAngleDelta");
    QTest::addColumn<QPoint>("eventPixelDelta");
    QTest::addColumn<Qt::KeyboardModifiers>("eventModifiers");
    QTest::addColumn<bool>("eventPhases");
    QTest::addColumn<bool>("eventInverted");
    // result
    QTest::addColumn<QPoint>("expectedPosition");
    QTest::addColumn<qreal>("expectedScale");
    QTest::addColumn<int>("expectedRotation");

    // move the item
    QTest::newRow("vertical wheel angle delta to adjust x")
            << Qt::Vertical << false << 1 << "x" << 1.5 << true
            << QPoint(160, 120) << QPoint(-360, 120) << QPoint() << Qt::KeyboardModifiers(Qt::NoModifier) << false << false
            << QPoint(15, 0) << 1.0 << 0;
    QTest::newRow("horizontal wheel angle delta to adjust y")
            << Qt::Horizontal << false << 1 << "y" << 1.5 << false
            << QPoint(160, 120) << QPoint(-360, 120) << QPoint() << Qt::KeyboardModifiers(Qt::NoModifier) << false << false
            << QPoint(0, -45) << 1.0 << 0;
    QTest::newRow("vertical wheel angle delta to adjust y, amplified and inverted")
            << Qt::Vertical << true << 4 << "y" << 1.5 << true
            << QPoint(160, 120) << QPoint(60, 60) << QPoint() << Qt::KeyboardModifiers(Qt::NoModifier) << false << true
            << QPoint(0, 30) << 1.0 << 0;
    QTest::newRow("horizontal wheel angle delta to adjust x, amplified and reversed")
            << Qt::Horizontal << false << -4 << "x" << 1.5 << false
            << QPoint(160, 120) << QPoint(60, 60) << QPoint() << Qt::KeyboardModifiers(Qt::NoModifier) << false << false
            << QPoint(-30, 0) << 1.0 << 0;
    QTest::newRow("vertical wheel pixel delta to adjust x")
            << Qt::Vertical << false << 1 << "x" << 1.5 << true
            << QPoint(160, 120) << QPoint(-360, 120) << QPoint(20, 20) << Qt::KeyboardModifiers(Qt::NoModifier) << true << false
            << QPoint(20, 0) << 1.0 << 0;
    QTest::newRow("horizontal wheel pixel delta to adjust y")
            << Qt::Horizontal << false << 1 << "y" << 1.5 << false
            << QPoint(160, 120) << QPoint(-360, 120) << QPoint(20, 20) << Qt::KeyboardModifiers(Qt::NoModifier) << true << false
            << QPoint(0, 20) << 1.0 << 0;
    QTest::newRow("vertical wheel pixel delta to adjust y, amplified and inverted")
            << Qt::Vertical << true << 4 << "y" << 1.5 << true
            << QPoint(160, 120) << QPoint(60, 60) << QPoint(20, 20) << Qt::KeyboardModifiers(Qt::NoModifier) << true << true
            << QPoint(0, 80) << 1.0 << 0;
    QTest::newRow("horizontal wheel pixel delta to adjust x, amplified and reversed")
            << Qt::Horizontal << false << -4 << "x" << 1.5 << false
            << QPoint(160, 120) << QPoint(60, 60) << QPoint(20, 20) << Qt::KeyboardModifiers(Qt::NoModifier) << true << false
            << QPoint(-80, 0) << 1.0 << 0;

    // scale the item
    QTest::newRow("vertical wheel angle delta to adjust scale")
            << Qt::Vertical << false << 1 << "scale" << 1.5 << true
            << QPoint(50, 32) << QPoint(360, 120) << QPoint() << Qt::KeyboardModifiers(Qt::NoModifier) << false << false
            << QPoint(55, 44) << 1.5 << 0;
    QTest::newRow("horizontal wheel angle delta to adjust scale, amplified and reversed, don't adjust position")
            << Qt::Horizontal << false << -2 << "scale" << 1.5 << false
            << QPoint(50, 32) << QPoint(-240, 360) << QPoint() << Qt::KeyboardModifiers(Qt::NoModifier) << false << false
            << QPoint(0, 0) << 5.0625 << 0;

    // rotate the item
    QTest::newRow("vertical wheel angle delta to adjust rotation")
            << Qt::Vertical << false << 1 << "rotation" << 1.5 << true
            << QPoint(50, 32) << QPoint(360, -120) << QPoint() << Qt::KeyboardModifiers(Qt::NoModifier) << false << false
            << QPoint(19, -31) << 1.0 << -15;
    QTest::newRow("horizontal wheel angle delta to adjust rotation, amplified and reversed, don't adjust position")
            << Qt::Horizontal << false << -2 << "rotation" << 1.5 << false
            << QPoint(80, 80) << QPoint(240, 360) << QPoint() << Qt::KeyboardModifiers(Qt::NoModifier) << false << false
            << QPoint(0, 0) << 1.0 << -60;
}

void tst_QQuickWheelHandler::singleHandler()
{
    // handler properties
    QFETCH(Qt::Orientation, orientation);
    QFETCH(bool, invertible);
    QFETCH(int, rotationScale);
    QFETCH(QString, property);
    QFETCH(qreal, targetScaleMultiplier);
    QFETCH(bool, targetTransformAroundCursor);
    // event
    QFETCH(QPoint, eventPos);
    QFETCH(QPoint, eventAngleDelta);
    QFETCH(QPoint, eventPixelDelta);
    QFETCH(Qt::KeyboardModifiers, eventModifiers);
    QFETCH(bool, eventPhases);
    QFETCH(bool, eventInverted);
    // result
    QFETCH(QPoint, expectedPosition);
    QFETCH(qreal, expectedScale);
    QFETCH(int, expectedRotation);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("rectWheel.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickItem *rect = window.rootObject();
    QVERIFY(rect != nullptr);
    QQuickWheelHandler *handler = rect->findChild<QQuickWheelHandler*>();
    QVERIFY(handler != nullptr);
    handler->setOrientation(orientation);
    handler->setInvertible(invertible);
    handler->setRotationScale(rotationScale);
    handler->setProperty(property);
    handler->setTargetScaleMultiplier(targetScaleMultiplier);
    handler->setTargetTransformAroundCursor(targetTransformAroundCursor);
    QSignalSpy activeChangedSpy(handler, SIGNAL(activeChanged()));

    if (eventPhases) {
        sendWheelEvent(window, eventPos, QPoint(), QPoint(), eventModifiers, Qt::ScrollBegin, eventInverted);
        sendWheelEvent(window, eventPos, eventAngleDelta, eventPixelDelta, eventModifiers, Qt::ScrollUpdate, eventInverted);
    } else {
        sendWheelEvent(window, eventPos, eventAngleDelta, eventPixelDelta, eventModifiers, Qt::NoScrollPhase, eventInverted);
    }
    QCOMPARE(rect->position().toPoint(), expectedPosition);
    QCOMPARE(activeChangedSpy.count(), 1);
    QCOMPARE(handler->active(), true);
    QCOMPARE(rect->scale(), expectedScale);
    QCOMPARE(rect->rotation(), expectedRotation);
    if (!eventPhases) {
        QTRY_COMPARE(handler->active(), false);
        QCOMPARE(activeChangedSpy.count(), 2);
    }

    // restore by rotating backwards
    if (eventPhases) {
        sendWheelEvent(window, eventPos, eventAngleDelta * -1, eventPixelDelta * -1, eventModifiers, Qt::ScrollUpdate, eventInverted);
        sendWheelEvent(window, eventPos, QPoint(), QPoint(), eventModifiers, Qt::ScrollEnd, eventInverted);
    } else {
        sendWheelEvent(window, eventPos, eventAngleDelta * -1, eventPixelDelta * -1, eventModifiers, Qt::NoScrollPhase, eventInverted);
    }
    QCOMPARE(activeChangedSpy.count(), eventPhases ? 2 : 3);
    QCOMPARE(handler->active(), !eventPhases);
    QCOMPARE(rect->position().toPoint(), QPoint(0, 0));
    QCOMPARE(rect->scale(), 1);
    QCOMPARE(rect->rotation(), 0);
}

void tst_QQuickWheelHandler::nestedHandler_data()
{
    // handler properties
    QTest::addColumn<Qt::Orientation>("orientation");
    QTest::addColumn<bool>("invertible");
    QTest::addColumn<int>("rotationScale");
    QTest::addColumn<QString>("property");
    QTest::addColumn<qreal>("targetScaleMultiplier");
    QTest::addColumn<bool>("targetTransformAroundCursor");
    // event
    QTest::addColumn<QPoint>("eventPos");
    QTest::addColumn<QPoint>("eventAngleDelta");
    QTest::addColumn<QPoint>("eventPixelDelta");
    QTest::addColumn<Qt::KeyboardModifiers>("eventModifiers");
    QTest::addColumn<bool>("eventPhases");
    QTest::addColumn<bool>("eventInverted");
    QTest::addColumn<int>("eventCount");
    // result: inner handler
    QTest::addColumn<QPoint>("innerPosition");
    QTest::addColumn<qreal>("innerScale");
    QTest::addColumn<int>("innerRotation");
    // result: outer handler
    QTest::addColumn<QPoint>("outerPosition");
    QTest::addColumn<qreal>("outerScale");
    QTest::addColumn<int>("outerRotation");

    // move the item
    QTest::newRow("vertical wheel angle delta to adjust x")
            << Qt::Vertical << false << 1 << "x" << 1.5 << true
            << QPoint(160, 120) << QPoint(120, 120) << QPoint() << Qt::KeyboardModifiers(Qt::NoModifier) << false << false << 10
            << QPoint(175,60) << 1.0 << 0
            << QPoint(75, 0) << 1.0 << 0;
    QTest::newRow("horizontal wheel pixel delta to adjust y")
            << Qt::Horizontal << false << 1 << "y" << 1.5 << false
            << QPoint(160, 120) << QPoint(120, 120) << QPoint(50, 50) << Qt::KeyboardModifiers(Qt::NoModifier) << true << false << 4
            << QPoint(100, 160) << 1.0 << 0
            << QPoint(0, 100) << 1.0 << 0;
}

void tst_QQuickWheelHandler::nestedHandler()
{
    // handler properties
    QFETCH(Qt::Orientation, orientation);
    QFETCH(bool, invertible);
    QFETCH(int, rotationScale);
    QFETCH(QString, property);
    QFETCH(qreal, targetScaleMultiplier);
    QFETCH(bool, targetTransformAroundCursor);
    // event
    QFETCH(QPoint, eventPos);
    QFETCH(QPoint, eventAngleDelta);
    QFETCH(QPoint, eventPixelDelta);
    QFETCH(Qt::KeyboardModifiers, eventModifiers);
    QFETCH(bool, eventPhases);
    QFETCH(bool, eventInverted);
    QFETCH(int, eventCount);
    // result: inner handler
    QFETCH(QPoint, innerPosition);
    QFETCH(qreal, innerScale);
    QFETCH(int, innerRotation);
    // result: outer handler
    QFETCH(QPoint, outerPosition);
    QFETCH(qreal, outerScale);
    QFETCH(int, outerRotation);

    QQuickView window;
    QByteArray errorMessage;
    QVERIFY2(QQuickTest::initView(window, testFileUrl("nested.qml"), true, &errorMessage), errorMessage.constData());
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickItem *outerRect = window.rootObject();
    QVERIFY(outerRect != nullptr);
    QQuickWheelHandler *outerHandler = outerRect->findChild<QQuickWheelHandler*>("outerWheelHandler");
    QVERIFY(outerHandler != nullptr);
    QQuickWheelHandler *innerHandler = outerRect->findChild<QQuickWheelHandler*>("innerWheelHandler");
    QVERIFY(innerHandler != nullptr);
    QQuickItem *innerRect = innerHandler->parentItem();
    QVERIFY(innerRect != nullptr);
    innerHandler->setOrientation(orientation);
    innerHandler->setInvertible(invertible);
    innerHandler->setRotationScale(rotationScale);
    innerHandler->setProperty(property);
    innerHandler->setTargetScaleMultiplier(targetScaleMultiplier);
    innerHandler->setTargetTransformAroundCursor(targetTransformAroundCursor);
    outerHandler->setOrientation(orientation);
    outerHandler->setInvertible(invertible);
    outerHandler->setRotationScale(rotationScale);
    outerHandler->setProperty(property);
    outerHandler->setTargetScaleMultiplier(targetScaleMultiplier);
    outerHandler->setTargetTransformAroundCursor(targetTransformAroundCursor);
    QSignalSpy innerActiveChangedSpy(innerHandler, SIGNAL(activeChanged()));
    QSignalSpy outerActiveChangedSpy(outerHandler, SIGNAL(activeChanged()));

    if (eventPhases)
        sendWheelEvent(window, eventPos, QPoint(), QPoint(), eventModifiers, Qt::ScrollBegin, eventInverted);
    for (int i = 0; i < eventCount; ++i)
        sendWheelEvent(window, eventPos, eventAngleDelta, eventPixelDelta, eventModifiers,
                       (eventPhases ? Qt::ScrollUpdate : Qt::NoScrollPhase), eventInverted);
    QCOMPARE(innerRect->position().toPoint(), innerPosition);

    /*
        If outer is activated, maybe inner should be deactivated? But the event
        doesn't get delivered to inner anymore, so it doesn't find out that
        it's no longer getting events. It will get deactivated after the
        timeout, just as if the user stopped scrolling.

        This situation is similar to QTBUG-50199, but it's questionable whether
        that was really so important. So far in Qt Quick, if you move the mouse
        while wheel momentum continues, or if the item moves out from under the
        mouse, a different item starts getting the events immediately. In
        non-Qt applications on most OSes, that's quite normal.
    */
    // QCOMPARE(innerActiveChangedSpy.count(), 2);
    // QCOMPARE(innerHandler->active(), false);
    QCOMPARE(innerRect->scale(), innerScale);
    QCOMPARE(innerRect->rotation(), innerRotation);
    QCOMPARE(outerRect->position().toPoint(), outerPosition);
    QCOMPARE(outerActiveChangedSpy.count(), 1);
    QCOMPARE(outerHandler->active(), true);
    QCOMPARE(outerRect->scale(), outerScale);
    QCOMPARE(outerRect->rotation(), outerRotation);
    if (!eventPhases) {
        QTRY_COMPARE(outerHandler->active(), false);
        QCOMPARE(outerActiveChangedSpy.count(), 2);
    }
}

QTEST_MAIN(tst_QQuickWheelHandler)

#include "tst_qquickwheelhandler.moc"
