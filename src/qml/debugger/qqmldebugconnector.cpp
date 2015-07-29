/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmldebugpluginmanager_p.h"
#include "qqmldebugconnector_p.h"
#include "qdebugmessageservice_p.h"
#include "qqmlenginecontrolservice_p.h"
#include "qqmlenginedebugservice_p.h"
#include "qqmlinspectorservice_p.h"
#include "qqmlprofilerservice_p.h"
#include "qv4debugservice_p.h"
#include <QtCore/QPluginLoader>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QDebug>

#include <private/qcoreapplication_p.h>
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

extern QQmlDebugConnector *loadQQmlDebugConnector(const QString &key);

struct QQmlDebugConnectorParams {
    QString pluginKey;
    QString arguments;
    QQmlDebugConnector *instance;

    QQmlDebugConnectorParams() : instance(0)
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
    if (params) {
        if (params->instance)
            qWarning() << "QML debugger: Cannot set plugin key after loading the plugin.";
        else
            params->pluginKey = key;
    }
}

QQmlDebugConnector *QQmlDebugConnector::instance()
{
    QQmlDebugConnectorParams *params = qmlDebugConnectorParams();
    if (!params)
        return 0;

    if (!QQmlEnginePrivate::qml_debugging_enabled) {
        if (!params->arguments.isEmpty()) {
            qWarning() << QString::fromLatin1(
                              "QML Debugger: Ignoring \"-qmljsdebugger=%1\". Debugging has not "
                              "been enabled.").arg(params->arguments);
            params->arguments.clear();
        }
        return 0;
    }

    if (!params->instance) {
        if (!params->pluginKey.isEmpty()) {
            if (params->pluginKey != QLatin1String("QQmlDebugServer"))
                return 0; // We cannot load anything else, yet
        } else if (params->arguments.isEmpty()) {
            return 0; // no explicit class name given and no command line arguments
        }
        params->instance = loadQQmlDebugConnector(QLatin1String("QQmlDebugServer"));
        if (params->instance) {
            QQmlEngineDebugService::instance();
            QV4DebugService::instance();
            QQmlProfilerService::instance();
            QDebugMessageService::instance();
            QQmlEngineControlService::instance();
            QQmlInspectorService::instance();
        }
    }

    return params->instance;
}

QT_END_NAMESPACE
