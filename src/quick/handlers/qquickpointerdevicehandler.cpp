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

#include "qquickpointerdevicehandler_p.h"
#include <private/qquickitem_p.h>
#include <QMouseEvent>
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmltype PointerDeviceHandler
    \qmlabstract
    \since 5.10
    \preliminary
    \instantiates QQuickPointerDeviceHandler
    \inherits PointerHandler
    \inqmlmodule Qt.labs.handlers
    \ingroup qtquick-handlers
    \brief Abstract handler for pointer events with device-specific constraints.

    An intermediate class (not registered as a QML type) for handlers which
    allow filtering based on device type, pointer type, or keyboard modifiers.
*/
QQuickPointerDeviceHandler::QQuickPointerDeviceHandler(QObject *parent)
    : QQuickPointerHandler(parent)
    , m_acceptedDevices(QQuickPointerDevice::AllDevices)
    , m_acceptedPointerTypes(QQuickPointerDevice::AllPointerTypes)
    , m_acceptedModifiers(Qt::KeyboardModifierMask)
{
}

QQuickPointerDeviceHandler::~QQuickPointerDeviceHandler()
{
}

/*!
    \qmlproperty int PointerDeviceHandler::acceptedDevices

    The types of pointing devices that can activate this Pointer Handler.

    By default, this property is set to
    \l{QtQuick::PointerDevice::type} {PointerDevice.AllDevices}.
    If you set it to an OR combination of device types, it will ignore events
    from non-matching devices.

    For example, a control could be made to respond to mouse and stylus clicks
    in one way, and touchscreen taps in another way, with two handlers:

    \qml
    Item {
       TapHandler {
           acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus
           onTapped: console.log("clicked")
       }
       TapHandler {
           acceptedDevices: PointerDevice.TouchScreen
           onTapped: console.log("tapped")
       }
    }
    \endqml
*/
void QQuickPointerDeviceHandler::setAcceptedDevices(QQuickPointerDevice::DeviceTypes acceptedDevices)
{
    if (m_acceptedDevices == acceptedDevices)
        return;

    m_acceptedDevices = acceptedDevices;
    emit acceptedDevicesChanged();
}

/*!
    \qmlproperty int PointerDeviceHandler::acceptedPointerTypes

    The types of pointing instruments (finger, stylus, eraser, etc.)
    that can activate this Pointer Handler.

    By default, this property is set to
    \l {QtQuick::PointerDevice::pointerType} {PointerDevice.AllPointerTypes}.
    If you set it to an OR combination of device types, it will ignore events
    from non-matching events.

    For example, a control could be made to respond to mouse, touch, and stylus clicks
    in some way, but delete itself if tapped with an eraser tool on a graphics tablet,
    with two handlers:

    \qml
    Rectangle {
       id: rect
       TapHandler {
           acceptedPointerTypes: PointerDevice.GenericPointer | PointerDevice.Finger | PointerDevice.Pen
           onTapped: console.log("clicked")
       }
       TapHandler {
           acceptedPointerTypes: PointerDevice.Eraser
           onTapped: rect.destroy()
       }
    }
    \endqml
*/
void QQuickPointerDeviceHandler::setAcceptedPointerTypes(QQuickPointerDevice::PointerTypes acceptedPointerTypes)
{
    if (m_acceptedPointerTypes == acceptedPointerTypes)
        return;

    m_acceptedPointerTypes = acceptedPointerTypes;
    emit acceptedPointerTypesChanged();
}

/*!
    \qmlproperty int PointerDeviceHandler::acceptedModifiers

    If this property is set, it will require the given keyboard modifiers to
    be pressed in order to react to pointer events, and otherwise ignore them.

    If this property is set to \c Qt.KeyboardModifierMask (the default value),
    then the PointerHandler ignores the modifier keys.

    For example, an \l [QML] Item could have two handlers of the same type,
    one of which is enabled only if the required keyboard modifiers are
    pressed:

    \qml
    Item {
       TapHandler {
           acceptedModifiers: Qt.ControlModifier
           onTapped: console.log("control-tapped")
       }
       TapHandler {
           acceptedModifiers: Qt.NoModifier
           onTapped: console.log("tapped")
       }
    }
    \endqml
*/
void QQuickPointerDeviceHandler::setAcceptedModifiers(Qt::KeyboardModifiers acceptedModifiers)
{
    if (m_acceptedModifiers == acceptedModifiers)
        return;

    m_acceptedModifiers = acceptedModifiers;
    emit acceptedModifiersChanged();
}

bool QQuickPointerDeviceHandler::wantsPointerEvent(QQuickPointerEvent *event)
{
    if (!QQuickPointerHandler::wantsPointerEvent(event))
        return false;
    qCDebug(lcPointerHandlerDispatch) << objectName()
        << "checking device type" << m_acceptedDevices
        << "pointer type" << m_acceptedPointerTypes
        << "modifiers" << m_acceptedModifiers;
    if ((event->device()->type() & m_acceptedDevices) == 0)
        return false;
    if ((event->device()->pointerType() & m_acceptedPointerTypes) == 0)
        return false;
    if (m_acceptedModifiers != Qt::KeyboardModifierMask && event->modifiers() != m_acceptedModifiers)
        return false;
    return true;
}

QT_END_NAMESPACE
