// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <qqmlhighlightsupport_p.h>
#include <qqmldiffer_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QLspSpecification;
using namespace QQmlJS::Dom;
using namespace QmlHighlighting;

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
    return enumToByteArray<SemanticTokenProtocolTypes>();
}

static QList<int> generateHighlights(QmlLsp::RegisteredSemanticTokens &cached,
                                     const QmlLsp::OpenDocument &doc,
                                     const std::optional<HighlightsRange> &range,
                                     HighlightingMode mode)
{
    DomItem file = doc.snapshot.doc.fileObject(GoTo::MostLikely);
    const auto fileObject = file.ownerAs<QmlFile>();
    QmlHighlighting::Utils::updateResultID(cached.resultId);
    if (!fileObject || !(fileObject && fileObject->isValid())) {
        if (const auto lastValidItem = doc.snapshot.validDoc.ownerAs<QmlFile>()) {
            const auto shiftedHighlights = QmlHighlighting::Utils::shiftHighlights(
                cached.highlights, lastValidItem->code(), doc.textDocument->toPlainText());
                return QmlHighlighting::Utils::encodeSemanticTokens(shiftedHighlights, mode);
        } else {
            // TODO: Implement regexp based fallback highlighting
            return {};
        }
    } else {
        HighlightsContainer highlights = QmlHighlighting::Utils::visitTokens(file, range);
        if (highlights.isEmpty())
            return {};
        // Record the highlights for future diffs, only record full highlights
        if (!range.has_value() )
            cached.highlights = highlights;

        return QmlHighlighting::Utils::encodeSemanticTokens(highlights, mode);
    }
}

/*!
\internal
A wrapper class that handles the semantic tokens request for a whole file as described in
https://microsoft.github.io/language-server-protocol/specifications/specification-3-16/#semanticTokens_fullRequest
Sends a QLspSpecification::SemanticTokens data as response that is generated for the entire file.
*/
SemanticTokenFullHandler::SemanticTokenFullHandler(QmlLsp::QQmlCodeModelManager *codeModelManager)
    : QQmlBaseModule(codeModelManager), m_mode(HighlightingMode::Default)
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
    const QByteArray uri = QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri);
    const auto doc = m_codeModelManager->openDocumentByUrl(uri);
    auto &cached = m_codeModelManager->registeredTokens(uri);
    const auto encoded = generateHighlights(cached, doc, std::nullopt, m_mode);

    if (encoded.isEmpty()) {
        result = nullptr;
        return;
    } else {
        result = SemanticTokens{cached.resultId, std::move(encoded)};
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
SemanticTokenDeltaHandler::SemanticTokenDeltaHandler(QmlLsp::QQmlCodeModelManager *codeModelManager)
    : QQmlBaseModule(codeModelManager), m_mode(HighlightingMode::Default)
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
    const QByteArray uri = QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri);
    const auto doc = m_codeModelManager->openDocumentByUrl(uri);
    auto &cached = m_codeModelManager->registeredTokens(uri);
    if (cached.resultId != request->m_parameters.previousResultId) {
        // The client is out of sync, send full tokens
        cached.resultId = request->m_parameters.previousResultId;
        const auto encoded = generateHighlights(cached, doc, std::nullopt, m_mode);
        result = QLspSpecification::SemanticTokens{ cached.resultId, encoded };
    } else {
        const auto cachedHighlights = QmlHighlighting::Utils::encodeSemanticTokens(cached.highlights);
        const auto encoded = generateHighlights(cached, doc, std::nullopt, m_mode);
        result = QLspSpecification::SemanticTokensDelta{
                cached.resultId,
                QmlHighlighting::Utils::computeDiff(cachedHighlights, encoded)
            };
    }
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
SemanticTokenRangeHandler::SemanticTokenRangeHandler(QmlLsp::QQmlCodeModelManager *codeModelManager)
    : QQmlBaseModule(codeModelManager), m_mode(HighlightingMode::Default)
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
    const QByteArray uri = QQmlLSUtils::lspUriToQmlUrl(request->m_parameters.textDocument.uri);
    const auto doc = m_codeModelManager->openDocumentByUrl(uri);
    const QString code = doc.textDocument->toPlainText();
    const auto range = request->m_parameters.range;
    int startOffset =
            int(QQmlLSUtils::textOffsetFrom(code, range.start.line, range.end.character));
    int endOffset = int(QQmlLSUtils::textOffsetFrom(code, range.end.line, range.end.character));
    auto &cached = m_codeModelManager->registeredTokens(uri);
    const auto encodedTokens = generateHighlights(
            cached,
            doc,
            QmlHighlighting::HighlightsRange{ startOffset, endOffset },
            m_mode);
    if (encodedTokens.isEmpty()) {
        result = nullptr;
    } else {
        result = SemanticTokens{ cached.resultId, std::move(encodedTokens) };
    }
}

void SemanticTokenRangeHandler::registerHandlers(QLanguageServer *, QLanguageServerProtocol *protocol)
{
    protocol->registerSemanticTokensRangeRequestHandler(getRequestHandler());
}

QQmlHighlightSupport::QQmlHighlightSupport(QmlLsp::QQmlCodeModelManager *codeModelManager)
    : m_full(codeModelManager), m_delta(codeModelManager), m_range(codeModelManager)
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

    QObject::connect(m_full.manager(), &QmlLsp::QQmlCodeModelManager::configurationChanged, this,
                     [protocol]() {
                         protocol->requestRequestingARefreshOfAllSemanticTokens({ }, []() { });
                     });
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
            const auto mode = HighlightingMode::QtCHighlighting;
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
