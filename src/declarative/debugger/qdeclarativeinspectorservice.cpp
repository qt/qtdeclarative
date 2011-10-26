/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

QT_BEGIN_NAMESPACE

Q_GLOBAL_STATIC(QDeclarativeInspectorService, serviceInstance)

QDeclarativeInspectorService::QDeclarativeInspectorService()
    : QDeclarativeDebugService(QLatin1String("QDeclarativeObserverMode"))
    , m_currentInspectorPlugin(0)
{
}

QDeclarativeInspectorService *QDeclarativeInspectorService::instance()
{
    return serviceInstance();
}

void QDeclarativeInspectorService::addView(QObject *view)
{
    m_views.append(view);
    updateStatus();
}

void QDeclarativeInspectorService::removeView(QObject *view)
{
    m_views.removeAll(view);
    updateStatus();
}

void QDeclarativeInspectorService::sendMessage(const QByteArray &message)
{
    if (status() != Enabled)
        return;

    QDeclarativeDebugService::sendMessage(message);
}

void QDeclarativeInspectorService::statusChanged(Status /*status*/)
{
    updateStatus();
}

void QDeclarativeInspectorService::updateStatus()
{
    if (m_views.isEmpty()) {
        if (m_currentInspectorPlugin) {
            m_currentInspectorPlugin->deactivate();
            m_currentInspectorPlugin = 0;
        }
        return;
    }

    if (status() == Enabled) {
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
        QPluginLoader loader(pluginPath);
        if (!loader.load())
            continue;

        QDeclarativeInspectorInterface *inspector =
                qobject_cast<QDeclarativeInspectorInterface*>(loader.instance());
        if (inspector)
            m_inspectorPlugins << inspector;
        else
            loader.unload();
    }
}

QT_END_NAMESPACE
