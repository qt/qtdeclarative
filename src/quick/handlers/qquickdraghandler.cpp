/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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
    \inherits SinglePointHandler
    \inqmlmodule Qt.labs.handlers
    \ingroup qtquick-handlers
    \brief Handler for dragging.

    DragHandler is a handler that is used to interactively move an Item.
    Like other Pointer Handlers, by default it is fully functional, and
    manipulates its \l {PointerHandler::target} {target}.

    \snippet pointerHandlers/dragHandler.qml 0

    It has properties to restrict the range of dragging.

    If it is declared within one Item but is assigned a different
    \l {PointerHandler::target} {target}, then it handles events within the
    bounds of the \l {PointerHandler::parent} {parent} Item but
    manipulates the \c target Item instead:

    \snippet pointerHandlers/dragHandlerDifferentTarget.qml 0

    A third way to use it is to set \l {PointerHandler::target} {target} to
    \c null and react to property changes in some other way:

    \snippet pointerHandlers/dragHandlerNullTarget.qml 0

    At this time, drag-and-drop is not yet supported.

    \sa Drag, MouseArea
*/

QQuickDragHandler::QQuickDragHandler(QObject *parent)
    : QQuickSinglePointHandler(parent)
{
}

QQuickDragHandler::~QQuickDragHandler()
{
}

bool QQuickDragHandler::wantsEventPoint(QQuickEventPoint *point)
{
    // If we've already been interested in a point, stay interested, even if it has strayed outside bounds.
    return ((point->state() != QQuickEventPoint::Pressed && this->point().id() == point->pointId())
            || QQuickSinglePointHandler::wantsEventPoint(point));
}

bool QQuickDragHandler::targetContains(QQuickEventPoint *point)
{
    Q_ASSERT(parentItem() && target());
    return target()->contains(localTargetPosition(point));
}

QPointF QQuickDragHandler::localTargetPosition(QQuickEventPoint *point)
{
    QPointF pos = point->position();
    if (target() != parentItem())
        pos = parentItem()->mapToItem(target(), pos);
    return pos;
}

void QQuickDragHandler::onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabState stateChange, QQuickEventPoint *point)
{
    if (grabber == this && stateChange == QQuickEventPoint::GrabExclusive) {
        // In case the grab got handed over from another grabber, we might not get the Press.
        if (!m_pressedInsideTarget) {
            if (target())
                m_pressTargetPos = QPointF(target()->width(), target()->height()) / 2;
            m_pressScenePos = point->scenePosition();
        } else if (m_pressTargetPos.isNull()) {
            if (target())
                m_pressTargetPos = localTargetPosition(point);
            m_pressScenePos = point->scenePosition();
        }
    }
    QQuickSinglePointHandler::onGrabChanged(grabber, stateChange, point);
}

void QQuickDragHandler::onActiveChanged()
{
    if (!active()) {
        m_pressTargetPos = QPointF();
        m_pressScenePos = m_pressTargetPos;
        m_pressedInsideTarget = false;
    }
}

void QQuickDragHandler::handleEventPoint(QQuickEventPoint *point)
{
    point->setAccepted();
    switch (point->state()) {
    case QQuickEventPoint::Pressed:
        if (target()) {
            m_pressedInsideTarget = targetContains(point);
            m_pressTargetPos = localTargetPosition(point);
        }
        m_pressScenePos = point->scenePosition();
        setPassiveGrab(point);
        break;
    case QQuickEventPoint::Updated: {
        QVector2D accumulatedDragDelta = QVector2D(point->scenePosition() - m_pressScenePos);
        if (active()) {
            // update translation property. Make sure axis is respected for it.
            if (!m_xAxis.enabled())
                accumulatedDragDelta.setX(0);
            if (!m_yAxis.enabled())
                accumulatedDragDelta.setY(0);
            setTranslation(accumulatedDragDelta);

            if (target() && target()->parentItem()) {
                const QPointF newTargetTopLeft = localTargetPosition(point) - m_pressTargetPos;
                const QPointF xformOrigin = target()->transformOriginPoint();
                const QPointF targetXformOrigin = newTargetTopLeft + xformOrigin;
                QPointF pos = target()->parentItem()->mapFromItem(target(), targetXformOrigin);
                pos -= xformOrigin;
                QPointF targetItemPos = target()->position();
                if (!m_xAxis.enabled())
                    pos.setX(targetItemPos.x());
                if (!m_yAxis.enabled())
                    pos.setY(targetItemPos.y());
                enforceAxisConstraints(&pos);
                moveTarget(pos, point);
            }
        } else if (!point->exclusiveGrabber() &&
                   ((m_xAxis.enabled() && QQuickWindowPrivate::dragOverThreshold(accumulatedDragDelta.x(), Qt::XAxis, point)) ||
                    (m_yAxis.enabled() && QQuickWindowPrivate::dragOverThreshold(accumulatedDragDelta.y(), Qt::YAxis, point)))) {
            setExclusiveGrab(point);
            if (auto parent = parentItem()) {
                if (point->pointerEvent()->asPointerTouchEvent())
                    parent->setKeepTouchGrab(true);
                // tablet and mouse are treated the same by Item's legacy event handling, and
                // touch becomes synth-mouse for Flickable, so we need to prevent stealing
                // mouse grab too, whenever dragging occurs in an enabled direction
                parent->setKeepMouseGrab(true);
            }
        }
    } break;
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

void QQuickDragHandler::setTranslation(const QVector2D &trans)
{
    if (trans == m_translation) // fuzzy compare?
        return;
    m_translation = trans;
    emit translationChanged();
}

/*!
    \qmlpropertygroup QtQuick::DragHandler::xAxis
    \qmlproperty real QtQuick::DragHandler::xAxis.minimum
    \qmlproperty real QtQuick::DragHandler::xAxis.maximum
    \qmlproperty bool QtQuick::DragHandler::xAxis.enabled

    \c xAxis controls the constraints for horizontal dragging.

    \c minimum is the minimum acceptable value of \l {Item::x}{x} to be
    applied to the \l {PointerHandler::target} {target}.
    \c maximum is the maximum acceptable value of \l {Item::x}{x} to be
    applied to the \l {PointerHandler::target} {target}.
    If \c enabled is true, horizontal dragging is allowed.
 */

/*!
    \qmlpropertygroup QtQuick::DragHandler::yAxis
    \qmlproperty real QtQuick::DragHandler::yAxis.minimum
    \qmlproperty real QtQuick::DragHandler::yAxis.maximum
    \qmlproperty bool QtQuick::DragHandler::yAxis.enabled

    \c yAxis controls the constraints for vertical dragging.

    \c minimum is the minimum acceptable value of \l {Item::y}{y} to be
    applied to the \l {PointerHandler::target} {target}.
    \c maximum is the maximum acceptable value of \l {Item::y}{y} to be
    applied to the \l {PointerHandler::target} {target}.
    If \c enabled is true, vertical dragging is allowed.
 */
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

/*!
    \readonly
    \qmlproperty QVector2D QtQuick::DragHandler::translation

    The translation since the gesture began.
*/

QT_END_NAMESPACE
