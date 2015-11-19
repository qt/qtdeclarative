/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include "qqmlinspectorclient.h"

#include <private/qpacket_p.h>
#include <private/qqmldebugconnection_p.h>
#include <QtCore/qdebug.h>

QQmlInspectorClient::QQmlInspectorClient(QQmlDebugConnection *connection) :
    QQmlDebugClient(QLatin1String("QmlInspector"), connection),
    m_lastRequestId(-1)
{
}

int QQmlInspectorClient::setInspectToolEnabled(bool enabled)
{
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++m_lastRequestId
       << QByteArray(enabled ? "enable" : "disable");

    sendMessage(ds.data());
    return m_lastRequestId;
}

int QQmlInspectorClient::setShowAppOnTop(bool showOnTop)
{
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++m_lastRequestId
       << QByteArray("showAppOnTop") << showOnTop;

    sendMessage(ds.data());
    return m_lastRequestId;
}

int QQmlInspectorClient::setAnimationSpeed(qreal speed)
{
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++m_lastRequestId
       << QByteArray("setAnimationSpeed") << speed;

    sendMessage(ds.data());
    return m_lastRequestId;
}

int QQmlInspectorClient::select(const QList<int> &objectIds)
{
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++m_lastRequestId
       << QByteArray("select") << objectIds;

    sendMessage(ds.data());
    return m_lastRequestId;
}

int QQmlInspectorClient::createObject(const QString &qml, int parentId, const QStringList &imports,
                                      const QString &filename)
{
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++m_lastRequestId
       << QByteArray("createObject") << qml << parentId << imports << filename;
    sendMessage(ds.data());
    return m_lastRequestId;
}

int QQmlInspectorClient::moveObject(int childId, int newParentId)
{
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++m_lastRequestId
       << QByteArray("moveObject") << childId << newParentId;
    sendMessage(ds.data());
    return m_lastRequestId;
}

int QQmlInspectorClient::destroyObject(int objectId)
{
    QPacket ds(connection()->currentDataStreamVersion());
    ds << QByteArray("request") << ++m_lastRequestId
       << QByteArray("destroyObject") << objectId;
    sendMessage(ds.data());
    return m_lastRequestId;
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
