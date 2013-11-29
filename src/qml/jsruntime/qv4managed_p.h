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
#ifndef QMLJS_MANAGED_H
#define QMLJS_MANAGED_H

#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtCore/QDebug>
#include "qv4global_p.h"
#include "qv4value_def_p.h"
#include "qv4internalclass_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

#define Q_MANAGED_CHECK \
    template <typename T> inline void qt_check_for_QMANAGED_macro(const T &_q_argument) const \
    { int i = qYouForgotTheQ_MANAGED_Macro(this, &_q_argument); i = i + 1; }

template <typename T>
inline int qYouForgotTheQ_MANAGED_Macro(T, T) { return 0; }

template <typename T1, typename T2>
inline void qYouForgotTheQ_MANAGED_Macro(T1, T2) {}

#define Q_MANAGED \
    public: \
        Q_MANAGED_CHECK \
        static const QV4::ManagedVTable static_vtbl; \
        template <typename T> \
        QV4::Returned<T> *asReturned() { return QV4::Returned<T>::create(this); } \


struct GCDeletable
{
    GCDeletable() : next(0), lastCall(false) {}
    virtual ~GCDeletable() {}
    GCDeletable *next;
    bool lastCall;
};

struct ManagedVTable
{
    ReturnedValue (*call)(Managed *, CallData *data);
    ReturnedValue (*construct)(Managed *, CallData *data);
    void (*markObjects)(Managed *, ExecutionEngine *e);
    void (*destroy)(Managed *);
    void (*collectDeletables)(Managed *, GCDeletable **deletable);
    ReturnedValue (*get)(Managed *, const StringRef name, bool *hasProperty);
    ReturnedValue (*getIndexed)(Managed *, uint index, bool *hasProperty);
    void (*put)(Managed *, const StringRef name, const ValueRef value);
    void (*putIndexed)(Managed *, uint index, const ValueRef value);
    PropertyAttributes (*query)(const Managed *, StringRef name);
    PropertyAttributes (*queryIndexed)(const Managed *, uint index);
    bool (*deleteProperty)(Managed *m, const StringRef name);
    bool (*deleteIndexedProperty)(Managed *m, uint index);
    ReturnedValue (*getLookup)(Managed *m, Lookup *l);
    void (*setLookup)(Managed *m, Lookup *l, const ValueRef v);
    bool (*isEqualTo)(Managed *m, Managed *other);
    Property *(*advanceIterator)(Managed *m, ObjectIterator *it, StringRef name, uint *index, PropertyAttributes *attributes);
    const char *className;
};

#define DEFINE_MANAGED_VTABLE(classname) \
const QV4::ManagedVTable classname::static_vtbl =    \
{                                               \
    call,                                       \
    construct,                                  \
    markObjects,                                \
    destroy,                                    \
    0,                                          \
    get,                                        \
    getIndexed,                                 \
    put,                                        \
    putIndexed,                                 \
    query,                                      \
    queryIndexed,                               \
    deleteProperty,                             \
    deleteIndexedProperty,                      \
    getLookup,                                  \
    setLookup,                                  \
    isEqualTo,                                  \
    advanceIterator,                            \
    #classname                                  \
}

#define DEFINE_MANAGED_VTABLE_WITH_DELETABLES(classname) \
const QV4::ManagedVTable classname::static_vtbl =    \
{                                               \
    call,                                       \
    construct,                                  \
    markObjects,                                \
    destroy,                                    \
    collectDeletables,                          \
    get,                                        \
    getIndexed,                                 \
    put,                                        \
    putIndexed,                                 \
    query,                                      \
    queryIndexed,                               \
    deleteProperty,                             \
    deleteIndexedProperty,                      \
    getLookup,                                  \
    setLookup,                                  \
    isEqualTo,                                  \
    advanceIterator,                            \
    #classname                                  \
}

struct Q_QML_EXPORT Managed
{
    Q_MANAGED
private:
    void *operator new(size_t);
    Managed(const Managed &other);
    void operator = (const Managed &other);

protected:
    Managed(InternalClass *internal)
        : internalClass(internal), _data(0)
    {
        Q_ASSERT(!internalClass || internalClass->vtable);
        inUse = 1; extensible = 1;
    }

public:
    void *operator new(size_t size, MemoryManager *mm);
    void *operator new(size_t, Managed *m) { return m; }
    void operator delete(void *ptr);
    void operator delete(void *ptr, MemoryManager *mm);

    inline void mark(QV4::ExecutionEngine *engine);

    enum Type {
        Type_Invalid,
        Type_String,
        Type_Object,
        Type_ArrayObject,
        Type_FunctionObject,
        Type_BooleanObject,
        Type_NumberObject,
        Type_StringObject,
        Type_DateObject,
        Type_RegExpObject,
        Type_ErrorObject,
        Type_ArgumentsObject,
        Type_JSONObject,
        Type_MathObject,
        Type_ForeachIteratorObject,
        Type_RegExp,

        Type_QmlSequence
    };

    ExecutionEngine *engine() const;

    template <typename T>
    T *as() {
        // ### FIXME:
        if (!this || !internalClass)
            return 0;
#if !defined(QT_NO_QOBJECT_CHECK)
        reinterpret_cast<T *>(this)->qt_check_for_QMANAGED_macro(*reinterpret_cast<T *>(this));
#endif
        return internalClass->vtable == &T::static_vtbl ? static_cast<T *>(this) : 0;
    }
    template <typename T>
    const T *as() const {
        // ### FIXME:
        if (!this)
            return 0;
#if !defined(QT_NO_QOBJECT_CHECK)
        reinterpret_cast<T *>(this)->qt_check_for_QMANAGED_macro(*reinterpret_cast<T *>(const_cast<Managed *>(this)));
#endif
        return internalClass->vtable == &T::static_vtbl ? static_cast<const T *>(this) : 0;
    }

    String *asString() { return type == Type_String ? reinterpret_cast<String *>(this) : 0; }
    Object *asObject() { return type != Type_String ? reinterpret_cast<Object *>(this) : 0; }
    ArrayObject *asArrayObject() { return type == Type_ArrayObject ? reinterpret_cast<ArrayObject *>(this) : 0; }
    FunctionObject *asFunctionObject() { return type == Type_FunctionObject ? reinterpret_cast<FunctionObject *>(this) : 0; }
    BooleanObject *asBooleanObject() { return type == Type_BooleanObject ? reinterpret_cast<BooleanObject *>(this) : 0; }
    NumberObject *asNumberObject() { return type == Type_NumberObject ? reinterpret_cast<NumberObject *>(this) : 0; }
    StringObject *asStringObject() { return type == Type_StringObject ? reinterpret_cast<StringObject *>(this) : 0; }
    DateObject *asDateObject() { return type == Type_DateObject ? reinterpret_cast<DateObject *>(this) : 0; }
    ErrorObject *asErrorObject() { return type == Type_ErrorObject ? reinterpret_cast<ErrorObject *>(this) : 0; }
    ArgumentsObject *asArgumentsObject() { return type == Type_ArgumentsObject ? reinterpret_cast<ArgumentsObject *>(this) : 0; }

    bool isListType() const { return type == Type_QmlSequence; }

    bool isArrayObject() const { return type == Type_ArrayObject; }
    bool isStringObject() const { return type == Type_StringObject; }

    QString className() const;

    Managed **nextFreeRef() {
        return reinterpret_cast<Managed **>(this);
    }
    Managed *nextFree() {
        return *reinterpret_cast<Managed **>(this);
    }
    void setNextFree(Managed *m) {
        *reinterpret_cast<Managed **>(this) = m;
    }

    void setVTable(const ManagedVTable *vt);

    ReturnedValue construct(CallData *d);
    ReturnedValue call(CallData *d);
    ReturnedValue get(const StringRef name, bool *hasProperty = 0);
    ReturnedValue getIndexed(uint index, bool *hasProperty = 0);
    void put(const StringRef name, const ValueRef value);
    void putIndexed(uint index, const ValueRef value);
    PropertyAttributes query(StringRef name) const;
    PropertyAttributes queryIndexed(uint index) const
    { return internalClass->vtable->queryIndexed(this, index); }

    bool deleteProperty(const StringRef name);
    bool deleteIndexedProperty(uint index)
    { return internalClass->vtable->deleteIndexedProperty(this, index); }
    ReturnedValue getLookup(Lookup *l)
    { return internalClass->vtable->getLookup(this, l); }
    void setLookup(Lookup *l, const ValueRef v);

    bool isEqualTo(Managed *other)
    { return internalClass->vtable->isEqualTo(this, other); }
    Property *advanceIterator(ObjectIterator *it, StringRef name, uint *index, PropertyAttributes *attributes);

    static void destroy(Managed *that) { that->_data = 0; }
    static ReturnedValue construct(Managed *m, CallData *d);
    static ReturnedValue call(Managed *m, CallData *);
    static ReturnedValue getLookup(Managed *m, Lookup *);
    static void setLookup(Managed *m, Lookup *l, const ValueRef v);
    static bool isEqualTo(Managed *m, Managed *other);

    uint internalType() const {
        return type;
    }

    ReturnedValue asReturnedValue() { return Value::fromManaged(this).asReturnedValue(); }


    InternalClass *internalClass;

    enum {
        SimpleArray = 1
    };

    union {
        uint _data;
        struct {
            uchar markBit :  1;
            uchar inUse   :  1;
            uchar extensible : 1; // used by Object
            uchar isNonStrictArgumentsObject : 1;
            uchar needsActivation : 1; // used by FunctionObject
            uchar strictMode : 1; // used by FunctionObject
            uchar bindingKeyFlag : 1;
            uchar hasAccessorProperty : 1;
            uchar type;
            mutable uchar subtype;
            uchar flags;
        };
    };

private:
    friend class MemoryManager;
    friend struct Identifiers;
    friend struct ObjectIterator;
};

template<>
inline Managed *value_cast(const Value &v) {
    return v.asManaged();
}

template<typename T>
inline T *managed_cast(Managed *m)
{
    return m->as<T>();
}

template<>
inline String *managed_cast(Managed *m)
{
    return m->asString();
}
template<>
inline Object *managed_cast(Managed *m)
{
    return m->asObject();
}
template<>
inline FunctionObject *managed_cast(Managed *m)
{
    return m->asFunctionObject();
}


inline ReturnedValue Managed::construct(CallData *d) {
    return internalClass->vtable->construct(this, d);
}
inline ReturnedValue Managed::call(CallData *d) {
    return internalClass->vtable->call(this, d);
}

}


QT_END_NAMESPACE

#endif
