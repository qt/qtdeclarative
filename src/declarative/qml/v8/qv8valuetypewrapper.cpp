/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
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

#include <private/qdeclarativevaluetype_p.h>
#include <private/qdeclarativebinding_p.h>

QT_BEGIN_NAMESPACE

class QV8ValueTypeResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(ValueTypeType);

public:
    enum ObjectType { Reference, Copy };

    QV8ValueTypeResource(QV8Engine *engine, ObjectType objectType);

    ObjectType objectType;
    QDeclarativeValueType *type;
};

class QV8ValueTypeReferenceResource : public QV8ValueTypeResource
{
public:
    QV8ValueTypeReferenceResource(QV8Engine *engine);

    QDeclarativeGuard<QObject> object;
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
}

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
}

v8::Local<v8::Object> QV8ValueTypeWrapper::newValueType(QObject *object, int property, QDeclarativeValueType *type)
{
    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_constructor->NewInstance(); 
    QV8ValueTypeReferenceResource *r = new QV8ValueTypeReferenceResource(m_engine);
    r->type = type; r->object = object; r->property = property;
    rv->SetExternalResource(r);
    return rv;
}

v8::Local<v8::Object> QV8ValueTypeWrapper::newValueType(const QVariant &value, QDeclarativeValueType *type)
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

    // XXX This is horribly inefficient.  Sadly people seem to have taken a liking to 
    // value type properties, so we should probably try and optimize it a little.
    // We should probably just replace all value properties with dedicated accessors.

    QByteArray propName = r->engine->toString(property).toUtf8();
    if (propName == QByteArray("toString")) {
        return r->engine->valueTypeWrapper()->m_toString;
    }

    int index = r->type->metaObject()->indexOfProperty(propName.constData());
    if (index == -1)
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

    QMetaProperty prop = r->type->metaObject()->property(index);
    QVariant result = prop.read(r->type);

    return r->engine->fromVariant(result);
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

        QDeclarativeBinding *newBinding = 0;

        if (value->IsFunction()) {
            QDeclarativeContextData *context = r->engine->callingContext();
            v8::Handle<v8::Function> function = v8::Handle<v8::Function>::Cast(value);

            QDeclarativePropertyCache::Data cacheData;
            cacheData.setFlags(QDeclarativePropertyCache::Data::IsWritable);
            cacheData.propType = reference->object->metaObject()->property(reference->property).userType();
            cacheData.coreIndex = reference->property;

            QDeclarativePropertyCache::ValueTypeData valueTypeData;
            valueTypeData.valueTypeCoreIdx = index;
            valueTypeData.valueTypePropType = p.userType();

            v8::Local<v8::StackTrace> trace = 
                v8::StackTrace::CurrentStackTrace(1, 
                        (v8::StackTrace::StackTraceOptions)(v8::StackTrace::kLineNumber | 
                                                            v8::StackTrace::kScriptName));
            v8::Local<v8::StackFrame> frame = trace->GetFrame(0);
            int lineNumber = frame->GetLineNumber();
            QString url = r->engine->toString(frame->GetScriptName());

            newBinding = new QDeclarativeBinding(&function, reference->object, context);
            newBinding->setSourceLocation(url, lineNumber);
            newBinding->setTarget(QDeclarativePropertyPrivate::restore(cacheData, valueTypeData, 
                                                                       reference->object, context));
            newBinding->setEvaluateFlags(newBinding->evaluateFlags() | QDeclarativeBinding::RequiresThisObject);
        }

        QDeclarativeAbstractBinding *oldBinding = 
            QDeclarativePropertyPrivate::setBinding(reference->object, reference->property, index, newBinding);
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
