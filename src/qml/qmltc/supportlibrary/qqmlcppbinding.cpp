// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlcppbinding_p.h"

#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlcontext.h>
#include <QtCore/qmetaobject.h>

#include <private/qqmltypedata_p.h>
#include <private/qqmlpropertybinding_p.h>
#include <private/qqmlbinding_p.h>
#include <private/qv4qmlcontext_p.h>
#include <private/qqmlproperty_p.h>
#include <private/qqmlbinding_p.h>

QT_BEGIN_NAMESPACE

template<typename CreateBinding>
inline decltype(auto) createBindingInScope(QObject *thisObject, CreateBinding create)
{
    QQmlEngine *qmlengine = qmlEngine(thisObject);
    Q_ASSERT(qmlengine);
    QV4::ExecutionEngine *v4 = qmlengine->handle();
    Q_ASSERT(v4);

    QQmlData *ddata = QQmlData::get(thisObject);
    Q_ASSERT(ddata && ddata->outerContext);
    QQmlRefPointer<QQmlContextData> ctxtdata = QQmlRefPointer<QQmlContextData>(ddata->outerContext);

    QV4::Scope scope(v4);
    QV4::ExecutionContext *executionCtx = v4->scriptContext();
    QV4::Scoped<QV4::QmlContext> qmlContext(
            scope, QV4::QmlContext::create(executionCtx, ctxtdata, thisObject));

    return create(ctxtdata, qmlContext);
}

QUntypedPropertyBinding
QQmlCppBinding::createBindingForBindable(const QV4::ExecutableCompilationUnit *unit,
                                         QObject *thisObject, qsizetype functionIndex,
                                         QObject *bindingTarget, int metaPropertyIndex,
                                         int valueTypePropertyIndex, const QString &propertyName)
{
    Q_UNUSED(propertyName);

    QV4::Function *v4Function = unit->runtimeFunctions.value(functionIndex, nullptr);
    if (!v4Function) {
        // TODO: align with existing logging of such
        qCritical() << "invalid JavaScript function index (internal error)";
        return QUntypedPropertyBinding();
    }
    if (metaPropertyIndex < 0) {
        // TODO: align with existing logging of such
        qCritical() << "invalid meta property index (internal error)";
        return QUntypedPropertyBinding();
    }

    const QMetaObject *mo = bindingTarget->metaObject();
    Q_ASSERT(mo);
    QMetaProperty property = mo->property(metaPropertyIndex);
    Q_ASSERT(valueTypePropertyIndex == -1 || QString::fromUtf8(property.name()) == propertyName);

    return createBindingInScope(
            thisObject,
            [&](const QQmlRefPointer<QQmlContextData> &ctxt, QV4::ExecutionContext *scope) {
                auto index = QQmlPropertyIndex(property.propertyIndex(), valueTypePropertyIndex);
                return QQmlPropertyBinding::create(property.metaType(), v4Function, thisObject,
                                                   ctxt, scope, bindingTarget, index);
            });
}

void QQmlCppBinding::createBindingForNonBindable(const QV4::ExecutableCompilationUnit *unit,
                                                 QObject *thisObject, qsizetype functionIndex,
                                                 QObject *bindingTarget, int metaPropertyIndex,
                                                 int valueTypePropertyIndex,
                                                 const QString &propertyName)
{
    Q_UNUSED(propertyName);

    QV4::Function *v4Function = unit->runtimeFunctions.value(functionIndex, nullptr);
    if (!v4Function) {
        // TODO: align with existing logging of such
        qCritical() << "invalid JavaScript function index (internal error)";
        return;
    }
    if (metaPropertyIndex < 0) {
        // TODO: align with existing logging of such
        qCritical() << "invalid meta property index (internal error)";
        return;
    }

    const QMetaObject *mo = bindingTarget->metaObject();
    Q_ASSERT(mo);
    QMetaProperty property = mo->property(metaPropertyIndex);
    Q_ASSERT(valueTypePropertyIndex != -1 || QString::fromUtf8(property.name()) == propertyName);

    createBindingInScope(
            thisObject,
            [&](const QQmlRefPointer<QQmlContextData> &ctxt, QV4::ExecutionContext *scope) -> void {
                QQmlBinding *binding = QQmlBinding::create(property.metaType(), v4Function,
                                                           thisObject, ctxt, scope);
                // almost as in qv4objectwrapper.cpp:535
                Q_ASSERT(!property.isAlias()); // we convert aliases to (almost) real properties
                binding->setTarget(bindingTarget, property.propertyIndex(), false,
                                   valueTypePropertyIndex);
                QQmlPropertyPrivate::setBinding(binding);
            });
}

QUntypedPropertyBinding QQmlCppBinding::createTranslationBindingForBindable(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit, QObject *bindingTarget,
        int metaPropertyIndex, const QQmlTranslation &translationData, const QString &propertyName)
{
    Q_UNUSED(propertyName);

    if (metaPropertyIndex < 0) {
        // TODO: align with existing logging of such
        qCritical() << "invalid meta property index (internal error)";
        return QUntypedPropertyBinding();
    }

    const QMetaObject *mo = bindingTarget->metaObject();
    Q_ASSERT(mo);
    QMetaProperty property = mo->property(metaPropertyIndex);
    Q_ASSERT(QString::fromUtf8(property.name()) == propertyName);

    return QQmlTranslationPropertyBinding::create(property.metaType(), unit, translationData);
}

void QQmlCppBinding::createTranslationBindingForNonBindable(
        const QQmlRefPointer<QV4::ExecutableCompilationUnit> &unit,
        const QQmlSourceLocation &location, const QQmlTranslation &translationData,
        QObject *thisObject, QObject *bindingTarget, int metaPropertyIndex,
        const QString &propertyName, int valueTypePropertyIndex)
{
    Q_UNUSED(propertyName);

    if (metaPropertyIndex < 0) {
        // TODO: align with existing logging of such
        qCritical() << "invalid meta property index (internal error)";
        return;
    }

    const QMetaObject *mo = bindingTarget->metaObject();
    Q_ASSERT(mo);
    QMetaProperty property = mo->property(metaPropertyIndex);
    Q_ASSERT(QString::fromUtf8(property.name()) == propertyName);

    createBindingInScope(
            thisObject,
            [&](const QQmlRefPointer<QQmlContextData> &ctxt, QV4::ExecutionContext *) -> void {
                QQmlBinding *binding = QQmlBinding::createTranslationBinding(
                        unit, ctxt, propertyName, translationData, location, thisObject);
                // almost as in qv4objectwrapper.cpp:535
                Q_ASSERT(!property.isAlias()); // we convert aliases to (almost) real properties
                binding->setTarget(bindingTarget, property.propertyIndex(), false,
                                   valueTypePropertyIndex);
                QQmlPropertyPrivate::setBinding(binding);
            });
}

QT_END_NAMESPACE
