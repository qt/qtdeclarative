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

#include "qquicktaphandler_p.h"
#include <qpa/qplatformtheme.h>
#include <private/qguiapplication_p.h>
#include <QtGui/qstylehints.h>

QT_BEGIN_NAMESPACE

qreal QQuickTapHandler::m_multiTapInterval(0.0);
// single tap distance is the same as the drag threshold
int QQuickTapHandler::m_mouseMultiClickDistanceSquared(-1);
int QQuickTapHandler::m_touchMultiTapDistanceSquared(-1);

/*!
    \qmltype TapHandler
    \instantiates QQuickTapHandler
    \inqmlmodule QtQuick
    \ingroup qtquick-handlers
    \brief Handler for taps and clicks

    TapHandler is a handler for taps on a touchscreen or clicks on a mouse.

    It requires that any movement between the press and release remains
    less than the drag threshold for a single tap, and less than
    QPlatformTheme::MouseDoubleClickDistance for multi-tap gestures
    (double-tap, triple-tap, etc.) with the mouse, or 10 pixels with touch.
    It also requires that the time between press and release remains
    less than QStyleHints::mouseDoubleClickInterval() for a single tap,
    and that the time from one press/release sequence to the next remains
    less than QStyleHints::mouseDoubleClickInterval() for multi-tap gestures.

    Note that buttons (such as QPushButton) are often implemented not to care
    whether the press and release occur close together: if you press the button
    and then change your mind, you need to drag all the way off the edge of the
    button in order to cancel the click.  If you want to achieve such behavior,
    it's enough to use a PointerHandler and consider the button clicked on
    every \l {QQuickPointerHandler:}{released} event.  But TapHandler requires
    that the events occur close together in both space and time, which is anyway
    necessary to detect double clicks or multi-click gestures.

    \sa MouseArea
*/

QQuickTapHandler::QQuickTapHandler(QObject *parent)
    : QQuickPointerSingleHandler(parent)
    , m_pressed(false)
    , m_tapCount(0)
    , m_lastTapTimestamp(0.0)
{
    setAcceptedButtons(Qt::LeftButton);
    if (m_mouseMultiClickDistanceSquared < 0) {
        m_multiTapInterval = qApp->styleHints()->mouseDoubleClickInterval() / 1000.0;
        m_mouseMultiClickDistanceSquared = QGuiApplicationPrivate::platformTheme()->
                    themeHint(QPlatformTheme::MouseDoubleClickDistance).toInt();
        m_mouseMultiClickDistanceSquared *= m_mouseMultiClickDistanceSquared;
        m_touchMultiTapDistanceSquared = QGuiApplicationPrivate::platformTheme()->
                    themeHint(QPlatformTheme::TouchDoubleTapDistance).toInt();
        m_touchMultiTapDistanceSquared *= m_touchMultiTapDistanceSquared;
    }
}

QQuickTapHandler::~QQuickTapHandler()
{
}

bool QQuickTapHandler::wantsEventPoint(QQuickEventPoint *point)
{
    if (point->state() == QQuickEventPoint::Pressed && parentContains(point))
        return true;
    // If the user has not dragged too far, it could be a tap.
    // Otherwise we want to give up the grab so that a competing handler
    // (e.g. DragHandler) gets a chance to take over.
    // Don't forget to emit released in case of a cancel.
    return !point->isDraggedOverThreshold();
}

void QQuickTapHandler::handleEventPoint(QQuickEventPoint *point)
{
    switch (point->state()) {
    case QQuickEventPoint::Pressed:
        setPressed(true, false, point);
        break;
    case QQuickEventPoint::Released:
        if ((point->pointerEvent()->buttons() & m_acceptedButtons) == Qt::NoButton)
            setPressed(false, false, point);
        break;
    default:
        break;
    }
}

void QQuickTapHandler::setPressed(bool press, bool cancel, QQuickEventPoint *point)
{
    if (m_pressed != press) {
        m_pressed = press;
        if (!cancel && !press && point->timeHeld() < m_multiTapInterval) {
            // Assuming here that pointerEvent()->timestamp() is in ms.
            qreal ts = point->pointerEvent()->timestamp() / 1000.0;
            if (ts - m_lastTapTimestamp < m_multiTapInterval &&
                    QVector2D(point->scenePos() - m_lastTapPos).lengthSquared() <
                    (point->pointerEvent()->device()->type() == QQuickPointerDevice::Mouse ?
                     m_mouseMultiClickDistanceSquared : m_touchMultiTapDistanceSquared))
                ++m_tapCount;
            else
                m_tapCount = 1;
            emit tapped(point);
            emit tapCountChanged();
            m_lastTapTimestamp = ts;
            m_lastTapPos = point->scenePos();
        }
        emit pressedChanged();
    }
}

void QQuickTapHandler::handleGrabCancel(QQuickEventPoint *point)
{
    QQuickPointerSingleHandler::handleGrabCancel(point);
    setPressed(false, true, point);
}

/*!
    \qmlproperty tapCount

    The number of taps which have occurred within the time and space
    constraints to be considered a single gesture.  For example, to detect
    a double-tap, you can write

    \qml
    Rectangle {
        width: 100; height: 30
        signal doubleTap
        TapHandler {
            acceptedButtons: Qt.AllButtons
            onTapped: if (tapCount == 2) doubleTap()
        }
    }
*/

QT_END_NAMESPACE
