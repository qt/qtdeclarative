// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qqmlhover_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(hoverLog, "qt.languageserver.hover")

QQmlHover::QQmlHover(QmlLsp::QQmlCodeModel *codeModel)
    : QQmlBaseModule(codeModel)
{
}

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
    using namespace QQmlJS::Dom;
    QLspSpecification::Hover result;
    ResponseScopeGuard guard(result, request->m_response);

    if (!request) {
        qCWarning(hoverLog) << "No hover information is available!";
        return;
    }

    const auto textDocument = request->m_parameters.textDocument;
    const auto [hoveredLine, hoveredCharacter] = request->m_parameters.position;
    qCDebug(hoverLog) << QStringLiteral("Hovered (line, col): (%1,%2)").arg(hoveredLine).arg(hoveredCharacter);

    const auto doc = m_codeModel->openDocumentByUrl(
                QQmlLSUtils::lspUriToQmlUrl(textDocument.uri));

    DomItem file = doc.snapshot.doc.fileObject(GoTo::MostLikely);
    if (!file) {
        guard.setError(QQmlLSUtils::ErrorMessage{
                0, u"Could not find the file %1"_s.arg(doc.snapshot.doc.canonicalFilePath()) });
        return;
    }

    // TODO: Fetch the actual documentation or other possible infos to be shown when hovered.
    // Early return if hovered element is not identifier kind (for example, don't perform anything on paranthesis)

    const auto documentation = QQmlLSUtils::getDocumentationFromLocation(
            file, { hoveredLine + 1, hoveredCharacter + 1 });
    if (documentation.isEmpty()) {
        qCDebug(hoverLog)
                << QStringLiteral(
                           "No documentation hints found for the item at (line, col): (%1,%2)")
                           .arg(hoveredLine)
                           .arg(hoveredCharacter);
        return;
    }

    QLspSpecification::MarkupContent content;

    // TODO: This should eventually be a Markdown kind.
    // We will do post-formatting what we fetch from documentation.
    content.kind = QLspSpecification::MarkupKind::PlainText;
    content.value = documentation;

    result.contents = content;
}

QT_END_NAMESPACE
