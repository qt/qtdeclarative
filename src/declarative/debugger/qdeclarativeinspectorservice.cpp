/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativeinspectorservice_p.h"
#include "qdeclarativeinspectorinterface_p.h"
#include "qdeclarativedebugserver_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QPluginLoader>

// print detailed information about loading of plugins
DEFINE_BOOL_CONFIG_OPTION(qmlDebugVerbose, QML_DEBUGGER_VERBOSE)

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QDeclarativeInspectorService, serviceInstance)

QDeclarativeInspectorService::QDeclarativeInspectorService()
    : QDeclarativeDebugService(QLatin1String("QDeclarativeObserverMode"), 1)
    , m_currentInspectorPlugin(0)
{
    registerService();
}

QDeclarativeInspectorService *QDeclarativeInspectorService::instance()
{
    return serviceInstance();
}

void QDeclarativeInspectorService::addView(QObject *view)
{
    m_views.append(view);
    updateState();
}

void QDeclarativeInspectorService::removeView(QObject *view)
{
    m_views.removeAll(view);
    updateState();
}

void QDeclarativeInspectorService::sendMessage(const QByteArray &message)
{
    if (state() != Enabled)
        return;

    QDeclarativeDebugService::sendMessage(message);
}

void QDeclarativeInspectorService::stateChanged(State /*state*/)
{
    QMetaObject::invokeMethod(this, "updateState", Qt::QueuedConnection);
}

void QDeclarativeInspectorService::updateState()
{
    if (m_views.isEmpty()) {
        if (m_currentInspectorPlugin) {
            m_currentInspectorPlugin->deactivate();
            m_currentInspectorPlugin = 0;
        }
        return;
    }

    if (state() == Enabled) {
        if (m_inspectorPlugins.isEmpty())
            loadInspectorPlugins();

        if (m_inspectorPlugins.isEmpty()) {
            qWarning() << "QDeclarativeInspector: No plugins found.";
            QDeclarativeDebugServer::instance()->removeService(this);
            return;
        }

        foreach (QDeclarativeInspectorInterface *inspector, m_inspectorPlugins) {
            if (inspector->canHandleView(m_views.first())) {
                m_currentInspectorPlugin = inspector;
                break;
            }
        }

        if (!m_currentInspectorPlugin) {
            qWarning() << "QDeclarativeInspector: No plugin available for view '" << m_views.first()->metaObject()->className() << "'.";
            return;
        }
        m_currentInspectorPlugin->activate(m_views.first());
    } else {
        if (m_currentInspectorPlugin) {
            m_currentInspectorPlugin->deactivate();
            m_currentInspectorPlugin = 0;
        }
    }
}

void QDeclarativeInspectorService::messageReceived(const QByteArray &message)
{
    QMetaObject::invokeMethod(this, "processMessage", Qt::QueuedConnection, Q_ARG(QByteArray, message));
}

void QDeclarativeInspectorService::processMessage(const QByteArray &message)
{
    if (m_currentInspectorPlugin)
        m_currentInspectorPlugin->clientMessage(message);
}

void QDeclarativeInspectorService::loadInspectorPlugins()
{
    QStringList pluginCandidates;
    const QStringList paths = QCoreApplication::libraryPaths();
    foreach (const QString &libPath, paths) {
        const QDir dir(libPath + QLatin1String("/qmltooling"));
        if (dir.exists())
            foreach (const QString &pluginPath, dir.entryList(QDir::Files))
                pluginCandidates << dir.absoluteFilePath(pluginPath);
    }

    foreach (const QString &pluginPath, pluginCandidates) {
        if (qmlDebugVerbose())
            qDebug() << "QDeclarativeInspector: Trying to load plugin " << pluginPath << "...";

        QPluginLoader loader(pluginPath);
        if (!loader.load()) {
            if (qmlDebugVerbose())
                qDebug() << "QDeclarativeInspector: Error while loading: " << loader.errorString();

            continue;
        }

        QDeclarativeInspectorInterface *inspector =
                qobject_cast<QDeclarativeInspectorInterface*>(loader.instance());
        if (inspector) {
            if (qmlDebugVerbose())
                qDebug() << "QDeclarativeInspector: Plugin successfully loaded.";
            m_inspectorPlugins << inspector;
        } else {
            if (qmlDebugVerbose())
                qDebug() << "QDeclarativeInspector: Plugin does not implement interface QDeclarativeInspectorInterface.";

            loader.unload();
        }
    }
}

QT_END_NAMESPACE
