// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qquickeventreplayservice.h"

#include <QtQuick/qquickwindow.h>

#include <QtGui/qtguiglobal.h>
#include <QtGui/qtestsupport_gui.h>
#include <QtGui/qwindow.h>

QT_BEGIN_NAMESPACE

Q_GUI_EXPORT void qt_handleMouseEvent(
        QWindow *window, const QPointF &local, const QPointF &global, Qt::MouseButtons state,
        Qt::MouseButton button, QEvent::Type type, Qt::KeyboardModifiers mods, int timestamp);

Q_GUI_EXPORT void qt_handleKeyEvent(
        QWindow *w, QEvent::Type t, int k, Qt::KeyboardModifiers mods,
        const QString &text = QString(), bool autorep = false, ushort count = 1);

Q_GUI_EXPORT void qt_handleWheelEvent(
        QWindow *window, const QPointF &local, const QPointF &global, QPoint pixelDelta,
        QPoint angleDelta, Qt::KeyboardModifiers mods, Qt::ScrollPhase phase);

QQuickEventReplayServiceImpl::QQuickEventReplayServiceImpl(QObject *parent)
    : QQuickEventReplayService(1, parent)
{
    m_elapsed.start();
    start();
}

void QQuickEventReplayServiceImpl::messageReceived(const QByteArray &message)
{
    QQmlDebugPacket stream(message);
    QQuickProfilerData data;

    stream >> data.time >> data.messageType >> data.detailType;

    if (data.messageType != Message::Event)
        return;

    switch (data.detailType) {
    case EventType::Key:
    case EventType::Mouse:
        break;
    default:
        return;
    }

    stream >> data.inputType >> data.inputA >> data.inputB;

    bool isFirstEvent = false;
    {
        QMutexLocker lock(&m_dataMutex);
        isFirstEvent = m_data.isEmpty();
        m_data.enqueue(std::move(data));
    }

    if (isFirstEvent)
        emit dataAvailable();
}

static QWindow *targetWindow()
{
    // If we have an exposed focus window, that's the one to receive events.
    if (QWindow *focusWindow = QGuiApplication::focusWindow()) {
        if (focusWindow->isExposed() && focusWindow->width() != 0 && focusWindow->height() != 0)
            return focusWindow;
    }

    // Otherwise, if we have exactly one QQuickWindow, use that.
    // Otherwise, we don't know what to do.
    QWindow *found = nullptr;
    const QWindowList windowList = QGuiApplication::allWindows();
    for (QWindow *window : windowList) {
        if (!qobject_cast<QQuickWindow *>(window))
            continue;
        if (!window->isExposed() || window->width() == 0 || window->height() == 0)
            continue;
        if (found)
            return nullptr;
        found = window;
    }

    return found;
}

void QQuickEventReplayServiceImpl::start()
{
    if (!targetWindow()) {
        // Poll on a timer since focusing the window is not the only way to
        // produce a unique target. Having exactly one window also works.
        QTimer::singleShot(16, this, &QQuickEventReplayServiceImpl::start);

        QMutexLocker lock(&m_dataMutex);
        if (!m_data.isEmpty()) {
            qWarning() << "Cannot determine target window for event replay. "
                          "Focus a window to use it.";
        }
        return;
    }

    QObject::connect(
            &m_schedule, &QTimer::timeout,
            this, &QQuickEventReplayServiceImpl::sendNextEvent);
    QObject::connect(this, &QQuickEventReplayServiceImpl::dataAvailable, this, [this]() {
        QMutexLocker lock(&m_dataMutex);
        scheduleNextEvent(m_data.head());
    });

    QMutexLocker lock(&m_dataMutex);
    if (!m_data.isEmpty())
        scheduleNextEvent(m_data.head());
}

static QEvent::Type eventType(int profilerEventType)
{
    using InputEventType = QQmlProfilerDefinitions::InputEventType;
    switch (profilerEventType) {
    case InputEventType::InputKeyPress:
        return QEvent::KeyPress;
    case InputEventType::InputKeyRelease:
        return QEvent::KeyRelease;
    case InputEventType::InputMousePress:
        return QEvent::MouseButtonPress;
    case InputEventType::InputMouseDoubleClick:
        return QEvent::MouseButtonDblClick;
    case InputEventType::InputMouseMove:
        return QEvent::MouseMove;
    case InputEventType::InputMouseRelease:
        return QEvent::MouseButtonRelease;
    case InputEventType::InputMouseWheel:
        return QEvent::Wheel;
    default:
        return QEvent::None;
    }
}

QQuickProfilerData QQuickEventReplayServiceImpl::takeNextEvent()
{
    QMutexLocker lock(&m_dataMutex);
    const QQuickProfilerData data = m_data.dequeue();
    if (!m_data.isEmpty())
        scheduleNextEvent(m_data.head());
    else
        m_schedule.stop();
    return data;
}

void QQuickEventReplayServiceImpl::sendNextEvent()
{
    QWindow *window = targetWindow();
    if (!window) {
        qWarning() << "Target window has disappeared during event replay";
        return;
    }

    const QQuickProfilerData data = takeNextEvent();
    Q_ASSERT(data.messageType == Message::Event);

    const QEvent::Type type = eventType(data.inputType);
    switch (data.detailType) {
    case EventType::Key: {
        qt_handleKeyEvent(window, type, data.inputA, Qt::KeyboardModifiers(data.inputB));
        break;
    }
    case EventType::Mouse: {
        switch (data.inputType) {
        case InputEventType::InputMouseMove: {
            m_currentPos = QPoint(data.inputA, data.inputB);
            qt_handleMouseEvent(
                    window, m_currentPos, window->mapToGlobal(m_currentPos), m_currentButtons,
                    Qt::NoButton, type, m_currentModifiers, m_elapsed.elapsed());
            break;
        }
        case InputEventType::InputMouseWheel: {
            qt_handleWheelEvent(
                    window, m_currentPos, window->mapToGlobal(m_currentPos), QPoint(),
                    QPoint(data.inputA, data.inputB), m_currentModifiers, Qt::ScrollUpdate);
            break;
        }
        case InputEventType::InputMouseDoubleClick:
            // Ingore double clicks. We consider the constituent low level events instead.
            break;
        default: {
            m_currentButtons = Qt::MouseButtons(data.inputB);
            qt_handleMouseEvent(
                    window, m_currentPos, window->mapToGlobal(m_currentPos), m_currentButtons,
                    Qt::MouseButton(data.inputA), type, m_currentModifiers, m_elapsed.elapsed());
        }
        }
    }
    }
}

void QQuickEventReplayServiceImpl::scheduleNextEvent(const QQuickProfilerData &nextEvent)
{
    // We assume that our timer starts at the same time as the timer of the program that
    // recorded the events. That's really the best thing we can do because both of them
    // effectively start at an arbitrary point in time. Trying to calculate a clock skew
    // here is guesswork at best. The client can front-pad the timestamps if the transfer
    // mechanism is so slow that it influences the accuracy of the playback.
    // QTimer only has millisecond resolution but that should be good enough for input events.
    m_schedule.start(std::max((nextEvent.time - m_elapsed.nsecsElapsed()) / 1000000ll, 1ll));
}

QT_END_NAMESPACE
