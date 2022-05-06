/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include <QtQml/qqmlextensionplugin.h>
#include <private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick);
Q_GHS_KEEP_REFERENCE(QQuick_initializeModule);

class QtQuick2Plugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)
public:
    QtQuick2Plugin(QObject *parent = nullptr) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtQuick;
        volatile auto initialization = &QQuick_initializeModule;
        Q_UNUSED(registration);
        Q_UNUSED(initialization);
    }
};

QT_END_NAMESPACE

#include "qtquickplugin.moc"
