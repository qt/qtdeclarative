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

#include "qv4context_p.h"
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
        mark = engine->jsStackTop;
    }

    explicit Scope(ExecutionEngine *e)
        : engine(e)
#ifndef QT_NO_DEBUG
        , size(0)
#endif
    {
        mark = engine->jsStackTop;
    }

    ~Scope() {
#ifndef QT_NO_DEBUG
        Q_ASSERT(engine->jsStackTop >= mark);
        memset(mark, 0, (engine->jsStackTop - mark)*sizeof(SafeValue));
#endif
        engine->jsStackTop = mark;
    }

    SafeValue *alloc(int nValues) {
        SafeValue *ptr = engine->jsStackTop;
        engine->jsStackTop += nValues;
#ifndef QT_NO_DEBUG
        size += nValues;
#endif
        return ptr;
    }

    bool hasException() const {
        return engine->hasException;
    }

    ExecutionEngine *engine;
    SafeValue *mark;
#ifndef QT_NO_DEBUG
    mutable int size;
#endif

private:
    Q_DISABLE_COPY(Scope)
};

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
        *ptr = t->getPointer() ? Value::fromManaged(t->getPointer()) : Primitive::undefinedValue();
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
        *ptr = t->getPointer() ? Value::fromManaged(t->getPointer()) : Primitive::undefinedValue();
        return *this;
    }

    ScopedValue &operator=(const ScopedValue &other) {
        *ptr = *other.ptr;
        return *this;
    }

    SafeValue *operator->() {
        return ptr;
    }

    const SafeValue *operator->() const {
        return ptr;
    }

    ReturnedValue asReturnedValue() const { return ptr->val; }

    SafeValue *ptr;
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
        *ptr = p ? QV4::Value::fromManaged(p) : QV4::Primitive::undefinedValue();
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
        ptr->val = value_convert<T>(scope.engine->currentContext(), v);
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
        ptr->val = value_convert<T>(scope.engine->currentContext(), QV4::Value::fromReturnedValue(v));
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

    ReturnedValue asReturnedValue() const {
#if QT_POINTER_SIZE == 8
        return ptr->val ? ptr->val : Primitive::undefinedValue().asReturnedValue();
#else
        return ptr->val;
#endif
    }

    SafeValue *ptr;
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
    inline ReturnedValue argument(int i) {
        return i < argc ? args[i].asReturnedValue() : Primitive::undefinedValue().asReturnedValue();
    }

    SafeValue thisObject;
    SafeValue args[1];
};

struct ScopedCallData {
    ScopedCallData(Scope &scope, int argc)
    {
        int size = qMax(argc, (int)QV4::Global::ReservedArgumentCount) + qOffsetOf(QV4::CallData, args)/sizeof(QV4::SafeValue);
        ptr = reinterpret_cast<CallData *>(scope.engine->stackPush(size));
        ptr->tag = QV4::Value::Integer_Type;
        ptr->argc = argc;
#ifndef QT_NO_DEBUG
        scope.size += size;
        for (int ii = 0; ii < qMax(argc, (int)QV4::Global::ReservedArgumentCount); ++ii)
            ptr->args[ii] = QV4::Primitive::undefinedValue();
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
    template <typename T>
    ValueRef &operator=(Returned<T> *v) {
        ptr->val = v->asReturnedValue();
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
    SafeValue *operator->() {
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
    ValueRef(Value *v) { ptr = reinterpret_cast<SafeValue *>(v); }
private:
    SafeValue *ptr;
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
    {
#if QT_POINTER_SIZE == 4
        ptr->tag = Value::Managed_Type;
#endif
        ptr->m = t;
        return *this;
    }
    Referenced &operator=(Returned<T> *t) {
#if QT_POINTER_SIZE == 4
        ptr->tag = Value::Managed_Type;
#endif
        ptr->m = t->getPointer();
        return *this;
    }

    operator const T *() const {
        return ptr ? static_cast<T*>(ptr->managed()) : 0;
    }
    const T *operator->() const {
        return static_cast<T*>(ptr->managed());
    }

    operator T *() {
        return ptr ? static_cast<T*>(ptr->managed()) : 0;
    }
    T *operator->() {
        return static_cast<T*>(ptr->managed());
    }

    T *getPointer() const {
        return static_cast<T *>(ptr->managed());
    }
    ReturnedValue asReturnedValue() const { return ptr ? ptr->val : Primitive::undefinedValue().asReturnedValue(); }
    operator Returned<T> *() const { return ptr ? Returned<T>::create(getPointer()) : 0; }

    bool operator==(const Referenced<T> &other) {
        if (ptr == other.ptr)
            return true;
        return ptr && other.ptr && ptr->m == other.ptr->m;
    }
    bool operator!=(const Referenced<T> &other) {
        if (ptr == other.ptr)
            return false;
        return !ptr || ptr->m != other.ptr->m;
    }
    bool operator!() const { return !ptr || !ptr->managed(); }

    static Referenced null() { return Referenced(Null); }
    bool isNull() const { return !ptr; }
private:
    enum _Null { Null };
    Referenced(_Null) { ptr = 0; }
    SafeValue *ptr;
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

struct Encode {
    static ReturnedValue undefined() {
        return quint64(Value::Undefined_Type) << Value::Tag_Shift;
    }
    static ReturnedValue null() {
        return quint64(Value::_Null_Type) << Value::Tag_Shift;
    }

    Encode(bool b) {
        val = (quint64(Value::_Boolean_Type) << Value::Tag_Shift) | (uint)b;
    }
    Encode(double d) {
        Value v;
        v.setDouble(d);
        val = v.val;
    }
    Encode(int i) {
        val = (quint64(Value::_Integer_Type) << Value::Tag_Shift) | (uint)i;
    }
    Encode(uint i) {
        if (i <= INT_MAX) {
            val = (quint64(Value::_Integer_Type) << Value::Tag_Shift) | i;
        } else {
            Value v;
            v.setDouble(i);
            val = v.val;
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
    quint64 val;
private:
    Encode(void *);
};


template <typename T>
inline Value &Value::operator=(Returned<T> *t)
{
    val = t->getPointer()->asReturnedValue();
    return *this;
}

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

inline SafeValue &SafeValue::operator=(const ValueRef v)
{
    val = v.asReturnedValue();
    return *this;
}

template<typename T>
inline Returned<T> *SafeValue::as()
{
    return Returned<T>::create(value_cast<T>(*this));
}

template<typename T> inline
Referenced<T> SafeValue::asRef()
{
    return Referenced<T>(*this);
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
inline Returned<T> * Safe<T>::ret() const
{
    return Returned<T>::create(static_cast<T *>(managed()));
}

inline Primitive::operator ValueRef()
{
    return ValueRef(this);
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

inline ReturnedValue CallContext::argument(int i) {
    return i < callData->argc ? callData->args[i].asReturnedValue() : Primitive::undefinedValue().asReturnedValue();
}


}

QT_END_NAMESPACE

#endif
