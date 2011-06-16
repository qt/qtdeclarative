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

static QString internal(QLatin1String("You've stumbled onto an internal implementation detail "
                                      "that should never have been exposed."));

class QV8ContextResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(ContextType);

public:
    QV8ContextResource(QV8Engine *engine, QDeclarativeContextData *context, QObject *scopeObject);
    ~QV8ContextResource();

    inline QDeclarativeContextData *getContext() const;
    inline QObject *getScopeObject() const;

    quint32 isSharedContext:1;
    quint32 hasSubContexts:1;
    quint32 readOnly:1;
    quint32 dummy:29;

    QObject *secondaryScope;

    // This is a pretty horrible hack, and an abuse of external strings.  When we create a 
    // sub-context (a context created by a Qt.include() in an external javascript file),
    // we pass a specially crafted SubContext external string as the v8::Script::Data() to
    // the script, which contains a pointer to the context.  We can then access the 
    // v8::Script::Data() later on to resolve names and URLs against the sub-context instead
    // of the main outer context.
    struct SubContext : public v8::String::ExternalStringResource {
        SubContext(QDeclarativeContextData *context) : context(context) {}
        QDeclarativeGuardedContextData context;

        virtual const uint16_t* data() const { return (const uint16_t *)internal.constData(); }
        virtual size_t length() const { return internal.length(); }
    };

private:
    QDeclarativeGuardedContextData context;
    QDeclarativeGuard<QObject> scopeObject;

};

QV8ContextResource::QV8ContextResource(QV8Engine *engine, QDeclarativeContextData *context, QObject *scopeObject)
: QV8ObjectResource(engine), isSharedContext(false), hasSubContexts(false), readOnly(true), 
  secondaryScope(0), context(context), scopeObject(scopeObject)
{
}

QV8ContextResource::~QV8ContextResource()
{
    if (context && context->isJSContext)
        context->destroy();
}

// Returns the scope object
QObject *QV8ContextResource::getScopeObject() const
{
    if (isSharedContext)
        return QDeclarativeEnginePrivate::get(engine->engine())->sharedScope;
    else
        return scopeObject;
}

// Returns the context, including resolving a subcontext
QDeclarativeContextData *QV8ContextResource::getContext() const
{
    if (isSharedContext)
        return QDeclarativeEnginePrivate::get(engine->engine())->sharedContext;
    
    if (!hasSubContexts)
        return context;

    v8::Local<v8::Value> callingdata = v8::Context::GetCallingScriptData();
    if (callingdata.IsEmpty() || !callingdata->IsString())
        return context;

    v8::Local<v8::String> callingstring = callingdata->ToString();
    Q_ASSERT(callingstring->IsExternal());
    Q_ASSERT(callingstring->GetExternalStringResource());

    SubContext *sc = static_cast<SubContext *>(callingstring->GetExternalStringResource());
    return sc->context;
}

QV8ContextWrapper::QV8ContextWrapper()
: m_engine(0)
{
}

QV8ContextWrapper::~QV8ContextWrapper()
{
}

void QV8ContextWrapper::destroy()
{
    qPersistentDispose(m_sharedContext);
    qPersistentDispose(m_urlConstructor);
    qPersistentDispose(m_constructor);
}

void QV8ContextWrapper::init(QV8Engine *engine)
{
    m_engine = engine;
    {
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->InstanceTemplate()->SetFallbackPropertyHandler(Getter, Setter);
    m_constructor = qPersistentNew<v8::Function>(ft->GetFunction());
    }
    {
    v8::Local<v8::FunctionTemplate> ft = v8::FunctionTemplate::New();
    ft->InstanceTemplate()->SetHasExternalResource(true);
    ft->InstanceTemplate()->SetFallbackPropertyHandler(NullGetter, NullSetter);
    m_urlConstructor = qPersistentNew<v8::Function>(ft->GetFunction());
    }
    {
    v8::Local<v8::Object> sharedContext = m_constructor->NewInstance();
    QV8ContextResource *r = new QV8ContextResource(engine, 0, 0);
    r->isSharedContext = true;
    sharedContext->SetExternalResource(r);
    m_sharedContext = qPersistentNew<v8::Object>(sharedContext);
    }
}

v8::Local<v8::Object> QV8ContextWrapper::qmlScope(QDeclarativeContextData *ctxt, QObject *scope)
{
    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_constructor->NewInstance(); 
    QV8ContextResource *r = new QV8ContextResource(m_engine, ctxt, scope);
    rv->SetExternalResource(r);
    return rv;
}

v8::Local<v8::Object> QV8ContextWrapper::urlScope(const QUrl &url)
{
    QDeclarativeContextData *context = new QDeclarativeContextData;
    context->url = url;
    context->isInternal = true;
    context->isJSContext = true;

    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_urlConstructor->NewInstance(); 
    QV8ContextResource *r = new QV8ContextResource(m_engine, context, 0);
    rv->SetExternalResource(r);
    return rv;
}

void QV8ContextWrapper::setReadOnly(v8::Handle<v8::Object> qmlglobal, bool readOnly)
{
    QV8ContextResource *resource = v8_resource_cast<QV8ContextResource>(qmlglobal);
    Q_ASSERT(resource);
    resource->readOnly = readOnly;
}

void QV8ContextWrapper::addSubContext(v8::Handle<v8::Object> qmlglobal, v8::Handle<v8::Script> script, 
                                      QDeclarativeContextData *ctxt)
{
    QV8ContextResource *resource = v8_resource_cast<QV8ContextResource>(qmlglobal);
    Q_ASSERT(resource);
    resource->hasSubContexts = true;
    script->SetData(v8::String::NewExternal(new QV8ContextResource::SubContext(ctxt)));
}

QObject *QV8ContextWrapper::setSecondaryScope(v8::Handle<v8::Object> ctxt, QObject *scope)
{
    QV8ContextResource *resource = v8_resource_cast<QV8ContextResource>(ctxt);
    if (!resource) return 0;

    QObject *rv = resource->secondaryScope;
    resource->secondaryScope = scope;
    return rv;
}

QDeclarativeContextData *QV8ContextWrapper::callingContext()
{
    v8::Local<v8::Object> qmlglobal = v8::Context::GetCallingQmlGlobal();
    if (qmlglobal.IsEmpty()) return 0;

    QV8ContextResource *r = v8_resource_cast<QV8ContextResource>(qmlglobal);
    return r?r->getContext():0;
}

QDeclarativeContextData *QV8ContextWrapper::context(v8::Handle<v8::Value> value)
{
    if (!value->IsObject())
        return 0;

    v8::Handle<v8::Object> qmlglobal = v8::Handle<v8::Object>::Cast(value);
    QV8ContextResource *r = v8_resource_cast<QV8ContextResource>(qmlglobal);
    return r?r->getContext():0;
}

v8::Handle<v8::Value> QV8ContextWrapper::NullGetter(v8::Local<v8::String> property, 
                                                    const v8::AccessorInfo &info)
{
    QV8ContextResource *resource = v8_resource_check<QV8ContextResource>(info.This());

    QV8Engine *engine = resource->engine;

    QString error = QLatin1String("Can't find variable: ") + engine->toString(property);
    v8::ThrowException(v8::Exception::ReferenceError(engine->toString(error)));
    return v8::Undefined();
}

v8::Handle<v8::Value> QV8ContextWrapper::Getter(v8::Local<v8::String> property, 
                                                const v8::AccessorInfo &info)
{
    QV8ContextResource *resource = v8_resource_check<QV8ContextResource>(info.This());

    // Its possible we could delay the calculation of the "actual" context (in the case
    // of sub contexts) until it is definately needed.
    QDeclarativeContextData *context = resource->getContext();

    if (!context)
        return v8::Undefined();

    if (v8::Context::GetCallingQmlGlobal() != info.This())
        return v8::Handle<v8::Value>();

    // Search type (attached property/enum/imported scripts) names
    // Secondary scope object
    // while (context) {
    //     Search context properties
    //     Search scope object
    //     Search context object
    //     context = context->parent
    // }

    QV8Engine *engine = resource->engine;

    QObject *scopeObject = resource->getScopeObject();

    QHashedV8String propertystring(property);

    if (context->imports && QV8Engine::startsWithUpper(property)) {
        // Search for attached properties, enums and imported scripts
        QDeclarativeTypeNameCache::Data *data = context->imports->data(propertystring);

        if (data) {
            if (data->importedScriptIndex != -1) {
                int index = data->importedScriptIndex;
                if (index < context->importedScripts.count())
                    return context->importedScripts.at(index);
                else
                    return v8::Undefined();
            } else if (data->type) {
                return engine->typeWrapper()->newObject(scopeObject, data->type);
            } else if (data->typeNamespace) {
                return engine->typeWrapper()->newObject(scopeObject, data->typeNamespace);
            }
            Q_ASSERT(!"Unreachable");
        }

        // Fall through
    }

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine->engine());
    QV8QObjectWrapper *qobjectWrapper = engine->qobjectWrapper();

    if (resource->secondaryScope) {
        v8::Handle<v8::Value> result = qobjectWrapper->getProperty(resource->secondaryScope, propertystring, 
                                                                   QV8QObjectWrapper::IgnoreRevision);
        if (!result.IsEmpty()) return result;
    }

    while (context) {
        // Search context properties
        if (context->propertyNames) {
            int propertyIdx = context->propertyNames->value(propertystring);

            if (propertyIdx != -1) {
                typedef QDeclarativeEnginePrivate::CapturedProperty CapturedProperty;

                if (propertyIdx < context->idValueCount) {

                    if (ep->captureProperties)
                        ep->capturedProperties << CapturedProperty(&context->idValues[propertyIdx].bindings);

                    return engine->newQObject(context->idValues[propertyIdx]);
                } else {

                    QDeclarativeContextPrivate *cp = context->asQDeclarativeContextPrivate();

                    if (ep->captureProperties)
                        ep->capturedProperties << CapturedProperty(context->asQDeclarativeContext(), -1, 
                                                                   propertyIdx + cp->notifyIndex);

                    const QVariant &value = cp->propertyValues.at(propertyIdx);
                    if (value.userType() == qMetaTypeId<QList<QObject*> >()) {
                        QDeclarativeListProperty<QObject> prop(context->asQDeclarativeContext(), (void*)propertyIdx,
                                                               0,
                                                               QDeclarativeContextPrivate::context_count,
                                                               QDeclarativeContextPrivate::context_at);
                        return engine->listWrapper()->newList(prop, qMetaTypeId<QDeclarativeListProperty<QObject> >());
                    } else {
                        return engine->fromVariant(cp->propertyValues.at(propertyIdx));
                    }
                }
            }
        }

        // Search scope object
        if (scopeObject) {
            v8::Handle<v8::Value> result = qobjectWrapper->getProperty(scopeObject, propertystring,
                                                                       QV8QObjectWrapper::CheckRevision);
            if (!result.IsEmpty()) return result;
        }
        scopeObject = 0;


        // Search context object
        if (context->contextObject) {
            v8::Handle<v8::Value> result = qobjectWrapper->getProperty(context->contextObject, propertystring,
                                                                       QV8QObjectWrapper::CheckRevision);
            if (!result.IsEmpty()) return result;
        }

        context = context->parent;
    }

    QString error = QLatin1String("Can't find variable: ") + engine->toString(property);
    v8::ThrowException(v8::Exception::ReferenceError(engine->toString(error)));
    return v8::Undefined();
}

v8::Handle<v8::Value> QV8ContextWrapper::NullSetter(v8::Local<v8::String> property, 
                                                    v8::Local<v8::Value>,
                                                    const v8::AccessorInfo &info)
{
    QV8ContextResource *resource = v8_resource_check<QV8ContextResource>(info.This());

    QV8Engine *engine = resource->engine;

    if (!resource->readOnly) {
        return v8::Handle<v8::Value>();
    } else {
        QString error = QLatin1String("Invalid write to global property \"") + engine->toString(property) + 
                        QLatin1String("\"");
        v8::ThrowException(v8::Exception::Error(engine->toString(error)));
        return v8::Handle<v8::Value>();
    }
}

v8::Handle<v8::Value> QV8ContextWrapper::Setter(v8::Local<v8::String> property, 
                                                v8::Local<v8::Value> value,
                                                const v8::AccessorInfo &info)
{
    QV8ContextResource *resource = v8_resource_check<QV8ContextResource>(info.This());

    // Its possible we could delay the calculation of the "actual" context (in the case
    // of sub contexts) until it is definately needed.
    QDeclarativeContextData *context = resource->getContext();

    if (!context)
        return v8::Undefined();

    if (v8::Context::GetCallingQmlGlobal() != info.This())
        return v8::Handle<v8::Value>();

    // See QV8ContextWrapper::Getter for resolution order
    
    QV8Engine *engine = resource->engine;
    QObject *scopeObject = resource->getScopeObject();

    QHashedV8String propertystring(property);

    QV8QObjectWrapper *qobjectWrapper = engine->qobjectWrapper();

    // Search scope object
    if (resource->secondaryScope && 
        qobjectWrapper->setProperty(resource->secondaryScope, propertystring, value, 
                                    QV8QObjectWrapper::IgnoreRevision))
        return value;

    while (context) {
        // Search context properties
        if (context->propertyNames && -1 != context->propertyNames->value(propertystring))
            return value;

        // Search scope object
        if (scopeObject && 
            qobjectWrapper->setProperty(scopeObject, propertystring, value, QV8QObjectWrapper::CheckRevision))
            return value;
        scopeObject = 0;

        // Search context object
        if (context->contextObject &&
            qobjectWrapper->setProperty(context->contextObject, propertystring, value, 
                                        QV8QObjectWrapper::CheckRevision))
            return value;

        context = context->parent;
    }

    if (!resource->readOnly) {
        return v8::Handle<v8::Value>();
    } else {
        QString error = QLatin1String("Invalid write to global property \"") + engine->toString(property) + 
                        QLatin1String("\"");
        v8::ThrowException(v8::Exception::Error(engine->toString(error)));
        return v8::Undefined();
    }
}

QT_END_NAMESPACE
