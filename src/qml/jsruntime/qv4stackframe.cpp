// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

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

static const CompiledData::CodeOffsetToLineAndStatement *lineAndStatement(const CppStackFrame *frame)
{
    if (!frame->v4Function || frame->instructionPointer <= 0)
        return nullptr;

    auto findLine = [](const CompiledData::CodeOffsetToLineAndStatement &entry, uint offset) {
        return entry.codeOffset < offset;
    };

    const QV4::CompiledData::Function *cf = frame->v4Function->compiledFunction;
    const uint offset = frame->instructionPointer;
    const CompiledData::CodeOffsetToLineAndStatement *lineAndStatementNumbers
            = cf->lineAndStatementNumberTable();
    const uint nLineAndStatementNumbers = cf->nLineAndStatementNumbers;
    return std::lower_bound(
                lineAndStatementNumbers, lineAndStatementNumbers + nLineAndStatementNumbers,
                offset, findLine) - 1;
}

int CppStackFrame::lineNumber() const
{
    if (auto *line = lineAndStatement(this))
        return line->line;
    return missingLineNumber();
}

int CppStackFrame::statementNumber() const
{
    if (auto *statement = lineAndStatement(this))
        return statement->statement;
    return -1;
}

int CppStackFrame::missingLineNumber() const
{
    // Remove the first bit so that we can cast to positive int and negate.
    // Remove the last bit so that it can't be -1.
    const int result = -int(quintptr(this) & 0x7ffffffe);
    Q_ASSERT(result < -1);
    return result;
}

ReturnedValue QV4::CppStackFrame::thisObject() const
{
    if (isJSTypesFrame())
        return static_cast<const JSTypesStackFrame *>(this)->thisObject();

    Q_ASSERT(isMetaTypesFrame());
    const auto metaTypesFrame = static_cast<const MetaTypesStackFrame *>(this);
    return QObjectWrapper::wrap(metaTypesFrame->context()->engine(), metaTypesFrame->thisObject());
}
