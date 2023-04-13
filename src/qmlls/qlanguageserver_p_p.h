// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QLANGUAGESERVER_P_P_H
#define QLANGUAGESERVER_P_P_H

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

#include "qlanguageserver_p.h"
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>
#include <QtCore/QMutex>
#include <QtCore/QHash>
#include <QtCore/private/qobject_p.h>
#include <QtLanguageServer/private/qlspnotifysignals_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

class QRequestInProgress
{
public:
    bool canceled = false;
};

class QLanguageServerPrivate : public QObjectPrivate
{
public:
    QLanguageServerPrivate(const QJsonRpcTransport::DataHandler &h);
    mutable QMutex mutex;
    // mutex gated, monotonically increasing
    QLanguageServer::RunStatus runStatus = QLanguageServer::RunStatus::NotSetup;
    QHash<QJsonValue, QRequestInProgress> requestsInProgress; // mutex gated
    bool clientInitialized = false; // mutex gated
    QLspSpecification::InitializeParams clientInfo; // immutable after runStatus > DidInitialize
    QLspSpecification::InitializeResult serverInfo; // immutable after runStatus > DidInitialize
    QLspSpecification::Responses::ShutdownResponseType shutdownResponse;
    QHash<QString, QLanguageServerModule *> modules;
    QLanguageServerProtocol protocol;
    QLspNotifySignals notifySignals;
};

QT_END_NAMESPACE
#endif // QLANGUAGESERVER_P_P_H
