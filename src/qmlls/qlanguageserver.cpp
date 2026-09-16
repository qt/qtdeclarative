// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qlanguageserver_p_p.h"

#include <QtLanguageServer/private/qlspnotifysignals_p.h>
#include <QtJsonRpc/private/qjsonrpcprotocol_p_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lspServerLog, "qt.languageserver.server")

using namespace QLspSpecification;
using namespace Qt::StringLiterals;

QLanguageServerPrivate::QLanguageServerPrivate(const QJsonRpcTransport::DataHandler &h)
    : protocol(h)
{
}

/*!
\internal
\class QLanguageServer
\brief Implements a server for the language server protocol

QLanguageServer is a class that uses the QLanguageServerProtocol to
provide a server implementation.
It handles the lifecycle management, and can be extended via
QLanguageServerModule subclasses.

The language server keeps a strictly monotonically increasing runState that can be queried
from any thread (and is thus mutex gated), the normal run state is Initialized.

The language server also keeps track of the task canceled by the client (or implicitly when
shutting down, and isRequestCanceled can be called from any thread.
*/

QLanguageServer::QLanguageServer(const QJsonRpcTransport::DataHandler &h, QObject *parent)
    : QObject(*new QLanguageServerPrivate(h), parent)
{
    Q_D(QLanguageServer);
    registerMethods(*d->protocol.typedRpc());
    d->notifySignals.registerHandlers(&d->protocol);
    registerHandlers(&d->protocol);
}

QLanguageServerProtocol *QLanguageServer::protocol()
{
    Q_D(QLanguageServer);
    return &d->protocol;
}

QLanguageServer::RunStatus QLanguageServer::runStatus() const
{
    const Q_D(QLanguageServer);
    QMutexLocker l(&d->mutex);
    return d->runStatus;
}

void QLanguageServer::registerModule(QLanguageServerModule *serverModule)
{
    Q_D(QLanguageServer);
    Q_ASSERT(serverModule);
    serverModule->registerHandlers(this, &d->protocol);
    serverModule->setupCapabilities(d->serverInfo.capabilities);
}

QLspNotifySignals *QLanguageServer::notifySignals()
{
    Q_D(QLanguageServer);
    return &d->notifySignals;
}

void QLanguageServer::registerMethods(QJsonRpc::TypedRpc &typedRpc)
{
    typedRpc.installMessagePreprocessor(
            [this](const QJsonDocument &doc, const QJsonParseError &err,
                   const QJsonRpcProtocol::Handler<QJsonRpcProtocol::Response> &responder) {
                Q_D(QLanguageServer);
                if (!doc.isObject()) {
                    qCWarning(lspServerLog)
                            << "non object jsonrpc message" << doc << err.errorString();
                    return QJsonRpcProtocol::Processing::Stop;
                }
                bool sendErrorResponse = false;
                RunStatus rState = RunStatus::NotInitialized;
                QJsonValue id = doc.object()[u"id"];
                {
                    QMutexLocker l(&d->mutex);
                    // the normal case is d->runStatus == RunStatus::Initialized
                    if (d->runStatus != RunStatus::Initialized) {
                        if (d->runStatus == RunStatus::NotInitialized && !doc.isNull()
                            && doc.object()[u"method"].toString()
                                    == QString::fromUtf8(
                                            QLspSpecification::Requests::InitializeMethod)) {
                            return QJsonRpcProtocol::Processing::Continue;
                        } else if (!doc.isNull()
                                   && doc.object()[u"method"].toString()
                                           == QString::fromUtf8(
                                                   QLspSpecification::Notifications::ExitMethod)) {
                            return QJsonRpcProtocol::Processing::Continue;
                        }
                        if (id.isString() || id.isDouble()) {
                            sendErrorResponse = true;
                            rState = d->runStatus;
                        } else {
                            return QJsonRpcProtocol::Processing::Stop;
                        }
                    }
                }
                if (!sendErrorResponse) {
                    if (id.isString() || id.isDouble()) {
                        QMutexLocker l(&d->mutex);
                        d->requestsInProgress.insert(id, QRequestInProgress {});
                    }
                    return QJsonRpcProtocol::Processing::Continue;
                }
                if (rState == RunStatus::NotInitialized)
                    responder(QJsonRpcProtocol::MessageHandler::error(
                            int(QLspSpecification::ErrorCodes::ServerNotInitialized),
                            u"Request on non initialized Language Server (runStatus %1): %2"_s
                                    .arg(int(rState))
                                    .arg(QString::fromUtf8(doc.toJson()))));
                else
                    responder(QJsonRpcProtocol::MessageHandler::error(
                            int(QLspSpecification::ErrorCodes::InvalidRequest),
                            u"Method called on stopping Language Server (runStatus %1)"_s.arg(
                                    int(rState))));
                return QJsonRpcProtocol::Processing::Stop;
            });
    typedRpc.installOnCloseAction([this](QJsonRpc::TypedResponse::Status,
                                         const QJsonRpc::IdType &id, QJsonRpc::TypedRpc &) {
        Q_D(QLanguageServer);
        QJsonValue idValue = QTypedJson::toJsonValue(id);
        bool lastReq;
        {
            QMutexLocker l(&d->mutex);
            d->requestsInProgress.remove(idValue);
            lastReq = d->runStatus == RunStatus::WaitPending && d->requestsInProgress.size() <= 1;
            if (lastReq)
                d->runStatus = RunStatus::Stopping;
        }
        if (lastReq)
            executeShutdown();
    });
}

const QLspSpecification::InitializeParams &QLanguageServer::clientInfo() const
{
    const Q_D(QLanguageServer);

    if (int(runStatus()) < int(RunStatus::Initialized))
        qCWarning(lspServerLog) << "asked for Language Server clientInfo before initialization";
    return d->clientInfo;
}

void QLanguageServer::receiveData(const QByteArray &data, bool isEndOfMessage)
{
    if (!data.isEmpty())
        protocol()->receiveData(data);

    const Q_D(QLanguageServer);
    // read next message if not shutting down
    if (isEndOfMessage && d->runStatus != RunStatus::Stopped)
        emit readNextMessage();
}

void QLanguageServer::registerHandlers(QLanguageServerProtocol *protocol)
{
    protocol->registerInitializeRequestHandler(
            [this](const QByteArray &,
                   const QLspSpecification::Requests::InitializeParamsType &params,
                   QLspSpecification::Responses::InitializeResponseType &&response) {
                Q_D(QLanguageServer);
                {
                    QMutexLocker l(&d->mutex);
                    if (d->runStatus == RunStatus::Initialized) {
                        response.sendErrorResponse(
                                int(QLspSpecification::ErrorCodes::InvalidRequest),
                                u"Received multiple initialization requests"_s.toUtf8());
                    }
                }

                qCDebug(lspServerLog) << "init";
                d->clientInfo = params;
                {
                    QMutexLocker l(&d->mutex);
                    d->runStatus = RunStatus::Initialized;
                }
                response.sendResponse(d->serverInfo);
            });

    QObject::connect(notifySignals(), &QLspNotifySignals::receivedInitializedNotification, this,
                     [this](const QLspSpecification::Notifications::InitializedParamsType &) {
                         emit clientInitialized(this);
                     });

    protocol->registerShutdownRequestHandler(
            [this](const QByteArray &, const QLspSpecification::Requests::ShutdownParamsType &,
                   QLspSpecification::Responses::ShutdownResponseType &&response) {
                Q_D(QLanguageServer);
                RunStatus rStatus;
                bool shouldExecuteShutdown = false;
                {
                    QMutexLocker l(&d->mutex);
                    rStatus = d->runStatus;
                    if (rStatus == RunStatus::Initialized) {
                        d->shutdownResponse = std::move(response);
                        if (d->requestsInProgress.size() <= 1) {
                            d->runStatus = RunStatus::Stopping;
                            shouldExecuteShutdown = true;
                        } else {
                            d->runStatus = RunStatus::WaitPending;
                        }
                    }
                }
                if (rStatus != RunStatus::Initialized)
                    emit lifecycleError();
                else if (shouldExecuteShutdown)
                    executeShutdown();
            });

    QObject::connect(notifySignals(), &QLspNotifySignals::receivedExitNotification, this,
                     [this](const QLspSpecification::Notifications::ExitParamsType &) {
                         Q_D(QLanguageServer);
                         QMutexLocker l(&d->mutex);
                         RunStatus runStatus = d->runStatus;
                         if (runStatus != RunStatus::WaitingForExit) {
                             emit lifecycleError();
                             return;
                         }
                         d->runStatus = RunStatus::Stopped;
                         emit exit();
                     });
}

void QLanguageServer::executeShutdown()
{
    RunStatus rStatus = runStatus();
    if (rStatus != RunStatus::Stopping) {
        emit lifecycleError();
        return;
    }
    QLspSpecification::Responses::ShutdownResponseType shutdownResponse;
    {
        Q_D(QLanguageServer);
        QMutexLocker l(&d->mutex);
        rStatus = d->runStatus;
        if (rStatus == RunStatus::Stopping) {
            shutdownResponse = std::move(d->shutdownResponse);
            d->runStatus = RunStatus::WaitingForExit;
        }
    }
    if (rStatus != RunStatus::Stopping)
        emit lifecycleError();
    else
        shutdownResponse.sendResponse(nullptr);
}

QT_END_NAMESPACE
