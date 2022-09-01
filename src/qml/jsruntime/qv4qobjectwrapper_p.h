// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QV4QOBJECTWRAPPER_P_H
#define QV4QOBJECTWRAPPER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qpair.h>
#include <QtCore/qhash.h>
#include <private/qqmldata_p.h>
#include <private/qintrusivelist_p.h>

#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4lookup_p.h>

QT_BEGIN_NAMESPACE

class QObject;
class QQmlData;
class QQmlPropertyCache;
class QQmlPropertyData;

namespace QV4 {
struct QObjectSlotDispatcher;

namespace Heap {

struct QQmlValueTypeWrapper;

struct Q_QML_EXPORT QObjectWrapper : Object {
    void init(QObject *object)
    {
        Object::init();
        qObj.init(object);
    }

    void destroy() {
        qObj.destroy();
        Object::destroy();
    }

    QObject *object() const { return qObj.data(); }
    static void markObjects(Heap::Base *that, MarkStack *markStack);

private:
    QV4QPointer<QObject> qObj;
};

#define QObjectMethodMembers(class, Member) \
    Member(class, Pointer, QQmlValueTypeWrapper *, valueTypeWrapper) \
    Member(class, NoMark, QV4QPointer<QObject>, qObj) \
    Member(class, NoMark, int, index)

DECLARE_HEAP_OBJECT(QObjectMethod, FunctionObject) {
    DECLARE_MARKOBJECTS(QObjectMethod);

    QQmlPropertyData *methods;
    int methodCount;
    alignas(alignof(QQmlPropertyData)) std::byte _singleMethod[sizeof(QQmlPropertyData)];

    void init(QV4::ExecutionContext *scope);
    void destroy()
    {
        if (methods != reinterpret_cast<const QQmlPropertyData *>(&_singleMethod))
            delete[] methods;
        qObj.destroy();
        FunctionObject::destroy();
    }

    void ensureMethodsCache();

    const QMetaObject *metaObject();
    QObject *object() const { return qObj.data(); }
    void setObject(QObject *o) { qObj = o; }

};

struct QMetaObjectWrapper : FunctionObject {
    const QMetaObject* metaObject;
    QQmlPropertyData *constructors;
    int constructorCount;

    void init(const QMetaObject* metaObject);
    void destroy();
    void ensureConstructorsCache();
};

struct QmlSignalHandler : Object {
    void init(QObject *object, int signalIndex);
    void destroy() {
        qObj.destroy();
        Object::destroy();
    }
    int signalIndex;

    QObject *object() const { return qObj.data(); }
    void setObject(QObject *o) { qObj = o; }

private:
    QV4QPointer<QObject> qObj;
};

}

struct Q_QML_EXPORT QObjectWrapper : public Object
{
    V4_OBJECT2(QObjectWrapper, Object)
    V4_NEEDS_DESTROY

    enum RevisionMode { IgnoreRevision, CheckRevision };

    static void initializeBindings(ExecutionEngine *engine);

    QObject *object() const { return d()->object(); }

    ReturnedValue getQmlProperty(
            const QQmlRefPointer<QQmlContextData> &qmlContext, String *name,
            RevisionMode revisionMode, bool *hasProperty = nullptr,
            bool includeImports = false) const;
    \
    static ReturnedValue getQmlProperty(ExecutionEngine *engine, const QQmlRefPointer<QQmlContextData> &qmlContext,
            QObject *object, String *name, RevisionMode revisionMode, bool *hasProperty = nullptr,
            const QQmlPropertyData **property = nullptr);

    static bool setQmlProperty(
            ExecutionEngine *engine, const QQmlRefPointer<QQmlContextData> &qmlContext,
            QObject *object, String *name, RevisionMode revisionMode, const Value &value);

    static ReturnedValue wrap(ExecutionEngine *engine, QObject *object);
    static ReturnedValue wrapConst(ExecutionEngine *engine, QObject *object);
    static void markWrapper(QObject *object, MarkStack *markStack);

    using Object::get;

    static void setProperty(ExecutionEngine *engine, QObject *object, int propertyIndex, const Value &value);
    void setProperty(ExecutionEngine *engine, int propertyIndex, const Value &value);
    static void setProperty(
            ExecutionEngine *engine, QObject *object,
            const QQmlPropertyData *property, const Value &value);

    void destroyObject(bool lastCall);

    static ReturnedValue getProperty(
            ExecutionEngine *engine, QObject *object, const QQmlPropertyData *property);

    static ReturnedValue virtualResolveLookupGetter(const Object *object, ExecutionEngine *engine, Lookup *lookup);
    static ReturnedValue lookupAttached(Lookup *l, ExecutionEngine *engine, const Value &object);

    template <typename ReversalFunctor> static ReturnedValue lookupGetterImpl(Lookup *l, ExecutionEngine *engine, const Value &object, bool useOriginalProperty, ReversalFunctor revert);
    static bool virtualResolveLookupSetter(Object *object, ExecutionEngine *engine, Lookup *lookup, const Value &value);

protected:
    static bool virtualIsEqualTo(Managed *that, Managed *o);
    static ReturnedValue create(ExecutionEngine *engine, QObject *object);

    static const QQmlPropertyData *findProperty(QObject *o, const QQmlRefPointer<QQmlContextData> &qmlContext,
            String *name, RevisionMode revisionMode, QQmlPropertyData *local);

    const QQmlPropertyData *findProperty(const QQmlRefPointer<QQmlContextData> &qmlContext,
            String *name, RevisionMode revisionMode, QQmlPropertyData *local) const;

    static ReturnedValue virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty);
    static bool virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver);
    static PropertyAttributes virtualGetOwnProperty(const Managed *m, PropertyKey id, Property *p);
    static OwnPropertyKeyIterator *virtualOwnPropertyKeys(const Object *m, Value *target);

    static ReturnedValue method_connect(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_disconnect(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

private:
    Q_NEVER_INLINE static ReturnedValue wrap_slowPath(ExecutionEngine *engine, QObject *object);
    Q_NEVER_INLINE static ReturnedValue wrapConst_slowPath(ExecutionEngine *engine, QObject *object);
};

inline ReturnedValue QObjectWrapper::wrap(ExecutionEngine *engine, QObject *object)
{
    if (Q_UNLIKELY(QQmlData::wasDeleted(object)))
        return QV4::Encode::null();

    auto ddata = QQmlData::get(object);
    if (Q_LIKELY(ddata && ddata->jsEngineId == engine->m_engineId && !ddata->jsWrapper.isUndefined())) {
        // We own the JS object
        return ddata->jsWrapper.value();
    }

    return wrap_slowPath(engine, object);
}

// Unfortunately we still need a non-const QObject* here because QQmlData needs to register itself in QObjectPrivate.
inline ReturnedValue QObjectWrapper::wrapConst(ExecutionEngine *engine, QObject *object)
{
    if (Q_UNLIKELY(QQmlData::wasDeleted(object)))
        return QV4::Encode::null();

    return wrapConst_slowPath(engine, object);
}

template <typename ReversalFunctor>
inline ReturnedValue QObjectWrapper::lookupGetterImpl(Lookup *lookup, ExecutionEngine *engine, const Value &object, bool useOriginalProperty, ReversalFunctor revertLookup)
{
    // we can safely cast to a QV4::Object here. If object is something else,
    // the internal class won't match
    Heap::Object *o = static_cast<Heap::Object *>(object.heapObject());
    if (!o || o->internalClass != lookup->qobjectLookup.ic)
        return revertLookup();

    const Heap::QObjectWrapper *This = static_cast<const Heap::QObjectWrapper *>(o);
    QObject *qobj = This->object();
    if (QQmlData::wasDeleted(qobj))
        return QV4::Encode::undefined();

    QQmlData *ddata = QQmlData::get(qobj, /*create*/false);
    if (!ddata)
        return revertLookup();

    const QQmlPropertyData *property = lookup->qobjectLookup.propertyData;
    if (ddata->propertyCache.data() != lookup->qobjectLookup.propertyCache) {
        if (property->isOverridden() && (!useOriginalProperty || property->isFunction() || property->isSignalHandler()))
            return revertLookup();

        const QQmlPropertyCache *fromMo = ddata->propertyCache.data();
        const QQmlPropertyCache *toMo = lookup->qobjectLookup.propertyCache;
        bool canConvert = false;
        while (fromMo) {
            if (fromMo == toMo) {
                canConvert = true;
                break;
            }
            fromMo = fromMo->parent().data();
        }
        if (!canConvert)
            return revertLookup();
    }

    return getProperty(engine, qobj, property);
}

struct QQmlValueTypeWrapper;

struct Q_QML_EXPORT QObjectMethod : public QV4::FunctionObject
{
    V4_OBJECT2(QObjectMethod, QV4::FunctionObject)
    V4_NEEDS_DESTROY

    enum { DestroyMethod = -1, ToStringMethod = -2 };

    static ReturnedValue create(QV4::ExecutionContext *scope, QObject *object, int index);
    static ReturnedValue create(QV4::ExecutionContext *scope, Heap::QQmlValueTypeWrapper *valueType, int index);

    int methodIndex() const { return d()->index; }
    QObject *object() const { return d()->object(); }

    QV4::ReturnedValue method_toString(QV4::ExecutionEngine *engine) const;
    QV4::ReturnedValue method_destroy(QV4::ExecutionEngine *ctx, const Value *args, int argc) const;

    static ReturnedValue virtualCall(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);

    ReturnedValue callInternal(const Value *thisObject, const Value *argv, int argc) const;

    static QPair<QObject *, int> extractQtMethod(const QV4::FunctionObject *function);
};


struct Q_QML_EXPORT QMetaObjectWrapper : public QV4::FunctionObject
{
    V4_OBJECT2(QMetaObjectWrapper, QV4::FunctionObject)
    V4_NEEDS_DESTROY

    static ReturnedValue create(ExecutionEngine *engine, const QMetaObject* metaObject);
    const QMetaObject *metaObject() const { return d()->metaObject; }

protected:
    static ReturnedValue virtualCallAsConstructor(const FunctionObject *, const Value *argv, int argc, const Value *);
    static bool virtualIsEqualTo(Managed *a, Managed *b);

private:
    void init(ExecutionEngine *engine);
    ReturnedValue constructInternal(const Value *argv, int argc) const;
};

struct Q_QML_EXPORT QmlSignalHandler : public QV4::Object
{
    V4_OBJECT2(QmlSignalHandler, QV4::Object)
    V4_PROTOTYPE(signalHandlerPrototype)
    V4_NEEDS_DESTROY

    int signalIndex() const { return d()->signalIndex; }
    QObject *object() const { return d()->object(); }

    static void initProto(ExecutionEngine *v4);
};

using QObjectBiPointer = QBiPointer<QObject, const QObject>;

class MultiplyWrappedQObjectMap : public QObject,
                                  private QHash<QObjectBiPointer, QV4::WeakValue>
{
    Q_OBJECT
public:
    typedef QHash<QObjectBiPointer, QV4::WeakValue>::ConstIterator ConstIterator;
    typedef QHash<QObjectBiPointer, QV4::WeakValue>::Iterator Iterator;

    using value_type = QHash<QObjectBiPointer, QV4::WeakValue>::value_type;

    ConstIterator begin() const { return QHash<QObjectBiPointer, QV4::WeakValue>::constBegin(); }
    Iterator begin() { return QHash<QObjectBiPointer, QV4::WeakValue>::begin(); }
    ConstIterator end() const { return QHash<QObjectBiPointer, QV4::WeakValue>::constEnd(); }
    Iterator end() { return QHash<QObjectBiPointer, QV4::WeakValue>::end(); }

    template<typename Pointer>
    void insert(Pointer key, Heap::Object *value)
    {
        QHash<QObjectBiPointer, WeakValue>::operator[](key).set(value->internalClass->engine, value);
        connect(key, SIGNAL(destroyed(QObject*)), this, SLOT(removeDestroyedObject(QObject*)));
    }

    template<typename Pointer>
    ReturnedValue value(Pointer key) const
    {
        ConstIterator it = find(key);
        return it == end()
                ? QV4::WeakValue().value()
                : it->value();
    }

    Iterator erase(Iterator it);

    template<typename Pointer>
    void remove(Pointer key)
    {
        Iterator it = find(key);
        if (it == end())
            return;
        erase(it);
    }

    template<typename Pointer>
    void mark(Pointer key, MarkStack *markStack)
    {
        Iterator it = find(key);
        if (it == end())
            return;
        it->markOnce(markStack);
    }

private Q_SLOTS:
    void removeDestroyedObject(QObject*);
};

}

QT_END_NAMESPACE

#endif // QV4QOBJECTWRAPPER_P_H


