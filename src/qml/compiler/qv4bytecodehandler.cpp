// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
