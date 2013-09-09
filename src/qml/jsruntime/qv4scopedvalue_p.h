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

struct ScopedValue;
struct ValueRef;

struct ReturnedValue
{
    ReturnedValue(const Value &v)
        : v(v) {}
    // no destructor


private:
    friend struct ValueRef;
    friend struct ScopedValue;
    QV4::Value v;
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

    ScopedValue(const ValueScope &scope, const ReturnedValue &v)
    {
        ptr = scope.engine->jsStackTop++;
        *ptr = v.v;
    }

    ScopedValue &operator=(const Value &v) {
        *ptr = v;
        return *this;
    }

    ScopedValue &operator=(const ReturnedValue &v) {
        *ptr = v.v;
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
#ifndef QT_NO_DEBUG
        , size(qMax(argc, (int)QV4::Global::ReservedArgumentCount) + offsetof(QV4::CallData, args)/sizeof(QV4::Value))
#endif
    {
        ptr = reinterpret_cast<CallData *>(e->stackPush(qMax(argc, (int)QV4::Global::ReservedArgumentCount) + offsetof(QV4::CallData, args)/sizeof(QV4::Value)));
        ptr->tag = 0;
        ptr->argc = argc;
    }

    ~ScopedCallData() {
#ifndef QT_NO_DEBUG
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
#ifndef QT_NO_DEBUG
    int size;
#endif
    CallData *ptr;
};

struct ValueRef {
    ValueRef(const ScopedValue &v)
        : ptr(v.ptr) {}
    ValueRef(const PersistentValue &v)
        : ptr(&v.d->value) {}
    ValueRef(PersistentValuePrivate *p)
        : ptr(&p->value) {}
    // Important: Do NOT add a copy constructor to this class
    // adding a copy constructor actually changes the calling convention, ie.
    // is not even binary compatible. Adding it would break assumptions made
    // in the jit'ed code.
    ValueRef &operator=(const ScopedValue &o)
    { *ptr = *o.ptr; return *this; }
    ValueRef &operator=(const ValueRef &o)
    { *ptr = *o.ptr; return *this; }
    ValueRef &operator=(const Value &v)
    { *ptr = v; return *this; }
    ValueRef &operator=(const ReturnedValue &v) {
        *ptr = v.v;
        return *this;
    }

    operator const Value *() const {
        return ptr;
    }
    const Value *operator->() const {
        return ptr;
    }

    operator Value *() {
        return ptr;
    }
    Value *operator->() {
        return ptr;
    }

    static ValueRef fromRawValue(Value *v) {
        return ValueRef(v);
    }
    static const ValueRef fromRawValue(const Value *v) {
        return ValueRef(const_cast<Value *>(v));
    }
    // ### get rid of this one!
    ValueRef(Value *v) { ptr = v; }
private:
    Value *ptr;
};


struct CallDataRef {
    CallDataRef(const ScopedCallData &c)
        : ptr(c.ptr) {}
    CallDataRef(CallData *v) { ptr = v; }
    // Important: Do NOT add a copy constructor to this class
    // adding a copy constructor actually changes the calling convention, ie.
    // is not even binary compatible. Adding it would break assumptions made
    // in the jit'ed code.
    CallDataRef &operator=(const ScopedCallData &c)
    { *ptr = *c.ptr; return *this; }
    CallDataRef &operator=(const CallDataRef &o)
    { *ptr = *o.ptr; return *this; }

    operator const CallData *() const {
        return ptr;
    }
    const CallData *operator->() const {
        return ptr;
    }

    operator CallData *() {
        return ptr;
    }
    CallData *operator->() {
        return ptr;
    }

private:
    CallData *ptr;
};

}

QT_END_NAMESPACE

#endif
