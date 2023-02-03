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

#include <private/qqmljsscope_p.h>
#include <private/qqmljslogger_p.h>
#include <QtCore/qset.h>

#include <map>
#include <unordered_map>
#include <vector>
#include <memory>

QT_BEGIN_NAMESPACE

class QQmlJSTypeResolver;
struct QQmlJSTypePropagator;
class QQmlJSImportVisitor;

namespace QQmlSA {

// ### FIXME: Replace with a proper PIMPL'd type
using Element = QQmlJSScope::ConstPtr;
using FixSuggestion = QQmlJSFixSuggestion;

class GenericPassPrivate;
class PassManager;

class Q_QMLCOMPILER_EXPORT GenericPass
{
public:
    Q_DISABLE_COPY_MOVE(GenericPass)
    GenericPass(PassManager *manager);
    virtual ~GenericPass();

    void emitWarning(QAnyStringView diagnostic, LoggerWarningId id,
                     QQmlJS::SourceLocation srcLocation = QQmlJS::SourceLocation());
    void emitWarning(QAnyStringView diagnostic, LoggerWarningId id,
                     QQmlJS::SourceLocation srcLocation, const FixSuggestion &fix);

    Element resolveTypeInFileScope(QAnyStringView typeName);
    Element resolveType(QAnyStringView moduleName, QAnyStringView typeName); // #### TODO: revisions
    Element resolveLiteralType(const QQmlJSMetaPropertyBinding &binding);

    Element resolveIdToElement(QAnyStringView id, const Element &context);
    QString resolveElementToId(const Element &element, const Element &context);

    QString sourceCode(QQmlJS::SourceLocation location);

private:
    std::unique_ptr<GenericPassPrivate> d; // PIMPL might be overkill
};

class Q_QMLCOMPILER_EXPORT ElementPass : public GenericPass
{
public:
    ElementPass(PassManager *manager) : GenericPass(manager) { }

    virtual bool shouldRun(const Element &element);
    virtual void run(const Element &element) = 0;
};

class Q_QMLCOMPILER_EXPORT PropertyPass : public GenericPass
{
public:
    PropertyPass(PassManager *manager);

    virtual void onBinding(const QQmlSA::Element &element, const QString &propertyName,
                           const QQmlJSMetaPropertyBinding &binding,
                           const QQmlSA::Element &bindingScope, const QQmlSA::Element &value);
    virtual void onRead(const QQmlSA::Element &element, const QString &propertyName,
                        const QQmlSA::Element &readScope, QQmlJS::SourceLocation location);
    virtual void onWrite(const QQmlSA::Element &element, const QString &propertyName,
                         const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                         QQmlJS::SourceLocation location);
};

class Q_QMLCOMPILER_EXPORT LintPlugin
{
public:
    LintPlugin() = default;
    virtual ~LintPlugin() = default;

    Q_DISABLE_COPY_MOVE(LintPlugin)

    virtual void registerPasses(PassManager *manager, const Element &rootElement) = 0;
};

// ### FIXME: Make this (at least partially) private again as soon as possible
class Q_QMLCOMPILER_EXPORT PassManager
{
public:
    Q_DISABLE_COPY_MOVE(PassManager)

    friend class GenericPass;
    PassManager(QQmlJSImportVisitor *visitor, QQmlJSTypeResolver *resolver)
        : m_visitor(visitor), m_typeResolver(resolver)
    {
        Q_UNUSED(m_typeResolver);
    }
    void registerElementPass(std::unique_ptr<ElementPass> pass);
    bool registerPropertyPass(std::shared_ptr<PropertyPass> pass, QAnyStringView moduleName,
                              QAnyStringView typeName,
                              QAnyStringView propertyName = QAnyStringView(),
                              bool allowInheritance = true);
    void analyze(const Element &root);

    bool hasImportedModule(QAnyStringView name) const;

    bool isCategoryEnabled(LoggerWarningId category) const;
    void setCategoryEnabled(LoggerWarningId category, bool enabled = true);

private:
    friend struct ::QQmlJSTypePropagator;

    QSet<PropertyPass *> findPropertyUsePasses(const QQmlSA::Element &element,
                                               const QString &propertyName);

    void analyzeWrite(const QQmlSA::Element &element, QString propertyName,
                      const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                      QQmlJS::SourceLocation location);
    void analyzeRead(const QQmlSA::Element &element, QString propertyName,
                     const QQmlSA::Element &readScope, QQmlJS::SourceLocation location);
    void analyzeBinding(const QQmlSA::Element &element, const QQmlSA::Element &value,
                        QQmlJS::SourceLocation location);

    struct BindingInfo
    {
        QString fullPropertyName;
        QQmlJSMetaPropertyBinding binding;
        QQmlSA::Element bindingScope;
        bool isAttached;
    };

    struct PropertyPassInfo
    {
        QStringList properties;
        std::shared_ptr<PropertyPass> pass;
        bool allowInheritance = true;
    };

    void addBindingSourceLocations(const QQmlSA::Element &element,
                                   const QQmlSA::Element &scope = QQmlSA::Element(),
                                   const QString prefix = QString(), bool isAttached = false);

    std::vector<std::unique_ptr<ElementPass>> m_elementPasses;
    std::multimap<QString, PropertyPassInfo> m_propertyPasses;
    std::unordered_map<quint32, BindingInfo> m_bindingsByLocation;
    QQmlJSImportVisitor *m_visitor;
    QQmlJSTypeResolver *m_typeResolver;
};

class Q_QMLCOMPILER_EXPORT DebugElementPass : public ElementPass
{
    void run(const Element &element) override;
};

class Q_QMLCOMPILER_EXPORT DebugPropertyPass : public QQmlSA::PropertyPass
{
public:
    DebugPropertyPass(QQmlSA::PassManager *manager);

    void onRead(const QQmlSA::Element &element, const QString &propertyName,
                const QQmlSA::Element &readScope, QQmlJS::SourceLocation location) override;
    void onBinding(const QQmlSA::Element &element, const QString &propertyName,
                   const QQmlJSMetaPropertyBinding &binding, const QQmlSA::Element &bindingScope,
                   const QQmlSA::Element &value) override;
    void onWrite(const QQmlSA::Element &element, const QString &propertyName,
                 const QQmlSA::Element &value, const QQmlSA::Element &writeScope,
                 QQmlJS::SourceLocation location) override;
};
}

#define QmlLintPluginInterface_iid "org.qt-project.Qt.Qml.SA.LintPlugin/1.0"

Q_DECLARE_INTERFACE(QQmlSA::LintPlugin, QmlLintPluginInterface_iid)

QT_END_NAMESPACE

#endif
