// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4functiontable_p.h"

#include <assembler/MacroAssemblerCodeRef.h>

#include <QtCore/qdebug.h>

#include <qt_windows.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

enum UnwindOpcode: UINT8
{
    UWOP_PUSH_NONVOL = 0, /* info == register number */
    UWOP_ALLOC_LARGE,     /* no info, alloc size in next 2 slots */
    UWOP_ALLOC_SMALL,     /* info == size of allocation / 8 - 1 */
    UWOP_SET_FPREG,       /* no info, FP = RSP + UNWIND_INFO.FPRegOffset*16 */
    UWOP_SAVE_NONVOL,     /* info == register number, offset in next slot */
    UWOP_SAVE_NONVOL_FAR, /* info == register number, offset in next 2 slots */
    UWOP_SAVE_XMM128 = 8, /* info == XMM reg number, offset in next slot */
    UWOP_SAVE_XMM128_FAR, /* info == XMM reg number, offset in next 2 slots */
    UWOP_PUSH_MACHFRAME   /* info == 0: no error-code, 1: error-code */
};

enum Register : UINT8
{
    RAX = 0,
    RCX,
    RDX,
    RBX,
    RSP,
    RBP,
    RSI,
    RDI,
    NONE = 15
};

struct UnwindCode
{
    UnwindCode(UINT8 offset, UnwindOpcode operation, Register info)
        : offset(offset), operation(operation), info(info)
    {}

    UINT8 offset;
    UINT8 operation: 4;
    UINT8 info:      4;
};

struct UnwindInfo
{
    UINT8 Version : 3;
    UINT8 Flags : 5;
    UINT8 SizeOfProlog;
    UINT8 CountOfUnwindCodes;
    UINT8 FrameRegister : 4;
    UINT8 FrameRegisterOffset : 4;
    UnwindCode UnwindCodes[2];
};

struct ExceptionHandlerRecord
{
    RUNTIME_FUNCTION handler;
    UnwindInfo info;
};

void generateFunctionTable(Function *, JSC::MacroAssemblerCodeRef *codeRef)
{
    ExceptionHandlerRecord *record = reinterpret_cast<ExceptionHandlerRecord *>(
                codeRef->executableMemory()->exceptionHandlerStart());

    record->info.Version             = 1;
    record->info.Flags               = 0;
    record->info.SizeOfProlog        = 4;
    record->info.CountOfUnwindCodes  = 2;
    record->info.FrameRegister       = RBP;
    record->info.FrameRegisterOffset = 0;

    // Push frame pointer
    record->info.UnwindCodes[1] = UnwindCode(1, UWOP_PUSH_NONVOL,  RBP);
    // Set frame pointer from stack pointer
    record->info.UnwindCodes[0] = UnwindCode(4,   UWOP_SET_FPREG, NONE);

    const quintptr codeStart = quintptr(codeRef->code().executableAddress());
    const quintptr codeSize = codeRef->size();

    record->handler.BeginAddress = DWORD(codeStart - quintptr(record));
    record->handler.EndAddress   = DWORD(codeStart + codeSize - quintptr(record));
    record->handler.UnwindData   = offsetof(ExceptionHandlerRecord, info);

    if (!RtlAddFunctionTable(&record->handler, 1, DWORD64(record))) {
        const unsigned int errorCode = GetLastError();
        qWarning() << "Failed to install win64 unwind hook. Error code:" << errorCode;
    }
}

void destroyFunctionTable(Function *, JSC::MacroAssemblerCodeRef *codeRef)
{
    ExceptionHandlerRecord *record = reinterpret_cast<ExceptionHandlerRecord *>(
                codeRef->executableMemory()->exceptionHandlerStart());
    if (!RtlDeleteFunctionTable(&record->handler)) {
        const unsigned int errorCode = GetLastError();
        qWarning() << "Failed to remove win64 unwind hook. Error code:" << errorCode;
    }
}

size_t exceptionHandlerSize()
{
    return sizeof(ExceptionHandlerRecord);
}

} // QV4

QT_END_NAMESPACE
