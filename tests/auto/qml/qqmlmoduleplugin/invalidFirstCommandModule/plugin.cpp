// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QStringList>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QDebug>

class MyPluginType : public QObject
{
    Q_OBJECT
public:
    MyPluginType(QObject *parent=nullptr) : QObject(parent) {}
};


class MyPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    MyPlugin() {}

    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == "org.qtproject.InvalidFirstCommandModule");
        qmlRegisterType<MyPluginType>("org.qtproject.InvalidFirstCommandModule", 1, 0, "MyPluginType");
    }
};

#include "plugin.moc"
