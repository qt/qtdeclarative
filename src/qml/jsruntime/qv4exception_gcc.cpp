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

#include "qv4exception_p.h"

#include <private/qv4scopedvalue_p.h>
#include <unwind.h>
#include <cxxabi.h>
#include <bits/atomic_word.h>
#include <typeinfo>
#include <exception>

/*
 * This is a little bit hacky as it relies on the fact that exceptions are
 * reference counted in libstdc++ and that affects the layout of the standardized
 * cxa_exception, making it bigger. LLVM's libcxxabi stores the reference count
 * differently, so this here is entirely GNU libstdc++ specific.
 *
 * Eliminating this dependency is doable but requires replacing the use of C++ exceptions
 * with foreign exceptions (a different exception class) and then using __cxa_get_globals
 * to get hold of the exception inside the catch (...). AFAICS that would be portable.
 */

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

// This is what libstdc++ actually allocates
struct gcc_refcounted_compatible_exception {
    _Atomic_word refCount;
    cxa_exception x;
};

}

static void exception_cleanup(_Unwind_Reason_Code, _Unwind_Exception *ex)
{
    gcc_refcounted_compatible_exception *exception = reinterpret_cast<gcc_refcounted_compatible_exception *>(ex + 1) - 1;
    if (!--exception->refCount) {
        if (exception->x.exceptionDestructor)
            exception->x.exceptionDestructor(ex + 1);
        abi::__cxa_free_exception(ex + 1);
    }
}

static void exception_destructor(void *ex)
{
    reinterpret_cast<QV4::Exception *>(ex)->~Exception();
}

QT_BEGIN_NAMESPACE

using namespace QV4;

void Exception::throwInternal(ExecutionContext *throwingContext, const ValueRef exceptionValue)
{
    void *rawException = abi::__cxa_allocate_exception(sizeof(QV4::Exception));
    gcc_refcounted_compatible_exception *refCountedException = reinterpret_cast<gcc_refcounted_compatible_exception *>(rawException) - 1;
    cxa_exception *exception = &refCountedException->x;

    (void)new (rawException) Exception(throwingContext, exceptionValue);

    refCountedException->refCount = 1;
    exception->typeInfo = const_cast<std::type_info*>(&typeid(Exception));
    exception->exceptionDestructor = &exception_destructor;
    exception->unexpectedHandler = std::unexpected;
    exception->terminateHandler = std::terminate;
    exception->unwindHeader.exception_cleanup = &exception_cleanup;
#ifdef __ARM_EABI_UNWINDER__
    exception->unwindHeader.exception_class[0] = 'G';
    exception->unwindHeader.exception_class[1] = 'N';
    exception->unwindHeader.exception_class[2] = 'U';
    exception->unwindHeader.exception_class[3] = 'C';
    exception->unwindHeader.exception_class[4] = 'C';
    exception->unwindHeader.exception_class[5] = '+';
    exception->unwindHeader.exception_class[6] = '+';
    exception->unwindHeader.exception_class[7] = 0;
#else
    exception->unwindHeader.exception_class = 0x474e5543432b2b00; // GNUCC++0
#endif

    _Unwind_RaiseException(&exception->unwindHeader);
    abi::__cxa_begin_catch(rawException);
    std::terminate();
}

QT_END_NAMESPACE
