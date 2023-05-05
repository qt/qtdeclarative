// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmltccompilerpieces.h"

#include <private/qqmljsutils_p.h>

#include <tuple>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

static QString scopeName(const QQmlJSScope::ConstPtr &scope)
{
    Q_ASSERT(scope->isFullyResolved());
    const auto scopeType = scope->scopeType();
    if (scopeType == QQmlSA::ScopeType::GroupedPropertyScope
        || scopeType == QQmlSA::ScopeType::AttachedPropertyScope) {
        return scope->baseType()->internalName();
    }
    return scope->internalName();
}

QmltcCodeGenerator::PreparedValue
QmltcCodeGenerator::wrap_extensionType(const QQmlJSScope::ConstPtr &type,
                                       const QQmlJSMetaProperty &p, const QString &accessor)
{
    Q_ASSERT(type->isFullyResolved());

    QStringList prologue;
    QString value = accessor;
    QStringList epilogue;

    auto [owner, ownerKind] = QQmlJSScope::ownerOfProperty(type, p.propertyName());
    Q_ASSERT(owner);
    Q_ASSERT(owner->isFullyResolved());

    // properties are only visible when we use QML_{NAMESPACE_}EXTENDED
    if (ownerKind == QQmlJSScope::ExtensionType) {
        // extensions is a C++-only feature:
        Q_ASSERT(!owner->isComposite());

        // have to wrap the property into an extension, but we need to figure
        // out whether the type is QObject-based or not
        prologue << u"{"_s;
        const QString extensionObjectName = u"extObject"_s;
        if (type->accessSemantics() == QQmlJSScope::AccessSemantics::Reference) {
            // we have a Q_OBJECT. in this case, we call qmlExtendedObject()
            // function that should return us the extension object. for that we
            // have to figure out which specific extension we want here

            int extensionIndex = 0;
            auto cppBase = QQmlJSScope::nonCompositeBaseType(type);
            for (auto t = cppBase; t; t = t->baseType()) {
                if (auto [ext, kind] = t->extensionType(); kind != QQmlJSScope::NotExtension) {
                    if (ext->isSameType(owner))
                        break;
                    ++extensionIndex;
                }
            }

            prologue << u"static_assert(std::is_base_of<%1, %2>::value);"_s.arg(u"QObject"_s,
                                                                                scopeName(type));
            prologue << u"auto %1 = qobject_cast<%2 *>(QQmlPrivate::qmlExtendedObject(%3, %4));"_s
                                .arg(extensionObjectName, owner->internalName(), accessor,
                                     QString::number(extensionIndex));
        } else {
            // we have a Q_GADGET. the assumption for extension types is that we
            // can reinterpret_cast a Q_GADGET object into an extension type
            // object and then interact with the extension object right away
            prologue << u"static_assert(sizeof(%1) == sizeof(%2));"_s.arg(scopeName(type),
                                                                          owner->internalName());
            prologue << u"static_assert(alignof(%1) == alignof(%2));"_s.arg(scopeName(type),
                                                                            owner->internalName());
            prologue << u"auto %1 = reinterpret_cast<%2 *>(%3);"_s.arg(
                    extensionObjectName, owner->internalName(), accessor);
        }
        prologue << u"Q_ASSERT(%1);"_s.arg(extensionObjectName);
        value = extensionObjectName;
        epilogue << u"}"_s;
    }

    return { prologue, value, epilogue };
}

void QmltcCodeGenerator::generate_assignToListProperty(
        QStringList *block, const QQmlJSScope::ConstPtr &type, const QQmlJSMetaProperty &p,
        const QStringList &values, const QString &accessor, QString &qmlListVarName)
{
    Q_UNUSED(type); // might be needed
    const bool populateLocalListProperty = qmlListVarName.isEmpty();

    if (populateLocalListProperty) {
        auto [extensionPrologue, extensionAccessor, extensionEpilogue] =
                QmltcCodeGenerator::wrap_extensionType(
                        type, p, QmltcCodeGenerator::wrap_privateClass(accessor, p));

        qmlListVarName = u"listprop_%1"_s.arg(p.propertyName());
        QQmlJSScope::ConstPtr valueType = p.type()->valueType();
        *block << u"QQmlListProperty<%1> %2;"_s.arg(valueType->internalName(), qmlListVarName);
        *block << extensionPrologue;
        *block << u"%1 = %2->%3();"_s.arg(qmlListVarName, extensionAccessor, p.read());
        *block << extensionEpilogue;
    }
    for (const QString &value : values) {
        auto [prologue, wrappedValue, epilogue] =
                QmltcCodeGenerator::wrap_mismatchingTypeConversion(p, value);
        *block << prologue;
        *block << u"%1.append(std::addressof(%1), %2);"_s.arg(qmlListVarName, wrappedValue);
        *block << epilogue;
    }
}

void QmltcCodeGenerator::generate_assignToProperty(QStringList *block,
                                                   const QQmlJSScope::ConstPtr &type,
                                                   const QQmlJSMetaProperty &p,
                                                   const QString &value, const QString &accessor,
                                                   bool constructFromQObject)
{
    Q_ASSERT(block);
    Q_ASSERT(p.isValid());
    Q_ASSERT(!p.isList()); // NB: this code does not handle list properties

    const QString propertyName = p.propertyName();

    if (type->hasOwnProperty(p.propertyName()) && !p.isAlias()) {
        Q_ASSERT(!p.isPrivate());
        // this object is compiled, so just assignment should work fine
        auto [prologue, wrappedValue, epilogue] =
                QmltcCodeGenerator::wrap_mismatchingTypeConversion(p, value);
        *block += prologue;
        *block << u"%1->m_%2 = %3;"_s.arg(accessor, propertyName, wrappedValue);
        *block += epilogue;
    } else if (QString propertySetter = p.write(); !propertySetter.isEmpty()
               && !QQmlJSUtils::bindablePropertyHasDefaultAccessor(
                       p, QQmlJSUtils::PropertyAccessor_Write)) {
        // there's a WRITE function
        auto [prologue, wrappedValue, epilogue] =
                QmltcCodeGenerator::wrap_mismatchingTypeConversion(p, value);
        *block += prologue;

        auto [extensionPrologue, extensionAccessor, extensionEpilogue] =
                QmltcCodeGenerator::wrap_extensionType(
                        type, p, QmltcCodeGenerator::wrap_privateClass(accessor, p));
        *block += extensionPrologue;
        *block << extensionAccessor + u"->" + propertySetter + u"(" + wrappedValue + u");";
        *block += extensionEpilogue;

        *block += epilogue;
    } else {
        // this property is weird, fallback to `setProperty`
        *block << u"{ // couldn't find property setter, so using QObject::setProperty()"_s;
        QString val = value;
        if (constructFromQObject) {
            const QString variantName = u"var_" + propertyName;
            *block << u"QVariant " + variantName + u";";
            *block << variantName + u".setValue(" + val + u");";
            val = u"std::move(" + variantName + u")";
        }
        // NB: setProperty() would handle private properties
        *block << accessor + u"->setProperty(\"" + propertyName + u"\", " + val + u");";
        *block << u"}"_s;
    }
}

void QmltcCodeGenerator::generate_setIdValue(QStringList *block, const QString &context,
                                             qsizetype index, const QString &accessor,
                                             const QString &idString)
{
    Q_ASSERT(index >= 0);
    *block << u"Q_ASSERT(%1 < %2->numIdValues()); // make sure Id is in bounds"_s.arg(index).arg(
            context);
    *block << u"%1->setIdValue(%2 /* id: %3 */, %4);"_s.arg(context, QString::number(index),
                                                             idString, accessor);
}

void QmltcCodeGenerator::generate_callExecuteRuntimeFunction(
        QStringList *block, const QString &url, QQmlJSMetaMethod::AbsoluteFunctionIndex index,
        const QString &accessor, const QString &returnType, const QList<QmltcVariable> &parameters)
{
    *block << u"QQmlEnginePrivate *e = QQmlEnginePrivate::get(qmlEngine(" + accessor + u"));";

    const QString returnValueName = u"_ret"_s;
    QStringList args;
    args.reserve(parameters.size() + 1);
    QStringList types;
    types.reserve(parameters.size() + 1);
    if (returnType == u"void"_s) {
        args << u"nullptr"_s;
        types << u"QMetaType::fromType<void>()"_s;
    } else {
        *block << returnType + u" " + returnValueName + u"{};"; // TYPE _ret{};
        args << u"const_cast<void *>(reinterpret_cast<const void *>(std::addressof("
                        + returnValueName + u")))";
        types << u"QMetaType::fromType<std::decay_t<" + returnType + u">>()";
    }

    for (const QmltcVariable &p : parameters) {
        args << u"const_cast<void *>(reinterpret_cast<const void *>(std::addressof(" + p.name
                        + u")))";
        types << u"QMetaType::fromType<std::decay_t<" + p.cppType + u">>()";
    }

    *block << u"void *_a[] = { " + args.join(u", "_s) + u" };";
    *block << u"QMetaType _t[] = { " + types.join(u", "_s) + u" };";
    const qsizetype runtimeIndex = static_cast<qsizetype>(index);
    Q_ASSERT(runtimeIndex >= 0);
    *block << u"e->executeRuntimeFunction(" + url + u", " + QString::number(runtimeIndex) + u", "
                    + accessor + u", " + QString::number(parameters.size()) + u", _a, _t);";
    if (returnType != u"void"_s)
        *block << u"return " + returnValueName + u";";
}

void QmltcCodeGenerator::generate_createBindingOnProperty(
        QStringList *block, const QString &unitVarName, const QString &scope,
        qsizetype functionIndex, const QString &target, const QQmlJSScope::ConstPtr &targetType,
        int propertyIndex, const QQmlJSMetaProperty &p, int valueTypeIndex,
        const QString &subTarget)
{
    const QString propName = QQmlJSUtils::toLiteral(p.propertyName());
    if (QString bindable = p.bindable(); !bindable.isEmpty()) {
        // TODO: test that private properties are bindable
        QString createBindingForBindable = u"QT_PREPEND_NAMESPACE(QQmlCppBinding)::"
                                           u"createBindingForBindable("
                + unitVarName + u", " + scope + u", " + QString::number(functionIndex) + u", "
                + target + u", " + QString::number(propertyIndex) + u", "
                + QString::number(valueTypeIndex) + u", " + propName + u")";
        const QString accessor = (valueTypeIndex == -1) ? target : subTarget;

        QStringList prologue;
        QString value = QmltcCodeGenerator::wrap_privateClass(accessor, p);
        QStringList epilogue;
        if (targetType) {
            auto [pro, v, epi] = QmltcCodeGenerator::wrap_extensionType(targetType, p, value);
            std::tie(prologue, value, epilogue) = std::make_tuple(pro, v, epi);
        }

        *block += prologue;
        *block << value + u"->" + bindable + u"().setBinding(" + createBindingForBindable + u");";
        *block += epilogue;
    } else {
        QString createBindingForNonBindable =
                u"QT_PREPEND_NAMESPACE(QQmlCppBinding)::createBindingForNonBindable(" + unitVarName
                + u", " + scope + u", " + QString::number(functionIndex) + u", " + target + u", "
                + QString::number(propertyIndex) + u", " + QString::number(valueTypeIndex) + u", "
                + propName + u")";
        // Note: in this version, the binding is set implicitly
        *block << createBindingForNonBindable + u";";
    }
}

void QmltcCodeGenerator::generate_createTranslationBindingOnProperty(
        QStringList *block, const TranslationBindingInfo &info)
{
    const QString propName = QQmlJSUtils::toLiteral(info.property.propertyName());
    const QString qqmlTranslation = info.data.serializeForQmltc();

    if (QString bindable = info.property.bindable(); !bindable.isEmpty()) {
        // TODO: test that private properties are bindable
        QString createTranslationCode = uR"(QT_PREPEND_NAMESPACE(QQmlCppBinding)
        ::createTranslationBindingForBindable(%1, %2, %3, %4, %5))"_s
                                                .arg(info.unitVarName, info.target)
                                                .arg(info.propertyIndex)
                                                .arg(qqmlTranslation, propName);

        *block << QmltcCodeGenerator::wrap_privateClass(info.target, info.property) + u"->"
                        + bindable + u"().setBinding(" + createTranslationCode + u");";
    } else {
        QString locationString =
                u"QQmlSourceLocation(%1->fileName(), %2, %3)"_s.arg(info.unitVarName)
                        .arg(info.line)
                        .arg(info.column);
        QString createTranslationCode = uR"(QT_PREPEND_NAMESPACE(QQmlCppBinding)
    ::createTranslationBindingForNonBindable(
        %1, //unit
        %2, //location
        %3, //translationData
        %4, //thisObject
        %5, //bindingTarget
        %6, //metaPropertyIndex
        %7, //propertyName
        %8) //valueTypePropertyIndex
    )"_s.arg(info.unitVarName, locationString, qqmlTranslation, info.scope, info.target)
                                                .arg(info.propertyIndex)
                                                .arg(propName)
                                                .arg(info.valueTypeIndex);
        // Note: in this version, the binding is set implicitly
        *block << createTranslationCode + u";";
    }
}

QmltcCodeGenerator::PreparedValue
QmltcCodeGenerator::wrap_mismatchingTypeConversion(const QQmlJSMetaProperty &p, QString value)
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
    if (isDerivedFromBuiltin(propType, u"QVariant"_s)) {
        const QString variantName = u"var_" + p.propertyName();
        prologue << u"{ // accepts QVariant"_s;
        prologue << u"QVariant " + variantName + u";";
        prologue << variantName + u".setValue(" + value + u");";
        epilogue << u"}"_s;
        value = u"std::move(" + variantName + u")";
    } else if (isDerivedFromBuiltin(propType, u"QJSValue"_s)) {
        const QString jsvalueName = u"jsvalue_" + p.propertyName();
        prologue << u"{ // accepts QJSValue"_s;
        // Note: do not assume we have the engine, acquire it from `this`
        prologue << u"auto e = qmlEngine(this);"_s;
        prologue << u"QJSValue " + jsvalueName + u" = e->toScriptValue(" + value + u");";
        epilogue << u"}"_s;
        value = u"std::move(" + jsvalueName + u")";
    }
    return { prologue, value, epilogue };
}

QString QmltcCodeGenerator::wrap_privateClass(const QString &accessor, const QQmlJSMetaProperty &p)
{
    if (!p.isPrivate())
        return accessor;

    const QString privateType = p.privateClass();
    return u"static_cast<" + privateType + u" *>(QObjectPrivate::get(" + accessor + u"))";
}

QString QmltcCodeGenerator::wrap_qOverload(const QList<QmltcVariable> &parameters,
                                           const QString &overloaded)
{
    QStringList types;
    types.reserve(parameters.size());
    for (const QmltcVariable &p : parameters)
        types.emplaceBack(p.cppType);
    return u"qOverload<" + types.join(u", "_s) + u">(" + overloaded + u")";
}

QString QmltcCodeGenerator::wrap_addressof(const QString &addressed)
{
    return u"std::addressof(" + addressed + u")";
}

QT_END_NAMESPACE
