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
#include "qv4errorobject_p.h"
#include "qv4debugging_p.h"
#include "qv4unwindhelper_p.h"

#include <wtf/Platform.h>

#if USE(LIBUNWIND_DEBUG)
#include <libunwind.h>
#include <execinfo.h>
#endif

QT_BEGIN_NAMESPACE

using namespace QV4;


void Exception::throwException(ExecutionContext *context, const Value &value)
{
    if (context->engine->debugger)
        context->engine->debugger->aboutToThrow(value);

    UnwindHelper::prepareForUnwind(context);

#if USE(LIBUNWIND_DEBUG)
    printf("about to throw exception. walking stack first with libunwind:\n");
    unw_cursor_t cursor; unw_context_t uc;
    unw_word_t ip, sp;

    unw_getcontext(&uc);
    unw_init_local(&cursor, &uc);
    while (unw_step(&cursor) > 0) {
        unw_get_reg(&cursor, UNW_REG_IP, &ip);
        unw_get_reg(&cursor, UNW_REG_SP, &sp);
        printf("ip = %lx, sp = %lx ", (long) ip, (long) sp);
        void * const addr = (void*)ip;
        char **symbol = backtrace_symbols(&addr, 1);
        printf("%s", symbol[0]);
        free(symbol);
        printf("\n");
    }
    printf("stack walked. throwing exception now...\n");
#endif

    throwInternal(context, value);
}

Exception::Exception(ExecutionContext *throwingContext, const Value &exceptionValue)
    : e(throwingContext->engine)
{
    e->exceptionValue = exceptionValue;
    this->throwingContext = throwingContext->engine->current;
    accepted = false;
    if (ErrorObject *error = exceptionValue.asErrorObject())
        m_stackTrace = error->stackTrace;
    else
        m_stackTrace = throwingContext->engine->stackTrace();
}

Exception::~Exception()
{
    assert(accepted);
    e->exceptionValue = Value::undefinedValue();
}

void Exception::accept(ExecutionContext *catchingContext)
{
    assert(!accepted);
    accepted = true;
    partiallyUnwindContext(catchingContext);
}

void Exception::partiallyUnwindContext(ExecutionContext *catchingContext)
{
    if (!throwingContext)
        return;
    ExecutionContext *context = throwingContext;
    while (context != catchingContext)
        context = context->engine->popContext();
    throwingContext = context;
}

#if !defined(V4_CXX_ABI_EXCEPTION)
void Exception::throwInternal(ExecutionContext *throwingContext, const Value &exceptionValue)
{
    throw Exception(throwingContext, exceptionValue);
}
#endif

QT_END_NAMESPACE
