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

void FindWarningVisitor::importExportedNames(QQmlJSScope::ConstPtr scope)
{
    QList<QQmlJSScope::ConstPtr> scopes;
    while (!scope.isNull()) {
        if (scopes.contains(scope)) {
            QString inheritenceCycle;
            for (const auto &seen: qAsConst(scopes)) {
                if (!inheritenceCycle.isEmpty())
                    inheritenceCycle.append(QLatin1String(" -> "));
                inheritenceCycle.append(seen->baseTypeName());
            }

            if (m_warnInheritanceCycle) {
                m_errors.append({
                                    QStringLiteral("%1 is part of an inheritance cycle: %2\n")
                                       .arg(scope->internalName())
                                       .arg(inheritenceCycle),
                                    QtWarningMsg,
                                    QQmlJS::SourceLocation()
                                });
            }

            m_unknownImports.insert(scope->internalName());
            m_visitFailed = true;
            break;
        }

        scopes.append(scope);

        const auto properties = scope->properties();
        for (auto property : properties)
            m_currentScope->insertPropertyIdentifier(property);

        m_currentScope->addMethods(scope->methods());

        if (scope->baseTypeName().isEmpty()) {
            break;
        } else if (auto newScope = scope->baseType()) {
            scope = newScope;
        } else {
            m_errors.append({
                                scope->baseTypeName() + QStringLiteral(
                                        " was not found. Did you add all import paths?\n"),
                                QtWarningMsg,
                                QQmlJS::SourceLocation()
                            });
            m_unknownImports.insert(scope->baseTypeName());
            m_visitFailed = true;
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

void FindWarningVisitor::throwRecursionDepthError()
{
    QQmlJSImportVisitor::throwRecursionDepthError();
    m_visitFailed = true;
}

bool FindWarningVisitor::visit(QQmlJS::AST::ClassDeclaration *ast)
{
    enterEnvironment(QQmlJSScope::JSFunctionScope, ast->name.toString());
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::ClassDeclaration *)
{
    leaveEnvironment();
}

bool FindWarningVisitor::visit(QQmlJS::AST::ForStatement *)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, "forloop");
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::ForStatement *)
{
    leaveEnvironment();
}

bool FindWarningVisitor::visit(QQmlJS::AST::ForEachStatement *)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, "foreachloop");
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::ForEachStatement *)
{
    leaveEnvironment();
}

bool FindWarningVisitor::visit(QQmlJS::AST::ExpressionStatement *)
{
    if (m_pendingSingalHandler.isValid()) {
        enterEnvironment(QQmlJSScope::JSFunctionScope, "signalhandler");
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

bool FindWarningVisitor::visit(QQmlJS::AST::Block *)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, "block");
    if (m_pendingSingalHandler.isValid())
        flushPendingSignalParameters();
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::Block *)
{
    leaveEnvironment();
}

bool FindWarningVisitor::visit(QQmlJS::AST::CaseBlock *)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, "case");
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::CaseBlock *)
{
    leaveEnvironment();
}

bool FindWarningVisitor::visit(QQmlJS::AST::Catch *catchStatement)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, "catch");
    m_currentScope->insertJSIdentifier(
                catchStatement->patternElement->bindingIdentifier.toString(), {
                    QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                    catchStatement->patternElement->firstSourceLocation()
                });
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::Catch *)
{
    leaveEnvironment();
}

bool FindWarningVisitor::visit(QQmlJS::AST::WithStatement *withStatement)
{
    if (m_warnWithStatement) {
        m_errors.append({
                            QStringLiteral(
                                "with statements are strongly discouraged in QML "
                                "and might cause false positives when analysing unqalified "
                                "identifiers\n"),
                            QtWarningMsg,
                            withStatement->firstSourceLocation()
                        });
    }

    enterEnvironment(QQmlJSScope::JSLexicalScope, "with");
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::WithStatement *)
{
    leaveEnvironment();
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
    auto name = uisb->qualifiedId->name;
    if (name == QLatin1String("id")) {
        // found id
        auto expstat = cast<ExpressionStatement *>(uisb->statement);
        auto identexp = cast<IdentifierExpression *>(expstat->expression);
        m_scopesById.insert(identexp->name.toString(), m_currentScope);

        // Figure out whether the current scope is the root scope.
        if (auto parentScope = m_currentScope->parentScope()) {
            if (!parentScope->parentScope())
                m_rootId = identexp->name.toString();
        }
    } else {
        const QString signal = signalName(name);
        if (signal.isEmpty())
            return true;

        if (!m_currentScope->methods().contains(signal) && m_warnUnqualified) {
            m_errors.append({
                                QStringLiteral("no matching signal found for handler \"%1\"\n")
                                   .arg(name.toString()),
                                QtWarningMsg,
                                uisb->firstSourceLocation()
                            });
            return true;
        }

        const auto statement = uisb->statement;
        if (statement->kind == Node::Kind::Kind_ExpressionStatement) {
            if (cast<ExpressionStatement *>(statement)->expression->asFunctionDefinition()) {
                // functions are already handled
                // they do not get names inserted according to the signal, but access their formal
                // parameters
                return true;
            }
        }

        const auto methods = m_currentScope->methods();
        const auto methodsRange = methods.equal_range(signal);
        for (auto method = methodsRange.first; method != methodsRange.second; ++method) {
            if (method->methodType() != QQmlJSMetaMethod::Signal)
                continue;

            const auto firstSourceLocation = statement->firstSourceLocation();
            bool hasMultilineStatementBody
                    = statement->lastSourceLocation().startLine > firstSourceLocation.startLine;
            m_pendingSingalHandler = firstSourceLocation;
            m_signalHandlers.insert(firstSourceLocation, {*method, hasMultilineStatementBody});
            break; // If there are multiple candidates for the signal, it's a mess anyway.
        }
        return true;
    }
    return true;
}

bool FindWarningVisitor::visit(QQmlJS::AST::UiPublicMember *uipm)
{
    if (uipm->type == QQmlJS::AST::UiPublicMember::Signal) {
        QQmlJSMetaMethod method;
        method.setMethodType(QQmlJSMetaMethod::Signal);
        method.setMethodName(uipm->name.toString());
        QQmlJS::AST::UiParameterList *param = uipm->parameters;
        while (param) {
            method.addParameter(param->name.toString(), param->type->name.toString());
            param = param->next;
        }
        m_currentScope->addMethod(method);
    } else {
        // property bool inactive: !active
        // extract name inactive
        QQmlJSMetaProperty property(
                uipm->name.toString(),
                // TODO: complex types etc.
                uipm->memberType ? uipm->memberType->name.toString() : QString(),
                uipm->typeModifier == QLatin1String("list"), !uipm->isReadonlyMember, false,
                uipm->memberType ? (uipm->memberType->name == QLatin1String("alias")) : false, 0);
        property.setType(m_rootScopeImports.value(property.typeName()));
        m_currentScope->insertPropertyIdentifier(property);
    }
    return true;
}

bool FindWarningVisitor::visit(QQmlJS::AST::IdentifierExpression *idexp)
{
    m_memberAccessChains[m_currentScope].append(
                {{idexp->name.toString(), QString(), idexp->firstSourceLocation()}});
    m_fieldMemberBase = idexp;
    return true;
}

FindWarningVisitor::FindWarningVisitor(
        QQmlJSImporter *importer, QStringList qmltypesFiles, QString code, QString fileName,
        bool silent, bool warnUnqualified, bool warnWithStatement, bool warnInheritanceCycle)
    : QQmlJSImportVisitor(importer, QFileInfo {fileName}.canonicalPath(), qmltypesFiles),
      m_code(std::move(code)),
      m_rootId(QLatin1String("<id>")),
      m_filePath(std::move(fileName)),
      m_colorOut(silent),
      m_warnUnqualified(warnUnqualified),
      m_warnWithStatement(warnWithStatement),
      m_warnInheritanceCycle(warnInheritanceCycle)
{
    m_currentScope->setInternalName("global");

    // setup color output
    m_colorOut.insertMapping(Error, ColorOutput::RedForeground);
    m_colorOut.insertMapping(Warning, ColorOutput::PurpleForeground);
    m_colorOut.insertMapping(Info, ColorOutput::BlueForeground);
    m_colorOut.insertMapping(Normal, ColorOutput::DefaultColor);
    m_colorOut.insertMapping(Hint, ColorOutput::GreenForeground);
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

static MessageColors messageColor(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return Normal;
    case QtWarningMsg:
        return Warning;
    case QtCriticalMsg:
    case QtFatalMsg:
        return Error;
    case QtInfoMsg:
        return Info;
    }

    return Normal;
}

bool FindWarningVisitor::check()
{
    for (const auto &error : qAsConst(m_errors)) {
        if (error.loc.isValid()) {
            m_colorOut.writePrefixedMessage(
                        QStringLiteral("%1:%2: %3")
                            .arg(error.loc.startLine).arg(error.loc.startColumn).arg(error.message),
                        messageColor(error.type));
        } else {
            m_colorOut.writePrefixedMessage(error.message, messageColor(error.type));
        }
    }

    if (m_visitFailed)
        return false;

    // now that all ids are known, revisit any Connections whose target were perviously unknown
    for (auto const &outstandingConnection: m_outstandingConnections) {
        auto targetScope = m_scopesById[outstandingConnection.targetName];
        if (outstandingConnection.scope && !targetScope.isNull())
            outstandingConnection.scope->addMethods(targetScope->methods());
        QScopedValueRollback<QQmlJSScope::Ptr> rollback(m_currentScope, outstandingConnection.scope);
        outstandingConnection.uiod->initializer->accept(this);
    }

    if (!m_warnUnqualified)
        return true;

    CheckIdentifiers check(&m_colorOut, m_code, m_rootScopeImports, m_filePath);
    return check(m_scopesById, m_signalHandlers, m_memberAccessChains, m_globalScope, m_rootId);
}

bool FindWarningVisitor::visit(QQmlJS::AST::VariableDeclarationList *vdl)
{
    while (vdl) {
        m_currentScope->insertJSIdentifier(
                    vdl->declaration->bindingIdentifier.toString(),
                    {
                        (vdl->declaration->scope == QQmlJS::AST::VariableScope::Var)
                            ? QQmlJSScope::JavaScriptIdentifier::FunctionScoped
                            : QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                        vdl->declaration->firstSourceLocation()
                    });
        vdl = vdl->next;
    }
    return true;
}

bool FindWarningVisitor::visit(QQmlJS::AST::FormalParameterList *fpl)
{
    for (auto const &boundName : fpl->boundNames()) {
        m_currentScope->insertJSIdentifier(
                    boundName.id, {
                        QQmlJSScope::JavaScriptIdentifier::Parameter,
                        fpl->firstSourceLocation()
                    });
    }
    return true;
}

bool FindWarningVisitor::visit(QQmlJS::AST::UiEnumDeclaration *uied)
{
    QQmlJSMetaEnum qmlEnum(uied->name.toString());
    for (const auto *member = uied->members; member; member = member->next)
        qmlEnum.addKey(member->member.toString());
    m_currentScope->addEnum(qmlEnum);
    return true;
}

bool FindWarningVisitor::visit(QQmlJS::AST::UiObjectBinding *uiob)
{
    // property QtObject __styleData: QtObject {...}

    QString name;
    for (auto id = uiob->qualifiedTypeNameId; id; id = id->next)
        name += id->name.toString() + QLatin1Char('.');

    name.chop(1);

    QQmlJSMetaProperty prop(uiob->qualifiedId->name.toString(), name, false, true, true,
                      name == QLatin1String("alias"), 0);
    prop.setType(m_rootScopeImports.value(uiob->qualifiedTypeNameId->name.toString()));
    m_currentScope->addProperty(prop);

    enterEnvironment(QQmlJSScope::QMLScope, name);
    m_currentScope->resolveTypes(m_rootScopeImports);
    importExportedNames(m_currentScope);
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::UiObjectBinding *uiob)
{
    const QQmlJSScope::ConstPtr childScope = m_currentScope;
    leaveEnvironment();
    QQmlJSMetaProperty property(uiob->qualifiedId->name.toString(),
                          uiob->qualifiedTypeNameId->name.toString(),
                          false, true, true,
                          uiob->qualifiedTypeNameId->name == QLatin1String("alias"),
                          0);
    property.setType(childScope);
    m_currentScope->addProperty(property);
}

bool FindWarningVisitor::visit(QQmlJS::AST::UiObjectDefinition *uiod)
{
    using namespace QQmlJS::AST;

    QString name;
    for (auto id = uiod->qualifiedTypeNameId; id; id = id->next)
        name += id->name.toString() + QLatin1Char('.');

    name.chop(1);
    enterEnvironment(QQmlJSScope::QMLScope, name);
    if (name.isLower())
        return false; // Ignore grouped properties for now

    m_currentScope->resolveTypes(m_rootScopeImports);
    importExportedNames(m_currentScope);

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
        if (targetScope)
            m_currentScope->addMethods(targetScope->methods());
    }
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

void FindWarningVisitor::endVisit(QQmlJS::AST::UiObjectDefinition *)
{
    auto childScope = m_currentScope;
    leaveEnvironment();

    if (m_currentScope == m_globalScope
            || m_currentScope->baseTypeName() == QStringLiteral("Component")) {
        return;
    }

    const auto properties = childScope->properties();
    const auto it = properties.find(QStringLiteral("parent"));
    if (it != properties.end()) {
        auto property = *it;
        property.setType(QQmlJSScope::ConstPtr(m_currentScope));
        childScope->addProperty(property);
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
        chain.last().append(FieldMember {
                                fieldMember->name.toString(), type, fieldMember->identifierToken
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
