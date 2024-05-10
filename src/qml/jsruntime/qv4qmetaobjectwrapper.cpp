// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4qmetaobjectwrapper_p.h"

#include <private/qqmlobjectorgadget_p.h>
#include <private/qv4jscall_p.h>
#include <private/qv4qobjectwrapper_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

namespace QV4 {

void Heap::QMetaObjectWrapper::init(const QMetaObject *metaObject)
{
    FunctionObject::init();
    this->metaObject = metaObject;
    constructors = nullptr;
    constructorCount = 0;
}

void Heap::QMetaObjectWrapper::destroy()
{
    delete[] constructors;
}

void Heap::QMetaObjectWrapper::ensureConstructorsCache() {

    const int count = metaObject->constructorCount();
    if (constructorCount != count) {
        delete[] constructors;
        constructorCount = count;
        if (count == 0) {
            constructors = nullptr;
            return;
        }
        constructors = new QQmlPropertyData[count];

        for (int i = 0; i < count; ++i) {
            QMetaMethod method = metaObject->constructor(i);
            QQmlPropertyData &d = constructors[i];
            d.load(method);
            d.setCoreIndex(i);
        }
    }
}


ReturnedValue QMetaObjectWrapper::create(ExecutionEngine *engine, const QMetaObject* metaObject) {

     Scope scope(engine);
     Scoped<QMetaObjectWrapper> mo(scope, engine->memoryManager->allocate<QMetaObjectWrapper>(metaObject)->asReturnedValue());
     mo->init(engine);
     return mo->asReturnedValue();
}

void QMetaObjectWrapper::init(ExecutionEngine *) {
    const QMetaObject & mo = *d()->metaObject;

    for (int i = 0; i < mo.enumeratorCount(); i++) {
        QMetaEnum Enum = mo.enumerator(i);
        for (int k = 0; k < Enum.keyCount(); k++) {
            const char* key = Enum.key(k);
            const int value = Enum.value(k);
            defineReadonlyProperty(QLatin1String(key), Value::fromInt32(value));
        }
    }
}

ReturnedValue QMetaObjectWrapper::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *)
{
    const QMetaObjectWrapper *This = static_cast<const QMetaObjectWrapper*>(f);
    return This->constructInternal(argv, argc);
}

ReturnedValue QMetaObjectWrapper::constructInternal(const Value *argv, int argc) const
{

    d()->ensureConstructorsCache();

    ExecutionEngine *v4 = engine();
    const QMetaObject* mo = d()->metaObject;
    if (d()->constructorCount == 0) {
        return v4->throwTypeError(QLatin1String(mo->className())
                                  + QLatin1String(" has no invokable constructor"));
    }

    Scope scope(v4);
    Scoped<QObjectWrapper> object(scope);
    JSCallData cData(nullptr, argv, argc);
    CallData *callData = cData.callData(scope);

    const QQmlObjectOrGadget objectOrGadget(mo);

    if (d()->constructorCount == 1) {
        object = QObjectMethod::callPrecise(
                objectOrGadget, d()->constructors[0], v4, callData, QMetaObject::CreateInstance);
    } else if (const QQmlPropertyData *ctor = QObjectMethod::resolveOverloaded(
                    objectOrGadget, d()->constructors, d()->constructorCount, v4, callData)) {
        object = QObjectMethod::callPrecise(
                objectOrGadget, *ctor, v4, callData, QMetaObject::CreateInstance);
    }
    if (object) {
        Scoped<QMetaObjectWrapper> metaObject(scope, this);
        object->defineDefaultProperty(v4->id_constructor(), metaObject);
        object->setPrototypeOf(const_cast<QMetaObjectWrapper*>(this));
    }

    return object.asReturnedValue();
}

bool QMetaObjectWrapper::virtualIsEqualTo(Managed *a, Managed *b)
{
    const QMetaObjectWrapper *aMetaObject = a->as<QMetaObjectWrapper>();
    Q_ASSERT(aMetaObject);
    const QMetaObjectWrapper *bMetaObject = b->as<QMetaObjectWrapper>();
    return bMetaObject && aMetaObject->metaObject() == bMetaObject->metaObject();
}

DEFINE_OBJECT_VTABLE(QMetaObjectWrapper);

} // namespace QV4

QT_END_NAMESPACE
