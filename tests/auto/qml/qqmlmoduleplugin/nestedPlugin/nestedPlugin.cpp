// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QStringList>
#include <QtQml/qqmlextensionplugin.h>
#include <QtQml/qqml.h>
#include <QDebug>

class MyPluginTypeNested : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString value READ value)

public:
    QString value() const { return "Hello"; }
};

class MyNestedPluginType : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString value READ value)

public:
    QString value() const { return "Goodbye"; }
};

class MyPluginNested : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override
    {
        Q_ASSERT(QLatin1String(uri) == "org.qtproject.AutoTestQmlNestedPluginType");
        qmlRegisterType<MyPluginTypeNested>(uri, 1, 0, "MyPluginType");
        qmlRegisterType<MyPluginTypeNested>(uri, 1, 0, "Conflict");

        QString nestedUri(uri);
        nestedUri += QLatin1String(".Nested");

        qmlRegisterType<MyNestedPluginType>(nestedUri.toLatin1().constData(), 1, 0, "MyNestedPluginType");
        qmlRegisterType<MyNestedPluginType>(nestedUri.toLatin1().constData(), 1, 0, "Conflict");
    }
};

#include "nestedPlugin.moc"
