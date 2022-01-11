/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QQMLLANGUAGESERVER_H
#define QQMLLANGUAGESERVER_H

#include "qlanguageserver_p.h"
#include "qqmlcodemodel.h"
#include "textsynchronization.h"
#include "qmllintsuggestions.h"
#include "workspace.h"
#include "../shared/qqmltoolingsettings.h"

QT_BEGIN_NAMESPACE
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

public slots:
    void exit();
    void errorExit();

private:
    QQmlCodeModel m_codeModel;
    QLanguageServer m_server;
    TextSynchronization m_textSynchronization;
    QmlLintSuggestions m_lint;
    WorkspaceHandlers m_workspace;
    int m_returnValue = 1;
};

} // namespace QmlLsp
QT_END_NAMESPACE
#endif // QQMLLANGUAGESERVER_H
