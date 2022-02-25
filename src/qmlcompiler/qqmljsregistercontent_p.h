/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

#include <variant>

QT_BEGIN_NAMESPACE

class QQmlJSRegisterContent
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
        JavaScriptObjectProperty,

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
        Builtin,
        Unknown,
    };

    QQmlJSRegisterContent() = default;
    bool isValid() const { return !m_storedType.isNull(); }

    QString descriptiveName() const;

    friend bool operator==(const QQmlJSRegisterContent &a, const QQmlJSRegisterContent &b);
    friend bool operator!=(const QQmlJSRegisterContent &a, const QQmlJSRegisterContent &b)
    {
        return !(a == b);
    }

    bool isType() const { return m_content.index() == Type; }
    bool isProperty() const { return m_content.index() == Property; }
    bool isEnumeration() const { return m_content.index() == Enum; }
    bool isMethod() const { return m_content.index() == Method; }
    bool isImportNamespace() const { return m_content.index() == ImportNamespace; }
    bool isList() const;

    bool isWritable() const;

    QQmlJSScope::ConstPtr storedType() const { return m_storedType; }
    QQmlJSScope::ConstPtr scopeType() const { return m_scope; }

    QQmlJSScope::ConstPtr type() const { return std::get<QQmlJSScope::ConstPtr>(m_content); }
    QQmlJSMetaProperty property() const { return std::get<QQmlJSMetaProperty>(m_content); }
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

    ContentVariant variant() const { return m_variant; }

    friend size_t qHash(const QQmlJSRegisterContent &registerContent, size_t seed = 0)
    {
        seed = qHashMulti(seed, registerContent.m_storedType, registerContent.m_content.index(),
                          registerContent.m_scope, registerContent.m_variant);
        switch (registerContent.m_content.index()) {
        case Type:
            return qHash(std::get<QQmlJSScope::ConstPtr>(registerContent.m_content), seed);
        case Property:
            return qHash(std::get<QQmlJSMetaProperty>(registerContent.m_content), seed);
        case Enum:
            return qHash(std::get<std::pair<QQmlJSMetaEnum, QString>>(registerContent.m_content),
                         seed);
        case Method:
            return qHash(std::get<QList<QQmlJSMetaMethod>>(registerContent.m_content), seed);
        case ImportNamespace:
            return qHash(std::get<uint>(registerContent.m_content), seed);
        }

        Q_UNREACHABLE();
        return seed;
    }

    static QQmlJSRegisterContent create(const QQmlJSScope::ConstPtr &storedType,
                                        const QQmlJSScope::ConstPtr &type, ContentVariant variant,
                                        const QQmlJSScope::ConstPtr &scope = {});

    static QQmlJSRegisterContent create(const QQmlJSScope::ConstPtr &storedType,
                                        const QQmlJSMetaProperty &property, ContentVariant variant,
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

    QQmlJSRegisterContent storedIn(const QQmlJSScope::ConstPtr &newStoredType) const
    {
        QQmlJSRegisterContent result = *this;
        result.m_storedType = newStoredType;
        return result;
    }

private:
    enum ContentKind { Type, Property, Enum, Method, ImportNamespace };

    using Content = std::variant<
        QQmlJSScope::ConstPtr,
        QQmlJSMetaProperty,
        std::pair<QQmlJSMetaEnum, QString>,
        QList<QQmlJSMetaMethod>,
        uint
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
