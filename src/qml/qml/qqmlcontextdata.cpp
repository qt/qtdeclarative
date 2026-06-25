// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qqmlcontextdata_p.h"

#include <QtQml/qqmlengine.h>
#include <QtQml/private/qqmlcomponentattached_p.h>
#include <QtQml/private/qqmljavascriptexpression_p.h>
#include <QtQml/private/qqmlguardedcontextdata_p.h>

QT_BEGIN_NAMESPACE

/*!
    \class QQmlContextData
    \internal

    \brief Runtime embodiment of a single QML component instance's context.

    A QQmlContextData holds the state that backs a QML context: the parent/child
    links, the id values, the imports, the compilation unit, the owned objects,
    the imported scripts and so on.

    Contexts usually form at component boundaries. That is, the root object of
    a document, an inline component or the instantiation point of a Component.
    When looking up unqalified names, the engine travels the context hierarchy
    and considers both context properties and the context object for each
    context. This is the reason why you can implicitly refer to the properties
    of root objects (recursively), but not to the properties of intermediate
    objects in the document tree. You can, however, manually mess with the
    context hierarchy, and some QML types (e.g. models and views) do that.

    The public \l QQmlContext is a thin shell on top of QQmlContextData. The
    relationship is deliberately asymmetric:

    \list
    \li An \e internal context (\c m_isInternal == true) is created by the engine
        while instantiating a component. The QQmlContextData is the primary object
        and \e owns its publicContext, which is minted lazily by asQQmlContext().
        Almost every context created during .qml instantiation is internal.
    \li An \e external context (\c m_isInternal == false) is created when the user
        constructs a QQmlContext explicitly; that QQmlContext owns the
        QQmlContextData.
    \endlist

    QQmlContextData::get() bridges public to private; asQQmlContext()/
    publicContext() bridge back. Lifetime is reference counted (\c m_refCount,
    addref()/release()), with the cycle-avoiding twists described below.

    \section1 The context tree: parent and childContexts

    Contexts form a tree that mirrors the \e{document nesting} of QML, not the
    QObject parent hierarchy and not the visual item tree:

    \list
    \li \c m_parent -- up the tree, to the next \e{component root}.
    \li \c m_childContexts, \c m_nextChild, \c m_prevChild -- an intrusive child
        list with a back-link to the slot that points at each node, so insert and
        unlink are O(1) (see the constructor).
    \endlist

    Parent and child links are \e not reference counted: that would create cyclic
    references. A parent keeps its children alive through ownership (see below);
    children point back at the parent weakly. The tree propagates expression
    refreshes downward (refreshExpressions() and friends walk childContexts())
    and resolves relative URLs upward (url()/baseUrl() walk the parent chain).

    \section1 Ownership: who keeps whom alive

    Because parent->child links are raw, something else must pin contexts. There
    are three ownership modes (\c enum \c Ownership):

    \list
    \li \c RefCounted (createRefCounted()/createBareContext()) - held by whoever
        holds the QQmlRefPointer.
    \li \c OwnedByParent (createChild()) - released when the parent drops it via
        clearParent().
    \li \c OwnedByPublicContext - released via clearPublicContext().
    \endlist

    \c m_ownedByParent and \c m_ownedByPublicContext are mutually exclusive bits.
    Component root contexts created during instantiation are \c RefCounted; the
    field that actually pins a root context in memory is the object side's
    \c{QQmlData::ownContext}, which is a QQmlRefPointer (see below).

    \section1 Binding to a type: the compilation unit

    A context created for a compiled QML type remembers:

    \list
    \li \c m_typeCompilationUnit -- the CU this context belongs to.
    \li \c m_componentObjectIndex -- the object index within that CU of the
        component that created the context (0 for the document root, the IC root
        index for an inline component).
    \endlist

    initFromTypeCompilationUnit() wires these up and sizes the id table from the
    component root's \c nNamedObjectsInComponent. So a context is the runtime
    embodiment of one component instance: it knows its CU, its root object index,
    how many ids that component declares, and the import set (\c m_imports) used
    for name resolution.

    \section1 The object side: QQmlData::context, outerContext, ownContext

    Three QQmlData fields tie a QObject into the context world:

    \list
    \li \c outerContext - the context of the enclosing component instance: where
        the object lives, where its id (if any) is registered, and where name
        lookup for its bindings starts. Siblings created by the same component
        share it. Not refcounted.
    \li \c ownContext - non-null \e only for component roots (the document root,
        an inline-component root, or a base-type level in a composite chain). It
        is the refcounted pointer that keeps the introduced context alive. A plain
        child object has \c ownContext == nullptr.
    \li \c context - the effective context for the object's own bindings:
        \c{== outerContext} for a borrowed child, \c{== ownContext.data()} for a
        root.
    \endlist

    installContext() establishes these (called from
    QQmlObjectCreator::initializeDData). A context also keeps the inverse mapping:
    a doubly-linked list of the QQmlData it owns (\c m_ownedObjects), severed in
    clearOwnedObjects() on destruction.

    Now, given this explanation we clearly have a problem: The same component
    root object can live in and own multiple contexts. It can inherit from
    another QML type after all, with more inner objects. It can even have
    different IDs in different outer contexts. That's where the linked contexts
    come into play.

    \section1 Linked contexts: the composite (base-type) chain

    A QML type may derive from another QML type (MyButton.qml : Button.qml :
    C++ QQuickButton). Each composite (QML-defined) level is a separate
    compilation unit and gets its \e own context, yet they all describe the same
    single QObject. These per-level contexts are chained through
    \c m_linkedContext (\c{this} owns the next link, so the chain is refcounted
    derived->base):

    \list
    \li \c{ddata->outerContext}/\c context/\c ownContext point at the
        most-derived type the enclosing document instantiated.
    \li linkedContext() walks the base types. The deepest-base type is first
        and the most-derived one last.
    \endlist

    Object creation proceeds deepest-base-first, so installContext() appends each
    newly installed (more derived) root to the end of the linked chain. Because
    one QObject spans the whole chain, deepClearContextObject() must sweep every
    link, not just the head, when detaching the context object.

    \section1 Ids: the id-value table and context guards

    Each context owns an array of \c ContextGuard, one per id declared in its
    component (\c m_idValues, \c m_idValueCount). The index<->name mapping is the
    lazily filled \c m_propertyNameCache (propertyIndex()/propertyName()). A
    ContextGuard is a QQmlGuard plus a QQmlNotifier: assigning or destroying an
    id'd object fires the notifier so alias and id-referencing bindings
    re-evaluate. The \c ObjectWasSet tag (wasSet()) distinguishes "slot exists but
    empty" from "set to null". findObjectId() does the reverse name lookup.

    \section1 The context object

    \c m_contextObject is the scope object whose properties are in unqualified
    scope for bindings evaluated in this context (for a root context, the root
    instance). isValid() couples an internal context's validity to the liveness
    of its context object.

    \section1 The extra slot

    A single union slot, discriminated by \c m_hasExtraObject, serves two
    mutually exclusive purposes:

    \list
    \li \c m_incubator - while a context is built asynchronously, the
        QQmlIncubatorPrivate driving construction.
    \li \c m_extraObject - repurposed afterward for component-specific side data;
        currently only QQmlDelegateModel (QQmlDelegateModelItem::dataForObject).
    \endlist

    \section1 Other per-context state

    \list
    \li \c m_expressions - intrusive list of QQmlJavaScriptExpressions evaluated
        in this context; the basis of refreshExpressions().
    \li \c m_importedScripts - the JS array of .import'ed scripts; downgraded
        strong->weak on invalidation so closures keep working without pinning.
    \li \c m_imports - the QQmlTypeNameCache for resolving type names here.
    \li \c m_componentAttacheds - uses of the Component attached property.
    \li \c m_contextGuards - external weak references to this context.
    \li \c m_baseUrl/\c m_baseUrlString - explicit base-URL overrides.
    \endlist
*/

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

    clearChildrenAndSiblings();
    clearImportedScripts();

    m_engine = nullptr;
    clearParent();
}

void QQmlContextData::clearContextRecursively()
{
    emitDestruction();
    clearExpressions();

    for (auto ctxIt = m_childContexts; ctxIt; ctxIt = ctxIt->m_nextChild)
        ctxIt->clearContextRecursively();

    m_engine = nullptr;
}

void QQmlContextData::clearChildrenAndSiblings()
{
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
}

void QQmlContextData::clearImportedScripts()
{
    if (!m_hasWeakImportedScripts) { // might be called multiple times
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
}

void QQmlContextData::clearOwnedObjects()
{
    while (m_ownedObjects) {
        QQmlData *co = m_ownedObjects;
        m_ownedObjects = m_ownedObjects->nextContextObject;

        if (co->context == this)
            co->context = nullptr;
        co->outerContext = nullptr;
        co->nextContextObject = nullptr;
        co->prevContextObject = nullptr;
    }
}

void QQmlContextData::clearContextGuards()
{
    for (QQmlGuardedContextData *contextGuard = m_contextGuards; contextGuard;) {
        // TODO: Is this dead code? Why?
        QQmlGuardedContextData *next = contextGuard->next();
        contextGuard->setContextData({});
        contextGuard = next;
    }
    m_contextGuards = nullptr;
}

void QQmlContextData::clearIdValues()
{
    delete[] std::exchange(m_idValues, nullptr);
    m_idValueCount = 0;
}

void QQmlContextData::clearExpressions()
{
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
    emitDestruction();
    clearExpressions();
    Q_ASSERT(refCount() == 1);

    clearOwnedObjects();
    Q_ASSERT(refCount() == 1);

    clearContextGuards();
    Q_ASSERT(refCount() == 1);

    clearIdValues();

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

void QQmlContextData::setTypeCompilationUnit(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit)
{
    m_propertyNameCache = QV4::IdentifierHash();
    delete[] std::exchange(m_idValues, nullptr);
    initFromTypeCompilationUnit(unit, m_componentObjectIndex);
}

void QQmlContextData::initFromTypeCompilationUnit(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit, int subComponentIndex)
{
    m_typeCompilationUnit = unit;
    m_componentObjectIndex = subComponentIndex == -1 ? /*root object*/0 : subComponentIndex;
    Q_ASSERT(!m_idValues);
    m_idValueCount = m_typeCompilationUnit->objectAt(m_componentObjectIndex)
            ->nNamedObjectsInComponent;
    if (m_idValueCount > 0)
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
    if (m_typeCompilationUnit) {
        m_propertyNameCache = m_typeCompilationUnit->namedObjectsPerComponent(m_componentObjectIndex);
    } else {
        auto engine = m_engine;
        if (!engine) {
            // in some circumstances, we run into an invalidated context. In that case, we have no engine
            // obviously, there's also no names to be found. Ideally, we'd have a special empty IdentifierHash
            // for this which doesn't depend on an engine being available, but that currently doesn't exist.
            // If we're evaluating, we should however still be able to find a parent context with an engine
            for (auto ctxt = parent(); ctxt; ctxt = ctxt->parent()) {
                if ((engine = ctxt->engine()))
                    break;
            }
        }
        Q_ASSERT(engine);
        m_propertyNameCache = QV4::IdentifierHash(engine->handle());
    }
    Q_ASSERT(m_propertyNameCache.isValid());
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
