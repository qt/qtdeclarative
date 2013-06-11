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

QT_BEGIN_NAMESPACE

static QString internal(QLatin1String("You've stumbled onto an internal implementation detail "
                                      "that should never have been exposed."));

class QV8ContextResource : public QV8ObjectResource
{
    V8_RESOURCE_TYPE(ContextType)

public:
    QV8ContextResource(QV8Engine *engine, QQmlContextData *context, QObject *scopeObject, bool ownsContext = false);
    ~QV8ContextResource();

    inline QQmlContextData *getContext() const;
    inline QObject *getScopeObject() const;

    quint32 isSharedContext:1;
    quint32 hasSubContexts:1;
    quint32 readOnly:1;
    quint32 ownsContext:1;
    quint32 dummy:28;

    // This is a pretty horrible hack, and an abuse of external strings.  When we create a 
    // sub-context (a context created by a Qt.include() in an external javascript file),
    // we pass a specially crafted SubContext external string as the v8::Script::Data() to
    // the script, which contains a pointer to the context.  We can then access the 
    // v8::Script::Data() later on to resolve names and URLs against the sub-context instead
    // of the main outer context.
    struct SubContext : public v8::String::ExternalStringResource {
        SubContext(QQmlContextData *context) : context(context) {}
        QQmlGuardedContextData context;

        virtual const uint16_t* data() const { return (const uint16_t *)internal.constData(); }
        virtual size_t length() const { return internal.length(); }
    };

private:
    QQmlGuardedContextData context;
    QQmlGuard<QObject> scopeObject;

};

QV8ContextResource::QV8ContextResource(QV8Engine *engine, QQmlContextData *context, QObject *scopeObject, bool ownsContext)
: QV8ObjectResource(engine), isSharedContext(false), hasSubContexts(false), readOnly(true),
  ownsContext(ownsContext), context(context), scopeObject(scopeObject)
{
}

QV8ContextResource::~QV8ContextResource()
{
    if (context && ownsContext)
        context->destroy();
}

// Returns the scope object
QObject *QV8ContextResource::getScopeObject() const
{
    if (isSharedContext)
        return QQmlEnginePrivate::get(engine->engine())->sharedScope;
    else
        return scopeObject;
}

// Returns the context, including resolving a subcontext
QQmlContextData *QV8ContextResource::getContext() const
{
    if (isSharedContext)
        return QQmlEnginePrivate::get(engine->engine())->sharedContext;
    
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

v8::Local<v8::Object> QV8ContextWrapper::qmlScope(QQmlContextData *ctxt, QObject *scope)
{
    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_constructor->NewInstance(); 
    QV8ContextResource *r = new QV8ContextResource(m_engine, ctxt, scope);
    rv->SetExternalResource(r);
    return rv;
}

v8::Local<v8::Object> QV8ContextWrapper::urlScope(const QUrl &url)
{
    QQmlContextData *context = new QQmlContextData;
    context->url = url;
    context->isInternal = true;
    context->isJSContext = true;

    // XXX NewInstance() should be optimized
    v8::Local<v8::Object> rv = m_urlConstructor->NewInstance(); 
    QV8ContextResource *r = new QV8ContextResource(m_engine, context, 0, true);
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
                                      QQmlContextData *ctxt)
{
    QV8ContextResource *resource = v8_resource_cast<QV8ContextResource>(qmlglobal);
    Q_ASSERT(resource);
    resource->hasSubContexts = true;
    script->SetData(v8::String::NewExternal(new QV8ContextResource::SubContext(ctxt)));
}

QQmlContextData *QV8ContextWrapper::callingContext()
{
    v8::Local<v8::Object> qmlglobal = v8::Context::GetCallingQmlGlobal();
    if (qmlglobal.IsEmpty()) return 0;

    QV8ContextResource *r = v8_resource_cast<QV8ContextResource>(qmlglobal);
    return r?r->getContext():0;
}

QQmlContextData *QV8ContextWrapper::context(v8::Handle<v8::Value> value)
{
    if (!value->IsObject())
        return 0;

    v8::Handle<v8::Object> qmlglobal = v8::Handle<v8::Object>::Cast(value);
    QV8ContextResource *r = v8_resource_cast<QV8ContextResource>(qmlglobal);
    return r?r->getContext():0;
}

void QV8ContextWrapper::takeContextOwnership(v8::Handle<v8::Object> qmlglobal)
{
    QV8ContextResource *r = v8_resource_cast<QV8ContextResource>(qmlglobal);
    r->ownsContext = true;
}

v8::Handle<v8::Value> QV8ContextWrapper::NullGetter(v8::Local<v8::String>,
                                                    const v8::AccessorInfo &)
{
    // V8 will throw a ReferenceError if appropriate ("typeof" should not throw)
    return v8::Handle<v8::Value>();
}

v8::Handle<v8::Value> QV8ContextWrapper::Getter(v8::Local<v8::String> property, 
                                                const v8::AccessorInfo &info)
{
    QV8ContextResource *resource = v8_resource_check<QV8ContextResource>(info.This());

    // Its possible we could delay the calculation of the "actual" context (in the case
    // of sub contexts) until it is definately needed.
    QQmlContextData *context = resource->getContext();
    QQmlContextData *expressionContext = context;

    if (!context)
        return v8::Undefined();

    if (v8::Context::GetCallingQmlGlobal() != info.This())
        return v8::Handle<v8::Value>();

    // Search type (attached property/enum/imported scripts) names
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
        QQmlTypeNameCache::Result r = context->imports->query(propertystring);
        
        if (r.isValid()) { 
            if (r.scriptIndex != -1) {
                int index = r.scriptIndex;
                if (index < context->importedScripts.count())
                    return context->importedScripts.at(index);
                else
                    return v8::Undefined();
            } else if (r.type) {
                return engine->typeWrapper()->newObject(scopeObject, r.type);
            } else if (r.importNamespace) {
                return engine->typeWrapper()->newObject(scopeObject, context->imports, r.importNamespace);
            }
            Q_ASSERT(!"Unreachable");
        }

        // Fall through
    }

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine->engine());
    QV8QObjectWrapper *qobjectWrapper = engine->qobjectWrapper();

    while (context) {
        // Search context properties
        if (context->propertyNames) {
            int propertyIdx = context->propertyNames->value(propertystring);

            if (propertyIdx != -1) {

                if (propertyIdx < context->idValueCount) {

                    ep->captureProperty(&context->idValues[propertyIdx].bindings);
                    return engine->newQObject(context->idValues[propertyIdx]);
                } else {

                    QQmlContextPrivate *cp = context->asQQmlContextPrivate();

                    ep->captureProperty(context->asQQmlContext(), -1,
                                        propertyIdx + cp->notifyIndex);

                    const QVariant &value = cp->propertyValues.at(propertyIdx);
                    if (value.userType() == qMetaTypeId<QList<QObject*> >()) {
                        QQmlListProperty<QObject> prop(context->asQQmlContext(), (void*) qintptr(propertyIdx),
                                                               QQmlContextPrivate::context_count,
                                                               QQmlContextPrivate::context_at);
                        return engine->listWrapper()->newList(prop, qMetaTypeId<QQmlListProperty<QObject> >());
                    } else {
                        return engine->fromVariant(cp->propertyValues.at(propertyIdx));
                    }
                }
            }
        }

        // Search scope object
        if (scopeObject) {
            v8::Handle<v8::Value> result = qobjectWrapper->getProperty(scopeObject, propertystring,
                                                                       context, QV8QObjectWrapper::CheckRevision);
            if (!result.IsEmpty()) return result;
        }
        scopeObject = 0;


        // Search context object
        if (context->contextObject) {
            v8::Handle<v8::Value> result = qobjectWrapper->getProperty(context->contextObject, propertystring,
                                                                       context, QV8QObjectWrapper::CheckRevision);
            if (!result.IsEmpty()) return result;
        }

        context = context->parent;
    }

    expressionContext->unresolvedNames = true;

    // V8 will throw a ReferenceError if appropriate ("typeof" should not throw)
    return v8::Handle<v8::Value>();
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
                        QLatin1Char('"');
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
    QQmlContextData *context = resource->getContext();
    QQmlContextData *expressionContext = context;

    if (!context)
        return v8::Undefined();

    if (v8::Context::GetCallingQmlGlobal() != info.This())
        return v8::Handle<v8::Value>();

    // See QV8ContextWrapper::Getter for resolution order
    
    QV8Engine *engine = resource->engine;
    QObject *scopeObject = resource->getScopeObject();

    QHashedV8String propertystring(property);

    QV8QObjectWrapper *qobjectWrapper = engine->qobjectWrapper();

    while (context) {
        // Search context properties
        if (context->propertyNames && -1 != context->propertyNames->value(propertystring))
            return value;

        // Search scope object
        if (scopeObject && 
            qobjectWrapper->setProperty(scopeObject, propertystring, context, value, QV8QObjectWrapper::CheckRevision))
            return value;
        scopeObject = 0;

        // Search context object
        if (context->contextObject &&
            qobjectWrapper->setProperty(context->contextObject, propertystring, context, value,
                                        QV8QObjectWrapper::CheckRevision))
            return value;

        context = context->parent;
    }

    expressionContext->unresolvedNames = true;

    if (!resource->readOnly) {
        return v8::Handle<v8::Value>();
    } else {
        QString error = QLatin1String("Invalid write to global property \"") + engine->toString(property) + 
                        QLatin1Char('"');
        v8::ThrowException(v8::Exception::Error(engine->toString(error)));
        return v8::Undefined();
    }
}

QT_END_NAMESPACE
