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

#ifndef QQMLDEBUGCONNECTION_P_H
#define QQMLDEBUGCONNECTION_P_H

#include <QtCore/qobject.h>

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
    QQmlDebugConnection(QObject *parent = 0);
    ~QQmlDebugConnection();

    void connectToHost(const QString &hostName, quint16 port);
    void startLocalServer(const QString &fileName);

    void setDataStreamVersion(int dataStreamVersion);
    int dataStreamVersion();

    bool isConnected() const;
    void close();
    bool waitForConnected(int msecs = 30000);

    QQmlDebugClient *client(const QString &name) const;
    bool addClient(const QString &name, QQmlDebugClient *client);
    bool removeClient(const QString &name);

    float serviceVersion(const QString &serviceName) const;
    bool sendMessage(const QString &name, const QByteArray &message);

signals:
    void connected();

private Q_SLOTS:
    void newConnection();
    void socketConnected();
    void socketDisconnected();
    void protocolReadyRead();
    void handshakeTimeout();
};

QT_END_NAMESPACE

#endif // QQMLDEBUGCONNECTION_P_H
