/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlinspectorclient_p_p.h"

#include <private/qpacket_p.h>
#include <private/qqmldebugconnection_p.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QQmlInspectorClient::QQmlInspectorClient(QQmlDebugConnection *connection) :
    QQmlDebugClient(*new QQmlInspectorClientPrivate(connection))
{
}

QQmlInspectorClientPrivate::QQmlInspectorClientPrivate(QQmlDebugConnection *connection) :
    QQmlDebugClientPrivate(QLatin1String("QmlInspector"), connection)
{
}

int QQmlInspectorClient::setInspectToolEnabled(bool enabled)
{
    Q_D(QQmlInspectorClient);
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++(d->m_lastRequestId)
       << QByteArray(enabled ? "enable" : "disable");

    sendMessage(ds.data());
    return d->m_lastRequestId;
}

int QQmlInspectorClient::setShowAppOnTop(bool showOnTop)
{
    Q_D(QQmlInspectorClient);
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++(d->m_lastRequestId)
       << QByteArray("showAppOnTop") << showOnTop;

    sendMessage(ds.data());
    return d->m_lastRequestId;
}

int QQmlInspectorClient::setAnimationSpeed(qreal speed)
{
    Q_D(QQmlInspectorClient);
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++(d->m_lastRequestId)
       << QByteArray("setAnimationSpeed") << speed;

    sendMessage(ds.data());
    return d->m_lastRequestId;
}

int QQmlInspectorClient::select(const QList<int> &objectIds)
{
    Q_D(QQmlInspectorClient);
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++(d->m_lastRequestId)
       << QByteArray("select") << objectIds;

    sendMessage(ds.data());
    return d->m_lastRequestId;
}

int QQmlInspectorClient::createObject(const QString &qml, int parentId, const QStringList &imports,
                                      const QString &filename)
{
    Q_D(QQmlInspectorClient);
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++(d->m_lastRequestId)
       << QByteArray("createObject") << qml << parentId << imports << filename;
    sendMessage(ds.data());
    return d->m_lastRequestId;
}

int QQmlInspectorClient::moveObject(int childId, int newParentId)
{
    Q_D(QQmlInspectorClient);
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++(d->m_lastRequestId)
       << QByteArray("moveObject") << childId << newParentId;
    sendMessage(ds.data());
    return d->m_lastRequestId;
}

int QQmlInspectorClient::destroyObject(int objectId)
{
    Q_D(QQmlInspectorClient);
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++(d->m_lastRequestId)
       << QByteArray("destroyObject") << objectId;
    sendMessage(ds.data());
    return d->m_lastRequestId;
}

void QQmlInspectorClient::messageReceived(const QByteArray &message)
{
    QPacket ds(connection()->currentDataStreamVersion(), message);
    QByteArray type;
    ds >> type;

    if (type != QByteArray("response")) {
        qDebug() << "Unhandled message of type" << type;
        return;
    }

    int responseId;
    bool result;
    ds >> responseId >> result;
    emit responseReceived(responseId, result);
}

QT_END_NAMESPACE
