// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

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
    m_metaObject = metaObject;
}

void Heap::QMetaObjectWrapper::destroy()
{
    delete[] m_constructors;
    FunctionObject::destroy();
}

ReturnedValue QMetaObjectWrapper::create(ExecutionEngine *engine, const QMetaObject* metaObject) {

     Scope scope(engine);
     Scoped<QMetaObjectWrapper> mo(scope, engine->memoryManager->allocate<QMetaObjectWrapper>(metaObject)->asReturnedValue());
     mo->init(engine);
     return mo->asReturnedValue();
}

void QMetaObjectWrapper::init(ExecutionEngine *) {
    const QMetaObject &mo = *d()->metaObject();

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
    Q_ASSERT(f->as<QMetaObjectWrapper>());
    return construct(static_cast<const QMetaObjectWrapper*>(f)->d(), argv, argc);
}

ReturnedValue QMetaObjectWrapper::constructInternal(
        const QMetaObject *mo, const QQmlPropertyData *constructors, Heap::FunctionObject *d,
        const Value *argv, int argc)
{
    ExecutionEngine *v4 = d->internalClass->engine;

    if (!constructors) {
        return v4->throwTypeError(QLatin1String(mo->className())
                                  + QLatin1String(" has no invokable constructor"));
    }

    Scope scope(v4);
    ScopedObject object(scope);
    JSCallData cData(nullptr, argv, argc);
    CallData *callData = cData.callData(scope);

    const QQmlObjectOrGadget objectOrGadget(mo);

    const auto callType = [](QMetaType metaType) {
        return metaType.flags() & QMetaType::PointerToQObject
                ? QMetaObject::CreateInstance
                : QMetaObject::ConstructInPlace;
    };

    const int constructorCount = mo->constructorCount();
    if (constructorCount == 1) {
        object = QObjectMethod::callPrecise(
                objectOrGadget, constructors[0], v4, callData,
                callType(constructors[0].propType()));
    } else if (const QQmlPropertyData *ctor = QObjectMethod::resolveOverloaded(
                       objectOrGadget, constructors, constructorCount, v4, callData)) {
        object = QObjectMethod::callPrecise(
                objectOrGadget, *ctor, v4, callData, callType(ctor->propType()));
    }

    if (object) {
        Scoped<FunctionObject> functionObject(scope, d);
        object->defineDefaultProperty(v4->id_constructor(), functionObject);
        object->setPrototypeOf(functionObject);
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
