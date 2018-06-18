/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickpinchhandler_p.h"
#include <QtQuick/qquickwindow.h>
#include <private/qsgadaptationlayer_p.h>
#include <private/qquickitem_p.h>
#include <private/qguiapplication_p.h>
#include <private/qquickwindow_p.h>
#include <QEvent>
#include <QMouseEvent>
#include <QDebug>
#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPinchHandler, "qt.quick.handler.pinch")

/*!
    \qmltype PinchHandler
    \instantiates QQuickPinchHandler
    \inherits MultiPointHandler
    \inqmlmodule Qt.labs.handlers
    \ingroup qtquick-handlers
    \brief Handler for pinch gestures.

    PinchHandler is a handler that interprets a multi-finger gesture to
    interactively rotate, zoom, and drag an Item. Like other Pointer Handlers,
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

    \sa PinchArea
*/

QQuickPinchHandler::QQuickPinchHandler(QObject *parent)
    : QQuickMultiPointHandler(parent, 2)
    , m_activeScale(1)
    , m_activeRotation(0)
    , m_activeTranslation(0,0)
    , m_minimumScale(-qInf())
    , m_maximumScale(qInf())
    , m_minimumRotation(-qInf())
    , m_maximumRotation(qInf())
    , m_minimumX(-qInf())
    , m_maximumX(qInf())
    , m_minimumY(-qInf())
    , m_maximumY(qInf())
    , m_pinchOrigin(PinchCenter)
    , m_startScale(1)
    , m_startRotation(0)
{
}

QQuickPinchHandler::~QQuickPinchHandler()
{
}

/*!
    \qmlproperty real QtQuick::PinchHandler::minimumScale

    The minimum acceptable \l {Item::scale}{scale} to be applied
    to the \l target.
*/
void QQuickPinchHandler::setMinimumScale(qreal minimumScale)
{
    if (m_minimumScale == minimumScale)
        return;

    m_minimumScale = minimumScale;
    emit minimumScaleChanged();
}

/*!
    \qmlproperty real QtQuick::PinchHandler::maximumScale

    The maximum acceptable \l {Item::scale}{scale} to be applied
    to the \l target.
*/
void QQuickPinchHandler::setMaximumScale(qreal maximumScale)
{
    if (m_maximumScale == maximumScale)
        return;

    m_maximumScale = maximumScale;
    emit maximumScaleChanged();
}

/*!
    \qmlproperty real QtQuick::PinchHandler::minimumRotation

    The minimum acceptable \l {Item::rotation}{rotation} to be applied
    to the \l target.
*/
void QQuickPinchHandler::setMinimumRotation(qreal minimumRotation)
{
    if (m_minimumRotation == minimumRotation)
        return;

    m_minimumRotation = minimumRotation;
    emit minimumRotationChanged();
}

/*!
    \qmlproperty real QtQuick::PinchHandler::maximumRotation

    The maximum acceptable \l {Item::rotation}{rotation} to be applied
    to the \l target.
*/
void QQuickPinchHandler::setMaximumRotation(qreal maximumRotation)
{
    if (m_maximumRotation == maximumRotation)
        return;

    m_maximumRotation = maximumRotation;
    emit maximumRotationChanged();
}

/*!
    \qmlproperty real QtQuick::PinchHandler::pinchOrigin

    The point to be held in place, around which the \l target is scaled and
    rotated.

    \value FirstPoint
        the first touch point, wherever the first finger is pressed
    \value PinchCenter
        the centroid between all the touch points at the time when the
        PinchHandler becomes \l active
    \value TargetCenter
        the center of the \l target
*/
void QQuickPinchHandler::setPinchOrigin(QQuickPinchHandler::PinchOrigin pinchOrigin)
{
    if (m_pinchOrigin == pinchOrigin)
        return;

    m_pinchOrigin = pinchOrigin;
    emit pinchOriginChanged();
}

/*!
    \qmlproperty real QtQuick::PinchHandler::minimumX

    The minimum acceptable x coordinate of the centroid
*/
void QQuickPinchHandler::setMinimumX(qreal minX)
{
    if (m_minimumX == minX)
        return;
    m_minimumX = minX;
    emit minimumXChanged();
}

/*!
    \qmlproperty real QtQuick::PinchHandler::maximumX

    The maximum acceptable x coordinate of the centroid
*/
void QQuickPinchHandler::setMaximumX(qreal maxX)
{
    if (m_maximumX == maxX)
        return;
    m_maximumX = maxX;
    emit maximumXChanged();
}

/*!
    \qmlproperty real QtQuick::PinchHandler::minimumY

    The minimum acceptable y coordinate of the centroid
*/
void QQuickPinchHandler::setMinimumY(qreal minY)
{
    if (m_minimumY == minY)
        return;
    m_minimumY = minY;
    emit minimumYChanged();
}

/*!
    \qmlproperty real QtQuick::PinchHandler::maximumY

    The maximum acceptable y coordinate of the centroid
*/
void QQuickPinchHandler::setMaximumY(qreal maxY)
{
    if (m_maximumY == maxY)
        return;
    m_maximumY = maxY;
    emit maximumYChanged();
}

bool QQuickPinchHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    if (!QQuickMultiPointHandler::wantsPointerEvent(event))
        return false;

#if QT_CONFIG(gestures)
    if (const auto gesture = event->asPointerNativeGestureEvent()) {
        if (minimumPointCount() == 2) {
            switch (gesture->type()) {
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
    \qmlproperty int QtQuick::PinchHandler::minimumTouchPoints

    The pinch begins when this number of fingers are pressed.
    Until then, PinchHandler tracks the positions of any pressed fingers,
    but if it's an insufficient number, it does not scale or rotate
    its \l target, and the \l active property will remain false.
*/

/*!
    \qmlproperty bool QtQuick::PinchHandler::active

    This property is true when all the constraints (epecially \l minimumTouchPoints)
    are satisfied and the \l target, if any, is being manipulated.
*/

void QQuickPinchHandler::onActiveChanged()
{
    if (active()) {
        m_startMatrix = QMatrix4x4();
        m_startCentroid = touchPointCentroid();
        m_startAngles = angles(m_startCentroid);
        m_startDistance = averageTouchPointDistance(m_startCentroid);
        m_activeRotation = 0;
        m_activeTranslation = QVector2D();
        if (const QQuickItem *t = target()) {
            m_startScale = t->scale(); // TODO incompatible with independent x/y scaling
            m_startRotation = t->rotation();
            QVector3D xformOrigin(t->transformOriginPoint());
            m_startMatrix.translate(t->x(), t->y());
            m_startMatrix.translate(xformOrigin);
            m_startMatrix.scale(m_startScale);
            m_startMatrix.rotate(m_startRotation, 0, 0, -1);
            m_startMatrix.translate(-xformOrigin);
        } else {
            m_startScale = 1;
            m_startRotation = 0;
        }
        qCInfo(lcPinchHandler) << "activated with starting scale" << m_startScale << "rotation" << m_startRotation;
    } else {
        qCInfo(lcPinchHandler) << "deactivated with scale" << m_activeScale << "rotation" << m_activeRotation;
    }
}

void QQuickPinchHandler::handlePointerEventImpl(QQuickPointerEvent *event)
{
    if (Q_UNLIKELY(lcPinchHandler().isDebugEnabled())) {
        for (QQuickEventPoint *point : qAsConst(m_currentPoints))
            qCDebug(lcPinchHandler) << point->state() << point->sceneGrabPosition() << "->" << point->scenePosition();
    }

    qreal dist = 0;
#if QT_CONFIG(gestures)
    if (const auto gesture = event->asPointerNativeGestureEvent()) {
        switch (gesture->type()) {
        case Qt::EndNativeGesture:
            m_activeScale = 1;
            m_activeRotation = 0;
            m_activeTranslation = QVector2D();
            m_centroid = QPointF();
            m_centroidVelocity = QVector2D();
            setActive(false);
            emit updated();
            return;
        case Qt::ZoomNativeGesture:
            m_activeScale *= 1 + gesture->value();
            break;
        case Qt::RotateNativeGesture:
            m_activeRotation += gesture->value();
            break;
        default:
            // Nothing of interest (which is unexpected, because wantsPointerEvent() should have returned false)
            return;
        }
        if (!active()) {
            m_centroid = gesture->point(0)->scenePosition();
            setActive(true);
            m_startCentroid = m_centroid;
            // Native gestures for 2-finger pinch do not allow dragging, so
            // the centroid won't move during the gesture, and translation stays at zero
            m_centroidVelocity = QVector2D();
            m_activeTranslation = QVector2D();
        }
    } else
#endif // QT_CONFIG(gestures)
    {
        bool containsReleasedPoints = event->isReleaseEvent();
        if (!active()) {
            // Verify that at least one of the points has moved beyond threshold needed to activate the handler
            for (QQuickEventPoint *point : qAsConst(m_currentPoints)) {
                if (!containsReleasedPoints && QQuickWindowPrivate::dragOverThreshold(point) && grabPoints(m_currentPoints)) {
                    setActive(true);
                    break;
                } else {
                    setPassiveGrab(point);
                }
                if (point->state() == QQuickEventPoint::Pressed) {
                    point->setAccepted(false); // don't stop propagation
                    setPassiveGrab(point);
                }
            }
            if (!active())
                return;
        }
        // TODO check m_pinchOrigin: right now it acts like it's set to PinchCenter
        m_centroid = touchPointCentroid();
        m_centroidVelocity = touchPointCentroidVelocity();
        // avoid mapping the minima and maxima, as they might have unmappable values
        // such as -inf/+inf. Because of this we perform the bounding to min/max in local coords.
        // 1. scale
        dist = averageTouchPointDistance(m_centroid);
        m_activeScale = dist / m_startDistance;
        m_activeScale = qBound(m_minimumScale/m_startScale, m_activeScale, m_maximumScale/m_startScale);

        // 2. rotate
        QVector<PointData> newAngles = angles(m_centroid);
        const qreal angleDelta = averageAngleDelta(m_startAngles, newAngles);
        m_activeRotation += angleDelta;
        m_startAngles = std::move(newAngles);

        if (!containsReleasedPoints)
            acceptPoints(m_currentPoints);
    }

    QPointF centroidParentPos;
    QRectF bounds(m_minimumX, m_minimumY, m_maximumX - m_minimumX, m_maximumY - m_minimumY);
    if (target() && target()->parentItem()) {
        centroidParentPos = target()->parentItem()->mapFromScene(m_centroid);
        centroidParentPos = QPointF(qBound(bounds.left(), centroidParentPos.x(), bounds.right()),
                                   qBound(bounds.top(), centroidParentPos.y(), bounds.bottom()));
    }
    const qreal totalRotation = m_startRotation + m_activeRotation;
    const qreal rotation = qBound(m_minimumRotation, totalRotation, m_maximumRotation);
    m_activeRotation += (rotation - totalRotation);   //adjust for the potential bounding above
    const qreal scale = m_startScale * m_activeScale;

    if (target() && target()->parentItem()) {
        // 3. Drag/translate
        const QPointF centroidStartParentPos = target()->parentItem()->mapFromScene(m_startCentroid);
        m_activeTranslation = QVector2D(centroidParentPos - centroidStartParentPos);

        // apply rotation + scaling around the centroid - then apply translation.
        QMatrix4x4 mat;

        const QVector3D centroidParentVector(centroidParentPos);
        mat.translate(centroidParentVector);
        mat.rotate(m_activeRotation, 0, 0, 1);
        mat.scale(m_activeScale);
        mat.translate(-centroidParentVector);
        mat.translate(QVector3D(m_activeTranslation));

        mat = mat * m_startMatrix;

        QPointF xformOriginPoint = target()->transformOriginPoint();
        QPointF pos = mat * xformOriginPoint;
        pos -= xformOriginPoint;

        target()->setPosition(pos);
        target()->setRotation(rotation);
        target()->setScale(scale);

        // TODO some translation inadvertently happens; try to hold the chosen pinch origin in place
    } else {
        m_activeTranslation = QVector2D(m_centroid - m_startCentroid);
    }

    qCDebug(lcPinchHandler) << "centroid" << m_startCentroid << "->"  << m_centroid
                            << ", distance" << m_startDistance << "->" << dist
                            << ", startScale" << m_startScale << "->" << scale
                            << ", activeRotation" << m_activeRotation
                            << ", rotation" << rotation
                            << " from " << event->device()->type();

    emit updated();
}

/*!
    \readonly
    \qmlproperty QPointF QtQuick::PinchHandler::centroid

    A point exactly in the middle of the currently-pressed touch points.
    If \l pinchOrigin is set to \c PinchCenter, the \l target will be rotated
    around this point.
*/

/*!
    \readonly
    \qmlproperty QVector2D QtQuick::PinchHandler::centroidVelocity

    The average velocity of the \l centroid: a vector representing the speed
    and direction of movement of the whole group of touchpoints, in logical
    pixels per second.
*/

/*!
    \readonly
    \qmlproperty real QtQuick::PinchHandler::scale

    The scale factor. It is 1.0 when the gesture begins, increases as the
    touchpoints are spread apart, and decreases as the touchpoints are brought
    together. If \l target is not null, this will be automatically applied to its
    \l {Item::scale}{scale}. Otherwise, bindings can be used to do arbitrary
    things with this value.
*/

/*!
    \readonly
    \qmlproperty real QtQuick::PinchHandler::rotation

    The rotation of the pinch gesture in degrees, with positive values clockwise.
    It is 0 when the gesture begins. If \l target is not null, this will be
    automatically applied to its \l {Item::rotation}{rotation}. Otherwise,
    bindings can be used to do arbitrary things with this value.
*/

/*!
    \readonly
    \qmlproperty QVector2D QtQuick::PinchHandler::translation

    The translation of the gesture \l centroid. It is \c (0, 0) when the
    gesture begins.
*/

QT_END_NAMESPACE
