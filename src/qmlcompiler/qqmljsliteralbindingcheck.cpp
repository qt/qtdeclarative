// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsliteralbindingcheck_p.h"

#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qqmljsmetatypes_p.h>
#include <private/qqmlsa_p.h>
#include <private/qqmlsasourcelocation_p.h>
#include <private/qqmlstringconverters_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

// This makes no sense, but we want to warn about things QQmlPropertyResolver complains about.
static bool canConvertForLiteralBinding(QQmlJSTypeResolver *resolver,
                                        const QQmlSA::Element &fromElement,
                                        const QQmlSA::Element &toElement)
{
    Q_ASSERT(resolver);
    auto from = QQmlJSScope::scope(fromElement);
    auto to = QQmlJSScope::scope(toElement);
    if (resolver->equals(from, to))
        return true;

    if (!resolver->canConvertFromTo(from, to))
        return false;

    const bool fromIsString = resolver->equals(from, resolver->stringType());

    if (resolver->equals(to, resolver->stringType())
            || resolver->equals(to, resolver->stringListType())
            || resolver->equals(to, resolver->byteArrayType())
            || resolver->equals(to, resolver->urlType())) {
        return fromIsString;
    }

    if (resolver->isNumeric(to))
        return resolver->isNumeric(from);

    if (resolver->equals(to, resolver->boolType()))
        return resolver->equals(from, resolver->boolType());

    return true;
}

QQmlJSLiteralBindingCheck::QQmlJSLiteralBindingCheck(QQmlSA::PassManager *passManager)
    : LiteralBindingCheckBase(passManager),
      m_resolver(QQmlSA::PassManagerPrivate::resolver(*passManager))
{
}

QQmlJSStructuredTypeError QQmlJSLiteralBindingCheck::check(const QString &typeName,
                                                           const QString &value) const
{
    return QQmlJSValueTypeFromStringCheck::hasError(typeName, value);
}

static QString literalPrettyTypeName(QQmlSA::BindingType type)
{
    switch (type) {
    case QQmlSA::BindingType::BoolLiteral:
        return u"bool"_s;
    case QQmlSA::BindingType::NumberLiteral:
        return u"double"_s;
    case QQmlSA::BindingType::StringLiteral:
        return u"string"_s;
    case QQmlSA::BindingType::RegExpLiteral:
        return u"regexp"_s;
    case QQmlSA::BindingType::Null:
        return u"null"_s;
    default:
        return QString();
    }
    Q_UNREACHABLE_RETURN(QString());
}

QQmlSA::Property LiteralBindingCheckBase::getProperty(const QString &propertyName,
                                                      const QQmlSA::Binding &binding,
                                                      const QQmlSA::Element &bindingScope) const
{
    if (!QQmlSA::Binding::isLiteralBinding(binding.bindingType()))
        return {};

    const QString unqualifiedPropertyName = [&propertyName]() -> QString {
        if (auto idx = propertyName.lastIndexOf(u'.'); idx != -1 && idx != propertyName.size() - 1)
            return propertyName.sliced(idx + 1);
        return propertyName;
    }();

    return bindingScope.property(unqualifiedPropertyName);
}


void LiteralBindingCheckBase::onBinding(const QQmlSA::Element &element, const QString &propertyName,
                                        const QQmlSA::Binding &binding,
                                        const QQmlSA::Element &bindingScope,
                                        const QQmlSA::Element &value)
{
    Q_UNUSED(value);

    const auto property = getProperty(propertyName, binding, bindingScope);
    if (!property.isValid())
        return;

    // If the property is defined in the same scope where it is set,
    // we are in fact allowed to set it, even if it's not writable.
    if (property.isReadonly() && !element.hasOwnProperty(propertyName)) {
        emitWarning(u"Cannot assign to read-only property %1"_s.arg(propertyName),
                    qmlReadOnlyProperty, binding.sourceLocation());
        return;
    }
    if (auto propertyType = property.type(); propertyType) {
        auto construction = check(propertyType.internalId(), binding.stringValue());
        if (construction.isValid()) {
            const QString warningMessage =
                    u"Construction from string is deprecated. Use structured value type "
                    u"construction instead for type \"%1\""_s.arg(propertyType.internalId());

            if (!construction.code.isNull()) {
                QQmlSA::FixSuggestion suggestion(
                        u"Replace string by structured value construction"_s,
                        binding.sourceLocation(), construction.code);
                emitWarning(warningMessage, qmlIncompatibleType, binding.sourceLocation(), suggestion);
                return;
            }
            emitWarning(warningMessage, qmlIncompatibleType, binding.sourceLocation());
            return;
        }
    }

}

void QQmlJSLiteralBindingCheck::onBinding(const QQmlSA::Element &element,
                                          const QString &propertyName,
                                          const QQmlSA::Binding &binding,
                                          const QQmlSA::Element &bindingScope,
                                          const QQmlSA::Element &value)
{
    LiteralBindingCheckBase::onBinding(element, propertyName, binding, bindingScope, value);

    const auto property = getProperty(propertyName, binding, bindingScope);
    if (!property.isValid())
        return;

    if (!canConvertForLiteralBinding(m_resolver, resolveLiteralType(binding), property.type())) {
        emitWarning(u"Cannot assign literal of type %1 to %2"_s.arg(
                            literalPrettyTypeName(binding.bindingType()),
                            QQmlJSScope::prettyName(property.typeName())),
                    qmlIncompatibleType, binding.sourceLocation());
        return;
    }
}

QT_END_NAMESPACE
