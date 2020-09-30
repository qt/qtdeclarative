/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#ifndef SCOPETREE_H
#define SCOPETREE_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "metatypes.h"

#include <QtQml/private/qqmljssourcelocation_p.h>

#include <QtCore/qset.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/qversionnumber.h>

#include <optional>

enum class ScopeType
{
    JSFunctionScope,
    JSLexicalScope,
    QMLScope
};

struct JavaScriptIdentifier
{
    enum Kind {
        Parameter,
        FunctionScoped,
        LexicalScoped,
        Injected
    };

    Kind kind = FunctionScoped;
    QQmlJS::SourceLocation location;
};

class ScopeTree
{
    Q_DISABLE_COPY_MOVE(ScopeTree)
public:
    using Ptr = QSharedPointer<ScopeTree>;
    using WeakPtr = QWeakPointer<ScopeTree>;
    using ConstPtr = QSharedPointer<const ScopeTree>;
    using WeakConstPtr = QWeakPointer<const ScopeTree>;

    enum class AccessSemantics {
        Reference,
        Value,
        None
    };

    enum Flag {
        Creatable = 0x1,
        Composite = 0x2,
        Singleton = 0x4
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAGS(Flags);

    class Export {
    public:
        Export() = default;
        Export(QString package, QString type, const QTypeRevision &version,
               int metaObjectRevision);

        bool isValid() const;

        int metaObjectRevision() const { return m_metaObjectRevision; }
        void setMetaObjectRevision(int metaObjectRevision)
        {
            m_metaObjectRevision = metaObjectRevision;
        }

        QString package() const { return m_package; }
        QString type() const { return m_type; }

    private:
        QString m_package;
        QString m_type;
        QTypeRevision m_version;
        int m_metaObjectRevision = 0;
    };

    static ScopeTree::Ptr create(ScopeType type = ScopeType::QMLScope,
                                 const ScopeTree::Ptr &parentScope = ScopeTree::Ptr());
    static ScopeTree::ConstPtr findCurrentQMLScope(const ScopeTree::ConstPtr &scope);

    ScopeTree::Ptr parentScope() const { return m_parentScope.toStrongRef(); }

    void insertJSIdentifier(const QString &name, const JavaScriptIdentifier &identifier);

    // inserts property as qml identifier as well as the corresponding
    void insertPropertyIdentifier(const MetaProperty &prop);

    bool isIdInCurrentScope(const QString &id) const;
    void addIdToAccessed(const QString &id, const QQmlJS::SourceLocation &location);
    void accessMember(const QString &name, const QString &parentType,
                      const QQmlJS::SourceLocation &location);

    ScopeType scopeType() const { return m_scopeType; }

    void addMethods(const QMultiHash<QString, MetaMethod> &methods) { m_methods.unite(methods); }
    void addMethod(const MetaMethod &method) { m_methods.insert(method.methodName(), method); }
    QMultiHash<QString, MetaMethod> methods() const { return m_methods; }

    void addEnum(const MetaEnum &fakeEnum) { m_enums.insert(fakeEnum.name(), fakeEnum); }
    QHash<QString, MetaEnum> enums() const { return m_enums; }

    QString fileName() const { return m_fileName; }
    void setFileName(const QString &file) { m_fileName = file; }

    // The name the type uses to refer to itself. Either C++ class name or base name of
    // QML file. isComposite tells us if this is a C++ or a QML name.
    QString internalName() const { return m_internalName; }
    void setInternalName(const QString &internalName) { m_internalName = internalName; }

    void addExport(const QString &name, const QString &package, const QTypeRevision &version);
    void setExportMetaObjectRevision(int exportIndex, int metaObjectRevision);
    QList<Export> exports() const { return m_exports; }

    // If isComposite(), this is the QML/JS name of the prototype. Otherwise it's the
    // relevant base class (in the hierarchy starting from QObject) of a C++ type.
    void setBaseTypeName(const QString &baseTypeName) { m_baseTypeName = baseTypeName; }
    QString baseTypeName() const { return m_baseTypeName; }
    ScopeTree::ConstPtr baseType() const { return m_baseType; }

    void addProperty(const MetaProperty &prop) { m_properties.insert(prop.propertyName(), prop); }
    QHash<QString, MetaProperty> properties() const { return m_properties; }

    QString defaultPropertyName() const { return m_defaultPropertyName; }
    void setDefaultPropertyName(const QString &name) { m_defaultPropertyName = name; }

    QString attachedTypeName() const { return m_attachedTypeName; }
    void setAttachedTypeName(const QString &name) { m_attachedTypeName = name; }
    ScopeTree::ConstPtr attachedType() const { return m_attachedType; }

    bool isSingleton() const { return m_flags & Singleton; }
    bool isCreatable() const { return m_flags & Creatable; }
    bool isComposite() const { return m_flags & Composite; }
    void setIsSingleton(bool v) { m_flags = v ? (m_flags | Singleton) : (m_flags & ~Singleton); }
    void setIsCreatable(bool v) { m_flags = v ? (m_flags | Creatable) : (m_flags & ~Creatable); }
    void setIsComposite(bool v) { m_flags = v ? (m_flags | Composite) : (m_flags & ~Composite); }

    void setAccessSemantics(AccessSemantics semantics) { m_semantics = semantics; }
    AccessSemantics accessSemantics() const { return m_semantics; }

    struct FieldMember
    {
        QString m_name;
        QString m_parentType;
        QQmlJS::SourceLocation m_location;
    };

    QVector<QVector<FieldMember>> memberAccessChains() const
    {
        return m_memberAccessChains;
    }

    bool isIdInCurrentQMlScopes(const QString &id) const;
    bool isIdInCurrentJSScopes(const QString &id) const;
    bool isIdInjectedFromSignal(const QString &id) const;

    std::optional<JavaScriptIdentifier> findJSIdentifier(const QString &id) const;

    QVector<ScopeTree::Ptr> childScopes() const
    {
        return m_childScopes;
    }

    void resolveTypes(const QHash<QString, ConstPtr> &contextualTypes);

private:
    ScopeTree(ScopeType type, const ScopeTree::Ptr &parentScope = ScopeTree::Ptr());

    QHash<QString, JavaScriptIdentifier> m_jsIdentifiers;

    QMultiHash<QString, MetaMethod> m_methods;
    QHash<QString, MetaProperty> m_properties;
    QHash<QString, MetaEnum> m_enums;

    QVector<QVector<FieldMember>> m_memberAccessChains;

    QVector<ScopeTree::Ptr> m_childScopes;
    ScopeTree::WeakPtr m_parentScope;

    QString m_fileName;
    QString m_internalName;
    QString m_baseTypeName;
    ScopeTree::WeakConstPtr m_baseType;

    ScopeType m_scopeType = ScopeType::QMLScope;
    QList<Export> m_exports;

    QString m_defaultPropertyName;
    QString m_attachedTypeName;
    ScopeTree::WeakConstPtr m_attachedType;

    Flags m_flags;
    AccessSemantics m_semantics = AccessSemantics::Reference;
};

#endif // SCOPETREE_H
