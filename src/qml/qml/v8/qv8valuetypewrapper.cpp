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

#include "qv8valuetypewrapper_p.h"
#include "qv8engine_p.h"

#include <private/qqmlvaluetype_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qqmlglobal_p.h>

QT_BEGIN_NAMESPACE

class QV8ValueTypeResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(ValueTypeType)

public:
    enum ObjectType { Reference, Copy };

    QV8ValueTypeResource(QV8Engine *engine, ObjectType objectType);

    ObjectType objectType;
    QQmlValueType *type;
};

class QV8ValueTypeReferenceResource : public QV8ValueTypeResource
{
public:
    QV8ValueTypeReferenceResource(QV8Engine *engine);

    QQmlGuard<QObject> object;
    int property;
};

class QV8ValueTypeCopyResource : public QV8ValueTypeResource
{
public:
    QV8ValueTypeCopyResource(QV8Engine *engine);

    QVariant value;
};

QV8ValueTypeResource::QV8ValueTypeResource(QV8Engine *engine, ObjectType objectType)
: QV8ObjectResource(engine), objectType(objectType)
{
}

QV8ValueTypeReferenceResource::QV8ValueTypeReferenceResource(QV8Engine *engine)
: QV8ValueTypeResource(engine, Reference)
{
}

QV8ValueTypeCopyResource::QV8ValueTypeCopyResource(QV8Engine *engine)
: QV8ValueTypeResource(engine, Copy)
{
}

QV8ValueTypeWrapper::QV8ValueTypeWrapper()
: m_engine(0)
{
}

QV8ValueTypeWrapper::~QV8ValueTypeWrapper()
{
}

void QV8ValueTypeWrapper::destroy()
{
    qPersistentDispose(m_toString);
    qPersistentDispose(m_constructor);
    qPersistentDispose(m_toStringSymbol);
}

static quint32 toStringHash = -1;

void QV8ValueTypeWrapper::init(QV8Engine *engine)
{
    m_engine = engine;
    m_toString = qPersistentNew<v8::Function>(v8::FunctionTemplate::New(ToString)->GetFunction());
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetNamedPropertyHandler(Getter, Setter);
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->InstanceTemplate()->MarkAsUseUserObjectComparison();
    ft->InstanceTemplate()->SetAccessor(v8::String::New("toString"), ToStringGetter, 0,
                                        m_toString, v8::DEFAULT,
                                        v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
    m_constructor = qPersistentNew<v8::Function>(ft->GetFunction());

    m_toStringSymbol = qPersistentNew<v8::String>(v8::String::NewSymbol("toString"));
    m_toStringString = QHashedV8String(m_toStringSymbol);
    toStringHash = m_toStringString.hash();
}

v8::Local<v8::Object> QV8ValueTypeWrapper::newValueType(QObject *object, int property, QQmlValueType *type)
{
    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_constructor->NewInstance(); 
    QV8ValueTypeReferenceResource *r = new QV8ValueTypeReferenceResource(m_engine);
    r->type = type; r->object = object; r->property = property;
    rv->SetExternalResource(r);
    return rv;
}

v8::Local<v8::Object> QV8ValueTypeWrapper::newValueType(const QVariant &value, QQmlValueType *type)
{
    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_constructor->NewInstance(); 
    QV8ValueTypeCopyResource *r = new QV8ValueTypeCopyResource(m_engine);
    r->type = type; r->value = value;
    rv->SetExternalResource(r);
    return rv;
}

static bool readReferenceValue(QV8ValueTypeReferenceResource *reference)
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

bool QV8ValueTypeWrapper::isValueType(v8::Handle<v8::Object> obj) const
{
    QV8ValueTypeResource *r = v8_resource_cast<QV8ValueTypeResource>(obj);
    return (r != 0);
}

QVariant QV8ValueTypeWrapper::toVariant(v8::Handle<v8::Object> obj, int typeHint, bool *succeeded)
{
    // NOTE: obj must not be an external resource object (ie, wrapper object)
    // instead, it is a normal js object which one of the value-type providers
    // may know how to convert to the given type.
    return QQml_valueTypeProvider()->createVariantFromJsObject(typeHint, QQmlV8Handle::fromHandle(obj), m_engine, succeeded);
}

QVariant QV8ValueTypeWrapper::toVariant(v8::Handle<v8::Object> obj)
{
    QV8ValueTypeResource *r = v8_resource_cast<QV8ValueTypeResource>(obj);
    if (r) return toVariant(r);
    else return QVariant();
}

QVariant QV8ValueTypeWrapper::toVariant(QV8ObjectResource *r)
{
    Q_ASSERT(r->resourceType() == QV8ObjectResource::ValueTypeType);
    QV8ValueTypeResource *resource = static_cast<QV8ValueTypeResource *>(r);
    
    if (resource->objectType == QV8ValueTypeResource::Reference) {
        QV8ValueTypeReferenceResource *reference = static_cast<QV8ValueTypeReferenceResource *>(resource);

        if (reference->object && readReferenceValue(reference)) {
            return reference->type->value();
        } else {
            return QVariant();
        }

    } else {
        Q_ASSERT(resource->objectType == QV8ValueTypeResource::Copy);

        QV8ValueTypeCopyResource *copy = static_cast<QV8ValueTypeCopyResource *>(resource);

        return copy->value;
    }
}

bool QV8ValueTypeWrapper::isEqual(QV8ObjectResource *r, const QVariant& value)
{
    Q_ASSERT(r->resourceType() == QV8ObjectResource::ValueTypeType);
    QV8ValueTypeResource *resource = static_cast<QV8ValueTypeResource *>(r);

    if (resource->objectType == QV8ValueTypeResource::Reference) {
        QV8ValueTypeReferenceResource *reference = static_cast<QV8ValueTypeReferenceResource *>(resource);
        if (reference->object && readReferenceValue(reference)) {
            return reference->type->isEqual(value);
        } else {
            return false;
        }
    } else {
        Q_ASSERT(resource->objectType == QV8ValueTypeResource::Copy);
        QV8ValueTypeCopyResource *copy = static_cast<QV8ValueTypeCopyResource *>(resource);
        resource->type->setValue(copy->value);
        if (resource->type->isEqual(value))
            return true;
        return (value == copy->value);
    }
}

v8::Handle<v8::Value> QV8ValueTypeWrapper::ToStringGetter(v8::Local<v8::String> property,
                                                        const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    return info.Data();
}

v8::Handle<v8::Value> QV8ValueTypeWrapper::ToString(const v8::Arguments &args)
{
    QV8ValueTypeResource *resource = v8_resource_cast<QV8ValueTypeResource>(args.This());
    if (resource) {
        if (resource->objectType == QV8ValueTypeResource::Reference) {
            QV8ValueTypeReferenceResource *reference = static_cast<QV8ValueTypeReferenceResource *>(resource);
            if (reference->object && readReferenceValue(reference)) {
                return resource->engine->toString(resource->type->toString());
            } else {
                return v8::Undefined();
            }
        } else {
            Q_ASSERT(resource->objectType == QV8ValueTypeResource::Copy);
            QV8ValueTypeCopyResource *copy = static_cast<QV8ValueTypeCopyResource *>(resource);
            resource->type->setValue(copy->value);
            return resource->engine->toString(resource->type->toString());
        }
    } else {
        return v8::Undefined();
    }
}

v8::Handle<v8::Value> QV8ValueTypeWrapper::Getter(v8::Local<v8::String> property, 
                                                  const v8::AccessorInfo &info)
{
    QV8ValueTypeResource *r =  v8_resource_cast<QV8ValueTypeResource>(info.This());
    if (!r) return v8::Handle<v8::Value>();

    QHashedV8String propertystring(property);

    {
        // Comparing the hash first actually makes a measurable difference here, at least on x86
        quint32 hash = propertystring.hash();
        if (hash == toStringHash &&
            r->engine->valueTypeWrapper()->m_toStringString == propertystring) {
            return r->engine->valueTypeWrapper()->m_toString;
        }
    }

    // Note: readReferenceValue() can change the reference->type.
    if (r->objectType == QV8ValueTypeResource::Reference) {
        QV8ValueTypeReferenceResource *reference = static_cast<QV8ValueTypeReferenceResource *>(r);

        if (!reference->object || !readReferenceValue(reference))
            return v8::Handle<v8::Value>();

    } else {
        Q_ASSERT(r->objectType == QV8ValueTypeResource::Copy);

        QV8ValueTypeCopyResource *copy = static_cast<QV8ValueTypeCopyResource *>(r);

        r->type->setValue(copy->value);
    }

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    {
        QQmlData *ddata = QQmlData::get(r->type, false);
        if (ddata && ddata->propertyCache)
            result = ddata->propertyCache->property(propertystring, 0, 0);
        else
            result = QQmlPropertyCache::property(r->engine->engine(), r->type,
                                                         propertystring, 0, local);
    }

    if (!result)
        return v8::Handle<v8::Value>();

    if (result->isFunction()) {
        // calling a Q_INVOKABLE function of a value type
        QQmlContextData *context = r->engine->callingContext();
        return r->engine->qobjectWrapper()->getProperty(r->type, propertystring, context, QV8QObjectWrapper::IgnoreRevision);
    }

#define VALUE_TYPE_LOAD(metatype, cpptype, constructor) \
    if (result->propType == metatype) { \
        cpptype v; \
        void *args[] = { &v, 0 }; \
        r->type->qt_metacall(QMetaObject::ReadProperty, result->coreIndex, args); \
        return constructor(v); \
    }

    // These four types are the most common used by the value type wrappers
    VALUE_TYPE_LOAD(QMetaType::QReal, qreal, v8::Number::New);
    VALUE_TYPE_LOAD(QMetaType::Int, int, v8::Integer::New);
    VALUE_TYPE_LOAD(QMetaType::QString, QString, r->engine->toString);
    VALUE_TYPE_LOAD(QMetaType::Bool, bool, v8::Boolean::New);

    QVariant v(result->propType, (void *)0);
    void *args[] = { v.data(), 0 };
    r->type->qt_metacall(QMetaObject::ReadProperty, result->coreIndex, args);
    return r->engine->fromVariant(v);
#undef VALUE_TYPE_ACCESSOR
}

v8::Handle<v8::Value> QV8ValueTypeWrapper::Setter(v8::Local<v8::String> property, 
                                                  v8::Local<v8::Value> value,
                                                  const v8::AccessorInfo &info)
{
    QV8ValueTypeResource *r =  v8_resource_cast<QV8ValueTypeResource>(info.This());
    if (!r) return value;

    QByteArray propName = r->engine->toString(property).toUtf8();
    if (r->objectType == QV8ValueTypeResource::Reference) {
        QV8ValueTypeReferenceResource *reference = static_cast<QV8ValueTypeReferenceResource *>(r);
        QMetaProperty writebackProperty = reference->object->metaObject()->property(reference->property);

        if (!reference->object || !writebackProperty.isWritable() || !readReferenceValue(reference))
            return value;

        // we lookup the index after readReferenceValue() since it can change the reference->type.
        int index = r->type->metaObject()->indexOfProperty(propName.constData());
        if (index == -1)
            return value;
        QMetaProperty p = r->type->metaObject()->property(index);

        QQmlBinding *newBinding = 0;

        if (value->IsFunction()) {
            if (value->ToObject()->GetHiddenValue(r->engine->bindingFlagKey()).IsEmpty()) {
                // assigning a JS function to a non-var-property is not allowed.
                QString error = QLatin1String("Cannot assign JavaScript function to value-type property");
                v8::ThrowException(v8::Exception::Error(r->engine->toString(error)));
                return value;
            }

            QQmlContextData *context = r->engine->callingContext();
            v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(value);

            QQmlPropertyData cacheData;
            cacheData.setFlags(QQmlPropertyData::IsWritable |
                               QQmlPropertyData::IsValueTypeVirtual);
            cacheData.propType = reference->object->metaObject()->property(reference->property).userType();
            cacheData.coreIndex = reference->property;
            cacheData.valueTypeFlags = 0;
            cacheData.valueTypeCoreIndex = index;
            cacheData.valueTypePropType = p.userType();

            v8::Local<v8::StackTrace> trace = 
                v8::StackTrace::CurrentStackTrace(1, 
                        (v8::StackTrace::StackTraceOptions)(v8::StackTrace::kLineNumber | 
                                                            v8::StackTrace::kScriptName));
            v8::Local<v8::StackFrame> frame = trace->GetFrame(0);
            int lineNumber = frame->GetLineNumber();
            int columnNumber = frame->GetColumn();
            QString url = r->engine->toString(frame->GetScriptName());

            newBinding = new QQmlBinding(&function, reference->object, context,
                                         url, qmlSourceCoordinate(lineNumber), qmlSourceCoordinate(columnNumber));
            newBinding->setTarget(reference->object, cacheData, context);
            newBinding->setEvaluateFlags(newBinding->evaluateFlags() |
                                         QQmlBinding::RequiresThisObject);
        }

        QQmlAbstractBinding *oldBinding = 
            QQmlPropertyPrivate::setBinding(reference->object, reference->property, index, newBinding);
        if (oldBinding)
            oldBinding->destroy();

        if (!value->IsFunction()) {
            QVariant v = r->engine->toVariant(value, -1);

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
        Q_ASSERT(r->objectType == QV8ValueTypeResource::Copy);

        QV8ValueTypeCopyResource *copy = static_cast<QV8ValueTypeCopyResource *>(r);

        int index = r->type->metaObject()->indexOfProperty(propName.constData());
        if (index == -1)
            return value;

        QVariant v = r->engine->toVariant(value, -1);

        r->type->setValue(copy->value);
        QMetaProperty p = r->type->metaObject()->property(index);
        p.write(r->type, v);
        copy->value = r->type->value();
    }

    return value;
}

QT_END_NAMESPACE
