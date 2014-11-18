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

#include "qqmlvaluetypewrapper_p.h"
#include <private/qv8engine_p.h>

#include <private/qqmlvaluetype_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qqmlbuiltinfunctions_p.h>

#include <private/qv4engine_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4variantobject_p.h>
#include <private/qv4qmlextensions_p.h>

QT_BEGIN_NAMESPACE


DEFINE_OBJECT_VTABLE(QV4::QQmlValueTypeWrapper);

namespace QV4 {
namespace Heap {

struct QQmlValueTypeReference : QQmlValueTypeWrapper
{
    QQmlValueTypeReference(QV8Engine *engine);
    QPointer<QObject> object;
    int property;
};

struct QQmlValueTypeCopy : QQmlValueTypeWrapper
{
    QQmlValueTypeCopy(QV8Engine *engine);
    QVariant value;
};

}
}

using namespace QV4;

struct QQmlValueTypeReference : public QQmlValueTypeWrapper
{
    V4_OBJECT2(QQmlValueTypeReference, QQmlValueTypeWrapper)
};

DEFINE_OBJECT_VTABLE(QQmlValueTypeReference);

struct QQmlValueTypeCopy : public QQmlValueTypeWrapper
{
    V4_OBJECT2(QQmlValueTypeCopy, QQmlValueTypeWrapper)
};

DEFINE_OBJECT_VTABLE(QQmlValueTypeCopy);

Heap::QQmlValueTypeWrapper::QQmlValueTypeWrapper(QV8Engine *engine, ObjectType objectType)
    : Heap::Object(QV8Engine::getV4(engine))
    , v8(engine)
    , objectType(objectType)
{
    setVTable(QV4::QQmlValueTypeWrapper::staticVTable());
}

Heap::QQmlValueTypeReference::QQmlValueTypeReference(QV8Engine *engine)
    : Heap::QQmlValueTypeWrapper(engine, Reference)
{
}

Heap::QQmlValueTypeCopy::QQmlValueTypeCopy(QV8Engine *engine)
    : Heap::QQmlValueTypeWrapper(engine, Copy)
{
}


static bool readReferenceValue(const QQmlValueTypeReference *reference)
{
    // A reference resource may be either a "true" reference (eg, to a QVector3D property)
    // or a "variant" reference (eg, to a QVariant property which happens to contain a value-type).
    QMetaProperty writebackProperty = reference->d()->object->metaObject()->property(reference->d()->property);
    if (writebackProperty.userType() == QMetaType::QVariant) {
        // variant-containing-value-type reference
        QVariant variantReferenceValue;
        reference->d()->type->readVariantValue(reference->d()->object, reference->d()->property, &variantReferenceValue);
        int variantReferenceType = variantReferenceValue.userType();
        if (variantReferenceType != reference->d()->type->userType()) {
            // This is a stale VariantReference.  That is, the variant has been
            // overwritten with a different type in the meantime.
            // We need to modify this reference to the updated value type, if
            // possible, or return false if it is not a value type.
            if (QQmlValueTypeFactory::isValueType(variantReferenceType)) {
                QQmlValueType *vt = 0;
                if (const QMetaObject *mo = QQmlValueTypeFactory::metaObjectForMetaType(variantReferenceType))
                    vt = new QQmlValueType(variantReferenceType, mo);
                reference->d()->type.reset(vt);
                if (!reference->d()->type) {
                    return false;
                }
            } else {
                return false;
            }
        }
        reference->d()->type->setValue(variantReferenceValue);
    } else {
        // value-type reference
        reference->d()->type->read(reference->d()->object, reference->d()->property);
    }
    return true;
}

void QQmlValueTypeWrapper::initProto(ExecutionEngine *v4)
{
    if (v4->qmlExtensions()->valueTypeWrapperPrototype)
        return;

    Scope scope(v4);
    Scoped<Object> o(scope, v4->newObject());
    o->defineDefaultProperty(v4->id_toString, method_toString, 1);
    v4->qmlExtensions()->valueTypeWrapperPrototype = o->d();
}

ReturnedValue QQmlValueTypeWrapper::create(QV8Engine *v8, QObject *object, int property, const QMetaObject *metaObject, int typeId)
{
    ExecutionEngine *v4 = QV8Engine::getV4(v8);
    Scope scope(v4);
    initProto(v4);

    Scoped<QQmlValueTypeReference> r(scope, v4->memoryManager->alloc<QQmlValueTypeReference>(v8));
    ScopedObject proto(scope, v4->qmlExtensions()->valueTypeWrapperPrototype);
    r->setPrototype(proto);
    r->d()->type.reset(new QQmlValueType(typeId, metaObject)); r->d()->object = object; r->d()->property = property;
    return r->asReturnedValue();
}

ReturnedValue QQmlValueTypeWrapper::create(QV8Engine *v8, const QVariant &value,  const QMetaObject *metaObject, int typeId)
{
    ExecutionEngine *v4 = QV8Engine::getV4(v8);
    Scope scope(v4);
    initProto(v4);

    Scoped<QQmlValueTypeCopy> r(scope, v4->memoryManager->alloc<QQmlValueTypeCopy>(v8));
    ScopedObject proto(scope, v4->qmlExtensions()->valueTypeWrapperPrototype);
    r->setPrototype(proto);
    r->d()->type.reset(new QQmlValueType(typeId, metaObject)); r->d()->value = value;
    return r->asReturnedValue();
}

QVariant QQmlValueTypeWrapper::toVariant() const
{
    if (d()->objectType == Heap::QQmlValueTypeWrapper::Reference) {
        const QQmlValueTypeReference *reference = static_cast<const QQmlValueTypeReference *>(this);

        if (reference->d()->object && readReferenceValue(reference)) {
            return reference->d()->type->value();
        } else {
            return QVariant();
        }
    } else {
        Q_ASSERT(d()->objectType == Heap::QQmlValueTypeWrapper::Copy);
        return static_cast<const QQmlValueTypeCopy *>(this)->d()->value;
    }
}

void QQmlValueTypeWrapper::destroy(Heap::Base *that)
{
    Heap::QQmlValueTypeWrapper *w = static_cast<Heap::QQmlValueTypeWrapper *>(that);
    if (w->objectType == Heap::QQmlValueTypeWrapper::Reference)
        static_cast<Heap::QQmlValueTypeReference *>(w)->Heap::QQmlValueTypeReference::~QQmlValueTypeReference();
    else
        static_cast<Heap::QQmlValueTypeCopy *>(w)->Heap::QQmlValueTypeCopy::~QQmlValueTypeCopy();
}

bool QQmlValueTypeWrapper::isEqualTo(Managed *m, Managed *other)
{
    Q_ASSERT(m && m->as<QQmlValueTypeWrapper>() && other);
    QV4::QQmlValueTypeWrapper *lv = static_cast<QQmlValueTypeWrapper *>(m);

    if (QV4::VariantObject *rv = other->as<VariantObject>())
        return lv->isEqual(rv->d()->data);

    if (QV4::QQmlValueTypeWrapper *v = other->as<QQmlValueTypeWrapper>())
        return lv->isEqual(v->toVariant());

    return false;
}

PropertyAttributes QQmlValueTypeWrapper::query(const Managed *m, String *name)
{
    Q_ASSERT(m->as<const QQmlValueTypeWrapper>());
    const QQmlValueTypeWrapper *r = static_cast<const QQmlValueTypeWrapper *>(m);

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    {
        QQmlData *ddata = QQmlData::get(r->d()->type.data(), false);
        if (ddata && ddata->propertyCache)
            result = ddata->propertyCache->property(name, 0, 0);
        else
            result = QQmlPropertyCache::property(r->d()->v8->engine(), r->d()->type.data(), name, 0, local);
    }
    return result ? Attr_Data : Attr_Invalid;
}

bool QQmlValueTypeWrapper::isEqual(const QVariant& value)
{
    if (d()->objectType == Heap::QQmlValueTypeWrapper::Reference) {
        QQmlValueTypeReference *reference = static_cast<QQmlValueTypeReference *>(this);
        if (reference->d()->object && readReferenceValue(reference)) {
            return reference->d()->type->isEqual(value);
        } else {
            return false;
        }
    } else {
        Q_ASSERT(d()->objectType == Heap::QQmlValueTypeWrapper::Copy);
        QQmlValueTypeCopy *copy = static_cast<QQmlValueTypeCopy *>(this);
        d()->type->setValue(copy->d()->value);
        if (d()->type->isEqual(value))
            return true;
        return (value == copy->d()->value);
    }
}

ReturnedValue QQmlValueTypeWrapper::method_toString(CallContext *ctx)
{
    Object *o = ctx->d()->callData->thisObject.asObject();
    if (!o)
        return ctx->engine()->throwTypeError();
    QQmlValueTypeWrapper *w = o->as<QQmlValueTypeWrapper>();
    if (!w)
        return ctx->engine()->throwTypeError();

    if (w->d()->objectType == Heap::QQmlValueTypeWrapper::Reference) {
        QQmlValueTypeReference *reference = static_cast<QQmlValueTypeReference *>(w);
        if (reference->d()->object && readReferenceValue(reference)) {
            return w->d()->v8->toString(w->d()->type->toString());
        } else {
            return QV4::Encode::undefined();
        }
    } else {
        Q_ASSERT(w->d()->objectType == Heap::QQmlValueTypeWrapper::Copy);
        QQmlValueTypeCopy *copy = static_cast<QQmlValueTypeCopy *>(w);
        w->d()->type->setValue(copy->d()->value);
        return w->d()->v8->toString(w->d()->type->toString());
    }
}

ReturnedValue QQmlValueTypeWrapper::get(Managed *m, String *name, bool *hasProperty)
{
    Q_ASSERT(m->as<QQmlValueTypeWrapper>());
    QQmlValueTypeWrapper *r = static_cast<QQmlValueTypeWrapper *>(m);
    QV4::ExecutionEngine *v4 = m->engine();

    // ### Remove this once we can do proper this calls.
    if (name == v4->id_toString)
        return Object::get(m, name, hasProperty);

    // Note: readReferenceValue() can change the reference->type.
    if (r->d()->objectType == Heap::QQmlValueTypeWrapper::Reference) {
        QQmlValueTypeReference *reference = static_cast<QQmlValueTypeReference *>(r);

        if (!reference->d()->object || !readReferenceValue(reference))
            return Primitive::undefinedValue().asReturnedValue();

    } else {
        Q_ASSERT(r->d()->objectType == Heap::QQmlValueTypeWrapper::Copy);

        QQmlValueTypeCopy *copy = static_cast<QQmlValueTypeCopy *>(r);

        r->d()->type->setValue(copy->d()->value);
    }

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    {
        QQmlData *ddata = QQmlData::get(r->d()->type.data(), false);
        if (ddata && ddata->propertyCache)
            result = ddata->propertyCache->property(name, 0, 0);
        else
            result = QQmlPropertyCache::property(r->d()->v8->engine(), r->d()->type.data(), name, 0, local);
    }

    if (!result)
        return Object::get(m, name, hasProperty);

    if (hasProperty)
        *hasProperty = true;

    if (result->isFunction()) {
        // calling a Q_INVOKABLE function of a value type
        Scope scope(v4);
        ScopedContext c(scope, v4->rootContext());
        return QV4::QObjectMethod::create(c, r->d()->type.data(), result->coreIndex);
    }

#define VALUE_TYPE_LOAD(metatype, cpptype, constructor) \
    if (result->propType == metatype) { \
        cpptype v; \
        void *args[] = { &v, 0 }; \
        QMetaObject::metacall(r->d()->type.data(), QMetaObject::ReadProperty, result->coreIndex, args); \
        return constructor(v); \
    }

    // These four types are the most common used by the value type wrappers
    VALUE_TYPE_LOAD(QMetaType::QReal, qreal, QV4::Encode);
    VALUE_TYPE_LOAD(QMetaType::Int, int, QV4::Encode);
    VALUE_TYPE_LOAD(QMetaType::QString, QString, r->d()->v8->toString);
    VALUE_TYPE_LOAD(QMetaType::Bool, bool, QV4::Encode);

    QVariant v(result->propType, (void *)0);
    void *args[] = { v.data(), 0 };
    QMetaObject::metacall(r->d()->type.data(), QMetaObject::ReadProperty, result->coreIndex, args);
    return r->d()->v8->fromVariant(v);
#undef VALUE_TYPE_ACCESSOR
}

void QQmlValueTypeWrapper::put(Managed *m, String *name, const ValueRef value)
{
    Q_ASSERT(m->as<QQmlValueTypeWrapper>());
    ExecutionEngine *v4 = m->engine();
    Scope scope(v4);
    if (scope.hasException())
        return;

    Scoped<QQmlValueTypeWrapper> r(scope, static_cast<QQmlValueTypeWrapper *>(m));

    QByteArray propName = name->toQString().toUtf8();
    if (r->d()->objectType == Heap::QQmlValueTypeWrapper::Reference) {
        Scoped<QQmlValueTypeReference> reference(scope, static_cast<Heap::QQmlValueTypeReference *>(r->d()));
        QMetaProperty writebackProperty = reference->d()->object->metaObject()->property(reference->d()->property);

        if (!reference->d()->object || !writebackProperty.isWritable() || !readReferenceValue(reference))
            return;

        // we lookup the index after readReferenceValue() since it can change the reference->type.
        int index = r->d()->type->metaObject()->indexOfProperty(propName.constData());
        if (index == -1)
            return;
        QMetaProperty p = r->d()->type->metaObject()->property(index);

        QQmlBinding *newBinding = 0;

        QV4::ScopedFunctionObject f(scope, value);
        if (f) {
            if (!f->bindingKeyFlag()) {
                // assigning a JS function to a non-var-property is not allowed.
                QString error = QLatin1String("Cannot assign JavaScript function to value-type property");
                Scoped<String> e(scope, r->d()->v8->toString(error));
                v4->throwError(e);
                return;
            }

            QQmlContextData *context = r->d()->v8->callingContext();

            QQmlPropertyData cacheData;
            cacheData.setFlags(QQmlPropertyData::IsWritable |
                               QQmlPropertyData::IsValueTypeVirtual);
            cacheData.propType = reference->d()->object->metaObject()->property(reference->d()->property).userType();
            cacheData.coreIndex = reference->d()->property;
            cacheData.valueTypeFlags = 0;
            cacheData.valueTypeCoreIndex = index;
            cacheData.valueTypePropType = p.userType();

            QV4::Scoped<QQmlBindingFunction> bindingFunction(scope, f);
            bindingFunction->initBindingLocation();

            newBinding = new QQmlBinding(value, reference->d()->object, context);
            newBinding->setTarget(reference->d()->object, cacheData, context);
        }

        QQmlAbstractBinding *oldBinding =
            QQmlPropertyPrivate::setBinding(reference->d()->object, reference->d()->property, index, newBinding);
        if (oldBinding)
            oldBinding->destroy();

        if (!f) {
            QVariant v = r->d()->v8->toVariant(value, -1);

            if (p.isEnumType() && (QMetaType::Type)v.type() == QMetaType::Double)
                v = v.toInt();

            p.write(reference->d()->type.data(), v);

            if (writebackProperty.userType() == QMetaType::QVariant) {
                QVariant variantReferenceValue = r->d()->type->value();
                reference->d()->type->writeVariantValue(reference->d()->object, reference->d()->property, 0, &variantReferenceValue);
            } else {
                reference->d()->type->write(reference->d()->object, reference->d()->property, 0);
            }
        }

    } else {
        Q_ASSERT(r->d()->objectType == Heap::QQmlValueTypeWrapper::Copy);

        Scoped<QQmlValueTypeCopy> copy(scope, static_cast<Heap::QQmlValueTypeCopy *>(r->d()));

        int index = r->d()->type->metaObject()->indexOfProperty(propName.constData());
        if (index == -1)
            return;

        QVariant v = r->d()->v8->toVariant(value, -1);

        r->d()->type->setValue(copy->d()->value);
        QMetaProperty p = r->d()->type->metaObject()->property(index);
        p.write(r->d()->type.data(), v);
        copy->d()->value = r->d()->type->value();
    }
}

QT_END_NAMESPACE
