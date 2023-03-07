// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlboundsignal_p.h"

#include <private/qmetaobject_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include "qqmlengine_p.h"
#include "qqmlglobal_p.h"
#include <private/qqmlprofiler_p.h>
#include <private/qqmldebugconnector_p.h>
#include <private/qqmldebugserviceinterfaces_p.h>
#include "qqmlinfo.h"

#include <private/qjsvalue_p.h>
#include <private/qv4value_p.h>
#include <private/qv4jscall_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4qmlcontext_p.h>

#include <QtCore/qdebug.h>

#include <qtqml_tracepoints_p.h>

QT_BEGIN_NAMESPACE

Q_TRACE_POINT(qtqml, QQmlHandlingSignal_entry, const QQmlEngine *engine, const QString &function,
              const QString &fileName, int line, int column)
Q_TRACE_POINT(qtqml, QQmlHandlingSignal_exit)

QQmlBoundSignalExpression::QQmlBoundSignalExpression(const QObject *target, int index, const QQmlRefPointer<QQmlContextData> &ctxt, QObject *scope,
        const QString &expression, const QString &fileName, quint16 line, quint16 column,
        const QString &handlerName, const QString &parameterString)
    : QQmlJavaScriptExpression(),
      m_index(index),
      m_target(target)
{
    init(ctxt, scope);

    QV4::ExecutionEngine *v4 = engine()->handle();

    QString function;

    // Add some leading whitespace to account for the binding's column offset.
    // It's 2 off because a, we start counting at 1 and b, the '(' below is not counted.
    function += QString(qMax(column, (quint16)2) - 2, QChar(QChar::Space))
              + QLatin1String("(function ") + handlerName + QLatin1Char('(');

    if (parameterString.isEmpty()) {
        QString error;
        //TODO: look at using the property cache here (as in the compiler)
        //      for further optimization
        QMetaMethod signal = QMetaObjectPrivate::signal(m_target->metaObject(), m_index);
        function += QQmlPropertyCache::signalParameterStringForJS(v4, signal.parameterNames(), &error);

        if (!error.isEmpty()) {
            qmlWarning(scopeObject()) << error;
            return;
        }
    } else
        function += parameterString;

    function += QLatin1String(") { ") + expression + QLatin1String(" })");
    QV4::Scope valueScope(v4);
    QV4::ScopedFunctionObject f(valueScope, evalFunction(context(), scopeObject(), function, fileName, line));
    QV4::ScopedContext context(valueScope, f->scope());
    setupFunction(context, f->function());
}

QQmlBoundSignalExpression::QQmlBoundSignalExpression(const QObject *target, int index, const QQmlRefPointer<QQmlContextData> &ctxt,
        QObject *scopeObject, QV4::Function *function, QV4::ExecutionContext *scope)
    : QQmlJavaScriptExpression(),
      m_index(index),
      m_target(target)
{
    // It's important to call init first, because m_index gets remapped in case of cloned signals.
    init(ctxt, scopeObject);

    QV4::ExecutionEngine *engine = ctxt->engine()->handle();

    if (!function->isClosureWrapper()) {
        QList<QByteArray> signalParameters = QMetaObjectPrivate::signal(m_target->metaObject(), m_index).parameterNames();
        if (!signalParameters.isEmpty()) {
            QString error;
            QQmlPropertyCache::signalParameterStringForJS(engine, signalParameters, &error);
            if (!error.isEmpty()) {
                qmlWarning(scopeObject) << error;
                return;
            }
            function->updateInternalClass(engine, signalParameters);
        }
    }

    QV4::Scope valueScope(engine);
    QV4::Scoped<QV4::QmlContext> qmlContext(valueScope, scope);
    if (!qmlContext)
        qmlContext = QV4::QmlContext::create(engine->rootContext(), ctxt, scopeObject);
    if (auto closure = function->nestedFunction()) {
        // If the function is marked as having a nested function, then the user wrote:
        //   onSomeSignal: function() { /*....*/ }
        // So take that nested function:
        setupFunction(qmlContext, closure);
    } else {
        setupFunction(qmlContext, function);

        // If it's a closure wrapper but we cannot directly access the nested function
        // we need to run the outer function to get the nested one.
        if (function->isClosureWrapper()) {
            bool isUndefined = false;
            QV4::ScopedFunctionObject result(
                        valueScope, QQmlJavaScriptExpression::evaluate(&isUndefined));

            Q_ASSERT(!isUndefined);
            Q_ASSERT(result->function());
            Q_ASSERT(result->function()->compilationUnit == function->compilationUnit);

            QV4::Scoped<QV4::ExecutionContext> callContext(valueScope, result->scope());
            setupFunction(callContext, result->function());
        }
    }
}

void QQmlBoundSignalExpression::init(const QQmlRefPointer<QQmlContextData> &ctxt, QObject *scope)
{
    setNotifyOnValueChanged(false);
    setContext(ctxt);
    setScopeObject(scope);

    Q_ASSERT(m_target && m_index > -1);
    m_index = QQmlPropertyCache::originalClone(m_target, m_index);
}

QQmlBoundSignalExpression::~QQmlBoundSignalExpression()
{
}

QString QQmlBoundSignalExpression::expressionIdentifier() const
{
    QQmlSourceLocation loc = sourceLocation();
    return loc.sourceFile + QLatin1Char(':') + QString::number(loc.line);
}

void QQmlBoundSignalExpression::expressionChanged()
{
    // bound signals do not notify on change.
}

QString QQmlBoundSignalExpression::expression() const
{
    if (expressionFunctionValid())
        return QStringLiteral("function() { [native code] }");
    return QString();
}

// Parts of this function mirror code in QQmlExpressionPrivate::value() and v4Value().
// Changes made here may need to be made there and vice versa.
void QQmlBoundSignalExpression::evaluate(void **a)
{
    if (!expressionFunctionValid())
        return;

    QQmlEngine *qmlengine = engine();

    // If there is no engine, we have no way to evaluate anything.
    // This can happen on destruction.
    if (!qmlengine)
        return;

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(qmlengine);
    QV4::ExecutionEngine *v4 = qmlengine->handle();
    QV4::Scope scope(v4);

    ep->referenceScarceResources(); // "hold" scarce resources in memory during evaluation.

    if (a) {
        //TODO: lookup via signal index rather than method index as an optimization
        const QMetaObject *targetMeta = m_target->metaObject();
        const QMetaMethod metaMethod = targetMeta->method(
                    QMetaObjectPrivate::signal(targetMeta, m_index).methodIndex());

        int argCount = metaMethod.parameterCount();
        QQmlMetaObject::ArgTypeStorage storage;
        storage.reserve(argCount + 1);
        storage.append(QMetaType()); // We're not interested in the return value
        for (int i = 0; i < argCount; ++i) {
            const QMetaType type = metaMethod.parameterMetaType(i);
            if (!type.isValid())
                argCount = 0;
            else if (type.flags().testFlag(QMetaType::IsEnumeration))
                storage.append(type.underlyingType());
            else
                storage.append(type);
        }

        QQmlJavaScriptExpression::evaluate(a, storage.constData(), argCount);
    } else {
        void *ignoredResult = nullptr;
        QMetaType invalidType;
        QQmlJavaScriptExpression::evaluate(&ignoredResult, &invalidType, 0);
    }

    ep->dereferenceScarceResources(); // "release" scarce resources if top-level expression evaluation is complete.
}

////////////////////////////////////////////////////////////////////////


/*! \internal
    \a signal MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
QQmlBoundSignal::QQmlBoundSignal(QObject *target, int signal, QObject *owner,
                                 QQmlEngine *engine)
    : QQmlNotifierEndpoint(QQmlNotifierEndpoint::QQmlBoundSignal),
      m_prevSignal(nullptr), m_nextSignal(nullptr),
      m_enabled(true)
{
    addToObject(owner);

    /*
        If this is a cloned method, connect to the 'original'. For example,
        for the signal 'void aSignal(int parameter = 0)', if the method
        index refers to 'aSignal()', get the index of 'aSignal(int)'.
        This ensures that 'parameter' will be available from QML.
    */
    signal = QQmlPropertyCache::originalClone(target, signal);
    QQmlNotifierEndpoint::connect(target, signal, engine);
}

QQmlBoundSignal::~QQmlBoundSignal()
{
    removeFromObject();
}

void QQmlBoundSignal::addToObject(QObject *obj)
{
    Q_ASSERT(!m_prevSignal);
    Q_ASSERT(obj);

    QQmlData *data = QQmlData::get(obj, true);

    m_nextSignal = data->signalHandlers;
    if (m_nextSignal) m_nextSignal->m_prevSignal = &m_nextSignal;
    m_prevSignal = &data->signalHandlers;
    data->signalHandlers = this;
}

void QQmlBoundSignal::removeFromObject()
{
    if (m_prevSignal) {
        *m_prevSignal = m_nextSignal;
        if (m_nextSignal) m_nextSignal->m_prevSignal = m_prevSignal;
        m_prevSignal = nullptr;
        m_nextSignal = nullptr;
    }
}

/*!
    Returns the signal expression.
*/
QQmlBoundSignalExpression *QQmlBoundSignal::expression() const
{
    return m_expression.data();
}

/*!
    Sets the signal expression to \a e.

    The QQmlBoundSignal instance takes ownership of \a e (and does not add a reference).
*/
void QQmlBoundSignal::takeExpression(QQmlBoundSignalExpression *e)
{
    m_expression.adopt(e);
    if (m_expression)
        m_expression->setNotifyOnValueChanged(false);
}

/*!
    This property holds whether the item will emit signals.

    The QQmlBoundSignal callback will only emit a signal if this property is set to true.

    By default, this property is true.
 */
void QQmlBoundSignal::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
}

void QQmlBoundSignal_callback(QQmlNotifierEndpoint *e, void **a)
{
    QQmlBoundSignal *s = static_cast<QQmlBoundSignal*>(e);

    if (!s->m_expression || !s->m_enabled)
        return;

    QV4DebugService *service = QQmlDebugConnector::service<QV4DebugService>();
    if (service)
        service->signalEmitted(QString::fromUtf8(QMetaObjectPrivate::signal(
                                                     s->m_expression->target()->metaObject(),
                                                     s->signalIndex()).methodSignature()));

    QQmlEngine *engine;
    if (s->m_expression && (engine = s->m_expression->engine())) {
        Q_TRACE_SCOPE(QQmlHandlingSignal, engine,
                      s->m_expression->function() ? s->m_expression->function()->name()->toQString() : QString(),
                      s->m_expression->sourceLocation().sourceFile, s->m_expression->sourceLocation().line,
                      s->m_expression->sourceLocation().column);
        QQmlHandlingSignalProfiler prof(QQmlEnginePrivate::get(engine)->profiler,
                                        s->m_expression.data());
        s->m_expression->evaluate(a);
        if (s->m_expression && s->m_expression->hasError()) {
            QQmlEnginePrivate::warning(engine, s->m_expression->error(engine));
        }
    }
}

////////////////////////////////////////////////////////////////////////

QQmlPropertyObserver::QQmlPropertyObserver(QQmlBoundSignalExpression *expr)
    : QPropertyObserver([](QPropertyObserver *self, QUntypedPropertyData *) {
                           auto This = static_cast<QQmlPropertyObserver*>(self);
                           This->expression->evaluate(nullptr);
                       })
{
    expression.adopt(expr);
}

QT_END_NAMESPACE
