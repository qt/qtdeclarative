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
    \inqmlmodule QtQuick
    \ingroup qtquick-handlers
    \brief Handler for pinch gestures

    PinchHandler is a handler that is used to interactively rotate and zoom an Item.
*/

QQuickPinchHandler::QQuickPinchHandler(QObject *parent)
    : QQuickMultiPointerHandler(parent, 2)
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

void QQuickPinchHandler::setMinimumScale(qreal minimumScale)
{
    if (m_minimumScale == minimumScale)
        return;

    m_minimumScale = minimumScale;
    emit minimumScaleChanged();
}

void QQuickPinchHandler::setMaximumScale(qreal maximumScale)
{
    if (m_maximumScale == maximumScale)
        return;

    m_maximumScale = maximumScale;
    emit maximumScaleChanged();
}

void QQuickPinchHandler::setMinimumRotation(qreal minimumRotation)
{
    if (m_minimumRotation == minimumRotation)
        return;

    m_minimumRotation = minimumRotation;
    emit minimumRotationChanged();
}

void QQuickPinchHandler::setMaximumRotation(qreal maximumRotation)
{
    if (m_maximumRotation == maximumRotation)
        return;

    m_maximumRotation = maximumRotation;
    emit maximumRotationChanged();
}

void QQuickPinchHandler::setPinchOrigin(QQuickPinchHandler::PinchOrigin pinchOrigin)
{
    if (m_pinchOrigin == pinchOrigin)
        return;

    m_pinchOrigin = pinchOrigin;
    emit pinchOriginChanged();
}

/*!
    \qmlproperty QQuickPinchHandler::minimumX

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
    \qmlproperty QQuickPinchHandler::maximumX

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
    \qmlproperty QQuickPinchHandler::minimumY

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
    \qmlproperty QQuickPinchHandler::maximumY

    The maximum acceptable y coordinate of the centroid
 */
void QQuickPinchHandler::setMaximumY(qreal maxY)
{
    if (m_maximumY == maxY)
        return;
    m_maximumY = maxY;
    emit maximumYChanged();
}

/*!
    \qmlproperty QQuickPinchHandler::minimumTouchPoints

    The pinch begins when this number of fingers are pressed.
    Until then, PinchHandler tracks the positions of any pressed fingers,
    but if it's an insufficient number, it does not scale or rotate
    its \l target, and the \l active property will remain false.
*/

/*!
    \qmlproperty QQuickPinchHandler::active
*/

void QQuickPinchHandler::onActiveChanged()
{
    if (active()) {
        if (const QQuickItem *t = target()) {
            m_startScale = t->scale(); // TODO incompatible with independent x/y scaling
            m_startRotation = t->rotation();
            m_startCentroid = touchPointCentroid();
            m_startAngles = angles(m_startCentroid);
            m_startDistance = averageTouchPointDistance(m_startCentroid);
            QVector3D xformOrigin(t->transformOriginPoint());
            m_startMatrix = QMatrix4x4();
            m_startMatrix.translate(t->x(), t->y());
            m_startMatrix.translate(xformOrigin);
            m_startMatrix.scale(m_startScale);
            m_startMatrix.rotate(m_startRotation, 0, 0, -1);
            m_startMatrix.translate(-xformOrigin);
            m_activeRotation = 0;
            m_activeTranslation = QPointF(0,0);
            qCInfo(lcPinchHandler) << "activated with starting scale" << m_startScale << "rotation" << m_startRotation;
        }
    } else {
        qCInfo(lcPinchHandler) << "deactivated with scale" << m_activeScale << "rotation" << m_activeRotation;
    }
}

void QQuickPinchHandler::handlePointerEventImpl(QQuickPointerEvent *event)
{
    Q_UNUSED(event)
    if (Q_UNLIKELY(lcPinchHandler().isDebugEnabled())) {
        for (QQuickEventPoint *point : qAsConst(m_currentPoints))
            qCDebug(lcPinchHandler) << point->state() << point->sceneGrabPos() << "->" << point->scenePos();
    }

    if (!active()) {
        // Verify that least one of the points have moved beyond threshold needed to activate the handler
        for (QQuickEventPoint *point : qAsConst(m_currentPoints)) {
            if (QQuickWindowPrivate::dragOverThreshold(point)) {
                if (grabPoints(m_currentPoints))
                    setActive(true);
                break;
            }
        }
        if (!active())
            return;
    }
    // TODO check m_pinchOrigin: right now it acts like it's set to PinchCenter
    m_centroid = touchPointCentroid();
    m_centroidVelocity = touchPointCentroidVelocity();
    QRectF bounds(m_minimumX, m_minimumY, m_maximumX, m_maximumY);
    // avoid mapping the minima and maxima, as they might have unmappable values
    // such as -inf/+inf. Because of this we perform the bounding to min/max in local coords.
    QPointF centroidParentPos;
    if (target() && target()->parentItem()) {
        centroidParentPos = target()->parentItem()->mapFromScene(m_centroid);
        centroidParentPos = QPointF(qBound(bounds.left(), centroidParentPos.x(), bounds.right()),
                                   qBound(bounds.top(), centroidParentPos.y(), bounds.bottom()));
    }
    // 1. scale
    const qreal dist = averageTouchPointDistance(m_centroid);
    m_activeScale = dist / m_startDistance;
    m_activeScale = qBound(m_minimumScale/m_startScale, m_activeScale, m_maximumScale/m_startScale);
    const qreal scale = m_startScale * m_activeScale;

    // 2. rotate
    QVector<PointData> newAngles = angles(m_centroid);
    const qreal angleDelta = averageAngleDelta(m_startAngles, newAngles);
    m_activeRotation += angleDelta;
    const qreal totalRotation = m_startRotation + m_activeRotation;
    const qreal rotation = qBound(m_minimumRotation, totalRotation, m_maximumRotation);
    m_activeRotation += (rotation - totalRotation);   //adjust for the potential bounding above
    m_startAngles = std::move(newAngles);

    if (target() && target()->parentItem()) {
        // 3. Drag/translate
        const QPointF centroidStartParentPos = target()->parentItem()->mapFromScene(m_startCentroid);
        m_activeTranslation = centroidParentPos - centroidStartParentPos;

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

        qCDebug(lcPinchHandler) << "centroid" << m_startCentroid << "->"  << m_centroid
                                << ", distance" << m_startDistance << "->" << dist
                                << ", startScale" << m_startScale << "->" << scale
                                << ", activeRotation" << m_activeRotation
                                << ", rotation" << rotation;
    }

    acceptPoints(m_currentPoints);
    emit updated();
}

QT_END_NAMESPACE
