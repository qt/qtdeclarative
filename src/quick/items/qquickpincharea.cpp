// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpincharea_p_p.h"
#include "qquickwindow.h"

#include <QtCore/qmath.h>
#include <QtGui/qevent.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformnativeinterface.h>
#include <private/qguiapplication_p.h>
#include <QVariant>

#include <float.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPA, "qt.quick.pincharea")

/*!
    \qmltype PinchEvent
    \instantiates QQuickPinchEvent
    \inqmlmodule QtQuick
    \ingroup qtquick-input-events
    \brief For specifying information about a pinch event.

    \b {The PinchEvent type was added in QtQuick 1.1}

    The \c center, \c startCenter, \c previousCenter properties provide the center position between the two touch points.

    The \c scale and \c previousScale properties provide the scale factor.

    The \c angle, \c previousAngle and \c rotation properties provide the angle between the two points and the amount of rotation.

    The \c point1, \c point2, \c startPoint1, \c startPoint2 properties provide the positions of the touch points.

    The \c accepted property may be set to false in the \c onPinchStarted handler if the gesture should not
    be handled.

    \sa PinchArea
*/

/*!
    \qmlproperty QPointF QtQuick::PinchEvent::center
    \qmlproperty QPointF QtQuick::PinchEvent::startCenter
    \qmlproperty QPointF QtQuick::PinchEvent::previousCenter

    These properties hold the position of the center point between the two touch points.

    \list
    \li \c center is the current center point
    \li \c previousCenter is the center point of the previous event.
    \li \c startCenter is the center point when the gesture began
    \endlist
*/

/*!
    \qmlproperty real QtQuick::PinchEvent::scale
    \qmlproperty real QtQuick::PinchEvent::previousScale

    These properties hold the scale factor determined by the change in distance between the two touch points.

    \list
    \li \c scale is the current scale factor.
    \li \c previousScale is the scale factor of the previous event.
    \endlist

    When a pinch gesture is started, the scale is \c 1.0.
*/

/*!
    \qmlproperty real QtQuick::PinchEvent::angle
    \qmlproperty real QtQuick::PinchEvent::previousAngle
    \qmlproperty real QtQuick::PinchEvent::rotation

    These properties hold the angle between the two touch points.

    \list
    \li \c angle is the current angle between the two points in the range -180 to 180.
    \li \c previousAngle is the angle of the previous event.
    \li \c rotation is the total rotation since the pinch gesture started.
    \endlist

    When a pinch gesture is started, the rotation is \c 0.0.
*/

/*!
    \qmlproperty QPointF QtQuick::PinchEvent::point1
    \qmlproperty QPointF QtQuick::PinchEvent::startPoint1
    \qmlproperty QPointF QtQuick::PinchEvent::point2
    \qmlproperty QPointF QtQuick::PinchEvent::startPoint2

    These properties provide the actual touch points generating the pinch.

    \list
    \li \c point1 and \c point2 hold the current positions of the points.
    \li \c startPoint1 and \c startPoint2 hold the positions of the points when the second point was touched.
    \endlist
*/

/*!
    \qmlproperty bool QtQuick::PinchEvent::accepted

    Setting this property to false in the \c PinchArea::onPinchStarted handler
    will result in no further pinch events being generated, and the gesture
    ignored.
*/

/*!
    \qmlproperty int QtQuick::PinchEvent::pointCount

    Holds the number of points currently touched.  The PinchArea will not react
    until two touch points have initited a gesture, but will remain active until
    all touch points have been released.
*/

QQuickPinch::QQuickPinch()
    : m_target(nullptr), m_minScale(1.0), m_maxScale(1.0)
    , m_minRotation(0.0), m_maxRotation(0.0)
    , m_axis(NoDrag), m_xmin(-FLT_MAX), m_xmax(FLT_MAX)
    , m_ymin(-FLT_MAX), m_ymax(FLT_MAX), m_active(false)
{
}

QQuickPinchAreaPrivate::~QQuickPinchAreaPrivate()
{
    delete pinch;
}

/*!
    \qmltype PinchArea
    \instantiates QQuickPinchArea
    \inqmlmodule QtQuick
    \ingroup qtquick-input
    \inherits Item
    \brief Enables simple pinch gesture handling.

    \b {The PinchArea type was added in QtQuick 1.1}

    A PinchArea is an invisible item that is typically used in conjunction with
    a visible item in order to provide pinch gesture handling for that item.

    The \l enabled property is used to enable and disable pinch handling for
    the proxied item. When disabled, the pinch area becomes transparent to
    mouse/touch events.

    PinchArea can be used in two ways:

    \list
    \li setting a \c pinch.target to provide automatic interaction with an item
    \li using the onPinchStarted, onPinchUpdated and onPinchFinished handlers
    \endlist

    Since Qt 5.5, PinchArea can react to native pinch gesture events from the
    operating system if available; otherwise it reacts only to touch events.

    \sa PinchEvent, QNativeGestureEvent, QTouchEvent
*/

/*!
    \qmlsignal QtQuick::PinchArea::pinchStarted(PinchEvent pinch)

    This signal is emitted when the pinch area detects that a pinch gesture has
    started: two touch points (fingers) have been detected, and they have moved
    beyond the \l {QStyleHints}{startDragDistance} threshold for the gesture to begin.

    The \a pinch parameter (not the same as the \l {PinchArea}{pinch}
    property) provides information about the pinch gesture, including the scale,
    center and angle of the pinch. At the time of the \c pinchStarted signal,
    these values are reset to the default values, regardless of the results
    from previous gestures: pinch.scale will be \c 1.0 and pinch.rotation will be \c 0.0.
    As the gesture progresses, \l pinchUpdated will report the deviation from those
    defaults.

    To ignore this gesture set the \c pinch.accepted property to false.  The gesture
    will be canceled and no further events will be sent.
*/

/*!
    \qmlsignal QtQuick::PinchArea::pinchUpdated(PinchEvent pinch)

    This signal is emitted when the pinch area detects that a pinch gesture has changed.

    The \a pinch parameter provides information about the pinch
    gesture, including the scale, center and angle of the pinch. These values
    reflect changes only since the beginning of the current gesture, and
    therefore are not limited by the minimum and maximum limits in the
    \l {PinchArea}{pinch} property.
*/

/*!
    \qmlsignal QtQuick::PinchArea::pinchFinished(PinchEvent pinch)

    This signal is emitted when the pinch area detects that a pinch gesture has finished.

    The \a pinch parameter (not the same as the \l {PinchArea}{pinch}
    property) provides information about the pinch gesture, including the
    scale, center and angle of the pinch.
*/

/*!
    \qmlsignal QtQuick::PinchArea::smartZoom(PinchEvent pinch)
    \since 5.5

    This signal is emitted when the pinch area detects a smart zoom gesture.
    This gesture occurs only on certain operating systems such as \macos.

    The \a pinch parameter provides information about the pinch
    gesture, including the location where the gesture occurred.  \c pinch.scale
    will be greater than zero when the gesture indicates that the user wishes to
    enter smart zoom, and zero when exiting (even though typically the same gesture
    is used to toggle between the two states).
*/


/*!
    \qmlpropertygroup QtQuick::PinchArea::pinch
    \qmlproperty Item QtQuick::PinchArea::pinch.target
    \qmlproperty bool QtQuick::PinchArea::pinch.active
    \qmlproperty real QtQuick::PinchArea::pinch.minimumScale
    \qmlproperty real QtQuick::PinchArea::pinch.maximumScale
    \qmlproperty real QtQuick::PinchArea::pinch.minimumRotation
    \qmlproperty real QtQuick::PinchArea::pinch.maximumRotation
    \qmlproperty enumeration QtQuick::PinchArea::pinch.dragAxis
    \qmlproperty real QtQuick::PinchArea::pinch.minimumX
    \qmlproperty real QtQuick::PinchArea::pinch.maximumX
    \qmlproperty real QtQuick::PinchArea::pinch.minimumY
    \qmlproperty real QtQuick::PinchArea::pinch.maximumY

    \c pinch provides a convenient way to make an item react to pinch gestures.

    \list
    \li \c pinch.target specifies the id of the item to drag.
    \li \c pinch.active specifies if the target item is currently being dragged.
    \li \c pinch.minimumScale and \c pinch.maximumScale limit the range of the Item.scale property, but not the \c PinchEvent \l {PinchEvent}{scale} property.
    \li \c pinch.minimumRotation and \c pinch.maximumRotation limit the range of the Item.rotation property, but not the \c PinchEvent \l {PinchEvent}{rotation} property.
    \li \c pinch.dragAxis specifies whether dragging in not allowed (\c Pinch.NoDrag), can be done horizontally (\c Pinch.XAxis), vertically (\c Pinch.YAxis), or both (\c Pinch.XAndYAxis)
    \li \c pinch.minimum and \c pinch.maximum limit how far the target can be dragged along the corresponding axes.
    \endlist
*/

QQuickPinchArea::QQuickPinchArea(QQuickItem *parent)
  : QQuickItem(*(new QQuickPinchAreaPrivate), parent)
{
    Q_D(QQuickPinchArea);
    d->init();
    setAcceptTouchEvents(true);
#ifdef Q_OS_MACOS
    setAcceptHoverEvents(true); // needed to enable touch events on mouse hover.
#endif
}

QQuickPinchArea::~QQuickPinchArea()
{
}
/*!
    \qmlproperty bool QtQuick::PinchArea::enabled
    This property holds whether the item accepts pinch gestures.

    This property defaults to true.
*/
bool QQuickPinchArea::isEnabled() const
{
    Q_D(const QQuickPinchArea);
    return d->enabled;
}

void QQuickPinchArea::setEnabled(bool a)
{
    Q_D(QQuickPinchArea);
    if (a != d->enabled) {
        d->enabled = a;
        emit enabledChanged();
    }
}

void QQuickPinchArea::touchEvent(QTouchEvent *event)
{
    Q_D(QQuickPinchArea);
    if (!d->enabled || !isVisible()) {
        QQuickItem::touchEvent(event);
        return;
    }

    // A common non-trivial starting scenario is the user puts down one finger,
    // then that finger remains stationary while putting down a second one.
    // However QQuickWindow will not send TouchUpdates for TouchPoints which
    // were not initially accepted; that would be inefficient and noisy.
    // So even if there is only one touchpoint so far, it's important to accept it
    // in order to get updates later on (and it's accepted by default anyway).
    // If the user puts down one finger, we're waiting for the other finger to drop.
    // Therefore updatePinch() must do the right thing for any combination of
    // points and states that may occur, and there is no reason to ignore any event.
    // One consequence though is that if PinchArea is on top of something else,
    // it's always going to accept the touches, and that means the item underneath
    // will not get them (unless the PA's parent is doing parent filtering,
    // as the Flickable does, for example).
    switch (event->type()) {
    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
        d->touchPoints.clear();
        for (int i = 0; i < event->pointCount(); ++i) {
            auto &tp = event->point(i);
            if (tp.state() != QEventPoint::State::Released) {
                d->touchPoints << tp;
                tp.setAccepted();
            }
        }
        updatePinch(event, false);
        break;
    case QEvent::TouchEnd:
        clearPinch(event);
        break;
    case QEvent::TouchCancel:
        cancelPinch(event);
        break;
    default:
        QQuickItem::touchEvent(event);
    }
}

void QQuickPinchArea::clearPinch(QTouchEvent *event)
{
    Q_D(QQuickPinchArea);
    qCDebug(lcPA, "clear: %" PRIdQSIZETYPE " touchpoints", d->touchPoints.size());
    d->touchPoints.clear();
    if (d->inPinch) {
        d->inPinch = false;
        QPointF pinchCenter = mapFromScene(d->sceneLastCenter);
        QQuickPinchEvent pe(pinchCenter, d->pinchLastScale, d->pinchLastAngle, d->pinchRotation);
        pe.setStartCenter(d->pinchStartCenter);
        pe.setPreviousCenter(pinchCenter);
        pe.setPreviousAngle(d->pinchLastAngle);
        pe.setPreviousScale(d->pinchLastScale);
        pe.setStartPoint1(mapFromScene(d->sceneStartPoint1));
        pe.setStartPoint2(mapFromScene(d->sceneStartPoint2));
        pe.setPoint1(mapFromScene(d->lastPoint1));
        pe.setPoint2(mapFromScene(d->lastPoint2));
        emit pinchFinished(&pe);
        if (d->pinch && d->pinch->target())
            d->pinch->setActive(false);
    }
    d->pinchStartDist = 0;
    d->pinchActivated = false;
    d->initPinch = false;
    d->pinchRejected = false;
    d->id1 = -1;
    if (event) {
        for (const auto &point : event->points()) {
            if (event->exclusiveGrabber(point) == this)
                event->setExclusiveGrabber(point, nullptr);
        }
    }
    setKeepTouchGrab(false);
    setKeepMouseGrab(false);
}

void QQuickPinchArea::cancelPinch(QTouchEvent *event)
{
    Q_D(QQuickPinchArea);
    qCDebug(lcPA, "cancel: %" PRIdQSIZETYPE " touchpoints", d->touchPoints.size());
    d->touchPoints.clear();
    if (d->inPinch) {
        d->inPinch = false;
        QPointF pinchCenter = mapFromScene(d->sceneLastCenter);
        QQuickPinchEvent pe(d->pinchStartCenter, d->pinchStartScale, d->pinchStartAngle, d->pinchStartRotation);
        pe.setStartCenter(d->pinchStartCenter);
        pe.setPreviousCenter(pinchCenter);
        pe.setPreviousAngle(d->pinchLastAngle);
        pe.setPreviousScale(d->pinchLastScale);
        pe.setStartPoint1(mapFromScene(d->sceneStartPoint1));
        pe.setStartPoint2(mapFromScene(d->sceneStartPoint2));
        pe.setPoint1(pe.startPoint1());
        pe.setPoint2(pe.startPoint2());
        emit pinchFinished(&pe);

        d->pinchLastScale = d->pinchStartScale;
        d->sceneLastCenter = d->sceneStartCenter;
        d->pinchLastAngle = d->pinchStartAngle;
        d->lastPoint1 = pe.startPoint1();
        d->lastPoint2 = pe.startPoint2();
        updatePinchTarget();

        if (d->pinch && d->pinch->target())
            d->pinch->setActive(false);
    }
    d->pinchStartDist = 0;
    d->pinchActivated = false;
    d->initPinch = false;
    d->pinchRejected = false;
    d->id1 = -1;
    for (const auto &point : event->points()) {
        if (event->exclusiveGrabber(point) == this)
            event->setExclusiveGrabber(point, nullptr);
    }
    setKeepTouchGrab(false);
    setKeepMouseGrab(false);
}

void QQuickPinchArea::updatePinch(QTouchEvent *event, bool filtering)
{
    Q_D(QQuickPinchArea);

    if (d->touchPoints.size() < 2) {
        // A pinch gesture is not occurring, so stealing the grab is permitted.
        setKeepTouchGrab(false);
        setKeepMouseGrab(false);
        // During filtering, there's no need to hold a grab for one point,
        // because filtering happens for every event anyway.
        // But if we receive the event via direct delivery, and give up the grab,
        // not only will we not see any more updates, but any filtering parent
        // (such as Flickable) will also not get a chance to filter them.
        // Continuing to hold the grab in this case keeps tst_TouchMouse::pinchOnFlickable() working.
        if (filtering && !d->touchPoints.isEmpty() && event->exclusiveGrabber(d->touchPoints.first()) == this)
            event->setExclusiveGrabber(d->touchPoints.first(), nullptr);
    }

    if (d->touchPoints.size() == 0) {
        if (d->inPinch) {
            d->inPinch = false;
            QPointF pinchCenter = mapFromScene(d->sceneLastCenter);
            QQuickPinchEvent pe(pinchCenter, d->pinchLastScale, d->pinchLastAngle, d->pinchRotation);
            pe.setStartCenter(d->pinchStartCenter);
            pe.setPreviousCenter(pinchCenter);
            pe.setPreviousAngle(d->pinchLastAngle);
            pe.setPreviousScale(d->pinchLastScale);
            pe.setStartPoint1(mapFromScene(d->sceneStartPoint1));
            pe.setStartPoint2(mapFromScene(d->sceneStartPoint2));
            pe.setPoint1(mapFromScene(d->lastPoint1));
            pe.setPoint2(mapFromScene(d->lastPoint2));
            setKeepTouchGrab(false);
            setKeepMouseGrab(false);
            emit pinchFinished(&pe);
            d->pinchStartDist = 0;
            d->pinchActivated = false;
            if (d->pinch && d->pinch->target())
                d->pinch->setActive(false);
        }
        d->initPinch = false;
        d->pinchRejected = false;
        return;
    }

    QEventPoint touchPoint1 = d->touchPoints.at(0);
    QEventPoint touchPoint2 = d->touchPoints.at(d->touchPoints.size() >= 2 ? 1 : 0);

    if (touchPoint1.state() == QEventPoint::State::Pressed)
        d->sceneStartPoint1 = touchPoint1.scenePosition();

    if (touchPoint2.state() == QEventPoint::State::Pressed)
        d->sceneStartPoint2 = touchPoint2.scenePosition();

    qCDebug(lcPA) << "updating based on" << touchPoint1 << touchPoint2;

    QRectF bounds = clipRect();
    // Pinch is not started unless there are exactly two touch points
    // AND one or more of the points has just now been pressed (wasn't pressed already)
    // AND both points are inside the bounds.
    if (d->touchPoints.size() == 2
            && (touchPoint1.state() == QEventPoint::State::Pressed || touchPoint2.state() == QEventPoint::State::Pressed) &&
            bounds.contains(touchPoint1.position()) && bounds.contains(touchPoint2.position())) {
        d->id1 = touchPoint1.id();
        if (!d->pinchActivated)
             qCDebug(lcPA, "pinch activating");
        d->pinchActivated = true;
        d->initPinch = true;
        event->setExclusiveGrabber(touchPoint1, this);
        event->setExclusiveGrabber(touchPoint2, this);
        setKeepTouchGrab(true);
        setKeepMouseGrab(true);
    }
    if (d->pinchActivated && !d->pinchRejected) {
        const int dragThreshold = QGuiApplication::styleHints()->startDragDistance();
        QPointF p1 = touchPoint1.scenePosition();
        QPointF p2 = touchPoint2.scenePosition();
        qreal dx = p1.x() - p2.x();
        qreal dy = p1.y() - p2.y();
        qreal dist = qSqrt(dx*dx + dy*dy);
        QPointF sceneCenter = (p1 + p2)/2;
        qreal angle = QLineF(p1, p2).angle();
        if (d->touchPoints.size() == 1) {
            // If we only have one point then just move the center
            if (d->id1 == touchPoint1.id())
                sceneCenter = d->sceneLastCenter + touchPoint1.scenePosition() - d->lastPoint1;
            else
                sceneCenter = d->sceneLastCenter + touchPoint2.scenePosition() - d->lastPoint2;
            angle = d->pinchLastAngle;
        }
        d->id1 = touchPoint1.id();
        if (angle > 180)
            angle -= 360;
        qCDebug(lcPA, "pinch \u2316 %.1lf,%.1lf \u21e4%.1lf\u21e5 \u2220 %.1lf",
                sceneCenter.x(), sceneCenter.y(), dist, angle);
        if (!d->inPinch || d->initPinch) {
            if (d->touchPoints.size() >= 2) {
                if (d->initPinch) {
                    if (!d->inPinch)
                        d->pinchStartDist = dist;
                    d->initPinch = false;
                }
                d->sceneStartCenter = sceneCenter;
                d->sceneLastCenter = sceneCenter;
                d->pinchStartCenter = mapFromScene(sceneCenter);
                d->pinchStartAngle = angle;
                d->pinchLastScale = 1.0;
                d->pinchLastAngle = angle;
                d->pinchRotation = 0.0;
                d->lastPoint1 = p1;
                d->lastPoint2 = p2;
                if (qAbs(dist - d->pinchStartDist) >= dragThreshold ||
                        (pinch()->axis() != QQuickPinch::NoDrag &&
                         (qAbs(p1.x()-d->sceneStartPoint1.x()) >= dragThreshold
                          || qAbs(p1.y()-d->sceneStartPoint1.y()) >= dragThreshold
                          || qAbs(p2.x()-d->sceneStartPoint2.x()) >= dragThreshold
                          || qAbs(p2.y()-d->sceneStartPoint2.y()) >= dragThreshold))) {
                    QQuickPinchEvent pe(d->pinchStartCenter, 1.0, angle, 0.0);
                    d->pinchStartDist = dist;
                    pe.setStartCenter(d->pinchStartCenter);
                    pe.setPreviousCenter(d->pinchStartCenter);
                    pe.setPreviousAngle(d->pinchLastAngle);
                    pe.setPreviousScale(d->pinchLastScale);
                    pe.setStartPoint1(mapFromScene(d->sceneStartPoint1));
                    pe.setStartPoint2(mapFromScene(d->sceneStartPoint2));
                    pe.setPoint1(mapFromScene(d->lastPoint1));
                    pe.setPoint2(mapFromScene(d->lastPoint2));
                    pe.setPointCount(d->touchPoints.size());
                    emit pinchStarted(&pe);
                    if (pe.accepted()) {
                        d->inPinch = true;
                        event->setExclusiveGrabber(touchPoint1, this);
                        event->setExclusiveGrabber(touchPoint2, this);
                        setKeepTouchGrab(true);
                        // So that PinchArea works in PathView, grab mouse events too.
                        // We should be able to remove these setKeepMouseGrab calls when QTBUG-105567 is fixed.
                        setKeepMouseGrab(true);
                        d->inPinch = true;
                        if (d->pinch && d->pinch->target()) {
                            auto targetParent = pinch()->target()->parentItem();
                            d->pinchStartPos = targetParent ?
                                        targetParent->mapToScene(pinch()->target()->position()) :
                                        pinch()->target()->position();
                            d->pinchStartScale = d->pinch->target()->scale();
                            d->pinchStartRotation = d->pinch->target()->rotation();
                            d->pinch->setActive(true);
                        }
                    } else {
                        d->pinchRejected = true;
                    }
                }
            }
        } else if (d->pinchStartDist > 0) {
            qreal scale = dist ? dist / d->pinchStartDist : d->pinchLastScale;
            qreal da = d->pinchLastAngle - angle;
            if (da > 180)
                da -= 360;
            else if (da < -180)
                da += 360;
            d->pinchRotation += da;
            QPointF pinchCenter = mapFromScene(sceneCenter);
            QQuickPinchEvent pe(pinchCenter, scale, angle, d->pinchRotation);
            pe.setStartCenter(d->pinchStartCenter);
            pe.setPreviousCenter(mapFromScene(d->sceneLastCenter));
            pe.setPreviousAngle(d->pinchLastAngle);
            pe.setPreviousScale(d->pinchLastScale);
            pe.setStartPoint1(mapFromScene(d->sceneStartPoint1));
            pe.setStartPoint2(mapFromScene(d->sceneStartPoint2));
            pe.setPoint1(touchPoint1.position());
            pe.setPoint2(touchPoint2.position());
            pe.setPointCount(d->touchPoints.size());
            d->pinchLastScale = scale;
            d->sceneLastCenter = sceneCenter;
            d->pinchLastAngle = angle;
            d->lastPoint1 = touchPoint1.scenePosition();
            d->lastPoint2 = touchPoint2.scenePosition();
            emit pinchUpdated(&pe);
            updatePinchTarget();
        }
    }
}

void QQuickPinchArea::updatePinchTarget()
{
    Q_D(QQuickPinchArea);
    if (d->pinch && d->pinch->target()) {
        qreal s = d->pinchStartScale * d->pinchLastScale;
        s = qMin(qMax(pinch()->minimumScale(),s), pinch()->maximumScale());
        pinch()->target()->setScale(s);
        QPointF pos = d->sceneLastCenter - d->sceneStartCenter + d->pinchStartPos;
        if (auto targetParent = pinch()->target()->parentItem())
            pos = targetParent->mapFromScene(pos);

        if (pinch()->axis() & QQuickPinch::XAxis) {
            qreal x = pos.x();
            if (x < pinch()->xmin())
                x = pinch()->xmin();
            else if (x > pinch()->xmax())
                x = pinch()->xmax();
            pinch()->target()->setX(x);
        }
        if (pinch()->axis() & QQuickPinch::YAxis) {
            qreal y = pos.y();
            if (y < pinch()->ymin())
                y = pinch()->ymin();
            else if (y > pinch()->ymax())
                y = pinch()->ymax();
            pinch()->target()->setY(y);
        }
        if (d->pinchStartRotation >= pinch()->minimumRotation()
                && d->pinchStartRotation <= pinch()->maximumRotation()) {
            qreal r = d->pinchRotation + d->pinchStartRotation;
            r = qMin(qMax(pinch()->minimumRotation(),r), pinch()->maximumRotation());
            pinch()->target()->setRotation(r);
        }
    }
}

/*! \internal
    PinchArea needs to filter touch events going to its children: in case
    one of them stops event propagation by accepting the touch event, filtering
    is the only way PinchArea can see the touch event.

    This method is called childMouseEventFilter instead of childPointerEventFilter
    for historical reasons, but actually filters all pointer events (and the
    occasional QEvent::UngrabMouse).
*/
bool QQuickPinchArea::childMouseEventFilter(QQuickItem *i, QEvent *e)
{
    Q_D(QQuickPinchArea);
    if (!d->enabled || !isVisible())
        return QQuickItem::childMouseEventFilter(i, e);
    auto *te = static_cast<QTouchEvent*>(e);
    switch (e->type()) {
    case QEvent::TouchBegin:
        clearPinch(te);
        Q_FALLTHROUGH();
    case QEvent::TouchUpdate: {
            const auto &points = te->points();
            d->touchPoints.clear();
            for (auto &tp : points) {
                if (tp.state() != QEventPoint::State::Released)
                    d->touchPoints << tp;
            }
            updatePinch(te, true);
        }
        e->setAccepted(d->inPinch);
        return d->inPinch;
    case QEvent::TouchEnd:
        clearPinch(te);
        break;
    default:
        break;
    }

    return QQuickItem::childMouseEventFilter(i, e);
}

void QQuickPinchArea::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

void QQuickPinchArea::itemChange(ItemChange change, const ItemChangeData &value)
{
    QQuickItem::itemChange(change, value);
}

bool QQuickPinchArea::event(QEvent *event)
{
    Q_D(QQuickPinchArea);
    if (!d->enabled || !isVisible())
        return QQuickItem::event(event);

    switch (event->type()) {
#if QT_CONFIG(gestures)
    case QEvent::NativeGesture: {
        QNativeGestureEvent *gesture = static_cast<QNativeGestureEvent *>(event);
        switch (gesture->gestureType()) {
        case Qt::BeginNativeGesture:
            clearPinch(nullptr); // probably not necessary; JIC
            d->pinchStartCenter = gesture->position();
            d->pinchStartAngle = 0.0;
            d->pinchStartRotation = 0.0;
            d->pinchRotation = 0.0;
            d->pinchStartScale = 1.0;
            d->pinchLastAngle = 0.0;
            d->pinchLastScale = 1.0;
            d->sceneStartPoint1 = gesture->scenePosition();
            d->sceneStartPoint2 = gesture->scenePosition(); // TODO we never really know
            d->lastPoint1 = gesture->scenePosition();
            d->lastPoint2 = gesture->scenePosition(); // TODO we never really know
            if (d->pinch && d->pinch->target()) {
                d->pinchStartPos = d->pinch->target()->position();
                d->pinchStartScale = d->pinch->target()->scale();
                d->pinchStartRotation = d->pinch->target()->rotation();
                d->pinch->setActive(true);
            }
            break;
        case Qt::EndNativeGesture:
            clearPinch(nullptr);
            break;
        case Qt::ZoomNativeGesture: {
            if (d->pinchRejected)
                break;
            qreal scale = d->pinchLastScale * (1.0 + gesture->value());
            QQuickPinchEvent pe(d->pinchStartCenter, scale, d->pinchLastAngle, 0.0);
            pe.setStartCenter(d->pinchStartCenter);
            pe.setPreviousCenter(d->pinchStartCenter);
            pe.setPreviousAngle(d->pinchLastAngle);
            pe.setPreviousScale(d->pinchLastScale);
            pe.setStartPoint1(mapFromScene(d->sceneStartPoint1));
            pe.setStartPoint2(mapFromScene(d->sceneStartPoint2));
            pe.setPoint1(mapFromScene(d->lastPoint1));
            pe.setPoint2(mapFromScene(d->lastPoint2));
            pe.setPointCount(2);
            d->pinchLastScale = scale;
            if (d->inPinch)
                emit pinchUpdated(&pe);
            else
                emit pinchStarted(&pe);
            d->inPinch = true;
            if (pe.accepted())
                updatePinchTarget();
            else
                d->pinchRejected = true;
        } break;
        case Qt::SmartZoomNativeGesture: {
            if (gesture->value() > 0.0 && d->pinch && d->pinch->target()) {
                d->pinchStartPos = pinch()->target()->position();
                d->pinchStartCenter = mapToItem(pinch()->target()->parentItem(), pinch()->target()->boundingRect().center());
                d->pinchStartScale = d->pinch->target()->scale();
                d->pinchStartRotation = d->pinch->target()->rotation();
                d->pinchLastScale = d->pinchStartScale = d->pinch->target()->scale();
                d->pinchLastAngle = d->pinchStartRotation = d->pinch->target()->rotation();
            }
            QQuickPinchEvent pe(gesture->position(), gesture->value(), d->pinchLastAngle, 0.0);
            pe.setStartCenter(gesture->position());
            pe.setPreviousCenter(d->pinchStartCenter);
            pe.setPreviousAngle(d->pinchLastAngle);
            pe.setPreviousScale(d->pinchLastScale);
            pe.setStartPoint1(gesture->position());
            pe.setStartPoint2(gesture->position());
            pe.setPoint1(mapFromScene(gesture->scenePosition()));
            pe.setPoint2(mapFromScene(gesture->scenePosition()));
            pe.setPointCount(2);
            emit smartZoom(&pe);
        } break;
        case Qt::RotateNativeGesture: {
            if (d->pinchRejected)
                break;
            qreal angle = d->pinchLastAngle + gesture->value();
            QQuickPinchEvent pe(d->pinchStartCenter, d->pinchLastScale, angle, 0.0);
            pe.setStartCenter(d->pinchStartCenter);
            pe.setPreviousCenter(d->pinchStartCenter);
            pe.setPreviousAngle(d->pinchLastAngle);
            pe.setPreviousScale(d->pinchLastScale);
            pe.setStartPoint1(mapFromScene(d->sceneStartPoint1));
            pe.setStartPoint2(mapFromScene(d->sceneStartPoint2));
            pe.setPoint1(mapFromScene(d->lastPoint1));
            pe.setPoint2(mapFromScene(d->lastPoint2));
            pe.setPointCount(2);
            d->pinchLastAngle = angle;
            if (d->inPinch)
                emit pinchUpdated(&pe);
            else
                emit pinchStarted(&pe);
            d->inPinch = true;
            d->pinchRotation = angle;
            if (pe.accepted())
                updatePinchTarget();
            else
                d->pinchRejected = true;
        } break;
        default:
            return QQuickItem::event(event);
        }
    } break;
#endif // gestures
    case QEvent::Wheel:
        event->ignore();
        return false;
    default:
         return QQuickItem::event(event);
    }

    return true;
}

QQuickPinch *QQuickPinchArea::pinch()
{
    Q_D(QQuickPinchArea);
    if (!d->pinch)
        d->pinch = new QQuickPinch;
    return d->pinch;
}

QT_END_NAMESPACE

#include "moc_qquickpincharea_p.cpp"
