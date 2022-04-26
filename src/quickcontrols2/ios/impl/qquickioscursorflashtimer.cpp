/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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

#include "qquickioscursorflashtimer_p.h"
#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>

QT_BEGIN_NAMESPACE

QQuickIOSCursorFlashTimer::QQuickIOSCursorFlashTimer(QObject *parent)
    : QObject(parent)
{
}

bool QQuickIOSCursorFlashTimer::visible() const
{
    return m_visible;
}

void QQuickIOSCursorFlashTimer::setVisible(bool visible)
{
    if (m_visible == visible)
        return;
    m_visible = visible;
    emit visibleChanged();
}

int QQuickIOSCursorFlashTimer::cursorPosition() const
{
    return m_cursorPosition;
}

void QQuickIOSCursorFlashTimer::setCursorPosition(int cursorPosition)
{
    if (m_cursorPosition == cursorPosition)
        return;
    m_cursorPosition = cursorPosition;
    emit cursorPositionChanged();
    start();
}

bool QQuickIOSCursorFlashTimer::running() const
{
    return m_running;
}

void QQuickIOSCursorFlashTimer::setRunning(bool running)
{
    if (running == m_running)
        return;
    m_running = running;
    emit runningChanged(m_running);
    if (!running) {
        stop();
        setVisible(false);
    } else {
        start();
    }
}

void QQuickIOSCursorFlashTimer::start()
{
    stop();
    m_timer = startTimer(QGuiApplication::styleHints()->cursorFlashTime() / 2);
}

void QQuickIOSCursorFlashTimer::stop()
{
    killTimer(m_timer);
    m_timer = 0;
}

void QQuickIOSCursorFlashTimer::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_timer)
        setVisible(!visible());
}

QT_END_NAMESPACE
