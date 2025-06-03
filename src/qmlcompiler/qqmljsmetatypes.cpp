// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsmetatypes_p.h"
#include "qqmljstyperesolver_p.h"

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#    include "QtQml/private/qqmltranslation_p.h"
#endif

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

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
/*!
 *  Extracts the information about translations from a binding.
 *  An additional context string is needed for text based translation (e.g. with qsTr())
 *  and can be obtained from the name of the qml file.
 *
 *  \sa QQmlTranslation
 */
QQmlTranslation QQmlJSMetaPropertyBinding::translationDataValue(QString qmlFileNameForContext) const
{
    QQmlTranslation::Data data;
    if (auto translation = std::get_if<Content::TranslationById>(&m_bindingContent)) {
        data = QQmlTranslation::QsTrIdData(translation->id, translation->number);
    } else if (auto translation = std::get_if<Content::TranslationString>(&m_bindingContent)) {
        const QString context = translation->context.isEmpty()
                ? QQmlTranslation::contextFromQmlFilename(qmlFileNameForContext)
                : translation->context;
        data = QQmlTranslation::QsTrData(context, translation->text, translation->comment,
                                         translation->number);
    }
    return QQmlTranslation(data);
}
#endif

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
    case QQmlJSMetaPropertyBinding::Invalid:
    case QQmlJSMetaPropertyBinding::Script:
    case QQmlJSMetaPropertyBinding::Object:
    case QQmlJSMetaPropertyBinding::Interceptor:
    case QQmlJSMetaPropertyBinding::ValueSource:
    case QQmlJSMetaPropertyBinding::AttachedProperty:
    case QQmlJSMetaPropertyBinding::GroupProperty:
        return {};
    }
    Q_UNREACHABLE_RETURN({});
}

QT_END_NAMESPACE
