// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtCore/qstring.h>

#include "qqmlprogresssupport_p.h"

using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

QQmlProgressSupport::QQmlProgressSupport(QmlLsp::QQmlCodeModelManager *manager)
    : m_codeModelManager(manager)
{
}

void QQmlProgressSupport::registerHandlers(QLanguageServer *server,
                                           QLanguageServerProtocol *protocol)
{
    m_protocol = protocol;
    QObject::connect(server, &QLanguageServer::clientInitialized, this,
                     &QQmlProgressSupport::clientInitialized);
}

void QQmlProgressSupport::clientInitialized(QLanguageServer *server)
{
    if (auto window = server->clientInfo().capabilities.window;
        !window || !window->workDoneProgress.value_or(false)) {
        return;
    }

    QObject::connect(m_codeModelManager, &QmlLsp::QQmlCodeModelManager::backgroundBuildStarted,
                     this, &QQmlProgressSupport::onBackgroundBuildStarted);
    QObject::connect(m_codeModelManager, &QmlLsp::QQmlCodeModelManager::backgroundBuildFinished,
                     this, &QQmlProgressSupport::onBackgroundBuildDone);

    QObject::connect(server->notifySignals(),
                     &QLspNotifySignals::receivedWorkDoneProgressCancelNotification, this,
                     &QQmlProgressSupport::onBackgroundBuildCancelRequested);
}

int QQmlProgressSupport::Tokens::createUniqueToken(const QByteArray &uri)
{
    const int token = m_idForBackgroundBuilds++;
    m_tokens.insert(uri, { uri, token, InCreation });
    m_uriByToken.insert(token, uri);
    return token;
}

QQmlProgressSupport::UriWithToken *QQmlProgressSupport::Tokens::find(const QByteArray &uri)
{
    const auto it = m_tokens.find(uri);
    return it == m_tokens.end() ? nullptr : &*it;
}

std::optional<QQmlProgressSupport::UriWithToken> QQmlProgressSupport::Tokens::takeToken(int token)
{
    const auto it = m_uriByToken.find(token);
    if (it == m_uriByToken.end())
        return {};

    const auto it2 = m_tokens.find(*it);
    m_uriByToken.erase(it);
    if (it2 == m_tokens.end())
        return {};
    std::optional<QQmlProgressSupport::UriWithToken> result = std::move(*it2);
    m_tokens.erase(it2);
    return result;
}

void QQmlProgressSupport::Tokens::removeToken(const QByteArray &uri)
{
    const auto it = m_tokens.find(uri);
    if (it != m_tokens.end())
        return;

    if (const auto it2 = m_uriByToken.find(it->token); it2 != m_uriByToken.end())
        m_uriByToken.erase(it2);
    m_tokens.erase(it);
}

void QQmlProgressSupport::onBackgroundBuildStarted(const QByteArray &uri)
{
    QLspSpecification::Requests::WorkDoneProgressCreateParamsType p;
    const int token = m_tokens.createUniqueToken(uri);
    p.token = token;
    m_protocol->requestWorkDoneProgressCreate(p, [this, uri, token]() {
        QLspSpecification::ProgressParams beginParams{ token };
        QLspSpecification::WorkDoneProgressBegin workDoneProgressBegin{};
        workDoneProgressBegin.title = "Qmlls running background build";
        workDoneProgressBegin.cancellable = true;
        workDoneProgressBegin.message =
                "Building \"" + QUrl::fromEncoded(uri).toLocalFile().toUtf8() + "\"";
        workDoneProgressBegin.cancellable = true;
        beginParams.value = workDoneProgressBegin;
        m_protocol->notifyProgress(beginParams);

        const auto token = m_tokens.find(uri);
        if (!token)
            return;

        switch (token->status) {
        case InCreation:
            token->status = Created;
            break;
        case Created:
            Q_ASSERT(false);
        case Finished:
            onBackgroundBuildDone(token->uri);
            break;
        }
    });
}

void QQmlProgressSupport::onBackgroundBuildDone(const QByteArray &uri)
{
    const auto token = m_tokens.find(uri);
    if (!token)
        return;

    switch (token->status) {
    case InCreation:
        // We can't report progress if the WorkDoneProgressCreate request didn't finish yet. Let the
        // WorkDoneProgressCreate callback call this method again after the request was created.
        token->status = Finished;
        return;
    case Finished:
    case Created:
        QLspSpecification::WorkDoneProgressEnd workDoneProgressEnd;
        workDoneProgressEnd.message = "Build terminated";
        const QLspSpecification::ProgressParams endParams{ token->token, workDoneProgressEnd };
        m_protocol->notifyProgress(endParams);
        m_tokens.removeToken(token->uri);
    }
}

void QQmlProgressSupport::onBackgroundBuildCancelRequested(
        const QLspSpecification::Notifications::WorkDoneProgressCancelParamsType &p)
{
    const auto tokenNumber = std::get_if<int>(&p.token);
    if (!tokenNumber) {
        qCWarning(lspServerLog) << "Ignoring unknown token" << std::get<QByteArray>(p.token)
                                << "in cancellation request.";
        return;
    }

    const auto token = m_tokens.takeToken(*tokenNumber);
    if (!token) {
        qCWarning(lspServerLog) << "Ignoring unknown token" << *tokenNumber
                                << "in cancellation request.";
        return;
    }

    m_codeModelManager->cancelBackgroundBuild(token->uri);
}

void QQmlProgressSupport::setupCapabilities(QLspSpecification::ServerCapabilities &) { }

QT_END_NAMESPACE
