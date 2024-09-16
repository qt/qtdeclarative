// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>

class LineNumberArea;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    CodeEditor(QWidget *parent = nullptr);

    void lineNumberAreaPaintEvent(QPaintEvent *event);
    int lineNumberAreaWidth() const;

private:
    void setupConnections();
    void resizeEvent(QResizeEvent *event) final;

private slots:
    void updateLineNumberArea(const QRect &rect, int dy);
    void updateLineNumberAreaWidth();
    void highlightCurrentLine();

private:
    LineNumberArea *m_lineNumberArea = nullptr;
};

#endif // CODEEDITOR_H
