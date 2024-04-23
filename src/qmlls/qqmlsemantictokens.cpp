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
    Q_UNUSED(item);
    return false;
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
