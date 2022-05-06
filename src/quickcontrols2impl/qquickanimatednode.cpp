/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

#include "qquickanimatednode_p.h"

#include <QtQuick/qquickitem.h>
#include <QtQuick/qquickwindow.h>

// based on qtdeclarative/examples/quick/scenegraph/threadedanimation

QT_BEGIN_NAMESPACE

QQuickAnimatedNode::QQuickAnimatedNode(QQuickItem *target)
    : m_window(target->window())
{
}

bool QQuickAnimatedNode::isRunning() const
{
    return m_running;
}

int QQuickAnimatedNode::currentTime() const
{
    int time = m_currentTime;
    if (m_running)
        time += m_timer.elapsed();
    return time;
}

void QQuickAnimatedNode::setCurrentTime(int time)
{
    m_currentTime = time;
    m_timer.restart();
}

int QQuickAnimatedNode::duration() const
{
    return m_duration;
}

void QQuickAnimatedNode::setDuration(int duration)
{
    m_duration = duration;
}

int QQuickAnimatedNode::loopCount() const
{
    return m_loopCount;
}

void QQuickAnimatedNode::setLoopCount(int count)
{
    m_loopCount = count;
}

void QQuickAnimatedNode::sync(QQuickItem *target)
{
    Q_UNUSED(target);
}

QQuickWindow *QQuickAnimatedNode::window() const
{
    return m_window;
}

void QQuickAnimatedNode::start(int duration)
{
    if (m_running)
        return;

    m_running = true;
    m_currentLoop = 0;
    m_timer.restart();
    if (duration > 0)
        m_duration = duration;

    connect(m_window, &QQuickWindow::beforeRendering, this, &QQuickAnimatedNode::advance, Qt::DirectConnection);
    connect(m_window, &QQuickWindow::frameSwapped, this, &QQuickAnimatedNode::update, Qt::DirectConnection);

    // If we're inside a QQuickWidget, this call is necessary to ensure the widget
    // gets updated for the first time.
    m_window->update();

    emit started();
}

void QQuickAnimatedNode::restart()
{
    stop();
    start();
}

void QQuickAnimatedNode::stop()
{
    if (!m_running)
        return;

    m_running = false;
    disconnect(m_window, &QQuickWindow::beforeRendering, this, &QQuickAnimatedNode::advance);
    disconnect(m_window, &QQuickWindow::frameSwapped, this, &QQuickAnimatedNode::update);
    emit stopped();
}

void QQuickAnimatedNode::updateCurrentTime(int time)
{
    Q_UNUSED(time);
}

void QQuickAnimatedNode::advance()
{
    int time = currentTime();
    if (time > m_duration) {
        time = 0;
        setCurrentTime(0);

        if (m_loopCount > 0 && ++m_currentLoop >= m_loopCount) {
            time = m_duration; // complete
            stop();
        }
    }
    updateCurrentTime(time);

    // If we're inside a QQuickWidget, this call is necessary to ensure the widget gets updated.
    m_window->update();
}

void QQuickAnimatedNode::update()
{
    if (m_running)
        m_window->update();
}

QT_END_NAMESPACE
