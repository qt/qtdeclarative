// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpinchhandler_p.h"
#include <QtQml/qqmlinfo.h>
#include <QtQuick/qquickwindow.h>
#include <private/qsgadaptationlayer_p.h>
#include <private/qquickitem_p.h>
#include <private/qguiapplication_p.h>
#include <private/qquickmultipointhandler_p_p.h>
#include <private/qquickwindow_p.h>
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>
#include <math.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPinchHandler, "qt.quick.handler.pinch")

/*!
    \qmltype PinchHandler
    \instantiates QQuickPinchHandler
    \inherits MultiPointHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-input-handlers
    \brief Handler for pinch gestures.

    PinchHandler is a handler that interprets a multi-finger gesture to
    interactively rotate, zoom, and drag an Item. Like other Input Handlers,
    by default it is fully functional, and manipulates its \l target,
    which is the Item within which it is declared.

    \snippet pointerHandlers/pinchHandler.qml 0

    It has properties to restrict the range of dragging, rotation, and zoom.

    If it is declared within one Item but is assigned a different \l target, it
    handles events within the bounds of the outer Item but manipulates the
    \c target Item instead:

    \snippet pointerHandlers/pinchHandlerDifferentTarget.qml 0

    A third way to use it is to set \l target to \c null and react to property
    changes in some other way:

    \snippet pointerHandlers/pinchHandlerNullTarget.qml 0

    \image touchpoints-pinchhandler.png

    \note The pinch begins when the number of fingers pressed is between
    \l {MultiPointHandler::minimumPointCount}{minimumPointCount} and
    \l {MultiPointHandler::maximumPointCount}{maximumPointCount}, inclusive.
    Until then, PinchHandler tracks the positions of any pressed fingers,
    but if it's a disallowed number, it does not scale or rotate
    its \l target, and the \l active property remains \c false.

    \sa PinchArea, QPointerEvent::pointCount(), QNativeGestureEvent::fingerCount(), {Qt Quick Examples - Pointer Handlers}
*/

QQuickPinchHandler::QQuickPinchHandler(QQuickItem *parent)
    : QQuickMultiPointHandler(parent, 2)
{
    // Tell QQuickPointerDeviceHandler::wantsPointerEvent() to ignore button state
    d_func()->acceptedButtons = Qt::NoButton;
}

#if QT_DEPRECATED_SINCE(6, 5)
/*!
    \qmlproperty real QtQuick::PinchHandler::minimumScale
    \deprecated [6.5] Use scaleAxis.minimum

    The minimum acceptable \l {Item::scale}{scale} to be applied
    to the \l target.
*/
void QQuickPinchHandler::setMinimumScale(qreal minimumScale)
{
    if (qFuzzyCompare(m_scaleAxis.minimum(), minimumScale))
        return;

    m_scaleAxis.setMinimum(minimumScale);
    emit minimumScaleChanged();
}

/*!
    \qmlproperty real QtQuick::PinchHandler::maximumScale
    \deprecated [6.5] Use scaleAxis.maximum

    The maximum acceptable \l {Item::scale}{scale} to be applied
    to the \l target.
*/
void QQuickPinchHandler::setMaximumScale(qreal maximumScale)
{
    if (qFuzzyCompare(m_scaleAxis.maximum(), maximumScale))
        return;

    m_scaleAxis.setMaximum(maximumScale);
    emit maximumScaleChanged();
}
#endif

/*!
    \readonly
    \qmlproperty real QtQuick::PinchHandler::activeScale

    The scale factor while the pinch gesture is being performed.
    It is 1.0 when the gesture begins, increases as the touchpoints are spread
    apart, and decreases as the touchpoints are brought together.
    If \l target is not null, its \l {Item::scale}{scale} will be automatically
    multiplied by this value.
    Otherwise, bindings can be used to do arbitrary things with this value.

    \sa QtQuick::PinchHandler::scaleAxis.activeValue
*/

void QQuickPinchHandler::setActiveScale(qreal scale)
{
    if (scale == activeScale())
        return;

    qreal delta = scale / m_scaleAxis.activeValue();
    m_scaleAxis.updateValue(scale, m_scaleAxis.m_startValue * scale, delta);
    emit scaleChanged(delta);
}

/*!
    \qmlsignal QtQuick::PinchHandler::scaleChanged(qreal delta)

    The \c scaleChanged signal is emitted when \l activeScale (and therefore
    \l persistentScale) changes. The \a delta value gives the multiplicative
    change in scale. For example, if the user moves fingers to change the pinch
    distance so that \c activeScale changes from 2 to 2.5, \c
    scaleChanged(1.25) will be emitted. You can use that to incrementally
    change the scale of an item:

    \snippet pointerHandlers/pinchHandlerScaleOrRotationChanged.qml 0

    \note If you set the \l persistentScale property directly, \c delta is \c 1.
*/

/*!
    \readonly
    \qmlproperty QVector2D QtQuick::PinchHandler::scale
    \deprecated [6.5] Use persistentScale
*/

/*!
    \qmlproperty real QtQuick::PinchHandler::persistentScale

    The scale factor that will automatically be set on the \l target if it is not null.
    Otherwise, bindings can be used to do arbitrary things with this value.
    While the pinch gesture is being performed, it is continuously multiplied by
    \l activeScale; after the gesture ends, it stays the same; and when the next
    pinch gesture begins, it begins to be multiplied by activeScale again.

    It's possible to set this property, as a way of synchronizing the basis
    scale with a scale that was set in some other way, for example by another
    handler. If you set this property directly, \c activeScale does not change,
    and \c scaleChanged(1) is emitted.
*/

void QQuickPinchHandler::setPersistentScale(qreal scale)
{
    if (scale == persistentScale())
        return;

    m_scaleAxis.updateValue(m_scaleAxis.activeValue(), scale);
    emit scaleChanged(1);
}

#if QT_DEPRECATED_SINCE(6, 5)
/*!
    \qmlproperty real QtQuick::PinchHandler::minimumRotation
    \deprecated [6.5] Use rotationAxis.minimum

    The minimum acceptable \l {Item::rotation}{rotation} to be applied
    to the \l target.
*/
void QQuickPinchHandler::setMinimumRotation(qreal minimumRotation)
{
    if (qFuzzyCompare(m_rotationAxis.minimum(), minimumRotation))
        return;

    m_rotationAxis.setMinimum(minimumRotation);
    emit minimumRotationChanged();
}

/*!
    \qmlproperty real QtQuick::PinchHandler::maximumRotation
    \deprecated [6.5] Use rotationAxis.maximum

    The maximum acceptable \l {Item::rotation}{rotation} to be applied
    to the \l target.
*/
void QQuickPinchHandler::setMaximumRotation(qreal maximumRotation)
{
    if (qFuzzyCompare(m_rotationAxis.maximum(), maximumRotation))
        return;

    m_rotationAxis.setMaximum(maximumRotation);
    emit maximumRotationChanged();
}
#endif

/*!
    \qmlsignal QtQuick::PinchHandler::rotationChanged(qreal delta)

    The \c rotationChanged signal is emitted when \l activeRotation (and
    therefore \l persistentRotation) changes. The \a delta value gives the
    additive change in rotation. For example, if the user moves fingers to
    change the pinch distance so that \c activeRotation changes from 10 to 30
    degrees, \c rotationChanged(20) will be emitted. You can use that to
    incrementally change the rotation of an item:

    \snippet pointerHandlers/pinchHandlerScaleOrRotationChanged.qml 0

    \note If you set the \l persistentRotation property directly, \c delta is \c 0.
*/

/*!
    \readonly
    \qmlproperty QVector2D QtQuick::PinchHandler::rotation
    \deprecated [6.5] Use activeRotation
*/

/*!
    \readonly
    \qmlproperty real QtQuick::PinchHandler::activeRotation

    The rotation of the pinch gesture in degrees, with positive values clockwise.
    It is \c 0 when the gesture begins. If \l target is not null, this will be
    automatically added to its \l {Item::rotation}{rotation}. Otherwise,
    bindings can be used to do arbitrary things with this value.

    \sa QtQuick::PinchHandler::rotationAxis.activeValue
*/

void QQuickPinchHandler::setActiveRotation(qreal rot)
{
    if (rot == activeRotation())
        return;

    qreal delta = rot - m_rotationAxis.activeValue();
    m_rotationAxis.updateValue(rot, m_rotationAxis.m_startValue + rot, delta);
    emit rotationChanged(delta);
}

/*!
    \qmlproperty real QtQuick::PinchHandler::persistentRotation

    The rotation to be applied to the \l target if it is not null.
    Otherwise, bindings can be used to do arbitrary things with this value.
    While the pinch gesture is being performed, \l activeRotation is continuously
    added; after the gesture ends, it stays the same; and when the next
    pinch gesture begins, it begins to be modified by activeRotation again.

    It's possible to set this property, as a way of synchronizing the basis
    rotation with a rotation that was set in some other way, for example by
    another handler. If you set this property directly, \c activeRotation does
    not change, and \c rotationChanged(0) is emitted.
*/

void QQuickPinchHandler::setPersistentRotation(qreal rot)
{
    if (rot == persistentRotation())
        return;

    m_rotationAxis.updateValue(m_rotationAxis.activeValue(), rot);
    emit rotationChanged(0);
}

/*!
    \qmlsignal QtQuick::PinchHandler::translationChanged(QVector2D delta)

    The \c translationChanged signal is emitted when \l activeTranslation (and
    therefore \l persistentTranslation) changes. The \a delta vector gives the
    change in translation. You can use that to incrementally change the
    position of an item:

    \snippet pointerHandlers/pinchHandlerNullTarget.qml 0

    \note If you set the \l persistentTranslation property directly,
    \c delta is \c {0, 0}.
*/

/*!
    \readonly
    \qmlproperty QVector2D QtQuick::PinchHandler::translation
    \deprecated [6.5] Use activeTranslation
*/
/*!
    \readonly
    \qmlproperty QPointF QtQuick::PinchHandler::activeTranslation

    The translation of the cluster of points while the pinch gesture is being
    performed. It is \c {0, 0} when the gesture begins, and increases as the
    \l {eventPoint}{eventPoint(s)} are dragged downward and to the right. After the gesture
    ends, it stays the same; and when the next pinch gesture begins, it is
    reset to \c {0, 0} again.

    \note On some touchpads, such as on a \macos trackpad, native gestures do
    not generate any translation values, and this property stays at \c (0, 0).
*/

/*!
    \qmlproperty QPointF QtQuick::PinchHandler::persistentTranslation

    The translation to be applied to the \l target if it is not \c null.
    Otherwise, bindings can be used to do arbitrary things with this value.
    While the pinch gesture is being performed, \l activeTranslation is
    continuously added to it; after the gesture ends, it stays the same.

    It's possible to set this property, as a way of synchronizing the basis
    translation with a translation that was set in some other way, for example
    by another handler. If you set this property directly, \c activeTranslation
    does not change, and \c translationChanged({0, 0}) is emitted.

    \note On some touchpads, such as on a \macos trackpad, native gestures do
    not generate any translation values, and this property stays at \c (0, 0).
*/

void QQuickPinchHandler::setPersistentTranslation(const QPointF &trans)
{
    if (trans == persistentTranslation())
        return;

    m_xAxis.updateValue(m_xAxis.activeValue(), trans.x());
    m_yAxis.updateValue(m_yAxis.activeValue(), trans.y());
    emit translationChanged({});
}

bool QQuickPinchHandler::wantsPointerEvent(QPointerEvent *event)
{
    if (!QQuickMultiPointHandler::wantsPointerEvent(event))
        return false;

#if QT_CONFIG(gestures)
    if (event->type() == QEvent::NativeGesture) {
        const auto gesture = static_cast<const QNativeGestureEvent *>(event);
        if (!gesture->fingerCount() || (gesture->fingerCount() >= minimumPointCount() &&
                                        gesture->fingerCount() <= maximumPointCount())) {
            switch (gesture->gestureType()) {
            case Qt::BeginNativeGesture:
            case Qt::EndNativeGesture:
            case Qt::ZoomNativeGesture:
            case Qt::RotateNativeGesture:
                return parentContains(event->point(0));
            default:
                return false;
            }
        } else {
            return false;
        }
    }
#endif

    return true;
}

/*!
    \qmlpropertygroup QtQuick::PinchHandler::xAxis
    \qmlproperty real QtQuick::PinchHandler::xAxis.minimum
    \qmlproperty real QtQuick::PinchHandler::xAxis.maximum
    \qmlproperty bool QtQuick::PinchHandler::xAxis.enabled
    \qmlproperty real QtQuick::PinchHandler::xAxis.activeValue

    \c xAxis controls the constraints for horizontal translation of the \l target item.

    \c minimum is the minimum acceptable x coordinate of the translation.
    \c maximum is the maximum acceptable x coordinate of the translation.
    If \c enabled is true, horizontal dragging is allowed.

    The \c activeValueChanged signal is emitted when \c activeValue changes, to
    provide the increment by which it changed.
    This is intended for incrementally adjusting one property via multiple handlers.

    \snippet pointerHandlers/pinchHandlerAxisValueDeltas.qml 0

    \note The snippet is contrived: PinchHandler already knows how to move,
    scale and rotate its parent item, but this code achieves different behavior
    in a less-declarative way, to illustrate how to use \c activeValueChanged
    in special cases.
*/

/*!
    \qmlpropertygroup QtQuick::PinchHandler::yAxis
    \qmlproperty real QtQuick::PinchHandler::yAxis.minimum
    \qmlproperty real QtQuick::PinchHandler::yAxis.maximum
    \qmlproperty bool QtQuick::PinchHandler::yAxis.enabled
    \qmlproperty real QtQuick::PinchHandler::yAxis.activeValue

    \c yAxis controls the constraints for vertical translation of the \l target item.

    \c minimum is the minimum acceptable y coordinate of the translation.
    \c maximum is the maximum acceptable y coordinate of the translation.
    If \c enabled is true, vertical dragging is allowed.

    The \c activeValueChanged signal is emitted when \c activeValue changes, to
    provide the increment by which it changed.
    This is intended for incrementally adjusting one property via multiple handlers.

    \snippet pointerHandlers/pinchHandlerAxisValueDeltas.qml 0

    \note The snippet is contrived: PinchHandler already knows how to move,
    scale and rotate its parent item, but this code achieves different behavior
    in a less-declarative way, to illustrate how to use \c activeValueChanged
    in special cases.
*/

/*!
    \qmlpropertygroup QtQuick::PinchHandler::scaleAxis
    \qmlproperty real QtQuick::PinchHandler::scaleAxis.minimum
    \qmlproperty real QtQuick::PinchHandler::scaleAxis.maximum
    \qmlproperty bool QtQuick::PinchHandler::scaleAxis.enabled
    \qmlproperty real QtQuick::PinchHandler::scaleAxis.activeValue

    \c scaleAxis controls the constraints for setting the \l {QtQuick::Item::scale}{scale}
    of the \l target item according to the distance between the touchpoints.

    \c minimum is the minimum acceptable scale.
    \c maximum is the maximum acceptable scale.
    If \c enabled is true, scaling is allowed.
    \c activeValue is the same as \l {QtQuick::PinchHandler::activeScale}.

    The \c activeValueChanged signal is emitted when \c activeValue changes, to
    provide the multiplier for the incremental change.
    This is intended for incrementally adjusting one property via multiple handlers.

    \snippet pointerHandlers/pinchHandlerAxisValueDeltas.qml 0

    \note The snippet is contrived: PinchHandler already knows how to move,
    scale and rotate its parent item, but this code achieves different behavior
    in a less-declarative way, to illustrate how to use \c activeValueChanged
    in special cases.
*/

/*!
    \qmlpropertygroup QtQuick::PinchHandler::rotationAxis
    \qmlproperty real QtQuick::PinchHandler::rotationAxis.minimum
    \qmlproperty real QtQuick::PinchHandler::rotationAxis.maximum
    \qmlproperty bool QtQuick::PinchHandler::rotationAxis.enabled
    \qmlproperty real QtQuick::PinchHandler::rotationAxis.activeValue

    \c rotationAxis controls the constraints for setting the \l {QtQuick::Item::rotation}{rotation}
    of the \l target item according to the rotation of the group of touchpoints.

    \c minimum is the minimum acceptable rotation.
    \c maximum is the maximum acceptable rotation.
    If \c enabled is true, rotation is allowed.
    \c activeValue is the same as \l {QtQuick::PinchHandler::activeRotation}.

    The \c activeValueChanged signal is emitted when \c activeValue changes, to
    provide the increment by which it changed.
    This is intended for incrementally adjusting one property via multiple handlers.

    \snippet pointerHandlers/pinchHandlerAxisValueDeltas.qml 0

    \note The snippet is contrived: PinchHandler already knows how to move,
    scale and rotate its parent item, but this code achieves different behavior
    in a less-declarative way, to illustrate how to use \c activeValueChanged
    in special cases.
*/

/*!
    \readonly
    \qmlproperty bool QtQuick::PinchHandler::active

    This property is \c true when all the constraints (epecially
    \l {MultiPointHandler::minimumPointCount}{minimumPointCount} and
    \l {MultiPointHandler::maximumPointCount}{maximumPointCount}) are satisfied
    and the \l target, if any, is being manipulated.
*/

void QQuickPinchHandler::onActiveChanged()
{
    QQuickMultiPointHandler::onActiveChanged();
    const bool curActive = active();
    m_xAxis.onActiveChanged(curActive, 0);
    m_yAxis.onActiveChanged(curActive, 0);
    m_scaleAxis.onActiveChanged(curActive, 1);
    m_rotationAxis.onActiveChanged(curActive, 0);

    if (curActive) {
        m_startAngles = angles(centroid().sceneGrabPosition());
        m_startDistance = averageTouchPointDistance(centroid().sceneGrabPosition());
        m_startTargetPos = target() ? target()->position() : QPointF();
        qCDebug(lcPinchHandler) << "activated with starting scale" << m_scaleAxis.m_startValue
                                << "rotation" << m_rotationAxis.m_startValue
                                << "target pos" << m_startTargetPos;
    } else {
        m_startTargetPos = QPointF();
        qCDebug(lcPinchHandler) << "deactivated with scale" << m_scaleAxis.m_activeValue << "rotation" << m_rotationAxis.m_activeValue;
    }
}

void QQuickPinchHandler::handlePointerEventImpl(QPointerEvent *event)
{
    QQuickMultiPointHandler::handlePointerEventImpl(event);
    if (Q_UNLIKELY(lcPinchHandler().isDebugEnabled())) {
        for (const QQuickHandlerPoint &p : currentPoints())
            qCDebug(lcPinchHandler) << Qt::hex << p.id() << p.sceneGrabPosition() << "->" << p.scenePosition();
    }

    qreal dist = 0;
#if QT_CONFIG(gestures)
    if (event->type() == QEvent::NativeGesture) {
        const auto gesture = static_cast<const QNativeGestureEvent *>(event);
        mutableCentroid().reset(event, event->point(0));
        switch (gesture->gestureType()) {
        case Qt::BeginNativeGesture:
            setActive(true);
            // Native gestures for 2-finger pinch do not allow dragging, so
            // the centroid won't move during the gesture, and translation stays at zero
            return;
        case Qt::EndNativeGesture:
            mutableCentroid().reset();
            setActive(false);
            emit updated();
            return;
        case Qt::ZoomNativeGesture:
            setActiveScale(m_scaleAxis.activeValue() * (1 + gesture->value()));
            break;
        case Qt::RotateNativeGesture:
            setActiveRotation(m_rotationAxis.activeValue() + gesture->value());
            break;
        default:
            // Nothing of interest (which is unexpected, because wantsPointerEvent() should have returned false)
            return;
        }
    } else
#endif // QT_CONFIG(gestures)
    {
        const bool containsReleasedPoints = event->isEndEvent();
        QVector<QEventPoint> chosenPoints;
        for (const QQuickHandlerPoint &p : currentPoints()) {
            auto ep = event->pointById(p.id());
            Q_ASSERT(ep);
            chosenPoints << *ep;
        }
        if (!active()) {
            // Verify that at least one of the points has moved beyond threshold needed to activate the handler
            int numberOfPointsDraggedOverThreshold = 0;
            QVector2D accumulatedDrag;
            const QVector2D currentCentroid(centroid().scenePosition());
            const QVector2D pressCentroid(centroid().scenePressPosition());

            const int dragThreshold = QQuickPointerHandler::dragThreshold();
            const int dragThresholdSquared = dragThreshold * dragThreshold;

            double accumulatedCentroidDistance = 0;     // Used to detect scale
            if (event->isBeginEvent())
                m_accumulatedStartCentroidDistance = 0;   // Used to detect scale

            float accumulatedMovementMagnitude = 0;

            for (auto &point : chosenPoints) {
                if (!containsReleasedPoints) {
                    accumulatedDrag += QVector2D(point.scenePressPosition() - point.scenePosition());
                    /*
                       In order to detect a drag, we want to check if all points have moved more or
                       less in the same direction.

                       We then take each point, and convert the point to a local coordinate system where
                       the centroid is the origin. This is done both for the press positions and the
                       current positions. We will then have two positions:

                       - pressCentroidRelativePosition
                           is the start point relative to the press centroid
                       - currentCentroidRelativePosition
                           is the current point relative to the current centroid

                       If those two points are far enough apart, it might not be considered as a drag
                       anymore. (Note that the threshold will matched to the average of the relative
                       movement of all the points). Therefore, a big relative movement will make a big
                       contribution to the average relative movement.

                       The algorithm then can be described as:
                         For each point:
                          - Calculate vector pressCentroidRelativePosition (from the press centroid to the press position)
                          - Calculate vector currentCentroidRelativePosition (from the current centroid to the current position)
                          - Calculate the relative movement vector:

                             centroidRelativeMovement = currentCentroidRelativePosition - pressCentroidRelativePosition

                           and measure its magnitude. Add the magnitude to the accumulatedMovementMagnitude.

                         Finally, if the accumulatedMovementMagnitude is below some threshold, it means
                         that the points were stationary or they were moved in parallel (e.g. the hand
                         was moved, but the relative position between each finger remained very much
                         the same). This is then used to rule out if there is a rotation or scale.
                    */
                    QVector2D pressCentroidRelativePosition = QVector2D(point.scenePosition()) - currentCentroid;
                    QVector2D currentCentroidRelativePosition = QVector2D(point.scenePressPosition()) - pressCentroid;
                    QVector2D centroidRelativeMovement = currentCentroidRelativePosition - pressCentroidRelativePosition;
                    accumulatedMovementMagnitude += centroidRelativeMovement.length();

                    accumulatedCentroidDistance += qreal(pressCentroidRelativePosition.length());
                    if (event->isBeginEvent())
                        m_accumulatedStartCentroidDistance += qreal((QVector2D(point.scenePressPosition()) - pressCentroid).length());
                } else {
                    setPassiveGrab(event, point);
                }
                if (point.state() == QEventPoint::Pressed) {
                    point.setAccepted(false); // don't stop propagation
                    setPassiveGrab(event, point);
                }
                Q_D(QQuickMultiPointHandler);
                if (d->dragOverThreshold(point))
                    ++numberOfPointsDraggedOverThreshold;
            }

            const bool requiredNumberOfPointsDraggedOverThreshold =
                    numberOfPointsDraggedOverThreshold >= minimumPointCount() &&
                    numberOfPointsDraggedOverThreshold <= maximumPointCount();
            accumulatedMovementMagnitude /= currentPoints().size();

            QVector2D avgDrag = accumulatedDrag / currentPoints().size();
            if (!xAxis()->enabled())
                avgDrag.setX(0);
            if (!yAxis()->enabled())
                avgDrag.setY(0);

            const qreal centroidMovementDelta = qreal((currentCentroid - pressCentroid).length());

            qreal distanceToCentroidDelta = qAbs(accumulatedCentroidDistance - m_accumulatedStartCentroidDistance); // Used to detect scale
            if (numberOfPointsDraggedOverThreshold >= 1) {
                if (requiredNumberOfPointsDraggedOverThreshold &&
                        avgDrag.lengthSquared() >= dragThresholdSquared && accumulatedMovementMagnitude < dragThreshold) {
                    // Drag
                    if (grabPoints(event, chosenPoints))
                        setActive(true);
                } else if (distanceToCentroidDelta > dragThreshold) {    // all points should in accumulation have been moved beyond threshold (?)
                    // Scale
                    if (grabPoints(event, chosenPoints))
                        setActive(true);
                } else if (distanceToCentroidDelta < dragThreshold && (centroidMovementDelta < dragThreshold)) {
                    // Rotate
                    // Since it wasn't a scale and if we exceeded the dragthreshold, and the
                    // centroid didn't moved much, the points must have been moved around the centroid.
                    if (grabPoints(event, chosenPoints))
                        setActive(true);
                }
            }
            if (!active())
                return;
        }

        // avoid mapping the minima and maxima, as they might have unmappable values
        // such as -inf/+inf. Because of this we perform the bounding to min/max in local coords.
        // 1. scale
        qreal activeScale = 1;
        if (m_scaleAxis.enabled()) {
            dist = averageTouchPointDistance(centroid().scenePosition());
            activeScale = dist / m_startDistance;
            activeScale = qBound(m_scaleAxis.minimum() / m_scaleAxis.m_startValue, activeScale,
                                 m_scaleAxis.maximum() / m_scaleAxis.m_startValue);
            setActiveScale(activeScale);
        }

        // 2. rotate
        if (m_rotationAxis.enabled()) {
            QVector<PointData> newAngles = angles(centroid().scenePosition());
            const qreal angleDelta = averageAngleDelta(m_startAngles, newAngles);
            setActiveRotation(m_rotationAxis.m_activeValue + angleDelta);
            m_startAngles = std::move(newAngles);
        }

        if (!containsReleasedPoints)
            acceptPoints(chosenPoints);
    }


    if (target() && target()->parentItem()) {
        auto *t = target();
        const QPointF centroidParentPos = t->parentItem()->mapFromScene(centroid().scenePosition());
        // 3. Drag/translate
        const QPointF centroidStartParentPos = t->parentItem()->mapFromScene(centroid().sceneGrabPosition());
        auto activeTranslation = centroidParentPos - centroidStartParentPos;
        // apply rotation + scaling around the centroid - then apply translation.
        QPointF pos = QQuickItemPrivate::get(t)->adjustedPosForTransform(centroidParentPos,
                m_startTargetPos, QVector2D(activeTranslation),
                t->scale(), m_scaleAxis.persistentValue() / m_scaleAxis.m_startValue,
                t->rotation(), m_rotationAxis.persistentValue() - m_rotationAxis.m_startValue);

        if (xAxis()->enabled())
            pos.setX(qBound(xAxis()->minimum(), pos.x(), xAxis()->maximum()));
        else
            pos.rx() -= qreal(activeTranslation.x());
        if (yAxis()->enabled())
            pos.setY(qBound(yAxis()->minimum(), pos.y(), yAxis()->maximum()));
        else
            pos.ry() -= qreal(activeTranslation.y());

        const QVector2D delta(activeTranslation.x() - m_xAxis.activeValue(),
                              activeTranslation.y() - m_yAxis.activeValue());
        m_xAxis.updateValue(activeTranslation.x(), m_xAxis.persistentValue() + delta.x(), delta.x());
        m_yAxis.updateValue(activeTranslation.y(), m_yAxis.persistentValue() + delta.y(), delta.y());
        emit translationChanged(delta);
        t->setPosition(pos);
        t->setRotation(m_rotationAxis.persistentValue());
        t->setScale(m_scaleAxis.persistentValue());
    } else {
        auto activeTranslation = centroid().scenePosition() - centroid().scenePressPosition();
        auto accumulated = QPointF(m_xAxis.m_startValue, m_yAxis.m_startValue) + activeTranslation;
        const QVector2D delta(activeTranslation.x() - m_xAxis.activeValue(),
                              activeTranslation.y() - m_yAxis.activeValue());
        m_xAxis.updateValue(activeTranslation.x(), accumulated.x(), delta.x());
        m_yAxis.updateValue(activeTranslation.y(), accumulated.y(), delta.y());
        emit translationChanged(delta);
    }

    qCDebug(lcPinchHandler) << "centroid" << centroid().scenePressPosition() << "->"  << centroid().scenePosition()
                            << ", distance" << m_startDistance << "->" << dist
                            << ", scale" << m_scaleAxis.m_startValue << "->" << m_scaleAxis.m_accumulatedValue
                            << ", rotation" << m_rotationAxis.m_startValue << "->" << m_rotationAxis.m_accumulatedValue
                            << ", translation" << persistentTranslation()
                            << " from " << event->device()->type();

    emit updated();
}

/*!
    \internal
    \qmlproperty flags QtQuick::PinchHandler::acceptedButtons

    This property is not used in PinchHandler.
*/

/*!
    \readonly
    \qmlproperty QtQuick::handlerPoint QtQuick::PinchHandler::centroid

    A point exactly in the middle of the currently-pressed touch points.
    The \l target will be rotated around this point.
*/

QT_END_NAMESPACE

#include "moc_qquickpinchhandler_p.cpp"
