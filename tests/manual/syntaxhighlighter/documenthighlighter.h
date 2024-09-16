// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef DOCUMENTHIGHLIGHTER_H
#define DOCUMENTHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QQuickTextDocument>
#include <QtQml>

class Highlighter : public QSyntaxHighlighter
{
public:
    Highlighter(QTextDocument *parent = nullptr, int underlineTest = 0);

    void highlightBlock(const QString &text) override;

    QRegularExpression m_testRegex;
    int m_style = 0;
};

class DocumentHighlighter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQuickTextDocument *document READ document WRITE setDocument NOTIFY documentChanged)
    Q_PROPERTY(int style READ style WRITE setStyle NOTIFY styleChanged)
    QML_ELEMENT

public:
    DocumentHighlighter(QObject *parent = nullptr);

    QQuickTextDocument *document() const { return m_document; }
    void setDocument(QQuickTextDocument * document);

    int style() const { return m_highlighter.m_style; }
    void setStyle(int style);

signals:
    void documentChanged();
    void styleChanged();

private:
    QQuickTextDocument *m_document = nullptr;
    Highlighter m_highlighter;
};

#endif // DOCUMENTHIGHLIGHTER_H
