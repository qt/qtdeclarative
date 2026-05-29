// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmljavascriptexpression_p.h"
#include "qqmljavascriptexpression_p.h"

#include <private/qqmlexpression_p.h>
#include <private/qv4context_p.h>
#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4script_p.h>
#include <private/qv4errorobject_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4jscall_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qqmlbuiltinfunctions_p.h>
#include <private/qqmlsourcecoordinate_p.h>
#include <private/qqmlabstractbinding_p.h>
#include <private/qqmlpropertybinding_p.h>
#include <private/qproperty_p.h>

QT_BEGIN_NAMESPACE

bool QQmlDelayedError::addError(QQmlEnginePrivate *e)
{
    if (!e) return false;

    if (e->inProgressCreations == 0) return false; // Not in construction

    if (prevError) return true; // Already in error chain

    prevError = &e->erroredBindings;
    nextError = e->erroredBindings;
    e->erroredBindings = this;
    if (nextError) nextError->prevError = &nextError;

    return true;
}

void QQmlDelayedError::setErrorLocation(const QQmlSourceLocation &sourceLocation)
{
    m_error.setUrl(QUrl(sourceLocation.sourceFile));
    m_error.setLine(qmlConvertSourceCoordinate<quint16, int>(sourceLocation.line));
    m_error.setColumn(qmlConvertSourceCoordinate<quint16, int>(sourceLocation.column));
}

void QQmlDelayedError::setErrorDescription(const QString &description)
{
    m_error.setDescription(description);
}

void QQmlDelayedError::setErrorObject(QObject *object)
{
    m_error.setObject(object);
}

void QQmlDelayedError::catchJavaScriptException(QV4::ExecutionEngine *engine)
{
    m_error = engine->catchExceptionAsQmlError();
}


QQmlJavaScriptExpression::QQmlJavaScriptExpression()
    :  m_context(nullptr),
      m_prevExpression(nullptr),
      m_nextExpression(nullptr),
      m_v4Function(nullptr)
{
}

QQmlJavaScriptExpression::~QQmlJavaScriptExpression()
{
    if (m_prevExpression) {
        *m_prevExpression = m_nextExpression;
        if (m_nextExpression)
            m_nextExpression->m_prevExpression = m_prevExpression;
    }

    while (qpropertyChangeTriggers) {
        auto current = qpropertyChangeTriggers;
        qpropertyChangeTriggers = current->next;
        QRecyclePool<TriggerList>::Delete(current);
    }

    clearActiveGuards();
    clearError();
    if (m_scopeObject.isT2()) // notify DeleteWatcher of our deletion.
        m_scopeObject.asT2()->_s = nullptr;
}

QString QQmlJavaScriptExpression::expressionIdentifier() const
{
    if (auto f = function()) {
        QString url = f->sourceFile();
        uint lineNumber = f->compiledFunction->location.line();
        uint columnNumber = f->compiledFunction->location.column();
        return url + QString::asprintf(":%u:%u", lineNumber, columnNumber);
    }

    return QStringLiteral("[native code]");
}

void QQmlJavaScriptExpression::setNotifyOnValueChanged(bool v)
{
    activeGuards.setTag(v ? NotifyOnValueChanged : NoGuardTag);
    if (!v)
        clearActiveGuards();
}

void QQmlJavaScriptExpression::resetNotifyOnValueChanged()
{
    setNotifyOnValueChanged(false);
}

QQmlSourceLocation QQmlJavaScriptExpression::sourceLocation() const
{
    if (m_v4Function)
        return m_v4Function->sourceLocation();
    return QQmlSourceLocation();
}

void QQmlJavaScriptExpression::setContext(const QQmlRefPointer<QQmlContextData> &context)
{
    if (m_prevExpression) {
        *m_prevExpression = m_nextExpression;
        if (m_nextExpression)
            m_nextExpression->m_prevExpression = m_prevExpression;
        m_prevExpression = nullptr;
        m_nextExpression = nullptr;
    }

    m_context = context.data();

    if (context)
        context->addExpression(this);
}

void QQmlJavaScriptExpression::refresh()
{
}

QV4::ReturnedValue QQmlJavaScriptExpression::evaluate(bool *isUndefined)
{
    QQmlEngine *qmlengine = engine();
    if (!qmlengine) {
        if (isUndefined)
            *isUndefined = true;
        return QV4::Encode::undefined();
    }

    QV4::Scope scope(qmlengine->handle());
    QV4::JSCallArguments jsCall(scope);

    return evaluate(jsCall.callData(scope), isUndefined);
}

class QQmlJavaScriptExpressionCapture
{
    Q_DISABLE_COPY_MOVE(QQmlJavaScriptExpressionCapture)
public:
    QQmlJavaScriptExpressionCapture(QQmlJavaScriptExpression *expression, QQmlEngine *engine)
        : watcher(expression)
        , capture(engine, expression, &watcher)
        , ep(QQmlEnginePrivate::get(engine))
    {
        Q_ASSERT(expression->notifyOnValueChanged() || expression->activeGuards.isEmpty());

        lastPropertyCapture = ep->propertyCapture;
        ep->propertyCapture = expression->notifyOnValueChanged() ? &capture : nullptr;

        if (expression->notifyOnValueChanged())
            capture.guards.copyAndClearPrepend(expression->activeGuards);
    }

    ~QQmlJavaScriptExpressionCapture()
    {
        if (capture.errorString) {
            for (int ii = 0; ii < capture.errorString->size(); ++ii)
                qWarning("%s", qPrintable(capture.errorString->at(ii)));
            delete capture.errorString;
            capture.errorString = nullptr;
        }

        while (QQmlJavaScriptExpressionGuard *g = capture.guards.takeFirst())
            g->Delete();

        ep->propertyCapture = lastPropertyCapture;
    }

    bool catchException(const QV4::Scope &scope) const
    {
        if (scope.hasException()) {
            if (watcher.wasDeleted())
                scope.engine->catchException(); // ignore exception
            else
                capture.expression->delayedError()->catchJavaScriptException(scope.engine);
            return true;
        }

        if (!watcher.wasDeleted() && capture.expression->hasDelayedError())
            capture.expression->delayedError()->clearError();
        return false;
    }

private:
    QQmlJavaScriptExpression::DeleteWatcher watcher;
    QQmlPropertyCapture capture;
    QQmlEnginePrivate *ep;
    QQmlPropertyCapture *lastPropertyCapture;
};

QV4::ReturnedValue QQmlJavaScriptExpression::evaluate(QV4::CallData *callData, bool *isUndefined)
{
    QQmlEngine *qmlEngine = engine();
    QV4::Function *v4Function = function();
    if (!v4Function || !qmlEngine) {
        if (isUndefined)
            *isUndefined = true;
        return QV4::Encode::undefined();
    }

    // All code that follows must check with watcher before it accesses data members
    // incase we have been deleted.
    QQmlJavaScriptExpressionCapture capture(this, qmlEngine);

    QV4::Scope scope(qmlEngine->handle());

    if (QObject *thisObject = scopeObject()) {
        callData->thisObject = QV4::QObjectWrapper::wrap(scope.engine, thisObject);
        if (callData->thisObject.isNullOrUndefined())
            callData->thisObject = scope.engine->globalObject;
    } else {
        callData->thisObject = scope.engine->globalObject;
    }

    Q_ASSERT(m_qmlScope.valueRef());
    QV4::ScopedValue result(scope, v4Function->call(
            &(callData->thisObject.asValue<QV4::Value>()),
            callData->argValues<QV4::Value>(), callData->argc(),
            static_cast<QV4::ExecutionContext *>(m_qmlScope.valueRef())));

    if (capture.catchException(scope)) {
        if (isUndefined)
            *isUndefined = true;
    } else if (isUndefined) {
        *isUndefined = result->isUndefined();
    }

    return result->asReturnedValue();
}

bool QQmlJavaScriptExpression::evaluate(void **a, const QMetaType *types, int argc)
{
    // All code that follows must check with watcher before it accesses data members
    // incase we have been deleted.
    QQmlEngine *qmlEngine = engine();

    // If there is no engine, we have no way to evaluate anything.
    // This can happen on destruction.
    if (!qmlEngine)
        return false;

    QQmlJavaScriptExpressionCapture capture(this, qmlEngine);

    QV4::Scope scope(qmlEngine->handle());

    Q_ASSERT(m_qmlScope.valueRef());
    Q_ASSERT(function());
    const bool resultIsDefined = function()->call(
                scopeObject(), a, types, argc,
                static_cast<QV4::ExecutionContext *>(m_qmlScope.valueRef()));

    return !capture.catchException(scope) && resultIsDefined;
}

void QQmlPropertyCapture::captureProperty(QQmlNotifier *n)
{
    if (watcher->wasDeleted())
        return;

    Q_ASSERT(expression);
    // Try and find a matching guard
    while (!guards.isEmpty() && !guards.first()->isConnected(n))
        guards.takeFirst()->Delete();

    QQmlJavaScriptExpressionGuard *g = nullptr;
    if (!guards.isEmpty()) {
        g = guards.takeFirst();
        g->cancelNotify();
        Q_ASSERT(g->isConnected(n));
    } else {
        g = QQmlJavaScriptExpressionGuard::New(expression, engine);
        g->connect(n);
    }

    expression->activeGuards.prepend(g);
}

/*! \internal

    \a n is in the signal index range (see QObjectPrivate::signalIndex()).
*/
void QQmlPropertyCapture::captureProperty(QObject *o, int c, int n, bool doNotify)
{
    if (watcher->wasDeleted())
        return;

    Q_ASSERT(expression);

    // If c < 0 we won't find any property. We better leave the metaobjects alone in that case.
    // QQmlListModel expects us _not_ to trigger the creation of dynamic metaobjects from here.
    if (c >= 0) {
        const QQmlData *ddata = QQmlData::get(o, /*create=*/false);
        const QMetaObject *metaObjectForBindable = nullptr;
        if (auto const propCache = (ddata ? ddata->propertyCache.data() : nullptr)) {
            Q_ASSERT(propCache->property(c));
            if (propCache->property(c)->notifiesViaBindable())
                metaObjectForBindable = propCache->metaObject();
        } else {
            const QMetaObject *m = o->metaObject();
            if (m->property(c).isBindable())
                metaObjectForBindable = m;
        }
        if (metaObjectForBindable) {
            captureBindableProperty(o, metaObjectForBindable, c);
            return;
        }
    }

    captureNonBindableProperty(o, n, c, doNotify);
}

void QQmlPropertyCapture::captureProperty(
        QObject *o, const QQmlPropertyCache *propertyCache, const QQmlPropertyData *propertyData,
        bool doNotify)
{
    if (watcher->wasDeleted())
        return;

    Q_ASSERT(expression);

    if (propertyData->notifiesViaBindable()) {
        if (const QMetaObject *metaObjectForBindable = propertyCache->metaObject()) {
            captureBindableProperty(o, metaObjectForBindable, propertyData->coreIndex());
            return;
        }
    }

    captureNonBindableProperty(o, propertyData->notifyIndex(), propertyData->coreIndex(), doNotify);
}

bool QQmlJavaScriptExpression::needsPropertyChangeTrigger(QObject *target, int propertyIndex)
{
    TriggerList **prev = &qpropertyChangeTriggers;
    TriggerList *current = qpropertyChangeTriggers;
    while (current) {
        if (!current->target) {
            *prev = current->next;
            QRecyclePool<TriggerList>::Delete(current);
            current = *prev;
        } else if (current->target == target && current->propertyIndex == propertyIndex) {
            return false; // already installed
        } else {
            prev = &current->next;
            current = current->next;
        }
    }

    return true;
}

void QQmlPropertyCapture::captureTranslation()
{
    // use a unique invalid index to avoid needlessly querying the metaobject for
    // the correct index of of the  translationLanguage property
    int const invalidIndex = -2;
    if (expression->needsPropertyChangeTrigger(engine, invalidIndex)) {
        auto trigger = expression->allocatePropertyChangeTrigger(engine, invalidIndex);
        trigger->setSource(QQmlEnginePrivate::get(engine)->translationLanguage);
    }
}

void QQmlPropertyCapture::captureBindableProperty(
        QObject *o, const QMetaObject *metaObjectForBindable, int c)
{
    // if the property is a QPropery, and we're binding to a QProperty
    // the automatic capturing process already takes care of everything
    if (!expression->mustCaptureBindableProperty())
        return;

    if (expression->needsPropertyChangeTrigger(o, c)) {
        auto trigger = expression->allocatePropertyChangeTrigger(o, c);
        QUntypedBindable bindable;
        void *argv[] = { &bindable };
        metaObjectForBindable->metacall(o, QMetaObject::BindableProperty, c, argv);
        bindable.observe(trigger);
    }
}

void QQmlPropertyCapture::captureNonBindableProperty(QObject *o, int n, int c, bool doNotify)
{
    if (n == -1) {
        if (!errorString) {
            errorString = new QStringList;
            QString preamble = QLatin1String("QQmlExpression: Expression ") +
                    expression->expressionIdentifier() +
                    QLatin1String(" depends on non-NOTIFYable properties:");
            errorString->append(preamble);
        }

        const QMetaProperty metaProp = o->metaObject()->property(c);
        QString error = QLatin1String("    ") +
                QString::fromUtf8(o->metaObject()->className()) +
                QLatin1String("::") +
                QString::fromUtf8(metaProp.name());
        errorString->append(error);
    } else {

        // Try and find a matching guard
        while (!guards.isEmpty() && !guards.first()->isConnected(o, n))
            guards.takeFirst()->Delete();

        QQmlJavaScriptExpressionGuard *g = nullptr;
        if (!guards.isEmpty()) {
            g = guards.takeFirst();
            g->cancelNotify();
            Q_ASSERT(g->isConnected(o, n));
        } else {
            g = QQmlJavaScriptExpressionGuard::New(expression, engine);
            g->connect(o, n, engine, doNotify);
        }

        expression->activeGuards.prepend(g);
    }
}

QQmlError QQmlJavaScriptExpression::error(QQmlEngine *engine) const
{
    Q_UNUSED(engine);

    if (m_error)
        return m_error->error();
    else
        return QQmlError();
}

QQmlDelayedError *QQmlJavaScriptExpression::delayedError()
{
    if (!m_error)
        m_error = new QQmlDelayedError;
    return m_error.data();
}

QV4::ReturnedValue
QQmlJavaScriptExpression::evalFunction(
        const QQmlRefPointer<QQmlContextData> &ctxt, QObject *scopeObject,
        const QString &code, const QString &filename, quint16 line)
{
    QQmlEngine *engine = ctxt->engine();
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);

    QV4::ExecutionEngine *v4 = engine->handle();
    QV4::Scope scope(v4);

    QV4::Scoped<QV4::QmlContext> qmlContext(scope, QV4::QmlContext::create(v4->rootContext(), ctxt, scopeObject));
    QV4::Script script(v4, qmlContext, /*parse as QML binding*/true, code, filename, line);
    QV4::ScopedValue result(scope);
    script.parse();
    if (!v4->hasException)
        result = script.run();
    if (v4->hasException) {
        QQmlError error = v4->catchExceptionAsQmlError();
        if (error.description().isEmpty())
            error.setDescription(QLatin1String("Exception occurred during function evaluation"));
        if (error.line() == -1)
            error.setLine(line);
        if (error.url().isEmpty())
            error.setUrl(QUrl::fromLocalFile(filename));
        error.setObject(scopeObject);
        ep->warning(error);
        return QV4::Encode::undefined();
    }
    return result->asReturnedValue();
}

void QQmlJavaScriptExpression::createQmlBinding(
        const QQmlRefPointer<QQmlContextData> &ctxt, QObject *qmlScope, const QString &code,
        const QString &filename, quint16 line)
{
    QQmlEngine *engine = ctxt->engine();
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);

    QV4::ExecutionEngine *v4 = engine->handle();
    QV4::Scope scope(v4);

    QV4::Scoped<QV4::QmlContext> qmlContext(scope, QV4::QmlContext::create(v4->rootContext(), ctxt, qmlScope));
    QV4::Script script(v4, qmlContext, /*parse as QML binding*/true, code, filename, line);
    script.parse();
    if (v4->hasException) {
        QQmlDelayedError *error = delayedError();
        error->catchJavaScriptException(v4);
        error->setErrorObject(qmlScope);
        if (!error->addError(ep))
            ep->warning(error->error());
        return;
    }
    setupFunction(qmlContext, script.vmFunction);
}

void QQmlJavaScriptExpression::setupFunction(QV4::ExecutionContext *qmlContext, QV4::Function *f)
{
    if (!qmlContext || !f)
        return;
    m_qmlScope.set(qmlContext->engine(), *qmlContext);
    m_v4Function = f;
    m_compilationUnit.reset(m_v4Function->executableCompilationUnit());
}

void QQmlJavaScriptExpression::setCompilationUnit(const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit)
{
    m_compilationUnit = compilationUnit;
}

void QPropertyChangeTrigger::trigger(QPropertyObserver *observer, QUntypedPropertyData *) {
    auto This = static_cast<QPropertyChangeTrigger *>(observer);
    This->m_expression->expressionChanged();
}

QMetaProperty QPropertyChangeTrigger::property() const
{
    if (!target)
        return {};
    auto const mo = target->metaObject();
    if (!mo)
        return {};
    return mo->property(propertyIndex);
}

QPropertyChangeTrigger *QQmlJavaScriptExpression::allocatePropertyChangeTrigger(QObject *target, int propertyIndex)
{
    auto trigger = QQmlEnginePrivate::get(engine())->qPropertyTriggerPool.New( this );
    trigger->target = target;
    trigger->propertyIndex = propertyIndex;
    auto oldHead = qpropertyChangeTriggers;
    trigger->next = oldHead;
    qpropertyChangeTriggers = trigger;
    return trigger;
}

void QQmlJavaScriptExpression::clearActiveGuards()
{
    while (QQmlJavaScriptExpressionGuard *g = activeGuards.takeFirst())
        g->Delete();
}

void QQmlJavaScriptExpressionGuard_callback(QQmlNotifierEndpoint *e, void **)
{
    QQmlJavaScriptExpression *expression =
        static_cast<QQmlJavaScriptExpressionGuard *>(e)->expression;

    expression->expressionChanged();
}

QT_END_NAMESPACE
