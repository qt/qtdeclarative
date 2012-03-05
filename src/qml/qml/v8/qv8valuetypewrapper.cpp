/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv8valuetypewrapper_p.h"
#include "qv8engine_p.h"

#include <private/qqmlvaluetype_p.h>
#include <private/qqmlbinding_p.h>

QT_BEGIN_NAMESPACE

class QV8ValueTypeResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(ValueTypeType);

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

        if (reference->object) {
            reference->type->read(reference->object, reference->property);
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
        if (reference->object) {
            reference->type->read(reference->object, reference->property);
            return reference->type->isEqual(value);
        } else {
            return false;
        }
    } else {
        Q_ASSERT(resource->objectType == QV8ValueTypeResource::Copy);
        QV8ValueTypeCopyResource *copy = static_cast<QV8ValueTypeCopyResource *>(resource);
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
            if (reference->object) {
                reference->type->read(reference->object, reference->property);
                return resource->engine->toString(resource->type->toString());
            } else {
                return v8::Undefined();
            }
        } else {
            Q_ASSERT(resource->objectType == QV8ValueTypeResource::Copy);
            QV8ValueTypeCopyResource *copy = static_cast<QV8ValueTypeCopyResource *>(resource);
            QString result = copy->value.toString();
            if (result.isEmpty() && !copy->value.canConvert(QVariant::String)) {
                result = QString::fromLatin1("QVariant(%0)").arg(QString::fromLatin1(copy->value.typeName()));
            }
            return resource->engine->toString(result);
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

    QQmlPropertyData local;
    QQmlPropertyData *result = 0;
    {
        QQmlData *ddata = QQmlData::get(r->type, false);
        if (ddata && ddata->propertyCache)
            result = ddata->propertyCache->property(propertystring);
        else
            result = QQmlPropertyCache::property(r->engine->engine(), r->type,
                                                         propertystring, local);
    }

    if (!result)
        return v8::Handle<v8::Value>();

    if (r->objectType == QV8ValueTypeResource::Reference) {
        QV8ValueTypeReferenceResource *reference = static_cast<QV8ValueTypeReferenceResource *>(r);

        if (!reference->object)
            return v8::Handle<v8::Value>();

        r->type->read(reference->object, reference->property);
    } else {
        Q_ASSERT(r->objectType == QV8ValueTypeResource::Copy);

        QV8ValueTypeCopyResource *copy = static_cast<QV8ValueTypeCopyResource *>(r);

        r->type->setValue(copy->value);
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
    int index = r->type->metaObject()->indexOfProperty(propName.constData());
    if (index == -1)
        return value;

    if (r->objectType == QV8ValueTypeResource::Reference) {
        QV8ValueTypeReferenceResource *reference = static_cast<QV8ValueTypeReferenceResource *>(r);

        if (!reference->object || 
            !reference->object->metaObject()->property(reference->property).isWritable())
            return value;

        r->type->read(reference->object, reference->property);
        QMetaProperty p = r->type->metaObject()->property(index);

        QQmlBinding *newBinding = 0;

        if (value->IsFunction()) {
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

            newBinding = new QQmlBinding(&function, reference->object, context);
            newBinding->setSourceLocation(url, lineNumber, columnNumber);
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

            reference->type->write(reference->object, reference->property, 0);
        }

    } else {
        Q_ASSERT(r->objectType == QV8ValueTypeResource::Copy);

        QV8ValueTypeCopyResource *copy = static_cast<QV8ValueTypeCopyResource *>(r);

        QVariant v = r->engine->toVariant(value, -1);

        r->type->setValue(copy->value);
        QMetaProperty p = r->type->metaObject()->property(index);
        p.write(r->type, v);
        copy->value = r->type->value();
    }

    return value;
}

QT_END_NAMESPACE
