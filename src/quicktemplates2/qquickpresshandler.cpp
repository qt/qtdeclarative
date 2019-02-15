/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    pressPos = event->localPos();
    if (Qt::LeftButton == (event->buttons() & Qt::LeftButton)) {
        timer.start(QGuiApplication::styleHints()->mousePressAndHoldInterval(), control);
        delayedMousePressEvent = new QMouseEvent(event->type(), event->pos(), event->button(), event->buttons(), event->modifiers());
    } else {
        timer.stop();
    }

    if (isSignalConnected(control, "pressed(QQuickMouseEvent*)", pressedSignalIndex)) {
        QQuickMouseEvent mev;
        mev.reset(pressPos.x(), pressPos.y(), event->button(), event->buttons(),
                  QGuiApplication::keyboardModifiers(), false/*isClick*/, false/*wasHeld*/);
        mev.setAccepted(true);
        QQuickMouseEvent *mevPtr = &mev;
        void *args[] = { nullptr, &mevPtr };
        QMetaObject::metacall(control, QMetaObject::InvokeMetaMethod, pressedSignalIndex, args);
        event->setAccepted(mev.isAccepted());
    }
}

void QQuickPressHandler::mouseMoveEvent(QMouseEvent *event)
{
    if (qAbs(int(event->localPos().x() - pressPos.x())) > QGuiApplication::styleHints()->startDragDistance())
        timer.stop();
}

void QQuickPressHandler::mouseReleaseEvent(QMouseEvent *event)
{
    if (!longPress) {
        timer.stop();

        if (isSignalConnected(control, "released(QQuickMouseEvent*)", releasedSignalIndex)) {
            QQuickMouseEvent mev;
            mev.reset(pressPos.x(), pressPos.y(), event->button(), event->buttons(),
                      QGuiApplication::keyboardModifiers(), false/*isClick*/, false/*wasHeld*/);
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
        mev.reset(pressPos.x(), pressPos.y(), Qt::LeftButton, Qt::LeftButton,
                  QGuiApplication::keyboardModifiers(), false/*isClick*/, true/*wasHeld*/);
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
    if (delayedMousePressEvent) {
        delete delayedMousePressEvent;
        delayedMousePressEvent = 0;
    }
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
