// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef QQMLLANGUAGESERVER_P_H
#define QQMLLANGUAGESERVER_P_H

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
#include "qqmlcodemodel_p.h"
#include "qtextsynchronization_p.h"
#include "qqmllintsuggestions_p.h"
#include "qworkspace_p.h"
#include "qqmlcompletionsupport_p.h"

QT_BEGIN_NAMESPACE

class QQmlToolingSettings;

namespace QmlLsp {

/*
 * The language server protocol calls "URI" what QML calls "URL".
 * According to RFC 3986, a URL is a special case of URI that not only
 * identifies a resource but also shows how to access it.
 * In QML, however, URIs are distinct from URLs. URIs are the
 * identifiers of modules, for example "QtQuick.Controls".
 * In order to not confuse the terms we interpret language server URIs
 * as URLs in the QML code model.
 * This method marks a point of translation between the terms, but does
 * not have to change the actual URI/URL.
 */
inline QByteArray lspUriToQmlUrl(const QByteArray &uri) { return uri; }

class QQmlLanguageServer : public QLanguageServerModule
{
    Q_OBJECT
public:
    QQmlLanguageServer(std::function<void(const QByteArray &)> sendData,
                       QQmlToolingSettings *settings = nullptr);

    QString name() const final;
    void registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol) final;
    void setupCapabilities(const QLspSpecification::InitializeParams &clientInfo,
                           QLspSpecification::InitializeResult &serverInfo) final;

    int returnValue() const;

    QQmlCodeModel *codeModel();
    QLanguageServer *server();
    TextSynchronization *textSynchronization();
    QmlLintSuggestions *lint();
    WorkspaceHandlers *worspace();

public Q_SLOTS:
    void exit();
    void errorExit();

private:
    QQmlCodeModel m_codeModel;
    QLanguageServer m_server;
    TextSynchronization m_textSynchronization;
    QmlLintSuggestions m_lint;
    WorkspaceHandlers m_workspace;
    QmlCompletionSupport m_completionSupport;
    int m_returnValue = 1;
};

} // namespace QmlLsp
QT_END_NAMESPACE
#endif // QQMLLANGUAGESERVER_P_H
