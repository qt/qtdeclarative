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

#include "qv8variantwrapper_p.h"
#include "qv8variantresource_p.h"
#include "qv8engine_p.h"
#include <private/qqmlengine_p.h>

QT_BEGIN_NAMESPACE

QV8VariantResource::QV8VariantResource(QV8Engine *engine, const QVariant &data)
: QV8ObjectResource(engine), QQmlEnginePrivate::ScarceResourceData(data), m_isScarceResource(false), m_vmePropertyReferenceCount(0)
{
}

void QV8VariantResource::addVmePropertyReference()
{
    if (m_isScarceResource && ++m_vmePropertyReferenceCount == 1) {
        // remove from the ep->scarceResources list
        // since it is now no longer eligible to be
        // released automatically by the engine.
        node.remove();
    }
}

void QV8VariantResource::removeVmePropertyReference()
{
    if (m_isScarceResource && --m_vmePropertyReferenceCount == 0) {
        // and add to the ep->scarceResources list
        // since it is now eligible to be released
        // automatically by the engine.
        QQmlEnginePrivate::get(engine->engine())->scarceResources.insert(this);
    }
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
    m_valueOf = qPersistentNew<v8::Function>(v8::FunctionTemplate::New(ValueOf)->GetFunction());

    {
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetFallbackPropertyHandler(Getter, Setter);
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->InstanceTemplate()->MarkAsUseUserObjectComparison();
    ft->InstanceTemplate()->SetAccessor(v8::String::New("toString"), ToStringGetter, 0, 
                                        m_toString, v8::DEFAULT, 
                                        v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
    ft->InstanceTemplate()->SetAccessor(v8::String::New("valueOf"), ValueOfGetter, 0,
                                        m_valueOf, v8::DEFAULT,
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
    ft->InstanceTemplate()->SetAccessor(v8::String::New("valueOf"), ValueOfGetter, 0,
                                        m_valueOf, v8::DEFAULT,
                                        v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete));
    m_scarceConstructor = qPersistentNew<v8::Function>(ft->GetFunction());
    }

}

void QV8VariantWrapper::destroy()
{
    qPersistentDispose(m_valueOf);
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
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(m_engine->engine());
        Q_ASSERT(ep->scarceResourcesRefCount);
        rv = m_scarceConstructor->NewInstance();
        r->m_isScarceResource = true;
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

QVariant &QV8VariantWrapper::variantValue(v8::Handle<v8::Value> value)
{
    Q_ASSERT(isVariant(value));
    QV8VariantResource *r =  v8_resource_cast<QV8VariantResource>(value->ToObject());
    return static_cast<QV8VariantResource *>(r)->data;
}

v8::Handle<v8::Value> QV8VariantWrapper::Getter(v8::Local<v8::String> /* property */,
                                                const v8::AccessorInfo & /* info */)
{
    return v8::Handle<v8::Value>();
}

v8::Handle<v8::Value> QV8VariantWrapper::Setter(v8::Local<v8::String> /* property */,
                                                v8::Local<v8::Value> value,
                                                const v8::AccessorInfo & /* info */)
{
    return value;
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

v8::Handle<v8::Value> QV8VariantWrapper::ValueOfGetter(v8::Local<v8::String> property,
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

v8::Handle<v8::Value> QV8VariantWrapper::ValueOf(const v8::Arguments &args)
{
    QV8VariantResource *resource = v8_resource_cast<QV8VariantResource>(args.This());
    if (resource) {
        QVariant v = resource->data;
        switch (v.type()) {
        case QVariant::Invalid:
            return v8::Undefined();
        case QVariant::String:
            return resource->engine->toString(v.toString());
        case QVariant::Int:
        case QVariant::Double:
        case QVariant::UInt:
            return v8::Number::New(v.toDouble());
        case QVariant::Bool:
            return v8::Boolean::New(v.toBool());
        default:
            break;
        }
    }
    return args.This();
}

QT_END_NAMESPACE
