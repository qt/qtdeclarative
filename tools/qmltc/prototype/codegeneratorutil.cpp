/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "prototype/codegeneratorutil.h"
#include "prototype/qml2cpppropertyutils.h"

#include <QtCore/qstringbuilder.h>

#include <tuple>

// NB: this variable would behave correctly as long as QML init and QML finalize
// are non-virtual functions
const QQmlJSAotVariable CodeGeneratorUtility::childrenOffsetVariable = { u"qsizetype"_qs,
                                                                         u"QML_choffset"_qs,
                                                                         QString() };

const QQmlJSAotVariable CodeGeneratorUtility::compilationUnitVariable = {
    u"QV4::ExecutableCompilationUnit *"_qs, u"QML_cu"_qs, QString()
};

static std::tuple<QStringList, QString, QStringList>
wrapIfMismatchingType(const QQmlJSMetaProperty &p, QString value)
{
    auto isDerivedFromBuiltin = [](QQmlJSScope::ConstPtr t, const QString &builtin) {
        for (; t; t = t->baseType()) {
            if (t->internalName() == builtin)
                return true;
        }
        return false;
    };
    QStringList prologue;
    QStringList epilogue;
    auto propType = p.type();
    if (isDerivedFromBuiltin(propType, u"QVariant"_qs)) {
        const QString variantName = u"var_" + p.propertyName();
        prologue << u"{ // accepts QVariant"_qs;
        prologue << u"QVariant " + variantName + u";";
        prologue << variantName + u".setValue(" + value + u");";
        epilogue << u"}"_qs;
        value = u"std::move(" + variantName + u")";
    } else if (isDerivedFromBuiltin(propType, u"QJSValue"_qs)) {
        const QString jsvalueName = u"jsvalue_" + p.propertyName();
        prologue << u"{ // accepts QJSValue"_qs;
        // Note: do not assume we have the engine, acquire it from `this`
        prologue << u"auto e = qmlEngine(this);"_qs;
        prologue << u"QJSValue " + jsvalueName + u" = e->toScriptValue(" + value + u");";
        epilogue << u"}"_qs;
        value = u"std::move(" + jsvalueName + u")";
    }
    return { prologue, value, epilogue };
}

// TODO: when assigning to a property, if property.type() != value.type(), have
// to wrap it (actually, wrap unconditionally if property.type() is QVariant or
// QQmlComponent -- other cases out of scope for now)
QStringList CodeGeneratorUtility::generate_assignToProperty(
        const QQmlJSScope::ConstPtr &type, const QString &propertyName, const QQmlJSMetaProperty &p,
        const QString &value, const QString &accessor, bool constructQVariant)
{
    Q_ASSERT(p.isValid());
    Q_ASSERT(!p.isList()); // NB: this code does not handle list properties

    QStringList code;
    code.reserve(6); // should always be enough

    if (type->hasOwnProperty(propertyName)) {
        Q_ASSERT(!p.isPrivate());
        // this object is compiled, so just assignment should work fine
        code << accessor + u"->m_" + propertyName + u" = " + value + u";";
    } else if (QString propertySetter = p.write(); !propertySetter.isEmpty()) {
        // there's a WRITE function
        auto [prologue, wrappedValue, epilogue] = wrapIfMismatchingType(p, value);
        code += prologue;
        code << CodeGeneratorUtility::generate_getPrivateClass(accessor, p) + u"->" + propertySetter
                        + u"(" + wrappedValue + u");";
        code += epilogue;
    } else {
        // this property is weird, fallback to `setProperty`
        code << u"{ // couldn't find property setter, so using QObject::setProperty()"_qs;
        QString val = value;
        if (constructQVariant) {
            const QString variantName = u"var_" + propertyName;
            code << u"QVariant " + variantName + u";";
            code << variantName + u".setValue(" + val + u");";
            val = u"std::move(" + variantName + u")";
        }
        // NB: setProperty() would handle private properties
        code << accessor + u"->setProperty(\"" + propertyName + u"\", " + val + u");";
        code << u"}"_qs;
    }

    return code;
}

QStringList CodeGeneratorUtility::generate_assignToSpecialAlias(
        const QQmlJSScope::ConstPtr &type, const QString &propertyName, const QQmlJSMetaProperty &p,
        const QString &value, const QString &accessor, bool constructQVariant)
{
    Q_UNUSED(type);
    Q_UNUSED(propertyName);
    Q_UNUSED(constructQVariant);

    Q_ASSERT(p.isValid());
    Q_ASSERT(!p.isList()); // NB: this code does not handle list properties
    Q_ASSERT(p.isAlias());
    Q_ASSERT(p.type());

    QStringList code;
    code.reserve(6); // should always be enough
    // pretend there's a WRITE function
    Qml2CppPropertyData data(p);
    auto [prologue, wrappedValue, epilogue] = wrapIfMismatchingType(p, value);
    code += prologue;
    code << CodeGeneratorUtility::generate_getPrivateClass(accessor, p) + u"->" + data.write + u"("
                    + wrappedValue + u");";
    code += epilogue;
    return code;
}

QStringList CodeGeneratorUtility::generate_callExecuteRuntimeFunction(
        const QString &url, qsizetype index, const QString &accessor, const QString &returnType,
        const QList<QQmlJSAotVariable> &parameters)
{
    QStringList code;
    code.reserve(12); // should always be enough

    code << u"QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(" + accessor + u"));";

    const QString returnValueName = u"_ret"_qs;
    QStringList args;
    args.reserve(parameters.size() + 1);
    QStringList types;
    types.reserve(parameters.size() + 1);
    if (returnType == u"void"_qs) {
        args << u"nullptr"_qs;
        types << u"QMetaType::fromType<void>()"_qs;
    } else {
        code << returnType + u" " + returnValueName + u"{};"; // TYPE _ret{};
        args << u"const_cast<void *>(reinterpret_cast<const void *>(std::addressof("
                        + returnValueName + u")))";
        types << u"QMetaType::fromType<std::decay_t<" + returnType + u">>()";
    }

    for (const QQmlJSAotVariable &p : parameters) {
        args << u"const_cast<void *>(reinterpret_cast<const void *>(std::addressof(" + p.name
                        + u")))";
        types << u"QMetaType::fromType<std::decay_t<" + p.cppType + u">>()";
    }

    code << u"void *_a[] = { " + args.join(u", "_qs) + u" };";
    code << u"QMetaType _t[] = { " + types.join(u", "_qs) + u" };";
    code << u"e->executeRuntimeFunction(" + url + u", " + QString::number(index) + u", " + accessor
                    + u", " + QString::number(parameters.size()) + u", _a, _t);";
    if (returnType != u"void"_qs)
        code << u"return " + returnValueName + u";";

    return code;
}

QStringList CodeGeneratorUtility::generate_createBindingOnProperty(
        const QString &unitVarName, const QString &scope, qsizetype functionIndex,
        const QString &target, int propertyIndex, const QQmlJSMetaProperty &p, int valueTypeIndex,
        const QString &subTarget)
{
    QStringList code;
    code.reserve(1);

    const QString propName = u"QStringLiteral(\"" + p.propertyName() + u"\")";
    if (QString bindable = p.bindable(); !bindable.isEmpty()) {
        // TODO: test that private properties are bindable
        QString createBindingForBindable = u"QT_PREPEND_NAMESPACE(QQmlCppBinding)::"
                                           u"createBindingForBindable("
                + unitVarName + u", " + scope + u", " + QString::number(functionIndex) + u", "
                + target + u", " + QString::number(propertyIndex) + u", "
                + QString::number(valueTypeIndex) + u", " + propName + u")";
        const QString accessor = (valueTypeIndex == -1) ? target : subTarget;
        code << CodeGeneratorUtility::generate_getPrivateClass(accessor, p) + u"->" + bindable
                        + u"().setBinding(" + createBindingForBindable + u");";
    } else {
        // TODO: test that bindings on private properties also work
        QString createBindingForNonBindable =
                u"QT_PREPEND_NAMESPACE(QQmlCppBinding)::createBindingForNonBindable(" + unitVarName
                + u", " + scope + u", " + QString::number(functionIndex) + u", " + target + u", "
                + QString::number(propertyIndex) + u", " + QString::number(valueTypeIndex) + u", "
                + propName + u")";
        // Note: in this version, the binding is set implicitly
        code << createBindingForNonBindable + u";";
    }

    return code;
}

QString CodeGeneratorUtility::generate_qOverload(const QList<QQmlJSAotVariable> &params,
                                                 const QString &overloaded)
{
    QStringList types;
    types.reserve(params.size());
    for (const QQmlJSAotVariable &p : params)
        types.emplaceBack(p.cppType);
    return u"qOverload<" + types.join(u", "_qs) + u">(" + overloaded + u")";
}

QString CodeGeneratorUtility::generate_addressof(const QString &addressed)
{
    return u"(&" + addressed + u")";
}

QString CodeGeneratorUtility::generate_getPrivateClass(const QString &accessor,
                                                       const QQmlJSMetaProperty &p)
{
    if (!p.isPrivate())
        return accessor;

    const QString privateType = p.privateClass();
    return u"static_cast<" + privateType + u" *>(QObjectPrivate::get(" + accessor + u"))";
}

QString CodeGeneratorUtility::generate_setIdValue(const QString &context, qsizetype index,
                                                  const QString &accessor, const QString &idString)
{
    Q_ASSERT(index >= 0);
    const QString idComment = u"/* id: " + idString + u" */";
    return context + u"->setIdValue(" + QString::number(index) + idComment + u", " + accessor
            + u")";
}
