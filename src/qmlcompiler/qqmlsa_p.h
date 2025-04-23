// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLSA_P_H
#define QQMLSA_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <qtqmlcompilerexports.h>

#include <private/qqmljslogger_p.h>
#include <QtCore/qset.h>
#include "qqmljsmetatypes_p.h"

#include <map>
#include <unordered_map>
#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE

class QQmlJSTypeResolver;
struct QQmlJSTypePropagator;
class QQmlJSImportVisitor;

namespace QQmlSA {

class Bindings;
class GenericPassPrivate;
class PassManager;

enum class Flag {
    Creatable = 0x1,
    Composite = 0x2,
    Singleton = 0x4,
    Script = 0x8,
    CustomParser = 0x10,
    Array = 0x20,
    InlineComponent = 0x40,
    WrappedInImplicitComponent = 0x80,
    HasBaseTypeError = 0x100,
    HasExtensionNamespace = 0x200,
    IsListProperty = 0x400,
};

struct PropertyPassInvocation
{
    QString property;
    std::shared_ptr<QQmlSA::PropertyPass> pass;
    bool allowInheritance = true;
};

class BindingsPrivate
{
    friend class QT_PREPEND_NAMESPACE(QQmlJSMetaPropertyBinding);
    Q_DECLARE_PUBLIC(QQmlSA::Binding::Bindings)

public:
    explicit BindingsPrivate(QQmlSA::Binding::Bindings *);
    BindingsPrivate(QQmlSA::Binding::Bindings *, const BindingsPrivate &);
    BindingsPrivate(QQmlSA::Binding::Bindings *, BindingsPrivate &&);
    ~BindingsPrivate() = default;

    QMultiHash<QString, Binding>::const_iterator constBegin() const;
    QMultiHash<QString, Binding>::const_iterator constEnd() const;

    static QQmlSA::Binding::Bindings
    createBindings(const QMultiHash<QString, QQmlJSMetaPropertyBinding> &);
    static QQmlSA::Binding::Bindings
            createBindings(QPair<QMultiHash<QString, QQmlJSMetaPropertyBinding>::const_iterator,
                                 QMultiHash<QString, QQmlJSMetaPropertyBinding>::const_iterator>);

private:
    QMultiHash<QString, Binding> m_bindings;
    QQmlSA::Binding::Bindings *q_ptr;
};

class BindingPrivate
{
    friend class QT_PREPEND_NAMESPACE(QQmlJSMetaPropertyBinding);
    Q_DECLARE_PUBLIC(Binding)

public:
    explicit BindingPrivate(Binding *);
    BindingPrivate(Binding *, const BindingPrivate &);

    static BindingPrivate *get(Binding *binding) { return binding->d_func(); }
    static const BindingPrivate *get(const Binding *binding) { return binding->d_func(); }

    static QQmlSA::Binding createBinding(const QQmlJSMetaPropertyBinding &);
    static QQmlJSMetaPropertyBinding binding(QQmlSA::Binding &binding);
    static const QQmlJSMetaPropertyBinding binding(const QQmlSA::Binding &binding);

    void setPropertyName(QString name) { m_binding.setPropertyName(name); }

    Element bindingScope() const { return m_bindingScope; }
    void setBindingScope(Element bindingScope) { m_bindingScope = bindingScope; }

    bool isAttached() const { return m_isAttached; }
    void setIsAttached(bool isAttached) { m_isAttached = isAttached; }

private:
    QQmlJSMetaPropertyBinding m_binding;
    Element m_bindingScope;
    Binding *q_ptr = nullptr;
    bool m_isAttached = false;
};

bool isRegularBindingType(BindingType type);

class MethodPrivate
{
    friend class QT_PREPEND_NAMESPACE(QQmlJSMetaMethod);
    Q_DECLARE_PUBLIC(Method)

public:
    explicit MethodPrivate(Method *);
    MethodPrivate(Method *, const MethodPrivate &);

    QString methodName() const;
    QQmlSA::SourceLocation sourceLocation() const;
    MethodType methodType() const;

    static QQmlSA::Method createMethod(const QQmlJSMetaMethod &);
    static QQmlJSMetaMethod method(const QQmlSA::Method &);

private:
    QQmlJSMetaMethod m_method;
    Method *q_ptr;
};

class MethodsPrivate
{
    friend class QT_PREPEND_NAMESPACE(QQmlJSMetaMethod);
    Q_DECLARE_PUBLIC(QQmlSA::Method::Methods)

public:
    explicit MethodsPrivate(QQmlSA::Method::Methods *);
    MethodsPrivate(QQmlSA::Method::Methods *, const MethodsPrivate &);
    MethodsPrivate(QQmlSA::Method::Methods *, MethodsPrivate &&);
    ~MethodsPrivate() = default;

    QMultiHash<QString, Method>::const_iterator constBegin() const;
    QMultiHash<QString, Method>::const_iterator constEnd() const;

    static QQmlSA::Method::Methods createMethods(const QMultiHash<QString, QQmlJSMetaMethod> &);

private:
    QMultiHash<QString, Method> m_methods;
    QQmlSA::Method::Methods *q_ptr;
};

class PropertyPrivate
{
    friend class QT_PREPEND_NAMESPACE(QQmlJSMetaProperty);
    Q_DECLARE_PUBLIC(QQmlSA::Property)

public:
    explicit PropertyPrivate(Property *);
    PropertyPrivate(Property *, const PropertyPrivate &);
    PropertyPrivate(Property *, PropertyPrivate &&);
    ~PropertyPrivate() = default;

    QString typeName() const;
    bool isValid() const;
    bool isReadonly() const;
    QQmlSA::Element type() const;

    static QQmlJSMetaProperty property(const QQmlSA::Property &property);
    static QQmlSA::Property createProperty(const QQmlJSMetaProperty &);

private:
    QQmlJSMetaProperty m_property;
    QQmlSA::Property *q_ptr;
};

class Q_QMLCOMPILER_EXPORT PassManagerPrivate
{
    friend class QT_PREPEND_NAMESPACE(QQmlJSScope);

public:
    Q_DISABLE_COPY_MOVE(PassManagerPrivate)

    friend class GenericPass;
    PassManagerPrivate(QQmlJSImportVisitor *visitor, QQmlJSTypeResolver *resolver)
        : m_visitor(visitor), m_typeResolver(resolver)
    {
    }

    static PassManagerPrivate *get(PassManager *manager) { return manager->d_func(); }
    static const PassManagerPrivate *get(const PassManager *manager) { return manager->d_func(); }
    static PassManager *createPassManager(QQmlJSImportVisitor *visitor, QQmlJSTypeResolver *resolver)
    {
        PassManager *result = new PassManager();
        result->d_ptr = std::make_unique<PassManagerPrivate>(visitor, resolver);
        return result;
    }
    static void deletePassManager(PassManager *q) { delete q; }

    void registerElementPass(std::unique_ptr<ElementPass> pass);
    bool registerPropertyPass(std::shared_ptr<PropertyPass> pass, QAnyStringView moduleName,
                              QAnyStringView typeName,
                              QAnyStringView propertyName = QAnyStringView(),
                              bool allowInheritance = true);
    void analyze(const Element &root);

    bool hasImportedModule(QAnyStringView name) const;

    static QQmlJSImportVisitor *visitor(const QQmlSA::PassManager &);
    static QQmlJSTypeResolver *resolver(const QQmlSA::PassManager &);


    QSet<PropertyPass *> findPropertyUsePasses(const QQmlSA::Element &element,
                                               const QString &propertyName);

    void analyzeWrite(const QQmlSA::Element &element, const QString &propertyName,
                      const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                      const QQmlSA::SourceLocation &location);
    void analyzeRead(const QQmlSA::Element &element, const QString &propertyName,
                     const QQmlSA::Element &readScope, const QQmlSA::SourceLocation &location);
    void analyzeCall(const QQmlSA::Element &element, const QString &propertyName,
                     const QQmlSA::Element &readScope, const QQmlSA::SourceLocation &location);
    void analyzeBinding(const QQmlSA::Element &element, const QQmlSA::Element &value,
                        const QQmlSA::SourceLocation &location);

    void addBindingSourceLocations(const QQmlSA::Element &element,
                                   const QQmlSA::Element &scope = QQmlSA::Element(),
                                   const QString prefix = QString(), bool isAttached = false);

    std::vector<std::unique_ptr<ElementPass>> m_elementPasses;
    std::multimap<QString, PropertyPassInvocation> m_propertyPasses;
    std::unordered_map<quint32, Binding> m_bindingsByLocation;
    QQmlJSImportVisitor *m_visitor = nullptr;
    QQmlJSTypeResolver *m_typeResolver = nullptr;
};

class FixSuggestionPrivate
{
    Q_DECLARE_PUBLIC(FixSuggestion)
    friend class QT_PREPEND_NAMESPACE(QQmlJSFixSuggestion);

public:
    explicit FixSuggestionPrivate(FixSuggestion *);
    FixSuggestionPrivate(FixSuggestion *, const QString &fixDescription,
                         const QQmlSA::SourceLocation &location, const QString &replacement);
    FixSuggestionPrivate(FixSuggestion *, const FixSuggestionPrivate &);
    FixSuggestionPrivate(FixSuggestion *, FixSuggestionPrivate &&);
    ~FixSuggestionPrivate() = default;

    QString fixDescription() const;
    QQmlSA::SourceLocation location() const;
    QString replacement() const;

    void setFileName(const QString &);
    QString fileName() const;

    void setHint(const QString &);
    QString hint() const;

    void setAutoApplicable(bool autoApplicable = true);
    bool isAutoApplicable() const;

    static QQmlJSFixSuggestion &fixSuggestion(QQmlSA::FixSuggestion &);
    static const QQmlJSFixSuggestion &fixSuggestion(const QQmlSA::FixSuggestion &);

private:
    QQmlJSFixSuggestion m_fixSuggestion;
    QQmlSA::FixSuggestion *q_ptr;
};

} // namespace QQmlSA

QT_END_NAMESPACE

#endif
