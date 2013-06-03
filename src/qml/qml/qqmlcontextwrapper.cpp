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

#include "qqmlcontextwrapper_p.h"
#include <private/qv8engine_p.h>

#include <private/qqmlengine_p.h>
#include <private/qqmlcontext_p.h>

#include <private/qv4engine_p.h>
#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4objectproto_p.h>
#include <private/qv4mm_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qqmllistwrapper_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

DEFINE_MANAGED_VTABLE(QmlContextWrapper);
DEFINE_MANAGED_VTABLE(QmlContextNullWrapper);

QmlContextWrapper::QmlContextWrapper(QV8Engine *engine, QQmlContextData *context, QObject *scopeObject, bool ownsContext)
    : Object(QV8Engine::getV4(engine)),
      v8(engine), readOnly(true), ownsContext(ownsContext),
      context(context), scopeObject(scopeObject)
{
    vtbl = &static_vtbl;
}

QmlContextWrapper::~QmlContextWrapper()
{
    if (context && ownsContext)
        context->destroy();
}

QV4::Value QmlContextWrapper::qmlScope(QV8Engine *v8, QQmlContextData *ctxt, QObject *scope)
{
    ExecutionEngine *v4 = QV8Engine::getV4(v8);

    QmlContextWrapper *w = new (v4->memoryManager) QmlContextWrapper(v8, ctxt, scope);
    w->prototype = v4->objectPrototype;
    return Value::fromObject(w);
}

QV4::Value QmlContextWrapper::urlScope(QV8Engine *v8, const QUrl &url)
{
    ExecutionEngine *v4 = QV8Engine::getV4(v8);

    QQmlContextData *context = new QQmlContextData;
    context->url = url;
    context->isInternal = true;
    context->isJSContext = true;

    QmlContextWrapper *w = new (v4->memoryManager) QmlContextNullWrapper(v8, context, 0);
    w->prototype = v4->objectPrototype;
    return Value::fromObject(w);
}

QQmlContextData *QmlContextWrapper::callingContext(ExecutionEngine *v4)
{
    QV4::Object *qmlglobal = v4->qmlContextObject();
    if (!qmlglobal)
        return 0;

    QmlContextWrapper *c = qmlglobal->as<QmlContextWrapper>();
    return c ? c->getContext() : 0;
}

QQmlContextData *QmlContextWrapper::getContext(const Value &value)
{
    Object *o = value.asObject();
    QmlContextWrapper *c = o ? o->as<QmlContextWrapper>() : 0;
    if (!c)
        return 0;

    return c ? c->getContext():0;
}

void QmlContextWrapper::takeContextOwnership(const Value &qmlglobal)
{
    Object *o = qmlglobal.asObject();
    QmlContextWrapper *c = o ? o->as<QmlContextWrapper>() : 0;
    assert(c);
    c->ownsContext = true;
}


Value QmlContextWrapper::get(Managed *m, ExecutionContext *ctx, String *name, bool *hasProperty)
{
    QmlContextWrapper *resource = m->as<QmlContextWrapper>();
    if (!resource)
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
                return QmlTypeWrapper::create(engine, scopeObject, r.type);
            } else if (r.importNamespace) {
                return QmlTypeWrapper::create(engine, scopeObject, context->imports, r.importNamespace);
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
                        return QmlListWrapper::create(engine, prop, qMetaTypeId<QQmlListProperty<QObject> >());
                    } else {
                        return engine->fromVariant(cp->propertyValues.at(propertyIdx));
                    }
                }
            }
        }

        // Search scope object
        if (scopeObject) {
            if (QV4::QObjectWrapper *o = qobjectWrapper->newQObject(scopeObject)->v4Value().as<QV4::QObjectWrapper>()) {
                bool hasProp = false;
                QV4::Value result = o->getQmlProperty(o->engine()->current, propertystring.string().asString(), QV4::QObjectWrapper::CheckRevision, &hasProp);
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
                                                                       context, QV4::QObjectWrapper::CheckRevision)->v4Value();
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

void QmlContextWrapper::put(Managed *m, ExecutionContext *ctx, String *name, const Value &value)
{
    QmlContextWrapper *wrapper = m->as<QmlContextWrapper>();
    if (!wrapper)
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
            qobjectWrapper->setProperty(scopeObject, propertystring, context, value, QV4::QObjectWrapper::CheckRevision))
            return;
        scopeObject = 0;

        // Search context object
        if (context->contextObject &&
            qobjectWrapper->setProperty(context->contextObject, propertystring, context, value,
                                        QV4::QObjectWrapper::CheckRevision))
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

void QmlContextNullWrapper::put(Managed *m, ExecutionContext *ctx, String *name, const Value &value)
{
    QmlContextWrapper *w = m->as<QmlContextWrapper>();
    if (w && w->readOnly) {
        QString error = QLatin1String("Invalid write to global property \"") + name->toQString() +
                        QLatin1Char('"');
        ctx->throwError(Value::fromString(ctx->engine->newString(error)));
    }

    Object::put(m, ctx, name, value);
}


QT_END_NAMESPACE
