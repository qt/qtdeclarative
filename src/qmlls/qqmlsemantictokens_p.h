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

class Highlights
{
public:
    using HighlightsContainer = QMap<int, Token>;
    void addHighlight(const QQmlJS::SourceLocation &loc, int tokenType, int tokenModifier = 0);
    void addHighlight(const QMap<QQmlJS::Dom::FileLocationRegion, QQmlJS::SourceLocation> &regions,
                      QQmlJS::Dom::FileLocationRegion region, int tokenModifier = 0);
    QList<int> collectTokens(const QQmlJS::Dom::DomItem &item);
    HighlightsContainer &highlights() { return m_highlights; }
    const HighlightsContainer &highlights() const { return m_highlights; }

private:
    HighlightsContainer m_highlights;
};

struct HighlightingUtils
{
    static QList<int> encodeSemanticTokens(Highlights &highlights);
    static QList<QQmlJS::SourceLocation>
    sourceLocationsFromMultiLineToken(QStringView code,
                                      const QQmlJS::SourceLocation &tokenLocation);
};

class HighlightingVisitor
{
public:
    using HighlightsContainer = QMap<int, Token>;
    HighlightingVisitor(Highlights &highlights) : m_highlights(highlights) { }
    bool operator()(QQmlJS::Dom::Path, const QQmlJS::Dom::DomItem &item, bool);

private:
    void highlightComment(const QQmlJS::Dom::DomItem &item);
    void highlightImport(const QQmlJS::Dom::DomItem &item);
    void highlightBinding(const QQmlJS::Dom::DomItem &item);

private:
    Highlights &m_highlights;
};

QT_END_NAMESPACE

#endif // QQMLSEMANTICTOKENS_P_H
