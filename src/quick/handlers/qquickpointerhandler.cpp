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

#include "qquickpointerhandler_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPointerHandlerDispatch, "qt.quick.handler.dispatch")

/*!
    \qmltype PointerHandler
    \instantiates QQuickPointerHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-handlers
    \brief Handler for pointer events

    PointerHandler is a handler for pointer events regardless of source.
    They may represent events from a touch, mouse or tablet device.
*/

QQuickPointerHandler::QQuickPointerHandler(QObject *parent)
  : QObject(parent)
  , m_currentEvent(nullptr)
  , m_target(nullptr)
  , m_enabled(true)
  , m_active(false)
{
}

void QQuickPointerHandler::setGrab(QQuickEventPoint *point, bool grab)
{
    if (grab)
        point->setGrabberPointerHandler(this);
    else if (point->grabberPointerHandler() == this)
        point->setGrabberPointerHandler(nullptr);
}

QPointF QQuickPointerHandler::eventPos(const QQuickEventPoint *point) const
{
    return (m_target ? m_target->mapFromScene(point->scenePos()) : point->scenePos());
}

bool QQuickPointerHandler::targetContains(const QQuickEventPoint *point) const
{
    if (!m_target || !point)
        return false;
    return m_target->contains(m_target->mapFromScene(point->scenePos()));
}

/*!
     \qmlproperty QQuickPointerHandler::enabled

     If a PointerHandler is disabled, it will reject all events
     and no signals will be emitted.

     TODO is it too extreme not even to emit pressed/updated/released?
     or should we disable only the higher-level interpretation, in subclasses?
*/
void QQuickPointerHandler::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    emit enabledChanged();
}

void QQuickPointerHandler::setTarget(QQuickItem *target)
{
    if (m_target == target)
        return;

    if (m_target) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(m_target);
        p->extra.value().pointerHandlers.removeOne(this);
    }

    m_target = target;
    if (m_target) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(m_target);
        p->extra.value().pointerHandlers.append(this);
        // Accept all buttons, and leave filtering to pointerEvent() and/or user JS,
        // because there can be multiple handlers...
        m_target->setAcceptedMouseButtons(Qt::AllButtons);
    }
    emit targetChanged();
}

void QQuickPointerHandler::handlePointerEvent(QQuickPointerEvent *event)
{
    const bool wants = wantsPointerEvent(event);
    setActive(wants);
    if (wants)
        handlePointerEventImpl(event);
}

bool QQuickPointerHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    Q_UNUSED(event)
    return m_enabled;
}

void QQuickPointerHandler::setActive(bool active)
{
    if (m_active != active) {
        m_active = active;
        emit activeChanged();
    }
}

void QQuickPointerHandler::handlePointerEventImpl(QQuickPointerEvent *event)
{
    m_currentEvent = event;
}

QT_END_NAMESPACE
