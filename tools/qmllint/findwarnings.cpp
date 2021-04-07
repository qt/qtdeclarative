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

#include "findwarnings.h"
#include "checkidentifiers.h"

#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtQmlCompiler/private/qqmljstypedescriptionreader_p.h>
#include <QtQmlCompiler/private/qqmljstypereader_p.h>

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qv4codegen_p.h>
#include <QtQml/private/qqmlimportresolver_p.h>

#include <QtCore/qfile.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qscopedvaluerollback.h>

void FindWarningVisitor::checkInheritanceCycle(QQmlJSScope::ConstPtr scope)
{
    QQmlJSScope::ConstPtr originalScope = scope;
    QList<QQmlJSScope::ConstPtr> scopes;
    while (!scope.isNull()) {

        for (const QQmlJSAnnotation &annotation : scope->annotations()) {
            if (annotation.isDeprecation()) {
                QQQmlJSDeprecation deprecation = annotation.deprecation();

                QString message = QStringLiteral("Type \"%1\" is deprecated")
                        .arg(scope->internalName());

                if (!deprecation.reason.isEmpty())
                    message.append(QStringLiteral(" (Reason: %1)").arg(deprecation.reason));

                m_logger.log(message, Log_Deprecation,  originalScope->sourceLocation()
                );
            }
        }

        if (scopes.contains(scope)) {
            QString inheritenceCycle;
            for (const auto &seen: qAsConst(scopes)) {
                if (!inheritenceCycle.isEmpty())
                    inheritenceCycle.append(QLatin1String(" -> "));
                inheritenceCycle.append(seen->baseTypeName());
            }

            m_logger.log(QStringLiteral("%1 is part of an inheritance cycle: %2\n")
                         .arg(scope->internalName())
                         .arg(inheritenceCycle), Log_InheritanceCycle);

            m_unknownImports.insert(scope->internalName());
            break;
        }

        scopes.append(scope);

        if (scope->baseTypeName().isEmpty()) {
            break;
        } else if (auto newScope = scope->baseType()) {
            scope = newScope;
        } else {
            m_logger.log(scope->baseTypeName() + QStringLiteral(
                                        " was not found. Did you add all import paths?\n"),
                                Log_Import);
            m_unknownImports.insert(scope->baseTypeName());
            break;
        }
    }
}

void FindWarningVisitor::checkGroupedAndAttachedScopes(QQmlJSScope::ConstPtr scope)
{
    auto children = scope->childScopes();
    while (!children.isEmpty()) {
        auto childScope = children.takeFirst();
        const auto type = childScope->scopeType();
        switch (type) {
        case QQmlJSScope::GroupedPropertyScope:
        case QQmlJSScope::AttachedPropertyScope:
            if (!childScope->baseType()) {
                m_logger.log(
                            QStringLiteral("unknown %1 property scope %2.")
                            .arg(type == QQmlJSScope::GroupedPropertyScope
                                 ? QStringLiteral("grouped")
                                 : QStringLiteral("attached"),
                                 childScope->internalName()),
                            Log_UnqualifiedAccess,
                            childScope->sourceLocation()
                            );
            }
            children.append(childScope->childScopes());
        default:
            break;
        }
    }
}

void FindWarningVisitor::flushPendingSignalParameters()
{
    const SignalHandler handler = m_signalHandlers[m_pendingSingalHandler];
    for (const QString &parameter : handler.signal.parameterNames()) {
        m_currentScope->insertJSIdentifier(
                    parameter, {
                        QQmlJSScope::JavaScriptIdentifier::Injected,
                        m_pendingSingalHandler
                    });
    }
    m_pendingSingalHandler = QQmlJS::SourceLocation();
}

void FindWarningVisitor::checkDefaultProperty(const QQmlJSScope::ConstPtr &scope)
{
    if (scope == m_exportedRootScope) // inapplicable
        return;

    const QQmlJSScope *scopeOfDefaultProperty = nullptr;
    QString defaultPropertyName;
    // NB: start looking for default property in parent scope (because this
    // scope is not suitable), but progress through baseType()
    for (auto s = scope->parentScope(); s; s = s->baseType()) {
        defaultPropertyName = s->defaultPropertyName();
        if (!defaultPropertyName.isEmpty()) {
            scopeOfDefaultProperty = s.data();
            break;
        }
    }
    if (defaultPropertyName.isEmpty()) {
        m_logger.log(QStringLiteral("Cannot assign to non-existent default property"),
                     Log_Property, scope->sourceLocation() );
        return;
    }

    Q_ASSERT(scopeOfDefaultProperty);
    Q_ASSERT(scope->parentScope());
    QQmlJSMetaProperty defaultProp = scopeOfDefaultProperty->property(defaultPropertyName);

    // abuse QHash feature to construct default value through
    // operator[]. default bool is false, which is what's needed
    if (m_scopeHasDefaultPropertyAssignment[scopeOfDefaultProperty] && !defaultProp.isList()) {
        // already has some object assigned to a default property and
        // this default property is not a list property
        m_logger.log(QStringLiteral("Cannot assign multiple objects to a default non-list property"),
                     Log_Property, scope->sourceLocation());
    }
    m_scopeHasDefaultPropertyAssignment[scopeOfDefaultProperty] = true;

    auto propType = defaultProp.type().data();
    if (!propType) // should be an error somewhere else
        return;

    // scope's type hierarchy has to have property type
    for (const QQmlJSScope *type = scope.data(); type; type = type->baseType().data()) {
        if (type == propType)
            return;
    }

    m_logger.log(QStringLiteral("Cannot assign to default property of incompatible type"),
                 Log_Property, scope->sourceLocation());
}

void FindWarningVisitor::throwRecursionDepthError()
{
    QQmlJSImportVisitor::throwRecursionDepthError();
}

bool FindWarningVisitor::visit(QQmlJS::AST::ExpressionStatement *ast)
{
    if (m_pendingSingalHandler.isValid()) {
        enterEnvironment(QQmlJSScope::JSFunctionScope, "signalhandler", ast->firstSourceLocation());
        flushPendingSignalParameters();
    }
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::ExpressionStatement *)
{
    if (m_currentScope->scopeType() == QQmlJSScope::JSFunctionScope
            && m_currentScope->baseTypeName() == "signalhandler") {
        leaveEnvironment();
    }
}

bool FindWarningVisitor::visit(QQmlJS::AST::Block *block)
{
    if (!QQmlJSImportVisitor::visit(block))
        return false;
    if (m_pendingSingalHandler.isValid())
        flushPendingSignalParameters();
    return true;
}

bool FindWarningVisitor::visit(QQmlJS::AST::WithStatement *withStatement)
{
    m_logger.log(QStringLiteral(
                     "with statements are strongly discouraged in QML "
                                "and might cause false positives when analysing unqualified "
                                "identifiers\n"),
                 Log_WithStatement,
                 withStatement->firstSourceLocation());

    return QQmlJSImportVisitor::visit(withStatement);
}

static QString signalName(QStringView handlerName)
{
    if (handlerName.startsWith(u"on") && handlerName.size() > 2) {
        QString signal = handlerName.mid(2).toString();
        for (int i = 0; i < signal.length(); ++i) {
            QChar &ch = signal[i];
            if (ch.isLower())
                return QString();
            if (ch.isUpper()) {
                ch = ch.toLower();
                return signal;
            }
        }
    }
    return QString();
}

bool FindWarningVisitor::visit(QQmlJS::AST::UiScriptBinding *uisb)
{
    using namespace QQmlJS::AST;

    const auto qmlScope = m_currentScope;
    if (!QQmlJSImportVisitor::visit(uisb))
        return false;

    auto name = uisb->qualifiedId->name;
    if (name == QLatin1String("id")) {
        // Figure out whether the current scope is the root scope.
        if (auto parentScope = qmlScope->parentScope()) {
            if (!parentScope->parentScope()) {
                const auto expstat = cast<ExpressionStatement *>(uisb->statement);
                const auto identexp = cast<IdentifierExpression *>(expstat->expression);
                m_rootId = identexp->name.toString();
            }
        }
        return true;
    }

    const QString signal = signalName(name);
    if (signal.isEmpty()) {
        for (const auto &childScope : qmlScope->childScopes()) {
            if ((childScope->scopeType() == QQmlJSScope::AttachedPropertyScope
                 || childScope->scopeType() == QQmlJSScope::GroupedPropertyScope)
                    && childScope->internalName() == name) {
                return true;
            }
        }

        if (!qmlScope->hasProperty(name.toString())) {
            // TODO: Can this be in a better suited category?
            m_logger.log(
                        QStringLiteral("Binding assigned to \"%1\", but no property \"%1\" "
                                               "exists in the current element.\n").arg(name),
                        Log_Type,
                        uisb->firstSourceLocation()
                        );
            return true;
        }

        const auto property = qmlScope->property(name.toString());
        if (!property.type()) {
            m_logger.log(
                        QStringLiteral("No type found for property \"%1\". This may be due "
                                               "to a missing import statement or incomplete "
                                               "qmltypes files.\n").arg(name),
                        Log_Type,
                        uisb->firstSourceLocation()
                        );
        }

        const auto &annotations = property.annotations();

        const auto deprecationAnn = std::find_if(annotations.cbegin(), annotations.cend(), [](const QQmlJSAnnotation &ann) { return ann.isDeprecation(); });

        if (deprecationAnn != annotations.cend()) {
            const auto deprecation = deprecationAnn->deprecation();

            QString message = QStringLiteral("Binding on deprecated property \"%1\"")
                    .arg(property.propertyName());

            if (!deprecation.reason.isEmpty())
                message.append(QStringLiteral(" (Reason: %1)").arg(deprecation.reason));

            m_logger.log(message, Log_Deprecation, uisb->firstSourceLocation());
        }


        return true;
    }


    if (!qmlScope->hasMethod(signal)) {
        m_logger.log(
                    QStringLiteral("no matching signal found for handler \"%1\"\n")
                    .arg(name.toString()),
                    Log_UnqualifiedAccess,
                    uisb->firstSourceLocation()
                    );
        return true;
    }

    QQmlJSMetaMethod scopeSignal;
    for (QQmlJSScope::ConstPtr scope = qmlScope; scope; scope = scope->baseType()) {
        const auto methods = scope->ownMethods();
        const auto methodsRange = methods.equal_range(signal);
        for (auto method = methodsRange.first; method != methodsRange.second; ++method) {
            if (method->methodType() != QQmlJSMetaMethod::Signal)
                continue;
            scopeSignal = *method;
            break;
        }
    }

    const auto statement = uisb->statement;
    if (ExpressionStatement *expr = cast<ExpressionStatement *>(statement)) {
        if (FunctionExpression *func = expr->expression->asFunctionDefinition()) {
            // functions are already handled
            // they do not get names inserted according to the signal, but access their formal
            // parameters. Let's still check if the names match, though.
            const QStringList signalParameters = scopeSignal.parameterNames();
            qsizetype i = 0, end = signalParameters.length();
            for (FormalParameterList *formal = func->formals;
                 formal; ++i, formal = formal->next) {
                if (i == end) {
                    m_logger.log(
                                QStringLiteral("Signal handler for \"%2\" has more formal"
                                                       " parameters than the signal it handles.")
                                .arg(name),
                                Log_Signal,
                                uisb->firstSourceLocation()
                                );
                }

                const QStringView handlerParameter = formal->element->bindingIdentifier;
                const qsizetype j = signalParameters.indexOf(handlerParameter);
                if (j == i || j < 0)
                    continue;

                m_logger.log(
                            QStringLiteral("Parameter %1 to signal handler for \"%2\""
                                                   " is called \"%3\". The signal has a parameter"
                                                   " of the same name in position %4.\n")
                            .arg(i + 1).arg(name, handlerParameter).arg(j + 1),
                            Log_Signal,
                            uisb->firstSourceLocation()
                            );
            }

            return true;
        }
    }

    const auto firstSourceLocation = statement->firstSourceLocation();
    bool hasMultilineStatementBody
            = statement->lastSourceLocation().startLine > firstSourceLocation.startLine;
    m_pendingSingalHandler = firstSourceLocation;
    m_signalHandlers.insert(firstSourceLocation, {scopeSignal, hasMultilineStatementBody});
    return true;
}

bool FindWarningVisitor::visit(QQmlJS::AST::IdentifierExpression *idexp)
{
    const QString name = idexp->name.toString();
    if (name.front().isUpper() && m_importTypeLocationMap.contains(name)) {
        m_usedTypes.insert(name);
    }

    m_memberAccessChains[m_currentScope].append(
                {{name, QString(), idexp->firstSourceLocation()}});
    m_fieldMemberBase = idexp;
    return true;
}

FindWarningVisitor::FindWarningVisitor(
        QQmlJSImporter *importer, QStringList qmltypesFiles, QString code, QString fileName,
        bool silent)
    : QQmlJSImportVisitor(importer,
                          implicitImportDirectory(fileName, importer->resourceFileMapper()),
                          qmltypesFiles, fileName, code, silent),
      m_code(code),
      m_rootId(QLatin1String("<id>")),
      m_filePath(fileName)
{
    m_currentScope->setInternalName("global");

    QLatin1String jsGlobVars[] = {
        /* Not listed on the MDN page; browser and QML extensions: */
        // console/debug api
        QLatin1String("console"), QLatin1String("print"),
        // garbage collector
        QLatin1String("gc"),
        // i18n
        QLatin1String("qsTr"), QLatin1String("qsTrId"), QLatin1String("QT_TR_NOOP"),
        QLatin1String("QT_TRANSLATE_NOOP"), QLatin1String("QT_TRID_NOOP"),
        // XMLHttpRequest
        QLatin1String("XMLHttpRequest")
    };

    QQmlJSScope::JavaScriptIdentifier globalJavaScript = {
        QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
        QQmlJS::SourceLocation()
    };
    for (const char **globalName = QV4::Compiler::Codegen::s_globalNames;
         *globalName != nullptr;
         ++globalName) {
        m_currentScope->insertJSIdentifier(QString::fromLatin1(*globalName), globalJavaScript);
    }
    for (const auto& jsGlobVar: jsGlobVars)
        m_currentScope->insertJSIdentifier(jsGlobVar, globalJavaScript);
}

bool FindWarningVisitor::check()
{
    // now that all ids are known, revisit any Connections whose target were perviously unknown
    for (auto const &outstandingConnection: m_outstandingConnections) {
        auto targetScope = m_scopesById[outstandingConnection.targetName];
        if (outstandingConnection.scope) {
            for (const auto scope = targetScope; targetScope;
                 targetScope = targetScope->baseType()) {
                const auto connectionMethods = targetScope->ownMethods();
                for (const auto &method : connectionMethods)
                    outstandingConnection.scope->addOwnMethod(method);
            }
        }
        QScopedValueRollback<QQmlJSScope::Ptr> rollback(m_currentScope, outstandingConnection.scope);
        outstandingConnection.uiod->initializer->accept(this);
    }

    auto unusedImports = m_importLocations;
    for (const QString &type : m_usedTypes) {
        for (const auto &importLocation : m_importTypeLocationMap.values(type))
            unusedImports.remove(importLocation);

        // If there are no more unused imports left we can abort early
        if (unusedImports.isEmpty())
            break;
    }

    for (const auto &import : unusedImports) {
        m_logger.log(
                    QString::fromLatin1("Unused import at %1:%2:%3\n")
                    .arg(m_filePath)
                    .arg(import.startLine).arg(import.startColumn),
                    Log_UnusedImport, import);
    }

    CheckIdentifiers check(&m_logger, m_code, m_rootScopeImports, m_filePath);
    check(m_scopesById, m_signalHandlers, m_memberAccessChains, m_globalScope, m_rootId);

    return !m_logger.hasWarnings() && !m_logger.hasErrors();
}

bool FindWarningVisitor::visit(QQmlJS::AST::UiObjectBinding *uiob)
{
    if (!QQmlJSImportVisitor::visit(uiob))
        return false;

    checkInheritanceCycle(m_currentScope);
    return true;
}

bool FindWarningVisitor::visit(QQmlJS::AST::UiObjectDefinition *uiod)
{
    using namespace QQmlJS::AST;

    if (!QQmlJSImportVisitor::visit(uiod))
        return false;

    const QString name = m_currentScope->baseTypeName();
    if (name.isEmpty() || name.front().isLower())
        return false; // Ignore grouped properties for now

    checkInheritanceCycle(m_currentScope);

    if (name.endsWith("Connections")) {
        QString target;
        auto member = uiod->initializer->members;
        while (member) {
            if (member->member->kind == QQmlJS::AST::Node::Kind_UiScriptBinding) {
                auto asBinding = static_cast<QQmlJS::AST::UiScriptBinding*>(member->member);
                if (asBinding->qualifiedId->name == QLatin1String("target")) {
                    if (asBinding->statement->kind == QQmlJS::AST::Node::Kind_ExpressionStatement) {
                        auto expr = static_cast<QQmlJS::AST::ExpressionStatement*>(asBinding->statement)->expression;
                        if (auto idexpr = QQmlJS::AST::cast<QQmlJS::AST::IdentifierExpression*>(expr)) {
                            target = idexpr->name.toString();
                        } else {
                            // more complex expressions are not supported
                        }
                    }
                    break;
                }
            }
            member = member->next;
        }
        QQmlJSScope::ConstPtr targetScope;
        if (target.isEmpty()) {
            // no target set, connection comes from parentF
            QQmlJSScope::Ptr scope = m_currentScope;
            do {
                scope = scope->parentScope(); // TODO: rename method
            } while (scope->scopeType() != QQmlJSScope::QMLScope);
            targetScope = m_rootScopeImports.value(scope->baseTypeName());
        } else {
            // there was a target, check if we already can find it
            auto scopeIt =  m_scopesById.find(target);
            if (scopeIt != m_scopesById.end()) {
                targetScope = *scopeIt;
            } else {
                m_outstandingConnections.push_back({target, m_currentScope, uiod});
                return false; // visit children later once target is known
            }
        }
        for (const auto scope = targetScope; targetScope; targetScope = targetScope->baseType()) {
            const auto connectionMethods = targetScope->ownMethods();
            for (const auto &method : connectionMethods)
                m_currentScope->addOwnMethod(method);
        }
    }

    checkDefaultProperty(m_currentScope);

    return true;
}

bool FindWarningVisitor::visit(QQmlJS::AST::PatternElement *element)
{
    if (element->isVariableDeclaration()) {
        QQmlJS::AST::BoundNames names;
        element->boundNames(&names);
        for (const auto &name : names) {
            m_currentScope->insertJSIdentifier(
                        name.id, {
                            (element->scope == QQmlJS::AST::VariableScope::Var)
                                ? QQmlJSScope::JavaScriptIdentifier::FunctionScoped
                                : QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                            element->firstSourceLocation()
                        });
        }
    }

    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::UiObjectDefinition *uiod)
{
    auto childScope = m_currentScope;
    QQmlJSImportVisitor::endVisit(uiod);

    checkGroupedAndAttachedScopes(childScope);

    if (m_currentScope == m_globalScope
            || m_currentScope->baseTypeName() == QStringLiteral("Component")) {
        return;
    }

    auto property = childScope->property(QStringLiteral("parent"));
    if (!property.propertyName().isEmpty()) {
        property.setType(QQmlJSScope::ConstPtr(m_currentScope));
        childScope->addOwnProperty(property);
    }
}

bool FindWarningVisitor::visit(QQmlJS::AST::FieldMemberExpression *)
{
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::FieldMemberExpression *fieldMember)
{
    using namespace QQmlJS::AST;
    ExpressionNode *base = fieldMember->base;

    while (auto *nested = cast<NestedExpression *>(base))
        base = nested->expression;

    if (m_fieldMemberBase == base) {
        QString type;
        if (auto *binary = cast<BinaryExpression *>(base)) {
            if (binary->op == QSOperator::As) {
                if (auto *right = cast<TypeExpression *>(binary->right))
                    type = right->m_type->toString();
            }
        }


        auto &chain = m_memberAccessChains[m_currentScope];

        Q_ASSERT(!chain.last().isEmpty());

        const QString name = fieldMember->name.toString();
        if (m_importTypeLocationMap.contains(name)) {
            if (auto it = m_rootScopeImports.find(name); it != m_rootScopeImports.end() && !*(it))
                m_usedTypes.insert(name);
        }

        chain.last().append(FieldMember {
                                name, type, fieldMember->identifierToken
                            });
        m_fieldMemberBase = fieldMember;
    } else {
        m_fieldMemberBase = nullptr;
    }
}

bool FindWarningVisitor::visit(QQmlJS::AST::BinaryExpression *)
{
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::BinaryExpression *binExp)
{
    if (binExp->op == QSOperator::As && m_fieldMemberBase == binExp->left)
        m_fieldMemberBase = binExp;
    else
        m_fieldMemberBase = nullptr;
}

bool FindWarningVisitor::visit(QQmlJS::AST::UiPublicMember *uipb)
{
    QQmlJSImportVisitor::visit(uipb);
    if (uipb->type == QQmlJS::AST::UiPublicMember::Property && uipb->memberType != nullptr
        && !uipb->memberType->name.isEmpty() && uipb->memberType->name != QLatin1String("alias")) {
        const auto name = uipb->memberType->name.toString();
        if (m_importTypeLocationMap.contains(name)) {
            m_usedTypes.insert(name);
        } else {
            m_logger.log(name + QStringLiteral(" was not found. Did you add all import paths?\n"),
                         Log_Import);
        }
    }

    return true;
}
