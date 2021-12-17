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

#include "qqmljsscope_p.h"
#include "qqmljsannotation_p.h"
#include "qqmljslogger_p.h"
#include "qqmljsscopesbyid_p.h"

#include <private/qqmljsast_p.h>
#include <private/qqmljsdiagnosticmessage_p.h>
#include <private/qqmljsimporter_p.h>
#include <private/qv4compileddata_p.h>

QT_BEGIN_NAMESPACE

struct QQmlJSResourceFileMapper;
class QQmlJSImportVisitor : public QQmlJS::AST::Visitor
{
public:
    QQmlJSImportVisitor(QQmlJSImporter *importer, const QString &implicitImportDirectory,
                        const QStringList &qmltypesFiles = QStringList(), const QString &fileName = QString(), const QString &code = QString(), bool silent = true);

    QQmlJSScope::Ptr result() const;

    // TODO: Should be superseded by accessing the logger instead
    QList<QQmlJS::DiagnosticMessage> errors() const { return m_logger.warnings() + m_logger.errors(); }

    QQmlJSLogger &logger() { return m_logger; }

    QQmlJSImporter::ImportedTypes imports() const { return m_rootScopeImports; }
    QQmlJSScopesById addressableScopes() const { return m_scopesById; }
    QHash<QV4::CompiledData::Location, QQmlJSScope::ConstPtr> scopesBylocation() const
    {
        return m_scopesByIrLocation;
    }

    static QString implicitImportDirectory(
            const QString &localFile, QQmlJSResourceFileMapper *mapper);

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

    void throwRecursionDepthError() override;

    QString m_implicitImportDirectory;
    QString m_code;
    QString m_filePath;
    QString m_rootId;
    QStringView m_inlineComponentName;
    bool m_nextIsInlineComponent = false;
    QStringList m_qmltypesFiles;
    QQmlJSScope::Ptr m_currentScope;
    QQmlJSScope::Ptr m_savedBindingOuterScope;
    QQmlJSScope::Ptr m_exportedRootScope;
    QQmlJSScope::ConstPtr m_globalScope;
    QQmlJSScopesById m_scopesById;
    QQmlJSImporter::ImportedTypes m_rootScopeImports;

    // We need to record the locations as IR locations because those contain less data.
    // This way we can look up objects by IR location later.
    QHash<QV4::CompiledData::Location, QQmlJSScope::ConstPtr> m_scopesByIrLocation;

    // Maps all qmlNames to the source location of their import
    QMultiHash<QString, QQmlJS::SourceLocation> m_importTypeLocationMap;
    // Contains all import source locations (could be extracted from above but that is expensive)
    QSet<QQmlJS::SourceLocation> m_importLocations;
    // A set of all types that have been used during type resolution
    QSet<QString> m_usedTypes;

    QQmlJSImporter *m_importer;

    void enterEnvironment(QQmlJSScope::ScopeType type, const QString &name,
                          const QQmlJS::SourceLocation &location);
    // Finds an existing scope before attempting to create a new one. Returns \c
    // true if the scope already exists and \c false if the new scope is created
    bool enterEnvironmentNonUnique(QQmlJSScope::ScopeType type, const QString &name,
                                   const QQmlJS::SourceLocation &location);
    void leaveEnvironment();

    QVector<QQmlJSAnnotation> parseAnnotations(QQmlJS::AST::UiAnnotationList *list);
    void addDefaultProperties();
    void processDefaultProperties();
    void processPropertyBindings();
    void checkRequiredProperties();
    void processPropertyTypes();
    void processPropertyBindingObjects();
    void checkSignals();
    void flushPendingSignalParameters();

    QQmlJSScope::ConstPtr scopeById(const QString &id, const QQmlJSScope::ConstPtr &current);

    void breakInheritanceCycles(const QQmlJSScope::Ptr &scope);
    void checkDeprecation(const QQmlJSScope::ConstPtr &scope);
    void checkGroupedAndAttachedScopes(QQmlJSScope::ConstPtr scope);

    QQmlJSLogger m_logger;

    // Used to temporarily store annotations for functions and generators wrapped in UiSourceElements
    QVector<QQmlJSAnnotation> m_pendingMethodAnnotations;

    struct PendingPropertyType
    {
        QQmlJSScope::Ptr scope;
        QString name;
        QQmlJS::SourceLocation location;
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
    QVector<PendingPropertyObjectBinding> m_pendingPropertyObjectBindings;
    QVector<RequiredProperty> m_requiredProperties;
    QVector<QQmlJSScope::Ptr> m_objectBindingScopes;
    QVector<QQmlJSScope::Ptr> m_objectDefinitionScopes;

    QHash<QQmlJSScope::Ptr, QVector<WithVisibilityScope<QString>>> m_propertyBindings;
    QHash<QQmlJSScope::Ptr, QVector<WithVisibilityScope<QPair<QString, QStringList>>>> m_signals;

    QHash<QQmlJS::SourceLocation, QQmlJSMetaSignalHandler> m_signalHandlers;
    QQmlJS::SourceLocation m_pendingSignalHandler;

    struct OutstandingConnection
    {
        QString targetName;
        QQmlJSScope::Ptr scope;
        QQmlJS::AST::UiObjectDefinition *uiod;
    };

    QVarLengthArray<OutstandingConnection, 3>
            m_outstandingConnections; // Connections whose target we have not encountered

private:
    void importBaseModules();
    void resolveAliases();

    void visitFunctionExpressionHelper(QQmlJS::AST::FunctionExpression *fexpr);
    void processImportWarnings(
            const QString &what,
            const QQmlJS::SourceLocation &srcLocation = QQmlJS::SourceLocation());
    void addImportWithLocation(const QString &name, const QQmlJS::SourceLocation &loc);
};

QT_END_NAMESPACE

#endif // QQMLJSIMPORTEDMEMBERSVISITOR_P_H
