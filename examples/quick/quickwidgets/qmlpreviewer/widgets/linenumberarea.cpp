// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "linenumberarea.h"
#include "codeeditor.h"

LineNumberArea::LineNumberArea(CodeEditor *editor)
    : QWidget{editor}
    , m_codeEditor{editor}
{}

QSize LineNumberArea::sizeHint() const
{
    return QSize{m_codeEditor->lineNumberAreaWidth(), 0};
}

void LineNumberArea::paintEvent(QPaintEvent *event)
{
    m_codeEditor->lineNumberAreaPaintEvent(event);
}
