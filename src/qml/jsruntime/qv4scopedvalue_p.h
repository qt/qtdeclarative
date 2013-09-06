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
#ifndef QV4SCOPEDVALUE_P_H
#define QV4SCOPEDVALUE_P_H

#include "qv4engine_p.h"
#include "qv4value_def_p.h"

QT_BEGIN_NAMESPACE

#define SAVE_JS_STACK(ctx) Value *__jsStack = ctx->engine->jsStackTop
#define CHECK_JS_STACK(ctx) Q_ASSERT(__jsStack == ctx->engine->jsStackTop)

namespace QV4 {

struct ScopedValueArray {
    ScopedValueArray(ExecutionEngine *e, int size)
        : engine(e)
#ifndef QT_NO_DEBUG
        , size(size)
#endif
    {
        ptr = e->stackPush(size);
    }

    ~ScopedValueArray() {
#ifndef QT_NO_DEBUG
        engine->stackPop(size);
        Q_ASSERT(engine->jsStackTop == ptr);
#else
        engine->jsStackTop = ptr;
#endif
    }

    ExecutionEngine *engine;
#ifndef QT_NO_DEBUG
    int size;
#endif
    Value *ptr;
};

struct ScopedValue;

struct ValueScope {
    ValueScope(ExecutionContext *ctx)
        : engine(ctx->engine)
    {
        mark = ctx->engine->jsStackTop;
    }

    ValueScope(ExecutionEngine *e)
        : engine(e)
    {
        mark = e->jsStackTop;
    }

    ~ValueScope() {
        Q_ASSERT(engine->jsStackTop >= mark);
        engine->jsStackTop = mark;
    }

    ExecutionEngine *engine;
    Value *mark;
};

struct ScopedValue
{
    ScopedValue(const ValueScope &scope)
    {
        ptr = scope.engine->jsStackTop++;
    }

    ScopedValue(const ValueScope &scope, const Value &v)
    {
        ptr = scope.engine->jsStackTop++;
        *ptr = v;
    }

    ScopedValue &operator=(const Value &v) {
        *ptr = v;
        return *this;
    }

    ScopedValue &operator=(const ScopedValue &other) {
        *ptr = *other.ptr;
        return *this;
    }

    Value *operator->() {
        return ptr;
    }

    operator const Value &() const {
        return *ptr;
    }

    Value *ptr;
};

struct ScopedCallData {
    ScopedCallData(ExecutionEngine *e, int argc)
        : engine(e)
        // ### this check currently won't work because of exceptions
#if 0 //ndef QT_NO_DEBUG
        , size(qMax(argc, (int)QV4::Global::ReservedArgumentCount) + offsetof(QV4::CallData, args)/sizeof(QV4::Value))
#endif
    {
        ptr = reinterpret_cast<CallData *>(e->stackPush(qMax(argc, (int)QV4::Global::ReservedArgumentCount) + offsetof(QV4::CallData, args)/sizeof(QV4::Value)));
        ptr->tag = 0;
        ptr->argc = argc;
    }

    ~ScopedCallData() {
#if 0 //ndef QT_NO_DEBUG
        engine->stackPop(size);
        Q_ASSERT((void *)engine->jsStackTop == (void *)ptr);
#else
        engine->jsStackTop = reinterpret_cast<Value *>(ptr);
#endif
    }

    CallData *operator->() {
        return ptr;
    }

    operator CallData *() const {
        return ptr;
    }


    ExecutionEngine *engine;
#if 0 //ndef QT_NO_DEBUG
    int size;
#endif
    CallData *ptr;
};

}

QT_END_NAMESPACE

#endif
