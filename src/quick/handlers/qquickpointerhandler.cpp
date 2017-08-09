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

/*!
    Notification that the grab has changed in some way which is relevant to this handler.
    The \a grabber (subject) will be the PointerHandler whose state is changing,
    or null if the state change regards an Item. (TODO do we have any such cases?)
    The \a stateChange (verb) tells what happened.
    The \a point (object) is the point that was grabbed or ungrabbed.
    EventPoint has the sole responsibility to call this function.
    The PointerHandler must react in whatever way is appropriate, and must
    emit the relevant signals (for the benefit of QML code).
    A subclass is allowed to override this virtual function, but must always
    call its parent class's implementation in addition to (usually after)
    whatever custom behavior it implements.
*/
void QQuickPointerHandler::onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabState stateChange, QQuickEventPoint *point)
{
    qCDebug(lcPointerHandlerDispatch) << point << stateChange << grabber;
    Q_ASSERT(point);
    if (grabber == this) {
        bool wasCanceled = false;
        emit grabChanged(point);
        switch (stateChange) {
        case QQuickEventPoint::GrabPassive:
        case QQuickEventPoint::GrabExclusive:
            break;
        case QQuickEventPoint::CancelGrabPassive:
        case QQuickEventPoint::CancelGrabExclusive:
            wasCanceled = true; // the grab was stolen by something else
            Q_FALLTHROUGH();
        case QQuickEventPoint::UngrabPassive:
        case QQuickEventPoint::UngrabExclusive:
            setActive(false);
            point->setAccepted(false);
            if (auto par = parentItem()) {
                par->setKeepMouseGrab(m_hadKeepMouseGrab);
                par->setKeepTouchGrab(m_hadKeepTouchGrab);
            }
        case QQuickEventPoint::OverrideGrabPassive:
            // Passive grab is still there, but we won't receive point updates right now.
            // No need to notify about this.
            return;
        }
        if (wasCanceled)
            emit canceled(point);
        else
            emit grabChanged(point);
    }
}

void QQuickPointerHandler::setPassiveGrab(QQuickEventPoint *point, bool grab)
{
    qCDebug(lcPointerHandlerDispatch) << point << grab;
    if (grab) {
        point->setGrabberPointerHandler(this, false);
    } else {
        point->removePassiveGrabber(this);
    }
}

void QQuickPointerHandler::setExclusiveGrab(QQuickEventPoint *point, bool grab)
{
    // TODO m_hadKeepMouseGrab m_hadKeepTouchGrab
    qCDebug(lcPointerHandlerDispatch) << point << grab;
    // Don't allow one handler to cancel another's grab, unless it is stealing it for itself
    if (!grab && point->grabberPointerHandler() != this)
        return;
    point->setGrabberPointerHandler(grab ? this : nullptr, true);
}

void QQuickPointerHandler::cancelAllGrabs(QQuickEventPoint *point)
{
    qCDebug(lcPointerHandlerDispatch) << point;
    point->cancelAllGrabs(this);
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
    bool wants = wantsPointerEvent(event);
    qCDebug(lcPointerHandlerDispatch) << metaObject()->className() << objectName()
                                      << "on" << parentItem()->metaObject()->className() << parentItem()->objectName()
                                      << (wants ? "WANTS" : "DECLINES") << event;
    if (wants) {
        handlePointerEventImpl(event);
    } else {
        setActive(false);
        int pCount = event->pointCount();
        for (int i = 0; i < pCount; ++i) {
            QQuickEventPoint *pt = event->point(i);
            if (pt->grabberPointerHandler() == this && pt->state() != QQuickEventPoint::Stationary)
                pt->cancelExclusiveGrab();
        }
    }
    event->device()->eventDeliveryTargets().append(this);
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
