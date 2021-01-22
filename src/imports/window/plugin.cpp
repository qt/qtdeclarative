/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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
**
**
**
****************************************************************************/

#include <QtQml/qqmlextensionplugin.h>

#include "plugin.h"

extern void qml_register_types_QtQuick_Window();
GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Window);

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule QtQuick.Window 2.\QtMinorVersion
    \title Qt Quick Window QML Types
    \ingroup qmlmodules
    \brief Provides QML types for window management

    This QML module contains types for creating top-level windows and accessing screen information.

    To use the types in this module, import the module with the following line:

    \qml \QtMinorVersion
    import QtQuick.Window 2.\1
    \endqml
*/

//![class decl]
class QtQuick2WindowPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
public:
    QtQuick2WindowPlugin(QObject *parent = nullptr) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtQuick_Window;
        Q_UNUSED(registration);
    }
};
//![class decl]

QT_END_NAMESPACE

#include "plugin.moc"
