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

#include "qv8contextwrapper_p.h"
#include "qv8engine_p.h"

#include <private/qdeclarativeengine_p.h>
#include <private/qdeclarativecontext_p.h>

QT_BEGIN_NAMESPACE

class QV8TypeResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(TypeType);

public:
    QV8TypeResource(QV8Engine *engine);
    virtual ~QV8TypeResource();

    QV8TypeWrapper::TypeNameMode mode;

    QDeclarativeGuard<QObject> object;
    QDeclarativeType *type;
    QDeclarativeTypeNameCache *typeNamespace;
};

QV8TypeResource::QV8TypeResource(QV8Engine *engine)
: QV8ObjectResource(engine), mode(QV8TypeWrapper::IncludeEnums), type(0), typeNamespace(0)
{
}

QV8TypeResource::~QV8TypeResource()
{
    if (typeNamespace) typeNamespace->release();
}

QV8TypeWrapper::QV8TypeWrapper()
: m_engine(0)
{
}

QV8TypeWrapper::~QV8TypeWrapper()
{
}

void QV8TypeWrapper::destroy()
{
    qPersistentDispose(m_constructor);
}

void QV8TypeWrapper::init(QV8Engine *engine)
{
    m_engine = engine;
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetNamedPropertyHandler(Getter, Setter);
    ft->InstanceTemplate()->SetHasExternalResource(true);
    m_constructor = qPersistentNew<v8::Function>(ft->GetFunction());
}

v8::Local<v8::Object> QV8TypeWrapper::newObject(QObject *o, QDeclarativeType *t, TypeNameMode mode)
{
    Q_ASSERT(t);
    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_constructor->NewInstance(); 
    QV8TypeResource *r = new QV8TypeResource(m_engine);
    r->mode = mode; r->object = o; r->type = t;
    rv->SetExternalResource(r);
    return rv;
}

v8::Local<v8::Object> QV8TypeWrapper::newObject(QObject *o, QDeclarativeTypeNameCache *t, TypeNameMode mode)
{
    Q_ASSERT(t);
    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_constructor->NewInstance(); 
    QV8TypeResource *r = new QV8TypeResource(m_engine);
    t->addref();
    r->mode = mode; r->object = o; r->typeNamespace = t;
    rv->SetExternalResource(r);
    return rv;
}

v8::Handle<v8::Value> QV8TypeWrapper::Getter(v8::Local<v8::String> property, 
                                             const v8::AccessorInfo &info)
{
    v8::Object::ExternalResource *r = info.This()->GetExternalResource();
    QV8TypeResource *resource = v8_resource_cast<QV8TypeResource>(info.This());

    if (!resource) 
        return v8::Undefined();

    QV8Engine *v8engine = resource->engine;
    QObject *object = resource->object;

    QHashedV8String propertystring(property);

    if (resource->type) {
        QDeclarativeType *type = resource->type;

        if (QV8Engine::startsWithUpper(property)) {
            if (resource->mode == IncludeEnums) {
                QString name = v8engine->toString(property);

                // ### Optimize
                QByteArray enumName = name.toUtf8();
                const QMetaObject *metaObject = type->baseMetaObject();
                for (int ii = metaObject->enumeratorCount() - 1; ii >= 0; --ii) {
                    QMetaEnum e = metaObject->enumerator(ii);
                    int value = e.keyToValue(enumName.constData());
                    if (value != -1) 
                        return v8::Integer::New(value);
                }
            } 

            // Fall through to undefined

        } else if (resource->object) {
            QObject *ao = qmlAttachedPropertiesObjectById(type->attachedPropertiesId(), object);
            if (ao) 
                return v8engine->qobjectWrapper()->getProperty(ao, propertystring, 
                                                               QV8QObjectWrapper::IgnoreRevision);

            // Fall through to undefined
        }

        // Fall through to undefined

    } else if (resource->typeNamespace) {

        QDeclarativeTypeNameCache *typeNamespace = resource->typeNamespace;
        QDeclarativeTypeNameCache::Data *d = typeNamespace->data(propertystring);
        Q_ASSERT(!d || !d->typeNamespace); // Nested namespaces not supported

        if (d && d->type) {
            return v8engine->typeWrapper()->newObject(object, d->type, resource->mode);
        } else if (QDeclarativeMetaType::ModuleApiInstance *moduleApi = typeNamespace->moduleApi()) {
            // XXX TODO: Currently module APIs are implemented against QScriptValues.  Consequently we
            // can't do anything here until the QtScript/V8 binding is complete.
            return v8::Undefined();

        }

        // Fall through to undefined

    } else {
        Q_ASSERT(!"Unreachable");
    }
    return v8::Undefined(); 
}

v8::Handle<v8::Value> QV8TypeWrapper::Setter(v8::Local<v8::String> property, 
                                             v8::Local<v8::Value> value,
                                             const v8::AccessorInfo &info)
{
    v8::Object::ExternalResource *r = info.This()->GetExternalResource();
    QV8TypeResource *resource = v8_resource_cast<QV8TypeResource>(info.This());

    if (!resource) 
        return value;

    QV8Engine *v8engine = resource->engine;

    // XXX TODO: Implement writes to module API objects

    QHashedV8String propertystring(property);

    if (resource->type && resource->object) {
        QDeclarativeType *type = resource->type;
        QObject *object = resource->object;
        QObject *ao = qmlAttachedPropertiesObjectById(type->attachedPropertiesId(), object);
        if (ao) 
            v8engine->qobjectWrapper()->setProperty(ao, propertystring, value, 
                                                    QV8QObjectWrapper::IgnoreRevision);
    }

    return value;
}

QT_END_NAMESPACE
