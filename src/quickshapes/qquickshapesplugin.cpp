// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QtQuickShapes/private/qquickshape_p.h>

QT_BEGIN_NAMESPACE

Q_GHS_KEEP_REFERENCE(qml_register_types_QtQuick_Shapes);
Q_GHS_KEEP_REFERENCE(QQuickShapes_initializeModule);

class QmlShapesPlugin : public QQmlEngineExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlEngineExtensionInterface_iid)

public:
    QmlShapesPlugin(QObject *parent = nullptr) : QQmlEngineExtensionPlugin(parent)
    {
        volatile auto registration = &qml_register_types_QtQuick_Shapes;
        volatile auto initialize = &QQuickShapes_initializeModule;
        Q_UNUSED(registration);
        Q_UNUSED(initialize);
    }
};

QT_END_NAMESPACE

#include "qquickshapesplugin.moc"
