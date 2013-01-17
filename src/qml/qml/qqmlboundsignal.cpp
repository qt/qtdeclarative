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

#include "qqmlboundsignal_p.h"

#include <private/qmetaobject_p.h>
#include <private/qmetaobjectbuilder_p.h>
#include "qqmlengine_p.h"
#include "qqmlexpression_p.h"
#include "qqmlcontext_p.h"
#include "qqmlmetatype_p.h"
#include "qqml.h"
#include "qqmlcontext.h"
#include "qqmlglobal_p.h"
#include "qqmlrewrite_p.h"
#include <private/qqmlprofilerservice_p.h>
#include <private/qv8debugservice_p.h>
#include "qqmlinfo.h"

#include <QtCore/qstringbuilder.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

static QQmlJavaScriptExpression::VTable QQmlBoundSignalExpression_jsvtable = {
    QQmlBoundSignalExpression::expressionIdentifier,
    QQmlBoundSignalExpression::expressionChanged
};

QQmlBoundSignalExpression::QQmlBoundSignalExpression(QObject *target, int index,
                                                     QQmlContextData *ctxt, QObject *scope, const QByteArray &expression,
                                                     bool isRewritten, const QString &fileName, quint16 line, quint16 column)
    : QQmlJavaScriptExpression(&QQmlBoundSignalExpression_jsvtable),
      m_fileName(fileName),
      m_line(line),
      m_column(column),
      m_parameterCountForJS(-1),
      m_target(target),
      m_index(index),
      m_expressionFunctionValid(false),
      m_expressionFunctionRewritten(isRewritten),
      m_invalidParameterName(false)
{
    init(ctxt, scope);
    if (isRewritten)
        m_expressionUtf8 = expression;
    else
        m_expression = QString::fromUtf8(expression);
}

QQmlBoundSignalExpression::QQmlBoundSignalExpression(QObject *target, int index,
                                                     QQmlContextData *ctxt, QObject *scope, const QString &expression,
                                                     bool isRewritten, const QString &fileName, quint16 line, quint16 column)
    : QQmlJavaScriptExpression(&QQmlBoundSignalExpression_jsvtable),
      m_fileName(fileName),
      m_line(line),
      m_column(column),
      m_parameterCountForJS(-1),
      m_target(target),
      m_index(index),
      m_expressionFunctionValid(false),
      m_expressionFunctionRewritten(isRewritten),
      m_invalidParameterName(false)
{
    init(ctxt, scope);
    if (isRewritten)
        m_expressionUtf8 = expression.toUtf8();
    else
        m_expression = expression;
}

void QQmlBoundSignalExpression::init(QQmlContextData *ctxt, QObject *scope)
{
    setNotifyOnValueChanged(false);
    setContext(ctxt);
    setScopeObject(scope);

    Q_ASSERT(m_target && m_index > -1);
    m_index = QQmlPropertyCache::originalClone(m_target, m_index);
}

QQmlBoundSignalExpression::~QQmlBoundSignalExpression()
{
    qPersistentDispose(m_v8function);
    qPersistentDispose(m_v8qmlscope);
}

QString QQmlBoundSignalExpression::expressionIdentifier(QQmlJavaScriptExpression *e)
{
    QQmlBoundSignalExpression *This = static_cast<QQmlBoundSignalExpression *>(e);
    return This->sourceFile() + QLatin1Char(':') + QString::number(This->lineNumber());
}

void QQmlBoundSignalExpression::expressionChanged(QQmlJavaScriptExpression *)
{
    // bound signals do not notify on change.
}

QString QQmlBoundSignalExpression::expression() const
{
    if (m_expressionFunctionValid) {
        Q_ASSERT (context() && engine());
        v8::HandleScope handle_scope;
        v8::Context::Scope context_scope(QQmlEnginePrivate::get(engine())->v8engine()->context());
        return QV8Engine::toStringStatic(m_v8function->ToString());
    } else if (!m_expressionUtf8.isEmpty()) {
        return QString::fromUtf8(m_expressionUtf8);
    } else {
        return m_expression;
    }
}

// Parts of this function mirror code in QQmlExpressionPrivate::value() and v8value().
// Changes made here may need to be made there and vice versa.
void QQmlBoundSignalExpression::evaluate(void **a)
{
    Q_ASSERT (context() && engine());

    if (m_invalidParameterName)
        return;

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine());

    ep->referenceScarceResources(); // "hold" scarce resources in memory during evaluation.
    {
        v8::HandleScope handle_scope;
        v8::Context::Scope context_scope(ep->v8engine()->context());
        if (!m_expressionFunctionValid) {

            //TODO: look at using the property cache here (as in the compiler)
            //      for further optimization
            QMetaMethod signal = QMetaObjectPrivate::signal(m_target->metaObject(), m_index);
            QQmlRewrite::RewriteSignalHandler rewriter;

            QString expression;
            bool ok = true;

            if (m_expressionFunctionRewritten) {
                expression = QString::fromUtf8(m_expressionUtf8);

                //if we need parameters, and the rewrite doesn't include them,
                //create and insert the parameter string now
                if (m_parameterCountForJS == -1 && signal.parameterCount()) {
                    const QString &parameters = rewriter.createParameterString(signal.parameterNames(),
                                                                               ep->v8engine()->illegalNames());
                    int index = expression.indexOf(QLatin1Char('('), 1);
                    Q_ASSERT(index > -1);
                    expression.insert(index + 1, parameters);

                    setParameterCountForJS(rewriter.parameterCountForJS());
                }

                m_expressionUtf8.clear();
            } else {
                //expression is still in its original form, so perform a full rewrite
                expression = rewriter(m_expression, QString()/*no name hint available*/, &ok,
                                      signal.parameterNames(),
                                      ep->v8engine()->illegalNames());
                setParameterCountForJS(rewriter.parameterCountForJS());
                m_expression.clear();
            }

            if (rewriter.hasParameterError()) {
                qmlInfo(scopeObject()) << rewriter.parameterError();
                m_invalidParameterName = true;
                ep->dereferenceScarceResources();
                return;
            }

            if (ok) {
                m_v8function = evalFunction(context(), scopeObject(), expression,
                                            m_fileName, m_line, &m_v8qmlscope);
            }

            if (m_v8function.IsEmpty() || m_v8function->IsNull()) {
                ep->dereferenceScarceResources();
                return; // could not evaluate function.  Not valid.
            }

            setUseSharedContext(false);
            m_expressionFunctionValid = true;
        }

        if (!hasParameterInfo()) {
            QQmlJavaScriptExpression::evaluate(context(), m_v8function, 0);
        } else {
            QV8Engine *engine = ep->v8engine();
            QVarLengthArray<int, 9> dummy;
            //TODO: lookup via signal index rather than method index as an optimization
            int methodIndex = QMetaObjectPrivate::signal(m_target->metaObject(), m_index).methodIndex();
            int *argsTypes = QQmlPropertyCache::methodParameterTypes(m_target, methodIndex, dummy, 0);
            int argCount = argsTypes ? m_parameterCountForJS : 0;

            QVarLengthArray<v8::Handle<v8::Value>, 9> args(argCount);

            for (int ii = 0; ii < argCount; ++ii) {
                int type = argsTypes[ii + 1];
                //### ideally we would use metaTypeToJS, however it currently gives different results
                //    for several cases (such as QVariant type and QObject-derived types)
                //args[ii] = engine->metaTypeToJS(type, a[ii + 1]);
                if (type == QMetaType::QVariant) {
                    args[ii] = engine->fromVariant(*((QVariant *)a[ii + 1]));
                } else if (type == QMetaType::Int) {
                    //### optimization. Can go away if we switch to metaTypeToJS, or be expanded otherwise
                    args[ii] = v8::Integer::New(*reinterpret_cast<const int*>(a[ii + 1]));
                } else if (type == qMetaTypeId<QQmlV8Handle>()) {
                    args[ii] = reinterpret_cast<QQmlV8Handle *>(a[ii + 1])->toHandle();
                } else if (ep->isQObject(type)) {
                    if (!*reinterpret_cast<void* const *>(a[ii + 1]))
                        args[ii] = v8::Null();
                    else
                        args[ii] = engine->newQObject(*reinterpret_cast<QObject* const *>(a[ii + 1]));
                } else {
                    args[ii] = engine->fromVariant(QVariant(type, a[ii + 1]));
                }
            }

            QQmlJavaScriptExpression::evaluate(context(), m_v8function, argCount, args.data(), 0);
        }
    }
    ep->dereferenceScarceResources(); // "release" scarce resources if top-level expression evaluation is complete.
}

////////////////////////////////////////////////////////////////////////

QQmlAbstractBoundSignal::QQmlAbstractBoundSignal()
: m_prevSignal(0), m_nextSignal(0)
{
}

QQmlAbstractBoundSignal::~QQmlAbstractBoundSignal()
{
    removeFromObject();
}

void QQmlAbstractBoundSignal::addToObject(QObject *obj)
{
    Q_ASSERT(!m_prevSignal);
    Q_ASSERT(obj);

    QQmlData *data = QQmlData::get(obj, true);

    m_nextSignal = data->signalHandlers;
    if (m_nextSignal) m_nextSignal->m_prevSignal = &m_nextSignal;
    m_prevSignal = &data->signalHandlers;
    data->signalHandlers = this;
}

void QQmlAbstractBoundSignal::removeFromObject()
{
    if (m_prevSignal) {
        *m_prevSignal = m_nextSignal;
        if (m_nextSignal) m_nextSignal->m_prevSignal = m_prevSignal;
        m_prevSignal = 0;
        m_nextSignal = 0;
    }
}

/*! \internal
    \a signal MUST be in the signal index range (see QObjectPrivate::signalIndex()).
    This is different from QMetaMethod::methodIndex().
*/
QQmlBoundSignal::QQmlBoundSignal(QObject *target, int signal, QObject *owner,
                                 QQmlEngine *engine)
: m_expression(0), m_index(signal), m_isEvaluating(false)
{
    addToObject(owner);
    setCallback(QQmlNotifierEndpoint::QQmlBoundSignal);

    /*
        If this is a cloned method, connect to the 'original'. For example,
        for the signal 'void aSignal(int parameter = 0)', if the method
        index refers to 'aSignal()', get the index of 'aSignal(int)'.
        This ensures that 'parameter' will be available from QML.
    */
    m_index = QQmlPropertyCache::originalClone(target, m_index);
    QQmlNotifierEndpoint::connect(target, m_index, engine);
}

QQmlBoundSignal::~QQmlBoundSignal()
{
    m_expression = 0;
}

/*!
    Returns the signal index in the range returned by QObjectPrivate::signalIndex().
    This is different from QMetaMethod::methodIndex().
*/
int QQmlBoundSignal::index() const
{
    return m_index;
}

/*!
    Returns the signal expression.
*/
QQmlBoundSignalExpression *QQmlBoundSignal::expression() const
{
    return m_expression;
}

/*!
    Sets the signal expression to \a e.  Returns the current signal expression,
    or null if there is no signal expression.

    The QQmlBoundSignal instance adds a reference to \a e.  The caller
    assumes ownership of the returned QQmlBoundSignalExpression reference.
*/
QQmlBoundSignalExpressionPointer QQmlBoundSignal::setExpression(QQmlBoundSignalExpression *e)
{
    QQmlBoundSignalExpressionPointer rv = m_expression;
    m_expression = e;
    if (m_expression) m_expression->setNotifyOnValueChanged(false);
    return rv;
}

/*!
    Sets the signal expression to \a e.  Returns the current signal expression,
    or null if there is no signal expression.

    The QQmlBoundSignal instance takes ownership of \a e (and does not add a reference).  The caller
    assumes ownership of the returned QQmlBoundSignalExpression reference.
*/
QQmlBoundSignalExpressionPointer QQmlBoundSignal::takeExpression(QQmlBoundSignalExpression *e)
{
    QQmlBoundSignalExpressionPointer rv = m_expression;
    m_expression.take(e);
    if (m_expression) m_expression->setNotifyOnValueChanged(false);
    return rv;
}

void QQmlBoundSignal_callback(QQmlNotifierEndpoint *e, void **a)
{
    QQmlBoundSignal *s = static_cast<QQmlBoundSignal*>(e);
    if (!s->m_expression)
        return;

    if (QQmlDebugService::isDebuggingEnabled())
        QV8DebugService::instance()->signalEmitted(QString::fromLatin1(QMetaObjectPrivate::signal(s->m_expression->target()->metaObject(), s->m_index).methodSignature()));

    QQmlHandlingSignalProfiler prof(s->m_expression);

    s->m_isEvaluating = true;

    if (s->m_expression && s->m_expression->engine()) {
        s->m_expression->evaluate(a);
        if (s->m_expression && s->m_expression->hasError()) {
            QQmlEngine *engine = s->m_expression->engine();
            QQmlEnginePrivate::warning(engine, s->m_expression->error(engine));
        }
    }

    s->m_isEvaluating = false;
}

////////////////////////////////////////////////////////////////////////

QQmlBoundSignalExpressionPointer::QQmlBoundSignalExpressionPointer(QQmlBoundSignalExpression *o)
: o(o)
{
    if (o) o->addref();
}

QQmlBoundSignalExpressionPointer::QQmlBoundSignalExpressionPointer(const QQmlBoundSignalExpressionPointer &other)
: o(other.o)
{
    if (o) o->addref();
}

QQmlBoundSignalExpressionPointer::~QQmlBoundSignalExpressionPointer()
{
    if (o) o->release();
}

QQmlBoundSignalExpressionPointer &QQmlBoundSignalExpressionPointer::operator=(const QQmlBoundSignalExpressionPointer &other)
{
    if (other.o) other.o->addref();
    if (o) o->release();
    o = other.o;
    return *this;
}

QQmlBoundSignalExpressionPointer &QQmlBoundSignalExpressionPointer::operator=(QQmlBoundSignalExpression *other)
{
    if (other) other->addref();
    if (o) o->release();
    o = other;
    return *this;
}

/*!
Takes ownership of \a other.  take() does *not* add a reference, as it assumes ownership
of the callers reference of other.
*/
QQmlBoundSignalExpressionPointer &QQmlBoundSignalExpressionPointer::take(QQmlBoundSignalExpression *other)
{
    if (o) o->release();
    o = other;
    return *this;
}

QT_END_NAMESPACE
