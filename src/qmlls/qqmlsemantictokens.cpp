// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qqmlsemantictokens_p.h>

#include <QtQmlLS/private/qqmllsutils_p.h>
#include <QtQmlDom/private/qqmldomscriptelements_p.h>
#include <QtQmlDom/private/qqmldomfieldfilter_p.h>

#include <QtLanguageServer/private/qlanguageserverspec_p.h>
#include <QtLanguageServer/private/qlanguageserverprotocol_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(semanticTokens, "qt.languageserver.semanticTokens")

using namespace QQmlJS::AST;
using namespace QQmlJS::Dom;
using namespace QLspSpecification;

static int tokenTypeFromRegion(QQmlJS::Dom::FileLocationRegion region)
{
    switch (region) {
    case AsTokenRegion:
    case BreakKeywordRegion:
    case DoKeywordRegion:
    case CaseKeywordRegion:
    case CatchKeywordRegion:
    case ComponentKeywordRegion:
    case ContinueKeywordRegion:
    case ElseKeywordRegion:
    case EnumKeywordRegion:
    case ForKeywordRegion:
    case FinallyKeywordRegion:
    case FunctionKeywordRegion:
    case ImportTokenRegion:
    case OnTokenRegion:
    case PragmaKeywordRegion:
    case ReturnKeywordRegion:
    case SignalKeywordRegion:
    case ThrowKeywordRegion:
    case TryKeywordRegion:
    case WhileKeywordRegion:
    case PropertyKeywordRegion:
    case InOfTokenRegion:
    case DefaultKeywordRegion:
    case ReadonlyKeywordRegion:
    case RequiredKeywordRegion:
    case IfKeywordRegion:
    case SwitchKeywordRegion:
        return int(SemanticTokenTypes::Keyword);
    case QuestionMarkTokenRegion:
    case EllipsisTokenRegion:
    case OperatorTokenRegion:
        return int(SemanticTokenTypes::Operator);
    case QQmlJS::Dom::TypeIdentifierRegion:
        return int(SemanticTokenTypes::Type);
    case PragmaValuesRegion:
    case IdentifierRegion:
    case IdNameRegion:
        return int(SemanticTokenTypes::Variable);
    case ImportUriRegion:
        return int(SemanticTokenTypes::Namespace);
    case IdTokenRegion:
    case OnTargetRegion:
        return int(SemanticTokenTypes::Property);
    case VersionRegion:
        return int(SemanticTokenTypes::Number);
    default:
        return int(SemanticTokenTypes::Variable);
    }
    Q_UNREACHABLE_RETURN({});
}

static FieldFilter highlightingFilter()
{
    QMultiMap<QString, QString> fieldFilterAdd{};
    QMultiMap<QString, QString> fieldFilterRemove{
        { QString(), QString::fromUtf16(Fields::propertyInfos) },
        { QString(), QString::fromUtf16(Fields::fileLocationsTree) },
        { QString(), QString::fromUtf16(Fields::importScope) },
        { QString(), QString::fromUtf16(Fields::defaultPropertyName) },
        { QString(), QString::fromUtf16(Fields::get) },
    };
    return FieldFilter{ fieldFilterAdd, fieldFilterRemove };
}

bool HighlightingVisitor::operator()(Path, const DomItem &item, bool)
{
    switch (item.internalKind()) {
    case DomType::Comment:{
        highlightComment(item);
        return true;
    }
    case DomType::Import: {
        highlightImport(item);
        return true;
    }
    case DomType::Binding: {
        highlightBinding(item);
        return true;
    }
    default:
        return true;
    }
    Q_UNREACHABLE_RETURN(false);
}

void HighlightingVisitor::highlightComment(const DomItem &item)
{
    const auto comment = item.as<Comment>();
    Q_ASSERT(comment);
    const auto locs = HighlightingUtils::sourceLocationsFromMultiLineToken(
            comment->info().comment(), comment->info().sourceLocation());
    for (const auto &loc : locs)
        m_highlights.addHighlight(loc, int(SemanticTokenTypes::Comment));
}

void HighlightingVisitor::highlightImport(const DomItem &item)
{
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs)
        return;
    const auto regions = fLocs->info().regions;
    const auto import = item.as<Import>();
    Q_ASSERT(import);
    m_highlights.addHighlight(regions, ImportTokenRegion);
    if (import->uri.isModule())
        m_highlights.addHighlight(regions[ImportUriRegion], int(SemanticTokenTypes::Namespace));
    else
        m_highlights.addHighlight(regions[ImportUriRegion], int(SemanticTokenTypes::String));
    if (regions.contains(VersionRegion))
        m_highlights.addHighlight(regions, VersionRegion);
    if (regions.contains(AsTokenRegion)) {
        m_highlights.addHighlight(regions, AsTokenRegion);
        m_highlights.addHighlight(regions[IdNameRegion], int(SemanticTokenTypes::Namespace));
    }
}

void HighlightingVisitor::highlightBinding(const DomItem &item)
{
    const auto binding = item.as<Binding>();
    Q_ASSERT(binding);
    const auto fLocs = FileLocations::treeOf(item);
    if (!fLocs) {
        qCDebug(semanticTokens) << "Can't find the locations for" << item.internalKind();
        return;
    }
    const auto regions = fLocs->info().regions;
    // If dotted name, then defer it to be handled in ScriptIdentifierExpression
    if (binding->name().contains("."_L1))
        return;

    if (binding->bindingType() == BindingType::Normal) {
        m_highlights.addHighlight(regions[IdentifierRegion], int(SemanticTokenTypes::Property));
        // TODO: Binding property could have been marked as Required, Readonly or Const
        // Should also add modifier depending on the declaration of the property
        // Should go to defined scope and check readonly, required and const flags.
    } else {
        m_highlights.addHighlight(regions, OnTokenRegion);
        m_highlights.addHighlight(regions[IdentifierRegion], int(SemanticTokenTypes::Property));
    }
}

/*! \internal
    \brief Returns multiple source locations for a given raw comment

    Needed by semantic highlighting of comments. LSP clients usually don't support multiline
    tokens. In QML, we can have multiline tokens like string literals and comments.
    This method generates multiple source locations of sub-elements of token split by a newline
   delimiter.


*/
QList<QQmlJS::SourceLocation>
HighlightingUtils::sourceLocationsFromMultiLineToken(QStringView stringLiteral,
                                                     const QQmlJS::SourceLocation &locationInDocument)
{
    auto lineBreakLength = qsizetype(std::char_traits<char>::length("\n"));
    const auto lineLengths = [&lineBreakLength](QStringView literal) {
        std::vector<qsizetype> lineLengths;
        qsizetype startIndex = 0;
        qsizetype pos = literal.indexOf(u'\n');
        while (pos != -1) {
            // TODO: QTBUG-106813
            // Since a document could be opened in normalized form
            // we can't use platform dependent newline handling here.
            // Thus, we check manually if the literal contains \r so that we split
            // the literal at the correct offset.
            if (pos - 1 > 0 && literal[pos - 1] == u'\r') {
                // Handle Windows line endings
                lineBreakLength = qsizetype(std::char_traits<char>::length("\r\n"));
                // Move pos to the index of '\r'
                pos = pos - 1;
            }
            lineLengths.push_back(pos - startIndex);
            // Advance the lookup index, so it won't find the same index.
            startIndex = pos + lineBreakLength;
            pos = literal.indexOf('\n'_L1, startIndex);
        }
        // Push the last line
        if (startIndex < literal.length()) {
            lineLengths.push_back(literal.length() - startIndex);
        }
        return lineLengths;
    };

    QList<QQmlJS::SourceLocation> result;
    // First token location should start from the "stringLiteral"'s
    // location in the qml document.
    QQmlJS::SourceLocation lineLoc = locationInDocument;
    for (const auto lineLength : lineLengths(stringLiteral)) {
        lineLoc.length = lineLength;
        result.push_back(lineLoc);

        // update for the next line
        lineLoc.offset += lineLoc.length + lineBreakLength;
        ++lineLoc.startLine;
        lineLoc.startColumn = 1;
    }
    return result;
}

QList<int> HighlightingUtils::encodeSemanticTokens(Highlights &highlights)
{
    QList<int> result;
    const auto highlightingTokens = highlights.highlights();
    constexpr auto tokenEncodingLength = 5;
    result.reserve(tokenEncodingLength * highlightingTokens.size());

    int prevLine = 0;
    int prevColumn = 0;

    std::for_each(highlightingTokens.constBegin(), highlightingTokens.constEnd(), [&](const auto &token) {
        Q_ASSERT(token.startLine >= prevLine);
        if (token.startLine != prevLine)
            prevColumn = 0;
        result.emplace_back(token.startLine - prevLine);
        result.emplace_back(token.startColumn - prevColumn);
        result.emplace_back(token.length);
        result.emplace_back(token.tokenType);
        result.emplace_back(token.tokenModifier);
        prevLine = token.startLine;
        prevColumn = token.startColumn;
    });

    return result;
}

void Highlights::addHighlight(const QQmlJS::SourceLocation &loc, int tokenType, int tokenModifier)
{
    if (!loc.isValid()) {
        qCDebug(semanticTokens) << "Invalid locations: Cannot add highlight to token";
        return;
    }

    if (!m_highlights.contains(loc.offset))
        m_highlights.insert(loc.offset, Token(loc, tokenType, tokenModifier));
}

void Highlights::addHighlight(const QMap<FileLocationRegion, QQmlJS::SourceLocation> &regions,
                              FileLocationRegion region, int modifier)
{
    if (!regions.contains(region)) {
        qCDebug(semanticTokens) << "Invalid region: Cannot add highlight to token";
        return;
    }

    const auto loc = regions.value(region);
    return addHighlight(loc, tokenTypeFromRegion(region), modifier);
}

QList<int> Highlights::collectTokens(const QQmlJS::Dom::DomItem &item)
{
    using namespace QQmlJS::Dom;
    HighlightingVisitor highlightDomElements(*this);
    // In QmlFile level, visitTree visits even FileLocations tree which takes quite a time to
    // finish. HighlightingFilter is added to prevent unnecessary visits.
    item.visitTree(Path(), highlightDomElements, VisitOption::Default, emptyChildrenVisitor,
                   emptyChildrenVisitor, highlightingFilter());

    return HighlightingUtils::encodeSemanticTokens(*this);
}

QT_END_NAMESPACE
