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

#include <private/qv4engine_p.h>
#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4objectproto_p.h>
#include <private/qv4mm_p.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct Q_QML_EXPORT QmlContextWrapper : Object
{
    QmlContextWrapper(QV8Engine *engine, QQmlContextData *context, QObject *scopeObject, bool ownsContext = false);
    ~QmlContextWrapper();

    inline QObject *getScopeObject() const { return scopeObject; }
    inline QQmlContextData *getContext() const { return context; }

    QV8Engine *v8; // ### temporary, remove
    bool readOnly;
    bool ownsContext;

    QQmlGuardedContextData context;
    QQmlGuard<QObject> scopeObject;

    static Value get(Managed *m, ExecutionContext *ctx, String *name, bool *hasProperty);
    static void put(Managed *m, ExecutionContext *ctx, String *name, const Value &value);
    static void destroy(Managed *that);

private:
    const static ManagedVTable static_vtbl;
};

struct QmlContextNullWrapper : QmlContextWrapper
{
    QmlContextNullWrapper(QV8Engine *engine, QQmlContextData *context, QObject *scopeObject, bool ownsContext = false)
        : QmlContextWrapper(engine, context, scopeObject, ownsContext)
    {
        vtbl = &static_vtbl;
    }

    using Object::get;
    static void put(Managed *m, ExecutionContext *ctx, String *name, const Value &value);

private:
    const static ManagedVTable static_vtbl;
};

}

using namespace QV4;

DEFINE_MANAGED_VTABLE(QmlContextWrapper);
DEFINE_MANAGED_VTABLE(QmlContextNullWrapper);

QmlContextWrapper::QmlContextWrapper(QV8Engine *engine, QQmlContextData *context, QObject *scopeObject, bool ownsContext)
    : Object(QV8Engine::getV4(engine)),
      v8(engine), readOnly(true), ownsContext(ownsContext),
      context(context), scopeObject(scopeObject)
{
    type = Type_QmlContext;
    vtbl = &static_vtbl;
}

QmlContextWrapper::~QmlContextWrapper()
{
    if (context && ownsContext)
        context->destroy();
}



QV8ContextWrapper::QV8ContextWrapper()
    : m_engine(0), v4(0)
{
}

QV8ContextWrapper::~QV8ContextWrapper()
{
}

void QV8ContextWrapper::destroy()
{
}

void QV8ContextWrapper::init(QV8Engine *engine)
{
    m_engine = engine;
    v4 = QV8Engine::getV4(engine);
}

QV4::Value QV8ContextWrapper::qmlScope(QQmlContextData *ctxt, QObject *scope)
{
    QmlContextWrapper *w = new (v4->memoryManager) QmlContextWrapper(m_engine, ctxt, scope);
    w->prototype = v4->objectPrototype;
    return Value::fromObject(w);
}

QV4::Value QV8ContextWrapper::urlScope(const QUrl &url)
{
    QQmlContextData *context = new QQmlContextData;
    context->url = url;
    context->isInternal = true;
    context->isJSContext = true;

    QmlContextWrapper *w = new (v4->memoryManager) QmlContextNullWrapper(m_engine, context, 0);
    w->prototype = v4->objectPrototype;
    return Value::fromObject(w);
}

void QV8ContextWrapper::setReadOnly(const Value &qmlglobal, bool readOnly)
{
    Object *o = qmlglobal.asObject();
    QmlContextWrapper *c = o ? o->asQmlContext() : 0;
    assert(c);
    c->readOnly = readOnly;
}

QQmlContextData *QV8ContextWrapper::callingContext()
{
    QV4::Object *qmlglobal = QV8Engine::getV4(m_engine)->qmlContextObject();
    if (!qmlglobal)
        return 0;

    QmlContextWrapper *c = qmlglobal->asQmlContext();
    return c ? c->getContext() : 0;
}

QQmlContextData *QV8ContextWrapper::context(const Value &value)
{
    Object *o = value.asObject();
    QmlContextWrapper *c = o ? o->asQmlContext() : 0;
    if (!c)
        return 0;

    return c ? c->getContext():0;
}

void QV8ContextWrapper::takeContextOwnership(const Value &qmlglobal)
{
    Object *o = qmlglobal.asObject();
    QmlContextWrapper *c = o ? o->asQmlContext() : 0;
    assert(c);
    c->ownsContext = true;
}

Value QmlContextWrapper::get(Managed *m, ExecutionContext *ctx, String *name, bool *hasProperty)
{
    QmlContextWrapper *resource = m->asQmlContext();
    if (!m)
        ctx->throwTypeError();

    bool hasProp;
    Value result = Object::get(m, ctx, name, &hasProp);
    if (hasProp) {
        if (hasProperty)
            *hasProperty = hasProp;
        return result;
    }

    // Its possible we could delay the calculation of the "actual" context (in the case
    // of sub contexts) until it is definately needed.
    QQmlContextData *context = resource->getContext();
    QQmlContextData *expressionContext = context;

    if (!context) {
        if (hasProperty)
            *hasProperty = true;
        return result;
    }

    // Search type (attached property/enum/imported scripts) names
    // while (context) {
    //     Search context properties
    //     Search scope object
    //     Search context object
    //     context = context->parent
    // }

    QV8Engine *engine = resource->v8;

    QObject *scopeObject = resource->getScopeObject();

    QHashedV4String propertystring(Value::fromString(name));

    if (context->imports && name->startsWithUpper()) {
        // Search for attached properties, enums and imported scripts
        QQmlTypeNameCache::Result r = context->imports->query(propertystring);

        if (r.isValid()) {
            if (hasProperty)
                *hasProperty = true;
            if (r.scriptIndex != -1) {
                int index = r.scriptIndex;
                if (index < context->importedScripts.count())
                    return context->importedScripts.at(index).value();
                else
                    return QV4::Value::undefinedValue();
            } else if (r.type) {
                return engine->typeWrapper()->newObject(scopeObject, r.type)->v4Value();
            } else if (r.importNamespace) {
                return engine->typeWrapper()->newObject(scopeObject, context->imports, r.importNamespace)->v4Value();
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
                    if (hasProperty)
                        *hasProperty = true;
                    return engine->newQObject(context->idValues[propertyIdx]);
                } else {

                    QQmlContextPrivate *cp = context->asQQmlContextPrivate();

                    ep->captureProperty(context->asQQmlContext(), -1,
                                        propertyIdx + cp->notifyIndex);

                    const QVariant &value = cp->propertyValues.at(propertyIdx);
                    if (hasProperty)
                        *hasProperty = true;
                    if (value.userType() == qMetaTypeId<QList<QObject*> >()) {
                        QQmlListProperty<QObject> prop(context->asQQmlContext(), (void*) qintptr(propertyIdx),
                                                               QQmlContextPrivate::context_count,
                                                               QQmlContextPrivate::context_at);
                        return engine->listWrapper()->newList(prop, qMetaTypeId<QQmlListProperty<QObject> >())->v4Value();
                    } else {
                        return engine->fromVariant(cp->propertyValues.at(propertyIdx));
                    }
                }
            }
        }

        // Search scope object
        if (scopeObject) {
            QV4::Value wrapper = qobjectWrapper->newQObject(scopeObject)->v4Value();
            if (QV4::Object *o = wrapper.asObject()) {
                bool hasProp = false;
                QV4::Value result = o->get(o->engine()->current, propertystring.string().asString(), &hasProp);
                if (hasProp) {
                    if (hasProperty)
                        *hasProperty = true;
                    return result;
                }
            }
        }
        scopeObject = 0;


        // Search context object
        if (context->contextObject) {
            QV4::Value result = qobjectWrapper->getProperty(context->contextObject, propertystring,
                                                                       context, QV8QObjectWrapper::CheckRevision)->v4Value();
            if (!result.isEmpty()) {
                if (hasProperty)
                    *hasProperty = true;
                return result;
            }
        }

        context = context->parent;
    }

    expressionContext->unresolvedNames = true;

    return Value::undefinedValue();
}

void QmlContextNullWrapper::put(Managed *m, ExecutionContext *ctx, String *name, const Value &value)
{
    QmlContextWrapper *w = m->asQmlContext();
    if (w && w->readOnly) {
        QString error = QLatin1String("Invalid write to global property \"") + name->toQString() +
                        QLatin1Char('"');
        ctx->throwError(Value::fromString(ctx->engine->newString(error)));
    }

    Object::put(m, ctx, name, value);
}

void QmlContextWrapper::put(Managed *m, ExecutionContext *ctx, String *name, const Value &value)
{
    QmlContextWrapper *wrapper = m->asQmlContext();
    if (!m)
        ctx->throwTypeError();

    PropertyAttributes attrs;
    Property *pd  = wrapper->__getOwnProperty__(name, &attrs);
    if (pd) {
        wrapper->putValue(ctx, pd, attrs, value);
        return;
    }

    // Its possible we could delay the calculation of the "actual" context (in the case
    // of sub contexts) until it is definately needed.
    QQmlContextData *context = wrapper->getContext();
    QQmlContextData *expressionContext = context;

    if (!context)
        return;

    // See QV8ContextWrapper::Getter for resolution order

    QV8Engine *engine = wrapper->v8;
    QObject *scopeObject = wrapper->getScopeObject();

    QHashedV4String propertystring(Value::fromString(name));

    QV8QObjectWrapper *qobjectWrapper = engine->qobjectWrapper();

    while (context) {
        // Search context properties
        if (context->propertyNames && -1 != context->propertyNames->value(propertystring))
            return;

        // Search scope object
        if (scopeObject &&
            qobjectWrapper->setProperty(scopeObject, propertystring, context, value, QV8QObjectWrapper::CheckRevision))
            return;
        scopeObject = 0;

        // Search context object
        if (context->contextObject &&
            qobjectWrapper->setProperty(context->contextObject, propertystring, context, value,
                                        QV8QObjectWrapper::CheckRevision))
            return;

        context = context->parent;
    }

    expressionContext->unresolvedNames = true;

    if (wrapper->readOnly) {
        QString error = QLatin1String("Invalid write to global property \"") + name->toQString() +
                        QLatin1Char('"');
        ctx->throwError(Value::fromString(ctx->engine->newString(error)));
    }

    Object::put(m, ctx, name, value);
}

void QmlContextWrapper::destroy(Managed *that)
{
    static_cast<QmlContextWrapper *>(that)->~QmlContextWrapper();
}


QT_END_NAMESPACE
