/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#ifndef QQMLDEBUGCONNECTION_P_H
#define QQMLDEBUGCONNECTION_P_H

#include <QtCore/qobject.h>
#include <QtNetwork/qabstractsocket.h>

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

QT_BEGIN_NAMESPACE

class QQmlDebugClient;
class QQmlDebugConnectionPrivate;
class QQmlDebugConnection : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QQmlDebugConnection)
    Q_DECLARE_PRIVATE(QQmlDebugConnection)
public:
    QQmlDebugConnection(QObject *parent = nullptr);
    ~QQmlDebugConnection();

    void connectToHost(const QString &hostName, quint16 port);
    void startLocalServer(const QString &fileName);

    int currentDataStreamVersion() const;
    void setMaximumDataStreamVersion(int maximumVersion);

    bool isConnected() const;
    bool isConnecting() const;

    void close();
    bool waitForConnected(int msecs = 30000);

    QQmlDebugClient *client(const QString &name) const;
    bool addClient(const QString &name, QQmlDebugClient *client);
    bool removeClient(const QString &name);

    float serviceVersion(const QString &serviceName) const;
    bool sendMessage(const QString &name, const QByteArray &message);

signals:
    void connected();
    void disconnected();
    void socketError(QAbstractSocket::SocketError socketError);
    void socketStateChanged(QAbstractSocket::SocketState socketState);

private:
    void newConnection();
    void socketConnected();
    void socketDisconnected();
    void protocolReadyRead();
    void handshakeTimeout();
};

QT_END_NAMESPACE

#endif // QQMLDEBUGCONNECTION_P_H
