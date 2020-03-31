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
#include "componentversion.h"

#include <QtQml/private/qqmljssourcelocation_p.h>

#include <QtCore/qset.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>

enum class ScopeType
{
    JSFunctionScope,
    JSLexicalScope,
    QMLScope
};

struct MethodUsage
{
    MetaMethod method;
    QQmlJS::SourceLocation loc;
    bool hasMultilineHandlerBody;
};

class ScopeTree
{
    Q_DISABLE_COPY_MOVE(ScopeTree)
public:
    using Ptr = QSharedPointer<ScopeTree>;
    using WeakPtr = QWeakPointer<ScopeTree>;
    using ConstPtr = QSharedPointer<const ScopeTree>;
    using WeakConstPtr = QWeakPointer<const ScopeTree>;

    class Export {
    public:
        Export() = default;
        Export(QString package, QString type, const ComponentVersion &version,
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
        ComponentVersion m_version;
        int m_metaObjectRevision = 0;
    };

    static ScopeTree::Ptr create(ScopeType type = ScopeType::QMLScope, const QString &name = QString(),
                                 const ScopeTree::Ptr &parentScope = ScopeTree::Ptr());
    static ScopeTree::ConstPtr findCurrentQMLScope(const ScopeTree::ConstPtr &scope);

    ScopeTree::Ptr parentScope() const { return m_parentScope.toStrongRef(); }

    void insertJSIdentifier(const QString &id, ScopeType scope);
    void insertSignalIdentifier(const QString &id, const MetaMethod &method,
                                const QQmlJS::SourceLocation &loc, bool hasMultilineHandlerBody);
    // inserts property as qml identifier as well as the corresponding
    void insertPropertyIdentifier(const MetaProperty &prop);
    void addUnmatchedSignalHandler(const QString &handler,
                                   const QQmlJS::SourceLocation &location);

    bool isIdInCurrentScope(const QString &id) const;
    void addIdToAccessed(const QString &id, const QQmlJS::SourceLocation &location);
    void accessMember(const QString &name, const QString &parentType,
                      const QQmlJS::SourceLocation &location);

    bool isVisualRootScope() const;
    QString name() const { return m_name; }

    ScopeType scopeType() const { return m_scopeType; }

    void addMethods(const QHash<QString, MetaMethod> &methods) { m_methods.insert(methods); }
    void addMethod(const MetaMethod &method) { m_methods.insert(method.methodName(), method); }
    QHash<QString, MetaMethod> methods() const { return m_methods; }

    void addEnum(const MetaEnum &fakeEnum) { m_enums.insert(fakeEnum.name(), fakeEnum); }
    QHash<QString, MetaEnum> enums() const { return m_enums; }

    QString fileName() const { return m_fileName; }
    void setFileName(const QString &file) { m_fileName = file; }

    QString className() const { return m_className; }
    void setClassName(const QString &name) { m_className = name; }

    void addExport(const QString &name, const QString &package, const ComponentVersion &version);
    void setExportMetaObjectRevision(int exportIndex, int metaObjectRevision);
    QList<Export> exports() const { return m_exports; }

    void setSuperclassName(const QString &superclass) { m_superName = superclass; }
    QString superclassName() const { return m_superName; }

    void addProperty(const MetaProperty &prop) { m_properties.insert(prop.propertyName(), prop); }
    QHash<QString, MetaProperty> properties() const { return m_properties; }
    void updateParentProperty(const ScopeTree::ConstPtr &scope);

    QString defaultPropertyName() const { return m_defaultPropertyName; }
    void setDefaultPropertyName(const QString &name) { m_defaultPropertyName = name; }

    QString attachedTypeName() const { return m_attachedTypeName; }
    void setAttachedTypeName(const QString &name) { m_attachedTypeName = name; }

    bool isSingleton() const { return m_isSingleton; }
    bool isCreatable() const { return m_isCreatable; }
    bool isComposite() const { return m_isComposite; }
    void setIsSingleton(bool value) { m_isSingleton = value; }
    void setIsCreatable(bool value) { m_isCreatable = value; }
    void setIsComposite(bool value) { m_isSingleton = value; }

    struct FieldMember
    {
        QString m_name;
        QString m_parentType;
        QQmlJS::SourceLocation m_location;
    };

    QVector<QPair<QString, QQmlJS::SourceLocation>> unmatchedSignalHandlers() const
    {
        return m_unmatchedSignalHandlers;
    }

    QVector<QVector<FieldMember>> memberAccessChains() const
    {
        return m_memberAccessChains;
    }

    bool isIdInCurrentQMlScopes(const QString &id) const;
    bool isIdInCurrentJSScopes(const QString &id) const;
    bool isIdInjectedFromSignal(const QString &id) const;

    QVector<ScopeTree::Ptr> childScopes() const
    {
        return m_childScopes;
    }

    QMultiHash<QString, MethodUsage> injectedSignalIdentifiers() const
    {
        return m_injectedSignalIdentifiers;
    }

private:
    ScopeTree(ScopeType type, const QString &name = QString(),
              const ScopeTree::Ptr &parentScope = ScopeTree::Ptr());

    QSet<QString> m_jsIdentifiers;
    QMultiHash<QString, MethodUsage> m_injectedSignalIdentifiers;

    QHash<QString, MetaMethod> m_methods;
    QHash<QString, MetaProperty> m_properties;
    QHash<QString, MetaEnum> m_enums;

    QVector<QVector<FieldMember>> m_memberAccessChains;
    QVector<QPair<QString, QQmlJS::SourceLocation>> m_unmatchedSignalHandlers;

    QVector<ScopeTree::Ptr> m_childScopes;
    ScopeTree::WeakPtr m_parentScope;

    QString m_fileName;
    QString m_name;
    QString m_className;
    QString m_superName;

    ScopeType m_scopeType = ScopeType::QMLScope;
    QList<Export> m_exports;

    QString m_defaultPropertyName;
    QString m_attachedTypeName;
    bool m_isSingleton = false;
    bool m_isCreatable = true;
    bool m_isComposite = false;
};

#endif // SCOPETREE_H
