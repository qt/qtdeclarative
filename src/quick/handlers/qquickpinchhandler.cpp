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
    , m_startScale(1)
    , m_startRotation(0)
    , m_minimumScale(-qInf())
    , m_maximumScale(qInf())
    , m_minimumRotation(-qInf())
    , m_maximumRotation(qInf())
    , m_pinchOrigin(PinchCenter)
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
        m_startScale = m_scaleTransform.xScale(); // TODO incompatible with independent x/y scaling
        m_startRotation = m_rotationTransform.angle();
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
        m_scaleTransform.prependToItem(target());
        m_rotationTransform.prependToItem(target());
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
    QPointF centroid = touchPointCentroid();
    QVector3D origin(centroid);

    qreal startDist = averageStartingDistance(startCentroid);
    qreal dist = averageTouchPointDistance(centroid);
    qreal zoom = dist / startDist;

    qreal newScale = qBound(m_minimumScale, zoom * m_startScale, m_maximumScale);
    m_scaleTransform.setOrigin(origin);
    // TODO optionally allow separate x and y scaling?
    m_scaleTransform.setXScale(newScale);
    m_scaleTransform.setYScale(newScale);

    qreal startAngle = averageStartingAngle(startCentroid);
    qreal angle = averageTouchPointAngle(centroid);
    qreal angleDelta = startAngle - angle;
    m_rotationTransform.setOrigin(origin);
    m_rotationTransform.setAngle(qMin(m_maximumRotation, qMax(m_minimumRotation, m_startRotation + angleDelta)));

    // TODO some translation inadvertently happens; try to hold the chosen pinch origin in place

    qCDebug(lcPinchHandler) << "startCentroid" << startCentroid << "centroid"  << centroid << "dist" << dist << "starting dist" << startDist
                            << "zoom" << zoom << "startScale" << m_startScale << "startAngle" << startAngle << "angle" << angle
                            << "scale" << scale() << "rotation" << rotation();

    emit updated();
}

QT_END_NAMESPACE
