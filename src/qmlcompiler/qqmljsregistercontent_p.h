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

class Q_QMLCOMPILER_EXPORT QQmlJSRegisterContent
{
public:
    enum ContentVariant {
        ObjectById,
        TypeByName,
        Singleton,
        Script,
        MetaType,
        Extension,
        ScopeObject,

        JavaScriptGlobal,
        JavaScriptObject,
        JavaScriptParentScope,

        Property,
        Method,
        Enum,

        Attachment,
        ModulePrefix,

        MethodCall,

        ListValue,
        ListIterator,
        Builtin,
        Literal,
        Operation,
        Conversion,

        BaseType,
        Unknown,
    };

    enum { InvalidLookupIndex = -1 };

    QQmlJSRegisterContent();
    bool isValid() const { return !containedType().isNull(); }

    QString descriptiveName() const;
    QString containedTypeName() const;

    friend bool operator==(const QQmlJSRegisterContent &a, const QQmlJSRegisterContent &b)
    {
        return a.m_storedType == b.m_storedType && a.m_variant == b.m_variant
                && (a.m_scope ? (b.m_scope && *a.m_scope == *b.m_scope) : !b.m_scope)
                && a.m_content == b.m_content;
    }

    friend bool operator!=(const QQmlJSRegisterContent &a, const QQmlJSRegisterContent &b)
    {
        return !(a == b);
    }

    bool isType() const { return m_content.index() == size_t(Kind::Type); }
    bool isProperty() const { return m_content.index() == size_t(Kind::Property); }
    bool isEnumeration() const { return m_content.index() == size_t(Kind::Enum); }
    bool isMethod() const { return m_content.index() == size_t(Kind::Method); }
    bool isImportNamespace() const { return m_content.index() == size_t(Kind::ImportNamespace); }
    bool isConversion() const { return m_content.index() == size_t(Kind::Conversion); }
    bool isMethodCall() const { return m_content.index() == size_t(Kind::MethodCall); }
    bool isList() const;

    bool isWritable() const;
    bool isScopeObject() const;

    bool isJavaScriptReturnValue() const
    {
        return isMethodCall() && std::get<QQmlJSMetaMethod>(m_content).isJavaScriptFunction();
    }

    QQmlJSRegisterContent attacher() const;
    QQmlJSRegisterContent attachee() const;

    QQmlJSScope::ConstPtr storedType() const { return m_storedType; }
    QQmlJSScope::ConstPtr containedType() const;
    QQmlJSRegisterContent scopeType() const { return m_scope ? *m_scope : QQmlJSRegisterContent(); }

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
        switch (Kind(m_content.index())) {
        case Kind::Type:
            return std::get<std::pair<QQmlJSScope::ConstPtr, int>>(m_content).second;
        case Kind::Property:
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
    QList<QQmlJSMetaMethod> method() const
    {
        return std::get<std::pair<QList<QQmlJSMetaMethod>, QQmlJSScope::ConstPtr>>(
                       m_content).first;
    }
    QQmlJSScope::ConstPtr methodType() const
    {
        return std::get<std::pair<QList<QQmlJSMetaMethod>, QQmlJSScope::ConstPtr>>(
                       m_content).second;
    }
    uint importNamespace() const
    {
        return std::get<std::pair<uint, QQmlJSScope::ConstPtr>>(m_content).first;
    }
    QQmlJSScope::ConstPtr importNamespaceType() const
    {
        return std::get<std::pair<uint, QQmlJSScope::ConstPtr>>(m_content).second;
    }

    QQmlJSScope::ConstPtr conversionResult() const
    {
        return std::get<ConvertedTypes>(m_content).result;
    }

    QQmlJSRegisterContent conversionResultScope() const
    {
        const auto result = std::get<ConvertedTypes>(m_content).resultScope;
        return result ? *result : QQmlJSRegisterContent();
    }

    QList<QQmlJSRegisterContent> conversionOrigins() const
    {
        return std::get<ConvertedTypes>(m_content).origins;
    }

    QQmlJSMetaMethod methodCall() const
    {
        return std::get<QQmlJSMetaMethod>(m_content);
    }

    ContentVariant variant() const { return m_variant; }

    friend size_t qHash(const QQmlJSRegisterContent &registerContent, size_t seed = 0)
    {
        seed = qHashMulti(
                seed, registerContent.m_storedType, registerContent.m_content.index(),
                registerContent.m_variant);

        if (registerContent.m_scope)
            seed = qHash(registerContent.m_scope, seed);

        switch (Kind(registerContent.m_content.index())) {
        case Kind::Type:
            return qHash(std::get<std::pair<QQmlJSScope::ConstPtr, int>>(registerContent.m_content),
                         seed);
        case Kind::Property:
            return qHash(std::get<PropertyLookup>(registerContent.m_content), seed);
        case Kind::Enum:
            return qHash(std::get<std::pair<QQmlJSMetaEnum, QString>>(registerContent.m_content),
                         seed);
        case Kind::Method:
            return qHash(std::get<std::pair<QList<QQmlJSMetaMethod>, QQmlJSScope::ConstPtr>>(
                                 registerContent.m_content), seed);
        case Kind::ImportNamespace:
            return qHash(std::get<std::pair<uint, QQmlJSScope::ConstPtr>>(
                                 registerContent.m_content), seed);
        case Kind::Conversion:
            return qHash(std::get<ConvertedTypes>(registerContent.m_content), seed);
        case Kind::MethodCall:
            return qHash(std::get<QQmlJSMetaMethod>(registerContent.m_content), seed);
        }

        Q_UNREACHABLE_RETURN(seed);
    }

    static QQmlJSRegisterContent create(const QQmlJSScope::ConstPtr &type,
                                        int resultLookupIndex, ContentVariant variant,
                                        const QQmlJSRegisterContent &scope = {});

    static QQmlJSRegisterContent create(const QQmlJSMetaProperty &property,
                                        int baseLookupIndex, int resultLookupIndex,
                                        ContentVariant variant,
                                        const QQmlJSRegisterContent &scope);

    static QQmlJSRegisterContent create(const QQmlJSMetaEnum &enumeration,
                                        const QString &enumMember, ContentVariant variant,
                                        const QQmlJSRegisterContent &scope);

    static QQmlJSRegisterContent create(const QList<QQmlJSMetaMethod> &methods,
                                        const QQmlJSScope::ConstPtr &methodType,
                                        ContentVariant variant,
                                        const QQmlJSRegisterContent &scope);

    static QQmlJSRegisterContent create(const QQmlJSMetaMethod &method,
                                        const QQmlJSScope::ConstPtr &returnType,
                                        const QQmlJSRegisterContent &scope);

    static QQmlJSRegisterContent create(uint importNamespaceStringId,
                                        const QQmlJSScope::ConstPtr &importNamespaceType,
                                        ContentVariant variant,
                                        const QQmlJSRegisterContent &scope = {});

    static QQmlJSRegisterContent create(const QList<QQmlJSRegisterContent> &origins,
                                        const QQmlJSScope::ConstPtr &conversion,
                                        const QQmlJSRegisterContent &conversionScope,
                                        ContentVariant variant,
                                        const QQmlJSRegisterContent &scope = {});

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
    enum class Kind : size_t {
        Type, Property, Enum, Method, ImportNamespace, Conversion, MethodCall
    };

    struct ConvertedTypes
    {
        QList<QQmlJSRegisterContent> origins;
        QQmlJSScope::ConstPtr result;
        QSharedPointer<QQmlJSRegisterContent> resultScope;

        friend size_t qHash(const ConvertedTypes &types, size_t seed = 0)
        {
            return qHashMulti(seed, types.origins, types.result,
                              types.resultScope ? *types.resultScope : QQmlJSRegisterContent());
        }

        friend bool operator==(const ConvertedTypes &a, const ConvertedTypes &b)
        {
            return a.origins == b.origins && a.result == b.result
                    && (a.resultScope
                                ? (b.resultScope && *a.resultScope == *b.resultScope)
                                : !b.resultScope);
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
        std::pair<QList<QQmlJSMetaMethod>, QQmlJSScope::ConstPtr>,
        std::pair<uint, QQmlJSScope::ConstPtr>,
        ConvertedTypes,
        QQmlJSMetaMethod
    >;

    QQmlJSRegisterContent(const QQmlJSRegisterContent &scope, ContentVariant variant)
        : m_scope(QSharedPointer<QQmlJSRegisterContent>::create(scope)), m_variant(variant)
    {
    }

    QQmlJSScope::ConstPtr m_storedType;
    QSharedPointer<QQmlJSRegisterContent> m_scope;
    Content m_content;
    ContentVariant m_variant = Unknown;

    // TODO: Constant string/number/bool/enumval
};

QT_END_NAMESPACE

#endif // REGISTERCONTENT_H
