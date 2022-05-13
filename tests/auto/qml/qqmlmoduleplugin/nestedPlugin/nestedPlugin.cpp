// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include <QStringList>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QDebug>

class MyPluginType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString value READ value)

public:
    MyPluginType(QObject *parent=nullptr) : QObject(parent) {}

    QString value() const { return "Hello"; }
};

class MyNestedPluginType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString value READ value)

public:
    MyNestedPluginType(QObject *parent=nullptr) : QObject(parent) {}

    QString value() const { return "Goodbye"; }
};


class MyPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    MyPlugin() {}

    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == "org.qtproject.AutoTestQmlNestedPluginType");
        qmlRegisterType<MyPluginType>(uri, 1, 0, "MyPluginType");

        QString nestedUri(uri);
        nestedUri += QLatin1String(".Nested");

        qmlRegisterType<MyNestedPluginType>(nestedUri.toLatin1().constData(), 1, 0, "MyNestedPluginType");
    }
};

#include "nestedPlugin.moc"
