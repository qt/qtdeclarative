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

#include "qquickdraghandler_p.h"
#include <private/qquickwindow_p.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmltype DragHandler
    \instantiates QQuickDragHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-handlers
    \brief Handler for dragging

    DragHandler is a handler that is used to interactively move an Item.

    At this time, drag-and-drop is not yet supported.

    \sa Drag, MouseArea
*/

QQuickDragHandler::QQuickDragHandler(QObject *parent)
    : QQuickPointerSingleHandler(parent)
    , m_dragging(false)
{
}

QQuickDragHandler::~QQuickDragHandler()
{
}

bool QQuickDragHandler::wantsEventPoint(QQuickEventPoint *point)
{
    // If we've already been interested in a point, stay interested, even if it has strayed outside bounds.
    return ((point->state() != QQuickEventPoint::Pressed && currentPointId() == point->pointId())
            || QQuickPointerSingleHandler::wantsEventPoint(point));
}

void QQuickDragHandler::handleEventPoint(QQuickEventPoint *point)
{
    point->setAccepted();
    switch (point->state()) {
    case QQuickEventPoint::Pressed:
        if (target() && target()->parentItem())
            m_startPos = target()->parentItem()->mapToScene(target()->position());
        break;
    case QQuickEventPoint::Updated: {
        QPointF delta = point->scenePos() - point->scenePressPos();
        if (!m_xAxis.enabled())
            delta.setX(0);
        if (!m_yAxis.enabled())
            delta.setY(0);
        if (m_dragging) {
            if (target() && target()->parentItem()) {
                QPointF pos = target()->parentItem()->mapFromScene(m_startPos + delta);
                enforceAxisConstraints(&pos);
                target()->setPosition(pos);
            }
        } else if ((m_xAxis.enabled() && QQuickWindowPrivate::dragOverThreshold(delta.x(), Qt::XAxis, point)) ||
                   (m_yAxis.enabled() && QQuickWindowPrivate::dragOverThreshold(delta.y(), Qt::YAxis, point))) {
            m_dragging = true;
            setGrab(point, true);
            emit draggingChanged();
        }
    } break;
    case QQuickEventPoint::Released:
        if (m_dragging) {
            m_dragging = false;
            emit draggingChanged();
        }
        break;
    default:
        break;
    }
}

void QQuickDragHandler::enforceConstraints()
{
    if (!target() || !target()->parentItem())
        return;
    QPointF pos = target()->position();
    QPointF copy(pos);
    enforceAxisConstraints(&pos);
    if (pos != copy)
        target()->setPosition(pos);
}

void QQuickDragHandler::enforceAxisConstraints(QPointF *localPos)
{
    if (m_xAxis.enabled())
        localPos->setX(qBound(m_xAxis.minimum(), localPos->x(), m_xAxis.maximum()));
    if (m_yAxis.enabled())
        localPos->setY(qBound(m_yAxis.minimum(), localPos->y(), m_yAxis.maximum()));
}

QQuickDragAxis::QQuickDragAxis()
  : m_minimum(-DBL_MAX)
  , m_maximum(DBL_MAX)
  , m_enabled(true)
{
}

void QQuickDragAxis::setMinimum(qreal minimum)
{
    if (m_minimum == minimum)
        return;

    m_minimum = minimum;
    emit minimumChanged();
}

void QQuickDragAxis::setMaximum(qreal maximum)
{
    if (m_maximum == maximum)
        return;

    m_maximum = maximum;
    emit maximumChanged();
}

void QQuickDragAxis::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    emit enabledChanged();
}

QT_END_NAMESPACE
