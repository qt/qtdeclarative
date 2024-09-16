// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qqmlhighlightsupport_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QLspSpecification;
using namespace QQmlJS::Dom;

/*!
\internal
Make a list of enum names to register the supported token
types and modifiers. It is case-sensitive in the protocol
thus we need to lower the first characters of the enum names.
*/
template <typename EnumType>
static QList<QByteArray> enumToByteArray()
{
    QList<QByteArray> result;
    QMetaEnum metaEnum = QMetaEnum::fromType<EnumType>();
    for (auto i = 0; i < metaEnum.keyCount(); ++i) {
        auto &&enumName = QByteArray(metaEnum.key(i));
        enumName.front() = std::tolower(enumName.front());
        result.emplace_back(std::move(enumName));
    }

    return result;
}

QList<QByteArray> defaultTokenModifiersList()
{
    return enumToByteArray<QLspSpecification::SemanticTokenModifiers>();
}

QList<QByteArray> extendedTokenTypesList()
{
    return enumToByteArray<HighlightingUtils::SemanticTokenProtocolTypes>();
}

/*!
\internal
A wrapper class that handles the semantic tokens request for a whole file as described in
https://microsoft.github.io/language-server-protocol/specifications/specification-3-16/#semanticTokens_fullRequest
Sends a QLspSpecification::SemanticTokens data as response that is generated for the entire file.
*/
SemanticTokenFullHandler::SemanticTokenFullHandler(QmlLsp::QQmlCodeModel *codeModel)
    : QQmlBaseModule(codeModel), m_mode(HighlightingUtils::HighlightingMode::Default)
{
}

void SemanticTokenFullHandler::process(
        QQmlBaseModule<SemanticTokensRequest>::RequestPointerArgument request)
{
    if (!request) {
        qCWarning(semanticTokens) << "No semantic token request is available!";
        return;
    }

    Responses::SemanticTokensResultType result;
    ResponseScopeGuard guard(result, request->m_response);
    const auto doc = m_codeModel->openDocumentByUrl(
            QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri));
    DomItem file = doc.snapshot.doc.fileObject(GoTo::MostLikely);
    const auto fileObject = file.ownerAs<QmlFile>();
    if (!fileObject || !(fileObject && fileObject->isValid())) {
        guard.setError({
                int(QLspSpecification::ErrorCodes::RequestCancelled),
                "Cannot proceed: current QML document is invalid!"_L1,
        });
        return;
    }
    auto &&encoded = HighlightingUtils::collectTokens(file, std::nullopt, m_mode);
    auto &registeredTokens = m_codeModel->registeredTokens();
    if (!encoded.isEmpty()) {
        HighlightingUtils::updateResultID(registeredTokens.resultId);
        result = SemanticTokens{ registeredTokens.resultId, encoded };
        registeredTokens.lastTokens = std::move(encoded);
    } else {
        result = nullptr;
    }
}

void SemanticTokenFullHandler::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerSemanticTokensRequestHandler(getRequestHandler());
}

/*!
\internal
A wrapper class that handles the semantic tokens delta request for a file
https://microsoft.github.io/language-server-protocol/specifications/specification-3-16/#semanticTokens_deltaRequest
Sends either SemanticTokens or SemanticTokensDelta data as response.
This is generally requested when the text document is edited after receiving full highlighting data.
*/
SemanticTokenDeltaHandler::SemanticTokenDeltaHandler(QmlLsp::QQmlCodeModel *codeModel)
    : QQmlBaseModule(codeModel), m_mode(HighlightingUtils::HighlightingMode::Default)
{
}

void SemanticTokenDeltaHandler::process(
        QQmlBaseModule<SemanticTokensDeltaRequest>::RequestPointerArgument request)
{
    if (!request) {
        qCWarning(semanticTokens) << "No semantic token request is available!";
        return;
    }

    Responses::SemanticTokensDeltaResultType result;
    ResponseScopeGuard guard(result, request->m_response);
    const auto doc = m_codeModel->openDocumentByUrl(
            QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri));
    DomItem file = doc.snapshot.doc.fileObject(GoTo::MostLikely);
    const auto fileObject = file.ownerAs<QmlFile>();
    if (!fileObject || !(fileObject && fileObject->isValid())) {
        guard.setError({
                int(QLspSpecification::ErrorCodes::RequestCancelled),
                "Cannot proceed: current QML document is invalid!"_L1,
        });
        return;
    }
    auto newEncoded = HighlightingUtils::collectTokens(file, std::nullopt, m_mode);
    auto &registeredTokens = m_codeModel->registeredTokens();
    const auto lastResultId = registeredTokens.resultId;
    HighlightingUtils::updateResultID(registeredTokens.resultId);

    // Return full token list if result ids not align
    // otherwise compute the delta.
    if (lastResultId == request->m_parameters.previousResultId) {
        result = QLspSpecification::SemanticTokensDelta{
            registeredTokens.resultId,
            HighlightingUtils::computeDiff(registeredTokens.lastTokens, newEncoded)
        };
    } else if (!newEncoded.isEmpty()) {
        result = QLspSpecification::SemanticTokens{ registeredTokens.resultId, newEncoded };
    } else {
        result = nullptr;
    }
    registeredTokens.lastTokens = std::move(newEncoded);
}

void SemanticTokenDeltaHandler::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerSemanticTokensDeltaRequestHandler(getRequestHandler());
}

/*!
\internal
A wrapper class that handles the semantic tokens range request for a file
https://microsoft.github.io/language-server-protocol/specifications/specification-3-16/#semanticTokens_rangeRequest
Sends a QLspSpecification::SemanticTokens data as response that is generated for a range of file.
*/
SemanticTokenRangeHandler::SemanticTokenRangeHandler(QmlLsp::QQmlCodeModel *codeModel)
    : QQmlBaseModule(codeModel), m_mode(HighlightingUtils::HighlightingMode::Default)
{
}

void SemanticTokenRangeHandler::process(
        QQmlBaseModule<SemanticTokensRangeRequest>::RequestPointerArgument request)
{
    if (!request) {
        qCWarning(semanticTokens) << "No semantic token request is available!";
        return;
    }

    Responses::SemanticTokensRangeResultType result;
    ResponseScopeGuard guard(result, request->m_response);
    const auto doc = m_codeModel->openDocumentByUrl(
            QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri));
    DomItem file = doc.snapshot.doc.fileObject(GoTo::MostLikely);
    const auto qmlFile = file.as<QmlFile>();
    if (!qmlFile || !(qmlFile && qmlFile->isValid())) {
        guard.setError({
                int(QLspSpecification::ErrorCodes::RequestCancelled),
                "Cannot proceed: current QML document is invalid!"_L1,
        });
        return;
    }
    const QString &code = qmlFile->code();
    const auto range = request->m_parameters.range;
    int startOffset =
            int(QQmlLSUtils::textOffsetFrom(code, range.start.line, range.end.character));
    int endOffset = int(QQmlLSUtils::textOffsetFrom(code, range.end.line, range.end.character));
    auto &&encoded = HighlightingUtils::collectTokens(
            file, HighlightsRange{ startOffset, endOffset }, m_mode);
    auto &registeredTokens = m_codeModel->registeredTokens();
    if (!encoded.isEmpty()) {
        HighlightingUtils::updateResultID(registeredTokens.resultId);
        result = SemanticTokens{ registeredTokens.resultId, std::move(encoded) };
    } else {
        result = nullptr;
    }
}

void SemanticTokenRangeHandler::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerSemanticTokensRangeRequestHandler(getRequestHandler());
}

QQmlHighlightSupport::QQmlHighlightSupport(QmlLsp::QQmlCodeModel *codeModel)
    : m_full(codeModel), m_delta(codeModel), m_range(codeModel)
{
}

QString QQmlHighlightSupport::name() const
{
    return "QQmlHighlightSupport"_L1;
}

void QQmlHighlightSupport::registerHandlers(QLanguageServer *server, QLanguageServerProtocol *protocol)
{
    m_full.registerHandlers(server, protocol);
    m_delta.registerHandlers(server, protocol);
    m_range.registerHandlers(server, protocol);
}

void QQmlHighlightSupport::setupCapabilities(
        const QLspSpecification::InitializeParams &clientCapabilities,
        QLspSpecification::InitializeResult &serverCapabilities)
{
    QLspSpecification::SemanticTokensOptions options;
    options.range = true;
    options.full = QJsonObject({ { u"delta"_s, true } });

    if (auto clientInitOptions = clientCapabilities.initializationOptions) {
        if ((*clientInitOptions)[u"qtCreatorHighlighting"_s].toBool(false)) {
            const auto mode = HighlightingUtils::HighlightingMode::QtCHighlighting;
            m_delta.setHighlightingMode(mode);
            m_full.setHighlightingMode(mode);
            m_range.setHighlightingMode(mode);
        }
    }
    options.legend.tokenTypes = extendedTokenTypesList();
    options.legend.tokenModifiers = defaultTokenModifiersList();
    serverCapabilities.capabilities.semanticTokensProvider = options;
}

QT_END_NAMESPACE
