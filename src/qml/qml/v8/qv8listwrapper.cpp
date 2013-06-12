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

#include "qv8listwrapper_p.h"
#include "qv8engine_p.h"
#include <private/qqmllist_p.h>

QT_BEGIN_NAMESPACE

class QV8ListResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(ListType)
public:
    QV8ListResource(QV8Engine *engine) : QV8ObjectResource(engine) {}

    QQmlGuard<QObject> object;
    QQmlListProperty<QObject> property;
    int propertyType;
};

QV8ListWrapper::QV8ListWrapper()
: m_engine(0)
{
}

QV8ListWrapper::~QV8ListWrapper()
{
}

void QV8ListWrapper::init(QV8Engine *engine)
{
    m_engine = engine;
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetFallbackPropertyHandler(Getter, Setter, 0, 0, Enumerator);
    ft->InstanceTemplate()->SetIndexedPropertyHandler(IndexedGetter);
    ft->InstanceTemplate()->SetAccessor(v8::String::New("length"), LengthGetter, 0, 
                                        v8::Handle<v8::Value>(), v8::DEFAULT, 
                                        v8::PropertyAttribute(v8::ReadOnly | v8::DontDelete | v8::DontEnum));
    ft->InstanceTemplate()->SetHasExternalResource(true);
    m_constructor = qPersistentNew<v8::Function>(ft->GetFunction());
}

void QV8ListWrapper::destroy()
{
    qPersistentDispose(m_constructor);
}

v8::Handle<v8::Value> QV8ListWrapper::newList(QObject *object, int propId, int propType)
{
    if (!object || propId == -1)
        return v8::Null();

    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_constructor->NewInstance(); 
    QV8ListResource *r = new QV8ListResource(m_engine);
    r->object = object;
    r->propertyType = propType;
    void *args[] = { &r->property, 0 };
    QMetaObject::metacall(object, QMetaObject::ReadProperty, propId, args);
    rv->SetExternalResource(r);
    return rv;
}

v8::Handle<v8::Value> QV8ListWrapper::newList(const QQmlListProperty<QObject> &prop, int propType)
{
    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_constructor->NewInstance(); 
    QV8ListResource *r = new QV8ListResource(m_engine);
    r->object = prop.object;
    r->property = prop;
    r->propertyType = propType;
    rv->SetExternalResource(r);
    return rv;
}

QVariant QV8ListWrapper::toVariant(v8::Handle<v8::Object> obj)
{
    QV8ListResource *resource = v8_resource_cast<QV8ListResource>(obj);
    if (resource) return toVariant(resource);
    else return QVariant();
}

QVariant QV8ListWrapper::toVariant(QV8ObjectResource *r)
{
    Q_ASSERT(r->resourceType() == QV8ObjectResource::ListType);
    QV8ListResource *resource = static_cast<QV8ListResource *>(r);

    if (!resource->object)
        return QVariant();

    return QVariant::fromValue(QQmlListReferencePrivate::init(resource->property, resource->propertyType, 
                                                                      m_engine->engine()));
}

v8::Handle<v8::Value> QV8ListWrapper::Getter(v8::Local<v8::String> property, 
                                             const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    Q_UNUSED(info);
    return v8::Handle<v8::Value>();
}

v8::Handle<v8::Value> QV8ListWrapper::Setter(v8::Local<v8::String> property, 
                                             v8::Local<v8::Value> value,
                                             const v8::AccessorInfo &info)
{
    Q_UNUSED(property);
    Q_UNUSED(info);
    return value;
}

v8::Handle<v8::Value> QV8ListWrapper::IndexedGetter(uint32_t index, const v8::AccessorInfo &info)
{
    QV8ListResource *resource = v8_resource_cast<QV8ListResource>(info.This());

    if (!resource || resource->object.isNull()) return v8::Undefined();

    quint32 count = resource->property.count?resource->property.count(&resource->property):0;
    if (index < count && resource->property.at) {
        return resource->engine->newQObject(resource->property.at(&resource->property, index));
    } else {
        return v8::Undefined();
    }
}

v8::Handle<v8::Value> QV8ListWrapper::LengthGetter(v8::Local<v8::String> property, 
                                                   const v8::AccessorInfo &info)
{
    Q_UNUSED(property);

    QV8ListResource *resource = v8_resource_cast<QV8ListResource>(info.This());

    if (!resource || resource->object.isNull()) return v8::Undefined();

    quint32 count = resource->property.count?resource->property.count(&resource->property):0;

    return v8::Integer::NewFromUnsigned(count);
}

v8::Handle<v8::Array> QV8ListWrapper::Enumerator(const v8::AccessorInfo &info)
{
    QV8ListResource *resource = v8_resource_cast<QV8ListResource>(info.This());

    if (!resource || resource->object.isNull()) return v8::Array::New();

    quint32 count = resource->property.count?resource->property.count(&resource->property):0;

    v8::Local<v8::Array> rv = v8::Array::New(count);

    for (uint ii = 0; ii < count; ++ii)
        rv->Set(ii, v8::Number::New(ii));

    return rv;
}

QT_END_NAMESPACE
