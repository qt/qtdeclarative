// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#include "qqmllanguageserver.h"

#include "textsynchronization.h"

#include "qlanguageserver.h"
#include "lspcustomtypes.h"
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
\brief Class that sets up a QmlLanguageServer

This class sets up a QML language server.
It needs a function
\code
std::function<void(const QByteArray &)> sendData
\endcode
to send out its replies, and one should feed the data it receives to the server()->receive() method.
It is expected to call this method only from a single thread, and not to block, the simplest way to
achieve this is to avoid direct calls, and connect it as slot, while reading from another thread.

The Server is build with separate QLanguageServerModule that implement a given functionality, and
all of them are constructed and registered with the QLanguageServer in the constructor o this class.

Generally all operations are expected to be done in the object thread, and handlers are always
called from it, but they are free to delegate the response to another thread, the response handler
is thread safe. All the methods of the server() obect are also threadsafe.

The code model starts other threads to update its state, see its documentation for more information.
*/
QQmlLanguageServer::QQmlLanguageServer(std::function<void(const QByteArray &)> sendData,
                                       QQmlToolingSettings *settings)
    : m_codeModel(nullptr, settings),
      m_server(sendData),
      m_textSynchronization(&m_codeModel),
      m_lint(&m_server, &m_codeModel),
      m_workspace(&m_codeModel),
      m_completionSupport(&m_codeModel)
{
    m_server.addServerModule(this);
    m_server.addServerModule(&m_textSynchronization);
    m_server.addServerModule(&m_lint);
    m_server.addServerModule(&m_workspace);
    m_server.addServerModule(&m_completionSupport);
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
