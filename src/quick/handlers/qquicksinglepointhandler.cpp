/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qquicksinglepointhandler_p.h"
#include "qquicksinglepointhandler_p_p.h"

QT_BEGIN_NAMESPACE
Q_DECLARE_LOGGING_CATEGORY(DBG_TOUCH_TARGET)

/*!
    \qmltype SinglePointHandler
    \qmlabstract
    \preliminary
    \instantiates QQuickSinglePointHandler
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

bool QQuickSinglePointHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    Q_D(QQuickSinglePointHandler);
    if (!QQuickPointerDeviceHandler::wantsPointerEvent(event))
        return false;

    if (d->pointInfo.id()) {
        // We already know which one we want, so check whether it's there.
        // It's expected to be an update or a release.
        // If we no longer want it, cancel the grab.
        int candidatePointCount = 0;
        bool missing = true;
        QQuickEventPoint *point = nullptr;
        int c = event->pointCount();
        for (int i = 0; i < c; ++i) {
            QQuickEventPoint *p = event->point(i);
            const bool found = (p->pointId() == d->pointInfo.id());
            if (found)
                missing = false;
            if (wantsEventPoint(p)) {
                ++candidatePointCount;
                if (found)
                    point = p;
            }
        }
        if (missing)
            qCWarning(DBG_TOUCH_TARGET) << this << "pointId" << Qt::hex << d->pointInfo.id()
                << "is missing from current event, but was neither canceled nor released";
        if (point) {
            if (candidatePointCount == 1 || (candidatePointCount > 1 && d->ignoreAdditionalPoints)) {
                point->setAccepted();
                return true;
            } else {
                point->cancelAllGrabs(this);
            }
        } else {
            return false;
        }
    } else {
        // We have not yet chosen a point; choose the first one for which wantsEventPoint() returns true.
        int candidatePointCount = 0;
        int c = event->pointCount();
        QQuickEventPoint *chosen = nullptr;
        for (int i = 0; i < c && !chosen; ++i) {
            QQuickEventPoint *p = event->point(i);
            if (!p->exclusiveGrabber() && wantsEventPoint(p)) {
                if (!chosen)
                    chosen = p;
                ++candidatePointCount;
            }
        }
        if (chosen && candidatePointCount == 1) {
            setPointId(chosen->pointId());
            chosen->setAccepted();
        }
    }
    return d->pointInfo.id();
}

void QQuickSinglePointHandler::handlePointerEventImpl(QQuickPointerEvent *event)
{
    Q_D(QQuickSinglePointHandler);
    QQuickPointerDeviceHandler::handlePointerEventImpl(event);
    QQuickEventPoint *currentPoint = event->pointById(d->pointInfo.id());
    Q_ASSERT(currentPoint);
    d->pointInfo.reset(currentPoint);
    handleEventPoint(currentPoint);
    if (currentPoint->state() == QQuickEventPoint::Released && (event->buttons() & acceptedButtons()) == Qt::NoButton) {
        setExclusiveGrab(currentPoint, false);
        d->reset();
    }
    emit pointChanged();
}

void QQuickSinglePointHandler::onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabTransition transition, QQuickEventPoint *point)
{
    Q_D(QQuickSinglePointHandler);
    if (grabber != this)
        return;
    switch (transition) {
    case QQuickEventPoint::GrabExclusive:
        d->pointInfo.m_sceneGrabPosition = point->sceneGrabPosition();
        setActive(true);
        QQuickPointerHandler::onGrabChanged(grabber, transition, point);
        break;
    case QQuickEventPoint::GrabPassive:
        d->pointInfo.m_sceneGrabPosition = point->sceneGrabPosition();
        QQuickPointerHandler::onGrabChanged(grabber, transition, point);
        break;
    case QQuickEventPoint::OverrideGrabPassive:
        return; // don't emit
    case QQuickEventPoint::UngrabPassive:
    case QQuickEventPoint::UngrabExclusive:
    case QQuickEventPoint::CancelGrabPassive:
    case QQuickEventPoint::CancelGrabExclusive:
        // the grab is lost or relinquished, so the point is no longer relevant
        QQuickPointerHandler::onGrabChanged(grabber, transition, point);
        d->reset();
        break;
    }
}

void QQuickSinglePointHandler::setIgnoreAdditionalPoints(bool v)
{
    Q_D(QQuickSinglePointHandler);
    d->ignoreAdditionalPoints = v;
}

void QQuickSinglePointHandler::moveTarget(QPointF pos, QQuickEventPoint *point)
{
    Q_D(QQuickSinglePointHandler);
    target()->setPosition(pos);
    d->pointInfo.m_scenePosition = point->scenePosition();
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
    \qmlproperty HandlerPoint QtQuick::SinglePointHandler::point

    The event point currently being handled. When no point is currently being
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
