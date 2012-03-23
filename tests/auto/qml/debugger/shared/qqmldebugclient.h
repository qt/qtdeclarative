/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QQMLDEBUGCLIENT_H
#define QQMLDEBUGCLIENT_H

#include <QtNetwork/qtcpsocket.h>

class QQmlDebugConnectionPrivate;
class QQmlDebugConnection : public QIODevice
{
    Q_OBJECT
    Q_DISABLE_COPY(QQmlDebugConnection)
public:
    QQmlDebugConnection(QObject * = 0);
    ~QQmlDebugConnection();

    void connectToHost(const QString &hostName, quint16 port);

    qint64 bytesAvailable() const;
    bool isConnected() const;
    QAbstractSocket::SocketState state() const;
    void flush();
    bool isSequential() const;
    void close();
    bool waitForConnected(int msecs = 30000);

signals:
    void connected();
    void stateChanged(QAbstractSocket::SocketState socketState);
    void error(QAbstractSocket::SocketError socketError);

protected:
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    QQmlDebugConnectionPrivate *d;
    friend class QQmlDebugClient;
    friend class QQmlDebugClientPrivate;
};

class QQmlDebugClientPrivate;
class QQmlDebugClient : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(QQmlDebugClient)

public:
    enum State { NotConnected, Unavailable, Enabled };

    QQmlDebugClient(const QString &, QQmlDebugConnection *parent);
    ~QQmlDebugClient();

    QString name() const;
    float serviceVersion() const;
    State state() const;

    virtual void sendMessage(const QByteArray &);

protected:
    virtual void stateChanged(State);
    virtual void messageReceived(const QByteArray &);

private:
    QQmlDebugClientPrivate *d;
    friend class QQmlDebugConnection;
    friend class QQmlDebugConnectionPrivate;
};

#endif // QQMLDEBUGCLIENT_H
