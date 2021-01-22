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

#include <QtQmlWorkerScript/private/qtqmlworkerscriptglobal_p.h>
#include <QtQml/qqmlextensionplugin.h>

QT_BEGIN_NAMESPACE

/*!
    \qmlmodule QtQml.WorkerScript 2.\QtMinorVersion
    \title Qt QML WorkerScript QML Types
    \ingroup qmlmodules
    \brief Provides QML types for worker scripts
    \since 5.14

    This QML module contains types for using worker scripts.

    To use the types in this module, import the module with the following line:

    \qml \QtMinorVersion
    import QtQml.WorkerScript 2.\1
    \endqml
*/

class QtQmlWorkerScriptPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)
public:
    QtQmlWorkerScriptPlugin(QObject *parent = nullptr) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtQml_WorkerScript;
        Q_UNUSED(registration);
    }
};

QT_END_NAMESPACE

#include "plugin.moc"
