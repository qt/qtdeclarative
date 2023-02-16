// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickpresshandler_p_p.h"

#include <QtCore/private/qobject_p.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>
#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuickTemplates2/private/qquicktextarea_p.h>
#include <QtQuickTemplates2/private/qquicktextfield_p.h>

QT_BEGIN_NAMESPACE

void QQuickPressHandler::mousePressEvent(QMouseEvent *event)
{
    longPress = false;
    pressPos = event->position();
    if (Qt::LeftButton == (event->buttons() & Qt::LeftButton)) {
        timer.start(QGuiApplication::styleHints()->mousePressAndHoldInterval(), control);
        delayedMousePressEvent = std::make_unique<QMouseEvent>(event->type(), event->position().toPoint(), event->globalPosition().toPoint(),
                                                 event->button(), event->buttons(), event->modifiers(), event->pointingDevice());
    } else {
        timer.stop();
    }

    if (isSignalConnected(control, "pressed(QQuickMouseEvent*)", pressedSignalIndex)) {
        QQuickMouseEvent mev;
        mev.reset(pressPos.x(), pressPos.y(), event->button(), event->buttons(),
                  event->modifiers(), false/*isClick*/, false/*wasHeld*/);
        mev.setAccepted(true);
        QQuickMouseEvent *mevPtr = &mev;
        void *args[] = { nullptr, &mevPtr };
        QMetaObject::metacall(control, QMetaObject::InvokeMetaMethod, pressedSignalIndex, args);
        event->setAccepted(mev.isAccepted());
    }
}

void QQuickPressHandler::mouseMoveEvent(QMouseEvent *event)
{
    if (qAbs(int(event->position().x() - pressPos.x())) > QGuiApplication::styleHints()->startDragDistance())
        timer.stop();
}

void QQuickPressHandler::mouseReleaseEvent(QMouseEvent *event)
{
    if (!longPress) {
        timer.stop();

        if (isSignalConnected(control, "released(QQuickMouseEvent*)", releasedSignalIndex)) {
            QQuickMouseEvent mev;
            mev.reset(pressPos.x(), pressPos.y(), event->button(), event->buttons(),
                      event->modifiers(), false/*isClick*/, false/*wasHeld*/);
            mev.setAccepted(true);
            QQuickMouseEvent *mevPtr = &mev;
            void *args[] = { nullptr, &mevPtr };
            QMetaObject::metacall(control, QMetaObject::InvokeMetaMethod, releasedSignalIndex, args);
            event->setAccepted(mev.isAccepted());
        }
    }
}

void QQuickPressHandler::timerEvent(QTimerEvent *)
{
    timer.stop();
    clearDelayedMouseEvent();

    longPress = isSignalConnected(control, "pressAndHold(QQuickMouseEvent*)", pressAndHoldSignalIndex);
    if (longPress) {
        QQuickMouseEvent mev;
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        mev.reset(pressPos.x(), pressPos.y(), Qt::LeftButton, Qt::LeftButton,
                  QGuiApplication::keyboardModifiers(), false/*isClick*/, true/*wasHeld*/);
QT_WARNING_POP
        mev.setAccepted(true);
        // Use fast signal invocation since we already got its index
        QQuickMouseEvent *mevPtr = &mev;
        void *args[] = { nullptr, &mevPtr };
        QMetaObject::metacall(control, QMetaObject::InvokeMetaMethod, pressAndHoldSignalIndex, args);
        if (!mev.isAccepted())
            longPress = false;
    }
}

void QQuickPressHandler::clearDelayedMouseEvent()
{
    delayedMousePressEvent.reset();
}

bool QQuickPressHandler::isActive()
{
    return !(timer.isActive() || longPress);
}

bool QQuickPressHandler::isSignalConnected(QQuickItem *item, const char *signalName, int &signalIndex)
{
    if (signalIndex == -1)
        signalIndex = item->metaObject()->indexOfSignal(signalName);
    Q_ASSERT(signalIndex != -1);
    const auto signalMetaMethod = item->metaObject()->method(signalIndex);
    if (QQuickTextArea *textArea = qobject_cast<QQuickTextArea*>(item)) {
        return textArea->isSignalConnected(signalMetaMethod);
    } else if (QQuickTextField *textField = qobject_cast<QQuickTextField*>(item)) {
        return textField->isSignalConnected(signalMetaMethod);
    }
    qFatal("Unhandled control type for signal name: %s", signalName);
    return false;
}

QT_END_NAMESPACE
