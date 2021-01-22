/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/
#include "qv4stackframe_p.h"
#include <QtCore/qstring.h>

using namespace  QV4;

QString CppStackFrame::source() const
{
    return v4Function ? v4Function->sourceFile() : QString();
}

QString CppStackFrame::function() const
{
    return v4Function ? v4Function->name()->toQString() : QString();
}

int CppStackFrame::lineNumber() const
{
    if (!v4Function)
        return -1;

    auto findLine = [](const CompiledData::CodeOffsetToLine &entry, uint offset) {
        return entry.codeOffset < offset;
    };

    const QV4::CompiledData::Function *cf = v4Function->compiledFunction;
    uint offset = instructionPointer;
    const CompiledData::CodeOffsetToLine *lineNumbers = cf->lineNumberTable();
    uint nLineNumbers = cf->nLineNumbers;
    const CompiledData::CodeOffsetToLine *line = std::lower_bound(lineNumbers, lineNumbers + nLineNumbers, offset, findLine) - 1;
    return line->line;
}

ReturnedValue CppStackFrame::thisObject() const {
    return jsFrame->thisObject.asReturnedValue();
}

