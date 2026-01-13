// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MYDYNAMICMODULEPLUGIN_H
#define MYDYNAMICMODULEPLUGIN_H

#include <QtQml/qqmlextensionplugin.h>
#include <QtCore/qdebug.h>
#include <QtQml/qqml.h>

class MyCppComponent2 : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int myP READ myP WRITE setMyP NOTIFY myPChanged)

public:
    MyCppComponent2() : QObject(nullptr) { }

    int myP() const { return m_myP; }
    void setMyP(int newP)
    {
        m_myP = newP;
        emit myPChanged();
    }

signals:
    void myPChanged();

private:
    int m_myP = 0;
};

class MyDynamicModulePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    MyDynamicModulePlugin(QObject *parent = nullptr) : QQmlExtensionPlugin(parent) { }

    void registerTypes(const char *uri) override
    {
        qmlRegisterType<MyCppComponent2>(uri, 1, 0, "MyCppComponent2");
        qmlRegisterModule(uri, 1, 0);
        qWarning() << "Registered dynamic plugin!";
    }
    void unregisterTypes() override { qWarning() << "Unregistered dynamic plugin!"; }
};

#endif // MYDYNAMICMODULEPLUGIN_H
