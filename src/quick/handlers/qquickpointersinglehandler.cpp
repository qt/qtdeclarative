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

#include "qquickpointersinglehandler_p.h"

QT_BEGIN_NAMESPACE

/*!
    An intermediate class (not registered as a QML type)
    for the most common handlers: those which expect only a single point.
    wantsPointerEvent() will choose the first point which is inside the
    \l target item, and return true as long as the event contains that point.
    Override handleEventPoint() to implement a single-point handler.
*/

QQuickPointerSingleHandler::QQuickPointerSingleHandler(QObject *parent)
  : QQuickPointerDeviceHandler(parent)
  , m_currentPointId(0)
{
}

bool QQuickPointerSingleHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    if (!QQuickPointerDeviceHandler::wantsPointerEvent(event))
        return false;
    if (m_currentPointId) {
        // We already know which one we want, so check whether it's there.
        // It's expected to be an update or a release.
        return (event->pointById(m_currentPointId) != nullptr);
    } else {
        // We have not yet chosen a point; choose the first one within target bounds.
        int c = event->pointCount();
        for (int i = 0; i < c && !m_currentPointId; ++i) {
            QQuickEventPoint *p = event->point(i);
            if (!p->grabber() && targetContains(p))
                m_currentPointId = p->pointId();
        }
    }
    return m_currentPointId;
}

void QQuickPointerSingleHandler::handlePointerEventImpl(QQuickPointerEvent *event)
{
    QQuickPointerDeviceHandler::handlePointerEventImpl(event);
    QQuickEventPoint *currentPoint = event->pointById(m_currentPointId);
    Q_ASSERT(currentPoint);
    currentPoint->setAccepted(true);
    handleEventPoint(currentPoint);
    bool grab = currentPoint->isAccepted() && currentPoint->state() != QQuickEventPoint::Released;
    setGrab(currentPoint, grab);
    if (!grab)
        m_currentPointId = 0;
}

void QQuickPointerSingleHandler::setPressedButtons(Qt::MouseButtons buttons)
{
    if (buttons != m_pressedButtons) {
        m_pressedButtons = buttons;
        emit pressedButtonsChanged();
    }
}

QT_END_NAMESPACE
