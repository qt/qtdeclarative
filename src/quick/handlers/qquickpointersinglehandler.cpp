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

#include "qquickpointersinglehandler_p.h"

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(DBG_TOUCH_TARGET)

/*!
    An intermediate class (not registered as a QML type)
    for the most common handlers: those which expect only a single point.
    wantsPointerEvent() will choose the first point which is inside the
    \l target item, and return true as long as the event contains that point.
    Override handleEventPoint() to implement a single-point handler.
*/

QQuickPointerSingleHandler::QQuickPointerSingleHandler(QObject *parent)
  : QQuickPointerDeviceHandler(parent)
  , m_pointId(0)
  , m_rotation(0)
  , m_pressure(0)
  , m_acceptedButtons(Qt::AllButtons)
{
}

bool QQuickPointerSingleHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    if (!QQuickPointerDeviceHandler::wantsPointerEvent(event))
        return false;
    if (event->device()->pointerType() != QQuickPointerDevice::Finger &&
            (event->buttons() & m_acceptedButtons) == 0 && (event->button() & m_acceptedButtons) == 0)
        return false;

    if (m_pointId) {
        // We already know which one we want, so check whether it's there.
        // It's expected to be an update or a release.
        // If we no longer want it, cancel the grab.
        if (auto point = event->pointById(m_pointId)) {
            if (wantsEventPoint(point)) {
                point->setAccepted();
                return true;
            } else {
                point->cancelAllGrabs(this);
            }
        } else {
            qCWarning(DBG_TOUCH_TARGET) << this << "pointId" << m_pointId
                << "is missing from current event, but was neither canceled nor released";
            return false;
        }
    } else {
        // We have not yet chosen a point; choose the first one for which wantsEventPoint() returns true.
        int c = event->pointCount();
        for (int i = 0; i < c && !m_pointId; ++i) {
            QQuickEventPoint *p = event->point(i);
            if (!p->exclusiveGrabber() && wantsEventPoint(p)) {
                m_pointId = p->pointId();
                p->setAccepted();
            }
        }
    }
    return m_pointId;
}

void QQuickPointerSingleHandler::handlePointerEventImpl(QQuickPointerEvent *event)
{
    QQuickPointerDeviceHandler::handlePointerEventImpl(event);
    QQuickEventPoint *currentPoint = event->pointById(m_pointId);
    Q_ASSERT(currentPoint);
    if (!m_pointId || !currentPoint->isAccepted()) {
        reset();
    } else {
        if (event->asPointerTouchEvent()) {
            QQuickEventTouchPoint *tp = static_cast<QQuickEventTouchPoint *>(currentPoint);
            m_uniquePointId = tp->uniqueId();
            m_rotation = tp->rotation();
            m_pressure = tp->pressure();
            m_ellipseDiameters = tp->ellipseDiameters();
        } else if (event->asPointerTabletEvent()) {
            // TODO
        } else {
            m_uniquePointId = event->device()->uniqueId();
            m_rotation = 0;
            m_pressure = event->buttons() ? 1 : 0;
            m_ellipseDiameters = QSizeF();
        }
        m_pos = currentPoint->pos();
        if (currentPoint->state() == QQuickEventPoint::Updated)
            m_velocity = currentPoint->velocity();
        handleEventPoint(currentPoint);
        switch (currentPoint->state()) {
        case QQuickEventPoint::Pressed:
            m_pressPos = currentPoint->pos();
            setPressedButtons(event->buttons());
            emit pointIdChanged();
            break;
        case QQuickEventPoint::Released:
            setExclusiveGrab(currentPoint, false);
            reset();
            break;
        default:
            setPressedButtons(event->buttons());
            break;
        }
    }
    emit eventPointHandled();
}

bool QQuickPointerSingleHandler::wantsEventPoint(QQuickEventPoint *point)
{
    return parentContains(point);
}

void QQuickPointerSingleHandler::onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabState stateChange, QQuickEventPoint *point)
{
    QQuickPointerHandler::onGrabChanged(grabber, stateChange, point);
    if (grabber != this)
        return;
    switch (stateChange) {
    case QQuickEventPoint::GrabExclusive:
        setActive(true);
        Q_FALLTHROUGH();
    case QQuickEventPoint::GrabPassive:
        m_sceneGrabPos = point->sceneGrabPos();
        break;
    case QQuickEventPoint::OverrideGrabPassive:
        return; // don't emit
    case QQuickEventPoint::UngrabPassive:
    case QQuickEventPoint::UngrabExclusive:
    case QQuickEventPoint::CancelGrabPassive:
    case QQuickEventPoint::CancelGrabExclusive:
        // the grab is lost or relinquished, so the point is no longer relevant
        reset();
        break;
    }
    emit singlePointGrabChanged();
}

void QQuickPointerSingleHandler::setPressedButtons(Qt::MouseButtons buttons)
{
    if (buttons != m_pressedButtons) {
        m_pressedButtons = buttons;
        emit pressedButtonsChanged();
    }
}

void QQuickPointerSingleHandler::setAcceptedButtons(Qt::MouseButtons buttons)
{
    if (m_acceptedButtons == buttons)
        return;

    m_acceptedButtons = buttons;
    emit acceptedButtonsChanged();
}

void QQuickPointerSingleHandler::reset()
{
    bool pointIdChange = m_pointId != 0;
    m_pointId = 0;
    m_uniquePointId = QPointingDeviceUniqueId();
    m_pos = QPointF();
    m_pressPos = QPointF();
    m_sceneGrabPos = QPointF();
    m_velocity = QVector2D();
    m_rotation = 0;
    m_pressure = 0;
    m_ellipseDiameters = QSizeF();
    setPressedButtons(Qt::NoButton);
    if (pointIdChange)
        emit pointIdChanged();
}

QT_END_NAMESPACE
