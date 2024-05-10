// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qqmljsregistercontent_p.h"
#include "qqmljstyperesolver_p.h"

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QString QQmlJSRegisterContent::descriptiveName() const
{
    if (m_storedType.isNull())
        return u"(invalid type)"_s;

    const auto scope = [this]() -> QString {
        if (m_scope.isNull())
            return u"(invalid type)::"_s;
        return (m_scope->internalName().isEmpty()
                        ? (m_scope->filePath().isEmpty()
                                   ? u"??"_s
                                   : (u"(component in "_s + m_scope->filePath() + u")"_s))
                        : m_scope->internalName())
                + u"::"_s;
    };

    QString result;
    switch (m_content.index()) {
    case Type: {
        auto contained = std::get<std::pair<QQmlJSScope::ConstPtr, int>>(m_content).first;
        result += contained->internalName();
        if (m_storedType->internalName() != contained->internalName())
            result += u" stored as "_s + m_storedType->internalName();
        return result;
    }
    case Property: {
        const QQmlJSMetaProperty prop = std::get<PropertyLookup>(m_content).property;
        result += scope() + prop.propertyName() + u" with type "_s + prop.typeName();
        if (m_storedType->internalName() != prop.typeName())
            result += u" (stored as "_s + m_storedType->internalName() + u")";
        return result;
    }
    case Method: {
        const auto methods = std::get<QList<QQmlJSMetaMethod>>(m_content);
        if (methods.isEmpty())
            result = scope() + u"(unknown method)"_s;
        else
            result = scope() + methods[0].methodName() + u"(...)"_s;
        return result + u" (stored as "_s + m_storedType->internalName() + u")";
    }
    case Enum: {
        const auto e = std::get<std::pair<QQmlJSMetaEnum, QString>>(m_content);
        if (e.second.isEmpty())
            result = scope() + e.first.name();
        else
            result = scope() + e.first.name() + u"::"_s + e.second;
        return result + u" (stored as "_s + m_storedType->internalName() + u")";
    }
    case ImportNamespace: {
        return u"import namespace %1"_s.arg(std::get<uint>(m_content));
    }
    case Conversion: {
        return u"conversion to %1"_s.arg(std::get<ConvertedTypes>(m_content).result->internalName());
    }
    }

    Q_UNREACHABLE_RETURN(result + u"wat?"_s);
}

bool QQmlJSRegisterContent::isList() const
{
    switch (m_content.index()) {
    case Type:
        return std::get<std::pair<QQmlJSScope::ConstPtr, int>>(m_content).first->accessSemantics()
                == QQmlJSScope::AccessSemantics::Sequence;
    case Property:
        return std::get<PropertyLookup>(m_content).property.type()->accessSemantics()
                == QQmlJSScope::AccessSemantics::Sequence;
    case Conversion:
        return std::get<ConvertedTypes>(m_content).result->accessSemantics()
                == QQmlJSScope::AccessSemantics::Sequence;
    default:
        return false;
    }
}

bool QQmlJSRegisterContent::isWritable() const
{
    switch (m_content.index()) {
    case Property:
        return std::get<PropertyLookup>(m_content).property.isWritable();

    // TODO: What can we actually write?
    default:
        break;
    }

    return true;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(const QQmlJSScope::ConstPtr &storedType,
                                                    const QQmlJSScope::ConstPtr &type,
                                                    int resultLookupIndex,
                                                    QQmlJSRegisterContent::ContentVariant variant,
                                                    const QQmlJSScope::ConstPtr &scope)
{
    QQmlJSRegisterContent result(storedType, scope, variant);
    result.m_content = std::make_pair(type, resultLookupIndex);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(const QQmlJSScope::ConstPtr &storedType,
                                                    const QQmlJSMetaProperty &property,
                                                    int baseLookupIndex, int resultLookupIndex,
                                                    QQmlJSRegisterContent::ContentVariant variant,
                                                    const QQmlJSScope::ConstPtr &scope)
{
    QQmlJSRegisterContent result(storedType, scope, variant);
    result.m_content = PropertyLookup { property, baseLookupIndex, resultLookupIndex};
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(const QQmlJSScope::ConstPtr &storedType,
                                                    const QQmlJSMetaEnum &enumeration,
                                                    const QString &enumMember,
                                                    QQmlJSRegisterContent::ContentVariant variant,
                                                    const QQmlJSScope::ConstPtr &scope)
{
    QQmlJSRegisterContent result(storedType, scope, variant);
    result.m_content = std::make_pair(enumeration, enumMember);
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(const QQmlJSScope::ConstPtr &storedType,
                                                    const QList<QQmlJSMetaMethod> &methods,
                                                    QQmlJSRegisterContent::ContentVariant variant,
                                                    const QQmlJSScope::ConstPtr &scope)
{
    QQmlJSRegisterContent result(storedType, scope, variant);
    result.m_content = methods;
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(const QQmlJSScope::ConstPtr &storedType,
                                                    uint importNamespaceStringId,
                                                    QQmlJSRegisterContent::ContentVariant variant,
                                                    const QQmlJSScope::ConstPtr &scope)
{
    QQmlJSRegisterContent result(storedType, scope, variant);
    result.m_content = importNamespaceStringId;
    return result;
}

QQmlJSRegisterContent QQmlJSRegisterContent::create(const QQmlJSScope::ConstPtr &storedType,
                                                    const QList<QQmlJSScope::ConstPtr> &origins,
                                                    const QQmlJSScope::ConstPtr &conversion,
                                                    const QQmlJSScope::ConstPtr &conversionScope,
                                                    ContentVariant variant,
                                                    const QQmlJSScope::ConstPtr &scope)
{
    QQmlJSRegisterContent result(storedType, scope, variant);
    result.m_content = ConvertedTypes { origins, conversion, conversionScope };
    return result;
}

QT_END_NAMESPACE
