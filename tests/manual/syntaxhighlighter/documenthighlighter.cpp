// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "documenthighlighter.h"
#include <QtQuick/QQuickTextDocument>

Highlighter::Highlighter(QTextDocument *parent, int underlineTest)
    : QSyntaxHighlighter(parent)
{
    m_testRegex.setPattern("char");
}

void Highlighter::highlightBlock(const QString &text)
{
    QTextCharFormat fmt;
    if (m_style > 0)
        fmt.setForeground(Qt::darkBlue);
    fmt.setFontWeight(QFont::Bold);
    switch (m_style) {
    case 1:
        fmt.setFontUnderline(true);
        fmt.setUnderlineColor(QColor("cyan"));
        break;
    case 2:
        fmt.setFontUnderline(true);
        fmt.setUnderlineColor(QColor("red"));
        // fmt.setUnderlineStyle(QTextCharFormat::WaveUnderline); // not supported yet: QTBUG-39617
        break;
    case 3:
        fmt.setFontOverline(true);
        fmt.setFontUnderline(true);
        fmt.setUnderlineColor(QColor("green"));
        fmt.setBackground(QColor("lightgreen"));
        fmt.setForeground(Qt::magenta);
        fmt.setFontItalic(true);
        fmt.setFontStretch(200);
        fmt.setFontPointSize(14);
        fmt.setFontStyleHint(QFont::Decorative); // seems ignored in practice
        break;
    case 4:
        fmt.setFontStrikeOut(true);
        fmt.setUnderlineColor(QColor("orange"));
        fmt.setFontCapitalization(QFont::Capitalization::SmallCaps);
        break;
    }

    QRegularExpressionMatchIterator matchIterator = m_testRegex.globalMatch(text);
    while (matchIterator.hasNext()) {
        QRegularExpressionMatch match = matchIterator.next();
        setFormat(match.capturedStart(), match.capturedLength(), fmt);
    }
}

DocumentHighlighter::DocumentHighlighter(QObject *parent)
    : QObject(parent) {}

void DocumentHighlighter::setDocument(QQuickTextDocument *document)
{
    if (m_document == document)
        return;

    m_document = document;
    m_highlighter.setDocument(m_document->textDocument());
    m_highlighter.rehighlight();

    emit documentChanged();
}

void DocumentHighlighter::setStyle(int style)
{
    if (m_highlighter.m_style == style)
        return;

    m_highlighter.m_style = style;
    m_highlighter.rehighlight();
    emit styleChanged();
}
