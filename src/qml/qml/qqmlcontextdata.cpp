/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qqmlcontextdata_p.h"

#include <QtQml/qqmlengine.h>
#include <QtQml/private/qqmlcomponentattached_p.h>
#include <QtQml/private/qqmljavascriptexpression_p.h>
#include <QtQml/private/qqmlguardedcontextdata_p.h>

QT_BEGIN_NAMESPACE

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

    m_importedScripts.clear();

    m_engine = nullptr;
    clearParent();
}

void QQmlContextData::clearContextRecursively()
{
    clearContext();

    for (auto ctxIt = m_childContexts; ctxIt; ctxIt = ctxIt->m_nextChild)
        ctxIt->clearContextRecursively();
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
    if (m_engine)
        invalidate();
    m_linkedContext = nullptr;

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
        QQmlGuardedContextData *next = contextGuard->next();
        contextGuard->reset();
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
