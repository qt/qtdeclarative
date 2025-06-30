// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlcontextdata_p.h"

#include <QtQml/qqmlengine.h>
#include <QtQml/private/qqmlcomponentattached_p.h>
#include <QtQml/private/qqmljavascriptexpression_p.h>
#include <QtQml/private/qqmlguardedcontextdata_p.h>

QT_BEGIN_NAMESPACE

void QQmlContextData::installContext(QQmlData *ddata, QQmlContextData::QmlObjectKind kind)
{
    Q_ASSERT(ddata);
    if (kind == QQmlContextData::DocumentRoot) {
        if (ddata->context) {
            Q_ASSERT(ddata->context != this);
            Q_ASSERT(ddata->outerContext);
            Q_ASSERT(ddata->outerContext != this);
            QQmlRefPointer<QQmlContextData> c = ddata->context;
            while (QQmlRefPointer<QQmlContextData> linked = c->linkedContext())
                c = linked;
            c->setLinkedContext(this);
        } else {
            ddata->context = this;
        }
        ddata->ownContext.reset(ddata->context);
    } else if (!ddata->context) {
        ddata->context = this;
    }

    addOwnedObject(ddata);
}

QUrl QQmlContextData::resolvedUrl(const QUrl &src) const
{
    QUrl resolved;
    if (src.isRelative() && !src.isEmpty()) {
        const QUrl ownUrl = url();
        if (ownUrl.isValid()) {
            resolved = ownUrl.resolved(src);
        } else {
            for (QQmlRefPointer<QQmlContextData> ctxt = parent(); ctxt; ctxt = ctxt->parent())  {
                const QUrl ctxtUrl = ctxt->url();
                if (ctxtUrl.isValid()) {
                    resolved = ctxtUrl.resolved(src);
                    break;
                }
            }

            if (m_engine && resolved.isEmpty())
                resolved = m_engine->baseUrl().resolved(src);
        }
    } else {
        resolved = src;
    }

    if (resolved.isEmpty()) //relative but no ctxt
        return resolved;

    return m_engine ? m_engine->interceptUrl(resolved, QQmlAbstractUrlInterceptor::UrlString)
                    : resolved;
}

void QQmlContextData::emitDestruction()
{
    if (!m_hasEmittedDestruction) {
        m_hasEmittedDestruction = true;

        // Emit the destruction signal - must be emitted before invalidate so that the
        // context is still valid if bindings or resultant expression evaluation requires it
        if (m_engine) {
            while (m_componentAttacheds) {
                QQmlComponentAttached *attached = m_componentAttacheds;
                attached->removeFromList();
                emit attached->destruction();
            }

            for (QQmlRefPointer<QQmlContextData> child = m_childContexts; !child.isNull(); child = child->m_nextChild)
                child->emitDestruction();
        }
    }
}

void QQmlContextData::invalidate()
{
    emitDestruction();

    while (m_childContexts) {
        Q_ASSERT(m_childContexts != this);
        m_childContexts->invalidate();
    }

    if (m_prevChild) {
        *m_prevChild = m_nextChild;
        if (m_nextChild) m_nextChild->m_prevChild = m_prevChild;
        m_nextChild = nullptr;
        m_prevChild = nullptr;
    }

    if (!m_hasWeakImportedScripts) { // invalidate might be called multiple times
        if (m_engine && !m_importedScripts.isNullOrUndefined()) {
            QV4::Scope scope(m_engine->handle());
            QV4::ScopedValue val(scope, m_importedScripts.value());
            m_importedScripts.~PersistentValue();
            new (&m_weakImportedScripts) QV4::WeakValue();
            m_weakImportedScripts.set(m_engine->handle(), val);
            m_hasWeakImportedScripts = true;
        } else {
            // clear even if the value is null/undefined, in case it was set to explicit null/undefined
            m_importedScripts.clear();
        }
    }

    m_engine = nullptr;
    clearParent();
}

void QQmlContextData::clearContextRecursively()
{
    clearContext();

    for (auto ctxIt = m_childContexts; ctxIt; ctxIt = ctxIt->m_nextChild)
        ctxIt->clearContextRecursively();

    m_engine = nullptr;
}

void QQmlContextData::clearContext()
{
    emitDestruction();

    QQmlJavaScriptExpression *expression = m_expressions;
    while (expression) {
        QQmlJavaScriptExpression *nextExpression = expression->m_nextExpression;

        expression->m_prevExpression = nullptr;
        expression->m_nextExpression = nullptr;

        expression->setContext(nullptr);

        expression = nextExpression;
    }
    m_expressions = nullptr;
}

QQmlContextData::~QQmlContextData()
{
    Q_ASSERT(refCount() == 0);

    // avoid recursion
    addref();
    if (!m_hasWeakImportedScripts) {
        // avoid busy work in invalidate – we don't want to construct a weak value
        // just to throw it away afterwards
        m_importedScripts.clear();
    }
    invalidate();
    if (m_hasWeakImportedScripts)
        m_weakImportedScripts.~WeakValue();
    else
        m_importedScripts.~PersistentValue();
    m_linkedContext.reset();

    Q_ASSERT(refCount() == 1);
    clearContext();
    Q_ASSERT(refCount() == 1);

    while (m_ownedObjects) {
        QQmlData *co = m_ownedObjects;
        m_ownedObjects = m_ownedObjects->nextContextObject;

        if (co->context == this)
            co->context = nullptr;
        co->outerContext = nullptr;
        co->nextContextObject = nullptr;
        co->prevContextObject = nullptr;
    }
    Q_ASSERT(refCount() == 1);

    QQmlGuardedContextData *contextGuard = m_contextGuards;
    while (contextGuard) {
        // TODO: Is this dead code? Why?
        QQmlGuardedContextData *next = contextGuard->next();
        contextGuard->setContextData({});
        contextGuard = next;
    }
    m_contextGuards = nullptr;
    Q_ASSERT(refCount() == 1);

    delete [] m_idValues;
    m_idValues = nullptr;

    Q_ASSERT(refCount() == 1);
    if (m_publicContext)
        delete m_publicContext;

    Q_ASSERT(refCount() == 1);
}

void QQmlContextData::refreshExpressionsRecursive(QQmlJavaScriptExpression *expression)
{
    QQmlJavaScriptExpression::DeleteWatcher w(expression);

    if (expression->m_nextExpression)
        refreshExpressionsRecursive(expression->m_nextExpression);

    if (!w.wasDeleted())
        expression->refresh();
}

void QQmlContextData::refreshExpressionsRecursive(bool isGlobal)
{
    // For efficiency, we try and minimize the number of guards we have to create
    if (hasExpressionsToRun(isGlobal) && (m_nextChild || m_childContexts)) {
        QQmlGuardedContextData guard(this);

        if (m_childContexts)
            m_childContexts->refreshExpressionsRecursive(isGlobal);

        if (guard.isNull()) return;

        if (m_nextChild)
            m_nextChild->refreshExpressionsRecursive(isGlobal);

        if (guard.isNull()) return;

        if (hasExpressionsToRun(isGlobal))
            refreshExpressionsRecursive(m_expressions);

    } else if (hasExpressionsToRun(isGlobal)) {
        refreshExpressionsRecursive(m_expressions);
    } else if (m_nextChild && m_childContexts) {
        QQmlGuardedContextData guard(this);
        m_childContexts->refreshExpressionsRecursive(isGlobal);
        if (!guard.isNull() && m_nextChild)
            m_nextChild->refreshExpressionsRecursive(isGlobal);
    } else if (m_nextChild) {
        m_nextChild->refreshExpressionsRecursive(isGlobal);
    } else if (m_childContexts) {
        m_childContexts->refreshExpressionsRecursive(isGlobal);
    }
}

// Refreshes all expressions that could possibly depend on this context.  Refreshing flushes all
// context-tree dependent caches in the expressions, and should occur every time the context tree
// *structure* (not values) changes.
void QQmlContextData::refreshExpressions()
{
    bool isGlobal = (m_parent == nullptr);

    // For efficiency, we try and minimize the number of guards we have to create
    if (hasExpressionsToRun(isGlobal) && m_childContexts) {
        QQmlGuardedContextData guard(this);
        m_childContexts->refreshExpressionsRecursive(isGlobal);
        if (!guard.isNull() && hasExpressionsToRun(isGlobal))
            refreshExpressionsRecursive(m_expressions);
    } else if (hasExpressionsToRun(isGlobal)) {
        refreshExpressionsRecursive(m_expressions);
    } else if (m_childContexts) {
        m_childContexts->refreshExpressionsRecursive(isGlobal);
    }
}

void QQmlContextData::addOwnedObject(QQmlData *data)
{
    if (data->outerContext) {
        if (data->nextContextObject)
            data->nextContextObject->prevContextObject = data->prevContextObject;
        if (data->prevContextObject)
            *data->prevContextObject = data->nextContextObject;
        else if (data->outerContext->m_ownedObjects == data)
            data->outerContext->m_ownedObjects = data->nextContextObject;
    }

    data->outerContext = this;

    data->nextContextObject = m_ownedObjects;
    if (data->nextContextObject)
        data->nextContextObject->prevContextObject = &data->nextContextObject;
    data->prevContextObject = &m_ownedObjects;
    m_ownedObjects = data;
}

void QQmlContextData::setIdValue(int idx, QObject *obj)
{
    m_idValues[idx] = obj;
    m_idValues[idx].setContext(this);
}

QString QQmlContextData::findObjectId(const QObject *obj) const
{
    for (int ii = 0; ii < m_idValueCount; ii++) {
        if (m_idValues[ii] == obj)
            return propertyName(ii);
    }

    const QVariant objVariant = QVariant::fromValue(obj);
    if (m_publicContext) {
        QQmlContextPrivate *p = QQmlContextPrivate::get(m_publicContext);
        for (int ii = 0; ii < p->numPropertyValues(); ++ii)
            if (p->propertyValue(ii) == objVariant)
                return propertyName(ii);
    }

    if (m_contextObject) {
        // This is expensive, but nameForObject should really mirror contextProperty()
        for (const QMetaObject *metaObject = m_contextObject->metaObject();
             metaObject; metaObject = metaObject->superClass()) {
            for (int i = metaObject->propertyOffset(), end = metaObject->propertyCount();
                 i != end; ++i) {
                const QMetaProperty prop = metaObject->property(i);
                if (prop.metaType().flags() & QMetaType::PointerToQObject
                        && prop.read(m_contextObject) == objVariant) {
                    return QString::fromUtf8(prop.name());
                }
            }
        }
    }

    return QString();
}

void QQmlContextData::initFromTypeCompilationUnit(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit, int subComponentIndex)
{
    m_typeCompilationUnit = unit;
    m_componentObjectIndex = subComponentIndex == -1 ? /*root object*/0 : subComponentIndex;
    Q_ASSERT(!m_idValues);
    m_idValueCount = m_typeCompilationUnit->objectAt(m_componentObjectIndex)
            ->nNamedObjectsInComponent;
    m_idValues = new ContextGuard[m_idValueCount];
}

void QQmlContextData::addComponentAttached(QQmlComponentAttached *attached)
{
    attached->insertIntoList(&m_componentAttacheds);
}

void QQmlContextData::addExpression(QQmlJavaScriptExpression *expression)
{
    expression->insertIntoList(&m_expressions);
}

void QQmlContextData::initPropertyNames() const
{
    if (m_typeCompilationUnit)
        m_propertyNameCache = m_typeCompilationUnit->namedObjectsPerComponent(m_componentObjectIndex);
    else
        m_propertyNameCache = QV4::IdentifierHash(m_engine->handle());
    Q_ASSERT(!m_propertyNameCache.isEmpty());
}

QUrl QQmlContextData::url() const
{
    if (m_typeCompilationUnit)
        return m_typeCompilationUnit->finalUrl();
    return m_baseUrl;
}

QString QQmlContextData::urlString() const
{
    if (m_typeCompilationUnit)
        return m_typeCompilationUnit->finalUrlString();
    return m_baseUrlString;
}

QT_END_NAMESPACE
