// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSREGISTERCONTENT_P_H
#define QQMLJSREGISTERCONTENT_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmljsscope_p.h"
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

#include <variant>

QT_BEGIN_NAMESPACE

class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSRegisterContent
{
public:
    enum ContentVariant {
        ObjectById,
        Singleton,
        Script,
        MetaType,

        JavaScriptGlobal,
        JavaScriptObject,
        JavaScriptScopeProperty,
        GenericObjectProperty, // Can be JSObject property or QVariantMap

        ScopeProperty,
        ScopeMethod,
        ScopeAttached,
        ScopeModulePrefix,
        ExtensionScopeProperty,
        ExtensionScopeMethod,

        ObjectProperty,
        ObjectMethod,
        ObjectEnum,
        ObjectAttached,
        ObjectModulePrefix,
        ExtensionObjectProperty,
        ExtensionObjectMethod,
        ExtensionObjectEnum,

        MethodReturnValue,
        JavaScriptReturnValue,

        ListValue,
        ListIterator,
        Builtin,
        Unknown,
    };

    enum { InvalidLookupIndex = -1 };

    QQmlJSRegisterContent() = default;
    bool isValid() const { return !m_storedType.isNull(); }

    QString descriptiveName() const;

    friend bool operator==(const QQmlJSRegisterContent &a, const QQmlJSRegisterContent &b)
    {
        return a.m_storedType == b.m_storedType && a.m_variant == b.m_variant
                && a.m_scope == b.m_scope && a.m_content == b.m_content;
    }

    friend bool operator!=(const QQmlJSRegisterContent &a, const QQmlJSRegisterContent &b)
    {
        return !(a == b);
    }

    bool isType() const { return m_content.index() == Type; }
    bool isProperty() const { return m_content.index() == Property; }
    bool isEnumeration() const { return m_content.index() == Enum; }
    bool isMethod() const { return m_content.index() == Method; }
    bool isImportNamespace() const { return m_content.index() == ImportNamespace; }
    bool isConversion() const { return m_content.index() == Conversion; }
    bool isList() const;

    bool isWritable() const;

    QQmlJSScope::ConstPtr storedType() const { return m_storedType; }
    QQmlJSScope::ConstPtr scopeType() const { return m_scope; }

    QQmlJSScope::ConstPtr type() const
    {
        return std::get<std::pair<QQmlJSScope::ConstPtr, int>>(m_content).first;
    }
    QQmlJSMetaProperty property() const
    {
        return std::get<PropertyLookup>(m_content).property;
    }
    int baseLookupIndex() const
    {
        return std::get<PropertyLookup>(m_content).baseLookupIndex;
    }
    int resultLookupIndex() const
    {
        switch (m_content.index()) {
        case Type:
            return std::get<std::pair<QQmlJSScope::ConstPtr, int>>(m_content).second;
        case Property:
            return std::get<PropertyLookup>(m_content).resultLookupIndex;
        default:
            return InvalidLookupIndex;
        }
    }
    QQmlJSMetaEnum enumeration() const
    {
        return std::get<std::pair<QQmlJSMetaEnum, QString>>(m_content).first;
    }
    QString enumMember() const
    {
        return std::get<std::pair<QQmlJSMetaEnum, QString>>(m_content).second;
    }
    QList<QQmlJSMetaMethod> method() const { return std::get<QList<QQmlJSMetaMethod>>(m_content); }
    uint importNamespace() const { return std::get<uint>(m_content); }

    QQmlJSScope::ConstPtr conversionResult() const
    {
        return std::get<ConvertedTypes>(m_content).result;
    }

    QQmlJSScope::ConstPtr conversionResultScope() const
    {
        return std::get<ConvertedTypes>(m_content).resultScope;
    }

    QList<QQmlJSScope::ConstPtr> conversionOrigins() const
    {
        return std::get<ConvertedTypes>(m_content).origins;
    }

    ContentVariant variant() const { return m_variant; }

    friend size_t qHash(const QQmlJSRegisterContent &registerContent, size_t seed = 0)
    {
        seed = qHashMulti(seed, registerContent.m_storedType, registerContent.m_content.index(),
                          registerContent.m_scope, registerContent.m_variant);
        switch (registerContent.m_content.index()) {
        case Type:
            return qHash(std::get<std::pair<QQmlJSScope::ConstPtr, int>>(registerContent.m_content),
                         seed);
        case Property:
            return qHash(std::get<PropertyLookup>(registerContent.m_content), seed);
        case Enum:
            return qHash(std::get<std::pair<QQmlJSMetaEnum, QString>>(registerContent.m_content),
                         seed);
        case Method:
            return qHash(std::get<QList<QQmlJSMetaMethod>>(registerContent.m_content), seed);
        case ImportNamespace:
            return qHash(std::get<uint>(registerContent.m_content), seed);
        case Conversion:
            return qHash(std::get<ConvertedTypes>(registerContent.m_content), seed);
        }

        Q_UNREACHABLE_RETURN(seed);
    }

    static QQmlJSRegisterContent create(const QQmlJSScope::ConstPtr &storedType,
                                        const QQmlJSScope::ConstPtr &type,
                                        int resultLookupIndex, ContentVariant variant,
                                        const QQmlJSScope::ConstPtr &scope = {});

    static QQmlJSRegisterContent create(const QQmlJSScope::ConstPtr &storedType,
                                        const QQmlJSMetaProperty &property,
                                        int baseLookupIndex, int resultLookupIndex,
                                        ContentVariant variant,
                                        const QQmlJSScope::ConstPtr &scope);

    static QQmlJSRegisterContent create(const QQmlJSScope::ConstPtr &storedType,
                                        const QQmlJSMetaEnum &enumeration,
                                        const QString &enumMember, ContentVariant variant,
                                        const QQmlJSScope::ConstPtr &scope);

    static QQmlJSRegisterContent create(const QQmlJSScope::ConstPtr &storedType,
                                        const QList<QQmlJSMetaMethod> &methods,
                                        ContentVariant variant,
                                        const QQmlJSScope::ConstPtr &scope);

    static QQmlJSRegisterContent create(const QQmlJSScope::ConstPtr &storedType,
                                        uint importNamespaceStringId, ContentVariant variant,
                                        const QQmlJSScope::ConstPtr &scope = {});

    static QQmlJSRegisterContent create(const QQmlJSScope::ConstPtr &storedType,
                                        const QList<QQmlJSScope::ConstPtr> &origins,
                                        const QQmlJSScope::ConstPtr &conversion,
                                        const QQmlJSScope::ConstPtr &conversionScope,
                                        ContentVariant variant,
                                        const QQmlJSScope::ConstPtr &scope = {});

    QQmlJSRegisterContent storedIn(const QQmlJSScope::ConstPtr &newStoredType) const
    {
        QQmlJSRegisterContent result = *this;
        result.m_storedType = newStoredType;
        return result;
    }

    QQmlJSRegisterContent castTo(const QQmlJSScope::ConstPtr &newContainedType) const
    {
        // This is not a conversion but a run time cast. It may result in null or undefined.
        QQmlJSRegisterContent result = *this;
        result.m_content = std::make_pair(newContainedType, result.resultLookupIndex());
        return result;
    }

private:
    enum ContentKind { Type, Property, Enum, Method, ImportNamespace, Conversion };

    struct ConvertedTypes
    {
        QList<QQmlJSScope::ConstPtr> origins;
        QQmlJSScope::ConstPtr result;
        QQmlJSScope::ConstPtr resultScope;

        friend size_t qHash(const ConvertedTypes &types, size_t seed = 0)
        {
            return qHashMulti(seed, types.origins, types.result, types.resultScope);
        }

        friend bool operator==(const ConvertedTypes &a, const ConvertedTypes &b)
        {
            return a.origins == b.origins && a.result == b.result && a.resultScope == b.resultScope;
        }

        friend bool operator!=(const ConvertedTypes &a, const ConvertedTypes &b)
        {
            return !(a == b);
        }
    };

    struct PropertyLookup
    {
        QQmlJSMetaProperty property;
        int baseLookupIndex = InvalidLookupIndex;
        int resultLookupIndex = InvalidLookupIndex;

        friend size_t qHash(const PropertyLookup &property, size_t seed = 0)
        {
            return qHashMulti(
                    seed, property.property, property.baseLookupIndex, property.resultLookupIndex);
        }

        friend bool operator==(const PropertyLookup &a, const PropertyLookup &b)
        {
            return a.baseLookupIndex == b.baseLookupIndex
                    && a.resultLookupIndex == b.resultLookupIndex
                    && a.property == b.property;
        }

        friend bool operator!=(const PropertyLookup &a, const PropertyLookup &b)
        {
            return !(a == b);
        }
    };

    using Content = std::variant<
        std::pair<QQmlJSScope::ConstPtr, int>,
        PropertyLookup,
        std::pair<QQmlJSMetaEnum, QString>,
        QList<QQmlJSMetaMethod>,
        uint,
        ConvertedTypes
    >;

    QQmlJSRegisterContent(const QQmlJSScope::ConstPtr &storedType,
                          const QQmlJSScope::ConstPtr &scope, ContentVariant variant)
        : m_storedType(storedType), m_scope(scope), m_variant(variant)
    {
    }

    QQmlJSScope::ConstPtr m_storedType;
    QQmlJSScope::ConstPtr m_scope;
    Content m_content;
    ContentVariant m_variant = Unknown;

    // TODO: Constant string/number/bool/enumval
};

QT_END_NAMESPACE

#endif // REGISTERCONTENT_H
