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

struct Scope {
    Scope(ExecutionContext *ctx)
        : engine(ctx->engine)
#ifndef QT_NO_DEBUG
        , size(0)
#endif
    {
        mark = ctx->engine->jsStackTop;
    }

    Scope(ExecutionEngine *e)
        : engine(e)
    {
        mark = e->jsStackTop;
    }

    ~Scope() {
        Q_ASSERT(engine->jsStackTop >= mark);
        engine->jsStackTop = mark;
    }

    ExecutionEngine *engine;
    Value *mark;
#ifndef QT_NO_DEBUG
    mutable int size;
#endif
};

struct ScopedValue;
struct ValueRef;

struct ScopedValue
{
    ScopedValue(const Scope &scope)
    {
        ptr = scope.engine->jsStackTop++;
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    ScopedValue(const Scope &scope, const Value &v)
    {
        ptr = scope.engine->jsStackTop++;
        *ptr = v;
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    ScopedValue(const Scope &scope, const ReturnedValue &v)
    {
        ptr = scope.engine->jsStackTop++;
        ptr->val = v;
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    template<typename T>
    ScopedValue(const Scope &scope, Returned<T> *t)
    {
        ptr = scope.engine->jsStackTop++;
        *ptr = T::toValue(t->getPointer());
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    ScopedValue &operator=(const Value &v) {
        *ptr = v;
        return *this;
    }

    ScopedValue &operator=(const ReturnedValue &v) {
        ptr->val = v;
        return *this;
    }

    template<typename T>
    ScopedValue &operator=(Returned<T> *t) {
        *ptr = T::toValue(t->getPointer());
        return *this;
    }

    ScopedValue &operator=(const ScopedValue &other) {
        *ptr = *other.ptr;
        return *this;
    }

    Value *operator->() {
        return ptr;
    }

    const Value *operator->() const {
        return ptr;
    }

    operator const Value &() const {
        return *ptr;
    }

    ReturnedValue asReturnedValue() const { return ptr->val; }

    Value *ptr;
};

template<typename T>
struct Scoped
{
    Scoped(const Scope &scope)
    {
        ptr = scope.engine->jsStackTop++;
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    Scoped(const Scope &scope, const Value &v)
    {
        ptr = scope.engine->jsStackTop++;
        if (T::cast(v))
            *ptr = v;
        else
            *ptr = QV4::Value::undefinedValue();
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    Scoped(const Scope &scope, const ValueRef &v);

    Scoped(const Scope &scope, T *t)
    {
        ptr = scope.engine->jsStackTop++;
        *ptr = T::toValue(t);
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }
    template<typename X>
    Scoped(const Scope &scope, Returned<X> *x)
    {
        ptr = scope.engine->jsStackTop++;
        T *t = Returned<T>::getPointer(x);
        *ptr = T::toValue(t);
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    Scoped(const Scope &scope, const ReturnedValue &v)
    {
        ptr = scope.engine->jsStackTop++;
        if (T::cast(QV4::Value::fromReturnedValue(v)))
            ptr->val = v;
        else
            *ptr = QV4::Value::undefinedValue();
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    Scoped<T> &operator=(const Value &v) {
        if (T::cast(v))
            *ptr = v;
        else
            *ptr = QV4::Value::undefinedValue();
        return *this;
    }

    Scoped<T> &operator=(const ValueRef &v);

    Scoped<T> &operator=(const ReturnedValue &v) {
        if (T::cast(QV4::Value::fromReturnedValue(v)))
            ptr->val = v;
        else
            *ptr = QV4::Value::undefinedValue();
        return *this;
    }

    Scoped<T> &operator=(const Scoped<T> &other) {
        *ptr = *other.ptr;
        return *this;
    }

    Scoped<T> &operator=(T *t) {
        *ptr = T::toValue(t);
        return *this;
    }

    template<typename X>
    Scoped<T> &operator=(Returned<X> *x) {
        *ptr = T::toValue(Returned<T>::getPointer(x));
        return *this;
    }


    T *operator->() {
        return static_cast<T *>(ptr->managed());
    }

    const Value *operator->() const {
        return T::cast(*ptr);
    }

    bool operator!() const {
        return !ptr->managed();
    }
    operator bool() const {
        return ptr->managed();
    }

    T *getPointer() {
        return static_cast<T *>(ptr->managed());
    }

    Value asValue() const {
        return *ptr;
    }

    ReturnedValue asReturnedValue() const { return ptr->val; }

    Value *ptr;
};

struct ScopedCallData {
    ScopedCallData(Scope &scope, int argc)
    {
        int size = qMax(argc, (int)QV4::Global::ReservedArgumentCount) + offsetof(QV4::CallData, args)/sizeof(QV4::Value);
        ptr = reinterpret_cast<CallData *>(scope.engine->stackPush(size));
        ptr->tag = 0;
        ptr->argc = argc;
#ifndef QT_NO_DEBUG
        scope.size += size;
#endif
    }

    CallData *operator->() {
        return ptr;
    }

    operator CallData *() const {
        return ptr;
    }

    CallData *ptr;
};

struct ValueRef {
    ValueRef(const ScopedValue &v)
        : ptr(v.ptr) {}
    template <typename T>
    ValueRef(const Scoped<T> &v)
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
        ptr->val = v;
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

    ReturnedValue asReturnedValue() const { return ptr->val; }

    // ### get rid of this one!
    ValueRef(Value *v) { ptr = v; }
private:
    Value *ptr;
};

template<typename T>
inline Scoped<T>::Scoped(const Scope &scope, const ValueRef &v)
{
    ptr = scope.engine->jsStackTop++;
    if (T::cast(*v.operator ->()))
        *ptr = *v.operator ->();
    else
        *ptr = QV4::Value::undefinedValue();
#ifndef QT_NO_DEBUG
    ++scope.size;
#endif
}

template<typename T>
inline Scoped<T> &Scoped<T>::operator=(const ValueRef &v)
{
    if (T::cast(*v.operator ->()))
        *ptr = *v.operator ->();
    else
        *ptr = QV4::Value::undefinedValue();
    return *this;
}


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

struct Encode : private Value {
    static ReturnedValue undefined() {
        return quint64(_Undefined_Type) << Tag_Shift;
    }
    static ReturnedValue null() {
        return quint64(_Null_Type) << Tag_Shift;
    }

    Encode(bool b) {
        tag = _Boolean_Type;
        int_32 = b;
    }
    Encode(double d) {
        dbl = d;
    }
    Encode(int i) {
        tag = _Integer_Type;
        int_32 = i;
    }
    Encode(uint i) {
        if (i <= INT_MAX) {
            tag = _Integer_Type;
            int_32 = i;
        } else {
            dbl = i;
        }
    }
    Encode(ReturnedValue v) {
        val = v;
    }

    template<typename T>
    Encode(Returned<T> *t) {
        val = T::toValue(t->getPointer()).val;
    }

    operator ReturnedValue() const {
        return val;
    }
};

}

QT_END_NAMESPACE

#endif
