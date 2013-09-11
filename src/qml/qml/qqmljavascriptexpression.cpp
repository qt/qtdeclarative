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

#include "qqmljavascriptexpression_p.h"

#include <private/qqmlexpression_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4script_p.h>
#include <private/qv4errorobject_p.h>
#include <private/qv4scopedvalue_p.h>

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

void QQmlDelayedError::setErrorLocation(const QUrl &url, quint16 line, quint16 column)
{
    m_error.setUrl(url);
    m_error.setLine(line);
    m_error.setColumn(column);
}

void QQmlDelayedError::setErrorDescription(const QString &description)
{
    m_error.setDescription(description);
}

void QQmlDelayedError::setErrorObject(QObject *object)
{
    m_error.setObject(object);
}

void QQmlDelayedError::setError(const QV4::Exception &e)
{
    m_error.setDescription(e.value().toQStringNoThrow());
    QV4::ExecutionEngine::StackTrace trace = e.stackTrace();
    if (!trace.isEmpty()) {
        QV4::ExecutionEngine::StackFrame frame = trace.first();
        m_error.setUrl(QUrl(frame.source));
        m_error.setLine(frame.line);
        m_error.setColumn(frame.column);
    }

    m_error.setColumn(-1);
}


QQmlJavaScriptExpression::QQmlJavaScriptExpression(VTable *v)
: m_vtable(v)
{
}

QQmlJavaScriptExpression::~QQmlJavaScriptExpression()
{
    clearGuards();
    if (m_scopeObject.isT2()) // notify DeleteWatcher of our deletion.
        m_scopeObject.asT2()->_s = 0;
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

QV4::Value
QQmlJavaScriptExpression::evaluate(QQmlContextData *context,
                                   const QV4::Value &function, bool *isUndefined)
{
    return evaluate(context, function, 0, 0, isUndefined);
}

QV4::Value
QQmlJavaScriptExpression::evaluate(QQmlContextData *context,
                                   const QV4::Value &function,
                                   int argc, QV4::Value *args,
                                   bool *isUndefined)
{
    Q_ASSERT(context && context->engine);

    if (function.isEmpty() || function.isUndefined()) {
        if (isUndefined)
            *isUndefined = true;
        return QV4::Value::emptyValue();
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

    // All code that follows must check with watcher before it accesses data members
    // incase we have been deleted.
    DeleteWatcher watcher(this);

    QV4::ExecutionEngine *v4 = QV8Engine::getV4(ep->v8engine());
    QV4::Scope scope(v4);
    QV4::ScopedValue result(scope, QV4::Value::undefinedValue());
    QV4::ExecutionContext *ctx = v4->current;
    try {
        QV4::Value This = ep->v8engine()->global();
        if (scopeObject() && requiresThisObject()) {
            QV4::Value value = QV4::QObjectWrapper::wrap(ctx->engine, scopeObject());
            if (value.isObject())
                This = value;
        }

        QV4::ScopedCallData callData(scope, argc);
        callData->thisObject = This;
        memcpy(callData->args, args, argc*sizeof(QV4::Value));
        result = function.asFunctionObject()->call(callData);

        if (isUndefined)
            *isUndefined = result->isUndefined();

        if (!watcher.wasDeleted() && hasDelayedError())
            delayedError()->clearError();
    } catch (QV4::Exception &e) {
        e.accept(ctx);
        if (isUndefined)
            *isUndefined = true;
        if (!watcher.wasDeleted()) {
            if (!e.value().isEmpty()) {
                delayedError()->setError(e);
            } else {
                if (hasDelayedError()) delayedError()->clearError();
            }
        }
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

/*! \internal
    \reimp

    \a n is in the signal index range (see QObjectPrivate::signalIndex()).
*/
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
                g->connect(o, n, engine);
            }

            expression->activeGuards.prepend(g);
        }
    }
}

void QQmlJavaScriptExpression::clearError()
{
    if (m_vtable.hasValue()) {
        m_vtable.value().clearError();
        m_vtable.value().removeError();
    }
}

QQmlError QQmlJavaScriptExpression::error(QQmlEngine *engine) const
{
    if (m_vtable.hasValue())
        return m_vtable.constValue()->error();
    else
        return QQmlError();
}

QQmlDelayedError *QQmlJavaScriptExpression::delayedError()
{
    return &m_vtable.value();
}

void QQmlJavaScriptExpression::exceptionToError(const QV4::Exception &e, QQmlError &error)
{
    QV4::ExecutionEngine::StackTrace trace = e.stackTrace();
    if (!trace.isEmpty()) {
        QV4::ExecutionEngine::StackFrame frame = trace.first();
        error.setUrl(QUrl(frame.source));
        error.setLine(frame.line);
        error.setColumn(frame.column);
    }
    QV4::ErrorObject *errorObj = e.value().asErrorObject();
    if (errorObj && errorObj->asSyntaxError())
        error.setDescription(QV4::Value::fromReturnedValue(errorObj->get(errorObj->engine()->newString("message"))).toQStringNoThrow());
    else
        error.setDescription(e.value().toQStringNoThrow());
}

QV4::PersistentValue
QQmlJavaScriptExpression::evalFunction(QQmlContextData *ctxt, QObject *scope,
                                       const QString &code, const QString &filename, quint16 line,
                                       QV4::PersistentValue *qmlscope)
{
    QQmlEngine *engine = ctxt->engine;
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);

    QV4::ExecutionEngine *v4 = QV8Engine::getV4(ep->v8engine());
    QV4::ExecutionContext *ctx = v4->current;

    QV4::Value scopeObject = QV4::QmlContextWrapper::qmlScope(ep->v8engine(), ctxt, scope);
    QV4::Script script(v4, scopeObject.asObject(), code, filename, line);
    QV4::Value result;
    try {
        script.parse();
        result = script.run();
    } catch (QV4::Exception &e) {
        e.accept(ctx);
        QQmlError error;
        QQmlExpressionPrivate::exceptionToError(e, error);
        if (error.description().isEmpty())
            error.setDescription(QLatin1String("Exception occurred during function evaluation"));
        if (error.line() == -1)
            error.setLine(line);
        if (error.url().isEmpty())
            error.setUrl(QUrl::fromLocalFile(filename));
        error.setObject(scope);
        ep->warning(error);
        return QV4::PersistentValue();
    }
    if (qmlscope)
        *qmlscope = scopeObject;
    return result;
}

QV4::PersistentValue QQmlJavaScriptExpression::qmlBinding(QQmlContextData *ctxt, QObject *scope,
                                                       const QString &code, const QString &filename, quint16 line,
                                                       QV4::PersistentValue *qmlscope)
{
    QQmlEngine *engine = ctxt->engine;
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);

    QV4::ExecutionEngine *v4 = QV8Engine::getV4(ep->v8engine());
    QV4::ExecutionContext *ctx = v4->current;

    QV4::Value scopeObject = QV4::QmlContextWrapper::qmlScope(ep->v8engine(), ctxt, scope);
    QV4::Script script(v4, scopeObject.asObject(), code, filename, line);
    QV4::Value result;
    try {
        script.parse();
        result = script.qmlBinding();
    } catch (QV4::Exception &e) {
        e.accept(ctx);
        QQmlError error;
        QQmlExpressionPrivate::exceptionToError(e, error);
        if (error.description().isEmpty())
            error.setDescription(QLatin1String("Exception occurred during function evaluation"));
        if (error.line() == -1)
            error.setLine(line);
        if (error.url().isEmpty())
            error.setUrl(QUrl::fromLocalFile(filename));
        error.setObject(scope);
        ep->warning(error);
        return QV4::PersistentValue();
    }
    if (qmlscope)
        *qmlscope = scopeObject;
    return result;
}


void QQmlJavaScriptExpression::clearGuards()
{
    while (Guard *g = activeGuards.takeFirst())
        g->Delete();
}

void QQmlJavaScriptExpressionGuard_callback(QQmlNotifierEndpoint *e, void **)
{
    QQmlJavaScriptExpression *expression =
        static_cast<QQmlJavaScriptExpressionGuard *>(e)->expression;

    expression->m_vtable->expressionChanged(expression);
}

QT_END_NAMESPACE
