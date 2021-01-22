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

#ifndef QPACKETPROTOCOL_P_H
#define QPACKETPROTOCOL_P_H

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

class QIODevice;

class QPacketProtocolPrivate;
class QPacketProtocol : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QPacketProtocol)
public:
    explicit QPacketProtocol(QIODevice *dev, QObject *parent = nullptr);

    void send(const QByteArray &data);
    qint64 packetsAvailable() const;
    QByteArray read();
    bool waitForReadyRead(int msecs = 3000);

Q_SIGNALS:
    void readyRead();
    void error();

private:
    void bytesWritten(qint64 bytes);
    void readyToRead();
};

QT_END_NAMESPACE

#endif // QPACKETPROTOCOL_P_H
