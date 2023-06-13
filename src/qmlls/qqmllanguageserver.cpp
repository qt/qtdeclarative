// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllanguageserver_p.h"
#include "qtextsynchronization_p.h"
#include "qlanguageserver_p.h"
#include "qlspcustomtypes_p.h"

#include <QtCore/qdir.h>

#include <iostream>
#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QmlLsp {

using namespace QLspSpecification;
using namespace Qt::StringLiterals;
/*!
\internal
\class QmlLsp::QQmlLanguageServer
\brief Sets up a QmlLanguageServer.

This class sets up a QML language server.

Use the following function to send replies:

\code
std::function<void(const QByteArray &)> sendData
\endcode

And, feed the data that the function receives to the \c {server()->receive()}
method.

Call this method only from a single thread, and do not block. To achieve this,
avoid direct calls, and connect the method as a slot, while reading from another
thread.

The various tasks of the language server are divided between
QLanguageServerModule instances. Each instance is responsible for handling a
certain subset of client requests. For example, one instance handles completion
requests, another one updates the code in the code model when the client sends a
new file version, and so on. The QLanguageServerModule instances are
constructed and registered with QLanguageServer in the constructor of
this class.

Generally, do all operations in the object thread and always call handlers from
it. However, the operations can delegate the response to another thread, as the
response handler is thread safe. All the methods of the \c server() object are
also thread safe.

The code model starts other threads to update its state. See its documentation
for more information.
*/
QQmlLanguageServer::QQmlLanguageServer(std::function<void(const QByteArray &)> sendData,
                                       QQmlToolingSettings *settings)
    : m_codeModel(nullptr, settings),
      m_server(sendData),
      m_textSynchronization(&m_codeModel),
      m_lint(&m_server, &m_codeModel),
      m_workspace(&m_codeModel),
      m_completionSupport(&m_codeModel),
      m_navigationSupport(&m_codeModel),
      m_definitionSupport(&m_codeModel),
      m_referencesSupport(&m_codeModel),
      m_documentFormatting(&m_codeModel)
{
    m_server.addServerModule(this);
    m_server.addServerModule(&m_textSynchronization);
    m_server.addServerModule(&m_lint);
    m_server.addServerModule(&m_workspace);
    m_server.addServerModule(&m_completionSupport);
    m_server.addServerModule(&m_navigationSupport);
    m_server.addServerModule(&m_definitionSupport);
    m_server.addServerModule(&m_referencesSupport);
    m_server.addServerModule(&m_documentFormatting);
    m_server.finishSetup();
    qCWarning(lspServerLog) << "Did Setup";
}

void QQmlLanguageServer::registerHandlers(QLanguageServer *server,
                                          QLanguageServerProtocol *protocol)
{
    Q_UNUSED(protocol);
    QObject::connect(server, &QLanguageServer::lifecycleError, this,
                     &QQmlLanguageServer::errorExit);
    QObject::connect(server, &QLanguageServer::exit, this, &QQmlLanguageServer::exit);
    QObject::connect(server, &QLanguageServer::runStatusChanged, [](QLanguageServer::RunStatus r) {
        qCDebug(lspServerLog) << "runStatus" << int(r);
    });
    protocol->typedRpc()->registerNotificationHandler<Notifications::AddBuildDirsParams>(
            QByteArray(Notifications::AddBuildDirsMethod),
            [this](const QByteArray &, const Notifications::AddBuildDirsParams &params) {
                for (const auto &buildDirs : params.buildDirsToSet) {
                    QStringList dirPaths;
                    dirPaths.resize(buildDirs.buildDirs.size());
                    std::transform(buildDirs.buildDirs.begin(), buildDirs.buildDirs.end(),
                                   dirPaths.begin(), [](const QByteArray &utf8Str) {
                                       return QString::fromUtf8(utf8Str);
                                   });
                    m_codeModel.setBuildPathsForRootUrl(buildDirs.baseUri, dirPaths);
                }
            });
}

void QQmlLanguageServer::setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                                           QLspSpecification::InitializeResult &serverInfo)
{
    Q_UNUSED(clientInfo);
    QJsonObject expCap;
    if (serverInfo.capabilities.experimental.has_value() && serverInfo.capabilities.experimental->isObject())
        expCap = serverInfo.capabilities.experimental->toObject();
    expCap.insert(u"addBuildDirs"_s, QJsonObject({ { u"supported"_s, true } }));
    serverInfo.capabilities.experimental = expCap;
}

QString QQmlLanguageServer::name() const
{
    return u"QQmlLanguageServer"_s;
}

void QQmlLanguageServer::errorExit()
{
    qCWarning(lspServerLog) << "Error exit";
    fclose(stdin);
}

void QQmlLanguageServer::exit()
{
    m_returnValue = 0;
    fclose(stdin);
}

int QQmlLanguageServer::returnValue() const
{
    return m_returnValue;
}

QQmlCodeModel *QQmlLanguageServer::codeModel()
{
    return &m_codeModel;
}

QLanguageServer *QQmlLanguageServer::server()
{
    return &m_server;
}

TextSynchronization *QQmlLanguageServer::textSynchronization()
{
    return &m_textSynchronization;
}

QmlLintSuggestions *QQmlLanguageServer::lint()
{
    return &m_lint;
}

WorkspaceHandlers *QQmlLanguageServer::worspace()
{
    return &m_workspace;
}

} // namespace QmlLsp

QT_END_NAMESPACE
