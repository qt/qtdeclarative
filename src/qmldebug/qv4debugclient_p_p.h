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

#ifndef QV4DEBUGCLIENT_P_P_H
#define QV4DEBUGCLIENT_P_P_H

#include "qv4debugclient_p.h"
#include "qqmldebugclient_p_p.h"

#include <QtCore/qjsonobject.h>

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

class QV4DebugClientPrivate : public QQmlDebugClientPrivate
{
    Q_DECLARE_PUBLIC(QV4DebugClient)

public:
    QV4DebugClientPrivate(QQmlDebugConnection *connection);

    void sendMessage(const QByteArray &command, const QJsonObject &args = QJsonObject());
    void flushSendBuffer();
    QByteArray packMessage(const QByteArray &type, const QJsonObject &object);
    void onStateChanged(QQmlDebugClient::State state);

    int seq = 0;
    QList<QByteArray> sendBuffer;
    QByteArray response;
};

QT_END_NAMESPACE

#endif // QV4DEBUGCLIENT_P_P_H
