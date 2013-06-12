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

#include "qv8contextwrapper_p.h"
#include "qv8engine_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlcontext_p.h>

#include <private/qjsvalue_p.h>
#include <private/qscript_impl_p.h>

QT_BEGIN_NAMESPACE

class QV8TypeResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(TypeType)

public:
    QV8TypeResource(QV8Engine *engine);
    virtual ~QV8TypeResource();

    QV8TypeWrapper::TypeNameMode mode;

    QQmlGuard<QObject> object;

    QQmlType *type;
    QQmlTypeNameCache *typeNamespace;
    const void *importNamespace;
};

QV8TypeResource::QV8TypeResource(QV8Engine *engine)
: QV8ObjectResource(engine), mode(QV8TypeWrapper::IncludeEnums), type(0), typeNamespace(0), importNamespace(0)
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

// Returns a type wrapper for type t on o.  This allows access of enums, and attached properties.
v8::Local<v8::Object> QV8TypeWrapper::newObject(QObject *o, QQmlType *t, TypeNameMode mode)
{
    Q_ASSERT(t);
    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_constructor->NewInstance(); 
    QV8TypeResource *r = new QV8TypeResource(m_engine);
    r->mode = mode; r->object = o; r->type = t;
    rv->SetExternalResource(r);
    return rv;
}

// Returns a type wrapper for importNamespace (of t) on o.  This allows nested resolution of a type in a 
// namespace.
v8::Local<v8::Object> QV8TypeWrapper::newObject(QObject *o, QQmlTypeNameCache *t, 
                                                const void *importNamespace, TypeNameMode mode)
{
    Q_ASSERT(t);
    Q_ASSERT(importNamespace);
    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_constructor->NewInstance(); 
    QV8TypeResource *r = new QV8TypeResource(m_engine);
    t->addref();
    r->mode = mode; r->object = o; r->typeNamespace = t; r->importNamespace = importNamespace;
    rv->SetExternalResource(r);
    return rv;
}

QVariant QV8TypeWrapper::toVariant(QV8ObjectResource *r)
{
    Q_ASSERT(r->resourceType() == QV8ObjectResource::TypeType);
    QV8TypeResource *resource = static_cast<QV8TypeResource *>(r);
    QV8Engine *v8engine = resource->engine;

    if (resource->type && resource->type->isSingleton()) {
        QQmlEngine *e = v8engine->engine();
        QQmlType::SingletonInstanceInfo *siinfo = resource->type->singletonInstanceInfo();
        siinfo->init(e); // note: this will also create QJSValue singleton which isn't strictly required.
        QObject *qobjectSingleton = siinfo->qobjectApi(e);
        if (qobjectSingleton) {
            return QVariant::fromValue<QObject*>(qobjectSingleton);
        }
    }

    // only QObject Singleton Type can be converted to a variant.
    return QVariant();
}

v8::Handle<v8::Value> QV8TypeWrapper::Getter(v8::Local<v8::String> property, 
                                             const v8::AccessorInfo &info)
{
    QV8TypeResource *resource = v8_resource_cast<QV8TypeResource>(info.This());

    if (!resource) 
        return v8::Undefined();

    QV8Engine *v8engine = resource->engine;
    QQmlContextData *context = v8engine->callingContext();

    QObject *object = resource->object;

    QHashedV8String propertystring(property);

    if (resource->type) {
        QQmlType *type = resource->type;

        // singleton types are handled differently to other types.
        if (type->isSingleton()) {
            QQmlEngine *e = v8engine->engine();
            QQmlType::SingletonInstanceInfo *siinfo = type->singletonInstanceInfo();
            siinfo->init(e);

            QObject *qobjectSingleton = siinfo->qobjectApi(e);
            if (qobjectSingleton) {
                // check for enum value
                if (QV8Engine::startsWithUpper(property)) {
                    if (resource->mode == IncludeEnums) {
                        QString name = v8engine->toString(property);

                        // ### Optimize
                        QByteArray enumName = name.toUtf8();
                        const QMetaObject *metaObject = qobjectSingleton->metaObject();
                        for (int ii = metaObject->enumeratorCount() - 1; ii >= 0; --ii) {
                            QMetaEnum e = metaObject->enumerator(ii);
                            bool ok;
                            int value = e.keyToValue(enumName.constData(), &ok);
                            if (ok)
                                return v8::Integer::New(value);
                        }
                    }
                }

                // check for property.
                v8::Handle<v8::Value> rv = v8engine->qobjectWrapper()->getProperty(qobjectSingleton, propertystring, context, QV8QObjectWrapper::IgnoreRevision);
                return rv;
            } else if (!siinfo->scriptApi(e).isUndefined()) {
                // NOTE: if used in a binding, changes will not trigger re-evaluation since non-NOTIFYable.
                QJSValuePrivate *apiprivate = QJSValuePrivate::get(siinfo->scriptApi(e));
                QScopedPointer<QJSValuePrivate> propertyValue(apiprivate->property(property).give());
                return propertyValue->asV8Value(v8engine);
            }

            // Fall through to return empty handle

        } else {

            if (QV8Engine::startsWithUpper(property)) {
                bool ok = false;
                int value = type->enumValue(propertystring, &ok);
                if (ok)
                    return v8::Integer::New(value);

                // Fall through to return empty handle

            } else if (resource->object) {
                QObject *ao = qmlAttachedPropertiesObjectById(type->attachedPropertiesId(), object);
                if (ao)
                    return v8engine->qobjectWrapper()->getProperty(ao, propertystring, context,
                                                                   QV8QObjectWrapper::IgnoreRevision);

                // Fall through to return empty handle
            }

            // Fall through to return empty handle
        }

        // Fall through to return empty handle

    } else if (resource->typeNamespace) {
        Q_ASSERT(resource->importNamespace);
        QQmlTypeNameCache::Result r = resource->typeNamespace->query(propertystring,
                                                                             resource->importNamespace);

        if (r.isValid()) {
            QQmlContextData *context = v8engine->callingContext();
            if (r.type) {
                return v8engine->typeWrapper()->newObject(object, r.type, resource->mode);
            } else if (r.scriptIndex != -1) {
                int index = r.scriptIndex;
                if (index < context->importedScripts.count())
                    return context->importedScripts.at(index);
            } else if (r.importNamespace) {
                return v8engine->typeWrapper()->newObject(object, context->imports, r.importNamespace);
            }

            return v8::Undefined();

        }

        // Fall through to return empty handle

    } else {
        Q_ASSERT(!"Unreachable");
    }

    return v8::Handle<v8::Value>();
}

v8::Handle<v8::Value> QV8TypeWrapper::Setter(v8::Local<v8::String> property, 
                                             v8::Local<v8::Value> value,
                                             const v8::AccessorInfo &info)
{
    QV8TypeResource *resource = v8_resource_cast<QV8TypeResource>(info.This());

    if (!resource) 
        return value;

    QV8Engine *v8engine = resource->engine;
    QQmlContextData *context = v8engine->callingContext();

    QHashedV8String propertystring(property);

    QQmlType *type = resource->type;
    if (type && !type->isSingleton() && resource->object) {
        QObject *object = resource->object;
        QObject *ao = qmlAttachedPropertiesObjectById(type->attachedPropertiesId(), object);
        if (ao) 
            v8engine->qobjectWrapper()->setProperty(ao, propertystring, context, value,
                                                    QV8QObjectWrapper::IgnoreRevision);
    } else if (type && type->isSingleton()) {
        QQmlEngine *e = v8engine->engine();
        QQmlType::SingletonInstanceInfo *siinfo = type->singletonInstanceInfo();
        siinfo->init(e);

        QObject *qobjectSingleton = siinfo->qobjectApi(e);
        if (qobjectSingleton) {
            v8engine->qobjectWrapper()->setProperty(qobjectSingleton, propertystring, context, value,
                                                    QV8QObjectWrapper::IgnoreRevision);
        } else if (!siinfo->scriptApi(e).isUndefined()) {
            QScopedPointer<QJSValuePrivate> setvalp(new QJSValuePrivate(v8engine, value));
            QJSValuePrivate *apiprivate = QJSValuePrivate::get(siinfo->scriptApi(e));
            if (apiprivate->propertyFlags(property) & QJSValuePrivate::ReadOnly) {
                QString error = QLatin1String("Cannot assign to read-only property \"") +
                                v8engine->toString(property) + QLatin1Char('\"');
                v8::ThrowException(v8::Exception::Error(v8engine->toString(error)));
            } else {
                apiprivate->setProperty(property, setvalp.data());
            }
        }
    }

    return value;
}

QT_END_NAMESPACE
