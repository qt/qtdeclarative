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
    , m_scale(1)
    , m_rotation(0)
    , m_translation(0,0)
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
    connect(this, &QQuickPinchHandler::activeChanged, this, &QQuickPinchHandler::onActiveChanged);
    connect(this, &QQuickPinchHandler::targetChanged, this, &QQuickPinchHandler::onTargetChanged);
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
        m_startScale = m_scale; // TODO incompatible with independent x/y scaling
        m_startRotation = m_rotation;
        m_startAngles = angles(touchPointCentroid());
        m_activeRotation = 0;
        m_startMatrix = m_transform.matrix();
        qCInfo(lcPinchHandler) << "activated with starting scale" << m_startScale << "rotation" << m_startRotation;
        grabPoints(m_currentPoints);
    }
}

void QQuickPinchHandler::onTargetChanged()
{
    if (target()) {
        // TODO if m_target was previously set differently,
        // does prepending to the new target remove it from the old one?
        // If not, should we fix that there, or here?
        m_transform.prependToItem(target());
    }
}

void QQuickPinchHandler::handlePointerEventImpl(QQuickPointerEvent *event)
{
    Q_UNUSED(event)

    if (Q_UNLIKELY(lcPinchHandler().isDebugEnabled())) {
        for (QQuickEventPoint *point : qAsConst(m_currentPoints))
            qCDebug(lcPinchHandler) << point->state() << point->sceneGrabPos() << "->" << point->scenePos();
    }

    // TODO check m_pinchOrigin: right now it acts like it's set to PinchCenter
    QPointF startCentroid = startingCentroid();
    m_centroid = touchPointCentroid();
    m_centroid = QPointF(qBound(m_minimumX, m_centroid.x(), m_maximumX),
                         qBound(m_minimumY, m_centroid.y(), m_maximumY));


    // 1. scale
    qreal startDist = averageStartingDistance(startCentroid);
    qreal dist = averageTouchPointDistance(m_centroid);
    qreal activeScale = dist / startDist;
    activeScale = qBound(m_minimumScale/m_startScale, activeScale, m_maximumScale/m_startScale);
    m_scale = m_startScale * activeScale;

    // 2. rotate
    QVector<PointData> newAngles = angles(m_centroid);
    const qreal angleDelta = averageAngleDelta(m_startAngles, newAngles);
    m_activeRotation += angleDelta;
    const qreal totalRotation = m_startRotation + m_activeRotation;
    m_rotation = qBound(m_minimumRotation, totalRotation, m_maximumRotation);
    m_activeRotation += (m_rotation - totalRotation);   //adjust for the potential bounding above
    m_startAngles = std::move(newAngles);

    // 3. Drag/translate
    QPointF activeTranslation(m_centroid - startCentroid);

    // apply rotation + scaling around the centroid - then apply translation.
    QMatrix4x4 mat;
    QVector3D xlatOrigin(m_centroid - target()->position());
    mat.translate(xlatOrigin);
    mat.rotate(m_activeRotation, 0, 0, -1);
    mat.scale(activeScale);
    mat.translate(-xlatOrigin);
    mat.translate(QVector3D(activeTranslation));

    // TODO some translation inadvertently happens; try to hold the chosen pinch origin in place

    qCDebug(lcPinchHandler) << "startCentroid" << startCentroid << "centroid"  << m_centroid << "dist" << dist << "starting dist" << startDist
                            << "startScale" << m_startScale << "activeRotation" << m_activeRotation
                            << "scale" << m_scale << "rotation" << m_rotation;

    mat = mat * m_startMatrix;
    m_transform.setMatrix(mat);

    emit updated();
}

QT_END_NAMESPACE
