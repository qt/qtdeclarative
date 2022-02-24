/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include <private/qqmljsmetatypes_p.h>
#include <private/qqmljstyperesolver_p.h>

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
