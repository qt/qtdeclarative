// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqmlengine.h>

QT_BEGIN_NAMESPACE

class ModulePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    ModulePlugin(QObject *parent = nullptr) : QQmlExtensionPlugin(parent) {}
    void registerTypes(const char *uri) override;
};

void ModulePlugin::registerTypes(const char *uri)
{
    qmlRegisterModule(uri, 1, 0);
    qmlRegisterType(QUrl("qrc:/ModuleType.qml"), uri, 1, 0, "ModuleType");
}

QT_END_NAMESPACE

#include "moduleplugin.moc"
