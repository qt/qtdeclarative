/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef QV4SCOPEDVALUE_P_H
#define QV4SCOPEDVALUE_P_H

#include "qv4engine_p.h"
#include "qv4value_p.h"
#include "qv4persistent_p.h"
#include "qv4property_p.h"

QT_BEGIN_NAMESPACE

#define SAVE_JS_STACK(ctx) Value *__jsStack = ctx->engine->jsStackTop
#define CHECK_JS_STACK(ctx) Q_ASSERT(__jsStack == ctx->engine->jsStackTop)

namespace QV4 {

struct ScopedValue;

struct Scope {
    inline Scope(ExecutionContext *ctx)
        : engine(ctx->d()->engine)
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
        memset(mark, 0, (engine->jsStackTop - mark)*sizeof(Value));
#endif
        engine->jsStackTop = mark;
    }

    Value *alloc(int nValues) {
        Value *ptr = engine->jsStackTop;
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
    Value *mark;
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

    ScopedValue(const Scope &scope, Heap::Base *o)
    {
        ptr = scope.engine->jsStackTop++;
        ptr->m = o;
#if QT_POINTER_SIZE == 4
        ptr->tag = QV4::Value::Managed_Type;
#endif
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

    ScopedValue &operator=(const Value &v) {
        *ptr = v;
        return *this;
    }

    ScopedValue &operator=(Heap::Base *o) {
        ptr->m = o;
#if QT_POINTER_SIZE == 4
        ptr->tag = QV4::Value::Managed_Type;
#endif
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
        ptr->val = (quint64)(p ? &p->data : 0);
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
    Scoped(const Scope &scope, Heap::Base *o)
    {
        Value v;
        v.m = o;
#if QT_POINTER_SIZE == 4
        v.tag = QV4::Value::Managed_Type;
#endif
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
        ptr->val = value_convert<T>(scope.engine, v);
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
    Scoped(const Scope &scope, typename T::Data *t)
    {
        ptr = scope.engine->jsStackTop++;
        *ptr = t;
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
        ptr->val = value_convert<T>(scope.engine, QV4::Value::fromReturnedValue(v));
#ifndef QT_NO_DEBUG
        ++scope.size;
#endif
    }

    Scoped<T> &operator=(Heap::Base *o) {
        Value v;
        v.m = o;
#if QT_POINTER_SIZE == 4
        v.tag = QV4::Value::Managed_Type;
#endif
        setPointer(value_cast<T>(v));
        return *this;
    }
    Scoped<T> &operator=(typename T::Data *t) {
        *ptr = t;
        return *this;
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

    operator T *() {
        return static_cast<T *>(ptr->managed());
    }

    T *operator->() {
        return ptr->cast<T>();
    }

    bool operator!() const {
        return !ptr->m;
    }
    operator void *() const {
        return ptr->m;
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

    Value *ptr;
};

struct ScopedCallData {
    ScopedCallData(Scope &scope, int argc = 0)
    {
        int size = qMax(argc, (int)QV4::Global::ReservedArgumentCount) + qOffsetOf(QV4::CallData, args)/sizeof(QV4::Value);
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

inline Value &Value::operator =(const ScopedValue &v)
{
    val = v.ptr->val;
    return *this;
}

template<typename T>
inline Value &Value::operator=(const Scoped<T> &t)
{
    val = t.ptr->val;
    return *this;
}

inline Value &Value::operator=(const ValueRef v)
{
    val = v.asReturnedValue();
    return *this;
}

template<typename T>
inline TypedValue<T> &TypedValue<T>::operator =(T *t)
{
    m = t ? &t->data : 0;
#if QT_POINTER_SIZE == 4
    tag = Managed_Type;
#endif
    return *this;
}

template<typename T>
inline TypedValue<T> &TypedValue<T>::operator =(const Scoped<T> &v)
{
    m = v.ptr->m;
#if QT_POINTER_SIZE == 4
    tag = Managed_Type;
#endif
    return *this;
}

//template<typename T>
//inline TypedValue<T> &TypedValue<T>::operator =(const ManagedRef<T> &v)
//{
//    val = v.asReturnedValue();
//    return *this;
//}

template<typename T>
inline TypedValue<T> &TypedValue<T>::operator=(const TypedValue<T> &t)
{
    val = t.val;
    return *this;
}

inline PersistentValue &PersistentValue::operator=(const ScopedValue &other)
{
    return operator=(other.asReturnedValue());
}

inline ValueRef::ValueRef(const ScopedValue &v)
    : ptr(v.ptr)
{}

template <typename T>
inline ValueRef::ValueRef(const Scoped<T> &v)
    : ptr(v.ptr)
{}

inline ValueRef::ValueRef(const PersistentValue &v)
    : ptr(&v.d->value)
{}

inline ValueRef::ValueRef(PersistentValuePrivate *p)
    : ptr(&p->value)
{}

inline ValueRef &ValueRef::operator=(const ScopedValue &o)
{
    *ptr = *o.ptr;
    return *this;
}

struct ScopedProperty
{
    ScopedProperty(Scope &scope)
    {
        property = reinterpret_cast<Property*>(scope.alloc(sizeof(Property) / sizeof(Value)));
    }

    Property *operator->() { return property; }
    operator const Property &() { return *property; }

    Property *property;
};

}

QT_END_NAMESPACE

#endif
