// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlhover_p.h"
#include <QtQmlLS/private/qqmllshelputils_p.h>

QT_BEGIN_NAMESPACE

Q_STATIC_LOGGING_CATEGORY(hoverLog, "qt.languageserver.hover")

QQmlHover::QQmlHover(QmlLsp::QQmlCodeModel *codeModel)
    : QQmlBaseModule(codeModel), m_helpManager(std::make_unique<HelpManager>())
{
    // if set thorugh the commandline
    if (!codeModel->documentationRootPath().isEmpty())
        m_helpManager->setDocumentationRootPath(codeModel->documentationRootPath());

    connect(codeModel, &QmlLsp::QQmlCodeModel::documentationRootPathChanged, this, [this](const QString &path) {
        m_helpManager->setDocumentationRootPath(path);
    });
}

QQmlHover::~QQmlHover() = default;

QString QQmlHover::name() const
{
    return u"QQmlHover"_s;
}

void QQmlHover::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerHoverRequestHandler(getRequestHandler());
}

void QQmlHover::setupCapabilities(
        const QLspSpecification::InitializeParams &,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    serverCapabilities.capabilities.hoverProvider = true;
}

void QQmlHover::process(RequestPointerArgument request)
{
    if (!m_helpManager) {
        qCWarning(hoverLog)
                << "No help manager is available, documentation hints will not function!";
        return;
    }
    using namespace QQmlJS::Dom;
    QLspSpecification::Hover result;
    ResponseScopeGuard guard(result, request->m_response);
    if (!request) {
        qCWarning(hoverLog) << "No hover information is available!";
        return;
    }
    const auto textDocument = request->m_parameters.textDocument;
    const auto position = request->m_parameters.position;
    const auto doc = m_codeModel->openDocumentByUrl(QQmlLSUtils::lspUriToQmlUrl(textDocument.uri));
    DomItem file = doc.snapshot.doc.fileObject(GoTo::MostLikely);
    if (!file) {
        guard.setError(QQmlLSUtils::ErrorMessage{
                0, u"Could not find the file %1"_s.arg(doc.snapshot.doc.canonicalFilePath()) });
        return;
    }

    const auto documentation = m_helpManager->documentationForItem(file, position);
    if (!documentation.has_value()) {
        qCDebug(hoverLog)
                << QStringLiteral(
                           "No documentation hints found for the item at (line, col): (%1,%2)")
                           .arg(position.line)
                           .arg(position.character);
        return;
    }
    QLspSpecification::MarkupContent content;
    // TODO: We need to do post-formatting what we fetch from documentation.
    content.kind = QLspSpecification::MarkupKind::Markdown;
    content.value = documentation.value();
    result.contents = std::move(content);
}

QT_END_NAMESPACE
