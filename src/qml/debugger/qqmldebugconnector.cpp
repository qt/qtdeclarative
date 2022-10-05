// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldebugpluginmanager_p.h"
#include "qqmldebugconnector_p.h"
#include "qqmldebugservicefactory_p.h"
#include <QtCore/QPluginLoader>
#include <QtCore/QCborArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QDebug>
#include <QtCore/QDataStream>

#include <private/qcoreapplication_p.h>
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

// Connectors. We could add more plugins here, and distinguish by arguments to instance()
Q_QML_DEBUG_PLUGIN_LOADER(QQmlDebugConnector)

// Services
Q_QML_DEBUG_PLUGIN_LOADER(QQmlDebugService)

int QQmlDebugConnector::s_dataStreamVersion = QDataStream::Qt_4_7;

struct QQmlDebugConnectorParams {
    QString pluginKey;
    QStringList services;
    QString arguments;
    QQmlDebugConnector *instance;

    QQmlDebugConnectorParams() : instance(nullptr)
    {
        if (qApp) {
            QCoreApplicationPrivate *appD =
                    static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(qApp));
            if (appD)
                arguments = appD->qmljsDebugArgumentsString();
        }
    }
};

Q_GLOBAL_STATIC(QQmlDebugConnectorParams, qmlDebugConnectorParams)

void QQmlDebugConnector::setPluginKey(const QString &key)
{
    QQmlDebugConnectorParams *params = qmlDebugConnectorParams();
    if (params && params->pluginKey != key) {
        if (params->instance)
            qWarning() << "QML debugger: Cannot set plugin key after loading the plugin.";
        else
            params->pluginKey = key;
    }
}

void QQmlDebugConnector::setServices(const QStringList &services)
{
    QQmlDebugConnectorParams *params = qmlDebugConnectorParams();
    if (params)
        params->services = services;
}

QString QQmlDebugConnector::commandLineArguments()
{
    QQmlDebugConnectorParams *params = qmlDebugConnectorParams();
    if (!params)
        return QString();
    return params->arguments;
}

QQmlDebugConnector *QQmlDebugConnector::instance()
{
    QQmlDebugConnectorParams *params = qmlDebugConnectorParams();
    if (!params)
        return nullptr;

    if (!QQmlEnginePrivate::qml_debugging_enabled.load(std::memory_order_relaxed)) {
        if (!params->arguments.isEmpty()) {
            qWarning().noquote() << QString::fromLatin1(
                                        "QML Debugger: Ignoring \"-qmljsdebugger=%1\". Debugging "
                                        "has not been enabled.").arg(params->arguments);
            params->arguments.clear();
        }
        return nullptr;
    }

    if (!params->instance) {
        if (!params->pluginKey.isEmpty()) {
            params->instance = loadQQmlDebugConnector(params->pluginKey);
        } else if (params->arguments.isEmpty()) {
            return nullptr; // no explicit class name given and no command line arguments
        } else if (params->arguments.startsWith(QLatin1String("connector:"))) {
            static const int connectorBegin = int(strlen("connector:"));

            int connectorEnd = params->arguments.indexOf(QLatin1Char(','), connectorBegin);
            if (connectorEnd == -1)
                connectorEnd = params->arguments.size();

            params->instance = loadQQmlDebugConnector(params->arguments.mid(
                                                          connectorBegin,
                                                          connectorEnd - connectorBegin));
        } else {
            params->instance = loadQQmlDebugConnector(
                        params->arguments.startsWith(QLatin1String("native")) ?
                            QStringLiteral("QQmlNativeDebugConnector") :
                            QStringLiteral("QQmlDebugServer"));
        }

        if (params->instance) {
            const auto metaData = metaDataForQQmlDebugService();
            for (const QPluginParsedMetaData &md : metaData) {
                const auto keys = md.value(QtPluginMetaDataKeys::MetaData).toMap()
                        .value(QLatin1String("Keys")).toArray();
                for (const QCborValue key : keys) {
                    QString keyString = key.toString();
                    if (params->services.isEmpty() || params->services.contains(keyString))
                        loadQQmlDebugService(keyString);
                }
            }
        }
    }

    return params->instance;
}

QQmlDebugConnectorFactory::~QQmlDebugConnectorFactory()
{
    // This is triggered when the plugin is unloaded.
    QQmlDebugConnectorParams *params = qmlDebugConnectorParams();
    if (params) {
        params->pluginKey.clear();
        params->arguments.clear();
        params->services.clear();
        delete params->instance;
        params->instance = nullptr;
    }
}

QT_END_NAMESPACE

#include "moc_qqmldebugconnector_p.cpp"
