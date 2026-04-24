// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qlspcustomtypes_p.h"
#include "qworkspace_p.h"
#include "qqmllanguageserver_p.h"
#include "qqmllsutils_p.h"

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtLanguageServer/private/qlspnotifysignals_p.h>

#include <QtCore/qfile.h>
#include <variant>

QT_BEGIN_NAMESPACE
using namespace Qt::StringLiterals;
using namespace QLspSpecification;

void WorkspaceHandlers::registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol)
{
    QObject::connect(server->notifySignals(),
                     &QLspNotifySignals::receivedDidChangeWorkspaceFoldersNotification, this,
                     [this](const DidChangeWorkspaceFoldersParams &params) {
                         const WorkspaceFoldersChangeEvent &event = params.event;

                         const QList<WorkspaceFolder> &removed = event.removed;
                         QList<QByteArray> toRemove;
                         for (const WorkspaceFolder &folder : removed) {
                             toRemove.append(QQmlLSUtils::lspUriToQmlUrl(folder.uri));
                             m_codeModelManager->removeDirectory(
                                     QQmlLSUtils::lspUriToQmlUrl(folder.uri));
                         }
                         m_codeModelManager->removeRootUrls(toRemove);
                         const QList<WorkspaceFolder> &added = event.added;
                         QList<QByteArray> toAdd;
                         for (const WorkspaceFolder &folder : added) {
                             toAdd.append(QQmlLSUtils::lspUriToQmlUrl(folder.uri));
                         }
                         m_codeModelManager->addRootUrls(toAdd);
                     });

    QObject::connect(server, &QLanguageServer::clientInitialized, this,
                     &WorkspaceHandlers::clientInitialized, Qt::SingleShotConnection);

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
                    m_codeModelManager->setBuildPathsForRootUrl(buildDirs.baseUri, dirPaths);
                }
            });
}

void WorkspaceHandlers::setupCapabilities(QLspSpecification::ServerCapabilities &caps)
{
    WorkspaceFoldersServerCapabilities folders;
    folders.supported = true;
    folders.changeNotifications = true;
    if (!caps.workspace)
        caps.workspace = QJsonObject();
    caps.workspace->insert(u"workspaceFolders"_s, QTypedJson::toJsonValue(folders));

    QJsonObject expCap;
    if (caps.experimental.has_value() && caps.experimental->isObject())
        expCap = caps.experimental->toObject();
    expCap.insert(u"addBuildDirs"_s, QJsonObject({ { u"supported"_s, true } }));
    caps.experimental = expCap;
}

void WorkspaceHandlers::openInitialWorkspace(const InitializeParams &clientInfo)
{
    if (clientInfo.workspaceFolders) {
        QList<QByteArray> rootPaths;
        for (const auto &folder : *clientInfo.workspaceFolders) {
            rootPaths.append(QQmlLSUtils::lspUriToQmlUrl(folder.uri));
        }
        m_codeModelManager->addRootUrls(rootPaths);
        return;
    }

    // note: rootUri is deprecated in the LSP protocol
    if (clientInfo.rootUri) {
        m_codeModelManager->addRootUrls({ QQmlLSUtils::lspUriToQmlUrl(*clientInfo.rootUri) });
        return;
    }
    // note: rootPath is also deprecated in the LSP protocol. It was deprecated even before rootUri
    // was deprecated.
    if (clientInfo.rootPath) {
        m_codeModelManager->addRootUrls({
                QUrl::fromLocalFile(QString::fromUtf8(*clientInfo.rootPath)).toEncoded(),
        });
        return;
    }
}

void WorkspaceHandlers::clientInitialized(QLanguageServer *server)
{
    const auto &clientInfo = server->clientInfo();

    if (clientInfo.capabilities.workspace
        && clientInfo.capabilities.workspace->workspaceFolders.value_or(false)) {
        openInitialWorkspace(clientInfo);
    }
}

QT_END_NAMESPACE
