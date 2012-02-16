/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmlexpression.h"
#include "qqmlexpression_p.h"

#include "qqmlengine_p.h"
#include "qqmlcontext_p.h"
#include "qqmlrewrite_p.h"
#include "qqmlscriptstring_p.h"
#include "qqmlcompiler_p.h"

#include <QtCore/qdebug.h>

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

QQmlJavaScriptExpression::QQmlJavaScriptExpression(VTable *v)
: m_vtable(v)
{
}

QQmlJavaScriptExpression::~QQmlJavaScriptExpression()
{
    clearGuards();
}

static QQmlJavaScriptExpression::VTable QDeclarativeExpressionPrivate_jsvtable = {
    QQmlExpressionPrivate::expressionIdentifier,
    QQmlExpressionPrivate::expressionChanged
};

QQmlExpressionPrivate::QQmlExpressionPrivate()
: QQmlJavaScriptExpression(&QDeclarativeExpressionPrivate_jsvtable),
  expressionFunctionValid(true), expressionFunctionRewritten(false),
  extractExpressionFromFunction(false), line(-1), dataRef(0)
{
}

QQmlExpressionPrivate::~QQmlExpressionPrivate()
{
    qPersistentDispose(v8qmlscope);
    qPersistentDispose(v8function);
    if (dataRef) dataRef->release();
    dataRef = 0;
}

void QQmlExpressionPrivate::init(QQmlContextData *ctxt, const QString &expr, 
                                         QObject *me)
{
    expression = expr;

    QQmlAbstractExpression::setContext(ctxt);
    setScopeObject(me);
    expressionFunctionValid = false;
    expressionFunctionRewritten = false;
}

void QQmlExpressionPrivate::init(QQmlContextData *ctxt, v8::Handle<v8::Function> func,
                                         QObject *me)
{
    QQmlAbstractExpression::setContext(ctxt);
    setScopeObject(me);

    v8function = qPersistentNew<v8::Function>(func);
    setUseSharedContext(false);
    expressionFunctionValid = true;
    expressionFunctionRewritten = false;
    extractExpressionFromFunction = true;
}

void QQmlExpressionPrivate::init(QQmlContextData *ctxt, const QString &expr,
                                         bool isRewritten, QObject *me, const QString &srcUrl,
                                         int lineNumber, int columnNumber)
{
    url = srcUrl;
    line = lineNumber;
    column = columnNumber;

    expression = expr;

    expressionFunctionValid = false;
    expressionFunctionRewritten = isRewritten;

    QQmlAbstractExpression::setContext(ctxt);
    setScopeObject(me);
}

void QQmlExpressionPrivate::init(QQmlContextData *ctxt, const QByteArray &expr,
                                         bool isRewritten, QObject *me, const QString &srcUrl,
                                         int lineNumber, int columnNumber)
{
    url = srcUrl;
    line = lineNumber;
    column = columnNumber;

    if (isRewritten) {
        expressionFunctionValid = true;
        expressionFunctionRewritten = true;
        v8function = evalFunction(ctxt, me, expr.constData(), expr.length(),
                                  srcUrl, lineNumber, &v8qmlscope);
        setUseSharedContext(false);

        expressionUtf8 = expr;
    } else {
        expression = QString::fromUtf8(expr);

        expressionFunctionValid = false;
        expressionFunctionRewritten = isRewritten;
    }

    QQmlAbstractExpression::setContext(ctxt);
    setScopeObject(me);
}

// Callee owns the persistent handle
v8::Persistent<v8::Function>
QQmlExpressionPrivate::evalFunction(QQmlContextData *ctxt, QObject *scope,
                                            const char *code, int codeLength,
                                            const QString &filename, int line,
                                            v8::Persistent<v8::Object> *qmlscope)
{
    QQmlEngine *engine = ctxt->engine;
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);

    v8::HandleScope handle_scope;
    v8::Context::Scope ctxtscope(ep->v8engine()->context());

    v8::TryCatch tc;
    v8::Local<v8::Object> scopeobject = ep->v8engine()->qmlScope(ctxt, scope);
    v8::Local<v8::Script> script = ep->v8engine()->qmlModeCompile(code, codeLength, filename, line);
    if (tc.HasCaught()) {
        QQmlError error;
        error.setDescription(QLatin1String("Exception occurred during function compilation"));
        error.setLine(line);
        error.setUrl(QUrl::fromLocalFile(filename));
        v8::Local<v8::Message> message = tc.Message();
        if (!message.IsEmpty())
            QQmlExpressionPrivate::exceptionToError(message, error);
        ep->warning(error);
        return v8::Persistent<v8::Function>();
    }
    v8::Local<v8::Value> result = script->Run(scopeobject);
    if (tc.HasCaught()) {
        QQmlError error;
        error.setDescription(QLatin1String("Exception occurred during function evaluation"));
        error.setLine(line);
        error.setUrl(QUrl::fromLocalFile(filename));
        v8::Local<v8::Message> message = tc.Message();
        if (!message.IsEmpty())
            QQmlExpressionPrivate::exceptionToError(message, error);
        ep->warning(error);
        return v8::Persistent<v8::Function>();
    }
    if (qmlscope) *qmlscope = qPersistentNew<v8::Object>(scopeobject);
    return qPersistentNew<v8::Function>(v8::Local<v8::Function>::Cast(result));
}

// Callee owns the persistent handle
v8::Persistent<v8::Function> 
QQmlExpressionPrivate::evalFunction(QQmlContextData *ctxt, QObject *scope, 
                                            const QString &code, const QString &filename, int line,
                                            v8::Persistent<v8::Object> *qmlscope)
{
    QQmlEngine *engine = ctxt->engine;
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);

    v8::HandleScope handle_scope;
    v8::Context::Scope ctxtscope(ep->v8engine()->context());

    v8::TryCatch tc;
    v8::Local<v8::Object> scopeobject = ep->v8engine()->qmlScope(ctxt, scope);
    v8::Local<v8::Script> script = ep->v8engine()->qmlModeCompile(code, filename, line);
    if (tc.HasCaught()) {
        QQmlError error;
        error.setDescription(QLatin1String("Exception occurred during function compilation"));
        error.setLine(line);
        error.setUrl(QUrl::fromLocalFile(filename));
        v8::Local<v8::Message> message = tc.Message();
        if (!message.IsEmpty())
            QQmlExpressionPrivate::exceptionToError(message, error);
        ep->warning(error);
        return v8::Persistent<v8::Function>();
    }
    v8::Local<v8::Value> result = script->Run(scopeobject);
    if (tc.HasCaught()) {
        QQmlError error;
        error.setDescription(QLatin1String("Exception occurred during function evaluation"));
        error.setLine(line);
        error.setUrl(QUrl::fromLocalFile(filename));
        v8::Local<v8::Message> message = tc.Message();
        if (!message.IsEmpty())
            QQmlExpressionPrivate::exceptionToError(message, error);
        ep->warning(error);
        return v8::Persistent<v8::Function>();
    }
    if (qmlscope) *qmlscope = qPersistentNew<v8::Object>(scopeobject);
    return qPersistentNew<v8::Function>(v8::Local<v8::Function>::Cast(result));
}

QQmlExpression *
QQmlExpressionPrivate::create(QQmlContextData *ctxt, QObject *object,
                                      const QString &expr, bool isRewritten,
                                      const QString &url, int lineNumber, int columnNumber)
{
    return new QQmlExpression(ctxt, object, expr, isRewritten, url, lineNumber, columnNumber, *new QQmlExpressionPrivate);
}

/*!
    \class QQmlExpression
    \since 4.7
    \brief The QQmlExpression class evaluates JavaScript in a QML context.

    For example, given a file \c main.qml like this:

    \qml
    import QtQuick 2.0

    Item {
        width: 200; height: 200
    }
    \endqml

    The following code evaluates a JavaScript expression in the context of the
    above QML:

    \code
    QQmlEngine *engine = new QQmlEngine;
    QQmlComponent component(engine, QUrl::fromLocalFile("main.qml"));

    QObject *myObject = component.create();
    QQmlExpression *expr = new QQmlExpression(engine->rootContext(), myObject, "width * 2");
    int result = expr->evaluate().toInt();  // result = 400
    \endcode
*/

/*!
    Create an invalid QQmlExpression.

    As the expression will not have an associated QQmlContext, this will be a
    null expression object and its value will always be an invalid QVariant.
 */
QQmlExpression::QQmlExpression()
: QObject(*new QQmlExpressionPrivate, 0)
{
}

/*!  \internal */
QQmlExpression::QQmlExpression(QQmlContextData *ctxt, 
                                               QObject *object, const QString &expr, bool isRewritten,
                                               const QString &url, int lineNumber, int columnNumber,
                                               QQmlExpressionPrivate &dd)
: QObject(dd, 0)
{
    Q_D(QQmlExpression);
    d->init(ctxt, expr, isRewritten, object, url, lineNumber, columnNumber);
}

/*!  \internal */
QQmlExpression::QQmlExpression(QQmlContextData *ctxt,
                                               QObject *object, const QByteArray &expr,
                                               bool isRewritten,
                                               const QString &url, int lineNumber, int columnNumber,
                                               QQmlExpressionPrivate &dd)
: QObject(dd, 0)
{
    Q_D(QQmlExpression);
    d->init(ctxt, expr, isRewritten, object, url, lineNumber, columnNumber);
}

/*!
    Create a QQmlExpression object that is a child of \a parent.

    The \script provides the expression to be evaluated, the context to evaluate it in,
    and the scope object to evaluate it with.

    This constructor is functionally equivalent to the following, but in most cases
    is more efficient.
    \code
    QQmlExpression expression(script.context(), script.scopeObject(), script.script(), parent);
    \endcode

    \sa QQmlScriptString
*/
QQmlExpression::QQmlExpression(const QQmlScriptString &script, QObject *parent)
: QObject(*new QQmlExpressionPrivate, parent)
{
    Q_D(QQmlExpression);
    bool defaultConstruction = false;

    int id = script.d.data()->bindingId;
    if (id < 0) {
        defaultConstruction = true;
    } else {
        QQmlContextData *ctxtdata = QQmlContextData::get(script.context());

        QQmlEnginePrivate *engine = QQmlEnginePrivate::get(script.context()->engine());
        QQmlCompiledData *cdata = 0;
        QQmlTypeData *typeData = 0;
        if (engine && ctxtdata && !ctxtdata->url.isEmpty()) {
            typeData = engine->typeLoader.get(ctxtdata->url);
            cdata = typeData->compiledData();
        }

        if (cdata)
            d->init(ctxtdata, cdata->primitives.at(id), true, script.scopeObject(),
                    cdata->name, script.d.data()->lineNumber, script.d.data()->columnNumber);
        else
           defaultConstruction = true;

        if (cdata)
            cdata->release();
        if (typeData)
            typeData->release();
    }

    if (defaultConstruction)
        d->init(QQmlContextData::get(script.context()), script.script(), script.scopeObject());
}

/*!
    Create a QQmlExpression object that is a child of \a parent.

    The \a expression JavaScript will be executed in the \a ctxt QQmlContext.
    If specified, the \a scope object's properties will also be in scope during
    the expression's execution.
*/
QQmlExpression::QQmlExpression(QQmlContext *ctxt,
                                               QObject *scope,
                                               const QString &expression,
                                               QObject *parent)
: QObject(*new QQmlExpressionPrivate, parent)
{
    Q_D(QQmlExpression);
    d->init(QQmlContextData::get(ctxt), expression, scope);
}

/*! 
    \internal
*/
QQmlExpression::QQmlExpression(QQmlContextData *ctxt, QObject *scope,
                                               const QString &expression)
: QObject(*new QQmlExpressionPrivate, 0)
{
    Q_D(QQmlExpression);
    d->init(ctxt, expression, scope);
}

/*!  \internal */
QQmlExpression::QQmlExpression(QQmlContextData *ctxt, QObject *scope,
                                               const QString &expression, QQmlExpressionPrivate &dd)
: QObject(dd, 0)
{
    Q_D(QQmlExpression);
    d->init(ctxt, expression, scope);
}

/*!  
    \internal 

    To avoid exposing v8 in the public API, functionPtr must be a pointer to a v8::Handle<v8::Function>.  
    For example:
        v8::Handle<v8::Function> function;
        new QQmlExpression(ctxt, scope, &function, ...);
 */
QQmlExpression::QQmlExpression(QQmlContextData *ctxt, QObject *scope, void *functionPtr,
                                               QQmlExpressionPrivate &dd)
: QObject(dd, 0)
{
    v8::Handle<v8::Function> function = *(v8::Handle<v8::Function> *)functionPtr;

    Q_D(QQmlExpression);
    d->init(ctxt, function, scope);
}

/*!
    Destroy the QQmlExpression instance.
*/
QQmlExpression::~QQmlExpression()
{
}

/*!
    Returns the QQmlEngine this expression is associated with, or 0 if there
    is no association or the QQmlEngine has been destroyed.
*/
QQmlEngine *QQmlExpression::engine() const
{
    Q_D(const QQmlExpression);
    return d->context()?d->context()->engine:0;
}

/*!
    Returns the QQmlContext this expression is associated with, or 0 if there
    is no association or the QQmlContext has been destroyed.
*/
QQmlContext *QQmlExpression::context() const
{
    Q_D(const QQmlExpression);
    QQmlContextData *data = d->context();
    return data?data->asQQmlContext():0;
}

/*!
    Returns the expression string.
*/
QString QQmlExpression::expression() const
{
    Q_D(const QQmlExpression);
    if (d->extractExpressionFromFunction && context()->engine()) {
        QV8Engine *v8engine = QQmlEnginePrivate::getV8Engine(context()->engine());
        v8::HandleScope handle_scope;
        v8::Context::Scope scope(v8engine->context());

        return v8engine->toString(v8::Handle<v8::Value>(d->v8function));
    } else if (!d->expressionUtf8.isEmpty()) {
        return QString::fromUtf8(d->expressionUtf8);
    } else {
        return d->expression;
    }
}

/*!
    Set the expression to \a expression.
*/
void QQmlExpression::setExpression(const QString &expression)
{
    Q_D(QQmlExpression);

    d->resetNotifyOnValueChanged();
    d->expression = expression;
    d->expressionUtf8.clear();
    d->expressionFunctionValid = false;
    d->expressionFunctionRewritten = false;
    qPersistentDispose(d->v8function);
    qPersistentDispose(d->v8qmlscope);
}

void QQmlExpressionPrivate::exceptionToError(v8::Handle<v8::Message> message, 
                                                     QQmlError &error)
{
    Q_ASSERT(!message.IsEmpty());

    v8::Handle<v8::Value> name = message->GetScriptResourceName();
    v8::Handle<v8::String> description = message->Get();
    int lineNumber = message->GetLineNumber();

    v8::Local<v8::String> file = name->IsString()?name->ToString():v8::Local<v8::String>();
    if (file.IsEmpty() || file->Length() == 0) 
        error.setUrl(QUrl(QLatin1String("<Unknown File>")));
    else 
        error.setUrl(QUrl(QV8Engine::toStringStatic(file)));

    error.setLine(lineNumber);
    error.setColumn(-1);

    QString qDescription = QV8Engine::toStringStatic(description);
    if (qDescription.startsWith(QLatin1String("Uncaught ")))
        qDescription = qDescription.mid(9 /* strlen("Uncaught ") */);

    error.setDescription(qDescription);
}

void QQmlJavaScriptExpression::setNotifyOnValueChanged(bool v)
{
    activeGuards.setFlagValue(v);
    if (!v) clearGuards();
}

void QQmlJavaScriptExpression::resetNotifyOnValueChanged()
{
    clearGuards();
}

v8::Local<v8::Value>
QQmlJavaScriptExpression::evaluate(QQmlContextData *context,
                                           v8::Handle<v8::Function> function, bool *isUndefined)
{
    Q_ASSERT(context && context->engine);

    if (function.IsEmpty() || function->IsUndefined()) {
        if (isUndefined) *isUndefined = true;
        return v8::Local<v8::Value>();
    }

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context->engine);

    Q_ASSERT(notifyOnValueChanged() || activeGuards.isEmpty());
    GuardCapture capture(context->engine, this);

    QQmlEnginePrivate::PropertyCapture *lastPropertyCapture = ep->propertyCapture;
    ep->propertyCapture = notifyOnValueChanged()?&capture:0;


    if (notifyOnValueChanged())
        capture.guards.copyAndClearPrepend(activeGuards);

    QQmlContextData *lastSharedContext = 0;
    QObject *lastSharedScope = 0;

    bool sharedContext = useSharedContext();

    // All code that follows must check with watcher before it accesses data members 
    // incase we have been deleted.
    DeleteWatcher watcher(this);

    if (sharedContext) {
        lastSharedContext = ep->sharedContext;
        lastSharedScope = ep->sharedScope;
        ep->sharedContext = context;
        ep->sharedScope = scopeObject();
    }

    v8::Local<v8::Value> result;
    {
        v8::TryCatch try_catch;
        v8::Handle<v8::Object> This = ep->v8engine()->global();
        if (scopeObject() && requiresThisObject()) {
            v8::Handle<v8::Value> value = ep->v8engine()->newQObject(scopeObject());
            if (value->IsObject()) This = v8::Handle<v8::Object>::Cast(value);
        }

        result = function->Call(This, 0, 0);

        if (isUndefined)
            *isUndefined = try_catch.HasCaught() || result->IsUndefined();

        if (watcher.wasDeleted()) {
        } else if (try_catch.HasCaught()) {
            v8::Context::Scope scope(ep->v8engine()->context());
            v8::Local<v8::Message> message = try_catch.Message();
            if (!message.IsEmpty()) {
                QQmlExpressionPrivate::exceptionToError(message, delayedError()->error);
            } else {
                if (hasDelayedError()) delayedError()->error = QQmlError();
            }
        } else {
            if (hasDelayedError()) delayedError()->error = QQmlError();
        }
    }

    if (sharedContext) {
        ep->sharedContext = lastSharedContext;
        ep->sharedScope = lastSharedScope;
    }

    if (capture.errorString) {
        for (int ii = 0; ii < capture.errorString->count(); ++ii)
            qWarning("%s", qPrintable(capture.errorString->at(ii)));
        delete capture.errorString;
        capture.errorString = 0;
    }

    while (Guard *g = capture.guards.takeFirst())
        g->Delete();

    ep->propertyCapture = lastPropertyCapture;

    return result;
}

void QQmlJavaScriptExpression::GuardCapture::captureProperty(QQmlNotifier *n)
{
    if (expression) {

        // Try and find a matching guard
        while (!guards.isEmpty() && !guards.first()->isConnected(n))
            guards.takeFirst()->Delete();

        Guard *g = 0;
        if (!guards.isEmpty()) {
            g = guards.takeFirst();
            g->cancelNotify();
            Q_ASSERT(g->isConnected(n));
        } else {
            g = Guard::New(expression, engine);
            g->connect(n);
        }

        expression->activeGuards.prepend(g);
    }
}

void QQmlJavaScriptExpression::GuardCapture::captureProperty(QObject *o, int c, int n)
{
    if (expression) {
        if (n == -1) {
            if (!errorString) {
                errorString = new QStringList;
                QString preamble = QLatin1String("QQmlExpression: Expression ") +
                                   expression->m_vtable->expressionIdentifier(expression) +
                                   QLatin1String(" depends on non-NOTIFYable properties:");
                errorString->append(preamble);
            }

            const QMetaObject *metaObj = o->metaObject();
            QMetaProperty metaProp = metaObj->property(c);

            QString error = QLatin1String("    ") +
                            QString::fromUtf8(metaObj->className()) +
                            QLatin1String("::") +
                            QString::fromUtf8(metaProp.name());
            errorString->append(error);
        } else {

            // Try and find a matching guard
            while (!guards.isEmpty() && !guards.first()->isConnected(o, n))
                guards.takeFirst()->Delete();

            Guard *g = 0;
            if (!guards.isEmpty()) {
                g = guards.takeFirst();
                g->cancelNotify();
                Q_ASSERT(g->isConnected(o, n));
            } else {
                g = Guard::New(expression, engine);
                g->connect(o, n);
            }

            expression->activeGuards.prepend(g);
        }
    }
}

void QQmlJavaScriptExpression::clearError()
{
    if (m_vtable.hasValue()) {
        m_vtable.value().error = QQmlError();
        m_vtable.value().removeError();
    }
}

QQmlError QQmlJavaScriptExpression::error() const
{
    if (m_vtable.hasValue()) return m_vtable.constValue()->error;
    else return QQmlError();
}

QQmlDelayedError *QQmlJavaScriptExpression::delayedError()
{
    return &m_vtable.value();
}

void QQmlJavaScriptExpression::clearGuards()
{
    while (Guard *g = activeGuards.takeFirst())
        g->Delete();
}

// Must be called with a valid handle scope
v8::Local<v8::Value> QQmlExpressionPrivate::v8value(QObject *secondaryScope, bool *isUndefined)
{
    if (!expressionFunctionValid) {
        bool ok = true;

        QQmlRewrite::RewriteBinding rewriteBinding;
        rewriteBinding.setName(name);
        QString code;
        if (expressionFunctionRewritten)
            code = expression;
        else
            code = rewriteBinding(expression, &ok);

        if (ok) v8function = evalFunction(context(), scopeObject(), code, url, line, &v8qmlscope);
        setUseSharedContext(false);
        expressionFunctionValid = true;
    }


    if (secondaryScope) {
        v8::Local<v8::Value> result;
        QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context()->engine);
        QObject *restoreSecondaryScope = 0;
        restoreSecondaryScope = ep->v8engine()->contextWrapper()->setSecondaryScope(v8qmlscope, secondaryScope);
        result = evaluate(context(), v8function, isUndefined);
        ep->v8engine()->contextWrapper()->setSecondaryScope(v8qmlscope, restoreSecondaryScope);
        return result;
    } else {
        return evaluate(context(), v8function, isUndefined);
    }
}

QVariant QQmlExpressionPrivate::value(QObject *secondaryScope, bool *isUndefined)
{
    Q_Q(QQmlExpression);

    if (!context() || !context()->isValid()) {
        qWarning("QQmlExpression: Attempted to evaluate an expression in an invalid context");
        return QVariant();
    }

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(q->engine());
    QVariant rv;

    ep->referenceScarceResources(); // "hold" scarce resources in memory during evaluation.

    {
        v8::HandleScope handle_scope;
        v8::Context::Scope context_scope(ep->v8engine()->context());
        v8::Local<v8::Value> result = v8value(secondaryScope, isUndefined);
        rv = ep->v8engine()->toVariant(result, qMetaTypeId<QList<QObject*> >());
    }

    ep->dereferenceScarceResources(); // "release" scarce resources if top-level expression evaluation is complete.

    return rv;
}

/*!
    Evaulates the expression, returning the result of the evaluation,
    or an invalid QVariant if the expression is invalid or has an error.

    \a valueIsUndefined is set to true if the expression resulted in an
    undefined value.

    \sa hasError(), error()
*/
QVariant QQmlExpression::evaluate(bool *valueIsUndefined)
{
    Q_D(QQmlExpression);
    return d->value(0, valueIsUndefined);
}

/*!
Returns true if the valueChanged() signal is emitted when the expression's evaluated
value changes.
*/
bool QQmlExpression::notifyOnValueChanged() const
{
    Q_D(const QQmlExpression);
    return d->notifyOnValueChanged();
}

/*!
  Sets whether the valueChanged() signal is emitted when the
  expression's evaluated value changes.

  If \a notifyOnChange is true, the QQmlExpression will
  monitor properties involved in the expression's evaluation, and emit
  QQmlExpression::valueChanged() if they have changed.  This
  allows an application to ensure that any value associated with the
  result of the expression remains up to date.

  If \a notifyOnChange is false (default), the QQmlExpression
  will not montitor properties involved in the expression's
  evaluation, and QQmlExpression::valueChanged() will never be
  emitted.  This is more efficient if an application wants a "one off"
  evaluation of the expression.
*/
void QQmlExpression::setNotifyOnValueChanged(bool notifyOnChange)
{
    Q_D(QQmlExpression);
    d->setNotifyOnValueChanged(notifyOnChange);
}

/*!
    Returns the source file URL for this expression.  The source location must
    have been previously set by calling setSourceLocation().
*/
QString QQmlExpression::sourceFile() const
{
    Q_D(const QQmlExpression);
    return d->url;
}

/*!
    Returns the source file line number for this expression.  The source location 
    must have been previously set by calling setSourceLocation().
*/
int QQmlExpression::lineNumber() const
{
    Q_D(const QQmlExpression);
    return d->line;
}

/*!
    Returns the source file column number for this expression.  The source location
    must have been previously set by calling setSourceLocation().
*/
int QQmlExpression::columnNumber() const
{
    Q_D(const QQmlExpression);
    return d->column;
}

/*!
    Set the location of this expression to \a line of \a url. This information
    is used by the script engine.
*/
void QQmlExpression::setSourceLocation(const QString &url, int line, int column)
{
    Q_D(QQmlExpression);
    d->url = url;
    d->line = line;
    d->column = column;
}

/*!
    Returns the expression's scope object, if provided, otherwise 0.

    In addition to data provided by the expression's QQmlContext, the scope
    object's properties are also in scope during the expression's evaluation.
*/
QObject *QQmlExpression::scopeObject() const
{
    Q_D(const QQmlExpression);
    return d->scopeObject();
}

/*!
    Returns true if the last call to evaluate() resulted in an error,
    otherwise false.
    
    \sa error(), clearError()
*/
bool QQmlExpression::hasError() const
{
    Q_D(const QQmlExpression);
    return d->hasError();
}

/*!
    Clear any expression errors.  Calls to hasError() following this will
    return false.

    \sa hasError(), error()
*/
void QQmlExpression::clearError()
{
    Q_D(QQmlExpression);
    d->clearError();
}

/*!
    Return any error from the last call to evaluate().  If there was no error,
    this returns an invalid QQmlError instance.

    \sa hasError(), clearError()
*/

QQmlError QQmlExpression::error() const
{
    Q_D(const QQmlExpression);
    return d->error();
}

/*!
    \fn void QQmlExpression::valueChanged()

    Emitted each time the expression value changes from the last time it was
    evaluated.  The expression must have been evaluated at least once (by
    calling QQmlExpression::evaluate()) before this signal will be emitted.
*/

void QQmlExpressionPrivate::expressionChanged(QQmlJavaScriptExpression *e)
{
    QQmlExpressionPrivate *This = static_cast<QQmlExpressionPrivate *>(e);
    This->expressionChanged();
}

void QQmlExpressionPrivate::expressionChanged()
{
    Q_Q(QQmlExpression);
    emit q->valueChanged();
}

QString QQmlExpressionPrivate::expressionIdentifier(QQmlJavaScriptExpression *e)
{
    QQmlExpressionPrivate *This = static_cast<QQmlExpressionPrivate *>(e);
    return QLatin1String("\"") + This->expression + QLatin1String("\"");
}

QQmlAbstractExpression::QQmlAbstractExpression()
: m_prevExpression(0), m_nextExpression(0)
{
}

QQmlAbstractExpression::~QQmlAbstractExpression()
{
    if (m_prevExpression) {
        *m_prevExpression = m_nextExpression;
        if (m_nextExpression) 
            m_nextExpression->m_prevExpression = m_prevExpression;
    }

    if (m_context.isT2())
        m_context.asT2()->_s = 0;
}

QQmlContextData *QQmlAbstractExpression::context() const
{
    if (m_context.isT1()) return m_context.asT1();
    else return m_context.asT2()->_c;
}

void QQmlAbstractExpression::setContext(QQmlContextData *context)
{
    if (m_prevExpression) {
        *m_prevExpression = m_nextExpression;
        if (m_nextExpression) 
            m_nextExpression->m_prevExpression = m_prevExpression;
        m_prevExpression = 0;
        m_nextExpression = 0;
    }

    if (m_context.isT1()) m_context = context;
    else m_context.asT2()->_c = context;

    if (context) {
        m_nextExpression = context->expressions;
        if (m_nextExpression) 
            m_nextExpression->m_prevExpression = &m_nextExpression;
        m_prevExpression = &context->expressions;
        context->expressions = this;
    }
}

void QQmlAbstractExpression::refresh()
{
}

bool QQmlAbstractExpression::isValid() const
{
    return context() != 0;
}

QT_END_NAMESPACE

#include <moc_qqmlexpression.cpp>
