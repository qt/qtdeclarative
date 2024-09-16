// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QQmlEngine>
#include <QSyntaxHighlighter>

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QObject *parent = nullptr);

protected:
    // QSyntaxHighlighter interface
    void highlightBlock(const QString &text) override;
};

#endif // SYNTAXHIGHLIGHTER_H
