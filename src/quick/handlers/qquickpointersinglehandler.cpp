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
  , m_currentPoint(nullptr)
  , m_currentPointId(0)
{
}

bool QQuickPointerSingleHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    if (!QQuickPointerDeviceHandler::wantsPointerEvent(event))
        return false;
    int c = event->pointCount();
    for (int i = 0; i < c; ++i) {
        QQuickEventPoint *p = event->point(i);
        if (m_currentPointId) {
            if (m_currentPointId == p->pointId()) {
                m_currentPoint = p;
                return true;
            }
        } else {
            if (p->grabber()) {
                if (p->grabber() == target())
                    setCurrentPoint(p);
                else
                    continue;
            } else {
                if (targetContains(p))
                    setCurrentPoint(p);
            }
            if (m_currentPoint)
                return true;
        }
    }
    // If we didn't return yet, there are no interesting points
    setCurrentPoint(nullptr);
    return false;
}

void QQuickPointerSingleHandler::handlePointerEventImpl(QQuickPointerEvent *event)
{
    QQuickPointerDeviceHandler::handlePointerEventImpl(event);
    handleEventPoint(m_currentPoint);
    bool grab = m_currentPoint->isAccepted() && m_currentPoint->state() != QQuickEventPoint::Released;
    setGrab(m_currentPoint, grab);
    if (!grab)
        setCurrentPoint(nullptr);
}

void QQuickPointerSingleHandler::setCurrentPoint(QQuickEventPoint *p)
{
    m_currentPoint = p;
    m_currentPointId = p ? p->pointId() : 0;
}

QT_END_NAMESPACE
