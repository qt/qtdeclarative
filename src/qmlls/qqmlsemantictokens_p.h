// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLSEMANTICTOKENS_P_H
#define QQMLSEMANTICTOKENS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtLanguageServer/private/qlanguageserverspec_p.h>
#include <QtQmlDom/private/qqmldomitem_p.h>
#include <QtCore/qlist.h>
#include <QtCore/qmap.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(semanticTokens)

// Represents a semantic highlighting token
// startLine and startColumn are 0-based as in LSP spec.
struct Token
{
    Token() = default;
    Token(const QQmlJS::SourceLocation &loc, int tokenType, int tokenModifier = 0)
        : offset(loc.offset),
          length(loc.length),
          startLine(loc.startLine - 1),
          startColumn(loc.startColumn - 1),
          tokenType(tokenType),
          tokenModifier(tokenModifier)
    {
    }

    inline friend bool operator<(const Token &lhs, const Token &rhs)
    {
        return lhs.offset < rhs.offset;
    }

    inline friend bool operator==(const Token &lhs, const Token &rhs)
    {
        return lhs.offset == rhs.offset && lhs.length == rhs.length
                && lhs.startLine == rhs.startLine && lhs.startColumn == rhs.startColumn
                && lhs.tokenType == rhs.tokenType && lhs.tokenModifier == rhs.tokenModifier;
    }

    int offset;
    int length;
    int startLine;
    int startColumn;
    int tokenType;
    int tokenModifier;
};

using HighlightsContainer = QMap<int, QT_PREPEND_NAMESPACE(Token)>;

/*!
\internal
Offsets start from zero.
*/
struct HighlightsRange
{
    int startOffset;
    int endOffset;
};

class Highlights
{
public:
    void addHighlight(const QQmlJS::SourceLocation &loc, int tokenType, int tokenModifier = 0);
    void addHighlight(const QMap<QQmlJS::Dom::FileLocationRegion, QQmlJS::SourceLocation> &regions,
                      QQmlJS::Dom::FileLocationRegion region, int tokenModifier = 0);
    HighlightsContainer &highlights() { return m_highlights; }
    const HighlightsContainer &highlights() const { return m_highlights; }

private:
    HighlightsContainer m_highlights;
};

namespace HighlightingUtils
{
    QList<int> encodeSemanticTokens(Highlights &highlights);
    QList<QQmlJS::SourceLocation>
    sourceLocationsFromMultiLineToken(QStringView code,
                                      const QQmlJS::SourceLocation &tokenLocation);
    void addModifier(QLspSpecification::SemanticTokenModifiers modifier, int *baseModifier);
    bool rangeOverlapsWithSourceLocation(const QQmlJS::SourceLocation &loc, const HighlightsRange &r);
    QList<QLspSpecification::SemanticTokensEdit> computeDiff(const QList<int> &, const QList<int> &);
    void updateResultID(QByteArray &resultID);
    QList<int> collectTokens(const QQmlJS::Dom::DomItem &item,
                             const std::optional<HighlightsRange> &range);
};

class HighlightingVisitor
{
public:
    HighlightingVisitor(Highlights &highlights, const std::optional<HighlightsRange> &range);
    bool operator()(QQmlJS::Dom::Path, const QQmlJS::Dom::DomItem &item, bool);

private:
    void highlightComment(const QQmlJS::Dom::DomItem &item);
    void highlightImport(const QQmlJS::Dom::DomItem &item);
    void highlightBinding(const QQmlJS::Dom::DomItem &item);
    void highlightPragma(const QQmlJS::Dom::DomItem &item);
    void highlightEnumItem(const QQmlJS::Dom::DomItem &item);
    void highlightEnumDecl(const QQmlJS::Dom::DomItem &item);
    void highlightQmlObject(const QQmlJS::Dom::DomItem &item);
    void highlightComponent(const QQmlJS::Dom::DomItem &item);
    void highlightPropertyDefinition(const QQmlJS::Dom::DomItem &item);
    void highlightMethod(const QQmlJS::Dom::DomItem &item);
    void highlightScriptLiteral(const QQmlJS::Dom::DomItem &item);
    void highlightIdentifier(const QQmlJS::Dom::DomItem &item);
    void highlightBySemanticAnalysis(const QQmlJS::Dom::DomItem &item, QQmlJS::SourceLocation loc);
    void highlightScriptExpressions(const QQmlJS::Dom::DomItem &item);

private:
    Highlights &m_highlights;
    std::optional<HighlightsRange> m_range;
};

QT_END_NAMESPACE

#endif // QQMLSEMANTICTOKENS_P_H
