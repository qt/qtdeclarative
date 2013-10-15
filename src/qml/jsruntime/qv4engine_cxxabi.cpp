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
#include "qv4engine_p.h"

#if defined(V4_CXX_ABI_EXCEPTION)

// On arm we link libgcc statically and want to avoid exporting the _Unwind* symbols
#if defined(Q_PROCESSOR_ARM)
#define HIDE_EXPORTS
#endif

#include <unwind.h>
#include <exception>

namespace {

// 2.1.1 from http://mentorembedded.github.io/cxx-abi/abi-eh.html
struct cxa_exception {
    std::type_info *typeInfo;
    void (*exceptionDestructor)(void*);
    std::unexpected_handler unexpectedHandler;
    std::terminate_handler terminateHandler;
    cxa_exception *nextException;
    int handlerCount;
#ifdef __ARM_EABI_UNWINDER__
    cxa_exception *nextPropagatingException;
    int propagationCount;
#else
    int handlerSwitchValue;
    const char *actionRecord;
    const char *languageSpecificData;
    void *catchTemp;
    void *adjustedPtr;
#endif
    _Unwind_Exception unwindHeader;
};

struct cxa_eh_globals
{
    cxa_exception *caughtExceptions;
    unsigned int uncaughtExceptions;
#ifdef __ARM_EABI_UNWINDER__
    cxa_exception* propagatingExceptions;
#endif
};

}

extern "C" cxa_eh_globals *__cxa_get_globals();

static void exception_cleanup(_Unwind_Reason_Code, _Unwind_Exception *ex)
{
    free(ex);
}

QT_BEGIN_NAMESPACE

using namespace QV4;

void ExecutionEngine::throwInternal()
{
    _Unwind_Exception *exception = (_Unwind_Exception*)malloc(sizeof(_Unwind_Exception));
    memset(exception, 0, sizeof(*exception));
    exception->exception_cleanup = &exception_cleanup;

#ifdef __ARM_EABI_UNWINDER__
    exception->exception_class[0] = 'Q';
    exception->exception_class[1] = 'M';
    exception->exception_class[2] = 'L';
    exception->exception_class[3] = 'J';
    exception->exception_class[4] = 'S';
    exception->exception_class[5] = 'V';
    exception->exception_class[6] = '4';
    exception->exception_class[7] = 0;
#else
    exception->exception_class = 0x514d4c4a53563400; // QMLJSV40
#endif

    _Unwind_RaiseException(exception);
    std::terminate();
}

void ExecutionEngine::rethrowInternal()
{
    cxa_eh_globals *globals = __cxa_get_globals();
    cxa_exception *exception = globals->caughtExceptions;

    // Make sure we only re-throw our foreign exceptions. For general re-throw
    // we'd need different code.
#ifndef __ARM_EABI_UNWINDER__
    Q_ASSERT(exception->unwindHeader.exception_class == 0x514d4c4a53563400); // QMLJSV40
#endif

    globals->caughtExceptions = 0;
    _Unwind_RaiseException(&exception->unwindHeader);
    std::terminate();
}

QT_END_NAMESPACE

/*
 * We override these EABI defined symbols on Android, where we must statically link in the unwinder from libgcc.a
 * and thus also ensure that compiler generated cleanup code / landing pads end up calling these stubs, that
 * ultimately return control to our copy of the unwinder. The symbols are also exported from gnustl_shared, which
 * comes later in the link line.
 */
#if defined(__ANDROID__) && defined(__ARM_EABI_UNWINDER__)
#pragma GCC visibility push(default)
#ifdef __thumb__
asm ("  .pushsection .text.__cxa_end_cleanup\n"
"       .global __cxa_end_cleanup\n"
"       .type __cxa_end_cleanup, \"function\"\n"
"       .thumb_func\n"
"__cxa_end_cleanup:\n"
"       push\t{r1, r2, r3, r4}\n"
"       bl\t__gnu_end_cleanup\n"
"       pop\t{r1, r2, r3, r4}\n"
"       bl\t_Unwind_Resume @ Never returns\n"
"       .popsection\n");
#else
asm ("  .pushsection .text.__cxa_end_cleanup\n"
"       .global __cxa_end_cleanup\n"
"       .type __cxa_end_cleanup, \"function\"\n"
"__cxa_end_cleanup:\n"
"       stmfd\tsp!, {r1, r2, r3, r4}\n"
"       bl\t__gnu_end_cleanup\n"
"       ldmfd\tsp!, {r1, r2, r3, r4}\n"
"       bl\t_Unwind_Resume @ Never returns\n"
"       .popsection\n");
#endif
#pragma GCC visibility pop
#endif

#endif
