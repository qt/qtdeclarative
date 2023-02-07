// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qlanguageserver_p_p.h"

#include <QtLanguageServer/private/qlspnotifysignals_p.h>
#include <QtJsonRpc/private/qjsonrpcprotocol_p_p.h>

QT_BEGIN_NAMESPACE

using namespace QLspSpecification;
using namespace Qt::StringLiterals;

Q_LOGGING_CATEGORY(lspServerLog, "qt.languageserver.server")

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
from any thread (and is thus mutex gated), the normal run state is DidInitialize.

The language server also keeps track of the task canceled by the client (or implicitly when
shutting down, and isRequestCanceled can be called from any thread.
*/

QLanguageServer::QLanguageServer(const QJsonRpcTransport::DataHandler &h, QObject *parent)
    : QObject(*new QLanguageServerPrivate(h), parent)
{
    Q_D(QLanguageServer);
    registerMethods(*d->protocol.typedRpc());
    d->notifySignals.registerHandlers(&d->protocol);
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

void QLanguageServer::finishSetup()
{
    Q_D(QLanguageServer);
    RunStatus rStatus;
    {
        QMutexLocker l(&d->mutex);
        rStatus = d->runStatus;
        if (rStatus == RunStatus::NotSetup)
            d->runStatus = RunStatus::SettingUp;
    }
    if (rStatus != RunStatus::NotSetup) {
        emit lifecycleError();
        return;
    }
    emit runStatusChanged(RunStatus::SettingUp);

    registerHandlers(&d->protocol);
    for (auto module : d->modules)
        module->registerHandlers(this, &d->protocol);

    {
        QMutexLocker l(&d->mutex);
        rStatus = d->runStatus;
        if (rStatus == RunStatus::SettingUp)
            d->runStatus = RunStatus::DidSetup;
    }
    if (rStatus != RunStatus::SettingUp) {
        emit lifecycleError();
        return;
    }
    emit runStatusChanged(RunStatus::DidSetup);
}

void QLanguageServer::addServerModule(QLanguageServerModule *serverModule)
{
    Q_D(QLanguageServer);
    Q_ASSERT(serverModule);
    RunStatus rStatus;
    {
        QMutexLocker l(&d->mutex);
        rStatus = d->runStatus;
        if (rStatus == RunStatus::NotSetup) {
            if (d->modules.contains(serverModule->name())) {
                d->modules.insert(serverModule->name(), serverModule);
                qCWarning(lspServerLog) << "Duplicate add of QLanguageServerModule named"
                                        << serverModule->name() << ", overwriting.";
            } else {
                d->modules.insert(serverModule->name(), serverModule);
            }
        }
    }
    if (rStatus != RunStatus::NotSetup) {
        qCWarning(lspServerLog) << "Called QLanguageServer::addServerModule after setup";
        emit lifecycleError();
        return;
    }
}

QLanguageServerModule *QLanguageServer::moduleByName(const QString &n) const
{
    const Q_D(QLanguageServer);
    QMutexLocker l(&d->mutex);
    return d->modules.value(n);
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
                RunStatus rState;
                QJsonValue id = doc.object()[u"id"];
                {
                    QMutexLocker l(&d->mutex);
                    // the normal case is d->runStatus == RunStatus::DidInitialize
                    if (d->runStatus != RunStatus::DidInitialize) {
                        if (d->runStatus == RunStatus::DidSetup && !doc.isNull()
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
                if (rState == RunStatus::NotSetup || rState == RunStatus::DidSetup)
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

void QLanguageServer::setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                                        QLspSpecification::InitializeResult &serverInfo)
{
    Q_D(QLanguageServer);
    for (auto module : std::as_const(d->modules))
        module->setupCapabilities(clientInfo, serverInfo);
}

const QLspSpecification::InitializeParams &QLanguageServer::clientInfo() const
{
    const Q_D(QLanguageServer);

    if (int(runStatus()) < int(RunStatus::DidInitialize))
        qCWarning(lspServerLog) << "asked for Language Server clientInfo before initialization";
    return d->clientInfo;
}

const QLspSpecification::InitializeResult &QLanguageServer::serverInfo() const
{
    const Q_D(QLanguageServer);
    if (int(runStatus()) < int(RunStatus::DidInitialize))
        qCWarning(lspServerLog) << "asked for Language Server serverInfo before initialization";
    return d->serverInfo;
}

void QLanguageServer::receiveData(const QByteArray &d)
{
    protocol()->receiveData(d);
}

void QLanguageServer::registerHandlers(QLanguageServerProtocol *protocol)
{
    QObject::connect(notifySignals(), &QLspNotifySignals::receivedCancelNotification, this,
                     [this](const QLspSpecification::Notifications::CancelParamsType &params) {
                         Q_D(QLanguageServer);
                         QJsonValue id = QTypedJson::toJsonValue(params.id);
                         QMutexLocker l(&d->mutex);
                         if (d->requestsInProgress.contains(id))
                             d->requestsInProgress[id].canceled = true;
                         else
                             qCWarning(lspServerLog)
                                     << "Ignoring cancellation of non in progress request" << id;
                     });

    protocol->registerInitializeRequestHandler(
            [this](const QByteArray &,
                   const QLspSpecification::Requests::InitializeParamsType &params,
                   QLspSpecification::Responses::InitializeResponseType &&response) {
                qCDebug(lspServerLog) << "init";
                Q_D(QLanguageServer);
                RunStatus rStatus;
                {
                    QMutexLocker l(&d->mutex);
                    rStatus = d->runStatus;
                    if (rStatus == RunStatus::DidSetup)
                        d->runStatus = RunStatus::Initializing;
                }
                if (rStatus != RunStatus::DidSetup) {
                    if (rStatus == RunStatus::NotSetup || rStatus == RunStatus::SettingUp)
                        response.sendErrorResponse(
                                int(QLspSpecification::ErrorCodes::InvalidRequest),
                                u"Initialization request received on non setup language server"_s
                                        .toUtf8());
                    else
                        response.sendErrorResponse(
                                int(QLspSpecification::ErrorCodes::InvalidRequest),
                                u"Received multiple initialization requests"_s.toUtf8());
                    emit lifecycleError();
                    return;
                }
                emit runStatusChanged(RunStatus::Initializing);
                d->clientInfo = params;
                setupCapabilities(d->clientInfo, d->serverInfo);
                {
                    QMutexLocker l(&d->mutex);
                    d->runStatus = RunStatus::DidInitialize;
                }
                emit runStatusChanged(RunStatus::DidInitialize);
                response.sendResponse(d->serverInfo);
            });

    QObject::connect(notifySignals(), &QLspNotifySignals::receivedInitializedNotification, this,
                     [this](const QLspSpecification::Notifications::InitializedParamsType &) {
                         Q_D(QLanguageServer);
                         {
                             QMutexLocker l(&d->mutex);
                             d->clientInitialized = true;
                         }
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
                    if (rStatus == RunStatus::DidInitialize) {
                        d->shutdownResponse = std::move(response);
                        if (d->requestsInProgress.size() <= 1) {
                            d->runStatus = RunStatus::Stopping;
                            shouldExecuteShutdown = true;
                        } else {
                            d->runStatus = RunStatus::WaitPending;
                        }
                    }
                }
                if (rStatus != RunStatus::DidInitialize)
                    emit lifecycleError();
                else if (shouldExecuteShutdown)
                    executeShutdown();
            });

    QObject::connect(notifySignals(), &QLspNotifySignals::receivedExitNotification, this,
                     [this](const QLspSpecification::Notifications::ExitParamsType &) {
                         if (runStatus() != RunStatus::Stopped)
                             emit lifecycleError();
                         else
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
    emit shutdown();
    QLspSpecification::Responses::ShutdownResponseType shutdownResponse;
    {
        Q_D(QLanguageServer);
        QMutexLocker l(&d->mutex);
        rStatus = d->runStatus;
        if (rStatus == RunStatus::Stopping) {
            shutdownResponse = std::move(d->shutdownResponse);
            d->runStatus = RunStatus::Stopped;
        }
    }
    if (rStatus != RunStatus::Stopping)
        emit lifecycleError();
    else
        shutdownResponse.sendResponse(nullptr);
}

bool QLanguageServer::isRequestCanceled(const QJsonRpc::IdType &id) const
{
    const Q_D(QLanguageServer);
    QJsonValue idVal = QTypedJson::toJsonValue(id);
    QMutexLocker l(&d->mutex);
    return d->requestsInProgress.value(idVal).canceled || d->runStatus != RunStatus::DidInitialize;
}

bool QLanguageServer::isInitialized() const
{
    switch (runStatus()) {
    case RunStatus::NotSetup:
    case RunStatus::SettingUp:
    case RunStatus::DidSetup:
    case RunStatus::Initializing:
        return false;
    case RunStatus::DidInitialize:
    case RunStatus::WaitPending:
    case RunStatus::Stopping:
    case RunStatus::Stopped:
        break;
    }
    return true;
}

QT_END_NAMESPACE
