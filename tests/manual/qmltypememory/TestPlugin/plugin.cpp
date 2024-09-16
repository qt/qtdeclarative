// Copyright (C) 2013 Research In Motion.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtQml/QQmlExtensionPlugin>
#include <QtQml/qqml.h>
#include <qdebug.h>

class TestType : public QObject
{
    Q_OBJECT
};

class TestPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override
    {
        Q_ASSERT(uri == QLatin1String("TestPlugin"));
        qmlRegisterType<TestType>(uri, 1, 0, "TestTypePlugin");
    }
};

#include "plugin.moc"
