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
#include "textsynchronization.h"
#include "qqmllanguageserver.h"

#include "textdocument.h"

using namespace QLspSpecification;
QT_BEGIN_NAMESPACE

TextSynchronization::TextSynchronization(QmlLsp::QQmlCodeModel *codeModel, QObject *parent)
    : QLanguageServerModule(parent), m_codeModel(codeModel)
{
}

void TextSynchronization::didCloseTextDocument(const DidCloseTextDocumentParams &params)
{
    m_codeModel->closeOpenFile(params.textDocument.uri);
}

void TextSynchronization::didOpenTextDocument(const DidOpenTextDocumentParams &params)
{
    const TextDocumentItem &item = params.textDocument;
    const QString fileName = m_codeModel->uri2Path(item.uri);

    m_codeModel->newOpenFile(item.uri, item.version, item.text);
}

void TextSynchronization::didDidChangeTextDocument(const DidChangeTextDocumentParams &params)
{
    QByteArray uri = params.textDocument.uri;
    const QString fileName = m_codeModel->uri2Path(uri);
    auto openDoc = m_codeModel->openDocumentByUri(uri);
    std::shared_ptr<Utils::TextDocument> document = openDoc.textDocument;
    if (!document) {
        qCWarning(lspServerLog) << "Ingnoring changes to non open or closed document"
                                << QString::fromUtf8(uri);
        return;
    }
    const auto &changes = params.contentChanges;
    {
        QMutexLocker l(document->mutex());
        for (const auto &change : changes) {
            if (!change.range) {
                document->setPlainText(change.text);
                continue;
            }

            const auto &range = *change.range;
            const auto &rangeStart = range.start;
            const int start =
                    document->findBlockByNumber(rangeStart.line).position() + rangeStart.character;
            const auto &rangeEnd = range.end;
            const int end =
                    document->findBlockByNumber(rangeEnd.line).position() + rangeEnd.character;

            document->setPlainText(
                    document->toPlainText().replace(start, end - start, change.text));
        }
        document->setVersion(params.textDocument.version);
    }
    m_codeModel->addOpenToUpdate(uri);
    m_codeModel->openNeedUpdate();
}

void TextSynchronization::registerHandlers(QLanguageServer *server, QLanguageServerProtocol *)
{
    QObject::connect(server->notifySignals(),
                     &QLspNotifySignals::receivedDidOpenTextDocumentNotification, this,
                     &TextSynchronization::didOpenTextDocument);

    QObject::connect(server->notifySignals(),
                     &QLspNotifySignals::receivedDidChangeTextDocumentNotification, this,
                     &TextSynchronization::didDidChangeTextDocument);

    QObject::connect(server->notifySignals(),
                     &QLspNotifySignals::receivedDidCloseTextDocumentNotification, this,
                     &TextSynchronization::didCloseTextDocument);
}

QString TextSynchronization::name() const
{
    return u"TextSynchonization"_qs;
}

void TextSynchronization::setupCapabilities(const QLspSpecification::InitializeParams &,
                                            QLspSpecification::InitializeResult &serverInfo)
{
    TextDocumentSyncOptions syncOptions;
    syncOptions.openClose = true;
    syncOptions.change = TextDocumentSyncKind::Incremental;
    serverInfo.capabilities.textDocumentSync = syncOptions;
}

QT_END_NAMESPACE
