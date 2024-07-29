// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QQMLJSIMPORTEDMEMBERSVISITOR_P_H
#define QQMLJSIMPORTEDMEMBERSVISITOR_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#include <private/qqmljscontextualtypes_p.h>
#include <private/qtqmlcompilerexports_p.h>

#include "qqmljsannotation_p.h"
#include "qqmljsimporter_p.h"
#include "qqmljslogger_p.h"
#include "qqmljsscope_p.h"
#include "qqmljsscopesbyid_p.h"

#include <QtCore/qvariant.h>
#include <QtCore/qstack.h>

#include <private/qqmljsast_p.h>
#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qv4compileddata_p.h>

#include <functional>

QT_BEGIN_NAMESPACE

namespace QQmlJS::Dom {
class QQmlDomAstCreatorWithQQmlJSScope;
}

struct QQmlJSResourceFileMapper;
class Q_QMLCOMPILER_PRIVATE_EXPORT QQmlJSImportVisitor : public QQmlJS::AST::Visitor
{
public:
    QQmlJSImportVisitor();
    QQmlJSImportVisitor(const QQmlJSScope::Ptr &target,
                        QQmlJSImporter *importer, QQmlJSLogger *logger,
                        const QString &implicitImportDirectory,
                        const QStringList &qmldirFiles = QStringList());
    ~QQmlJSImportVisitor();

    using QQmlJS::AST::Visitor::endVisit;
    using QQmlJS::AST::Visitor::postVisit;
    using QQmlJS::AST::Visitor::preVisit;
    using QQmlJS::AST::Visitor::visit;

    QQmlJSScope::Ptr result() const { return m_exportedRootScope; }

    const QQmlJSLogger *logger() const { return m_logger; }
    QQmlJSLogger *logger() { return m_logger; }

    QQmlJSImporter::ImportedTypes imports() const { return m_rootScopeImports; }
    QQmlJSScopesById addressableScopes() const { return m_scopesById; }
    QHash<QQmlJS::SourceLocation, QQmlJSMetaSignalHandler> signalHandlers() const
    {
        return m_signalHandlers;
    }
    QSet<QQmlJSScope::ConstPtr> literalScopesToCheck() const { return m_literalScopesToCheck; }
    QList<QQmlJSScope::ConstPtr> qmlTypes() const { return m_qmlTypes; }
    QHash<QV4::CompiledData::Location, QQmlJSScope::ConstPtr> scopesBylocation() const
    {
        return m_scopesByIrLocation;
    }

    static QString implicitImportDirectory(
            const QString &localFile, QQmlJSResourceFileMapper *mapper);

    // ### should this be restricted?
    QQmlJSImporter *importer() { return m_importer; }
    const QQmlJSImporter *importer() const { return m_importer; }

    struct UnfinishedBinding
    {
        QQmlJSScope::Ptr owner;
        std::function<QQmlJSMetaPropertyBinding()> create;
        QQmlJSScope::BindingTargetSpecifier specifier = QQmlJSScope::SimplePropertyTarget;
    };

protected:
    // Linter warnings, we might want to move this at some point
    bool visit(QQmlJS::AST::StringLiteral *) override;

    bool visit(QQmlJS::AST::ExpressionStatement *ast) override;
    void endVisit(QQmlJS::AST::ExpressionStatement *ast) override;

    bool visit(QQmlJS::AST::UiProgram *) override;
    void endVisit(QQmlJS::AST::UiProgram *) override;
    bool visit(QQmlJS::AST::UiObjectDefinition *) override;
    void endVisit(QQmlJS::AST::UiObjectDefinition *) override;
    bool visit(QQmlJS::AST::UiInlineComponent *) override;
    void endVisit(QQmlJS::AST::UiInlineComponent *) override;
    bool visit(QQmlJS::AST::UiPublicMember *) override;
    void endVisit(QQmlJS::AST::UiPublicMember *) override;
    bool visit(QQmlJS::AST::UiRequired *required) override;
    bool visit(QQmlJS::AST::UiScriptBinding *) override;
    void endVisit(QQmlJS::AST::UiScriptBinding *) override;
    bool visit(QQmlJS::AST::UiArrayBinding *) override;
    void endVisit(QQmlJS::AST::UiArrayBinding *) override;
    bool visit(QQmlJS::AST::UiEnumDeclaration *uied) override;
    bool visit(QQmlJS::AST::FunctionExpression *fexpr) override;
    void endVisit(QQmlJS::AST::FunctionExpression *) override;
    bool visit(QQmlJS::AST::UiSourceElement *) override;
    bool visit(QQmlJS::AST::FunctionDeclaration *fdecl) override;
    void endVisit(QQmlJS::AST::FunctionDeclaration *) override;
    bool visit(QQmlJS::AST::ClassExpression *ast) override;
    void endVisit(QQmlJS::AST::ClassExpression *) override;
    bool visit(QQmlJS::AST::UiImport *import) override;
    bool visit(QQmlJS::AST::UiPragma *pragma) override;
    bool visit(QQmlJS::AST::ClassDeclaration *ast) override;
    void endVisit(QQmlJS::AST::ClassDeclaration *ast) override;
    bool visit(QQmlJS::AST::ForStatement *ast) override;
    void endVisit(QQmlJS::AST::ForStatement *ast) override;
    bool visit(QQmlJS::AST::ForEachStatement *ast) override;
    void endVisit(QQmlJS::AST::ForEachStatement *ast) override;
    bool visit(QQmlJS::AST::Block *ast) override;
    void endVisit(QQmlJS::AST::Block *ast) override;
    bool visit(QQmlJS::AST::CaseBlock *ast) override;
    void endVisit(QQmlJS::AST::CaseBlock *ast) override;
    bool visit(QQmlJS::AST::Catch *ast) override;
    void endVisit(QQmlJS::AST::Catch *ast) override;
    bool visit(QQmlJS::AST::WithStatement *withStatement) override;
    void endVisit(QQmlJS::AST::WithStatement *ast) override;

    bool visit(QQmlJS::AST::VariableDeclarationList *vdl) override;
    bool visit(QQmlJS::AST::FormalParameterList *fpl) override;

    bool visit(QQmlJS::AST::UiObjectBinding *uiob) override;
    void endVisit(QQmlJS::AST::UiObjectBinding *uiob) override;

    bool visit(QQmlJS::AST::ExportDeclaration *exp) override;
    void endVisit(QQmlJS::AST::ExportDeclaration *exp) override;

    bool visit(QQmlJS::AST::ESModule *module) override;
    void endVisit(QQmlJS::AST::ESModule *module) override;

    bool visit(QQmlJS::AST::Program *program) override;
    void endVisit(QQmlJS::AST::Program *program) override;

    void endVisit(QQmlJS::AST::FieldMemberExpression *) override;
    bool visit(QQmlJS::AST::IdentifierExpression *idexp) override;

    bool visit(QQmlJS::AST::PatternElement *) override;

    void throwRecursionDepthError() override;

    QString m_implicitImportDirectory;
    QStringList m_qmldirFiles;
    QQmlJSScope::Ptr m_currentScope;
    const QQmlJSScope::Ptr m_exportedRootScope;
    QQmlJSImporter *m_importer = nullptr;
    QQmlJSLogger *m_logger = nullptr;

    using RootDocumentNameType = QQmlJSScope::RootDocumentNameType;
    using InlineComponentNameType = QQmlJSScope::InlineComponentNameType;
    using InlineComponentOrDocumentRootName = QQmlJSScope::RootDocumentNameType;
    QQmlJSScope::InlineComponentOrDocumentRootName m_currentRootName =
            QQmlJSScope::RootDocumentNameType();
    bool m_nextIsInlineComponent = false;
    bool m_rootIsSingleton = false;
    QQmlJSScope::Ptr m_savedBindingOuterScope;
    QQmlJSScope::ConstPtr m_globalScope;
    QQmlJSScopesById m_scopesById;
    QQmlJSImporter::ImportedTypes m_rootScopeImports;
    QList<QQmlJSScope::ConstPtr> m_qmlTypes;

    // We need to record the locations as IR locations because those contain less data.
    // This way we can look up objects by IR location later.
    QHash<QV4::CompiledData::Location, QQmlJSScope::ConstPtr> m_scopesByIrLocation;

    // Maps all qmlNames to the source location of their import
    QMultiHash<QString, QQmlJS::SourceLocation> m_importTypeLocationMap;
    // Maps all static modules to the source location of their import
    QMultiHash<QString, QQmlJS::SourceLocation> m_importStaticModuleLocationMap;
    // Contains all import source locations (could be extracted from above but that is expensive)
    QSet<QQmlJS::SourceLocation> m_importLocations;
    // A set of all types that have been used during type resolution
    QSet<QString> m_usedTypes;

    QList<UnfinishedBinding> m_bindings;

    // stores JS functions and Script bindings per scope (only the name). mimics
    // the content of QmlIR::Object::functionsAndExpressions
    QHash<QQmlJSScope::ConstPtr, QList<QString>> m_functionsAndExpressions;

    struct FunctionOrExpressionIdentifier
    {
        QQmlJSScope::ConstPtr scope;
        QString name;
        friend bool operator==(const FunctionOrExpressionIdentifier &x,
                               const FunctionOrExpressionIdentifier &y)
        {
            return x.scope == y.scope && x.name == y.name;
        }
        friend bool operator!=(const FunctionOrExpressionIdentifier &x,
                               const FunctionOrExpressionIdentifier &y)
        {
            return !(x == y);
        }
        friend size_t qHash(const FunctionOrExpressionIdentifier &x, size_t seed = 0)
        {
            return qHashMulti(seed, x.scope, x.name);
        }
    };

    // tells whether last-processed UiScriptBinding is truly a script binding
    bool m_thisScriptBindingIsJavaScript = false;
    QStack<FunctionOrExpressionIdentifier> m_functionStack;
    // stores the number of functions inside each function
    QHash<FunctionOrExpressionIdentifier, int> m_innerFunctions;
    QQmlJSMetaMethod::RelativeFunctionIndex
    addFunctionOrExpression(const QQmlJSScope::ConstPtr &scope, const QString &name);
    void forgetFunctionExpression(const QString &name);
    int synthesizeCompilationUnitRuntimeFunctionIndices(const QQmlJSScope::Ptr &scope,
                                                        int count) const;
    void populateRuntimeFunctionIndicesForDocument() const;

    void enterEnvironment(QQmlJSScope::ScopeType type, const QString &name,
                          const QQmlJS::SourceLocation &location);
    // Finds an existing scope before attempting to create a new one. Returns \c
    // true if the scope already exists and \c false if the new scope is created
    bool enterEnvironmentNonUnique(QQmlJSScope::ScopeType type, const QString &name,
                                   const QQmlJS::SourceLocation &location);
    void leaveEnvironment();

    // A set of types that have not been resolved but have been used during the
    // AST traversal
    QSet<QQmlJSScope::ConstPtr> m_unresolvedTypes;
    template<typename ErrorHandler>
    bool isTypeResolved(const QQmlJSScope::ConstPtr &type, ErrorHandler handle)
    {
        if (type->isFullyResolved())
            return true;

        // Note: ignore duplicates, but only after we are certain that the type
        // is still unresolved
        if (m_unresolvedTypes.contains(type))
            return false;

        m_unresolvedTypes.insert(type);

        handle(type);
        return false;
    }
    bool isTypeResolved(const QQmlJSScope::ConstPtr &type);

    QVector<QQmlJSAnnotation> parseAnnotations(QQmlJS::AST::UiAnnotationList *list);
    void setAllBindings();
    void addDefaultProperties();
    void processDefaultProperties();
    void processPropertyBindings();
    void checkRequiredProperties();
    void processPropertyTypes();
    void processMethodTypes();
    void processPropertyBindingObjects();
    void flushPendingSignalParameters();

    QQmlJSScope::ConstPtr scopeById(const QString &id, const QQmlJSScope::ConstPtr &current);

    void breakInheritanceCycles(const QQmlJSScope::Ptr &scope);
    void checkDeprecation(const QQmlJSScope::ConstPtr &scope);
    void checkGroupedAndAttachedScopes(QQmlJSScope::ConstPtr scope);
    bool rootScopeIsValid() const { return m_exportedRootScope->sourceLocation().isValid(); }

    enum class BindingExpressionParseResult { Invalid, Script, Literal, Translation };
    BindingExpressionParseResult parseBindingExpression(const QString &name,
                                                        const QQmlJS::AST::Statement *statement);
    bool isImportPrefix(QString prefix) const;

    // Used to temporarily store annotations for functions and generators wrapped in UiSourceElements
    QVector<QQmlJSAnnotation> m_pendingMethodAnnotations;

    struct PendingPropertyType
    {
        QQmlJSScope::Ptr scope;
        QString name;
        QQmlJS::SourceLocation location;
    };

    struct PendingMethodTypeAnnotations
    {
        QQmlJSScope::Ptr scope;
        QString methodName;
        // This keeps type annotations' locations in order (parameters then return type).
        // If an annotation is not present, it is represented by an invalid source location.
        QVarLengthArray<QQmlJS::SourceLocation, 3> locations;
    };

    struct PendingPropertyObjectBinding
    {
        QQmlJSScope::Ptr scope;
        QQmlJSScope::Ptr childScope;
        QString name;
        QQmlJS::SourceLocation location;
        bool onToken;
    };

    struct RequiredProperty
    {
        QQmlJSScope::Ptr scope;
        QString name;
        QQmlJS::SourceLocation location;
    };

    /*!
        Utility wrapper that adds visibility scope to the data.

        This wrapper becomes useful for binding processing where we need to know
        both the property (or signal handler) owner and the scope in which the
        binding is executed (the "visibility" scope).

        As visibility scope (and data) does not typically have sufficient
        information about a proper source location of that data, the location
        also has to be provided to simplify the error reporting.
    */
    template<typename T>
    struct WithVisibilityScope
    {
        QQmlJSScope::Ptr visibilityScope;
        QQmlJS::SourceLocation dataLocation;
        T data;
    };

    QHash<QQmlJSScope::Ptr, QVector<QQmlJSScope::Ptr>> m_pendingDefaultProperties;
    QVector<PendingPropertyType> m_pendingPropertyTypes;
    QVector<PendingMethodTypeAnnotations> m_pendingMethodTypeAnnotations;
    QVector<PendingPropertyObjectBinding> m_pendingPropertyObjectBindings;
    QVector<RequiredProperty> m_requiredProperties;
    QVector<QQmlJSScope::Ptr> m_objectBindingScopes;
    QVector<QQmlJSScope::Ptr> m_objectDefinitionScopes;

    QHash<QQmlJSScope::Ptr, QVector<WithVisibilityScope<QString>>> m_propertyBindings;

    QHash<QQmlJS::SourceLocation, QQmlJSMetaSignalHandler> m_signalHandlers;
    QSet<QQmlJSScope::ConstPtr> m_literalScopesToCheck;
    QQmlJS::SourceLocation m_pendingSignalHandler;

private:
    void checkSignal(
            const QQmlJSScope::ConstPtr &signalScope, const QQmlJS::SourceLocation &location,
            const QString &handlerName, const QStringList &handlerParameters);
    void importBaseModules();
    void resolveAliasesAndIds();
    void handleIdDeclaration(QQmlJS::AST::UiScriptBinding *scriptBinding);

    void visitFunctionExpressionHelper(QQmlJS::AST::FunctionExpression *fexpr);
    void processImportWarnings(
            const QString &what,
            const QQmlJS::SourceLocation &srcLocation = QQmlJS::SourceLocation());
    void addImportWithLocation(const QString &name, const QQmlJS::SourceLocation &loc);
    void populateCurrentScope(QQmlJSScope::ScopeType type, const QString &name,
                              const QQmlJS::SourceLocation &location);
    void enterRootScope(QQmlJSScope::ScopeType type, const QString &name,
                           const QQmlJS::SourceLocation &location);

    void importFromHost(const QString &path, const QString &prefix,
                        const QQmlJS::SourceLocation &location);
    void importFromQrc(const QString &path, const QString &prefix,
                       const QQmlJS::SourceLocation &location);

public:
    friend class QQmlJS::Dom::QQmlDomAstCreatorWithQQmlJSScope;
};

QT_END_NAMESPACE

#endif // QQMLJSIMPORTEDMEMBERSVISITOR_P_H
