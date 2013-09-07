/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#if defined(_WIN32) && !(defined(WINCE) || defined(_WIN32_WCE))
#include <windows.h>
#include <DbgHelp.h>
#endif

#include "qv4stacktrace_p.h"
#include "qv4function_p.h"
#include "qv4engine_p.h"
#include "qv4unwindhelper_p.h"

#if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_MAC)
#define HAVE_GNU_BACKTRACE
#include <execinfo.h>
#endif

QT_BEGIN_NAMESPACE

using namespace QV4;

NativeStackTrace::NativeStackTrace(ExecutionContext *context)
{
    engine = context->engine;
    currentNativeFrame = 0;

#if defined(HAVE_GNU_BACKTRACE)
    UnwindHelper::prepareForUnwind(context);

    nativeFrameCount = backtrace(&trace[0], sizeof(trace) / sizeof(trace[0]));
#elif defined(Q_OS_WIN) && !defined(Q_OS_WINCE)

    int machineType = 0;

    CONTEXT winContext;
    memset(&winContext, 0, sizeof(winContext));
    winContext.ContextFlags = CONTEXT_FULL;
    RtlCaptureContext(&winContext);

    STACKFRAME64 sf64;
    memset(&sf64, 0, sizeof(sf64));

#if defined(Q_PROCESSOR_X86_32)
    machineType = IMAGE_FILE_MACHINE_I386;

    sf64.AddrFrame.Offset = winContext.Ebp;
    sf64.AddrFrame.Mode = AddrModeFlat;
    sf64.AddrPC.Offset = winContext.Eip;
    sf64.AddrPC.Mode = AddrModeFlat;
    sf64.AddrStack.Offset = winContext.Esp;
    sf64.AddrStack.Mode = AddrModeFlat;

#elif defined(Q_PROCESSOR_X86_64)
    machineType = IMAGE_FILE_MACHINE_AMD64;

    sf64.AddrFrame.Offset = winContext.Rbp;
    sf64.AddrFrame.Mode = AddrModeFlat;
    sf64.AddrPC.Offset = winContext.Rip;
    sf64.AddrPC.Mode = AddrModeFlat;
    sf64.AddrStack.Offset = winContext.Rsp;
    sf64.AddrStack.Mode = AddrModeFlat;

#else
#error "Platform unsupported!"
#endif

    nativeFrameCount = 0;

    while (StackWalk64(machineType, GetCurrentProcess(), GetCurrentThread(), &sf64, &winContext, 0, SymFunctionTableAccess64, SymGetModuleBase64, 0)) {

        if (sf64.AddrReturn.Offset == 0)
            break;

        trace[nativeFrameCount] = reinterpret_cast<void*>(sf64.AddrReturn.Offset);
        nativeFrameCount++;
        if (nativeFrameCount >= sizeof(trace) / sizeof(trace[0]))
            break;
    }

#else
    nativeFrameCount = 0;
#endif
    }

NativeFrame NativeStackTrace::nextFrame() {
    NativeFrame frame;
    frame.function = 0;
    frame.line = -1;

    for (; currentNativeFrame < nativeFrameCount && !frame.function; ++currentNativeFrame) {
        quintptr pc = reinterpret_cast<quintptr>(trace[currentNativeFrame]);
        // The pointers from the back trace point to the return address, but we are interested in
        // the caller site.
        pc = pc - 1;

        Function *f = engine->functionForProgramCounter(pc);
        if (!f)
            continue;

        frame.function = f;
        frame.line = f->lineNumberForProgramCounter(pc - reinterpret_cast<quintptr>(f->code));
    }

    return frame;
}

QT_END_NAMESPACE
