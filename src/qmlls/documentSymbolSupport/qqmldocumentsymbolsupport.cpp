// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldocumentsymbolsupport_p.h"
#include "documentSymbolSupport/documentsymbolutils_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
QQmlDocumentSymbolSupport::QQmlDocumentSymbolSupport(QmlLsp::QQmlCodeModel *model)
    : BaseT(model) { }

QString QQmlDocumentSymbolSupport::name() const
{
    return u"QQmlDocumentSymbolSupport"_s;
}

void QQmlDocumentSymbolSupport::setupCapabilities(
        const QLspSpecification::InitializeParams &,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    serverCapabilities.capabilities.documentSymbolProvider = true;
}

void QQmlDocumentSymbolSupport::registerHandlers(QLanguageServer *,
                                                 QLanguageServerProtocol *protocol)
{
    protocol->registerDocumentSymbolRequestHandler(getRequestHandler());
}

void QQmlDocumentSymbolSupport::process(QQmlDocumentSymbolSupport::RequestPointerArgument request)
{
    const auto doc = m_codeModel->openDocumentByUrl(
            QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri));
    const auto qmlFileItem = doc.snapshot.doc.fileObject(QQmlJS::Dom::GoTo::MostLikely);

    auto results = DocumentSymbolUtils::assembleSymbolsForQmlFile(qmlFileItem);
    ResponseScopeGuard guard(results, request->m_response);
}

QT_END_NAMESPACE
