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
#include "qqmljsannotation_p.h"

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
        GroupedPropertyScope,
        AttachedPropertyScope,
        EnumScope
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
        Singleton = 0x4,
        Script = 0x8,
        CustomParser = 0x10,
        Array = 0x20,
        InlineComponent = 0x40,
        WrappedInImplicitComponent = 0x80
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

    QQmlJSScope::Ptr parentScope()
    {
        return m_parentScope.toStrongRef();
    }

    QQmlJSScope::ConstPtr parentScope() const
    {
        return QQmlJSScope::WeakConstPtr(m_parentScope).toStrongRef();
    }

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

    void addOwnEnumeration(const QQmlJSMetaEnum &enumeration) { m_enumerations.insert(enumeration.name(), enumeration); }
    QHash<QString, QQmlJSMetaEnum> ownEnumerations() const { return m_enumerations; }
    QQmlJSMetaEnum ownEnumeration(const QString &name) const { return m_enumerations.value(name); }
    bool hasOwnEnumeration(const QString &name) const { return m_enumerations.contains(name); }

    bool hasEnumeration(const QString &name) const;
    bool hasEnumerationKey(const QString &name) const;
    QQmlJSMetaEnum enumeration(const QString &name) const;

    void setAnnotations(const QList<QQmlJSAnnotation> &annotation) { m_annotations = std::move(annotation); }
    const QList<QQmlJSAnnotation> &annotations() const { return m_annotations; }

    QString fileName() const { return m_fileName; }
    void setFileName(const QString &file) { m_fileName = file; }

    // The name the type uses to refer to itself. Either C++ class name or base name of
    // QML file. isComposite tells us if this is a C++ or a QML name.
    QString internalName() const { return m_internalName; }
    void setInternalName(const QString &internalName) { m_internalName = internalName; }

    bool causesImplicitComponentWrapping() const;
    bool isComponentRootElement() const;

    void addExport(const QString &name, const QString &package, const QTypeRevision &version);
    QList<Export> exports() const { return m_exports; }

    void setInterfaceNames(const QStringList& interfaces) { m_interfaceNames = interfaces; }
    QStringList interfaceNames() const { return m_interfaceNames; }

    bool hasInterface(const QString &name) const;
    bool hasOwnInterface(const QString &name) const { return m_interfaceNames.contains(name); }

    // If isComposite(), this is the QML/JS name of the prototype. Otherwise it's the
    // relevant base class (in the hierarchy starting from QObject) of a C++ type.
    void setBaseTypeName(const QString &baseTypeName) { m_baseTypeName = baseTypeName; }
    QString baseTypeName() const { return m_baseTypeName; }
    QQmlJSScope::ConstPtr baseType() const { return m_baseType; }
    void clearBaseType() { m_baseType = QQmlJSScope::WeakConstPtr(); }

    void addOwnProperty(const QQmlJSMetaProperty &prop) { m_properties.insert(prop.propertyName(), prop); }
    QHash<QString, QQmlJSMetaProperty> ownProperties() const { return m_properties; }
    QQmlJSMetaProperty ownProperty(const QString &name) const { return m_properties.value(name); }
    bool hasOwnProperty(const QString &name) const { return m_properties.contains(name); }

    bool hasProperty(const QString &name) const;
    QQmlJSMetaProperty property(const QString &name) const;

    void setPropertyLocallyRequired(const QString &name, bool isRequired);
    bool isPropertyRequired(const QString &name) const;
    bool isPropertyLocallyRequired(const QString &name) const;

    void addOwnPropertyBinding(const QQmlJSMetaPropertyBinding &binding)
    {
        m_propertyBindings.insert(binding.propertyName(), binding);
    }
    QHash<QString, QQmlJSMetaPropertyBinding> ownPropertyBindings() const
    {
        return m_propertyBindings;
    }
    QQmlJSMetaPropertyBinding ownPropertyBinding(const QString &name) const
    {
        return m_propertyBindings.value(name);
    }
    bool hasOwnPropertyBinding(const QString &name) const
    {
        return m_propertyBindings.contains(name);
    }

    bool hasPropertyBinding(const QString &name) const;
    QQmlJSMetaPropertyBinding propertyBinding(const QString &name) const;

    static QQmlJSScope::ConstPtr ownerOfProperty(const QQmlJSScope::ConstPtr &self,
                                                 const QString &name);

    bool isResolved() const;
    bool isFullyResolved() const;

    QString defaultPropertyName() const { return m_defaultPropertyName; }
    void setDefaultPropertyName(const QString &name) { m_defaultPropertyName = name; }

    QString parentPropertyName() const { return m_parentPropertyName; }
    void setParentPropertyName(const QString &name) { m_parentPropertyName = name; }

    QString ownAttachedTypeName() const { return m_attachedTypeName; }
    void setOwnAttachedTypeName(const QString &name) { m_attachedTypeName = name; }
    QQmlJSScope::ConstPtr ownAttachedType() const { return m_attachedType; }

    QString attachedTypeName() const;
    QQmlJSScope::ConstPtr attachedType() const;

    QString extensionTypeName() const { return m_extensionTypeName; }
    void setExtensionTypeName(const QString &name) { m_extensionTypeName =  name; }
    QQmlJSScope::ConstPtr extensionType() const { return m_extensionType; }

    QString valueTypeName() const { return m_valueTypeName; }
    void setValueTypeName(const QString &name) { m_valueTypeName = name; }
    QQmlJSScope::ConstPtr valueType() const { return m_valueType; }

    bool isSingleton() const { return m_flags & Singleton; }
    bool isCreatable() const { return m_flags & Creatable; }
    bool isComposite() const { return m_flags & Composite; }
    bool isScript() const { return m_flags & Script; }
    bool hasCustomParser() const { return m_flags & CustomParser; }
    bool isArrayScope() const { return m_flags & Array; }
    bool isInlineComponent() const { return m_flags & InlineComponent; }
    void setIsSingleton(bool v) { m_flags.setFlag(Singleton, v); }
    void setIsCreatable(bool v) { m_flags.setFlag(Creatable, v); }
    void setIsComposite(bool v) { m_flags.setFlag(Composite, v); }
    void setIsScript(bool v) { m_flags.setFlag(Script, v); }
    void setHasCustomParser(bool v)
    {
        m_flags.setFlag(CustomParser, v);;
    }
    void setIsArrayScope(bool v) { m_flags.setFlag(Array, v); }
    void setIsInlineComponent(bool v) { m_flags.setFlag(InlineComponent, v); }
    void setIsWrappedInImplicitComponent(bool v) { m_flags.setFlag(WrappedInImplicitComponent, v); }

    void setAccessSemantics(AccessSemantics semantics) { m_semantics = semantics; }
    AccessSemantics accessSemantics() const { return m_semantics; }

    bool isIdInCurrentQmlScopes(const QString &id) const;
    bool isIdInCurrentJSScopes(const QString &id) const;
    bool isIdInjectedFromSignal(const QString &id) const;

    std::optional<JavaScriptIdentifier> findJSIdentifier(const QString &id) const;

    QVector<QQmlJSScope::Ptr> childScopes()
    {
        return m_childScopes;
    }

    QVector<QQmlJSScope::ConstPtr> childScopes() const
    {
        QVector<QQmlJSScope::ConstPtr> result;
        result.reserve(m_childScopes.size());
        for (const auto &child : m_childScopes)
            result.append(child);
        return result;
    }

    static void resolveTypes(const QQmlJSScope::Ptr &self,
                             const QHash<QString, ConstPtr> &contextualTypes,
                             QSet<QString> *usedTypes = nullptr);
    static void resolveNonEnumTypes(const QQmlJSScope::Ptr &self,
                                    const QHash<QString, ConstPtr> &contextualTypes,
                                    QSet<QString> *usedTypes = nullptr);
    static void resolveEnums(const QQmlJSScope::Ptr &self,
                             const QHash<QString, QQmlJSScope::ConstPtr> &contextualTypes,
                             QSet<QString> *usedTypes = nullptr);

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

    /*!
      \internal
      Checks whether \a otherScope is the same type as this.

      In addition to checking whether the scopes are identical, we also cover duplicate scopes with
      the same internal name.
    */
    bool isSameType(const QQmlJSScope::ConstPtr &otherScope) const
    {
        return this == otherScope.get()
                || (!this->internalName().isEmpty()
                    && this->internalName() == otherScope->internalName());
    }

    bool inherits(const QQmlJSScope::ConstPtr &base) const
    {
        for (const QQmlJSScope *scope = this; scope; scope = scope->baseType().get()) {
            if (scope->isSameType(base))
                return true;
        }
        return false;
    }

    /*!
      \internal
      Checks whether \a derived type can be assigned to this type. Returns \c
      true if the type hierarchy of \a derived contains a type equal to this.

      \note Assigning \a derived to "QVariant" or "QJSValue" is always possible and
      the function returns \c true in this case. In addition any "QObject" based \a derived type
      can be assigned to a this type if that type is derived from "QQmlComponent".
    */
    bool canAssign(const QQmlJSScope::ConstPtr &derived) const;

    /*!
      \internal
      Checks whether this type or its parents have a custom parser.
    */
    bool isInCustomParserParent() const;

private:
    QQmlJSScope(ScopeType type, const QQmlJSScope::Ptr &parentScope = QQmlJSScope::Ptr());

    static QQmlJSScope::ConstPtr
    findType(const QString &name, const QHash<QString, QQmlJSScope::ConstPtr> &contextualTypes,
             QSet<QString> *usedTypes = nullptr);
    static void resolveType(const QQmlJSScope::Ptr &self,
                            const QHash<QString, ConstPtr> &contextualTypes,
                            QSet<QString> *usedTypes);
    static void updateChildScope(const QQmlJSScope::Ptr &childScope, const QQmlJSScope::Ptr &self,
                                 const QHash<QString, QQmlJSScope::ConstPtr> &contextualTypes,
                                 QSet<QString> *usedTypes);

    QHash<QString, JavaScriptIdentifier> m_jsIdentifiers;

    QMultiHash<QString, QQmlJSMetaMethod> m_methods;
    QHash<QString, QQmlJSMetaProperty> m_properties;
    QHash<QString, QQmlJSMetaPropertyBinding> m_propertyBindings;
    QHash<QString, QQmlJSMetaEnum> m_enumerations;

    QVector<QQmlJSAnnotation> m_annotations;
    QVector<QQmlJSScope::Ptr> m_childScopes;
    QQmlJSScope::WeakPtr m_parentScope;

    QString m_fileName;
    QString m_internalName;
    QString m_baseTypeName;
    QQmlJSScope::WeakConstPtr m_baseType;

    ScopeType m_scopeType = QMLScope;
    QList<Export> m_exports;
    QStringList m_interfaceNames;

    QString m_defaultPropertyName;
    QString m_parentPropertyName;
    QString m_attachedTypeName;
    QStringList m_requiredPropertyNames;
    QQmlJSScope::WeakConstPtr m_attachedType;

    QString m_valueTypeName;
    QQmlJSScope::WeakConstPtr m_valueType;

    QString m_extensionTypeName;
    QQmlJSScope::WeakConstPtr m_extensionType;

    Flags m_flags;
    AccessSemantics m_semantics = AccessSemantics::Reference;

    QQmlJS::SourceLocation m_sourceLocation;
};

QT_END_NAMESPACE

#endif // QQMLJSSCOPE_P_H
