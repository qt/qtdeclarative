// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
