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

#include "qqmlbinding_p.h"

#include "qqml.h"
#include "qqmlcontext.h"
#include "qqmlinfo.h"
#include "qqmlcompiler_p.h"
#include "qqmldata_p.h"
#include <private/qqmlprofilerservice_p.h>
#include <private/qqmltrace_p.h>
#include <private/qqmlexpression_p.h>
#include <private/qqmlrewrite_p.h>
#include <private/qqmlscriptstring_p.h>

#include <QVariant>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

// Used in qqmlabstractbinding.cpp
QQmlAbstractBinding::VTable QQmlBinding_vtable = {
    QQmlAbstractBinding::default_destroy<QQmlBinding>,
    QQmlBinding::expression,
    QQmlBinding::propertyIndex,
    QQmlBinding::object,
    QQmlBinding::setEnabled,
    QQmlBinding::update,
    QQmlBinding::retargetBinding
};

QQmlBinding::Identifier QQmlBinding::Invalid = -1;

QQmlBinding *
QQmlBinding::createBinding(Identifier id, QObject *obj, QQmlContext *ctxt,
                                   const QString &url, quint16 lineNumber)
{
    if (id < 0)
        return 0;

    QQmlBinding *rv = 0;

    QQmlContextData *ctxtdata = QQmlContextData::get(ctxt);
    QQmlEnginePrivate *engine = QQmlEnginePrivate::get(ctxt->engine());
    if (engine && ctxtdata && !ctxtdata->url.isEmpty()) {
        QQmlTypeData *typeData = engine->typeLoader.getType(ctxtdata->url);
        Q_ASSERT(typeData);

        if (QQmlCompiledData *cdata = typeData->compiledData()) {
            rv = new QQmlBinding(cdata->primitives.at(id), true, obj, ctxtdata, url, lineNumber, 0);
        }

        typeData->release();
    }

    return rv;
}

static QQmlJavaScriptExpression::VTable QQmlBinding_jsvtable = {
    QQmlBinding::expressionIdentifier,
    QQmlBinding::expressionChanged
};

QQmlBinding::QQmlBinding(const QString &str, QObject *obj, QQmlContext *ctxt)
: QQmlJavaScriptExpression(&QQmlBinding_jsvtable), QQmlAbstractBinding(Binding),
  m_lineNumber(0), m_columnNumber(0)
{
    setNotifyOnValueChanged(true);
    QQmlAbstractExpression::setContext(QQmlContextData::get(ctxt));
    setScopeObject(obj);

    QQmlRewrite::RewriteBinding rewriteBinding;
    QString code = rewriteBinding(str);

    m_expression = str.toUtf8();
    v8function = evalFunction(context(), obj, code, QString(), 0);
}

QQmlBinding::QQmlBinding(const QQmlScriptString &script, QObject *obj, QQmlContext *ctxt)
: QQmlJavaScriptExpression(&QQmlBinding_jsvtable), QQmlAbstractBinding(Binding)
{
    if (ctxt && !ctxt->isValid())
        return;

    const QQmlScriptStringPrivate *scriptPrivate = script.d.data();
    if (!ctxt && (!scriptPrivate->context || !scriptPrivate->context->isValid()))
        return;

    bool needRewrite = true;
    QString code;

    int id = scriptPrivate->bindingId;
    if (id >= 0) {
        QQmlContextData *ctxtdata = QQmlContextData::get(scriptPrivate->context);
        QQmlEnginePrivate *engine = QQmlEnginePrivate::get(scriptPrivate->context->engine());
        if (engine && ctxtdata && !ctxtdata->url.isEmpty()) {
            QQmlTypeData *typeData = engine->typeLoader.getType(ctxtdata->url);
            Q_ASSERT(typeData);

            if (QQmlCompiledData *cdata = typeData->compiledData()) {
                needRewrite = false;
                code = cdata->primitives.at(id);
                m_url = cdata->name;
            }

            typeData->release();
        }
    }

    if (needRewrite) {
        QQmlRewrite::RewriteBinding rewriteBinding;
        code = rewriteBinding(scriptPrivate->script);
    }

    setNotifyOnValueChanged(true);
    QQmlAbstractExpression::setContext(QQmlContextData::get(ctxt ? ctxt : scriptPrivate->context));
    setScopeObject(obj ? obj : scriptPrivate->scope);

    m_expression = scriptPrivate->script.toUtf8();
    m_lineNumber = scriptPrivate->lineNumber;
    m_columnNumber = scriptPrivate->columnNumber;

    v8function = evalFunction(context(), scopeObject(), code, QString(), m_lineNumber);
}

QQmlBinding::QQmlBinding(const QString &str, QObject *obj, QQmlContextData *ctxt)
: QQmlJavaScriptExpression(&QQmlBinding_jsvtable), QQmlAbstractBinding(Binding),
  m_lineNumber(0), m_columnNumber(0)
{
    setNotifyOnValueChanged(true);
    QQmlAbstractExpression::setContext(ctxt);
    setScopeObject(obj);

    QQmlRewrite::RewriteBinding rewriteBinding;
    QString code = rewriteBinding(str);

    m_expression = str.toUtf8();
    v8function = evalFunction(ctxt, obj, code, QString(), 0);
}

QQmlBinding::QQmlBinding(const QString &str, bool isRewritten, QObject *obj,
                         QQmlContextData *ctxt,
                         const QString &url, quint16 lineNumber, quint16 columnNumber)
: QQmlJavaScriptExpression(&QQmlBinding_jsvtable), QQmlAbstractBinding(Binding),
  m_url(url), m_lineNumber(lineNumber), m_columnNumber(columnNumber)
{
    setNotifyOnValueChanged(true);
    QQmlAbstractExpression::setContext(ctxt);
    setScopeObject(obj);

    QString code;
    if (isRewritten) {
        code = str;
    } else {
        QQmlRewrite::RewriteBinding rewriteBinding;
        code = rewriteBinding(str);
    }

    m_expression = str.toUtf8();

    v8function = evalFunction(ctxt, obj, code, url, m_lineNumber);
}

/*!
    \internal

    To avoid exposing v8 in the public API, functionPtr must be a pointer to a v8::Handle<v8::Function>.
    For example:
        v8::Handle<v8::Function> function;
        new QQmlBinding(&function, scope, ctxt);
 */
QQmlBinding::QQmlBinding(void *functionPtr, QObject *obj, QQmlContextData *ctxt,
                         const QString &url, quint16 lineNumber, quint16 columnNumber)
: QQmlJavaScriptExpression(&QQmlBinding_jsvtable), QQmlAbstractBinding(Binding),
  m_url(url), m_lineNumber(lineNumber), m_columnNumber(columnNumber)
{
    setNotifyOnValueChanged(true);
    QQmlAbstractExpression::setContext(ctxt);
    setScopeObject(obj);

    v8function = qPersistentNew<v8::Function>(*(v8::Handle<v8::Function> *)functionPtr);
}

QQmlBinding::~QQmlBinding()
{
    qPersistentDispose(v8function);
}

void QQmlBinding::setEvaluateFlags(EvaluateFlags flags)
{
    setRequiresThisObject(flags & RequiresThisObject);
}

QQmlBinding::EvaluateFlags QQmlBinding::evaluateFlags() const
{
    return requiresThisObject()?RequiresThisObject:None;
}

void QQmlBinding::setNotifyOnValueChanged(bool v)
{
    QQmlJavaScriptExpression::setNotifyOnValueChanged(v);
}

void QQmlBinding::update(QQmlPropertyPrivate::WriteFlags flags)
{
    if (!enabledFlag() || !context() || !context()->isValid())
        return;

    // Check that the target has not been deleted
    if (QQmlData::wasDeleted(object()))
        return;

    int lineNo = qmlSourceCoordinate(m_lineNumber);
    int columnNo = qmlSourceCoordinate(m_columnNumber);

    QQmlTrace trace("General Binding Update");
    trace.addDetail("URL", m_url);
    trace.addDetail("Line", lineNo);
    trace.addDetail("Column", columnNo);

    if (!updatingFlag()) {
        QQmlBindingProfiler prof(m_url, lineNo, columnNo, QQmlProfilerService::QmlBinding);
        setUpdatingFlag(true);

        QQmlAbstractExpression::DeleteWatcher watcher(this);

        if (m_core.propType == qMetaTypeId<QQmlBinding *>()) {

            int idx = m_core.coreIndex;
            Q_ASSERT(idx != -1);

            QQmlBinding *t = this;
            int status = -1;
            void *a[] = { &t, 0, &status, &flags };
            QMetaObject::metacall(*m_coreObject, QMetaObject::WriteProperty, idx, a);

        } else {
            QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context()->engine);
            ep->referenceScarceResources();

            bool isUndefined = false;

            v8::HandleScope handle_scope;
            v8::Context::Scope scope(ep->v8engine()->context());
            v8::Local<v8::Value> result =
                QQmlJavaScriptExpression::evaluate(context(), v8function, &isUndefined);

            trace.event("writing binding result");

            bool needsErrorLocationData = false;
            if (!watcher.wasDeleted() && !hasError())
                needsErrorLocationData = !QQmlPropertyPrivate::writeBinding(*m_coreObject, m_core, context(),
                                                                    this, result, isUndefined, flags);

            if (!watcher.wasDeleted()) {

                if (needsErrorLocationData)
                    delayedError()->setErrorLocation(QUrl(m_url), m_lineNumber, m_columnNumber);

                if (hasError()) {
                    if (!delayedError()->addError(ep)) ep->warning(this->error(context()->engine));
                } else {
                    clearError();
                }

            }

            ep->dereferenceScarceResources();
        }

        if (!watcher.wasDeleted())
            setUpdatingFlag(false);
    } else {
        QQmlProperty p = property();
        QQmlAbstractBinding::printBindingLoopError(p);
    }
}

QVariant QQmlBinding::evaluate()
{
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context()->engine);
    ep->referenceScarceResources();

    bool isUndefined = false;

    v8::HandleScope handle_scope;
    v8::Context::Scope scope(ep->v8engine()->context());
    v8::Local<v8::Value> result =
        QQmlJavaScriptExpression::evaluate(context(), v8function, &isUndefined);

    ep->dereferenceScarceResources();

    return ep->v8engine()->toVariant(result, qMetaTypeId<QList<QObject*> >());
}

QString QQmlBinding::expressionIdentifier(QQmlJavaScriptExpression *e)
{
    QQmlBinding *This = static_cast<QQmlBinding *>(e);

    return QLatin1Char('"') + QString::fromUtf8(This->m_expression) + QLatin1Char('"');
}

void QQmlBinding::expressionChanged(QQmlJavaScriptExpression *e)
{
    QQmlBinding *This = static_cast<QQmlBinding *>(e);
    This->update();
}

void QQmlBinding::refresh()
{
    update();
}

QString QQmlBinding::expression(const QQmlAbstractBinding *This)
{
    return static_cast<const QQmlBinding *>(This)->expression();
}

int QQmlBinding::propertyIndex(const QQmlAbstractBinding *This)
{
    return static_cast<const QQmlBinding *>(This)->propertyIndex();
}

QObject *QQmlBinding::object(const QQmlAbstractBinding *This)
{
    return static_cast<const QQmlBinding *>(This)->object();
}

void QQmlBinding::setEnabled(QQmlAbstractBinding *This, bool e, QQmlPropertyPrivate::WriteFlags f)
{
    static_cast<QQmlBinding *>(This)->setEnabled(e, f);
}

void QQmlBinding::update(QQmlAbstractBinding *This , QQmlPropertyPrivate::WriteFlags f)
{
    static_cast<QQmlBinding *>(This)->update(f);
}

void QQmlBinding::retargetBinding(QQmlAbstractBinding *This, QObject *o, int i)
{
    static_cast<QQmlBinding *>(This)->retargetBinding(o, i);
}

void QQmlBinding::setEnabled(bool e, QQmlPropertyPrivate::WriteFlags flags)
{
    setEnabledFlag(e);
    setNotifyOnValueChanged(e);

    if (e)
        update(flags);
}

QString QQmlBinding::expression() const
{
    return QString::fromUtf8(m_expression);
}

QObject *QQmlBinding::object() const
{
    if (m_coreObject.hasValue()) return m_coreObject.constValue()->target;
    else return *m_coreObject;
}

int QQmlBinding::propertyIndex() const
{
    if (m_coreObject.hasValue()) return m_coreObject.constValue()->targetProperty;
    else return m_core.encodedIndex();
}

void QQmlBinding::retargetBinding(QObject *t, int i)
{
    m_coreObject.value().target = t;
    m_coreObject.value().targetProperty = i;
}

void QQmlBinding::setTarget(const QQmlProperty &prop)
{
    setTarget(prop.object(), QQmlPropertyPrivate::get(prop)->core,
              QQmlPropertyPrivate::get(prop)->context);
}

void QQmlBinding::setTarget(QObject *object, const QQmlPropertyData &core, QQmlContextData *ctxt)
{
    m_coreObject = object;
    m_core = core;
    m_ctxt = ctxt;
}

QQmlProperty QQmlBinding::property() const
{
    return QQmlPropertyPrivate::restore(object(), m_core, *m_ctxt);
}

QT_END_NAMESPACE
