/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qdeclarativeexpression.h"
#include "private/qdeclarativeexpression_p.h"

#include "private/qdeclarativeengine_p.h"
#include "private/qdeclarativecontext_p.h"
#include "private/qdeclarativerewrite_p.h"
#include "private/qdeclarativecompiler_p.h"

#include <QtCore/qdebug.h>
#include <QtScript/qscriptprogram.h>

#include <private/qscriptdeclarativeclass_p.h>

QT_BEGIN_NAMESPACE

bool QDeclarativeDelayedError::addError(QDeclarativeEnginePrivate *e)
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

QDeclarativeQtScriptExpression::QDeclarativeQtScriptExpression()
: dataRef(0), expressionFunctionMode(ExplicitContext), scopeObject(0), trackChange(false), 
  guardList(0), guardListLength(0), guardObject(0), guardObjectNotifyIndex(-1), deleted(0)
{
}

QDeclarativeQtScriptExpression::~QDeclarativeQtScriptExpression()
{
    v8function.Dispose();
    v8qmlscope.Dispose();
    v8function = v8::Persistent<v8::Function>();
    v8qmlscope = v8::Persistent<v8::Function>();

    if (guardList) { delete [] guardList; guardList = 0; }
    if (dataRef) dataRef->release();
    if (deleted) *deleted = true;
}

QDeclarativeExpressionPrivate::QDeclarativeExpressionPrivate()
: expressionFunctionValid(true), line(-1)
{
}

QDeclarativeExpressionPrivate::~QDeclarativeExpressionPrivate()
{
}

void QDeclarativeExpressionPrivate::init(QDeclarativeContextData *ctxt, const QString &expr, 
                                         QObject *me)
{
    expression = expr;

    QDeclarativeAbstractExpression::setContext(ctxt);
    scopeObject = me;
    expressionFunctionValid = false;
}

void QDeclarativeExpressionPrivate::init(QDeclarativeContextData *ctxt, v8::Handle<v8::Function> func,
                                         QObject *me)
{
    // XXX aakenned
    // expression = func.toString();

    QDeclarativeAbstractExpression::setContext(ctxt);
    scopeObject = me;

    v8function = v8::Persistent<v8::Function>::New(func);
    expressionFunctionMode = ExplicitContext;
    expressionFunctionValid = true;
}

void QDeclarativeExpressionPrivate::init(QDeclarativeContextData *ctxt, void *expr, 
                                         QDeclarativeRefCount *rc, 
                                         QObject *me, const QString &srcUrl, int lineNumber)
{
    url = srcUrl;
    line = lineNumber;

    Q_ASSERT(!dataRef);

    dataRef = rc;
    if (dataRef) dataRef->addref();

    quint32 *exprData = (quint32 *)expr;
    QDeclarativeCompiledData *dd = (QDeclarativeCompiledData *)rc;

    expression = QString::fromRawData((QChar *)(exprData + 2), exprData[1]);

    int progIdx = *(exprData);
    bool isSharedProgram = progIdx & 0x80000000;
    progIdx &= 0x7FFFFFFF;

    v8function = evalFunction(ctxt, me, expression, url, line, &v8qmlscope);

    expressionFunctionMode = ExplicitContext;
    expressionFunctionValid = true;

    QDeclarativeAbstractExpression::setContext(ctxt);
    scopeObject = me;
}

// Callee owns the persistent handle
v8::Persistent<v8::Function> 
QDeclarativeExpressionPrivate::evalFunction(QDeclarativeContextData *ctxt, QObject *scope, 
                                            const QString &code, const QString &filename, int line,
                                            v8::Persistent<v8::Object> *qmlscope)
{
    QDeclarativeEngine *engine = ctxt->engine;
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);

    // XXX aakenned optimize
    v8::HandleScope handle_scope;
    v8::Context::Scope ctxtscope(ep->v8engine.context());
    
    // XXX try/catch?

    v8::Local<v8::Object> scopeobject = ep->v8engine.qmlScope(ctxt, scope);
    v8::Local<v8::Script> script = ep->v8engine.qmlModeCompile(code, filename, line);
    v8::Local<v8::Value> result = script->Run(scopeobject);
    if (qmlscope) *qmlscope = v8::Persistent<v8::Object>::New(scopeobject);
    return v8::Persistent<v8::Function>::New(v8::Local<v8::Function>::Cast(result));
}

QScriptValue QDeclarativeExpressionPrivate::evalInObjectScope(QDeclarativeContextData *context, QObject *object, 
                                                              const QString &program, const QString &fileName,
                                                              int lineNumber, QScriptValue *contextObject)
{
#if 0
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(context->engine);
    QScriptContext *scriptContext = QScriptDeclarativeClass::pushCleanContext(&ep->scriptEngine);
    if (contextObject) {
        *contextObject = ep->contextClass->newContext(context, object);
        scriptContext->pushScope(*contextObject);
    } else {
        scriptContext->pushScope(ep->contextClass->newContext(context, object));
    }
    scriptContext->pushScope(ep->globalClass->staticGlobalObject());
    QScriptValue rv = ep->scriptEngine.evaluate(program, fileName, lineNumber);
    ep->scriptEngine.popContext();
    return rv;
#else
    qFatal("Not impl");
#endif
}

QScriptValue QDeclarativeExpressionPrivate::evalInObjectScope(QDeclarativeContextData *context, QObject *object, 
                                                              const QScriptProgram &program, 
                                                              QScriptValue *contextObject)
{
#if 0
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(context->engine);
    QScriptContext *scriptContext = QScriptDeclarativeClass::pushCleanContext(&ep->scriptEngine);
    if (contextObject) {
        *contextObject = ep->contextClass->newContext(context, object);
        scriptContext->pushScope(*contextObject);
    } else {
        scriptContext->pushScope(ep->contextClass->newContext(context, object));
    }
    scriptContext->pushScope(ep->globalClass->staticGlobalObject());
    QScriptValue rv = ep->scriptEngine.evaluate(program);
    ep->scriptEngine.popContext();
    return rv;
#else
    qFatal("Not impl");
#endif
}

/*!
    \class QDeclarativeExpression
    \since 4.7
    \brief The QDeclarativeExpression class evaluates JavaScript in a QML context.

    For example, given a file \c main.qml like this:

    \qml
    import QtQuick 1.0

    Item {
        width: 200; height: 200
    }
    \endqml

    The following code evaluates a JavaScript expression in the context of the
    above QML:

    \code
    QDeclarativeEngine *engine = new QDeclarativeEngine;
    QDeclarativeComponent component(engine, QUrl::fromLocalFile("main.qml"));

    QObject *myObject = component.create();
    QDeclarativeExpression *expr = new QDeclarativeExpression(engine->rootContext(), myObject, "width * 2");
    int result = expr->evaluate().toInt();  // result = 400
    \endcode
*/

static int QDeclarativeExpression_notifyIdx = -1;

/*!
    Create an invalid QDeclarativeExpression.

    As the expression will not have an associated QDeclarativeContext, this will be a
    null expression object and its value will always be an invalid QVariant.
 */
QDeclarativeExpression::QDeclarativeExpression()
: QObject(*new QDeclarativeExpressionPrivate, 0)
{
    Q_D(QDeclarativeExpression);

    if (QDeclarativeExpression_notifyIdx == -1) 
        QDeclarativeExpression_notifyIdx = QDeclarativeExpression::staticMetaObject.indexOfMethod("_q_notify()");
    d->setNotifyObject(this, QDeclarativeExpression_notifyIdx);
}

/*!  \internal */
QDeclarativeExpression::QDeclarativeExpression(QDeclarativeContextData *ctxt, void *expr,
                                               QDeclarativeRefCount *rc, QObject *me, 
                                               const QString &url, int lineNumber,
                                               QDeclarativeExpressionPrivate &dd)
: QObject(dd, 0)
{
    Q_D(QDeclarativeExpression);
    d->init(ctxt, expr, rc, me, url, lineNumber);

    if (QDeclarativeExpression_notifyIdx == -1) 
        QDeclarativeExpression_notifyIdx = QDeclarativeExpression::staticMetaObject.indexOfMethod("_q_notify()");
    d->setNotifyObject(this, QDeclarativeExpression_notifyIdx);
}

/*!
    Create a QDeclarativeExpression object that is a child of \a parent.

    The \a expression JavaScript will be executed in the \a ctxt QDeclarativeContext.
    If specified, the \a scope object's properties will also be in scope during
    the expression's execution.
*/
QDeclarativeExpression::QDeclarativeExpression(QDeclarativeContext *ctxt,
                                               QObject *scope,
                                               const QString &expression,
                                               QObject *parent)
: QObject(*new QDeclarativeExpressionPrivate, parent)
{
    Q_D(QDeclarativeExpression);
    d->init(QDeclarativeContextData::get(ctxt), expression, scope);

    if (QDeclarativeExpression_notifyIdx == -1) 
        QDeclarativeExpression_notifyIdx = QDeclarativeExpression::staticMetaObject.indexOfMethod("_q_notify()");
    d->setNotifyObject(this, QDeclarativeExpression_notifyIdx);
}

/*! 
    \internal
*/
QDeclarativeExpression::QDeclarativeExpression(QDeclarativeContextData *ctxt, QObject *scope,
                                               const QString &expression)
: QObject(*new QDeclarativeExpressionPrivate, 0)
{
    Q_D(QDeclarativeExpression);
    d->init(ctxt, expression, scope);

    if (QDeclarativeExpression_notifyIdx == -1) 
        QDeclarativeExpression_notifyIdx = QDeclarativeExpression::staticMetaObject.indexOfMethod("_q_notify()");
    d->setNotifyObject(this, QDeclarativeExpression_notifyIdx);
}

/*!  \internal */
QDeclarativeExpression::QDeclarativeExpression(QDeclarativeContextData *ctxt, QObject *scope,
                                               const QString &expression, QDeclarativeExpressionPrivate &dd)
: QObject(dd, 0)
{
    Q_D(QDeclarativeExpression);
    d->init(ctxt, expression, scope);

    if (QDeclarativeExpression_notifyIdx == -1) 
        QDeclarativeExpression_notifyIdx = QDeclarativeExpression::staticMetaObject.indexOfMethod("_q_notify()");
    d->setNotifyObject(this, QDeclarativeExpression_notifyIdx);
}

/*!  
    \internal 

    To avoid exposing v8 in the public API, functionPtr must be a pointer to a v8::Handle<v8::Function>.  
    For example:
        v8::Handle<v8::Function> function;
        new QDeclarativeExpression(ctxt, scope, &function, ...);
 */
QDeclarativeExpression::QDeclarativeExpression(QDeclarativeContextData *ctxt, QObject *scope, void *functionPtr,
                       QDeclarativeExpressionPrivate &dd)
: QObject(dd, 0)
{
    v8::Handle<v8::Function> function = *(v8::Handle<v8::Function> *)functionPtr;

    Q_D(QDeclarativeExpression);
    d->init(ctxt, function, scope);

    if (QDeclarativeExpression_notifyIdx == -1)
        QDeclarativeExpression_notifyIdx = QDeclarativeExpression::staticMetaObject.indexOfMethod("_q_notify()");

    d->setNotifyObject(this, QDeclarativeExpression_notifyIdx);
}

/*!
    Destroy the QDeclarativeExpression instance.
*/
QDeclarativeExpression::~QDeclarativeExpression()
{
}

/*!
    Returns the QDeclarativeEngine this expression is associated with, or 0 if there
    is no association or the QDeclarativeEngine has been destroyed.
*/
QDeclarativeEngine *QDeclarativeExpression::engine() const
{
    Q_D(const QDeclarativeExpression);
    return d->context()?d->context()->engine:0;
}

/*!
    Returns the QDeclarativeContext this expression is associated with, or 0 if there
    is no association or the QDeclarativeContext has been destroyed.
*/
QDeclarativeContext *QDeclarativeExpression::context() const
{
    Q_D(const QDeclarativeExpression);
    QDeclarativeContextData *data = d->context();
    return data?data->asQDeclarativeContext():0;
}

/*!
    Returns the expression string.
*/
QString QDeclarativeExpression::expression() const
{
    Q_D(const QDeclarativeExpression);
    return d->expression;
}

/*!
    Set the expression to \a expression.
*/
void QDeclarativeExpression::setExpression(const QString &expression)
{
    Q_D(QDeclarativeExpression);

    d->resetNotifyOnChange();
    d->expression = expression;
    d->expressionFunctionValid = false;
    d->v8function.Dispose();
    d->v8qmlscope.Dispose();
    d->v8function = v8::Persistent<v8::Function>();
    d->v8qmlscope = v8::Persistent<v8::Function>();
}

void QDeclarativeExpressionPrivate::exceptionToError(v8::Handle<v8::Message> message, 
                                                     QDeclarativeError &error)
{
    Q_ASSERT(!message.IsEmpty());

    v8::Handle<v8::Value> name = message->GetScriptResourceName();
    v8::Handle<v8::String> description = message->Get();
    int lineNumber = message->GetLineNumber();

    Q_ASSERT(name->IsString());

    v8::Local<v8::String> file = name->ToString();
    if (file->Length() == 0) 
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

void QDeclarativeExpressionPrivate::exceptionToError(QScriptEngine *scriptEngine, 
                                                     QDeclarativeError &error)
{
    qFatal("Not implemented - we use v8 now");
}

bool QDeclarativeQtScriptExpression::notifyOnValueChange() const
{
    return trackChange;
}

void QDeclarativeQtScriptExpression::setNotifyOnValueChange(bool notify)
{
    trackChange = notify;
    if (!notify && guardList) 
        clearGuards();
}

void QDeclarativeQtScriptExpression::resetNotifyOnChange()
{
    clearGuards();
}

void QDeclarativeQtScriptExpression::setNotifyObject(QObject *object, int notifyIndex)
{
    if (guardList) clearGuards();

    if (!object || notifyIndex == -1) {
        guardObject = 0;
        notifyIndex = -1;
    } else {
        guardObject = object;
        guardObjectNotifyIndex = notifyIndex;

    }
}

void QDeclarativeQtScriptExpression::setEvaluateFlags(EvaluateFlags flags)
{
    evalFlags = flags;
}

QDeclarativeQtScriptExpression::EvaluateFlags QDeclarativeQtScriptExpression::evaluateFlags() const
{
    return evalFlags;
}

v8::Local<v8::Value> QDeclarativeQtScriptExpression::v8value(QObject *secondaryScope, bool *isUndefined)
{
    Q_ASSERT(context() && context()->engine);
    Q_ASSERT(!trackChange || (guardObject && guardObjectNotifyIndex != -1));

    if (v8function.IsEmpty() || v8function->IsUndefined()) {
        if (isUndefined) *isUndefined = true;
        return v8::Local<v8::Value>();
    }

    DeleteWatcher watcher(this);

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(context()->engine);

    bool lastCaptureProperties = ep->captureProperties;
    QPODVector<QDeclarativeEnginePrivate::CapturedProperty> lastCapturedProperties;
    ep->captureProperties = trackChange;
    ep->capturedProperties.copyAndClear(lastCapturedProperties);

    v8::Local<v8::Value> value = eval(secondaryScope, isUndefined);

    if (!watcher.wasDeleted() && trackChange) {
        if (ep->capturedProperties.count() == 0) {

            if (guardList) clearGuards();

        } else {

            updateGuards(ep->capturedProperties);

        }
    }

    lastCapturedProperties.copyAndClear(ep->capturedProperties);
    ep->captureProperties = lastCaptureProperties;

    return value;
}

v8::Local<v8::Value> QDeclarativeQtScriptExpression::eval(QObject *secondaryScope, bool *isUndefined)
{
    Q_ASSERT(context() && context()->engine);
    DeleteWatcher watcher(this);

    QDeclarativeEngine *engine = context()->engine;
    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);

    QObject *restoreSecondaryScope = 0;
    if (secondaryScope) 
        restoreSecondaryScope = ep->v8engine.contextWrapper()->setSecondaryScope(v8qmlscope, secondaryScope);

    v8::TryCatch try_catch;
    v8::Context::Scope scope(ep->v8engine.context()); // XXX is this needed?

    v8::Handle<v8::Object> This;

    if (evaluateFlags() & RequiresThisObject) {
        v8::Handle<v8::Value> value = ep->v8engine.newQObject(scopeObject);
        if (value->IsObject()) This = v8::Handle<v8::Object>::Cast(value);
    }
    if (This.IsEmpty()) {
        This = ep->v8engine.global();
    }

    v8::Local<v8::Value> result = v8function->Call(This, 0, 0);

    if (secondaryScope) 
        ep->v8engine.contextWrapper()->setSecondaryScope(v8qmlscope, restoreSecondaryScope);

    if (isUndefined)
        *isUndefined = try_catch.HasCaught() || result->IsUndefined();

    if (watcher.wasDeleted()) {
    } else if (try_catch.HasCaught()) {
        v8::Local<v8::Message> message = try_catch.Message();
        if (!message.IsEmpty()) {
            QDeclarativeExpressionPrivate::exceptionToError(message, error);
        } else {
            error = QDeclarativeError();
        }
    } else {
        error = QDeclarativeError();
    }

    return result;
}

void QDeclarativeQtScriptExpression::updateGuards(const QPODVector<QDeclarativeEnginePrivate::CapturedProperty> &properties)
{
    Q_ASSERT(guardObject);
    Q_ASSERT(guardObjectNotifyIndex != -1);

    if (properties.count() != guardListLength) {
        QDeclarativeNotifierEndpoint *newGuardList = new QDeclarativeNotifierEndpoint[properties.count()];

        for (int ii = 0; ii < qMin(guardListLength, properties.count()); ++ii) 
           guardList[ii].copyAndClear(newGuardList[ii]);

        delete [] guardList;
        guardList = newGuardList;
        guardListLength = properties.count();
    }

    bool outputWarningHeader = false;
    bool noChanges = true;
    for (int ii = 0; ii < properties.count(); ++ii) {
        QDeclarativeNotifierEndpoint &guard = guardList[ii];
        const QDeclarativeEnginePrivate::CapturedProperty &property = properties.at(ii);

        guard.target = guardObject;
        guard.targetMethod = guardObjectNotifyIndex;

        if (property.notifier != 0) {

            if (!noChanges && guard.isConnected(property.notifier)) {
                // Nothing to do

            } else {
                noChanges = false;

                bool existing = false;
                for (int jj = 0; !existing && jj < ii; ++jj) 
                    if (guardList[jj].isConnected(property.notifier)) 
                        existing = true;

                if (existing) {
                    // duplicate
                    guard.disconnect();
                } else {
                    guard.connect(property.notifier);
                }
            }


        } else if (property.notifyIndex != -1) {

            if (!noChanges && guard.isConnected(property.object, property.notifyIndex)) {
                // Nothing to do

            } else {
                noChanges = false;

                bool existing = false;
                for (int jj = 0; !existing && jj < ii; ++jj) 
                    if (guardList[jj].isConnected(property.object, property.notifyIndex)) 
                        existing = true;

                if (existing) {
                    // duplicate
                    guard.disconnect();
                } else {
                    guard.connect(property.object, property.notifyIndex);
                }
            }

        } else {
            if (!outputWarningHeader) {
                outputWarningHeader = true;
                qWarning() << "QDeclarativeExpression: Expression" << expression
                           << "depends on non-NOTIFYable properties:";
            }

            const QMetaObject *metaObj = property.object->metaObject();
            QMetaProperty metaProp = metaObj->property(property.coreIndex);

            qWarning().nospace() << "    " << metaObj->className() << "::" << metaProp.name();
        }
    }
}

// Must be called with a valid handle scope
v8::Local<v8::Value> QDeclarativeExpressionPrivate::v8value(QObject *secondaryScope, bool *isUndefined)
{
    if (!expressionFunctionValid) {
        QDeclarativeEngine *engine = context()->engine;
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);

        QDeclarativeRewrite::RewriteBinding rewriteBinding;
        rewriteBinding.setName(name);
        bool ok = true;
        const QString code = rewriteBinding(expression, &ok);
        if (ok) v8function = evalFunction(context(), scopeObject, code, url, line, &v8qmlscope);
        expressionFunctionMode = ExplicitContext;
        expressionFunctionValid = true;
    }

    return QDeclarativeQtScriptExpression::v8value(secondaryScope, isUndefined);
}

QVariant QDeclarativeExpressionPrivate::value(QObject *secondaryScope, bool *isUndefined)
{
    Q_Q(QDeclarativeExpression);

    if (!context() || !context()->isValid()) {
        qWarning("QDeclarativeExpression: Attempted to evaluate an expression in an invalid context");
        return QVariant();
    }

    QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(q->engine());
    QVariant rv;

    ep->referenceScarceResources(); // "hold" scarce resources in memory during evaluation.

    {
        v8::HandleScope handle_scope;
        v8::Local<v8::Value> result = v8value(secondaryScope, isUndefined);
        rv = ep->v8engine.toVariant(result, qMetaTypeId<QList<QObject*> >());
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
QVariant QDeclarativeExpression::evaluate(bool *valueIsUndefined)
{
    Q_D(QDeclarativeExpression);
    return d->value(0, valueIsUndefined);
}

/*!
Returns true if the valueChanged() signal is emitted when the expression's evaluated
value changes.
*/
bool QDeclarativeExpression::notifyOnValueChanged() const
{
    Q_D(const QDeclarativeExpression);
    return d->notifyOnValueChange();
}

/*!
  Sets whether the valueChanged() signal is emitted when the
  expression's evaluated value changes.

  If \a notifyOnChange is true, the QDeclarativeExpression will
  monitor properties involved in the expression's evaluation, and emit
  QDeclarativeExpression::valueChanged() if they have changed.  This
  allows an application to ensure that any value associated with the
  result of the expression remains up to date.

  If \a notifyOnChange is false (default), the QDeclarativeExpression
  will not montitor properties involved in the expression's
  evaluation, and QDeclarativeExpression::valueChanged() will never be
  emitted.  This is more efficient if an application wants a "one off"
  evaluation of the expression.
*/
void QDeclarativeExpression::setNotifyOnValueChanged(bool notifyOnChange)
{
    Q_D(QDeclarativeExpression);
    d->setNotifyOnValueChange(notifyOnChange);
}

/*!
    Returns the source file URL for this expression.  The source location must
    have been previously set by calling setSourceLocation().
*/
QString QDeclarativeExpression::sourceFile() const
{
    Q_D(const QDeclarativeExpression);
    return d->url;
}

/*!
    Returns the source file line number for this expression.  The source location 
    must have been previously set by calling setSourceLocation().
*/
int QDeclarativeExpression::lineNumber() const
{
    Q_D(const QDeclarativeExpression);
    return d->line;
}

/*!
    Set the location of this expression to \a line of \a url. This information
    is used by the script engine.
*/
void QDeclarativeExpression::setSourceLocation(const QString &url, int line)
{
    Q_D(QDeclarativeExpression);
    d->url = url;
    d->line = line;
}

/*!
    Returns the expression's scope object, if provided, otherwise 0.

    In addition to data provided by the expression's QDeclarativeContext, the scope
    object's properties are also in scope during the expression's evaluation.
*/
QObject *QDeclarativeExpression::scopeObject() const
{
    Q_D(const QDeclarativeExpression);
    return d->scopeObject;
}

/*!
    Returns true if the last call to evaluate() resulted in an error,
    otherwise false.
    
    \sa error(), clearError()
*/
bool QDeclarativeExpression::hasError() const
{
    Q_D(const QDeclarativeExpression);
    return d->error.isValid();
}

/*!
    Clear any expression errors.  Calls to hasError() following this will
    return false.

    \sa hasError(), error()
*/
void QDeclarativeExpression::clearError()
{
    Q_D(QDeclarativeExpression);
    d->error = QDeclarativeError();
}

/*!
    Return any error from the last call to evaluate().  If there was no error,
    this returns an invalid QDeclarativeError instance.

    \sa hasError(), clearError()
*/

QDeclarativeError QDeclarativeExpression::error() const
{
    Q_D(const QDeclarativeExpression);
    return d->error;
}

/*! \internal */
void QDeclarativeExpressionPrivate::_q_notify()
{
    emitValueChanged();
}

void QDeclarativeQtScriptExpression::clearGuards()
{
    delete [] guardList; 
    guardList = 0; 
    guardListLength = 0;
}

/*!
    \fn void QDeclarativeExpression::valueChanged()

    Emitted each time the expression value changes from the last time it was
    evaluated.  The expression must have been evaluated at least once (by
    calling QDeclarativeExpression::evaluate()) before this signal will be emitted.
*/

void QDeclarativeExpressionPrivate::emitValueChanged()
{
    Q_Q(QDeclarativeExpression);
    emit q->valueChanged();
}

QDeclarativeAbstractExpression::QDeclarativeAbstractExpression()
: m_context(0), m_prevExpression(0), m_nextExpression(0)
{
}

QDeclarativeAbstractExpression::~QDeclarativeAbstractExpression()
{
    if (m_prevExpression) {
        *m_prevExpression = m_nextExpression;
        if (m_nextExpression) 
            m_nextExpression->m_prevExpression = m_prevExpression;
    }
}

QDeclarativeContextData *QDeclarativeAbstractExpression::context() const
{
    return m_context;
}

void QDeclarativeAbstractExpression::setContext(QDeclarativeContextData *context)
{
    if (m_prevExpression) {
        *m_prevExpression = m_nextExpression;
        if (m_nextExpression) 
            m_nextExpression->m_prevExpression = m_prevExpression;
        m_prevExpression = 0;
        m_nextExpression = 0;
    }

    m_context = context;

    if (m_context) {
        m_nextExpression = m_context->expressions;
        if (m_nextExpression) 
            m_nextExpression->m_prevExpression = &m_nextExpression;
        m_prevExpression = &context->expressions;
        m_context->expressions = this;
    }
}

void QDeclarativeAbstractExpression::refresh()
{
}

bool QDeclarativeAbstractExpression::isValid() const
{
    return m_context != 0;
}

QT_END_NAMESPACE

#include <moc_qdeclarativeexpression.cpp>
