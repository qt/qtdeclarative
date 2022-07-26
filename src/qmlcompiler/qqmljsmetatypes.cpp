// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsmetatypes_p.h"
#include "qqmljstyperesolver_p.h"

QT_BEGIN_NAMESPACE

/*!
    \internal
    A binding is valid when it has both a target (m_propertyName is set)
    and some content set (m_bindingType != Invalid).
*/
bool QQmlJSMetaPropertyBinding::isValid() const { return !m_propertyName.isEmpty() && bindingType() != Invalid; }

QString QQmlJSMetaPropertyBinding::literalTypeName() const
{
    if (std::holds_alternative<Content::BoolLiteral>(m_bindingContent))
        return QLatin1String("bool");
    else if (std::holds_alternative<Content::NumberLiteral>(m_bindingContent))
        return QLatin1String("double");
    else if (std::holds_alternative<Content::StringLiteral>(m_bindingContent))
        return QLatin1String("string");
    else if (std::holds_alternative<Content::RegexpLiteral>(m_bindingContent))
        return QLatin1String("regexp");
    else if (std::holds_alternative<Content::Null>(m_bindingContent))
        return QLatin1String("$internal$.std::nullptr_t");
    return {};
}

bool QQmlJSMetaPropertyBinding::boolValue() const
{
    if (auto boolLit = std::get_if<Content::BoolLiteral>(&m_bindingContent))
        return boolLit->value;
    // warn
    return false;
}

double QQmlJSMetaPropertyBinding::numberValue() const
{
    if (auto numberLit = std::get_if<Content::NumberLiteral>(&m_bindingContent))
        return numberLit->value;
    // warn
    return 0;
}

QString QQmlJSMetaPropertyBinding::stringValue() const
{
    if (auto stringLiteral = std::get_if<Content::StringLiteral>(&m_bindingContent))
        return stringLiteral->value;
    // warn
    return {};
}

QString QQmlJSMetaPropertyBinding::regExpValue() const
{
    if (auto regexpLiteral = std::get_if<Content::RegexpLiteral>(&m_bindingContent))
        return regexpLiteral->value;
    // warn
    return {};
}

/*!
    \internal
    Uses \a resolver to return the correct type for the stored literal
    and a null scope pointer if the binding does not contain a literal
 */
QSharedPointer<const QQmlJSScope> QQmlJSMetaPropertyBinding::literalType(const QQmlJSTypeResolver *resolver) const
{
    Q_ASSERT(resolver);
    switch (bindingType()) {
    case QQmlJSMetaPropertyBinding::BoolLiteral:
        return resolver->boolType();
    case QQmlJSMetaPropertyBinding::NumberLiteral:
        return resolver->typeForName(QLatin1String("double"));
    case QQmlJSMetaPropertyBinding::Translation: // translations are strings
    case QQmlJSMetaPropertyBinding::TranslationById:
    case QQmlJSMetaPropertyBinding::StringLiteral:
        return resolver->stringType();
    case QQmlJSMetaPropertyBinding::RegExpLiteral:
        return resolver->typeForName(QLatin1String("regexp"));
    case QQmlJSMetaPropertyBinding::Null:
        return resolver->nullType();
    default:
        return {};
    }
    Q_UNREACHABLE();
    return {}; // needed on some compilers which do not see that every case in the switch returns
}

QT_END_NAMESPACE
