// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "syntaxhighlighter.h"

#include <QRegularExpression>

SyntaxHighlighter::SyntaxHighlighter(QObject *parent)
    : QSyntaxHighlighter{parent}
{}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    // Default
    setFormat(0, text.length(), QColor{214, 207 , 154});

    // import keyword
    QRegularExpression regex{"^\\bimport\\b||^\\s*property\\b"};
    QRegularExpressionMatchIterator it = regex.globalMatchView(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), QColor{69, 198, 214});
    }

    // Type name
    regex.setPattern("\\b[A-Z][a-zA-Z0-9_]*\\b");
    it = regex.globalMatchView(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), QColor{102, 163, 52});
    }

    // Property in binding
    regex.setPattern("\\b([a-zA-Z_]*.?[a-z_][a-zA-Z0-9_]*)\\s*:");
    it = regex.globalMatchView(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), QColor{255, 106, 173});
    }

    // Text
    regex.setPattern("\"([^\"]*)\"");
    it = regex.globalMatchView(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), QColor{214, 149, 69});
    }

    // Comment
    regex.setPattern("//*([^/]*)");
    it = regex.globalMatchView(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        setFormat(match.capturedStart(), match.capturedLength(), QColor{168, 171, 176});
    }
}
