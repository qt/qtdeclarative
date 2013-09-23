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

struct ScopedValue;

struct Scope {
    explicit Scope(ExecutionContext *ctx)
        : engine(ctx->engine)
#ifndef QT_NO_DEBUG
        , size(0)
#endif
    {
        mark = ctx->engine->jsStackTop;
    }

    explicit Scope(ExecutionEngine *e)
        : engine(e)
    {
        mark = e->jsStackTop;
    }

    ~Scope() {
#ifndef QT_NO_DEBUG
        Q_ASSERT(engine->jsStackTop >= mark);
        memset(mark, 0, (engine->jsStackTop - mark)*sizeof(Value));
#endif
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

    ScopedValue(const Scope &scope, Managed *m)
    {
        ptr = scope.engine->jsStackTop++;
        ptr->val = m->asReturnedValue();
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
        *ptr = t->getPointer() ? Value::fromManaged(t->getPointer()) : Value::undefinedValue();
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    ScopedValue &operator=(const Value &v) {
        *ptr = v;
        return *this;
    }

    ScopedValue &operator=(Managed *m) {
        ptr->val = m->asReturnedValue();
        return *this;
    }

    ScopedValue &operator=(const ReturnedValue &v) {
        ptr->val = v;
        return *this;
    }

    template<typename T>
    ScopedValue &operator=(Returned<T> *t) {
        *ptr = t->getPointer() ? Value::fromManaged(t->getPointer()) : Value::undefinedValue();
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
    enum _Convert { Convert };
    enum _Cast { Cast };

    inline void setPointer(Managed *p) {
#if QT_POINTER_SIZE == 8
        ptr->val = (quint64)p;
#else
        *ptr = p ? QV4::Value::fromManaged(p) : QV4::Value::undefinedValue();
#endif
    }

    Scoped(const Scope &scope)
    {
        ptr = scope.engine->jsStackTop++;
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    // ### GC FIX casting below to be safe
    Scoped(const Scope &scope, const Value &v)
    {
        ptr = scope.engine->jsStackTop++;
        setPointer(value_cast<T>(v));
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }
    Scoped(const Scope &scope, const ScopedValue &v)
    {
        ptr = scope.engine->jsStackTop++;
        setPointer(value_cast<T>(*v.ptr));
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    Scoped(const Scope &scope, const Value &v, _Convert)
    {
        ptr = scope.engine->jsStackTop++;
        ptr->val = value_convert<T>(scope.engine->current, v);
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    Scoped(const Scope &scope, const ValueRef &v);

    Scoped(const Scope &scope, T *t)
    {
        ptr = scope.engine->jsStackTop++;
        setPointer(t);
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }
    template<typename X>
    Scoped(const Scope &scope, X *t, _Cast)
    {
        ptr = scope.engine->jsStackTop++;
        setPointer(managed_cast<T>(t));
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    template<typename X>
    Scoped(const Scope &scope, Returned<X> *x)
    {
        ptr = scope.engine->jsStackTop++;
        setPointer(Returned<T>::getPointer(x));
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    Scoped(const Scope &scope, const ReturnedValue &v)
    {
        ptr = scope.engine->jsStackTop++;
        setPointer(value_cast<T>(QV4::Value::fromReturnedValue(v)));
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }
    Scoped(const Scope &scope, const ReturnedValue &v, _Convert)
    {
        ptr = scope.engine->jsStackTop++;
        ptr->val = value_convert<T>(scope.engine->current, QV4::Value::fromReturnedValue(v));
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    Scoped<T> &operator=(const Value &v) {
        setPointer(value_cast<T>(v));
        return *this;
    }

    Scoped<T> &operator=(const ValueRef &v);

    Scoped<T> &operator=(const ReturnedValue &v) {
        setPointer(value_cast<T>(QV4::Value::fromReturnedValue(v)));
        return *this;
    }

    Scoped<T> &operator=(const Scoped<T> &other) {
        *ptr = *other.ptr;
        return *this;
    }

    Scoped<T> &operator=(T *t) {
        setPointer(t);
        return *this;
    }

    template<typename X>
    Scoped<T> &operator=(Returned<X> *x) {
        setPointer(Returned<T>::getPointer(x));
        return *this;
    }


    T *operator->() {
        return static_cast<T *>(ptr->managed());
    }

    bool operator!() const {
        return !ptr->managed();
    }
    operator void *() const {
        return ptr->managed();
    }

    T *getPointer() {
        return static_cast<T *>(ptr->managed());
    }

    Value asValue() const {
#if QT_POINTER_SIZE == 8
        return ptr->val ? *ptr : QV4::Value::undefinedValue();
#else
        return *ptr;
#endif
    }

    ReturnedValue asReturnedValue() const {
#if QT_POINTER_SIZE == 8
        return ptr->val ? ptr->val : Value::undefinedValue().asReturnedValue();
#else
        return ptr->val;
#endif
    }

    Value *ptr;
};

struct CallData
{
    // below is to be compatible with Value. Initialize tag to 0
#if Q_BYTE_ORDER != Q_LITTLE_ENDIAN
    uint tag;
#endif
    int argc;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    uint tag;
#endif

    SafeValue thisObject;
    SafeValue args[1];
};

struct ScopedCallData {
    ScopedCallData(Scope &scope, int argc)
    {
        int size = qMax(argc, (int)QV4::Global::ReservedArgumentCount) + qOffsetOf(QV4::CallData, args)/sizeof(QV4::Value);
        ptr = reinterpret_cast<CallData *>(scope.engine->stackPush(size));
        ptr->tag = QV4::Value::Integer_Type;
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
    ValueRef(SafeValue &v) { ptr = &v; }
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
struct Referenced {
    // Important: Do NOT add a copy constructor to this class
    // adding a copy constructor actually changes the calling convention, ie.
    // is not even binary compatible. Adding it would break assumptions made
    // in the jit'ed code.
    Referenced(const Scoped<T> &v)
        : ptr(v.ptr) {}
    Referenced(Safe<T> &v) { ptr = &v; }
    Referenced(SafeValue &v) {
        ptr = value_cast<T>(v) ? &v : 0;
    }

    Referenced &operator=(const Referenced &o)
    { *ptr = *o.ptr; return *this; }
    Referenced &operator=(T *t)
    { ptr->val = t->asReturnedValue(); return *this; }
    Referenced &operator=(const Returned<T> *t) {
        ptr->val = t->getPointer()->asReturnedValue();
        return *this;
    }

    operator const T *() const {
        return static_cast<T*>(ptr->managed());
    }
    const T *operator->() const {
        return static_cast<T*>(ptr->managed());
    }

    operator T *() {
        return static_cast<T*>(ptr->managed());
    }
    T *operator->() {
        return static_cast<T*>(ptr->managed());
    }

    T *getPointer() const {
        return static_cast<T *>(ptr->managed());
    }
    ReturnedValue asReturnedValue() const { return ptr ? ptr->val : Value::undefinedValue().asReturnedValue(); }

    static Referenced null() { return Referenced(Null); }
    bool isNull() const { return !ptr; }
private:
    enum _Null { Null };
    Referenced(_Null) { ptr = 0; }
    Value *ptr;
};

typedef Referenced<String> StringRef;
typedef Referenced<Object> ObjectRef;
typedef Referenced<FunctionObject> FunctionObjectRef;

template<typename T>
inline Scoped<T>::Scoped(const Scope &scope, const ValueRef &v)
{
    ptr = scope.engine->jsStackTop++;
    setPointer(value_cast<T>(*v.operator ->()));
#ifndef QT_NO_DEBUG
    ++scope.size;
#endif
}

template<typename T>
inline Scoped<T> &Scoped<T>::operator=(const ValueRef &v)
{
    setPointer(value_cast<T>(*v.operator ->()));
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
        return quint64(Undefined_Type) << Tag_Shift;
    }
    static ReturnedValue null() {
        return quint64(_Null_Type) << Tag_Shift;
    }

    Encode(bool b) {
        tag = _Boolean_Type;
        int_32 = b;
    }
    Encode(double d) {
        setDouble(d);
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
            setDouble(i);
        }
    }
    Encode(ReturnedValue v) {
        val = v;
    }

    template<typename T>
    Encode(Returned<T> *t) {
        val = t->getPointer()->asReturnedValue();
    }

    operator ReturnedValue() const {
        return val;
    }
};

inline SafeValue &SafeValue::operator =(const ScopedValue &v)
{
    val = v.ptr->val;
    return *this;
}

template<typename T>
inline SafeValue &SafeValue::operator=(Returned<T> *t)
{
    val = t->getPointer()->asReturnedValue();
    return *this;
}

template<typename T>
inline SafeValue &SafeValue::operator=(const Scoped<T> &t)
{
    val = t.ptr->val;
    return *this;
}

template<typename T>
inline Returned<T> *SafeValue::as()
{
    return Returned<T>::create(value_cast<T>(*this));
}

template<typename T>
inline Safe<T> &Safe<T>::operator =(T *t)
{
    val = t->asReturnedValue();
    return *this;
}

template<typename T>
inline Safe<T> &Safe<T>::operator =(const Scoped<T> &v)
{
    val = v.ptr->val;
    return *this;
}

template<typename T>
inline Safe<T> &Safe<T>::operator=(Returned<T> *t)
{
    val = t->getPointer()->asReturnedValue();
    return *this;
}

template<typename T>
inline Safe<T> &Safe<T>::operator =(const Referenced<T> &v)
{
    val = v.asReturnedValue();
    return *this;
}

template<typename T>
inline Safe<T> &Safe<T>::operator=(const Safe<T> &t)
{
    val = t.val;
    return *this;
}

template<typename T>
inline Safe<T>::operator Returned<T> *()
{
    return Returned<T>::create(static_cast<T *>(managed()));
}


template<typename T>
PersistentValue::PersistentValue(Returned<T> *obj)
    : d(new PersistentValuePrivate(QV4::Value::fromManaged(obj->getPointer())))
{
}

template<typename T>
inline PersistentValue::PersistentValue(const Referenced<T> obj)
    : d(new PersistentValuePrivate(*obj.ptr))
{
}

template<typename T>
inline PersistentValue &PersistentValue::operator=(Returned<T> *obj)
{
    return operator=(QV4::Value::fromManaged(obj->getPointer()).asReturnedValue());
}

template<typename T>
inline PersistentValue &PersistentValue::operator=(const Referenced<T> obj)
{
    return operator=(*obj.ptr);
}


template<typename T>
inline WeakValue::WeakValue(Returned<T> *obj)
    : d(new PersistentValuePrivate(QV4::Value::fromManaged(obj->getPointer()), /*engine*/0, /*weak*/true))
{
}

template<typename T>
inline WeakValue &WeakValue::operator=(Returned<T> *obj)
{
    return operator=(QV4::Value::fromManaged(obj->getPointer()).asReturnedValue());
}


}

QT_END_NAMESPACE

#endif
