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

#ifndef QQMLJSSCOPE_P_H
#define QQMLJSSCOPE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include "qqmljsmetatypes_p.h"
#include "qdeferredpointer_p.h"

#include <QtQml/private/qqmljssourcelocation_p.h>

#include <QtCore/qset.h>
#include <QtCore/qhash.h>
#include <QtCore/qstring.h>
#include <QtCore/qversionnumber.h>

#include <optional>

QT_BEGIN_NAMESPACE

class QQmlJSImporter;
class QQmlJSScope;

template<>
class QDeferredFactory<QQmlJSScope>
{
public:
    QDeferredFactory() = default;

    QDeferredFactory(QQmlJSImporter *importer, const QString &filePath) :
        m_filePath(filePath), m_importer(importer)
    {}

    QQmlJSScope create() const;

    bool isValid() const
    {
        return !m_filePath.isEmpty() && m_importer != nullptr;
    }

private:
    QString m_filePath;
    QQmlJSImporter *m_importer = nullptr;
};

class QQmlJSScope
{
    Q_DISABLE_COPY(QQmlJSScope)
public:
    QQmlJSScope(QQmlJSScope &&) = default;
    QQmlJSScope &operator=(QQmlJSScope &&) = default;

    using Ptr = QDeferredSharedPointer<QQmlJSScope>;
    using WeakPtr = QDeferredWeakPointer<QQmlJSScope>;
    using ConstPtr = QDeferredSharedPointer<const QQmlJSScope>;
    using WeakConstPtr = QDeferredWeakPointer<const QQmlJSScope>;

    enum ScopeType
    {
        JSFunctionScope,
        JSLexicalScope,
        QMLScope,
        GroupedPropertyScope
    };

    enum class AccessSemantics {
        Reference,
        Value,
        None,
        Sequence
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
        Export(QString package, QString type, const QTypeRevision &version);

        bool isValid() const;

        QString package() const { return m_package; }
        QString type() const { return m_type; }
        QTypeRevision version() const { return m_version; }

    private:
        QString m_package;
        QString m_type;
        QTypeRevision m_version;
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

    static QQmlJSScope::Ptr create(ScopeType type = QQmlJSScope::QMLScope,
                                 const QQmlJSScope::Ptr &parentScope = QQmlJSScope::Ptr());
    static QQmlJSScope::ConstPtr findCurrentQMLScope(const QQmlJSScope::ConstPtr &scope);

    QQmlJSScope::Ptr parentScope() const { return m_parentScope.toStrongRef(); }

    void insertJSIdentifier(const QString &name, const JavaScriptIdentifier &identifier);

    // inserts property as qml identifier as well as the corresponding
    void insertPropertyIdentifier(const QQmlJSMetaProperty &prop);

    bool isIdInCurrentScope(const QString &id) const;

    ScopeType scopeType() const { return m_scopeType; }

    void addOwnMethod(const QQmlJSMetaMethod &method) { m_methods.insert(method.methodName(), method); }
    QMultiHash<QString, QQmlJSMetaMethod> ownMethods() const { return m_methods; }
    QList<QQmlJSMetaMethod> ownMethods(const QString &name) const { return m_methods.values(name); }
    bool hasOwnMethod(const QString &name) const { return m_methods.contains(name); }

    bool hasMethod(const QString &name) const;
    QList<QQmlJSMetaMethod> methods(const QString &name) const;

    void addEnumeration(const QQmlJSMetaEnum &enumeration)
    {
        m_enumerations.insert(enumeration.name(), enumeration);
    }
    QHash<QString, QQmlJSMetaEnum> enumerations() const { return m_enumerations; }
    QQmlJSMetaEnum enumeration(const QString &name) const { return m_enumerations.value(name); }
    bool hasEnumeration(const QString &name) const { return m_enumerations.contains(name); }

    QString fileName() const { return m_fileName; }
    void setFileName(const QString &file) { m_fileName = file; }

    // The name the type uses to refer to itself. Either C++ class name or base name of
    // QML file. isComposite tells us if this is a C++ or a QML name.
    QString internalName() const { return m_internalName; }
    void setInternalName(const QString &internalName) { m_internalName = internalName; }

    void addExport(const QString &name, const QString &package, const QTypeRevision &version);
    QList<Export> exports() const { return m_exports; }

    // If isComposite(), this is the QML/JS name of the prototype. Otherwise it's the
    // relevant base class (in the hierarchy starting from QObject) of a C++ type.
    void setBaseTypeName(const QString &baseTypeName) { m_baseTypeName = baseTypeName; }
    QString baseTypeName() const { return m_baseTypeName; }
    QQmlJSScope::ConstPtr baseType() const { return m_baseType; }

    void addOwnProperty(const QQmlJSMetaProperty &prop) { m_properties.insert(prop.propertyName(), prop); }
    QHash<QString, QQmlJSMetaProperty> ownProperties() const { return m_properties; }
    QQmlJSMetaProperty ownProperty(const QString &name) const { return m_properties.value(name); }
    bool hasOwnProperty(const QString &name) const { return m_properties.contains(name); }

    bool hasProperty(const QString &name) const;
    QQmlJSMetaProperty property(const QString &name) const;

    QString defaultPropertyName() const { return m_defaultPropertyName; }
    void setDefaultPropertyName(const QString &name) { m_defaultPropertyName = name; }

    QString attachedTypeName() const { return m_attachedTypeName; }
    void setAttachedTypeName(const QString &name) { m_attachedTypeName = name; }
    QQmlJSScope::ConstPtr attachedType() const { return m_attachedType; }

    QString valueTypeName() const { return m_valueTypeName; }
    void setValueTypeName(const QString &name) { m_valueTypeName = name; }
    QQmlJSScope::ConstPtr valueType() const { return m_valueType; }

    bool isSingleton() const { return m_flags & Singleton; }
    bool isCreatable() const { return m_flags & Creatable; }
    bool isComposite() const { return m_flags & Composite; }
    void setIsSingleton(bool v) { m_flags = v ? (m_flags | Singleton) : (m_flags & ~Singleton); }
    void setIsCreatable(bool v) { m_flags = v ? (m_flags | Creatable) : (m_flags & ~Creatable); }
    void setIsComposite(bool v) { m_flags = v ? (m_flags | Composite) : (m_flags & ~Composite); }

    void setAccessSemantics(AccessSemantics semantics) { m_semantics = semantics; }
    AccessSemantics accessSemantics() const { return m_semantics; }

    bool isIdInCurrentQMlScopes(const QString &id) const;
    bool isIdInCurrentJSScopes(const QString &id) const;
    bool isIdInjectedFromSignal(const QString &id) const;

    std::optional<JavaScriptIdentifier> findJSIdentifier(const QString &id) const;

    QVector<QQmlJSScope::Ptr> childScopes() const
    {
        return m_childScopes;
    }

    void resolveTypes(const QHash<QString, ConstPtr> &contextualTypes);
    void resolveGroupedScopes();

    void setSourceLocation(const QQmlJS::SourceLocation &sourceLocation)
    {
        m_sourceLocation = sourceLocation;
    }

    QQmlJS::SourceLocation sourceLocation() const
    {
        return m_sourceLocation;
    }

    static QQmlJSScope::ConstPtr nonCompositeBaseType(const QQmlJSScope::ConstPtr &type)
    {
        for (QQmlJSScope::ConstPtr base = type; base; base = base->baseType()) {
            if (!base->isComposite())
                return base;
        }
        return {};
    }

private:
    QQmlJSScope(ScopeType type, const QQmlJSScope::Ptr &parentScope = QQmlJSScope::Ptr());

    QHash<QString, JavaScriptIdentifier> m_jsIdentifiers;

    QMultiHash<QString, QQmlJSMetaMethod> m_methods;
    QHash<QString, QQmlJSMetaProperty> m_properties;
    QHash<QString, QQmlJSMetaEnum> m_enumerations;

    QVector<QQmlJSScope::Ptr> m_childScopes;
    QQmlJSScope::WeakPtr m_parentScope;

    QString m_fileName;
    QString m_internalName;
    QString m_baseTypeName;
    QQmlJSScope::WeakConstPtr m_baseType;

    ScopeType m_scopeType = QMLScope;
    QList<Export> m_exports;

    QString m_defaultPropertyName;
    QString m_attachedTypeName;
    QQmlJSScope::WeakConstPtr m_attachedType;

    QString m_valueTypeName;
    QQmlJSScope::WeakConstPtr m_valueType;

    Flags m_flags;
    AccessSemantics m_semantics = AccessSemantics::Reference;

    QQmlJS::SourceLocation m_sourceLocation;
};

QT_END_NAMESPACE

#endif // QQMLJSSCOPE_P_H
