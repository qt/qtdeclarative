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

#include "qquickpointerhandler_p.h"

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcPointerHandlerDispatch, "qt.quick.handler.dispatch")
Q_LOGGING_CATEGORY(lcPointerHandlerActive, "qt.quick.handler.active")

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
  , m_targetExplicitlySet(false)
  , m_hadKeepMouseGrab(false)
  , m_hadKeepTouchGrab(false)
{
}

QQuickPointerHandler::~QQuickPointerHandler()
{
    QQuickItem *parItem = parentItem();
    if (parItem) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(parItem);
        p->extra.value().pointerHandlers.removeOne(this);
    }
}

void QQuickPointerHandler::setPassiveGrab(QQuickEventPoint *point, bool grab)
{
    if (grab) {
        point->setGrabberPointerHandler(this, false);
        emit grabChanged(point);
    } else if (point->grabberPointerHandler() == this) {
        // TODO should giving up passive grab imply giving up exclusive grab too?
        // we're being inconsistent here: check whether the exclusive grabber is this,
        // then say that the passive grab was canceled.
        point->cancelPassiveGrab(this);
        emit grabChanged(point);
    }
}

void QQuickPointerHandler::setExclusiveGrab(QQuickEventPoint *point, bool grab)
{
    QQuickPointerHandler *oldGrabber = point->grabberPointerHandler();
    if (grab && oldGrabber != this) {
        if (target()) {
            m_hadKeepMouseGrab = target()->keepMouseGrab();
            m_hadKeepTouchGrab = target()->keepTouchGrab();
        }
        if (oldGrabber)
            oldGrabber->handleGrabCancel(point);
        point->setGrabberPointerHandler(this, true);
        onGrabChanged(point);
//        emit grabChanged(point); // TODO maybe
    } else if (!grab && oldGrabber == this) {
        if (auto tgt = target()) {
            tgt->setKeepMouseGrab(m_hadKeepMouseGrab);
            tgt->setKeepTouchGrab(m_hadKeepTouchGrab);
        }
        point->setGrabberPointerHandler(nullptr, true);
        onGrabChanged(point);
//        emit grabChanged(point); // TODO maybe
    }
}

void QQuickPointerHandler::cancelAllGrabs(QQuickEventPoint *point)
{
    point->cancelAllGrabs(this);
    emit grabChanged(point);
}

QPointF QQuickPointerHandler::eventPos(const QQuickEventPoint *point) const
{
    return (target() ? target()->mapFromScene(point->scenePos()) : point->scenePos());
}

bool QQuickPointerHandler::parentContains(const QQuickEventPoint *point) const
{
    if (point) {
        if (QQuickItem *par = parentItem())
            return par->contains(par->mapFromScene(point->scenePos()));
    }
    return false;
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
    m_targetExplicitlySet = true;
    if (m_target == target)
        return;

    m_target = target;
    emit targetChanged();
}

QQuickItem *QQuickPointerHandler::target() const
{
    if (!m_targetExplicitlySet)
        return parentItem();
    return m_target;
}

void QQuickPointerHandler::handlePointerEvent(QQuickPointerEvent *event)
{
    if (wantsPointerEvent(event))
        handlePointerEventImpl(event);
    else
        setActive(false);
}

void QQuickPointerHandler::handleGrabCancel(QQuickEventPoint *point)
{
    qCDebug(lcPointerHandlerDispatch) << point;
    Q_ASSERT(point);
    setActive(false);
    point->setAccepted(false);
    emit canceled(point);
}

void QQuickPointerHandler::handleGrab(QQuickEventPoint *point, QQuickPointerHandler *grabber, bool grab)
{
    Q_UNUSED(point);
    Q_UNUSED(grabber);
    Q_UNUSED(grab);
}

bool QQuickPointerHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    Q_UNUSED(event)
    return m_enabled;
}

void QQuickPointerHandler::setActive(bool active)
{
    if (m_active != active) {
        qCDebug(lcPointerHandlerActive) << this << m_active << "->" << active;
        m_active = active;
        onActiveChanged();
        emit activeChanged();
    }
}

void QQuickPointerHandler::handlePointerEventImpl(QQuickPointerEvent *event)
{
    m_currentEvent = event;
}

/*!
    \qmlproperty QQuickPointerHandler::parent

    The \l Item which is the scope of the handler; the Item in which it was declared.
    The handler will handle events on behalf of this Item, which means a
    pointer event is relevant if at least one of its event points occurs within
    the Item's interior.  Initially \l target() is the same, but target()
    can be reassigned.

    \sa QQuickPointerHandler::target(), QObject::parent()
*/

QT_END_NAMESPACE
