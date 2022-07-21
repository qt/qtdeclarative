// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4stackframe_p.h"
#include <private/qv4qobjectwrapper_p.h>
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
    if (!v4Function || instructionPointer <= 0)
        return -1;

    auto findLine = [](const CompiledData::CodeOffsetToLine &entry, uint offset) {
        return entry.codeOffset < offset;
    };

    const QV4::CompiledData::Function *cf = v4Function->compiledFunction;
    const uint offset = instructionPointer;
    const CompiledData::CodeOffsetToLine *lineNumbers = cf->lineNumberTable();
    const uint nLineNumbers = cf->nLineNumbers;
    const CompiledData::CodeOffsetToLine *line = std::lower_bound(lineNumbers, lineNumbers + nLineNumbers, offset, findLine) - 1;
    return line->line;
}

ReturnedValue QV4::CppStackFrame::thisObject() const
{
    if (isJSTypesFrame())
        return static_cast<const JSTypesStackFrame *>(this)->thisObject();

    Q_ASSERT(isMetaTypesFrame());
    const auto metaTypesFrame = static_cast<const MetaTypesStackFrame *>(this);
    return QObjectWrapper::wrap(metaTypesFrame->context()->engine(), metaTypesFrame->thisObject());
}
