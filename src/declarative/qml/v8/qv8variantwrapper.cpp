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

#include "qv8variantwrapper_p.h"
#include "qv8engine_p.h"
#include "qdeclarativeengine_p.h"

QT_BEGIN_NAMESPACE

class QV8VariantResource : public QV8ObjectResource, 
                           public QDeclarativeEnginePrivate::ScarceResourceData
{
    V8_RESOURCE_TYPE(VariantType);
public:
    QV8VariantResource(QV8Engine *engine, const QVariant &data);
};

QV8VariantResource::QV8VariantResource(QV8Engine *engine, const QVariant &data)
: QV8ObjectResource(engine), QDeclarativeEnginePrivate::ScarceResourceData(data)
{
}

QV8VariantWrapper::QV8VariantWrapper()
: m_engine(0)
{
}

QV8VariantWrapper::~QV8VariantWrapper()
{
}

void QV8VariantWrapper::init(QV8Engine *engine)
{
    m_engine = engine;
    m_toString = qPersistentNew<v8::Function>(v8::FunctionTemplate::New(ToString)->GetFunction());

    {
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetFallbackPropertyHandler(Getter, Setter);
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->InstanceTemplate()->MarkAsUseUserObjectComparison();
    ft->InstanceTemplate()->SetAccessor(v8::String::New("toString"), ToStringGetter, 0, 
                                        m_toString, v8::DEFAULT, 
                                        v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
    m_constructor = qPersistentNew<v8::Function>(ft->GetFunction());
    }
    {
    m_preserve = qPersistentNew<v8::Function>(v8::FunctionTemplate::New(Preserve)->GetFunction());
    m_destroy = qPersistentNew<v8::Function>(v8::FunctionTemplate::New(Destroy)->GetFunction());
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetFallbackPropertyHandler(Getter, Setter);
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->InstanceTemplate()->MarkAsUseUserObjectComparison();
    ft->InstanceTemplate()->SetAccessor(v8::String::New("preserve"), PreserveGetter, 0, 
                                        m_preserve, v8::DEFAULT, 
                                        v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("destroy"), DestroyGetter, 0, 
                                        m_destroy, v8::DEFAULT, 
                                        v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("toString"), ToStringGetter, 0, 
                                        m_toString, v8::DEFAULT, 
                                        v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
    m_scarceConstructor = qPersistentNew<v8::Function>(ft->GetFunction());
    }

}

void QV8VariantWrapper::destroy()
{
    qPersistentDispose(m_toString);
    qPersistentDispose(m_destroy);
    qPersistentDispose(m_preserve);
    qPersistentDispose(m_scarceConstructor);
    qPersistentDispose(m_constructor);
}

v8::Local<v8::Object> QV8VariantWrapper::newVariant(const QVariant &value)
{
    bool scarceResource = value.type() == QVariant::Pixmap ||
                          value.type() == QVariant::Image;

    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv;
    QV8VariantResource *r = new QV8VariantResource(m_engine, value);

    if (scarceResource) {
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(m_engine->engine());
        Q_ASSERT(ep->scarceResourcesRefCount);
        rv = m_scarceConstructor->NewInstance();
        ep->scarceResources.insert(r);
    } else {
        rv = m_constructor->NewInstance();
    }

    rv->SetExternalResource(r);
    return rv;
}

bool QV8VariantWrapper::isVariant(v8::Handle<v8::Value> value)
{
    return value->IsObject() && v8_resource_cast<QV8VariantResource>(value->ToObject());
}

QVariant QV8VariantWrapper::toVariant(v8::Handle<v8::Object> obj)
{
    QV8VariantResource *r =  v8_resource_cast<QV8VariantResource>(obj);
    return r?r->data:QVariant();
}

QVariant QV8VariantWrapper::toVariant(QV8ObjectResource *r)
{
    Q_ASSERT(r->resourceType() == QV8ObjectResource::VariantType);
    return static_cast<QV8VariantResource *>(r)->data;
}

v8::Handle<v8::Value> QV8VariantWrapper::Getter(v8::Local<v8::String> property, 
                                                const v8::AccessorInfo &info)
{
    return v8::Undefined();
}

v8::Handle<v8::Value> QV8VariantWrapper::Setter(v8::Local<v8::String> property, 
                                                v8::Local<v8::Value> value,
                                                const v8::AccessorInfo &info)
{
    return v8::Undefined();
}

v8::Handle<v8::Value> QV8VariantWrapper::PreserveGetter(v8::Local<v8::String> property, 
                                                        const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    return info.Data();
}

v8::Handle<v8::Value> QV8VariantWrapper::DestroyGetter(v8::Local<v8::String> property, 
                                                       const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    return info.Data();
}

v8::Handle<v8::Value> QV8VariantWrapper::ToStringGetter(v8::Local<v8::String> property, 
                                                        const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    return info.Data();
}

v8::Handle<v8::Value> QV8VariantWrapper::Preserve(const v8::Arguments &args)
{
    QV8VariantResource *resource = v8_resource_cast<QV8VariantResource>(args.This());
    if (resource) {
        resource->node.remove();
    }
    return v8::Undefined();
}

v8::Handle<v8::Value> QV8VariantWrapper::Destroy(const v8::Arguments &args)
{
    QV8VariantResource *resource = v8_resource_cast<QV8VariantResource>(args.This());
    if (resource) {
        resource->data = QVariant();
        resource->node.remove();
    }
    return v8::Undefined();
}

v8::Handle<v8::Value> QV8VariantWrapper::ToString(const v8::Arguments &args)
{
    QV8VariantResource *resource = v8_resource_cast<QV8VariantResource>(args.This());
    if (resource) {
        QString result = resource->data.toString();
        if (result.isEmpty() && !resource->data.canConvert(QVariant::String))
            result = QString::fromLatin1("QVariant(%0)").arg(QString::fromLatin1(resource->data.typeName()));
        return resource->engine->toString(result);
    } else {
        return v8::Undefined();
    }
}

QT_END_NAMESPACE
