// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>
#include <QtTest/qsignalspy.h>

#include <QtQuick/qquickwindow.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickpointerdevicehandler_p.h>
#include <QtQuick/private/qquicktaphandler_p.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQuickTemplates2/private/qquickapplicationwindow_p.h>
#include <QtQuickTemplates2/private/qquickoverlay_p.h>

using namespace QQuickControlsTestUtils;
using namespace QQuickVisualTestUtils;

class tst_QQuickOverlay : public QQmlDataTest
{
    Q_OBJECT

public:
    tst_QQuickOverlay();

private slots:
    void clearGrabbers();
    void retainOrientation();
    void pressedAndReleased();
};

class TestInputHandler : public QQuickPointerDeviceHandler
{
public:
    TestInputHandler(QQuickItem * parent)
        : QQuickPointerDeviceHandler(parent)
    {
        setGrabPermissions(GrabPermission::TakeOverForbidden);
    }

    bool wantsPointerEvent(QPointerEvent * event) override
    {
        return QQuickPointerDeviceHandler::wantsPointerEvent(event)
               && !eligiblePoints(event).isEmpty();
    }

    bool wantsEventPoint(const QPointerEvent * event, const QEventPoint & point) override
    {
        bool ret = QQuickPointerDeviceHandler::wantsEventPoint(event, point);
        return ret || (ret && event->exclusiveGrabber(point) != this);
    }

    bool grabPoints(QPointerEvent * event, const QVector<QEventPoint> & points)
    {
        if (points.isEmpty())
            return false;

        bool allowed = true;

        for (const auto & point : points) {
            if (event->exclusiveGrabber(point) != this && !canGrab(event, point)) {
                allowed = false;
                break;
            }
        }

        if (allowed) {
            for (auto point : points) {
                if (event->exclusiveGrabber(point) != this)
                    event->setExclusiveGrabber(point, this);
                point.setAccepted(true);
            }
        }

        return allowed;
    }

    bool grabEligiblePoints(QPointerEvent * event)
    {
        return grabPoints(event, eligiblePoints(event));
    }

    QList<QEventPoint> eligiblePoints(QPointerEvent * event)
    {
        QList<QEventPoint> ret;

        // elligible points should return all points which either
        // 1. have no owner
        // 2. are owned by us
        // 3. we can steal

        for (int i = 0; i < event->pointCount(); ++i) {
            auto p = event->point(i);
            auto * exclusiveGrabber = event->exclusiveGrabber(p);

            if (exclusiveGrabber && exclusiveGrabber != this && !canGrab(event, p))
                continue;

            if (p.state() != QEventPoint::Released && wantsEventPoint(event, p))
                ret << p;
        }

        return ret;
    }

    void handlePointerEventImpl(QPointerEvent * event) override
    {
        QQuickPointerDeviceHandler::handlePointerEventImpl(event);
        auto grabbed = grabEligiblePoints(event);

        if (!active()) {
            setActive(grabbed);
            return;
        }

        for (int i = 0; i < event->pointCount(); ++i) {
            auto point = event->point(i);

            if (event->exclusiveGrabber(point) == this)
                Q_ASSERT(m_points.contains(point.id()));
        }
    }

    void onGrabChanged(QQuickPointerHandler * grabber,
                        QPointingDevice::GrabTransition transition,
                        QPointerEvent * event,
                        QEventPoint & point) override
    {
        if (grabber == this) {
            switch (transition) {
            case QPointingDevice::GrabTransition::GrabExclusive:
                Q_ASSERT(!m_points.contains(point.id()));
                m_points.append(point.id());
                break;

            case QPointingDevice::GrabTransition::UngrabExclusive:
            case QPointingDevice::GrabTransition::CancelGrabExclusive:
                if (m_points.contains(point.id()))
                    m_points.removeOne(point.id());

                break;

            default:
                break;
            }
        }

        QQuickPointerDeviceHandler::onGrabChanged(grabber, transition, event, point);
    }

    QList<int> m_points;
};

tst_QQuickOverlay::tst_QQuickOverlay()
    : QQmlDataTest(QT_QMLTEST_DATADIR, FailOnWarningsPolicy::FailOnWarnings)
{
}

void tst_QQuickOverlay::clearGrabbers()
{
    QQuickWindow window;

    auto *overlay = QQuickOverlay::overlay(&window);
    auto *overlayItem = new QQuickItem(overlay);
    QVERIFY(overlayItem);

    const auto size = QSize(640, 480);
    window.resize(size);

    auto *item = new QQuickItem(window.contentItem());
    item->setSize(size);

    auto testPointerhandler = TestInputHandler(item);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    auto *dev = QTest::createTouchDevice();

    QTest::touchEvent(&window, dev)
        .press(0, QPoint(10, 10));

    QCOMPARE(testPointerhandler.m_points.size(), 1);
    QCOMPARE(testPointerhandler.m_points[0], 0);

    QTest::touchEvent(&window, dev)
        .stationary(0)
        .press(1, QPoint(20, 20));

    QCOMPARE(testPointerhandler.m_points.size(), 2);
    QCOMPARE(testPointerhandler.m_points[0], 0);
    QCOMPARE(testPointerhandler.m_points[1], 1);

    QTest::touchEvent(&window, dev)
        .move(0, QPoint(20, 20))
        .move(1, QPoint(30, 30));

    QCOMPARE(testPointerhandler.m_points.size(), 2);
    QCOMPARE(testPointerhandler.m_points[0], 0);
    QCOMPARE(testPointerhandler.m_points[1], 1);

    QTest::touchEvent(&window, dev)
        .release(0, QPoint(30, 30))
        .stationary(1);

    QCOMPARE(testPointerhandler.m_points.size(), 1);
    QCOMPARE(testPointerhandler.m_points[0], 1);

    QTest::touchEvent(&window, dev)
        .release(1, QPoint(40, 40));

    QVERIFY(testPointerhandler.m_points.isEmpty());
}

void tst_QQuickOverlay::retainOrientation()
{
    QQuickWindow window;
    auto *overlay = QQuickOverlay::overlay(&window);

    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    qreal rot = 10;
    overlay->setRotation(rot);
    const QSizeF sz = overlay->size();
    window.resize(window.size() + QSize(10, 10));
    // wait for the resize event to call QQuickOverlay::updateGeometry
    QTRY_COMPARE_NE(overlay->size(), sz);

    QCOMPARE(overlay->rotation(), rot);
}

void tst_QQuickOverlay::pressedAndReleased()
{
    QQuickControlsApplicationHelper helper(this, "tapHandler.qml");
    QVERIFY2(helper.ready, helper.failureMessage());
    QQuickApplicationWindow *window = helper.appWindow;
    window->show();
    QVERIFY(QTest::qWaitForWindowExposed(window));

    auto *overlay = QQuickOverlay::overlay(window);
    QVERIFY(overlay);
    const QSignalSpy pressedSpy(overlay, SIGNAL(pressed()));
    QVERIFY(pressedSpy.isValid());
    const QSignalSpy releasedSpy(overlay, SIGNAL(released()));
    QVERIFY(releasedSpy.isValid());

    auto *tapHandler = window->findChild<QQuickTapHandler *>();
    QVERIFY(tapHandler);
    const QSignalSpy tappedSpy(tapHandler, SIGNAL(tapped(QEventPoint,Qt::MouseButton)));
    QVERIFY(tappedSpy.isValid());

    // Left click should cause pressed to be emitted.
    const QPoint &windowCenter = mapCenterToWindow(window->contentItem());
    QTest::mousePress(window, Qt::LeftButton, Qt::NoModifier, windowCenter);
    QCOMPARE(pressedSpy.count(), 1);
    QTest::mouseRelease(window, Qt::LeftButton, Qt::NoModifier, windowCenter);
    QCOMPARE(releasedSpy.count(), 1);
    QCOMPARE(tappedSpy.count(), 1);

    // Right press shouldn't cause pressed and released to be emitted
    // now that we need that to synthesize QContextMenuEvents for ContextMenu.
    QTest::mousePress(window, Qt::RightButton, Qt::NoModifier, windowCenter);
    QCOMPARE(pressedSpy.count(), 1);
    QTest::mouseRelease(window, Qt::RightButton, Qt::NoModifier, windowCenter);
    QCOMPARE(releasedSpy.count(), 1);
    QCOMPARE(tappedSpy.count(), 1);
}

QTEST_MAIN(tst_QQuickOverlay)

#include "tst_qquickoverlay.moc"
