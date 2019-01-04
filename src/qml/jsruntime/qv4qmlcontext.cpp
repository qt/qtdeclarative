/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4qmlcontext_p.h"
#include <private/qv8engine_p.h>

#include <private/qqmlengine_p.h>
#include <private/qqmlcontext_p.h>

#include <private/qv4engine_p.h>
#include <private/qv4value_p.h>
#include <private/qv4objectproto_p.h>
#include <private/qv4mm_p.h>
#include <private/qv4function_p.h>
#include <private/qv4compileddata_p.h>
#include <private/qqmltypewrapper_p.h>
#include <private/qqmllistwrapper_p.h>
#include <private/qqmljavascriptexpression_p.h>
#include <private/qjsvalue_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4module_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4identifiertable_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

DEFINE_OBJECT_VTABLE(QQmlContextWrapper);
DEFINE_MANAGED_VTABLE(QmlContext);

void Heap::QQmlContextWrapper::init(QQmlContextData *context, QObject *scopeObject)
{
    Object::init();
    this->context = new QQmlContextDataRef(context);
    this->scopeObject.init(scopeObject);
}

void Heap::QQmlContextWrapper::destroy()
{
    delete context;
    scopeObject.destroy();
    Object::destroy();
}

ReturnedValue QQmlContextWrapper::getPropertyAndBase(const QQmlContextWrapper *resource, PropertyKey id, const Value *receiver, bool *hasProperty, Value *base, Lookup *lookup)
{
    if (!id.isString())
        return Object::virtualGet(resource, id, receiver, hasProperty);

    QV4::ExecutionEngine *v4 = resource->engine();
    QV4::Scope scope(v4);

    if (v4->callingQmlContext() != *resource->d()->context) {
        if (resource->d()->module) {
            Scoped<Module> module(scope, resource->d()->module);
            bool hasProp = false;
            ScopedValue value(scope, module->get(id, receiver, &hasProp));
            if (hasProp) {
                if (hasProperty)
                    *hasProperty = hasProp;
                return value->asReturnedValue();
            }
        }

        return Object::virtualGet(resource, id, receiver, hasProperty);
    }

    bool hasProp = false;
    ScopedValue result(scope, Object::virtualGet(resource, id, receiver, &hasProp));
    if (hasProp) {
        if (hasProperty)
            *hasProperty = hasProp;
        return result->asReturnedValue();
    }

    // It's possible we could delay the calculation of the "actual" context (in the case
    // of sub contexts) until it is definitely needed.
    QQmlContextData *context = resource->getContext();
    QQmlContextData *expressionContext = context;

    if (!context) {
        if (hasProperty)
            *hasProperty = true;
        return result->asReturnedValue();
    }

    // Search type (attached property/enum/imported scripts) names
    // while (context) {
    //     Search context properties
    //     Search scope object
    //     Search context object
    //     context = context->parent
    // }

    QObject *scopeObject = resource->getScopeObject();

    ScopedString name(scope, id.asStringOrSymbol());

    const auto performGobalLookUp = [&result, v4, &name, hasProperty]() {
        bool hasProp = false;
        result = v4->globalObject->get(name, &hasProp);
        if (hasProp) {
            if (hasProperty)
                *hasProperty = hasProp;
            return true;
        }
        return false;
    };

    // If the scope object is a QAbstractDynamicMetaObject, then QMetaObject::indexOfProperty
    // will call createProperty() on the QADMO and implicitly create the property. While that
    // is questionable behavior, there are two use-cases that we support in the light of this:
    //
    //    (1) The implicit creation of properties is necessary because it will also result in
    //        a recorded capture, which will allow a re-evaluation of bindings when the value
    //        is populated later. See QTBUG-35233 and the test-case in tst_qqmlpropertymap.
    //
    //    (1) Looking up "console" in order to place a console.log() call for example must
    //        find the console instead of creating a new property. Therefore we prioritize the
    //        lookup in the global object here.
    //
    // Note: The scope object is only a QADMO for example when somebody registers a QQmlPropertyMap
    // sub-class as QML type and then instantiates it in .qml.
    if (scopeObject && QQmlPropertyCache::isDynamicMetaObject(scopeObject->metaObject())) {
        // all bets are off, so don't try to optimize any lookups
        lookup = nullptr;
        if (performGobalLookUp())
            return result->asReturnedValue();
    }

    if (context->imports && name->startsWithUpper()) {
        // Search for attached properties, enums and imported scripts
        QQmlTypeNameCache::Result r = context->imports->query(name, QQmlImport::AllowRecursion);

        if (r.isValid()) {
            if (hasProperty)
                *hasProperty = true;
            if (r.scriptIndex != -1) {
                if (lookup) {
                    lookup->qmlContextScriptLookup.scriptIndex = r.scriptIndex;
                    lookup->qmlContextPropertyGetter = QQmlContextWrapper::lookupScript;
                    return lookup->qmlContextPropertyGetter(lookup, v4, base);
                }
                QV4::ScopedObject scripts(scope, context->importedScripts.valueRef());
                if (scripts)
                    return scripts->get(r.scriptIndex);
                return QV4::Encode::null();
            } else if (r.type.isValid()) {
                if (lookup) {
                    if (r.type.isSingleton()) {
                        QQmlEngine *e = v4->qmlEngine();
                        QQmlType::SingletonInstanceInfo *siinfo = r.type.singletonInstanceInfo();
                        siinfo->init(e);
                        if (siinfo->qobjectApi(e)) {
                            lookup->qmlContextSingletonLookup.singleton =
                                    static_cast<Heap::Object*>(
                                        Value::fromReturnedValue(
                                            QQmlTypeWrapper::create(v4, nullptr, r.type)
                                        ).heapObject());
                        } else {
                            QV4::ScopedObject o(scope, QJSValuePrivate::convertedToValue(v4, siinfo->scriptApi(e)));
                            lookup->qmlContextSingletonLookup.singleton = o->d();
                        }
                        lookup->qmlContextPropertyGetter = QQmlContextWrapper::lookupSingleton;
                        return lookup->qmlContextPropertyGetter(lookup, v4, base);
                    }
                }
                return QQmlTypeWrapper::create(v4, scopeObject, r.type);
            } else if (r.importNamespace) {
                return QQmlTypeWrapper::create(v4, scopeObject, context->imports, r.importNamespace);
            }
            Q_ASSERT(!"Unreachable");
        }

        // Fall through
    }

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(v4->qmlEngine());

    while (context) {
        // Search context properties
        const QV4::IdentifierHash &properties = context->propertyNames();
        if (properties.count()) {
            int propertyIdx = properties.value(name);

            if (propertyIdx != -1) {

                if (propertyIdx < context->idValueCount) {
                    if (hasProperty)
                        *hasProperty = true;

                    if (lookup) {
                        lookup->qmlContextIdObjectLookup.objectId = propertyIdx;
                        lookup->qmlContextPropertyGetter = QQmlContextWrapper::lookupIdObject;
                        return lookup->qmlContextPropertyGetter(lookup, v4, base);
                    }

                    if (ep->propertyCapture)
                        ep->propertyCapture->captureProperty(&context->idValues[propertyIdx].bindings);
                    return QV4::QObjectWrapper::wrap(v4, context->idValues[propertyIdx]);
                } else {

                    QQmlContextPrivate *cp = context->asQQmlContextPrivate();

                    if (ep->propertyCapture)
                        ep->propertyCapture->captureProperty(context->asQQmlContext(), -1, propertyIdx + cp->notifyIndex);

                    const QVariant &value = cp->propertyValues.at(propertyIdx);
                    if (hasProperty)
                        *hasProperty = true;
                    if (value.userType() == qMetaTypeId<QList<QObject*> >()) {
                        QQmlListProperty<QObject> prop(context->asQQmlContext(), (void*) qintptr(propertyIdx),
                                                               QQmlContextPrivate::context_count,
                                                               QQmlContextPrivate::context_at);
                        return QmlListWrapper::create(v4, prop, qMetaTypeId<QQmlListProperty<QObject> >());
                    } else {
                        return scope.engine->fromVariant(cp->propertyValues.at(propertyIdx));
                    }
                }
            }
        }

        // Search scope object
        if (scopeObject) {
            bool hasProp = false;
            QV4::ScopedValue result(scope, QV4::QObjectWrapper::getQmlProperty(v4, context, scopeObject,
                                                                               name, QV4::QObjectWrapper::CheckRevision, &hasProp));
            if (hasProp) {
                if (hasProperty)
                    *hasProperty = true;
                if (base)
                    *base = QV4::QObjectWrapper::wrap(v4, scopeObject);
                return result->asReturnedValue();
            }
        }
        scopeObject = nullptr;


        // Search context object
        if (context->contextObject) {
            bool hasProp = false;
            result = QV4::QObjectWrapper::getQmlProperty(v4, context, context->contextObject, name, QV4::QObjectWrapper::CheckRevision, &hasProp);
            if (hasProp) {
                if (hasProperty)
                    *hasProperty = true;
                if (base)
                    *base = QV4::QObjectWrapper::wrap(v4, context->contextObject);
                return result->asReturnedValue();
            }
        }

        context = context->parent;

        // As the hierarchy of contexts is not stable, we can't do accelerated lookups beyond
        // the immediate QML context (of the .qml file).
        lookup = nullptr;
    }

    // Do a lookup in the global object here to avoid expressionContext->unresolvedNames becoming
    // true if we access properties of the global object.
    if (performGobalLookUp())
        return result->asReturnedValue();

    expressionContext->unresolvedNames = true;

    return Encode::undefined();
}

ReturnedValue QQmlContextWrapper::virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty)
{
    Q_ASSERT(m->as<QQmlContextWrapper>());
    const QQmlContextWrapper *This = static_cast<const QQmlContextWrapper *>(m);
    return getPropertyAndBase(This, id, receiver, hasProperty, /*base*/nullptr);
}

bool QQmlContextWrapper::virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver)
{
    Q_ASSERT(m->as<QQmlContextWrapper>());

    if (id.isSymbol() || id.isArrayIndex())
        return Object::virtualPut(m, id, value, receiver);

    QQmlContextWrapper *resource = static_cast<QQmlContextWrapper *>(m);
    ExecutionEngine *v4 = resource->engine();
    QV4::Scope scope(v4);
    if (scope.hasException())
        return false;
    QV4::Scoped<QQmlContextWrapper> wrapper(scope, resource);

    auto member = wrapper->internalClass()->findValueOrSetter(id);
    if (member.index < UINT_MAX)
        return wrapper->putValue(member.index, member.attrs, value);

    // It's possible we could delay the calculation of the "actual" context (in the case
    // of sub contexts) until it is definitely needed.
    QQmlContextData *context = wrapper->getContext();
    QQmlContextData *expressionContext = context;

    if (!context)
        return false;

    // See QV8ContextWrapper::Getter for resolution order

    QObject *scopeObject = wrapper->getScopeObject();
    ScopedString name(scope, id.asStringOrSymbol());

    while (context) {
        const QV4::IdentifierHash &properties = context->propertyNames();
        // Search context properties
        if (properties.count()) {
            const int propertyIndex = properties.value(name);
            if (propertyIndex != -1) {
                if (propertyIndex < context->idValueCount) {
                    v4->throwError(QLatin1String("left-hand side of assignment operator is not an lvalue"));
                    return false;
                }
                return false;
            }
        }

        // Search scope object
        if (scopeObject &&
            QV4::QObjectWrapper::setQmlProperty(v4, context, scopeObject, name, QV4::QObjectWrapper::CheckRevision, value))
            return true;
        scopeObject = nullptr;

        // Search context object
        if (context->contextObject &&
            QV4::QObjectWrapper::setQmlProperty(v4, context, context->contextObject, name, QV4::QObjectWrapper::CheckRevision, value))
            return true;

        context = context->parent;
    }

    expressionContext->unresolvedNames = true;

    QString error = QLatin1String("Invalid write to global property \"") + name->toQString() +
            QLatin1Char('"');
    v4->throwError(error);
    return false;
}

ReturnedValue QQmlContextWrapper::resolveQmlContextPropertyLookupGetter(Lookup *l, ExecutionEngine *engine, Value *base)
{
    Scope scope(engine);
    PropertyKey name =engine->identifierTable->asPropertyKey(engine->currentStackFrame->v4Function->compilationUnit->
                                                             runtimeStrings[l->nameIndex]);

    // Special hack for bounded signal expressions, where the parameters of signals are injected
    // into the handler expression through the locals of the call context. So for onClicked: { ... }
    // the parameters of the clicked signal are injected and we must allow for them to be found here
    // before any other property from the QML context.
    ExecutionContext &ctx = static_cast<ExecutionContext &>(engine->currentStackFrame->jsFrame->context);
    if (ctx.d()->type == Heap::ExecutionContext::Type_CallContext) {
        uint index = ctx.d()->internalClass->indexOfValueOrGetter(name);
        if (index < UINT_MAX)
            return static_cast<Heap::CallContext*>(ctx.d())->locals[index].asReturnedValue();
    }

    Scoped<QQmlContextWrapper> qmlContext(scope, engine->qmlContext()->qml());
    bool hasProperty = false;
    ScopedValue result(scope, QQmlContextWrapper::getPropertyAndBase(qmlContext, name, /*receiver*/nullptr,
                                                                     &hasProperty, base, l));
    if (!hasProperty)
        return engine->throwReferenceError(name.toQString());
    return result->asReturnedValue();
}

ReturnedValue QQmlContextWrapper::lookupScript(Lookup *l, ExecutionEngine *engine, Value *base)
{
    Q_UNUSED(base)
    Scope scope(engine);
    Scoped<QmlContext> qmlContext(scope, engine->qmlContext());
    if (!qmlContext)
        return QV4::Encode::null();

    QQmlContextData *context = qmlContext->qmlContext();
    if (!context)
        return QV4::Encode::null();

    QV4::ScopedObject scripts(scope, context->importedScripts.valueRef());
    if (!scripts)
        return QV4::Encode::null();
    return scripts->get(l->qmlContextScriptLookup.scriptIndex);
}

ReturnedValue QQmlContextWrapper::lookupSingleton(Lookup *l, ExecutionEngine *engine, Value *base)
{
    Q_UNUSED(engine)
    Q_UNUSED(base)
    return Value::fromHeapObject(l->qmlContextSingletonLookup.singleton).asReturnedValue();
}

ReturnedValue QQmlContextWrapper::lookupIdObject(Lookup *l, ExecutionEngine *engine, Value *base)
{
    Q_UNUSED(base)
    Scope scope(engine);
    Scoped<QmlContext> qmlContext(scope, engine->qmlContext());
    if (!qmlContext)
        return QV4::Encode::null();

    QQmlContextData *context = qmlContext->qmlContext();
    if (!context)
        return QV4::Encode::null();

    QQmlEnginePrivate *qmlEngine = QQmlEnginePrivate::get(engine->qmlEngine());
    const int objectId = l->qmlContextIdObjectLookup.objectId;

    if (qmlEngine->propertyCapture)
        qmlEngine->propertyCapture->captureProperty(&context->idValues[objectId].bindings);

    return QV4::QObjectWrapper::wrap(engine, context->idValues[objectId]);
}

void Heap::QmlContext::init(QV4::ExecutionContext *outerContext, QV4::QQmlContextWrapper *qml)
{
    Heap::ExecutionContext::init(Heap::ExecutionContext::Type_QmlContext);
    outer.set(internalClass->engine, outerContext->d());

    this->activation.set(internalClass->engine, qml->d());
}

Heap::QmlContext *QmlContext::create(ExecutionContext *parent, QQmlContextData *context, QObject *scopeObject)
{
    Scope scope(parent);

    Scoped<QQmlContextWrapper> qml(scope, scope.engine->memoryManager->allocate<QQmlContextWrapper>(context, scopeObject));
    Heap::QmlContext *c = scope.engine->memoryManager->alloc<QmlContext>(parent, qml);
    Q_ASSERT(c->vtable() == staticVTable());
    return c;
}

QT_END_NAMESPACE
