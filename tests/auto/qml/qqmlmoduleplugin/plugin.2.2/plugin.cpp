// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QStringList>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QDebug>

class MyPluginType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue)
    Q_PROPERTY(int valueOnlyIn2 READ value WRITE setValue)

public:
    MyPluginType(QObject *parent=nullptr) : QObject(parent)
    {
        qWarning("import2.2 worked");
    }

    int value() const { return v; }
    void setValue(int i) { v = i; }

private:
    int v;
};


class MyPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    MyPlugin()
    {
        qWarning("plugin2.2 created");
    }

    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == "org.qtproject.AutoTestQmlPluginType");
        qmlRegisterType<MyPluginType>(uri, 2, 0, "MyPluginType");
        qmlRegisterModule(uri, 2, 2);
    }
};

#include "plugin.moc"
