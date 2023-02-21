// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicksinglepointhandler_p.h"
#include "qquicksinglepointhandler_p_p.h"
#include <private/qquickdeliveryagent_p_p.h>

#include <private/qquickdeliveryagent_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SinglePointHandler
    \qmlabstract
    \preliminary
    \nativetype QQuickSinglePointHandler
    \inherits PointerDeviceHandler
    \inqmlmodule QtQuick
    \brief Abstract handler for single-point Pointer Events.

    An intermediate class (not registered as a QML type)
    for the most common handlers: those which expect only a single point.
    wantsPointerEvent() will choose the first point which is inside the
    \l target item, and return true as long as the event contains that point.
    Override handleEventPoint() to implement a single-point handler.
*/

QQuickSinglePointHandler::QQuickSinglePointHandler(QQuickItem *parent)
  : QQuickPointerDeviceHandler(*(new QQuickSinglePointHandlerPrivate), parent)
{
}

QQuickSinglePointHandler::QQuickSinglePointHandler(QQuickSinglePointHandlerPrivate &dd, QQuickItem *parent)
  : QQuickPointerDeviceHandler(dd, parent)
{
}

bool QQuickSinglePointHandler::wantsPointerEvent(QPointerEvent *event)
{
    Q_D(QQuickSinglePointHandler);
    if (!QQuickPointerDeviceHandler::wantsPointerEvent(event))
        return false;

    if (d->pointInfo.id() != -1) {
        // We already know which one we want, so check whether it's there.
        // It's expected to be an update or a release.
        // If we no longer want it, cancel the grab.
        int candidatePointCount = 0;
        bool missing = true;
        QEventPoint *point = nullptr;
        for (int i = 0; i < event->pointCount(); ++i) {
            auto &p = event->point(i);
            const bool found = (p.id() == d->pointInfo.id());
            if (found)
                missing = false;
            if (wantsEventPoint(event, p)) {
                ++candidatePointCount;
                if (found)
                    point = &p;
            }
        }
        if (missing) {
            // Received a stray touch begin event => reset and start over.
            if (event->type() == QEvent::TouchBegin && event->points().count() == 1) {
                const QEventPoint &point = event->point(0);
                qCDebug(lcTouchTarget) << this << "pointId" << Qt::hex << point.id()
                    << "was received as a stray TouchBegin event. Canceling existing gesture"
                    " and starting over.";
                d->pointInfo.reset(event, point);
                return true;
            } else {
                qCWarning(lcTouchTarget) << this << "pointId" << Qt::hex << d->pointInfo.id()
                    << "is missing from current event, but was neither canceled nor released."
                    " Ignoring:" << event->type();
            }
        }
        if (point) {
            if (candidatePointCount == 1 || (candidatePointCount > 1 && d->ignoreAdditionalPoints)) {
                point->setAccepted();
                return true;
            } else {
                cancelAllGrabs(event, *point);
            }
        } else {
            return false;
        }
    } else {
        // We have not yet chosen a point; choose the first one for which wantsEventPoint() returns true.
        int candidatePointCount = 0;
        QEventPoint *chosen = nullptr;
        for (int i = 0; i < event->pointCount(); ++i) {
            auto &p = event->point(i);
            if (!event->exclusiveGrabber(p) && wantsEventPoint(event, p)) {
                ++candidatePointCount;
                if (!chosen) {
                    chosen = &p;
                    break;
                }
            }
        }
        if (chosen && candidatePointCount == 1) {
            setPointId(chosen->id());
            chosen->setAccepted();
        }
    }
    return d->pointInfo.id() != -1;
}

void QQuickSinglePointHandler::handlePointerEventImpl(QPointerEvent *event)
{
    Q_D(QQuickSinglePointHandler);
    QQuickPointerDeviceHandler::handlePointerEventImpl(event);
    QEventPoint *currentPoint = const_cast<QEventPoint *>(event->pointById(d->pointInfo.id()));
    Q_ASSERT(currentPoint);
    if (!QQuickDeliveryAgentPrivate::isSynthMouse(event))
        d->pointInfo.reset(event, *currentPoint);
    handleEventPoint(event, *currentPoint);
    emit pointChanged();
}

void QQuickSinglePointHandler::handleEventPoint(QPointerEvent *event, QEventPoint &point)
{
    if (point.state() == QEventPoint::Released) {
        // If it's a mouse or tablet event, with buttons,
        // do not deactivate unless all acceptable buttons are released.
        if (event->isSinglePointEvent()) {
            const Qt::MouseButtons releasedButtons = static_cast<QSinglePointEvent *>(event)->buttons();
            if ((releasedButtons & acceptedButtons()) != Qt::NoButton)
                return;
        }

        // Deactivate this handler on release
        setExclusiveGrab(event, point, false);
        d_func()->reset();
    }
}

void QQuickSinglePointHandler::onGrabChanged(QQuickPointerHandler *grabber, QPointingDevice::GrabTransition transition, QPointerEvent *event, QEventPoint &point)
{
    Q_D(QQuickSinglePointHandler);
    if (grabber != this)
        return;
    switch (transition) {
    case QPointingDevice::GrabExclusive:
        d->pointInfo.m_sceneGrabPosition = point.sceneGrabPosition();
        setActive(true);
        QQuickPointerHandler::onGrabChanged(grabber, transition, event, point);
        break;
    case QPointingDevice::GrabPassive:
        d->pointInfo.m_sceneGrabPosition = point.sceneGrabPosition();
        QQuickPointerHandler::onGrabChanged(grabber, transition, event, point);
        break;
    case QPointingDevice::OverrideGrabPassive:
        return; // don't emit
    case QPointingDevice::UngrabPassive:
    case QPointingDevice::UngrabExclusive:
    case QPointingDevice::CancelGrabPassive:
    case QPointingDevice::CancelGrabExclusive:
        // the grab is lost or relinquished, so the point is no longer relevant
        QQuickPointerHandler::onGrabChanged(grabber, transition, event, point);
        d->reset();
        break;
    }
}

void QQuickSinglePointHandler::setIgnoreAdditionalPoints(bool v)
{
    Q_D(QQuickSinglePointHandler);
    d->ignoreAdditionalPoints = v;
}

void QQuickSinglePointHandler::moveTarget(QPointF pos, QEventPoint &point)
{
    Q_D(QQuickSinglePointHandler);
    target()->setPosition(pos);
    d->pointInfo.m_scenePosition = point.scenePosition();
    d->pointInfo.m_position = target()->mapFromScene(d->pointInfo.m_scenePosition);
}

void QQuickSinglePointHandler::setPointId(int id)
{
    Q_D(QQuickSinglePointHandler);
    d->pointInfo.m_id = id;
}

QQuickHandlerPoint QQuickSinglePointHandler::point() const
{
    Q_D(const QQuickSinglePointHandler);
    return d->pointInfo;
}

/*!
    \readonly
    \qmlproperty handlerPoint QtQuick::SinglePointHandler::point

    The \l eventPoint currently being handled. When no point is currently being
    handled, this object is reset to default values (all coordinates are 0).
*/

QQuickSinglePointHandlerPrivate::QQuickSinglePointHandlerPrivate()
  : QQuickPointerDeviceHandlerPrivate()
{
}

void QQuickSinglePointHandlerPrivate::reset()
{
    Q_Q(QQuickSinglePointHandler);
    q->setActive(false);
    pointInfo.reset();
}

QT_END_NAMESPACE

#include "moc_qquicksinglepointhandler_p.cpp"
