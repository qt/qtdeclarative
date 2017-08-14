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
    \qmlabstract
    \since 5.10
    \preliminary
    \instantiates QQuickPointerHandler
    \inqmlmodule Qt.labs.handlers
    \ingroup qtquick-handlers
    \brief Abstract handler for pointer events.

    PointerHandler is the base class handler (not registered as a QML type) for
    pointer events without regard to source (touch, mouse or graphics tablet).
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
    or null if the state change regards an Item.
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
            break;
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

/*!
    \internal
    Acquire or give up a passive grab of the given \a point, according to the \a grab state.

    Unlike the exclusive grab, multiple PointerHandlers can have passive grabs
    simultaneously. This means that each of them will receive further events
    when the \a point moves, and when it is finally released. Typically a
    PointerHandler should acquire a passive grab as soon as a point is pressed,
    if the handler's constraints do not clearly rule out any interest in that
    point. For example, DragHandler needs a passive grab in order to watch the
    movement of a point to see whether it will be dragged past the drag
    threshold. When a handler is actively manipulating its \l target (that is,
    when \l active is true), it may be able to do its work with only a passive
    grab, or it may acquire an exclusive grab if the gesture clearly must not
    be interpreted in another way by another handler.
*/
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

/*!
    \internal
    Cancel any existing grab of the given \a point.
*/
void QQuickPointerHandler::cancelAllGrabs(QQuickEventPoint *point)
{
    qCDebug(lcPointerHandlerDispatch) << point;
    point->cancelAllGrabs(this);
}

QPointF QQuickPointerHandler::eventPos(const QQuickEventPoint *point) const
{
    return (target() ? target()->mapFromScene(point->scenePosition()) : point->scenePosition());
}

bool QQuickPointerHandler::parentContains(const QQuickEventPoint *point) const
{
    if (point) {
        if (QQuickItem *par = parentItem())
            return par->contains(par->mapFromScene(point->scenePosition()));
    }
    return false;
}

/*!
     \qmlproperty bool QtQuick::PointerHandler::enabled

     If a PointerHandler is disabled, it will reject all events
     and no signals will be emitted.
*/
void QQuickPointerHandler::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    emit enabledChanged();
}

/*!
    \qmlproperty Item QtQuick::PointerHandler::target

    The Item which this handler will manipulate.

    By default, it is the same as the \l parent: the Item within which
    the handler is declared. However, it can sometimes be useful to set the
    target to a different Item, in order to handle events within one item
    but manipulate another; or to \c null, to disable the default behavior
    and do something else instead.
*/
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

/*!
    \readonly
    \qmlproperty bool QtQuick::PointerHandler::active

    This holds true whenever this PointerHandler has taken sole responsibility
    for handing one or more EventPoints, by successfully taking an exclusive
    grab of those points. This means that it is keeping its properties
    up-to-date according to the movements of those Event Points and actively
    manipulating its \l target (if any).
*/
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
    \readonly
    \qmlproperty Item QtQuick::PointerHandler::parent

    The \l Item which is the scope of the handler; the Item in which it was declared.
    The handler will handle events on behalf of this Item, which means a
    pointer event is relevant if at least one of its event points occurs within
    the Item's interior.  Initially \l target() is the same, but target()
    can be reassigned.

    \sa QQuick::PointerHandler::target(), QObject::parent()
*/

/*!
    \qmlsignal QtQuick::PointerHandler::grabChanged(EventPoint point)

    This signal is emitted when this handler has acquired or relinquished a
    passive or exclusive grab of the given \a point.
*/

/*!
    \qmlsignal QtQuick::PointerHandler::canceled(EventPoint point)

    If this handler has already grabbed the given \a point, this signal is
    emitted when the grab is stolen by a different Pointer Handler or Item.
*/

QT_END_NAMESPACE
