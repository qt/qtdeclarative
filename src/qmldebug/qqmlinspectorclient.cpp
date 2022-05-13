// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

#include "moc_qqmlinspectorclient_p.cpp"
