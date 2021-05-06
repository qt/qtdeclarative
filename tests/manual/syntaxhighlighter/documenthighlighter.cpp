/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the manual tests of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
