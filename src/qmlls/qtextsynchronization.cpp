// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtextsynchronization_p.h"
#include "qqmllsutils_p.h"
#include "qtextdocument_p.h"

using namespace QLspSpecification;
using namespace Qt::StringLiterals;

QT_BEGIN_NAMESPACE

TextSynchronization::TextSynchronization(QmlLsp::QQmlCodeModel *codeModel, QObject *parent)
    : QLanguageServerModule(parent), m_codeModel(codeModel)
{
}

void TextSynchronization::didCloseTextDocument(const DidCloseTextDocumentParams &params)
{
    m_codeModel->closeOpenFile(QQmlLSUtils::lspUriToQmlUrl(params.textDocument.uri));
}

void TextSynchronization::didOpenTextDocument(const DidOpenTextDocumentParams &params)
{
    const TextDocumentItem &item = params.textDocument;
    const QString fileName = m_codeModel->url2Path(QQmlLSUtils::lspUriToQmlUrl(item.uri));
    m_codeModel->newOpenFile(QQmlLSUtils::lspUriToQmlUrl(item.uri), item.version,
                             QString::fromUtf8(item.text));
}

void TextSynchronization::didDidChangeTextDocument(const DidChangeTextDocumentParams &params)
{
    QByteArray url = QQmlLSUtils::lspUriToQmlUrl(params.textDocument.uri);
    const QString fileName = m_codeModel->url2Path(url);
    auto openDoc = m_codeModel->openDocumentByUrl(url);
    std::shared_ptr<Utils::TextDocument> document = openDoc.textDocument;
    if (!document) {
        qCWarning(lspServerLog) << "Ignoring changes to non open or closed document"
                                << QString::fromUtf8(url);
        return;
    }
    const auto &changes = params.contentChanges;
    {
        QMutexLocker l(document->mutex());
        for (const auto &change : changes) {
            if (!change.range) {
                document->setPlainText(QString::fromUtf8(change.text));
                continue;
            }

            const auto &range = *change.range;
            const auto &rangeStart = range.start;
            const int start =
                    document->findBlockByNumber(rangeStart.line).position() + rangeStart.character;
            const auto &rangeEnd = range.end;
            const int end =
                    document->findBlockByNumber(rangeEnd.line).position() + rangeEnd.character;

            document->setPlainText(document->toPlainText().replace(start, end - start,
                                                                   QString::fromUtf8(change.text)));
        }
        document->setVersion(params.textDocument.version);
        qCDebug(lspServerLog).noquote()
                << "text is\n:----------" << document->toPlainText() << "\n_________";
    }
    m_codeModel->addOpenToUpdate(url);
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
    return u"TextSynchonization"_s;
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
