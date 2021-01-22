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

#include <private/qv4bytecodehandler_p.h>

QT_USE_NAMESPACE
using namespace QV4;
using namespace Moth;

ByteCodeHandler::~ByteCodeHandler()
{
}

#define DISPATCH_INSTRUCTION(name, nargs, ...) \
    generate_##name( \
        __VA_ARGS__ \
    );

#define DECODE_AND_DISPATCH(instr) \
    { \
        INSTR_##instr(MOTH_DECODE_WITH_BASE) \
        Q_UNUSED(base_ptr); \
        _currentOffset = _nextOffset; \
        _nextOffset = code - start; \
        if (startInstruction(Instr::Type::instr) == ProcessInstruction) { \
            INSTR_##instr(DISPATCH) \
            endInstruction(Instr::Type::instr); \
        } \
        continue; \
    }

void ByteCodeHandler::decode(const char *code, uint len)
{
    MOTH_JUMP_TABLE;

    const char *start = code;
    const char *end = code + len;
    while (code < end) {
        MOTH_DISPATCH()

        FOR_EACH_MOTH_INSTR(DECODE_AND_DISPATCH)
    }
}

#undef DECODE_AND_DISPATCH
#undef DISPATCH_INSTRUCTION
