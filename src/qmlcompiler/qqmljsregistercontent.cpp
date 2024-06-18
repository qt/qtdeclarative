// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsregistercontent_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QQmlJSRegisterContent::QQmlJSRegisterContent() = default;

QString QQmlJSRegisterContent::descriptiveName() const
{
    if (m_storedType.isNull() && containedType().isNull())
        return u"(invalid type)"_s;

    const auto scope = [this]() -> QString {
        if (m_scope.isNull())
            return u"(invalid type)::"_s;

        const QQmlJSScope::ConstPtr scopeContained = m_scope->containedType();
        if (scopeContained.isNull())
            return u"(invalid type)::"_s;

        return (scopeContained->internalName().isEmpty()
                        ? (scopeContained->filePath().isEmpty()
                                   ? u"??"_s
                                   : (u"(component in "_s + scopeContained->filePath() + u")"_s))
                        : scopeContained->internalName())
                + u"::"_s;
    };

    QString result;
    switch (Kind(m_content.index())) {
    case Kind::Type: {
        const QQmlJSScope::ConstPtr contained = type();
        result += contained->internalName();
        if (m_storedType && m_storedType->internalName() != contained->internalName())
            result += u" stored as "_s + m_storedType->internalName();
        return result;
    }
    case Kind::Property: {
        const QQmlJSMetaProperty prop = property();
        result += scope() + prop.propertyName() + u" with type "_s + prop.typeName();
        if (m_storedType && m_storedType->internalName() != prop.typeName())
            result += u" (stored as "_s + m_storedType->internalName() + u")";
        return result;
    }
    case Kind::Method: {
        const auto methods = method();
        if (methods.isEmpty())
            result = scope() + u"(unknown method)"_s;
        else
            result = scope() + methods[0].methodName() + u"(...)"_s;
        if (m_storedType)
            return result + u" (stored as "_s + m_storedType->internalName() + u")";
        return result;
    }
    case Kind::Enum: {
        const QString enumName = enumeration().name();
        const QString memberName = enumMember();
        if (memberName.isEmpty())
            result = scope() + enumName;
        else
            result = scope() + enumName + u"::"_s + memberName;
        if (m_storedType)
            return result + u" (stored as "_s + m_storedType->internalName() + u")";
        return result;
    }
    case Kind::ImportNamespace: {
        return u"import namespace %1"_s.arg(importNamespace());
    }
    case Kind::Conversion: {
        return u"conversion to %1"_s.arg(conversionResult()->internalName());
    }
    }

    Q_UNREACHABLE_RETURN(result + u"wat?"_s);
}

QString QQmlJSRegisterContent::containedTypeName() const
{
    QQmlJSScope::ConstPtr type;

    switch (variant()) {
    case QQmlJSRegisterContent::MetaType:
        type = scopeType().containedType();
        break;
    default:
        type = containedType();
        break;
    }

    return QQmlJSScope::prettyName(
            type->internalName().isEmpty() ? type->baseTypeName() : type->internalName());
}

bool QQmlJSRegisterContent::isList() const
{
    switch (Kind(m_content.index())) {
    case Kind::Type:
        return std::get<std::pair<QQmlJSScope::ConstPtr, int>>(m_content).first->accessSemantics()
                == QQmlJSScope::AccessSemantics::Sequence;
    case Kind::Property:
        return std::get<PropertyLookup>(m_content).property.type()->accessSemantics()
                == QQmlJSScope::AccessSemantics::Sequence;
    case Kind::Conversion:
        return std::get<ConvertedTypes>(m_content).result->accessSemantics()
                == QQmlJSScope::AccessSemantics::Sequence;
    default:
        return false;
    }
}

bool QQmlJSRegisterContent::isWritable() const
{
    switch (Kind(m_content.index())) {
    case Kind::Property:
        return std::get<PropertyLookup>(m_content).property.isWritable();

    // TODO: What can we actually write?
    default:
        break;
    }

    return true;
}

/*!
 * \internal
 * Determines whether this is the scope object.
 * We omit any module prefixes seen on top of the object.
 * The module prefixes don't actually add anything unless they
 * are the prefix to an attachment.
 */
bool QQmlJSRegisterContent::isScopeObject() const
{
    switch (m_variant) {
    case ScopeObject:
        return true;
    case ModulePrefix:
        return m_scope->isScopeObject();
    default:
        break;
    }

    return false;
}

/*!
 * \internal
 * Precondition: This is an attachment.
 * Return the type that does the attaching.
 */
QQmlJSRegisterContent QQmlJSRegisterContent::attacher() const
{
    Q_ASSERT(m_variant == Attachment);
    return scopeType();
}

/*!
 * \internal
 * Precondition: This is an attachment.
 * Return the type of the object the attachment is attached to.
 */
QQmlJSRegisterContent QQmlJSRegisterContent::attachee() const
{
    Q_ASSERT(m_variant == Attachment);
    QQmlJSRegisterContent attachee = attacher().scopeType();
    while (attachee.variant() == ModulePrefix)
        attachee = attachee.scopeType();
    return attachee;
}

QQmlJSScope::ConstPtr QQmlJSRegisterContent::containedType() const
{
    if (isType())
        return type();
    if (isProperty())
        return property().type();
    if (isEnumeration())
        return enumeration().type();
    if (isMethod())
        return methodType();
    if (isImportNamespace())
        return importNamespaceType();
    if (isConversion())
        return conversionResult();

    Q_UNREACHABLE_RETURN({});
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(
        const QQmlJSScope::ConstPtr &type, int resultLookupIndex,
        QQmlJSRegisterContent::ContentVariant variant, const QQmlJSRegisterContent &scope)
{
    QQmlJSRegisterContent result(scope, variant);
    result.m_content = std::make_pair(type, resultLookupIndex);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(
        const QQmlJSMetaProperty &property, int baseLookupIndex, int resultLookupIndex,
        QQmlJSRegisterContent::ContentVariant variant, const QQmlJSRegisterContent &scope)
{
    QQmlJSRegisterContent result(scope, variant);
    result.m_content = PropertyLookup { property, baseLookupIndex, resultLookupIndex};
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(
        const QQmlJSMetaEnum &enumeration, const QString &enumMember,
        QQmlJSRegisterContent::ContentVariant variant, const QQmlJSRegisterContent &scope)
{
    QQmlJSRegisterContent result(scope, variant);
    result.m_content = std::make_pair(enumeration, enumMember);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(
        const QList<QQmlJSMetaMethod> &methods, const QQmlJSScope::ConstPtr &methodType,
        QQmlJSRegisterContent::ContentVariant variant, const QQmlJSRegisterContent &scope)
{
    // Methods can only be stored in QJSValue.
    Q_ASSERT(methodType->internalName() == u"QJSValue"_s);
    QQmlJSRegisterContent result(scope, variant);
    result.m_content = std::make_pair(methods, methodType);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(
        uint importNamespaceStringId, const QQmlJSScope::ConstPtr &importNamespaceType,
        QQmlJSRegisterContent::ContentVariant variant, const QQmlJSRegisterContent &scope)
{
    QQmlJSRegisterContent result(scope, variant);
    result.m_content = std::make_pair(importNamespaceStringId, importNamespaceType);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(
        const QList<QQmlJSRegisterContent> &origins, const QQmlJSScope::ConstPtr &conversion,
        const QQmlJSRegisterContent &conversionScope, ContentVariant variant,
        const QQmlJSRegisterContent &scope)
{
    QQmlJSRegisterContent result(scope, variant);

    result.m_content = ConvertedTypes {
        origins,
        conversion,
        QSharedPointer<QQmlJSRegisterContent>::create(conversionScope)
    };

    return result;
}

QT_END_NAMESPACE
