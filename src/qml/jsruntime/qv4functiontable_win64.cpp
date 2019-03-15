/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4functiontable_p.h"

#include <assembler/MacroAssemblerCodeRef.h>

#include <QtCore/qdebug.h>

#include <windows.h>

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
                codeRef->executableMemory()->exceptionHandler());

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
                codeRef->executableMemory()->exceptionHandler());
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
