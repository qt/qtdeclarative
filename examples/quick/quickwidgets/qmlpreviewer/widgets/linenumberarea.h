// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include <QWidget>

class CodeEditor;

class LineNumberArea : public QWidget
{
    Q_OBJECT

public:
    explicit LineNumberArea(CodeEditor *editor);

    QSize sizeHint() const final;

private:
    void paintEvent(QPaintEvent *event) final;

private:
    CodeEditor *m_codeEditor = nullptr;
};

#endif // LINENUMBERAREA_H
