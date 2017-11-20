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

#include "qquickhandlersmodule_p.h"
#include "qquickpointerhandler_p.h"
#include "qquickdraghandler_p.h"
#include "qquickpinchhandler_p.h"
#include "qquickpointhandler_p.h"
#include "qquicktaphandler_p.h"

static void initResources()
{
#ifdef QT_STATIC
    Q_INIT_RESOURCE(qmake_Qt_labs_handlers);
#endif
}

QT_BEGIN_NAMESPACE

static QQmlPrivate::AutoParentResult handler_autoParent(QObject *obj, QObject *parent)
{
    if (qmlobject_cast<QQuickItem *>(parent)) {
        QQuickPointerHandler *handler = qmlobject_cast<QQuickPointerHandler *>(obj);
        if (handler) {
            handler->setParent(parent);
            return QQmlPrivate::Parented;
        }
    }
    return QQmlPrivate::IncompatibleObject;
}

static void qt_quickhandlers_defineModule(const char *uri, int major, int minor)
{
    QQmlPrivate::RegisterAutoParent autoparent = { 0, &handler_autoParent };
    QQmlPrivate::qmlregister(QQmlPrivate::AutoParentRegistration, &autoparent);
    qmlRegisterUncreatableType<QQuickPointerEvent>(uri, major, minor, "PointerEvent",
        QQuickPointerHandler::tr("PointerEvent is only available as a parameter of several signals in PointerHandler"));
    qmlRegisterUncreatableType<QQuickEventPoint>(uri, major, minor, "EventPoint",
        QQuickPointerHandler::tr("EventPoint is only available as a member of PointerEvent"));
    qmlRegisterUncreatableType<QQuickEventTouchPoint>(uri, major, minor, "EventTouchPoint",
        QQuickPointerHandler::tr("EventTouchPoint is only available as a member of PointerEvent"));
    qmlRegisterUncreatableType<QQuickPointerDevice>(uri, major, minor, "PointerDevice",
        QQuickPointerHandler::tr("PointerDevice is only available as a property of PointerEvent"));
    qRegisterMetaType<QPointingDeviceUniqueId>("QPointingDeviceUniqueId");
    qmlRegisterUncreatableType<QPointingDeviceUniqueId>(uri, major, minor, "PointingDeviceUniqueId",
        QQuickPointerHandler::tr("PointingDeviceUniqueId is only available as a property of PointerEvent"));

    qmlRegisterUncreatableType<QQuickPointerHandler>(uri,major,minor,"PointerHandler",
        QQuickPointerHandler::tr("PointerHandler is an abstract base class"));
    qmlRegisterType<QQuickPointHandler>(uri,major,minor,"PointHandler");
    qmlRegisterType<QQuickDragHandler>(uri,major,minor,"DragHandler");
    qmlRegisterUncreatableType<QQuickDragAxis>(uri, major, minor, "DragAxis",
        QQuickDragHandler::tr("DragAxis is only available as a grouped property of DragHandler"));
    qmlRegisterType<QQuickPinchHandler>(uri,major,minor,"PinchHandler");
    qmlRegisterType<QQuickTapHandler>(uri,major,minor,"TapHandler");
    qRegisterMetaType<QQuickHandlerPoint>();
}

void QQuickHandlersModule::defineModule()
{
    initResources();

    const char uri[] = "Qt.labs.handlers";
    int majorVersion = 1;
    int minorVersion = 0;

    qt_quickhandlers_defineModule(uri, majorVersion, minorVersion);
}

QT_END_NAMESPACE
