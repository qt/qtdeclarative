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

using namespace QV4;

DEFINE_OBJECT_VTABLE(QmlValueTypeWrapper);

class QmlValueTypeReference : public QmlValueTypeWrapper
{
public:
    QmlValueTypeReference(QV8Engine *engine);

    QPointer<QObject> object;
    int property;
};

class QmlValueTypeCopy : public QmlValueTypeWrapper
{
public:
    QmlValueTypeCopy(QV8Engine *engine);

    QVariant value;
};

QmlValueTypeWrapper::QmlValueTypeWrapper(QV8Engine *engine, ObjectType objectType)
    : Object(QV8Engine::getV4(engine)), objectType(objectType)
{
    v8 = engine;
    setVTable(staticVTable());
}

QmlValueTypeWrapper::~QmlValueTypeWrapper()
{
}

QmlValueTypeReference::QmlValueTypeReference(QV8Engine *engine)
: QmlValueTypeWrapper(engine, Reference)
{
}

QmlValueTypeCopy::QmlValueTypeCopy(QV8Engine *engine)
: QmlValueTypeWrapper(engine, Copy)
{
}


static bool readReferenceValue(const QmlValueTypeReference *reference)
{
    // A reference resource may be either a "true" reference (eg, to a QVector3D property)
    // or a "variant" reference (eg, to a QVariant property which happens to contain a value-type).
    QMetaProperty writebackProperty = reference->object->metaObject()->property(reference->property);
    if (writebackProperty.userType() == QMetaType::QVariant) {
        // variant-containing-value-type reference
        QVariant variantReferenceValue;
        reference->type->readVariantValue(reference->object, reference->property, &variantReferenceValue);
        int variantReferenceType = variantReferenceValue.userType();
        if (variantReferenceType != reference->type->userType()) {
            // This is a stale VariantReference.  That is, the variant has been
            // overwritten with a different type in the meantime.
            // We need to modify this reference to the updated value type, if
            // possible, or return false if it is not a value type.
            if (QQmlValueTypeFactory::isValueType(variantReferenceType)) {
                reference->type = QQmlValueTypeFactory::valueType(variantReferenceType);
                if (!reference->type) {
                    return false;
                }
            } else {
                return false;
            }
        }
        reference->type->setValue(variantReferenceValue);
    } else {
        // value-type reference
        reference->type->read(reference->object, reference->property);
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
    v4->qmlExtensions()->valueTypeWrapperPrototype = o.getPointer();
}

ReturnedValue QmlValueTypeWrapper::create(QV8Engine *v8, QObject *object, int property, QQmlValueType *type)
{
    ExecutionEngine *v4 = QV8Engine::getV4(v8);
    Scope scope(v4);
    initProto(v4);

    Scoped<QmlValueTypeReference> r(scope, new (v4->memoryManager) QmlValueTypeReference(v8));
    r->setPrototype(v4->qmlExtensions()->valueTypeWrapperPrototype);
    r->type = type; r->object = object; r->property = property;
    return r.asReturnedValue();
}

ReturnedValue QmlValueTypeWrapper::create(QV8Engine *v8, const QVariant &value, QQmlValueType *type)
{
    ExecutionEngine *v4 = QV8Engine::getV4(v8);
    Scope scope(v4);
    initProto(v4);

    Scoped<QmlValueTypeCopy> r(scope, new (v4->memoryManager) QmlValueTypeCopy(v8));
    r->setPrototype(v4->qmlExtensions()->valueTypeWrapperPrototype);
    r->type = type; r->value = value;
    return r.asReturnedValue();
}

QVariant QmlValueTypeWrapper::toVariant() const
{
    if (objectType == QmlValueTypeWrapper::Reference) {
        const QmlValueTypeReference *reference = static_cast<const QmlValueTypeReference *>(this);

        if (reference->object && readReferenceValue(reference)) {
            return reference->type->value();
        } else {
            return QVariant();
        }
    } else {
        Q_ASSERT(objectType == QmlValueTypeWrapper::Copy);
        return static_cast<const QmlValueTypeCopy *>(this)->value;
    }
}

void QmlValueTypeWrapper::destroy(Managed *that)
{
    QmlValueTypeWrapper *w = that->as<QmlValueTypeWrapper>();
    assert(w);
    if (w->objectType == Reference)
        static_cast<QmlValueTypeReference *>(w)->~QmlValueTypeReference();
    else
        static_cast<QmlValueTypeCopy *>(w)->~QmlValueTypeCopy();
}

bool QmlValueTypeWrapper::isEqualTo(Managed *m, Managed *other)
{
    QV4::QmlValueTypeWrapper *lv = m->as<QmlValueTypeWrapper>();
    assert(lv);

    if (QV4::VariantObject *rv = other->as<VariantObject>())
        return lv->isEqual(rv->data);

    if (QV4::QmlValueTypeWrapper *v = other->as<QmlValueTypeWrapper>())
        return lv->isEqual(v->toVariant());

    return false;
}

PropertyAttributes QmlValueTypeWrapper::query(const Managed *m, StringRef name)
{
    const QmlValueTypeWrapper *r = m->as<const QmlValueTypeWrapper>();
    QV4::ExecutionEngine *v4 = m->engine();
    if (!r) {
        v4->currentContext()->throwTypeError();
        return PropertyAttributes();
    }

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    {
        QQmlData *ddata = QQmlData::get(r->type, false);
        if (ddata && ddata->propertyCache)
            result = ddata->propertyCache->property(name.getPointer(), 0, 0);
        else
            result = QQmlPropertyCache::property(r->v8->engine(), r->type, name.getPointer(), 0, local);
    }
    return result ? Attr_Data : Attr_Invalid;
}

bool QmlValueTypeWrapper::isEqual(const QVariant& value)
{
    if (objectType == QmlValueTypeWrapper::Reference) {
        QmlValueTypeReference *reference = static_cast<QmlValueTypeReference *>(this);
        if (reference->object && readReferenceValue(reference)) {
            return reference->type->isEqual(value);
        } else {
            return false;
        }
    } else {
        Q_ASSERT(objectType == QmlValueTypeWrapper::Copy);
        QmlValueTypeCopy *copy = static_cast<QmlValueTypeCopy *>(this);
        type->setValue(copy->value);
        if (type->isEqual(value))
            return true;
        return (value == copy->value);
    }
}

ReturnedValue QmlValueTypeWrapper::method_toString(CallContext *ctx)
{
    Object *o = ctx->callData->thisObject.asObject();
    if (!o)
        return ctx->throwTypeError();
    QmlValueTypeWrapper *w = o->as<QmlValueTypeWrapper>();
    if (!w)
        return ctx->throwTypeError();

    if (w->objectType == QmlValueTypeWrapper::Reference) {
        QmlValueTypeReference *reference = static_cast<QmlValueTypeReference *>(w);
        if (reference->object && readReferenceValue(reference)) {
            return w->v8->toString(w->type->toString());
        } else {
            return QV4::Encode::undefined();
        }
    } else {
        Q_ASSERT(w->objectType == QmlValueTypeWrapper::Copy);
        QmlValueTypeCopy *copy = static_cast<QmlValueTypeCopy *>(w);
        w->type->setValue(copy->value);
        return w->v8->toString(w->type->toString());
    }
}

ReturnedValue QmlValueTypeWrapper::get(Managed *m, const StringRef name, bool *hasProperty)
{
    QmlValueTypeWrapper *r = m->as<QmlValueTypeWrapper>();
    QV4::ExecutionEngine *v4 = m->engine();
    if (!r)
        return v4->currentContext()->throwTypeError();

    // Note: readReferenceValue() can change the reference->type.
    if (r->objectType == QmlValueTypeWrapper::Reference) {
        QmlValueTypeReference *reference = static_cast<QmlValueTypeReference *>(r);

        if (!reference->object || !readReferenceValue(reference))
            return Primitive::undefinedValue().asReturnedValue();

    } else {
        Q_ASSERT(r->objectType == QmlValueTypeWrapper::Copy);

        QmlValueTypeCopy *copy = static_cast<QmlValueTypeCopy *>(r);

        r->type->setValue(copy->value);
    }

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    {
        QQmlData *ddata = QQmlData::get(r->type, false);
        if (ddata && ddata->propertyCache)
            result = ddata->propertyCache->property(name.getPointer(), 0, 0);
        else
            result = QQmlPropertyCache::property(r->v8->engine(), r->type, name, 0, local);
    }

    if (!result)
        return Object::get(m, name, hasProperty);

    if (result->isFunction()) {
        // calling a Q_INVOKABLE function of a value type
        QQmlContextData *qmlContext = QV4::QmlContextWrapper::callingContext(v4);
        return QV4::QObjectWrapper::getQmlProperty(v4->currentContext(), qmlContext, r->type, name.getPointer(), QV4::QObjectWrapper::IgnoreRevision);
    }

#define VALUE_TYPE_LOAD(metatype, cpptype, constructor) \
    if (result->propType == metatype) { \
        cpptype v; \
        void *args[] = { &v, 0 }; \
        r->type->qt_metacall(QMetaObject::ReadProperty, result->coreIndex, args); \
        return constructor(v); \
    }

    // These four types are the most common used by the value type wrappers
    VALUE_TYPE_LOAD(QMetaType::QReal, qreal, QV4::Encode);
    VALUE_TYPE_LOAD(QMetaType::Int, int, QV4::Encode);
    VALUE_TYPE_LOAD(QMetaType::QString, QString, r->v8->toString);
    VALUE_TYPE_LOAD(QMetaType::Bool, bool, QV4::Encode);

    QVariant v(result->propType, (void *)0);
    void *args[] = { v.data(), 0 };
    r->type->qt_metacall(QMetaObject::ReadProperty, result->coreIndex, args);
    return r->v8->fromVariant(v);
#undef VALUE_TYPE_ACCESSOR
}

void QmlValueTypeWrapper::put(Managed *m, const StringRef name, const ValueRef value)
{
    ExecutionEngine *v4 = m->engine();
    Scope scope(v4);
    if (scope.hasException())
        return;

    Scoped<QmlValueTypeWrapper> r(scope, m->as<QmlValueTypeWrapper>());
    if (!r) {
        v4->currentContext()->throwTypeError();
        return;
    }

    QByteArray propName = name->toQString().toUtf8();
    if (r->objectType == QmlValueTypeWrapper::Reference) {
        QmlValueTypeReference *reference = static_cast<QmlValueTypeReference *>(r.getPointer());
        QMetaProperty writebackProperty = reference->object->metaObject()->property(reference->property);

        if (!reference->object || !writebackProperty.isWritable() || !readReferenceValue(reference))
            return;

        // we lookup the index after readReferenceValue() since it can change the reference->type.
        int index = r->type->metaObject()->indexOfProperty(propName.constData());
        if (index == -1)
            return;
        QMetaProperty p = r->type->metaObject()->property(index);

        QQmlBinding *newBinding = 0;

        QV4::ScopedFunctionObject f(scope, value);
        if (f) {
            if (!f->bindingKeyFlag) {
                // assigning a JS function to a non-var-property is not allowed.
                QString error = QLatin1String("Cannot assign JavaScript function to value-type property");
                Scoped<String> e(scope, r->v8->toString(error));
                v4->currentContext()->throwError(e);
                return;
            }

            QQmlContextData *context = r->v8->callingContext();

            QQmlPropertyData cacheData;
            cacheData.setFlags(QQmlPropertyData::IsWritable |
                               QQmlPropertyData::IsValueTypeVirtual);
            cacheData.propType = reference->object->metaObject()->property(reference->property).userType();
            cacheData.coreIndex = reference->property;
            cacheData.valueTypeFlags = 0;
            cacheData.valueTypeCoreIndex = index;
            cacheData.valueTypePropType = p.userType();

            QV4::Scoped<QQmlBindingFunction> bindingFunction(scope, f);
            bindingFunction->initBindingLocation();

            newBinding = new QQmlBinding(value, reference->object, context);
            newBinding->setTarget(reference->object, cacheData, context);
        }

        QQmlAbstractBinding *oldBinding =
            QQmlPropertyPrivate::setBinding(reference->object, reference->property, index, newBinding);
        if (oldBinding)
            oldBinding->destroy();

        if (!f) {
            QVariant v = r->v8->toVariant(value, -1);

            if (p.isEnumType() && (QMetaType::Type)v.type() == QMetaType::Double)
                v = v.toInt();

            p.write(reference->type, v);

            if (writebackProperty.userType() == QMetaType::QVariant) {
                QVariant variantReferenceValue = r->type->value();
                reference->type->writeVariantValue(reference->object, reference->property, 0, &variantReferenceValue);
            } else {
                reference->type->write(reference->object, reference->property, 0);
            }
        }

    } else {
        Q_ASSERT(r->objectType == QmlValueTypeWrapper::Copy);

        QmlValueTypeCopy *copy = static_cast<QmlValueTypeCopy *>(r.getPointer());

        int index = r->type->metaObject()->indexOfProperty(propName.constData());
        if (index == -1)
            return;

        QVariant v = r->v8->toVariant(value, -1);

        r->type->setValue(copy->value);
        QMetaProperty p = r->type->metaObject()->property(index);
        p.write(r->type, v);
        copy->value = r->type->value();
    }
}

QT_END_NAMESPACE
