// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <QtQuick/private/qquickdrag_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickmousearea_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <private/qquickflickable_p.h>
#include <QtQuick/qquickview.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>
#include <QtQuickTestUtils/private/qmlutils_p.h>
#include <QtQuickTestUtils/private/viewtestutils_p.h>
#include <QtGui/qstylehints.h>
#include <QtGui/QCursor>
#include <QtGui/QScreen>
#include <QEvent>
#include <QQmlComponent>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qwindowsysteminterface_p.h>

Q_LOGGING_CATEGORY(lcTests, "qt.quick.tests")

class CircleMask : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal radius READ radius WRITE setRadius NOTIFY radiusChanged)

public:
    virtual ~CircleMask() {}
    qreal radius() const { return m_radius; }
    void setRadius(qreal radius)
    {
        if (m_radius == radius)
            return;
        m_radius = radius;
        emit radiusChanged();
    }

    Q_INVOKABLE bool contains(const QPointF &point) const
    {
        QPointF center(m_radius, m_radius);
        QLineF line(center, point);
        return line.length() <= m_radius;
    }

signals:
    void radiusChanged();

private:
    qreal m_radius;
};

class EventSender : public QObject {
    Q_OBJECT

public:
    Q_INVOKABLE void sendMouseClick(QWindow* w ,qreal x , qreal y) {
        QTest::mouseClick(w, Qt::LeftButton, {}, QPointF(x, y).toPoint());
    }
};

class tst_QQuickMouseArea: public QQmlDataTest
{
    Q_OBJECT
public:
    tst_QQuickMouseArea()
        : QQmlDataTest(QT_QMLTEST_DATADIR)
    {
        qmlRegisterType<CircleMask>("Test", 1, 0, "CircleMask");
        qmlRegisterType<EventSender>("Test", 1, 0, "EventSender");
    }

private slots:
    void dragProperties();
    void resetDrag();
    void dragging_data() { acceptedButton_data(); }
    void dragging();
    void selfDrag();
    void dragSmoothed();
    void dragThreshold_data();
    void dragThreshold();
    void invalidDrag_data() { rejectedButton_data(); }
    void invalidDrag();
    void cancelDragging();
    void availableDistanceLessThanDragThreshold();
    void setDragOnPressed();
    void updateMouseAreaPosOnClick();
    void updateMouseAreaPosOnResize();
    void noOnClickedWithPressAndHold();
    void onMousePressRejected();
    void pressedCanceledOnWindowDeactivate_data();
    void pressedCanceledOnWindowDeactivate();
    void doubleClick_data() { acceptedButton_data(); }
    void doubleClick();
    void doubleTap();
    void clickTwice_data() { acceptedButton_data(); }
    void clickTwice();
    void invalidClick_data() { rejectedButton_data(); }
    void invalidClick();
    void pressedOrdering();
    void preventStealing();
    void preventStealingListViewChild();
    void clickThrough();
    void hoverPosition();
    void hoverPropagation();
    void hoverVisible();
    void hoverAfterPress();
    void subtreeHoverEnabled();
    void hoverWhenDisabled();
    void disableAfterPress();
    void disableParentOnPress_data();
    void disableParentOnPress();
    void onWheel();
    void transformedMouseArea_data();
    void transformedMouseArea();
    void pressedMultipleButtons_data();
    void pressedMultipleButtons();
    void changeAxis();
#if QT_CONFIG(cursor)
    void cursorShape();
#endif
    void moveAndReleaseWithoutPress();
    void nestedStopAtBounds();
    void nestedStopAtBounds_data();
    void nestedFlickableStopAtBounds();
    void containsPress_data();
    void containsPress();
    void ignoreBySource();
    void notPressedAfterStolenGrab();
    void pressAndHold_data();
    void pressAndHold();
    void pressOneAndTapAnother_data();
    void pressOneAndTapAnother();
    void mask();
    void nestedEventDelivery();
    void settingHiddenInPressUngrabs();
    void negativeZStackingOrder();
    void containsMouseAndVisibility();
    void containsMouseAndVisibilityMasked();
    void containsMouseAndHoverDisabled();
    void doubleClickToHide();
    void releaseFirstTouchAfterSecond();
#if QT_CONFIG(tabletevent)
    void tabletStylusTap();
#endif
    void syntheticRightClick();

private:
    int startDragDistance() const {
        return QGuiApplication::styleHints()->startDragDistance();
    }
    void acceptedButton_data();
    void rejectedButton_data();
    QPointingDevice *device = QTest::createTouchDevice(); // TODO const after fixing QTBUG-107864
};

Q_DECLARE_METATYPE(Qt::MouseButton)
Q_DECLARE_METATYPE(Qt::MouseButtons)

void tst_QQuickMouseArea::acceptedButton_data()
{
    QTest::addColumn<Qt::MouseButtons>("acceptedButtons");
    QTest::addColumn<Qt::MouseButton>("button");

    QTest::newRow("left") << Qt::MouseButtons(Qt::LeftButton) << Qt::LeftButton;
    QTest::newRow("right") << Qt::MouseButtons(Qt::RightButton) << Qt::RightButton;
    QTest::newRow("middle") << Qt::MouseButtons(Qt::MiddleButton) << Qt::MiddleButton;

    QTest::newRow("left (left|right)") << Qt::MouseButtons(Qt::LeftButton | Qt::RightButton) << Qt::LeftButton;
    QTest::newRow("right (right|middle)") << Qt::MouseButtons(Qt::RightButton | Qt::MiddleButton) << Qt::RightButton;
    QTest::newRow("middle (left|middle)") << Qt::MouseButtons(Qt::LeftButton | Qt::MiddleButton) << Qt::MiddleButton;
}

void tst_QQuickMouseArea::rejectedButton_data()
{
    QTest::addColumn<Qt::MouseButtons>("acceptedButtons");
    QTest::addColumn<Qt::MouseButton>("button");

    QTest::newRow("middle (left|right)") << Qt::MouseButtons(Qt::LeftButton | Qt::RightButton) << Qt::MiddleButton;
    QTest::newRow("left (right|middle)") << Qt::MouseButtons(Qt::RightButton | Qt::MiddleButton) << Qt::LeftButton;
    QTest::newRow("right (left|middle)") << Qt::MouseButtons(Qt::LeftButton | Qt::MiddleButton) << Qt::RightButton;
}

void tst_QQuickMouseArea::dragProperties()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("dragproperties.qml")));

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(rootItem != nullptr);
    QSignalSpy targetSpy(drag, SIGNAL(targetChanged()));
    drag->setTarget(rootItem);
    QCOMPARE(targetSpy.size(),1);
    drag->setTarget(rootItem);
    QCOMPARE(targetSpy.size(),1);

    // axis
    QCOMPARE(drag->axis(), QQuickDrag::XAndYAxis);
    QSignalSpy axisSpy(drag, SIGNAL(axisChanged()));
    drag->setAxis(QQuickDrag::XAxis);
    QCOMPARE(drag->axis(), QQuickDrag::XAxis);
    QCOMPARE(axisSpy.size(),1);
    drag->setAxis(QQuickDrag::XAxis);
    QCOMPARE(axisSpy.size(),1);

    // minimum and maximum properties
    QSignalSpy xminSpy(drag, SIGNAL(minimumXChanged()));
    QSignalSpy xmaxSpy(drag, SIGNAL(maximumXChanged()));
    QSignalSpy yminSpy(drag, SIGNAL(minimumYChanged()));
    QSignalSpy ymaxSpy(drag, SIGNAL(maximumYChanged()));

    QCOMPARE(drag->xmin(), 0.0);
    QCOMPARE(drag->xmax(), rootItem->width()-blackRect->width());
    QCOMPARE(drag->ymin(), 0.0);
    QCOMPARE(drag->ymax(), rootItem->height()-blackRect->height());

    drag->setXmin(10);
    drag->setXmax(10);
    drag->setYmin(10);
    drag->setYmax(10);

    QCOMPARE(drag->xmin(), 10.0);
    QCOMPARE(drag->xmax(), 10.0);
    QCOMPARE(drag->ymin(), 10.0);
    QCOMPARE(drag->ymax(), 10.0);

    QCOMPARE(xminSpy.size(),1);
    QCOMPARE(xmaxSpy.size(),1);
    QCOMPARE(yminSpy.size(),1);
    QCOMPARE(ymaxSpy.size(),1);

    drag->setXmin(10);
    drag->setXmax(10);
    drag->setYmin(10);
    drag->setYmax(10);

    QCOMPARE(xminSpy.size(),1);
    QCOMPARE(xmaxSpy.size(),1);
    QCOMPARE(yminSpy.size(),1);
    QCOMPARE(ymaxSpy.size(),1);

    // filterChildren
    QSignalSpy filterChildrenSpy(drag, SIGNAL(filterChildrenChanged()));

    drag->setFilterChildren(true);

    QVERIFY(drag->filterChildren());
    QCOMPARE(filterChildrenSpy.size(), 1);

    drag->setFilterChildren(true);
    QCOMPARE(filterChildrenSpy.size(), 1);

    // threshold
    QCOMPARE(int(drag->threshold()), qApp->styleHints()->startDragDistance());
    QSignalSpy thresholdSpy(drag, SIGNAL(thresholdChanged()));
    drag->setThreshold(0.0);
    QCOMPARE(drag->threshold(), 0.0);
    QCOMPARE(thresholdSpy.size(), 1);
    drag->setThreshold(99);
    QCOMPARE(thresholdSpy.size(), 2);
    drag->setThreshold(99);
    QCOMPARE(thresholdSpy.size(), 2);
    drag->resetThreshold();
    QCOMPARE(int(drag->threshold()), qApp->styleHints()->startDragDistance());
    QCOMPARE(thresholdSpy.size(), 3);
}

void tst_QQuickMouseArea::resetDrag()
{
    QQuickView window;
    window.setInitialProperties({{"haveTarget", true}});
    QVERIFY(QQuickTest::showView(window, testFileUrl("dragreset.qml")));

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(rootItem != nullptr);
    QSignalSpy targetSpy(drag, SIGNAL(targetChanged()));
    QVERIFY(drag->target() != nullptr);
    auto root = window.rootObject();
    QQmlProperty haveTarget {root, "haveTarget"};
    haveTarget.write(false);
    QCOMPARE(targetSpy.size(),1);
    QVERIFY(!drag->target());
}

void tst_QQuickMouseArea::dragging()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("dragging.qml")));

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    mouseRegion->setAcceptedButtons(acceptedButtons);

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());

    QVERIFY(!drag->active());

    QPoint p = QPoint(100,100);
    QTest::mousePress(&window, button, Qt::NoModifier, p);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.
    // The item is moved relative to the position of the mouse when the drag
    // was triggered, this prevents a sudden change in position when the drag
    // threshold is exceeded.

    int dragThreshold = QGuiApplication::styleHints()->startDragDistance();

    // move the minimum distance to activate drag
    p += QPoint(dragThreshold + 1, dragThreshold + 1);
    QTest::mouseMove(&window, p);
    QVERIFY(!drag->active());

    // from here on move the item
    p += QPoint(1, 1);
    QTest::mouseMove(&window, p);
    QTRY_VERIFY(drag->active());
    // on macOS the cursor movement is going through a native event which
    // means that it can actually take some time to show
    QTRY_COMPARE(blackRect->x(), 50.0 + 1);
    QCOMPARE(blackRect->y(), 50.0 + 1);

    p += QPoint(10, 10);
    QTest::mouseMove(&window, p);
    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    qreal relativeX = mouseRegion->mouseX();
    qreal relativeY = mouseRegion->mouseY();
    for (int i = 0; i < 20; i++) {
        p += QPoint(1, 1);
        QTest::mouseMove(&window, p);
        QTRY_COMPARE(mouseRegion->mouseX(), relativeX);
        QCOMPARE(mouseRegion->mouseY(), relativeY);
    }
    QVERIFY(drag->active());

    QTest::mouseRelease(&window, button, Qt::NoModifier, p);
    QTRY_VERIFY(!drag->active());
    QTRY_COMPARE(blackRect->x(), 81.0);
    QCOMPARE(blackRect->y(), 81.0);
}

void tst_QQuickMouseArea::selfDrag() // QTBUG-85111
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("selfDrag.qml")));

    QQuickMouseArea *ma = window.rootObject()->findChild<QQuickMouseArea*>("ma");
    QVERIFY(ma != nullptr);
    QQuickDrag *drag = ma->drag();
    QVERIFY(drag != nullptr);
    QCOMPARE(ma, drag->target());

    QQuickItem *fillRect = window.rootObject()->findChild<QQuickItem*>("fill");
    QVERIFY(fillRect != nullptr);

    QVERIFY(!drag->active());

    QPoint p = QPoint(100,100);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);

    QVERIFY(!drag->active());
    QCOMPARE(ma->x(), 0);
    QCOMPARE(ma->y(), 0);

    int dragThreshold = QGuiApplication::styleHints()->startDragDistance();

    // First move event triggers drag, second is acted upon.
    // move the minimum distance to activate drag
    p += QPoint(dragThreshold + 1, dragThreshold + 1);
    QTest::mouseMove(&window, p);
    QVERIFY(!drag->active());

    // from here on move the item
    p += QPoint(1, 1);
    QTest::mouseMove(&window, p);
    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(ma->x(), 1);
    QCOMPARE(ma->y(), 1);

    p += QPoint(10, 10);
    QTest::mouseMove(&window, p);
    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(ma->x(), 11);
    QCOMPARE(ma->y(), 11);

    qreal relativeX = ma->mouseX();
    qreal relativeY = ma->mouseY();
    for (int i = 0; i < 20; i++) {
        p += QPoint(1, 1);
        QTest::mouseMove(&window, p);
        QVERIFY(drag->active());
        QTRY_COMPARE(ma->mouseX(), relativeX);
        QCOMPARE(ma->mouseY(), relativeY);
    }

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);
    QTRY_VERIFY(!drag->active());
    QTRY_COMPARE(ma->x(), 31);
    QCOMPARE(ma->y(), 31);
}

void tst_QQuickMouseArea::dragSmoothed()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("dragging.qml")));

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    drag->setThreshold(5);

    mouseRegion->setAcceptedButtons(Qt::LeftButton);
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());
    QVERIFY(!drag->active());
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 102), 50);
    QTest::mouseMove(&window, QPoint(100, 106), 50);
    QTest::mouseMove(&window, QPoint(100, 122), 50);
    QTRY_COMPARE(blackRect->x(), 50.0);
    QTRY_COMPARE(blackRect->y(), 66.0);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,122));

    // reset rect position
    blackRect->setX(50.0);
    blackRect->setY(50.0);

    // now try with smoothed disabled
    drag->setSmoothed(false);
    QVERIFY(!drag->active());
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 102), 50);
    QTest::mouseMove(&window, QPoint(100, 106), 50);
    QTest::mouseMove(&window, QPoint(100, 122), 50);
    QTRY_COMPARE(blackRect->x(), 50.0);
    QTRY_COMPARE(blackRect->y(), 72.0);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 122));
}

void tst_QQuickMouseArea::dragThreshold_data()
{
    QTest::addColumn<bool>("preventStealing");
    QTest::newRow("without preventStealing") << false;
    QTest::newRow("with preventStealing") << true;
}

void tst_QQuickMouseArea::dragThreshold()
{
    QFETCH(bool, preventStealing);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("dragging.qml")));

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    mouseRegion->setPreventStealing(preventStealing);
    QQuickDrag *drag = mouseRegion->drag();

    drag->setThreshold(5);

    mouseRegion->setAcceptedButtons(Qt::LeftButton);
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());
    QVERIFY(!drag->active());
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);
    QTest::mouseMove(&window, QPoint(100, 102), 50);
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 100), 50);
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 104), 50);
    QTest::mouseMove(&window, QPoint(100, 105), 50);
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 106), 50);
    QTest::mouseMove(&window, QPoint(100, 108), 50);
    QVERIFY(drag->active());
    QTest::mouseMove(&window, QPoint(100, 116), 50);
    QTest::mouseMove(&window, QPoint(100, 122), 50);
    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(blackRect->x(), 50.0);
    QTRY_COMPARE(blackRect->y(), 66.0);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(122,122));
    QTRY_VERIFY(!drag->active());

    // Immediate drag threshold
    drag->setThreshold(0);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::mouseMove(&window, QPoint(100, 122), 50);
    QVERIFY(!drag->active());
    QTest::mouseMove(&window, QPoint(100, 123), 50);
    QVERIFY(drag->active());
    QTest::mouseMove(&window, QPoint(100, 124), 50);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 124));
    QTRY_VERIFY(!drag->active());
    drag->resetThreshold();
}
void tst_QQuickMouseArea::invalidDrag()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("dragging.qml")));

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    mouseRegion->setAcceptedButtons(acceptedButtons);

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());

    QVERIFY(!drag->active());

    QTest::mousePress(&window, button, Qt::NoModifier, QPoint(100,100));

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.

    QTest::mouseMove(&window, QPoint(111,111));
    QTest::qWait(50);
    QTest::mouseMove(&window, QPoint(122,122));
    QTest::qWait(50);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    QTest::mouseRelease(&window, button, Qt::NoModifier, QPoint(122,122));
    QTest::qWait(50);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);
}

void tst_QQuickMouseArea::cancelDragging()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("dragging.qml")));

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    mouseRegion->setAcceptedButtons(Qt::LeftButton);

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());

    QVERIFY(!drag->active());

    QPoint p = QPoint(100,100);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    p += QPoint(startDragDistance() + 1, 0);
    QTest::mouseMove(&window, p);

    p += QPoint(11, 11);
    QTest::mouseMove(&window, p);

    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    mouseRegion->QQuickItem::ungrabMouse();
    QTRY_VERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    QTest::mouseMove(&window, QPoint(132,132), 50);
    QTRY_VERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(122,122));
}

// QTBUG-58347
void tst_QQuickMouseArea::availableDistanceLessThanDragThreshold()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("availableDistanceLessThanDragThreshold.qml")));

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea);

    QPoint position(100, 100);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    position.setX(301);
    QTest::mouseMove(&window, position);
    position.setX(501);
    QTest::mouseMove(&window, position);
    QVERIFY(mouseArea->drag()->active());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);

    QVERIFY(!mouseArea->drag()->active());
    QCOMPARE(mouseArea->x(), 200.0);
}

void tst_QQuickMouseArea::setDragOnPressed()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("setDragOnPressed.qml")));

    QQuickMouseArea *mouseArea = qobject_cast<QQuickMouseArea *>(window.rootObject());
    QVERIFY(mouseArea);

    // target
    QQuickItem *target = mouseArea->findChild<QQuickItem*>("target");
    QVERIFY(target);

    QPoint p = QPoint(100, 100);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);

    QQuickDrag *drag = mouseArea->drag();
    QVERIFY(drag);
    QVERIFY(!drag->active());

    QCOMPARE(target->x(), 50.0);
    QCOMPARE(target->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.

    p += QPoint(startDragDistance() + 1, 0);
    QTest::mouseMove(&window, p);

    p += QPoint(11, 0);
    QTest::mouseMove(&window, p);
    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(target->x(), 61.0);
    QCOMPARE(target->y(), 50.0);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);
    QTRY_VERIFY(!drag->active());
    QCOMPARE(target->x(), 61.0);
    QCOMPARE(target->y(), 50.0);
}

void tst_QQuickMouseArea::updateMouseAreaPosOnClick()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("updateMousePosOnClick.qml")));

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QVERIFY(mouseRegion != nullptr);

    QQuickRectangle *rect = window.rootObject()->findChild<QQuickRectangle*>("ball");
    QVERIFY(rect != nullptr);

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});

    QCOMPARE(mouseRegion->mouseX(), 100.0);
    QCOMPARE(mouseRegion->mouseY(), 100.0);

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());
}

void tst_QQuickMouseArea::updateMouseAreaPosOnResize()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("updateMousePosOnResize.qml")));

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QVERIFY(mouseRegion != nullptr);

    QQuickRectangle *rect = window.rootObject()->findChild<QQuickRectangle*>("brother");
    QVERIFY(rect != nullptr);

    QCOMPARE(mouseRegion->mouseX(), 0.0);
    QCOMPARE(mouseRegion->mouseY(), 0.0);

    QMouseEvent event(QEvent::MouseButtonPress, rect->position().toPoint(),
                      window.mapToGlobal(rect->position().toPoint()),
                      Qt::LeftButton, Qt::LeftButton, {});
    QGuiApplication::sendEvent(&window, &event);

    QVERIFY(!mouseRegion->property("emitPositionChanged").toBool());
    QVERIFY(mouseRegion->property("mouseMatchesPos").toBool());

    QCOMPARE(mouseRegion->property("x1").toReal(), 0.0);
    QCOMPARE(mouseRegion->property("y1").toReal(), 0.0);

    QCOMPARE(mouseRegion->property("x2").toReal(), rect->x());
    QCOMPARE(mouseRegion->property("y2").toReal(), rect->y());

    QCOMPARE(mouseRegion->mouseX(), rect->x());
    QCOMPARE(mouseRegion->mouseY(), rect->y());
}

void tst_QQuickMouseArea::noOnClickedWithPressAndHold()
{
    {
        // We handle onPressAndHold, therefore no onClicked
        QQuickView window;
        QVERIFY(QQuickTest::showView(window, testFileUrl("clickandhold.qml")));
        QQuickMouseArea *mouseArea = qobject_cast<QQuickMouseArea*>(window.rootObject()->children().first());
        QVERIFY(mouseArea);

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});

        QCOMPARE(mouseArea->pressedButtons(), Qt::LeftButton);
        QVERIFY(!window.rootObject()->property("clicked").toBool());
        QVERIFY(!window.rootObject()->property("held").toBool());

        // timeout is 800 (in qquickmousearea.cpp)
        QTest::qWait(1000);
        QCoreApplication::processEvents();

        QVERIFY(!window.rootObject()->property("clicked").toBool());
        QVERIFY(window.rootObject()->property("held").toBool());

        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});

        QTRY_VERIFY(window.rootObject()->property("held").toBool());
        QVERIFY(!window.rootObject()->property("clicked").toBool());
    }

    {
        // We do not handle onPressAndHold, therefore we get onClicked
        QQuickView window;
        QVERIFY(QQuickTest::showView(window, testFileUrl("noclickandhold.qml")));

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});

        QVERIFY(!window.rootObject()->property("clicked").toBool());

        QTest::qWait(1000);

        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});

        QVERIFY(window.rootObject()->property("clicked").toBool());
    }
}

void tst_QQuickMouseArea::onMousePressRejected()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("rejectEvent.qml")));
    QVERIFY(window.rootObject()->property("enabled").toBool());

    QVERIFY(!window.rootObject()->property("mr1_pressed").toBool());
    QVERIFY(!window.rootObject()->property("mr1_released").toBool());
    QVERIFY(!window.rootObject()->property("mr1_canceled").toBool());
    QVERIFY(!window.rootObject()->property("mr2_pressed").toBool());
    QVERIFY(!window.rootObject()->property("mr2_released").toBool());
    QVERIFY(!window.rootObject()->property("mr2_canceled").toBool());

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});

    QVERIFY(window.rootObject()->property("mr1_pressed").toBool());
    QVERIFY(!window.rootObject()->property("mr1_released").toBool());
    QVERIFY(!window.rootObject()->property("mr1_canceled").toBool());
    QVERIFY(window.rootObject()->property("mr2_pressed").toBool());
    QVERIFY(!window.rootObject()->property("mr2_released").toBool());
    QVERIFY(!window.rootObject()->property("mr2_canceled").toBool());

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});

    QTRY_VERIFY(window.rootObject()->property("mr1_released").toBool());
    QVERIFY(!window.rootObject()->property("mr1_canceled").toBool());
    QVERIFY(!window.rootObject()->property("mr2_released").toBool());
}

void tst_QQuickMouseArea::pressedCanceledOnWindowDeactivate_data()
{
    QTest::addColumn<bool>("doubleClick");
    QTest::newRow("simple click") << false;
    QTest::newRow("double click") << true;
}


void tst_QQuickMouseArea::pressedCanceledOnWindowDeactivate()
{
    QFETCH(bool, doubleClick);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pressedCanceled.qml")));

    QVERIFY(!window.rootObject()->property("pressed").toBool());
    QVERIFY(!window.rootObject()->property("canceled").toBool());

    int expectedRelease = 0;
    int expectedClicks = 0;
    QCOMPARE(window.rootObject()->property("released").toInt(), expectedRelease);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), expectedClicks);

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100),
                           window.mapToGlobal(QPoint(100, 100)), Qt::LeftButton, Qt::LeftButton, {});
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100),
                             window.mapToGlobal(QPoint(100, 100)), Qt::LeftButton, Qt::LeftButton, {});

    QGuiApplication::sendEvent(&window, &pressEvent);

    QTRY_VERIFY(window.rootObject()->property("pressed").toBool());
    QVERIFY(!window.rootObject()->property("canceled").toBool());
    QCOMPARE(window.rootObject()->property("released").toInt(), expectedRelease);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), expectedClicks);

    if (doubleClick) {
        QGuiApplication::sendEvent(&window, &releaseEvent);
        QTRY_VERIFY(!window.rootObject()->property("pressed").toBool());
        QVERIFY(!window.rootObject()->property("canceled").toBool());
        QCOMPARE(window.rootObject()->property("released").toInt(), ++expectedRelease);
        QCOMPARE(window.rootObject()->property("clicked").toInt(), ++expectedClicks);

        QGuiApplication::sendEvent(&window, &pressEvent);
        QMouseEvent pressEvent2(QEvent::MouseButtonDblClick, QPoint(100, 100),
                                window.mapToGlobal(QPoint(100, 100)), Qt::LeftButton, Qt::LeftButton, {});
        QGuiApplication::sendEvent(&window, &pressEvent2);

        QTRY_VERIFY(window.rootObject()->property("pressed").toBool());
        QVERIFY(!window.rootObject()->property("canceled").toBool());
        QCOMPARE(window.rootObject()->property("released").toInt(), expectedRelease);
        QCOMPARE(window.rootObject()->property("clicked").toInt(), expectedClicks);
        QCOMPARE(window.rootObject()->property("doubleClicked").toInt(), 1);
    }

    QEvent windowDeactivateEvent(QEvent::WindowDeactivate);
    QGuiApplication::sendEvent(&window, &windowDeactivateEvent);
    QTRY_VERIFY(!window.rootObject()->property("pressed").toBool());
    QVERIFY(window.rootObject()->property("canceled").toBool());
    QCOMPARE(window.rootObject()->property("released").toInt(), expectedRelease);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), expectedClicks);

    //press again
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});
    QTRY_VERIFY(window.rootObject()->property("pressed").toBool());
    QVERIFY(!window.rootObject()->property("canceled").toBool());
    QCOMPARE(window.rootObject()->property("released").toInt(), expectedRelease);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), expectedClicks);

    //release
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});
    QTRY_VERIFY(!window.rootObject()->property("pressed").toBool());
    QVERIFY(!window.rootObject()->property("canceled").toBool());
    QCOMPARE(window.rootObject()->property("released").toInt(), ++expectedRelease);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), ++expectedClicks);
}

void tst_QQuickMouseArea::doubleClick()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("doubleclick.qml")));

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea);
    mouseArea->setAcceptedButtons(acceptedButtons);

    // The sequence for a double click is:
    // press, release, (click), press, double click, release
    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100),
                           window.mapToGlobal(QPoint(100, 100)), button, button, {});
    QGuiApplication::sendEvent(&window, &pressEvent);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100),
                             window.mapToGlobal(QPoint(100, 100)), button, button, {});
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("released").toInt(), 1);

    QGuiApplication::sendEvent(&window, &pressEvent);
    QMouseEvent pressEvent2 = QMouseEvent(QEvent::MouseButtonDblClick, QPoint(100, 100),
                                          window.mapToGlobal(QPoint(100, 100)), button, button, {});
    QGuiApplication::sendEvent(&window, &pressEvent2);
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("clicked").toInt(), 1);
    QCOMPARE(window.rootObject()->property("doubleClicked").toInt(), 1);
    QCOMPARE(window.rootObject()->property("released").toInt(), 2);
}

void tst_QQuickMouseArea::doubleTap() // QTBUG-112434
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("doubleclick.qml")));

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea);
    QPoint p1 = mouseArea->mapToScene(mouseArea->boundingRect().center()).toPoint();

    QTest::touchEvent(&window, device).press(0, p1);
    QQuickTouchUtils::flush(&window);
    QTest::touchEvent(&window, device).release(0, p1);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(window.rootObject()->property("released").toInt(), 1);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 1);

    p1 += QPoint(1, -1); // movement less than QPlatformTheme::TouchDoubleTapDistance
    QTest::touchEvent(&window, device).press(1, p1); // touchpoint ID is different the second time
    QQuickTouchUtils::flush(&window);
    QCOMPARE(mouseArea->isPressed(), true);
    // at this time QQuickDeliveryAgentPrivate::deliverTouchAsMouse() synthesizes the double-click event
    QCOMPARE(window.rootObject()->property("doubleClicked").toInt(), 1);

    QTest::touchEvent(&window, device).release(1, p1);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(window.rootObject()->property("released").toInt(), 2);
    QCOMPARE(mouseArea->isPressed(), false);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 1);

    // now tap with two fingers simultaneously: only one of them generates synth-mouse
    QPoint p2 = p1 + QPoint(50, 5);
    QTest::touchEvent(&window, device).press(2, p1).press(3, p2);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(mouseArea->isPressed(), true);
    QTest::touchEvent(&window, device).release(2, p1).release(3, p2);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(window.rootObject()->property("released").toInt(), 3);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 2);
    QCOMPARE(window.rootObject()->property("doubleClicked").toInt(), 1);
    QCOMPARE(mouseArea->isPressed(), false);

    // tap with two fingers simultaneously again: get another double-click from one point
    p1 -= QPoint(1, -1);
    p2 += QPoint(1, -1);
    QTest::touchEvent(&window, device).press(4, p1).press(5, p2);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(mouseArea->isPressed(), true);
    QTest::touchEvent(&window, device).release(4, p1).release(5, p2);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(window.rootObject()->property("released").toInt(), 4);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 2);
    QCOMPARE(window.rootObject()->property("doubleClicked").toInt(), 2);
    QCOMPARE(mouseArea->isPressed(), false); // make sure it doesn't get stuck
}

// QTBUG-14832
void tst_QQuickMouseArea::clickTwice()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("clicktwice.qml")));

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea);
    mouseArea->setAcceptedButtons(acceptedButtons);

    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100),
                           window.mapToGlobal(QPoint(100, 100)), button, button, {});
    QGuiApplication::sendEvent(&window, &pressEvent);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100),
                             window.mapToGlobal(QPoint(100, 100)), button, button, {});
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("pressed").toInt(), 1);
    QCOMPARE(window.rootObject()->property("released").toInt(), 1);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 1);

    QGuiApplication::sendEvent(&window, &pressEvent);

    QMouseEvent pressEvent2 = QMouseEvent(QEvent::MouseButtonDblClick, QPoint(100, 100),
                                          window.mapToGlobal(QPoint(100, 100)), button, button, {});
    QGuiApplication::sendEvent(&window, &pressEvent2);
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("pressed").toInt(), 2);
    QCOMPARE(window.rootObject()->property("released").toInt(), 2);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 2);
}

void tst_QQuickMouseArea::invalidClick()
{
    QFETCH(Qt::MouseButtons, acceptedButtons);
    QFETCH(Qt::MouseButton, button);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("doubleclick.qml")));

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea);
    mouseArea->setAcceptedButtons(acceptedButtons);

    // The sequence for a double click is:
    // press, release, (click), press, double click, release
    QMouseEvent pressEvent(QEvent::MouseButtonPress, QPoint(100, 100),
                           window.mapToGlobal(QPoint(100, 100)), button, button, {});
    QGuiApplication::sendEvent(&window, &pressEvent);

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint(100, 100),
                             window.mapToGlobal(QPoint(100, 100)), button, button, {});
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("released").toInt(), 0);

    QGuiApplication::sendEvent(&window, &pressEvent);
    QMouseEvent pressEvent2 = QMouseEvent(QEvent::MouseButtonDblClick, QPoint(100, 100),
                                          window.mapToGlobal(QPoint(100, 100)), button, button, {});
    QGuiApplication::sendEvent(&window, &pressEvent2);
    QGuiApplication::sendEvent(&window, &releaseEvent);

    QCOMPARE(window.rootObject()->property("clicked").toInt(), 0);
    QCOMPARE(window.rootObject()->property("doubleClicked").toInt(), 0);
    QCOMPARE(window.rootObject()->property("released").toInt(), 0);
}

void tst_QQuickMouseArea::pressedOrdering()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pressedOrdering.qml")));

    QCOMPARE(window.rootObject()->property("value").toString(), QLatin1String("base"));

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});

    QCOMPARE(window.rootObject()->property("value").toString(), QLatin1String("pressed"));

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});

    QCOMPARE(window.rootObject()->property("value").toString(), QLatin1String("toggled"));

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, {100, 100});

    QCOMPARE(window.rootObject()->property("value").toString(), QLatin1String("pressed"));
}

void tst_QQuickMouseArea::preventStealing()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("preventstealing.qml")));

    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable != nullptr);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea*>("mousearea");
    QVERIFY(mouseArea != nullptr);

    QSignalSpy mousePositionSpy(mouseArea, SIGNAL(positionChanged(QQuickMouseEvent*)));

    QPoint p = QPoint(80, 80);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);

    // Without preventStealing, mouse movement over MouseArea would
    // cause the Flickable to steal mouse and trigger content movement.

    p += QPoint(-startDragDistance() * 2, -startDragDistance() * 2);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);

    // We should have received all four move events
    QTRY_COMPARE(mousePositionSpy.size(), 4);
    mousePositionSpy.clear();
    QVERIFY(mouseArea->isPressed());

    // Flickable content should not have moved.
    QCOMPARE(flickable->contentX(), 0.);
    QCOMPARE(flickable->contentY(), 0.);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);

    // Now allow stealing and confirm Flickable does its thing.
    window.rootObject()->setProperty("stealing", false);

    p = QPoint(80, 80);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);

    // Without preventStealing, mouse movement over MouseArea would
    // cause the Flickable to steal mouse and trigger content movement.

    p += QPoint(-startDragDistance() * 2, -startDragDistance() * 2);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);
    p += QPoint(-10, -10);
    QTest::mouseMove(&window, p);

    // We should only have received the first move event
    QTRY_COMPARE(mousePositionSpy.size(), 1);
    // Our press should be taken away
    QVERIFY(!mouseArea->isPressed());

    // Flickable swallows the first move, then moves 2*10 px
    QTRY_COMPARE(flickable->contentX(), 20.);
    QCOMPARE(flickable->contentY(), 20.);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);
}

// QTBUG-103522
void tst_QQuickMouseArea::preventStealingListViewChild()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("preventStealingListViewChild.qml")));
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable*>(window.rootObject());
    QVERIFY(flickable);
    QQuickMouseArea *mouseArea = flickable->findChild<QQuickMouseArea*>();
    QVERIFY(mouseArea);
    QPoint p = mouseArea->mapToScene(mouseArea->boundingRect().center()).toPoint();
    const int threshold = qApp->styleHints()->startDragDistance();

    flickable->flick(0, -10000);
    for (int i = 0; i < 2; ++i) {
        QVERIFY(flickable->isMovingVertically());
        QTest::touchEvent(&window, device).press(0, p);
        QQuickTouchUtils::flush(&window);
        for (int j = 0; j < 4 && !mouseArea->drag()->active(); ++j) {
            p += QPoint(0, threshold);
            QTest::touchEvent(&window, device).move(0, p);
            QQuickTouchUtils::flush(&window);
        }
        // MouseArea should be dragged because of preventStealing; ListView does not steal the grab.
        QVERIFY(mouseArea->drag()->active());
        QCOMPARE(flickable->isDragging(), false);
        QTest::touchEvent(&window, device).release(0, p);
        QCOMPARE(mouseArea->drag()->active(), false);
    }
}

void tst_QQuickMouseArea::clickThrough()
{
    // timestamp delay to avoid generating a double click
    const int doubleClickInterval = qApp->styleHints()->mouseDoubleClickInterval() + 10;
    {
        QQuickView window;
        QVERIFY(QQuickTest::showView(window, testFileUrl("clickThrough.qml")));
        QQuickItem *root = window.rootObject();
        QVERIFY(root);

        // With no handlers defined, click, doubleClick and PressAndHold should propagate to those with handlers
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

        QTRY_COMPARE(root->property("presses").toInt(), 0);
        QTRY_COMPARE(root->property("clicks").toInt(), 1);

        QCOMPARE(root->property("doubleClicks").toInt(), 0);
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
        QTest::qWait(1000);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

        QTRY_COMPARE(root->property("presses").toInt(), 0);
        QTRY_COMPARE(root->property("clicks").toInt(), 1);
        QTRY_COMPARE(root->property("pressAndHolds").toInt(), 1);

        QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
        QTest::qWait(100);

        QCOMPARE(root->property("presses").toInt(), 0);
        QTRY_COMPARE(root->property("clicks").toInt(), 2);
        QTRY_COMPARE(root->property("doubleClicks").toInt(), 1);
        QCOMPARE(root->property("pressAndHolds").toInt(), 1);
    }
    {
        QQuickView window;
        QVERIFY(QQuickTest::showView(window, testFileUrl("clickThrough2.qml")));
        QQuickItem *root = window.rootObject();
        QVERIFY(root);

        // With handlers defined, click, doubleClick and PressAndHold should propagate only when explicitly ignored
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

        QCOMPARE(root->property("presses").toInt(), 0);
        QCOMPARE(root->property("clicks").toInt(), 0);

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
        QTest::qWait(1000);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
        QTest::qWait(100);

        QCOMPARE(root->property("presses").toInt(), 0);
        QCOMPARE(root->property("clicks").toInt(), 0);
        QCOMPARE(root->property("pressAndHolds").toInt(), 0);

        QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
        QTest::qWait(100);

        QCOMPARE(root->property("presses").toInt(), 0);
        QCOMPARE(root->property("clicks").toInt(), 0);
        QCOMPARE(root->property("doubleClicks").toInt(), 0);
        QCOMPARE(root->property("pressAndHolds").toInt(), 0);

        root->setProperty("letThrough", QVariant(true));

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

        QCOMPARE(root->property("presses").toInt(), 0);
        QTRY_COMPARE(root->property("clicks").toInt(), 1);

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
        QTest::qWait(1000);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
        QTest::qWait(100);

        QCOMPARE(root->property("presses").toInt(), 0);
        QCOMPARE(root->property("clicks").toInt(), 1);
        QCOMPARE(root->property("pressAndHolds").toInt(), 1);

        QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
        QTest::qWait(100);

        QCOMPARE(root->property("presses").toInt(), 0);
        QTRY_COMPARE(root->property("clicks").toInt(), 2);
        QCOMPARE(root->property("doubleClicks").toInt(), 1);
        QCOMPARE(root->property("pressAndHolds").toInt(), 1);

        root->setProperty("noPropagation", QVariant(true));

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
        QTest::qWait(1000);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
        QTest::qWait(100);

        QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
        QTest::qWait(100);

        QCOMPARE(root->property("presses").toInt(), 0);
        QTRY_COMPARE(root->property("clicks").toInt(), 2);
        QCOMPARE(root->property("doubleClicks").toInt(), 1);
        QCOMPARE(root->property("pressAndHolds").toInt(), 1);
    }
    {
        QQuickView window;
        QVERIFY(QQuickTest::showView(window, testFileUrl("qtbug34368.qml")));
        QQuickItem *root = window.rootObject();
        QVERIFY(root);

        // QTBUG-34368 - Shouldn't propagate to disabled mouse areas
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

        QCOMPARE(root->property("clicksEnabled").toInt(), 1);
        QCOMPARE(root->property("clicksDisabled").toInt(), 1); //Not disabled yet

        root->setProperty("disableLower", QVariant(true));

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100), doubleClickInterval);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

        QCOMPARE(root->property("clicksEnabled").toInt(), 2);
        QCOMPARE(root->property("clicksDisabled").toInt(), 1); //disabled, shouldn't increment
    }
    {
        QQuickView window;
        QVERIFY(QQuickTest::showView(window, testFileUrl("qtbug49100.qml")));
        QQuickItem *root = window.rootObject();
        QVERIFY(root);

        // QTBUG-49100
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

        QVERIFY(window.rootObject());
    }
}

void tst_QQuickMouseArea::hoverPosition()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("hoverPosition.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);

    QCOMPARE(root->property("mouseX").toReal(), qreal(0));
    QCOMPARE(root->property("mouseY").toReal(), qreal(0));

    QTest::mouseMove(&window,QPoint(10,32));

    QCOMPARE(root->property("mouseX").toReal(), qreal(10));
    QCOMPARE(root->property("mouseY").toReal(), qreal(32));
}

void tst_QQuickMouseArea::hoverPropagation()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("hoverPropagation.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);

    // QTBUG-18175, to behave like GV did.
    QCOMPARE(root->property("point1").toBool(), false);
    QCOMPARE(root->property("point2").toBool(), false);

    QTest::mouseMove(&window, {32, 32});

    QCOMPARE(root->property("point1").toBool(), true);
    QCOMPARE(root->property("point2").toBool(), false);

    QTest::mouseMove(&window, {232, 32});

    QCOMPARE(root->property("point1").toBool(), false);
    QCOMPARE(root->property("point2").toBool(), true);
}

void tst_QQuickMouseArea::hoverVisible()
{
    if (QGuiApplication::platformName() == QLatin1String("minimal"))
        QSKIP("Skipping due to grabWindow not functional on minimal platforms");

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("hoverVisible.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);

    QQuickMouseArea *mouseTracker = root->findChild<QQuickMouseArea*>("mousetracker");
    QVERIFY(mouseTracker != nullptr);

    QSignalSpy enteredSpy(mouseTracker, SIGNAL(entered()));

    // Note: We need to use a position that is different from the position in the last event
    // generated in the previous test case. Otherwise it is not interpreted as a move.
    QTest::mouseMove(&window,QPoint(11,33));

    QCOMPARE(mouseTracker->hovered(), false);
    QCOMPARE(enteredSpy.size(), 0);

    mouseTracker->setVisible(true);

    QCOMPARE(mouseTracker->hovered(), true);
    QCOMPARE(enteredSpy.size(), 1);

    QCOMPARE(QPointF(mouseTracker->mouseX(), mouseTracker->mouseY()), QPointF(11,33));

    // QTBUG-77983
    mouseTracker->setVisible(false);
    mouseTracker->setEnabled(false);

    QCOMPARE(mouseTracker->hovered(), false);
    mouseTracker->setVisible(true);
    // if the enabled property is false, the containsMouse property shouldn't become true
    // when an invisible mousearea become visible
    QCOMPARE(mouseTracker->hovered(), false);

    mouseTracker->parentItem()->setEnabled(false);
    mouseTracker->setVisible(false);
    mouseTracker->setEnabled(true);

    QCOMPARE(mouseTracker->hovered(), false);
    mouseTracker->setVisible(true);
    // if the parent item is not enabled, the containsMouse property will be false, even if
    // the mousearea is enabled
    QCOMPARE(mouseTracker->hovered(), false);

    mouseTracker->parentItem()->setEnabled(true);
    mouseTracker->setVisible(false);
    mouseTracker->setEnabled(true);

    QCOMPARE(mouseTracker->hovered(), false);
    mouseTracker->setVisible(true);
    QCOMPARE(mouseTracker->hovered(), true);
}

void tst_QQuickMouseArea::hoverAfterPress()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("hoverAfterPress.qml")));

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea != nullptr);
    QTest::mouseMove(&window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
    QTest::mouseMove(&window, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mouseMove(&window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
    QTest::mouseMove(&window, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mouseMove(&window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
}

void tst_QQuickMouseArea::subtreeHoverEnabled()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("qtbug54019.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);

    QQuickMouseArea *mouseArea = root->findChild<QQuickMouseArea*>();
    QQuickItemPrivate *rootPrivate = QQuickItemPrivate::get(root);
    QVERIFY(mouseArea != nullptr);
    QTest::mouseMove(&window, QPoint(10, 160));
    QCOMPARE(mouseArea->hovered(), false);
    QVERIFY(rootPrivate->subtreeHoverEnabled);
    QTest::mouseMove(&window, QPoint(10, 10));
    QCOMPARE(mouseArea->hovered(), true);
    QTest::mouseMove(&window, QPoint(160, 10));
    QCOMPARE(mouseArea->hovered(), false);
}

void tst_QQuickMouseArea::hoverWhenDisabled()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("hoverVisible.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);

    QQuickMouseArea *mouseArea = root->findChild<QQuickMouseArea*>();
    QVERIFY(mouseArea);
    mouseArea->setVisible(true);

    QTest::mouseMove(&window, QPoint(50, 50));
    QVERIFY(mouseArea->hovered());

    mouseArea->setEnabled(false);
    QTest::mouseMove(&window, QPoint(51, 50));
    QVERIFY(!mouseArea->hovered());

    mouseArea->setEnabled(true);
    QTest::mouseMove(&window, QPoint(50, 50));
    QVERIFY(mouseArea->hovered());

    mouseArea->setHoverEnabled(false);
    QTest::mouseMove(&window, QPoint(51, 50));
    QVERIFY(!mouseArea->hovered());

    mouseArea->setHoverEnabled(true);
    QTest::mouseMove(&window, QPoint(50, 50));
    QVERIFY(mouseArea->hovered());
}

void tst_QQuickMouseArea::disableAfterPress()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("dragging.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);

    QQuickMouseArea *mouseArea = root->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseArea->drag();
    QVERIFY(mouseArea != nullptr);
    QVERIFY(drag != nullptr);

    QSignalSpy mousePositionSpy(mouseArea, SIGNAL(positionChanged(QQuickMouseEvent*)));
    QSignalSpy mousePressSpy(mouseArea, SIGNAL(pressed(QQuickMouseEvent*)));
    QSignalSpy mouseReleaseSpy(mouseArea, SIGNAL(released(QQuickMouseEvent*)));

    // target
    QQuickItem *blackRect = root->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());

    QVERIFY(!drag->active());
    QPoint p = QPoint(100,100);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);
    QTRY_COMPARE(mousePressSpy.size(), 1);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    // First move event triggers drag, second is acted upon.
    // This is due to possibility of higher stacked area taking precedence.

    p += QPoint(startDragDistance() + 1, 0);
    QTest::mouseMove(&window, p);
    p += QPoint(11, 11);
    QTest::mouseMove(&window, p);

    QTRY_COMPARE(mousePositionSpy.size(), 2);

    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);

    mouseArea->setEnabled(false);

    // move should still be acted upon
    p += QPoint(11, 11);
    QTest::mouseMove(&window, p);
    p += QPoint(11, 11);
    QTest::mouseMove(&window, p);

    QTRY_COMPARE(mousePositionSpy.size(), 4);

    QVERIFY(drag->active());
    QCOMPARE(blackRect->x(), 83.0);
    QCOMPARE(blackRect->y(), 83.0);

    QVERIFY(mouseArea->isPressed());
    QVERIFY(mouseArea->hovered());

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);

    QTRY_COMPARE(mouseReleaseSpy.size(), 1);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 83.0);
    QCOMPARE(blackRect->y(), 83.0);

    QVERIFY(!mouseArea->isPressed());
    QVERIFY(!mouseArea->hovered()); // since hover is not enabled

    // Next press will be ignored
    blackRect->setX(50);
    blackRect->setY(50);

    mousePressSpy.clear();
    mousePositionSpy.clear();
    mouseReleaseSpy.clear();

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::qWait(50);
    QCOMPARE(mousePressSpy.size(), 0);

    QTest::mouseMove(&window, QPoint(111,111));
    QTest::qWait(50);
    QTest::mouseMove(&window, QPoint(122,122));
    QTest::qWait(50);

    QCOMPARE(mousePositionSpy.size(), 0);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(122,122));
    QTest::qWait(50);

    QCOMPARE(mouseReleaseSpy.size(), 0);
}

void tst_QQuickMouseArea::disableParentOnPress_data()
{
    QTest::addColumn<const QPointingDevice *>("device");

    QTest::newRow("core pointer") << QPointingDevice::primaryPointingDevice();
    QTest::newRow("touch") << static_cast<const QPointingDevice *>(device); // TODO QTBUG-107864
}

void tst_QQuickMouseArea::disableParentOnPress() // QTBUG-39806 and QTBUG-103788
{
    QFETCH(const QPointingDevice *, device);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("disableParentOnPress.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);
    QQuickMouseArea *mouseArea = root->findChild<QQuickMouseArea*>();
    QVERIFY(mouseArea);

    QSignalSpy pressedChangedSpy(mouseArea, &QQuickMouseArea::pressedChanged);
    QSignalSpy canceledSpy(mouseArea, &QQuickMouseArea::canceled);
    QSignalSpy enabledSpy(mouseArea, &QQuickMouseArea::enabledChanged);
    QSignalSpy parentEnabledSpy(root, &QQuickItem::enabledChanged);
    const QPoint p(100, 100);

    QQuickTest::pointerPress(device, &window, 0, p);
    QTRY_COMPARE(parentEnabledSpy.size(), 1);
    QVERIFY(!root->isEnabled());
    QVERIFY(mouseArea->isEnabled()); // enabled is independent, unfortunately (inverse of QTBUG-38364)
    QVERIFY(!QQuickItemPrivate::get(mouseArea)->effectiveEnable);
    // bug fix: it knows it got effectively disabled, so now it's no longer pressed
    QVERIFY(!mouseArea->isPressed());
    QCOMPARE(canceledSpy.size(), 1); // ...because the press was canceled
    QCOMPARE(pressedChangedSpy.size(), 2); // kerchunk
    QQuickTest::pointerRelease(device, &window, 0, p);

    // now re-enable it and try again
    root->setEnabled(true);
    QQuickTest::pointerPress(device, &window, 0, p);
    QTRY_VERIFY(!root->isEnabled());
    QVERIFY(!QQuickItemPrivate::get(mouseArea)->effectiveEnable);
    QVERIFY(!mouseArea->isPressed());
    QCOMPARE(canceledSpy.size(), 2);
    QCOMPARE(pressedChangedSpy.size(), 4);
    QQuickTest::pointerRelease(device, &window, 0, p);
}

void tst_QQuickMouseArea::onWheel()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("wheel.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);

    QWheelEvent wheelEvent(QPoint(10, 32), QPoint(10, 32), QPoint(60, 20), QPoint(0, 120),
                           Qt::NoButton, Qt::ControlModifier, Qt::NoScrollPhase, false);
    QGuiApplication::sendEvent(&window, &wheelEvent);

    QCOMPARE(root->property("angleDeltaY").toInt(), 120);
    QCOMPARE(root->property("mouseX").toReal(), qreal(10));
    QCOMPARE(root->property("mouseY").toReal(), qreal(32));
    QCOMPARE(root->property("controlPressed").toBool(), true);
}

void tst_QQuickMouseArea::transformedMouseArea_data()
{
    QTest::addColumn<bool>("insideTarget");
    QTest::addColumn<QList<QPoint> >("points");

    QList<QPoint> pointsInside;
    pointsInside << QPoint(200, 140)
                 << QPoint(140, 200)
                 << QPoint(200, 200)
                 << QPoint(260, 200)
                 << QPoint(200, 260);
    QTest::newRow("checking points inside") << true << pointsInside;

    QList<QPoint> pointsOutside;
    pointsOutside << QPoint(140, 140)
                  << QPoint(260, 140)
                  << QPoint(120, 200)
                  << QPoint(280, 200)
                  << QPoint(140, 260)
                  << QPoint(260, 260);
    QTest::newRow("checking points outside") << false << pointsOutside;
}

void tst_QQuickMouseArea::transformedMouseArea()
{
    QFETCH(bool, insideTarget);
    QFETCH(QList<QPoint>, points);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("transformedMouseArea.qml")));

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mouseArea");
    QVERIFY(mouseArea);

    for (const QPoint &point : points) {
        // check hover
        QTest::mouseMove(&window, point);
        QTRY_COMPARE(mouseArea->property("containsMouse").toBool(), insideTarget);

        // check mouse press
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point);
        QTRY_COMPARE(mouseArea->property("pressed").toBool(), insideTarget);

        // check mouse release
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
        QTRY_COMPARE(mouseArea->property("pressed").toBool(), false);
    }
}

struct MouseEvent {
    QEvent::Type type;
    Qt::MouseButton button;
};
Q_DECLARE_METATYPE(MouseEvent)

void tst_QQuickMouseArea::pressedMultipleButtons_data()
{
    QTest::addColumn<Qt::MouseButtons>("accepted");
    QTest::addColumn<QList<MouseEvent> >("mouseEvents");
    QTest::addColumn<QList<bool> >("pressed");
    QTest::addColumn<QList<Qt::MouseButtons> >("pressedButtons");
    QTest::addColumn<int>("changeCount");

    Qt::MouseButtons accepted;
    QList<MouseEvent> mouseEvents;
    QList<bool> pressed;
    QList<Qt::MouseButtons> pressedButtons;
    int changeCount;

    MouseEvent leftPress = { QEvent::MouseButtonPress, Qt::LeftButton };
    MouseEvent leftRelease = { QEvent::MouseButtonRelease, Qt::LeftButton };
    MouseEvent rightPress = { QEvent::MouseButtonPress, Qt::RightButton };
    MouseEvent rightRelease = { QEvent::MouseButtonRelease, Qt::RightButton };

    auto addRowWithFormattedTitleAndReset = [&]() {
        QByteArray title;
        title.append("Accept:");
        if (accepted & Qt::LeftButton)
            title.append(" LeftButton");
        if (accepted & Qt::RightButton)
            title.append(" RightButton");
        title.append(" | Events:");
        for (MouseEvent event : mouseEvents) {
            title.append(event.type == QEvent::MouseButtonPress ? " Press" : " Release");
            title.append(event.button == Qt::LeftButton ? " Left," : " Right,");
        }
        title.chop(1); // remove last comma
        QTest::newRow(title) << accepted << mouseEvents << pressed << pressedButtons << changeCount;

        mouseEvents.clear();
        pressed.clear();
        pressedButtons.clear();
    };

    accepted = Qt::LeftButton;
    changeCount = 2;
    mouseEvents << leftPress << rightPress << rightRelease << leftRelease;
    pressed << true << true << true << false;
    pressedButtons << Qt::LeftButton << Qt::LeftButton << Qt::LeftButton << Qt::NoButton;
    addRowWithFormattedTitleAndReset();

    accepted = Qt::LeftButton;
    changeCount = 2;
    mouseEvents << leftPress << rightPress << leftRelease << rightRelease;
    pressed << true << true << false << false;
    pressedButtons << Qt::LeftButton << Qt::LeftButton << Qt::NoButton << Qt::NoButton;
    addRowWithFormattedTitleAndReset();

    accepted = Qt::LeftButton | Qt::RightButton;
    changeCount = 4;
    mouseEvents << leftPress << rightPress << rightRelease << leftRelease;
    pressed << true << true << true << false;
    pressedButtons << Qt::LeftButton << (Qt::LeftButton | Qt::RightButton) << Qt::LeftButton
                   << Qt::NoButton;
    addRowWithFormattedTitleAndReset();

    accepted = Qt::RightButton;
    changeCount = 2;
    mouseEvents << rightPress << leftPress << rightRelease << leftRelease;
    pressed << true << true << false << false;
    pressedButtons << Qt::RightButton << Qt::RightButton << Qt::NoButton << Qt::NoButton;
    addRowWithFormattedTitleAndReset();
}

void tst_QQuickMouseArea::pressedMultipleButtons()
{
    QFETCH(Qt::MouseButtons, accepted);
    QFETCH(QList<MouseEvent>, mouseEvents);
    QFETCH(QList<bool>, pressed);
    QFETCH(QList<Qt::MouseButtons>, pressedButtons);
    QFETCH(int, changeCount);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("simple.qml")));

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mousearea");
    QVERIFY(mouseArea != nullptr);

    QSignalSpy pressedSpy(mouseArea, SIGNAL(pressedChanged()));
    QSignalSpy pressedButtonsSpy(mouseArea, SIGNAL(pressedButtonsChanged()));
    mouseArea->setAcceptedMouseButtons(accepted);

    QPoint point(10, 10);
    for (int i = 0; i < mouseEvents.size(); ++i) {
        const MouseEvent mouseEvent = mouseEvents.at(i);
        if (mouseEvent.type == QEvent::MouseButtonPress)
            QTest::mousePress(&window, mouseEvent.button, Qt::NoModifier, point);
        else
            QTest::mouseRelease(&window, mouseEvent.button, Qt::NoModifier, point);
        QCOMPARE(mouseArea->isPressed(), pressed.at(i));
        QCOMPARE(mouseArea->pressedButtons(), pressedButtons.at(i));
    }

    QCOMPARE(pressedSpy.size(), 2);
    QCOMPARE(pressedButtonsSpy.size(), changeCount);
}

void tst_QQuickMouseArea::changeAxis()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("changeAxis.qml")));

    QQuickMouseArea *mouseRegion = window.rootObject()->findChild<QQuickMouseArea*>("mouseregion");
    QQuickDrag *drag = mouseRegion->drag();
    QVERIFY(mouseRegion != nullptr);
    QVERIFY(drag != nullptr);

    mouseRegion->setAcceptedButtons(Qt::LeftButton);

    // target
    QQuickItem *blackRect = window.rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != nullptr);
    QCOMPARE(blackRect, drag->target());

    QVERIFY(!drag->active());

    // Start a diagonal drag
    QPoint p = QPoint(100, 100);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);

    QVERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 50.0);
    QCOMPARE(blackRect->y(), 50.0);

    p += QPoint(startDragDistance() + 1, startDragDistance() + 1);
    QTest::mouseMove(&window, p);
    p += QPoint(11, 11);
    QTest::mouseMove(&window, p);
    QTRY_VERIFY(drag->active());
    QTRY_COMPARE(blackRect->x(), 61.0);
    QCOMPARE(blackRect->y(), 61.0);
    QCOMPARE(drag->axis(), QQuickDrag::XAndYAxis);

    /* When blackRect.x becomes bigger than 75, the drag axis is changed to
     * Drag.YAxis by the QML code. Verify that this happens, and that the drag
     * movement is effectively constrained to the Y axis. */
    p += QPoint(22, 22);
    QTest::mouseMove(&window, p);

    QTRY_COMPARE(blackRect->x(), 83.0);
    QTRY_COMPARE(blackRect->y(), 83.0);
    QTRY_COMPARE(drag->axis(), QQuickDrag::YAxis);

    p += QPoint(11, 11);
    QTest::mouseMove(&window, p);

    QTRY_COMPARE(blackRect->y(), 94.0);
    QCOMPARE(blackRect->x(), 83.0);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);

    QTRY_VERIFY(!drag->active());
    QCOMPARE(blackRect->x(), 83.0);
    QCOMPARE(blackRect->y(), 94.0);
}

#if QT_CONFIG(cursor)
void tst_QQuickMouseArea::cursorShape()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData("import QtQuick 2.0\n MouseArea {}", QUrl());
    QScopedPointer<QObject> object(component.create());
    QQuickMouseArea *mouseArea = qobject_cast<QQuickMouseArea *>(object.data());
    QVERIFY(mouseArea);

    QSignalSpy spy(mouseArea, SIGNAL(cursorShapeChanged()));

    QCOMPARE(mouseArea->cursorShape(), Qt::ArrowCursor);
    QCOMPARE(mouseArea->cursor().shape(), Qt::ArrowCursor);

    mouseArea->setCursorShape(Qt::IBeamCursor);
    QCOMPARE(mouseArea->cursorShape(), Qt::IBeamCursor);
    QCOMPARE(mouseArea->cursor().shape(), Qt::IBeamCursor);
    QCOMPARE(spy.size(), 1);

    mouseArea->setCursorShape(Qt::IBeamCursor);
    QCOMPARE(spy.size(), 1);

    mouseArea->setCursorShape(Qt::WaitCursor);
    QCOMPARE(mouseArea->cursorShape(), Qt::WaitCursor);
    QCOMPARE(mouseArea->cursor().shape(), Qt::WaitCursor);
    QCOMPARE(spy.size(), 2);
}
#endif

void tst_QQuickMouseArea::moveAndReleaseWithoutPress()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("moveAndReleaseWithoutPress.qml")));
    QObject *root = window.rootObject();
    QVERIFY(root);

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    // the press was not accepted, make sure there is no move or release event
    QTest::mouseMove(&window, QPoint(110,110), 50);

    // use qwait here because we want to make sure an event does NOT happen
    // the test fails if the default state changes, while it shouldn't
    QTest::qWait(100);
    QCOMPARE(root->property("hadMove").toBool(), false);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(110,110));
    QTest::qWait(100);
    QCOMPARE(root->property("hadRelease").toBool(), false);
}

void tst_QQuickMouseArea::nestedStopAtBounds_data()
{
    QTest::addColumn<bool>("transpose");
    QTest::addColumn<bool>("invert");

    QTest::newRow("left") << false << false;
    QTest::newRow("right") << false << true;
    QTest::newRow("top") << true << false;
    QTest::newRow("bottom") << true << true;
}

void tst_QQuickMouseArea::nestedStopAtBounds()
{
    QFETCH(bool, transpose);
    QFETCH(bool, invert);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedStopAtBounds.qml")));

    QQuickMouseArea *outer =  window.rootObject()->findChild<QQuickMouseArea*>("outer");
    QVERIFY(outer);

    QQuickMouseArea *inner = outer->findChild<QQuickMouseArea*>("inner");
    QVERIFY(inner);
    inner->drag()->setAxis(transpose ? QQuickDrag::YAxis : QQuickDrag::XAxis);
    inner->setX(invert ? 100 : 0);
    inner->setY(invert ? 100 : 0);

    const int threshold = qApp->styleHints()->startDragDistance();

    QPoint position(200, 200);
    int &axis = transpose ? position.ry() : position.rx();

    // drag toward the aligned boundary.  Outer mouse area dragged.
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    axis += invert ? threshold * 2 : -threshold * 2;
    QTest::mouseMove(&window, position);
    axis += invert ? threshold : -threshold;
    QTest::mouseMove(&window, position);

    // outer drag will not receive mouse event, when the focus has been stolen.
    // => try to regain and time out if it fails.
    while (!QTest::qWaitFor([&outer]() { return outer->drag()->active(); }))
        window.raise();

    QCOMPARE(inner->drag()->active(), false);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);

    QVERIFY(!outer->drag()->active());

    axis = 200;
    outer->setX(50);
    outer->setY(50);

    // drag away from the aligned boundary.  Inner mouse area dragged.
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    axis += invert ? -threshold * 2 : threshold * 2;
    QTest::mouseMove(&window, position);
    axis += invert ? -threshold : threshold;
    QTest::mouseMove(&window, position);
    QTRY_COMPARE(outer->drag()->active(), false);
    QTRY_COMPARE(inner->drag()->active(), true);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);
}

void tst_QQuickMouseArea::nestedFlickableStopAtBounds()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("nestedFlickableStopAtBounds.qml")));

    QQuickMouseArea *mouseArea =  window.rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea);

    QQuickFlickable *flickable = mouseArea->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);

    const int threshold = qApp->styleHints()->startDragDistance();

    QPoint position(200, 280);
    int &pos = position.ry();

    // Drag up - should move the Flickable to end
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    pos -= threshold * 2;
    QTest::mouseMove(&window, position);
    pos -= threshold * 2;
    QTest::mouseMove(&window, position);
    QTest::qWait(10);
    pos -= 150;
    QTest::mouseMove(&window, position);
    QVERIFY(flickable->isDragging());
    QVERIFY(!mouseArea->drag()->active());
    QCOMPARE(flickable->isAtYEnd(), true);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);

    QTRY_VERIFY(!flickable->isMoving());

    pos = 280;

    // Drag up again - should activate MouseArea drag
    QVERIFY(!mouseArea->drag()->active());
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    pos -= threshold * 2;
    QTest::mouseMove(&window, position);
    pos -= threshold * 2;
    QTest::mouseMove(&window, position);
    QTest::qWait(10);
    pos -= 20;
    QTest::mouseMove(&window, position);
    QVERIFY(mouseArea->drag()->active());
    QCOMPARE(flickable->isAtYEnd(), true);
    QVERIFY(!flickable->isDragging());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);

    // Drag to the top and verify that the MouseArea doesn't steal the grab when we drag back (QTBUG-56036)
    pos = 50;

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    pos += threshold;
    QTest::mouseMove(&window, position);
    pos += threshold;
    QTest::mouseMove(&window, position);
    QTest::qWait(10);
    pos += 150;
    QTest::mouseMove(&window, position);
    QVERIFY(flickable->isDragging());
    QVERIFY(!mouseArea->drag()->active());
    QCOMPARE(flickable->isAtYBeginning(), true);
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);

    QTRY_VERIFY(!flickable->isMoving());

    pos = 280;

    // Drag up again - should not activate MouseArea drag
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, position);
    QTest::qWait(10);
    pos -= threshold;
    QTest::mouseMove(&window, position);
    pos -= threshold;
    QTest::mouseMove(&window, position);
    QTest::qWait(10);
    pos -= 100;
    QTest::mouseMove(&window, position);
    QVERIFY(flickable->isDragging());
    QVERIFY(!mouseArea->drag()->active());
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, position);
}

void tst_QQuickMouseArea::containsPress_data()
{
    QTest::addColumn<bool>("hoverEnabled");

    QTest::newRow("hover enabled") << true;
    QTest::newRow("hover disaabled") << false;
}

void tst_QQuickMouseArea::containsPress()
{
    QFETCH(bool, hoverEnabled);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("containsPress.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea != nullptr);

    QSignalSpy containsPressSpy(mouseArea, SIGNAL(containsPressChanged()));

    mouseArea->setHoverEnabled(hoverEnabled);

    QTest::mouseMove(&window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
    QCOMPARE(mouseArea->isPressed(), false);
    QCOMPARE(mouseArea->containsPress(), false);

    QTest::mouseMove(&window, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), hoverEnabled);
    QCOMPARE(mouseArea->isPressed(), false);
    QCOMPARE(mouseArea->containsPress(), false);

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QTRY_COMPARE(mouseArea->isPressed(), true);
    QCOMPARE(mouseArea->containsPress(), true);
    QCOMPARE(containsPressSpy.size(), 1);

    QTest::mouseMove(&window, QPoint(22,33));
    QCOMPARE(mouseArea->hovered(), false);
    QCOMPARE(mouseArea->isPressed(), true);
    QCOMPARE(mouseArea->containsPress(), false);
    QCOMPARE(containsPressSpy.size(), 2);

    QTest::mouseMove(&window, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), true);
    QCOMPARE(mouseArea->isPressed(), true);
    QCOMPARE(mouseArea->containsPress(), true);
    QCOMPARE(containsPressSpy.size(), 3);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(200,200));
    QCOMPARE(mouseArea->hovered(), hoverEnabled);
    QCOMPARE(mouseArea->isPressed(), false);
    QCOMPARE(mouseArea->containsPress(), false);
    QCOMPARE(containsPressSpy.size(), 4);
}

void tst_QQuickMouseArea::ignoreBySource()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("ignoreBySource.qml")));

    auto mouseDevPriv = QPointingDevicePrivate::get(QPointingDevice::primaryPointingDevice());
    auto touchDevPriv = QPointingDevicePrivate::get(device);

    QQuickItem *root = qobject_cast<QQuickItem*>(window.rootObject());
    QVERIFY(root);

    QQuickMouseArea *mouseArea = root->findChild<QQuickMouseArea*>("mousearea");
    QVERIFY(mouseArea);

    QQuickFlickable *flickable = root->findChild<QQuickFlickable*>("flickable");
    QVERIFY(flickable);

    // MouseArea should grab the press because it's interested in non-synthesized mouse events
    QPoint p = QPoint(80, 80);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);
    QCOMPARE(mouseDevPriv->firstPointExclusiveGrabber(), mouseArea);
    // That was a real mouse event
    QCOMPARE(root->property("lastEventSource").toInt(), int(Qt::MouseEventNotSynthesized));

    // Flickable content should not move
    p -= QPoint(startDragDistance() + 1, startDragDistance() + 1);
    QTest::mouseMove(&window, p);
    p -= QPoint(11, 11);
    QTest::mouseMove(&window, p);
    p -= QPoint(11, 11);
    QTest::mouseMove(&window, p);
    QCOMPARE(flickable->contentX(), 0.);
    QCOMPARE(flickable->contentY(), 0.);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, p);
    QCOMPARE(mouseDevPriv->firstPointExclusiveGrabber(), nullptr);

    // Now try touch events and confirm that MouseArea ignores them, while Flickable does its thing
    p = QPoint(80, 80);
    QTest::touchEvent(&window, device).press(0, p, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(touchDevPriv->firstPointExclusiveGrabber(), flickable);

    // That was a fake mouse event
    QCOMPARE(root->property("lastEventSource").toInt(), int(Qt::MouseEventSynthesizedByQt));
    p -= QPoint(startDragDistance() + 1, startDragDistance() + 1);
    QTest::touchEvent(&window, device).move(0, p, &window);
    p -= QPoint(11, 11);
    QTest::touchEvent(&window, device).move(0, p, &window);
    p -= QPoint(11, 11);
    QTest::touchEvent(&window, device).move(0, p, &window);

    QQuickTouchUtils::flush(&window);
    QCOMPARE(touchDevPriv->firstPointExclusiveGrabber(), flickable);
    QTest::touchEvent(&window, device).release(0, p, &window);
    QQuickTouchUtils::flush(&window);

    // Flickable content should have moved
    QTRY_VERIFY(flickable->contentX() > 1);
    QVERIFY(flickable->contentY() > 1);

    // Now tell the MouseArea to accept only synthesized events, and repeat the tests
    root->setProperty("allowedSource", Qt::MouseEventSynthesizedByQt);
    flickable->setContentX(0);
    flickable->setContentY(0);

    // MouseArea should ignore the press because it's interested in synthesized mouse events
    p = QPoint(80, 80);
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, p);
    QVERIFY(mouseDevPriv->firstPointExclusiveGrabber() != mouseArea);
    // That was a real mouse event
    QVERIFY(root->property("lastEventSource").toInt() == Qt::MouseEventNotSynthesized);

    // Flickable content should move
    p -= QPoint(startDragDistance() + 1, startDragDistance() + 1);
    QTest::mouseMove(&window, p);
    p -= QPoint(11, 11);
    QTest::mouseMove(&window, p);
    p -= QPoint(11, 11);
    QTest::mouseMove(&window, p);
    QTRY_VERIFY(flickable->contentX() > 1);
    QVERIFY(flickable->contentY() > 1);

    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(47, 47));
    flickable->setContentX(0);
    flickable->setContentY(0);

    // Now try touch events and confirm that MouseArea gets them, while Flickable doesn't
    p = QPoint(80, 80);
    QTest::touchEvent(&window, device).press(0, p, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(touchDevPriv->firstPointExclusiveGrabber(), mouseArea);
    p -= QPoint(startDragDistance() + 1, startDragDistance() + 1);
    QTest::touchEvent(&window, device).move(0, p, &window);
    p -= QPoint(11, 11);
    QTest::touchEvent(&window, device).move(0, p, &window);
    p -= QPoint(11, 11);
    QTest::touchEvent(&window, device).move(0, p, &window);
    QQuickTouchUtils::flush(&window);
    QCOMPARE(touchDevPriv->firstPointExclusiveGrabber(), mouseArea);
    QTest::touchEvent(&window, device).release(0, QPoint(47,47), &window);
    QQuickTouchUtils::flush(&window);

    // Flickable content should not have moved
    QCOMPARE(flickable->contentX(), 0);
    QCOMPARE(flickable->contentY(), 0);
}

void tst_QQuickMouseArea::notPressedAfterStolenGrab() // QTBUG-55325
{
    QQuickWindow window;
    window.resize(200, 200);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));

    QQuickMouseArea *ma = new QQuickMouseArea(window.contentItem());
    ma->setSize(window.size());
    QObject::connect(ma,
                     static_cast<void (QQuickMouseArea::*)(QQuickMouseEvent*)>(&QQuickMouseArea::pressed),
                     [&]() { qCDebug(lcTests) << "stealing grab now"; window.contentItem()->grabMouse(); });

    QTest::mouseClick(&window, Qt::LeftButton);
    QVERIFY(!ma->isPressed());
}

void tst_QQuickMouseArea::pressAndHold_data()
{
    QTest::addColumn<int>("pressAndHoldInterval");
    QTest::addColumn<int>("waitTime");

    QTest::newRow("default") << -1 << QGuiApplication::styleHints()->mousePressAndHoldInterval();
    QTest::newRow("short") << 500 << 500;
    QTest::newRow("long") << 1000 << 1000;
}

void tst_QQuickMouseArea::pressAndHold()
{
    QFETCH(int, pressAndHoldInterval);
    QFETCH(int, waitTime);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("pressAndHold.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea != nullptr);

    QSignalSpy pressAndHoldSpy(mouseArea, &QQuickMouseArea::pressAndHold);

    if (pressAndHoldInterval > -1)
        mouseArea->setPressAndHoldInterval(pressAndHoldInterval);
    else
        mouseArea->resetPressAndHoldInterval();

    QElapsedTimer t;
    t.start();
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
    QVERIFY(pressAndHoldSpy.wait());
    // should be off by no more than 20% of waitTime
    QVERIFY(qAbs(t.elapsed() - waitTime) < (waitTime * 0.2));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(50, 50));
}

void tst_QQuickMouseArea::pressOneAndTapAnother_data()
{
    QTest::addColumn<bool>("pressMouseFirst");
    QTest::addColumn<bool>("releaseMouseFirst");

    QTest::newRow("press mouse, tap touch, release mouse") << true << false; // QTBUG-64249 as written
    QTest::newRow("press touch, press mouse, release touch, release mouse") << false << false;
    QTest::newRow("press mouse, press touch, release mouse, release touch") << true << true;
    // TODO fix in a separate patch after the 5.9->5.10 merge
    // QTest::newRow("press touch, click mouse, release touch") << false << true;
}

void tst_QQuickMouseArea::pressOneAndTapAnother()
{
    QFETCH(bool, pressMouseFirst);
    QFETCH(bool, releaseMouseFirst);

    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("twoMouseAreas.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);
    QQuickMouseArea *bottomMA = root->findChild<QQuickMouseArea*>("bottom");
    QVERIFY(bottomMA);
    QQuickMouseArea *topMA = root->findChild<QQuickMouseArea*>("top");
    QVERIFY(topMA);

    QPoint upper(32, 32);
    QPoint lower(32, window.height() - 32);

    // press them both
    if (pressMouseFirst) {
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, lower);
        QTRY_COMPARE(bottomMA->isPressed(), true);

        QTest::touchEvent(&window, device).press(0, lower, &window);
        QQuickTouchUtils::flush(&window);
        QTRY_COMPARE(bottomMA->isPressed(), true);
    } else {
        QTest::touchEvent(&window, device).press(0, lower, &window);
        QQuickTouchUtils::flush(&window);
        QTRY_COMPARE(bottomMA->isPressed(), true);

        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, lower);
        QTRY_COMPARE(bottomMA->isPressed(), true);
    }

    // release them both and make sure neither one gets stuck
    if (releaseMouseFirst) {
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, lower);
        QTRY_COMPARE(bottomMA->isPressed(), false);

        QTest::touchEvent(&window, device).release(0, upper, &window);
        QQuickTouchUtils::flush(&window);
        QTRY_COMPARE(topMA->isPressed(), false);
    } else {
        QTest::touchEvent(&window, device).release(0, upper, &window);
        QQuickTouchUtils::flush(&window);

        QTRY_COMPARE(topMA->isPressed(), false);
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, lower);
        QTRY_COMPARE(bottomMA->isPressed(), false);
    }
}

void tst_QQuickMouseArea::mask()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("mask.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);

    // click inside the mask, and verify it registers
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100,100));

    QCOMPARE(window.rootObject()->property("pressed").toInt(), 1);
    QCOMPARE(window.rootObject()->property("released").toInt(), 1);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 1);

    // click outside the mask (but inside the MouseArea), and verify it doesn't register
    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));
    QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, QPoint(10,10));

    QCOMPARE(window.rootObject()->property("pressed").toInt(), 1);
    QCOMPARE(window.rootObject()->property("released").toInt(), 1);
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 1);
}

void tst_QQuickMouseArea::nestedEventDelivery() // QTBUG-70898
{
#ifdef Q_OS_MACOS
    QSKIP("this test currently crashes on MacOS 10.14 in CI. See QTBUG-86729");
#endif
    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("nestedSendEvent.qml"));
    QScopedPointer<QQuickWindow> window(qmlobject_cast<QQuickWindow *>(c.create()));
    QVERIFY(window.data());

    // Click each MouseArea and verify that it doesn't crash
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(50,50));
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, QPoint(50,150));
}

void tst_QQuickMouseArea::settingHiddenInPressUngrabs()
{
    // When an item sets itself hidden, while handling pressed, it doesn't receive the grab.
    // But that in turn means it doesn't see any release events, so we need to make sure it
    // receives an ungrab event.

    QQmlEngine engine;
    QQmlComponent c(&engine, testFileUrl("settingHiddenInPressUngrabs.qml"));
    QScopedPointer<QQuickWindow> window(qmlobject_cast<QQuickWindow *>(c.create()));
    QVERIFY(window.data());
    window->show();
    window->requestActivate();
    QVERIFY(QTest::qWaitForWindowExposed(window.data()));

    QQuickMouseArea *catArea = window->findChild<QQuickMouseArea*>("cat");
    QVERIFY(catArea != nullptr);
    auto pointOnCatArea = catArea->mapToScene(QPointF(5.0, 5.0)).toPoint();
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, pointOnCatArea);

    QCoreApplication::processEvents();
    // The click hides the cat area
    QTRY_VERIFY(!catArea->isVisible());
    // The cat area is not stuck in pressed state.
    QVERIFY(!catArea->isPressed());

    QQuickMouseArea *mouseArea = window->findChild<QQuickMouseArea*>("mouse");
    QVERIFY(mouseArea != nullptr);
    auto pointOnMouseArea = mouseArea->mapToScene(QPointF(5.0, 5.0)).toPoint();
    QTest::mouseClick(window.data(), Qt::LeftButton, Qt::NoModifier, pointOnMouseArea);

    QCoreApplication::processEvents();
    // The click disables the mouse area
    QTRY_VERIFY(!mouseArea->isEnabled());
    // The mouse area is not stuck in pressed state.
    QVERIFY(!mouseArea->isPressed());
}

void tst_QQuickMouseArea::negativeZStackingOrder() // QTBUG-83114
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("mouseAreasOverlapped.qml")));
    QQuickItem *root = window.rootObject();
    QVERIFY(root);

    QQuickMouseArea *parentMouseArea = root->findChild<QQuickMouseArea*>("parentMouseArea");
    QVERIFY(parentMouseArea != nullptr);
    QSignalSpy clickSpyParent(parentMouseArea, &QQuickMouseArea::clicked);
    QQuickMouseArea *childMouseArea = root->findChild<QQuickMouseArea*>("childMouseArea");
    QVERIFY(childMouseArea != nullptr);
    QSignalSpy clickSpyChild(childMouseArea, &QQuickMouseArea::clicked);

    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(150, 100));
    QCOMPARE(clickSpyChild.size(), 1);
    QCOMPARE(clickSpyParent.size(), 0);
    auto order = root->property("clicks").toList();
    QVERIFY(order.at(0) == "childMouseArea");

    // Now change stacking order and try again.
    childMouseArea->parentItem()->setZ(-1);
    root->setProperty("clicks", QVariantList());
    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, QPoint(150, 100));
    QCOMPARE(clickSpyChild.size(), 1);
    QCOMPARE(clickSpyParent.size(), 1);
    order = root->property("clicks").toList();
    QVERIFY(order.at(0) == "parentMouseArea");
}

// QTBUG-87197
void tst_QQuickMouseArea::containsMouseAndVisibility()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("containsMouse.qml")));

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea*>("mouseArea");
    QVERIFY(mouseArea != nullptr);
    QVERIFY(!mouseArea->isVisible());

    QTest::mouseMove(&window, QPoint(10, 10));
    QTRY_VERIFY(!mouseArea->hovered());

    mouseArea->setVisible(true);
    QVERIFY(mouseArea->isVisible());
    QTRY_VERIFY(mouseArea->hovered());

    /* we (ab-)use QPointF() as the 'reset' value in QQuickWindow's leave-event handling,
       but can't verify that this leaves an invalid interpretation of states for position
       QPoint(0, 0) as QTest::mouseMove interprets a null-position as "center of the window".

       So instead, verify the current (unexpectedly expected) behavior as far as testing is
       concern.
    */
    QTest::mouseMove(&window, QPoint(0, 0));
    QTRY_VERIFY(mouseArea->hovered());
    QTRY_VERIFY(mouseArea->isUnderMouse());

    // move to the edge (can't move outside)
    QTest::mouseMove(&window, QPoint(window.width() - 1, window.height() / 2));
    // then pretend we left
    QEvent event(QEvent::Leave);
    QGuiApplication::sendEvent(&window, &event);
    QVERIFY(!mouseArea->hovered());

    // toggle mouse area visibility - the hover state should not change
    mouseArea->setVisible(false);
    QVERIFY(!mouseArea->isVisible());
    QVERIFY(!mouseArea->hovered());

    mouseArea->setVisible(true);
    QVERIFY(mouseArea->isVisible());
    QVERIFY(!mouseArea->hovered());
}

// QTBUG-109567
void tst_QQuickMouseArea::containsMouseAndVisibilityMasked()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("containsMouseMasked.qml")));

    QQuickMouseArea *mouseArea1 = window.rootObject()->findChild<QQuickMouseArea *>("mouseArea1");
    QVERIFY(mouseArea1 != nullptr);
    QVERIFY(mouseArea1->isVisible());

    QQuickMouseArea *mouseArea2 = window.rootObject()->findChild<QQuickMouseArea *>("mouseArea2");
    QVERIFY(mouseArea2 != nullptr);
    QVERIFY(mouseArea2->isVisible());

    QTest::mouseMove(&window, QPoint(window.width() / 2, window.height() / 2));

    // Check that mouseArea" (i.e. the masking MouseArea) is the only hovered MouseArea.
    QTRY_VERIFY(!mouseArea1->hovered());
    QTRY_VERIFY(mouseArea2->hovered());

    // Toggle the visibility of the masked MouseArea (mouseArea1).
    mouseArea1->setVisible(false);
    QVERIFY(!mouseArea1->isVisible());

    mouseArea1->setVisible(true);
    QVERIFY(mouseArea1->isVisible());

    // Check that the masked MouseArea is not now hovered depite being under the mouse after
    // changing the visibility to visible. mouseArea2 should be the only hovered MouseArea still.
    QTRY_VERIFY(!mouseArea1->hovered());
    QTRY_VERIFY(mouseArea2->hovered());

    QTest::mouseMove(&window, QPoint(10, 10));

    QTRY_VERIFY(mouseArea1->hovered());
    QTRY_VERIFY(!mouseArea2->hovered());

    // Toggle the visibility of the masked MouseArea (mouseArea1).
    mouseArea1->setVisible(false);
    QVERIFY(!mouseArea1->isVisible());

    mouseArea1->setVisible(true);
    QVERIFY(mouseArea1->isVisible());

    QTRY_VERIFY(mouseArea1->hovered());
    QTRY_VERIFY(!mouseArea2->hovered());
}

// QTBUG-110594
void tst_QQuickMouseArea::containsMouseAndHoverDisabled()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("containsMouseAndHoverDisabled.qml")));

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>("mouseArea");
    QVERIFY(mouseArea != nullptr);
    QVERIFY(!mouseArea->hoverEnabled());

    QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QTRY_VERIFY(!mouseArea->hovered());
}

// QTBUG-35995 and QTBUG-102158
void tst_QQuickMouseArea::doubleClickToHide()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("doubleClickToHide.qml")));

    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>();
    QVERIFY(mouseArea);

    QTest::mouseDClick(&window, Qt::LeftButton, Qt::NoModifier, {10, 10});

    QCOMPARE(window.rootObject()->property("clicked").toInt(), 1);
    QCOMPARE(window.rootObject()->property("doubleClicked").toInt(), 1);
    QCOMPARE(mouseArea->isVisible(), false);
    QCOMPARE(mouseArea->isPressed(), false);
    QCOMPARE(mouseArea->pressedButtons(), Qt::NoButton);

    mouseArea->setVisible(true);

    QTest::mouseClick(&window, Qt::LeftButton, Qt::NoModifier, {10, 10});
    QCOMPARE(window.rootObject()->property("clicked").toInt(), 2);
}

void tst_QQuickMouseArea::releaseFirstTouchAfterSecond() // QTBUG-103766
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("simple.qml")));
    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>();
    QVERIFY(mouseArea);
    QSignalSpy pressSpy(mouseArea, SIGNAL(pressed(QQuickMouseEvent*)));
    QSignalSpy releaseSpy(mouseArea, &QQuickMouseArea::released);

    QTest::touchEvent(&window, device).press(0, {20, 20});
    QTRY_COMPARE(pressSpy.size(), 1);
    QTest::touchEvent(&window, device).stationary(0).press(1, {100, 20});
    QCOMPARE(pressSpy.size(), 1);   // touchpoint 0 is the touchmouse, touchpoint 1 is ignored
    QTest::touchEvent(&window, device).stationary(0).release(1, {100, 20});
    QCOMPARE(releaseSpy.size(), 0); // touchpoint 0 is the touchmouse, and remains pressed
    QTest::touchEvent(&window, device).release(0, {20, 20});
    QTRY_COMPARE(releaseSpy.size(), 1);
}

#if QT_CONFIG(tabletevent)
void tst_QQuickMouseArea::tabletStylusTap()
{
    QVERIFY(qApp->testAttribute(Qt::AA_SynthesizeMouseForUnhandledTabletEvents)); // MouseArea depends on it
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("simple.qml")));
    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>();
    QVERIFY(mouseArea);
    QSignalSpy pressSpy(mouseArea, SIGNAL(pressed(QQuickMouseEvent*)));
    QSignalSpy releaseSpy(mouseArea, &QQuickMouseArea::released);
    QSignalSpy clickSpy(mouseArea, &QQuickMouseArea::clicked);
    const qint64 stylusId = 1234567890;

    const QPoint point(100,100);
    QWindowSystemInterface::handleTabletEvent(&window, point, window.mapToGlobal(point),
            int(QInputDevice::DeviceType::Stylus), int(QPointingDevice::PointerType::Pen),
            Qt::LeftButton, 0.5, 0, 0, 0, 0, 0, stylusId, Qt::NoModifier);
    if (QWindowSystemInterfacePrivate::TabletEvent::platformSynthesizesMouse)
        QTest::mousePress(&window, Qt::LeftButton, Qt::NoModifier, point); // simulate what the platform does
    QTRY_COMPARE(pressSpy.size(), 1);
    QWindowSystemInterface::handleTabletEvent(&window, point, window.mapToGlobal(point),
            int(QInputDevice::DeviceType::Stylus), int(QPointingDevice::PointerType::Pen),
            Qt::NoButton, 0.5, 0, 0, 0, 0, 0, stylusId, Qt::NoModifier);
    if (QWindowSystemInterfacePrivate::TabletEvent::platformSynthesizesMouse)
        QTest::mouseRelease(&window, Qt::LeftButton, Qt::NoModifier, point);
    QTRY_COMPARE(releaseSpy.size(), 1);
    QCOMPARE(clickSpy.size(), 1);
    QCOMPARE(pressSpy.size(), 1);
}
#endif

void tst_QQuickMouseArea::syntheticRightClick()
{
    QQuickView window;
    QVERIFY(QQuickTest::showView(window, testFileUrl("simple.qml")));
    QQuickMouseArea *mouseArea = window.rootObject()->findChild<QQuickMouseArea *>();
    QVERIFY(mouseArea);
    mouseArea->setAcceptedButtons(Qt::RightButton);

    QSignalSpy clickSpy(mouseArea, &QQuickMouseArea::clicked);
    const QPointF p(20, 20);
    quint64 timestamp = 10;

    // The right-click is probably synthesized from a touch long-press IRL, but it doesn't matter for the DA's logic.
    // We could set QT_QUICK_ALLOW_SYNTHETIC_RIGHT_CLICK=0 to opt out, but otherwise it's allowed.
    QMouseEvent press(QEvent::MouseButtonPress, p, mouseArea->mapToScene(p), mouseArea->mapToGlobal(p),
                      Qt::RightButton, Qt::RightButton, Qt::NoModifier, Qt::MouseEventSynthesizedBySystem);
    press.setTimestamp(timestamp++);
    QGuiApplication::sendEvent(&window, &press);
    QCOMPARE(mouseArea->pressedButtons(), Qt::RightButton);

    QMouseEvent release(QEvent::MouseButtonRelease, p, mouseArea->mapToScene(p), mouseArea->mapToGlobal(p),
                        Qt::RightButton, Qt::RightButton, Qt::NoModifier, Qt::MouseEventSynthesizedBySystem);
    release.setTimestamp(timestamp);
    QGuiApplication::sendEvent(&window, &release);
    QCOMPARE(mouseArea->pressedButtons(), Qt::NoButton);
    QCOMPARE(clickSpy.size(), 1);
}

QTEST_MAIN(tst_QQuickMouseArea)

#include "tst_qquickmousearea.moc"
