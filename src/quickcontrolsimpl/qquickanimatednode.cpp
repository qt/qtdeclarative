// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    m_timer.start();
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
    m_timer.start();
    if (duration > 0)
        m_duration = duration;

    connect(m_window, &QQuickWindow::afterSynchronizing, this, &QQuickAnimatedNode::advance, Qt::DirectConnection);
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
    disconnect(m_window, &QQuickWindow::afterSynchronizing, this, &QQuickAnimatedNode::advance);
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

#include "moc_qquickanimatednode_p.cpp"
