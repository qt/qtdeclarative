// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsliteralbindingcheck_p.h"

#include <private/qqmljsimportvisitor_p.h>
#include <private/qqmljstyperesolver_p.h>
#include <private/qqmljsmetatypes_p.h>
#include <private/qqmlsa_p.h>
#include <private/qqmlsasourcelocation_p.h>

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
    : QQmlSA::PropertyPass(passManager),
      m_resolver(QQmlSA::PassManagerPrivate::resolver(*passManager))
{
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

void QQmlJSLiteralBindingCheck::onBinding(const QQmlSA::Element &element, const QString &propertyName,
                           const QQmlSA::Binding &binding, const QQmlSA::Element &bindingScope,
                           const QQmlSA::Element &value)
{
    Q_UNUSED(value);

    if (!QQmlSA::Binding::isLiteralBinding(binding.bindingType()))
        return;

    const QString unqualifiedPropertyName = [&propertyName]() {
        if (auto idx = propertyName.lastIndexOf(u'.'); idx != -1 && idx != propertyName.size() - 1)
            return propertyName.sliced(idx + 1);
        return propertyName;
    }();
    const auto property = bindingScope.property(unqualifiedPropertyName);
    if (!property.isValid())
        return;

    // If the property is defined in the same scope where it is set,
    // we are in fact allowed to set it, even if it's not writable.
    if (property.isReadonly() && !element.hasOwnProperty(propertyName)) {
        emitWarning(u"Cannot assign to read-only property %1"_s.arg(propertyName),
                    qmlReadOnlyProperty, binding.sourceLocation());
        return;
    }

    if (!canConvertForLiteralBinding(m_resolver, resolveLiteralType(binding), property.type())) {
        emitWarning(u"Cannot assign literal of type %1 to %2"_s.arg(
                            literalPrettyTypeName(binding.bindingType()),
                            QQmlJSScope::prettyName(property.typeName())),
                    qmlIncompatibleType, binding.sourceLocation());
        return;
    }
}

QT_END_NAMESPACE
