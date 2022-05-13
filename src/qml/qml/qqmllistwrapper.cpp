// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllistwrapper_p.h"
#include <private/qqmllist_p.h>
#include <private/qv4objectproto_p.h>
#include <qv4objectiterator_p.h>

#include <private/qv4functionobject_p.h>
#include <private/qv4qobjectwrapper_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

DEFINE_OBJECT_VTABLE(QmlListWrapper);

void Heap::QmlListWrapper::init()
{
    Object::init();
    object.init();
    QV4::Scope scope(internalClass->engine);
    QV4::ScopedObject o(scope, this);
    o->setArrayType(Heap::ArrayData::Custom);
}

void Heap::QmlListWrapper::destroy()
{
    object.destroy();
    Object::destroy();
}

ReturnedValue QmlListWrapper::create(ExecutionEngine *engine, QObject *object, int propId, QMetaType propType)
{
    if (!object || propId == -1)
        return Encode::null();

    Scope scope(engine);

    Scoped<QmlListWrapper> r(scope, engine->memoryManager->allocate<QmlListWrapper>());
    r->d()->object = object;
    r->d()->propertyType = propType.iface();
    void *args[] = { &r->d()->property(), nullptr };
    QMetaObject::metacall(object, QMetaObject::ReadProperty, propId, args);
    return r.asReturnedValue();
}

ReturnedValue QmlListWrapper::create(ExecutionEngine *engine, const QQmlListProperty<QObject> &prop, QMetaType propType)
{
    Scope scope(engine);

    Scoped<QmlListWrapper> r(scope, engine->memoryManager->allocate<QmlListWrapper>());
    r->d()->object = prop.object;
    r->d()->property() = prop;
    r->d()->propertyType = propType.iface();
    return r.asReturnedValue();
}

QVariant QmlListWrapper::toVariant() const
{
    if (!d()->object)
        return QVariant();

    return QVariant::fromValue(toListReference());
}

QQmlListReference QmlListWrapper::toListReference() const
{
    Heap::QmlListWrapper *wrapper = d();
    return QQmlListReferencePrivate::init(wrapper->property(), QMetaType(wrapper->propertyType));
}


ReturnedValue QmlListWrapper::virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty)
{
    Q_ASSERT(m->as<QmlListWrapper>());
    const QmlListWrapper *w = static_cast<const QmlListWrapper *>(m);
    QV4::ExecutionEngine *v4 = w->engine();

    if (id.isArrayIndex()) {
        uint index = id.asArrayIndex();
        quint32 count = w->d()->property().count ? w->d()->property().count(&w->d()->property()) : 0;
        if (index < count && w->d()->property().at) {
            if (hasProperty)
                *hasProperty = true;
            return QV4::QObjectWrapper::wrap(v4, w->d()->property().at(&w->d()->property(), index));
        }

        if (hasProperty)
            *hasProperty = false;
        return Value::undefinedValue().asReturnedValue();
    }

    if (id.isString() && id == v4->id_length()->propertyKey()) {
        if (hasProperty)
            *hasProperty = true;
        quint32 count = w->d()->property().count ? w->d()->property().count(&w->d()->property()) : 0;
        return Value::fromUInt32(count).asReturnedValue();
    }

    return Object::virtualGet(m, id, receiver, hasProperty);
}

bool QmlListWrapper::virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver)
{
    Q_ASSERT(m->as<QmlListWrapper>());

    const auto *w = static_cast<const QmlListWrapper *>(m);
    QV4::ExecutionEngine *v4 = w->engine();

    QQmlListProperty<QObject> *prop = &(w->d()->property());

    if (id.isArrayIndex()) {
        if (!prop->count || !prop->replace)
            return false;

        const uint index = id.asArrayIndex();
        const int count = prop->count(prop);
        if (count < 0 || index >= uint(count))
            return false;

        if (value.isNull()) {
            prop->replace(prop, index, nullptr);
            return true;
        }

        QV4::Scope scope(v4);
        QV4::ScopedObject so(scope, value.toObject(scope.engine));
        if (auto *wrapper = so->as<QV4::QObjectWrapper>()) {
            prop->replace(prop, index, wrapper->object());
            return true;
        }

        return false;
    }

    if (id.isString() && id == v4->id_length()->propertyKey()) {
        if (!prop->count)
            return false;

        const quint32 count = prop->count(prop);

        bool ok = false;
        const uint newLength = value.asArrayLength(&ok);
        if (!ok)
            return false;

        if (newLength == 0) {
            if (!prop->clear)
                return false;
            prop->clear(prop);
            return true;
        }

        if (newLength < count) {
            if (!prop->removeLast)
                return false;

            for (uint i = newLength; i < count; ++i)
                prop->removeLast(prop);

            return true;
        }

        if (!prop->append)
            return false;

        for (uint i = count; i < newLength; ++i)
            prop->append(prop, nullptr);

        return true;
    }

    return Object::virtualPut(m, id, value, receiver);
}

struct QmlListWrapperOwnPropertyKeyIterator : ObjectOwnPropertyKeyIterator
{
    ~QmlListWrapperOwnPropertyKeyIterator() override = default;
    PropertyKey next(const Object *o, Property *pd = nullptr, PropertyAttributes *attrs = nullptr) override;

};

PropertyKey QmlListWrapperOwnPropertyKeyIterator::next(const Object *o, Property *pd, PropertyAttributes *attrs)
{
    const QmlListWrapper *w = static_cast<const QmlListWrapper *>(o);

    quint32 count = w->d()->property().count ? w->d()->property().count(&w->d()->property()) : 0;
    if (arrayIndex < count) {
        uint index = arrayIndex;
        ++arrayIndex;
        if (attrs)
            *attrs = QV4::Attr_Data;
        if (pd)
            pd->value = QV4::QObjectWrapper::wrap(w->engine(), w->d()->property().at(&w->d()->property(), index));
        return PropertyKey::fromArrayIndex(index);
    }

    return ObjectOwnPropertyKeyIterator::next(o, pd, attrs);
}

OwnPropertyKeyIterator *QmlListWrapper::virtualOwnPropertyKeys(const Object *m, Value *target)
{
    *target = *m;
    return new QmlListWrapperOwnPropertyKeyIterator;
}

void PropertyListPrototype::init(ExecutionEngine *)
{
    defineDefaultProperty(QStringLiteral("push"), method_push, 1);
}

ReturnedValue PropertyListPrototype::method_push(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    ScopedObject instance(scope, thisObject->toObject(scope.engine));
    if (!instance)
        RETURN_UNDEFINED();
    QmlListWrapper *w = instance->as<QmlListWrapper>();
    if (!w)
        RETURN_UNDEFINED();

    QQmlListProperty<QObject> *property = &w->d()->property();
    if (!property->append)
        THROW_GENERIC_ERROR("List doesn't define an Append function");

    QV4::ScopedObject so(scope);
    for (int i = 0, ei = argc; i < ei; ++i)
    {
        if (argv[i].isNull()) {
            property->append(property, nullptr);
        } else {
            so = argv[i].toObject(scope.engine);
            if (QV4::QObjectWrapper *wrapper = so->as<QV4::QObjectWrapper>())
                property->append(property, wrapper->object() );
        }
    }
    return Encode::undefined();
}

QT_END_NAMESPACE
