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


DEFINE_OBJECT_VTABLE(QV4::QmlValueTypeWrapper);

namespace QV4 {
namespace Heap {

struct QmlValueTypeReference : QmlValueTypeWrapper
{
    QmlValueTypeReference(QV8Engine *engine);
    QPointer<QObject> object;
    int property;
};

struct QmlValueTypeCopy : QmlValueTypeWrapper
{
    QmlValueTypeCopy(QV8Engine *engine);
    QVariant value;
};

}
}

using namespace QV4;

struct QmlValueTypeReference : public QmlValueTypeWrapper
{
    V4_OBJECT2(QmlValueTypeReference, QmlValueTypeWrapper)
};

DEFINE_OBJECT_VTABLE(QmlValueTypeReference);

struct QmlValueTypeCopy : public QmlValueTypeWrapper
{
    V4_OBJECT2(QmlValueTypeCopy, QmlValueTypeWrapper)
};

DEFINE_OBJECT_VTABLE(QmlValueTypeCopy);

Heap::QmlValueTypeWrapper::QmlValueTypeWrapper(QV8Engine *engine, ObjectType objectType)
    : Heap::Object(QV8Engine::getV4(engine))
    , v8(engine)
    , objectType(objectType)
{
    setVTable(QV4::QmlValueTypeWrapper::staticVTable());
}

Heap::QmlValueTypeReference::QmlValueTypeReference(QV8Engine *engine)
    : Heap::QmlValueTypeWrapper(engine, Reference)
{
}

Heap::QmlValueTypeCopy::QmlValueTypeCopy(QV8Engine *engine)
    : Heap::QmlValueTypeWrapper(engine, Copy)
{
}


static bool readReferenceValue(const QmlValueTypeReference *reference)
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
                reference->d()->type = QQmlValueTypeFactory::valueType(variantReferenceType);
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

void QmlValueTypeWrapper::initProto(ExecutionEngine *v4)
{
    if (v4->qmlExtensions()->valueTypeWrapperPrototype)
        return;

    Scope scope(v4);
    Scoped<Object> o(scope, v4->newObject());
    o->defineDefaultProperty(v4->id_toString, method_toString, 1);
    v4->qmlExtensions()->valueTypeWrapperPrototype = o;
}

ReturnedValue QmlValueTypeWrapper::create(QV8Engine *v8, QObject *object, int property, QQmlValueType *type)
{
    ExecutionEngine *v4 = QV8Engine::getV4(v8);
    Scope scope(v4);
    initProto(v4);

    Scoped<QmlValueTypeReference> r(scope, v4->memoryManager->alloc<QmlValueTypeReference>(v8));
    r->setPrototype(v4->qmlExtensions()->valueTypeWrapperPrototype->asObject());
    r->d()->type = type; r->d()->object = object; r->d()->property = property;
    return r->asReturnedValue();
}

ReturnedValue QmlValueTypeWrapper::create(QV8Engine *v8, const QVariant &value, QQmlValueType *type)
{
    ExecutionEngine *v4 = QV8Engine::getV4(v8);
    Scope scope(v4);
    initProto(v4);

    Scoped<QmlValueTypeCopy> r(scope, v4->memoryManager->alloc<QmlValueTypeCopy>(v8));
    r->setPrototype(v4->qmlExtensions()->valueTypeWrapperPrototype->asObject());
    r->d()->type = type; r->d()->value = value;
    return r->asReturnedValue();
}

QVariant QmlValueTypeWrapper::toVariant() const
{
    if (d()->objectType == Heap::QmlValueTypeWrapper::Reference) {
        const QmlValueTypeReference *reference = static_cast<const QmlValueTypeReference *>(this);

        if (reference->d()->object && readReferenceValue(reference)) {
            return reference->d()->type->value();
        } else {
            return QVariant();
        }
    } else {
        Q_ASSERT(d()->objectType == Heap::QmlValueTypeWrapper::Copy);
        return static_cast<const QmlValueTypeCopy *>(this)->d()->value;
    }
}

void QmlValueTypeWrapper::destroy(Heap::Base *that)
{
    Heap::QmlValueTypeWrapper *w = static_cast<Heap::QmlValueTypeWrapper *>(that);
    if (w->objectType == Heap::QmlValueTypeWrapper::Reference)
        static_cast<Heap::QmlValueTypeReference *>(w)->Heap::QmlValueTypeReference::~QmlValueTypeReference();
    else
        static_cast<Heap::QmlValueTypeCopy *>(w)->Heap::QmlValueTypeCopy::~QmlValueTypeCopy();
}

bool QmlValueTypeWrapper::isEqualTo(Managed *m, Managed *other)
{
    Q_ASSERT(m && m->as<QmlValueTypeWrapper>() && other);
    QV4::QmlValueTypeWrapper *lv = static_cast<QmlValueTypeWrapper *>(m);

    if (QV4::VariantObject *rv = other->as<VariantObject>())
        return lv->isEqual(rv->d()->data);

    if (QV4::QmlValueTypeWrapper *v = other->as<QmlValueTypeWrapper>())
        return lv->isEqual(v->toVariant());

    return false;
}

PropertyAttributes QmlValueTypeWrapper::query(const Managed *m, String *name)
{
    Q_ASSERT(m->as<const QmlValueTypeWrapper>());
    const QmlValueTypeWrapper *r = static_cast<const QmlValueTypeWrapper *>(m);

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    {
        QQmlData *ddata = QQmlData::get(r->d()->type, false);
        if (ddata && ddata->propertyCache)
            result = ddata->propertyCache->property(name, 0, 0);
        else
            result = QQmlPropertyCache::property(r->d()->v8->engine(), r->d()->type, name, 0, local);
    }
    return result ? Attr_Data : Attr_Invalid;
}

bool QmlValueTypeWrapper::isEqual(const QVariant& value)
{
    if (d()->objectType == Heap::QmlValueTypeWrapper::Reference) {
        QmlValueTypeReference *reference = static_cast<QmlValueTypeReference *>(this);
        if (reference->d()->object && readReferenceValue(reference)) {
            return reference->d()->type->isEqual(value);
        } else {
            return false;
        }
    } else {
        Q_ASSERT(d()->objectType == Heap::QmlValueTypeWrapper::Copy);
        QmlValueTypeCopy *copy = static_cast<QmlValueTypeCopy *>(this);
        d()->type->setValue(copy->d()->value);
        if (d()->type->isEqual(value))
            return true;
        return (value == copy->d()->value);
    }
}

ReturnedValue QmlValueTypeWrapper::method_toString(CallContext *ctx)
{
    Object *o = ctx->d()->callData->thisObject.asObject();
    if (!o)
        return ctx->engine()->throwTypeError();
    QmlValueTypeWrapper *w = o->as<QmlValueTypeWrapper>();
    if (!w)
        return ctx->engine()->throwTypeError();

    if (w->d()->objectType == Heap::QmlValueTypeWrapper::Reference) {
        QmlValueTypeReference *reference = static_cast<QmlValueTypeReference *>(w);
        if (reference->d()->object && readReferenceValue(reference)) {
            return w->d()->v8->toString(w->d()->type->toString());
        } else {
            return QV4::Encode::undefined();
        }
    } else {
        Q_ASSERT(w->d()->objectType == Heap::QmlValueTypeWrapper::Copy);
        QmlValueTypeCopy *copy = static_cast<QmlValueTypeCopy *>(w);
        w->d()->type->setValue(copy->d()->value);
        return w->d()->v8->toString(w->d()->type->toString());
    }
}

ReturnedValue QmlValueTypeWrapper::get(Managed *m, String *name, bool *hasProperty)
{
    Q_ASSERT(m->as<QmlValueTypeWrapper>());
    QmlValueTypeWrapper *r = static_cast<QmlValueTypeWrapper *>(m);
    QV4::ExecutionEngine *v4 = m->engine();

    // Note: readReferenceValue() can change the reference->type.
    if (r->d()->objectType == Heap::QmlValueTypeWrapper::Reference) {
        QmlValueTypeReference *reference = static_cast<QmlValueTypeReference *>(r);

        if (!reference->d()->object || !readReferenceValue(reference))
            return Primitive::undefinedValue().asReturnedValue();

    } else {
        Q_ASSERT(r->d()->objectType == Heap::QmlValueTypeWrapper::Copy);

        QmlValueTypeCopy *copy = static_cast<QmlValueTypeCopy *>(r);

        r->d()->type->setValue(copy->d()->value);
    }

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    {
        QQmlData *ddata = QQmlData::get(r->d()->type, false);
        if (ddata && ddata->propertyCache)
            result = ddata->propertyCache->property(name, 0, 0);
        else
            result = QQmlPropertyCache::property(r->d()->v8->engine(), r->d()->type, name, 0, local);
    }

    if (!result)
        return Object::get(m, name, hasProperty);

    if (hasProperty)
        *hasProperty = true;

    if (result->isFunction()) {
        // calling a Q_INVOKABLE function of a value type
        Scope scope(v4);
        ScopedContext c(scope, v4->rootContext());
        return QV4::QObjectMethod::create(c, r->d()->type, result->coreIndex);
    }

#define VALUE_TYPE_LOAD(metatype, cpptype, constructor) \
    if (result->propType == metatype) { \
        cpptype v; \
        void *args[] = { &v, 0 }; \
        r->d()->type->qt_metacall(QMetaObject::ReadProperty, result->coreIndex, args); \
        return constructor(v); \
    }

    // These four types are the most common used by the value type wrappers
    VALUE_TYPE_LOAD(QMetaType::QReal, qreal, QV4::Encode);
    VALUE_TYPE_LOAD(QMetaType::Int, int, QV4::Encode);
    VALUE_TYPE_LOAD(QMetaType::QString, QString, r->d()->v8->toString);
    VALUE_TYPE_LOAD(QMetaType::Bool, bool, QV4::Encode);

    QVariant v(result->propType, (void *)0);
    void *args[] = { v.data(), 0 };
    r->d()->type->qt_metacall(QMetaObject::ReadProperty, result->coreIndex, args);
    return r->d()->v8->fromVariant(v);
#undef VALUE_TYPE_ACCESSOR
}

void QmlValueTypeWrapper::put(Managed *m, String *name, const ValueRef value)
{
    Q_ASSERT(m->as<QmlValueTypeWrapper>());
    ExecutionEngine *v4 = m->engine();
    Scope scope(v4);
    if (scope.hasException())
        return;

    Scoped<QmlValueTypeWrapper> r(scope, static_cast<QmlValueTypeWrapper *>(m));

    QByteArray propName = name->toQString().toUtf8();
    if (r->d()->objectType == Heap::QmlValueTypeWrapper::Reference) {
        Scoped<QmlValueTypeReference> reference(scope, static_cast<Heap::QmlValueTypeReference *>(r->d()));
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

            p.write(reference->d()->type, v);

            if (writebackProperty.userType() == QMetaType::QVariant) {
                QVariant variantReferenceValue = r->d()->type->value();
                reference->d()->type->writeVariantValue(reference->d()->object, reference->d()->property, 0, &variantReferenceValue);
            } else {
                reference->d()->type->write(reference->d()->object, reference->d()->property, 0);
            }
        }

    } else {
        Q_ASSERT(r->d()->objectType == Heap::QmlValueTypeWrapper::Copy);

        Scoped<QmlValueTypeCopy> copy(scope, static_cast<Heap::QmlValueTypeCopy *>(r->d()));

        int index = r->d()->type->metaObject()->indexOfProperty(propName.constData());
        if (index == -1)
            return;

        QVariant v = r->d()->v8->toVariant(value, -1);

        r->d()->type->setValue(copy->d()->value);
        QMetaProperty p = r->d()->type->metaObject()->property(index);
        p.write(r->d()->type, v);
        copy->d()->value = r->d()->type->value();
    }
}

QT_END_NAMESPACE
