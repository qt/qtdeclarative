/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <private/qqmlprofiler_p.h>
#include <private/qqmlexpression_p.h>
#include <private/qqmlscriptstring_p.h>
#include <private/qqmlcontextwrapper_p.h>
#include <private/qqmlbuiltinfunctions_p.h>
#include <private/qqmlvmemetaobject_p.h>

#include <QVariant>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

QQmlBinding::Identifier QQmlBinding::Invalid = -1;

QQmlBinding::QQmlBinding(const QString &str, QObject *obj, QQmlContext *ctxt)
    : QQmlJavaScriptExpression(),
      QQmlAbstractBinding(Binding)
{
    setNotifyOnValueChanged(true);
    QQmlJavaScriptExpression::setContext(QQmlContextData::get(ctxt));
    setScopeObject(obj);

    QV4::ExecutionEngine *v4 = QQmlEnginePrivate::get(context()->engine)->v4engine();
    m_function.set(v4, qmlBinding(context(), obj, str, QString(), 0));
}

QQmlBinding::QQmlBinding(const QQmlScriptString &script, QObject *obj, QQmlContext *ctxt)
    : QQmlJavaScriptExpression(),
      QQmlAbstractBinding(Binding)
{
    if (ctxt && !ctxt->isValid())
        return;

    const QQmlScriptStringPrivate *scriptPrivate = script.d.data();
    if (!ctxt && (!scriptPrivate->context || !scriptPrivate->context->isValid()))
        return;

    QString url;
    QV4::Function *runtimeFunction = 0;

    QQmlContextData *ctxtdata = QQmlContextData::get(scriptPrivate->context);
    QQmlEnginePrivate *engine = QQmlEnginePrivate::get(scriptPrivate->context->engine());
    if (engine && ctxtdata && !ctxtdata->urlString().isEmpty() && ctxtdata->typeCompilationUnit) {
        url = ctxtdata->urlString();
        if (scriptPrivate->bindingId != QQmlBinding::Invalid)
            runtimeFunction = ctxtdata->typeCompilationUnit->runtimeFunctions.at(scriptPrivate->bindingId);
    }

    setNotifyOnValueChanged(true);
    QQmlJavaScriptExpression::setContext(QQmlContextData::get(ctxt ? ctxt : scriptPrivate->context));
    setScopeObject(obj ? obj : scriptPrivate->scope);

    QV4::ExecutionEngine *v4 = QQmlEnginePrivate::get(context()->engine)->v4engine();
    if (runtimeFunction) {
        m_function.set(v4, QV4::QmlBindingWrapper::createQmlCallableForFunction(ctxtdata, scopeObject(), runtimeFunction));
    } else {
        QString code = scriptPrivate->script;
        m_function.set(v4, qmlBinding(context(), scopeObject(), code, url, scriptPrivate->lineNumber));
    }
}

QQmlBinding::QQmlBinding(const QString &str, QObject *obj, QQmlContextData *ctxt)
    : QQmlJavaScriptExpression(),
      QQmlAbstractBinding(Binding)
{
    setNotifyOnValueChanged(true);
    QQmlJavaScriptExpression::setContext(ctxt);
    setScopeObject(obj);

    QV4::ExecutionEngine *v4 = QQmlEnginePrivate::get(context()->engine)->v4engine();
    m_function.set(v4, qmlBinding(ctxt, obj, str, QString(), 0));
}

QQmlBinding::QQmlBinding(const QString &str, QObject *obj,
                         QQmlContextData *ctxt,
                         const QString &url, quint16 lineNumber, quint16 columnNumber)
    : QQmlJavaScriptExpression(),
      QQmlAbstractBinding(Binding)
{
    Q_UNUSED(columnNumber);
    setNotifyOnValueChanged(true);
    QQmlJavaScriptExpression::setContext(ctxt);
    setScopeObject(obj);

    QV4::ExecutionEngine *v4 = QQmlEnginePrivate::get(context()->engine)->v4engine();
    m_function.set(v4, qmlBinding(ctxt, obj, str, url, lineNumber));
}

QQmlBinding::QQmlBinding(const QV4::Value &functionPtr, QObject *obj, QQmlContextData *ctxt)
    : QQmlJavaScriptExpression(),
      QQmlAbstractBinding(Binding)
{
    setNotifyOnValueChanged(true);
    QQmlJavaScriptExpression::setContext(ctxt);
    setScopeObject(obj);

    m_function.set(functionPtr.as<QV4::Object>()->engine(), functionPtr);
}

QQmlBinding::~QQmlBinding()
{
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
    if (QQmlData::wasDeleted(targetObject()))
        return;

    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context()->engine);
    QV4::Scope scope(ep->v4engine());
    QV4::ScopedFunctionObject f(scope, m_function.value());
    Q_ASSERT(f);

    if (updatingFlag()) {
        QQmlProperty p = QQmlPropertyPrivate::restore(targetObject(), getPropertyData(), 0);
        QQmlAbstractBinding::printBindingLoopError(p);
        return;
    }

    QQmlBindingProfiler prof(ep->profiler, f);
    setUpdatingFlag(true);

    QQmlJavaScriptExpression::DeleteWatcher watcher(this);

    QQmlPropertyData pd = getPropertyData();

    if (pd.propType == qMetaTypeId<QQmlBinding *>()) {

        int idx = pd.coreIndex;
        Q_ASSERT(idx != -1);

        QQmlBinding *t = this;
        int status = -1;
        void *a[] = { &t, 0, &status, &flags };
        QMetaObject::metacall(*m_target, QMetaObject::WriteProperty, idx, a);

    } else {
        ep->referenceScarceResources();

        bool isUndefined = false;

        QV4::ScopedValue result(scope, QQmlJavaScriptExpression::evaluate(&isUndefined));

        bool needsErrorLocationData = false;
        if (!watcher.wasDeleted() && !hasError())
            needsErrorLocationData = !QQmlPropertyPrivate::writeBinding(*m_target, pd, context(),
                                                                this, result, isUndefined, flags);

        if (!watcher.wasDeleted()) {

            if (needsErrorLocationData)
                delayedError()->setErrorLocation(f->sourceLocation());

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
}

QVariant QQmlBinding::evaluate()
{
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context()->engine);
    ep->referenceScarceResources();

    bool isUndefined = false;

    QV4::Scope scope(ep->v4engine());
    QV4::ScopedValue result(scope, QQmlJavaScriptExpression::evaluate(&isUndefined));

    ep->dereferenceScarceResources();

    return scope.engine->toVariant(result, qMetaTypeId<QList<QObject*> >());
}

QString QQmlBinding::expressionIdentifier()
{
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(context()->engine);
    QV4::Scope scope(ep->v4engine());
    QV4::ScopedValue f(scope, m_function.value());
    QV4::Function *function = f->as<QV4::FunctionObject>()->function();

    QString url = function->sourceFile();
    quint16 lineNumber = function->compiledFunction->location.line;
    quint16 columnNumber = function->compiledFunction->location.column;

    return url + QLatin1Char(':') + QString::number(lineNumber) + QLatin1Char(':') + QString::number(columnNumber);
}

void QQmlBinding::expressionChanged()
{
    update();
}

void QQmlBinding::refresh()
{
    update();
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
    QV4::Scope scope(QQmlEnginePrivate::get(context()->engine)->v4engine());
    QV4::ScopedValue v(scope, m_function.value());
    return v->toQStringNoThrow();
}

QObject *QQmlBinding::targetObject() const
{
    return *m_target;
}

int QQmlBinding::targetPropertyIndex() const
{
    return m_index;
}

void QQmlBinding::setTarget(const QQmlProperty &prop)
{
    setTarget(prop.object(), QQmlPropertyPrivate::get(prop)->core);
}

void QQmlBinding::setTarget(QObject *object, const QQmlPropertyData &core)
{
    m_target = object;
    QQmlPropertyData pd = core;

    if (!object) {
        m_index = -1;
        return;
    }

    while (pd.isAlias()) {
        int coreIndex = core.coreIndex;
        int valueTypeIndex = core.getValueTypeCoreIndex();
        QQmlVMEMetaObject *vme = QQmlVMEMetaObject::getForProperty(object, coreIndex);

        int aValueTypeIndex;
        if (!vme->aliasTarget(coreIndex, &object, &coreIndex, &aValueTypeIndex)) {
            m_target = 0;
            m_index = -1;
            return;
        }
        if (valueTypeIndex == -1)
            valueTypeIndex = aValueTypeIndex;

        QQmlData *data = QQmlData::get(object, false);
        if (!data || !data->propertyCache) {
            m_target = 0;
            m_index = -1;
            return;
        }
        QQmlPropertyData *propertyData = data->propertyCache->property(coreIndex);
        Q_ASSERT(propertyData);

        m_target = object;
        pd = *propertyData;
        if (valueTypeIndex != -1) {
            const QMetaObject *valueTypeMetaObject = QQmlValueTypeFactory::metaObjectForMetaType(pd.propType);
            Q_ASSERT(valueTypeMetaObject);
            QMetaProperty vtProp = valueTypeMetaObject->property(valueTypeIndex);
            pd.setFlags(pd.getFlags() | QQmlPropertyData::IsValueTypeVirtual);
            pd.valueTypeFlags = QQmlPropertyData::flagsForProperty(vtProp);
            pd.valueTypePropType = vtProp.userType();
            pd.valueTypeCoreIndex = valueTypeIndex;
        }
    }
    m_index = pd.encodedIndex();

    QQmlData *data = QQmlData::get(*m_target, true);
    if (!data->propertyCache) {
        data->propertyCache = QQmlEnginePrivate::get(context()->engine)->cache(m_target->metaObject());
        data->propertyCache->addref();
    }
}

QQmlPropertyData QQmlBinding::getPropertyData() const
{
    int coreIndex;
    int valueTypeIndex = QQmlPropertyData::decodeValueTypePropertyIndex(m_index, &coreIndex);

    QQmlData *data = QQmlData::get(*m_target, false);
    Q_ASSERT(data && data->propertyCache);

    QQmlPropertyData *propertyData = data->propertyCache->property(coreIndex);
    Q_ASSERT(propertyData);

    QQmlPropertyData d = *propertyData;
    if (valueTypeIndex != -1) {
        const QMetaObject *valueTypeMetaObject = QQmlValueTypeFactory::metaObjectForMetaType(d.propType);
        Q_ASSERT(valueTypeMetaObject);
        QMetaProperty vtProp = valueTypeMetaObject->property(valueTypeIndex);
        d.setFlags(d.getFlags() | QQmlPropertyData::IsValueTypeVirtual);
        d.valueTypeFlags = QQmlPropertyData::flagsForProperty(vtProp);
        d.valueTypePropType = vtProp.userType();
        d.valueTypeCoreIndex = valueTypeIndex;
    }
    return d;
}

QT_END_NAMESPACE
