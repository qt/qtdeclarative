/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "codegen_p.h"

#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>
#include <QtQmlCompiler/private/qqmljsshadowcheck_p.h>
#include <QtQmlCompiler/private/qqmljsstoragegeneralizer_p.h>
#include <QtQmlCompiler/private/qqmljstypepropagator_p.h>

#include <QFileInfo>

QT_BEGIN_NAMESPACE

Codegen::Codegen(const QString &fileName, const QStringList &qmltypesFiles, QQmlJSLogger *logger,
                 QQmlJSTypeInfo *typeInfo, const QString &code)
    : m_fileName(fileName),
      m_qmltypesFiles(qmltypesFiles),
      m_logger(logger),
      m_typeInfo(typeInfo),
      m_code(code)
{
}

void Codegen::setDocument(QmlIR::JSCodeGen *codegen, QmlIR::Document *document)
{
    Q_UNUSED(codegen);
    m_document = document;
    m_pool = document->jsParserEngine.pool();
    m_unitGenerator = &document->jsGenerator;
    m_entireSourceCodeLines = document->code.split(u'\n');
}

void Codegen::setScope(const QmlIR::Object *object, const QmlIR::Object *scope)
{
    m_currentObject = object;
    m_scopeType = m_typeResolver->scopeForLocation(scope->location);
    m_objectType = m_typeResolver->scopeForLocation(object->location);
}

std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage>
Codegen::compileBinding(const QV4::Compiler::Context *context, const QmlIR::Binding &irBinding)
{
    QQmlJS::SourceLocation bindingLocation;
    bindingLocation.startColumn = irBinding.location.column;
    bindingLocation.startLine = irBinding.location.line;

    if (irBinding.type != QmlIR::Binding::Type_Script) {
        const auto bindingName = [&]() {
            switch (irBinding.type) {
            case QmlIR::Binding::Type_Invalid:
                return "invalid";
            case QmlIR::Binding::Type_Boolean:
                return "a boolean";
            case QmlIR::Binding::Type_Number:
                return "a number";
            case QmlIR::Binding::Type_String:
                return "a string";
            case QmlIR::Binding::Type_Null:
                return "null";
            case QmlIR::Binding::Type_Translation:
                return "a translation";
            case QmlIR::Binding::Type_TranslationById:
                return "a translation by id";
            case QmlIR::Binding::Type_Script:
                return "a script";
            case QmlIR::Binding::Type_Object:
                return "an object";
            case QmlIR::Binding::Type_AttachedProperty:
                return "an attached property";
            case QmlIR::Binding::Type_GroupProperty:
                return "a grouped property";
            }

            return "nothing";
        };

        return diagnose(QStringLiteral("Binding is not a script binding, but %1.")
                                .arg(QString::fromUtf8(bindingName())),
                        QtDebugMsg, bindingLocation);
    }

    Function function;
    function.qmlScope = m_scopeType;

    const QString propertyName = m_document->stringAt(irBinding.propertyNameIndex);

    const bool isProperty = m_objectType->hasProperty(propertyName);
    bool isSignal = false;
    if (!isProperty && QmlIR::IRBuilder::isSignalPropertyName(propertyName)) {
        const QString signalName = QmlIR::IRBuilder::signalNameFromSignalPropertyName(propertyName);

        if (signalName.endsWith(QStringLiteral("Changed"))
            && m_objectType->hasProperty(signalName.chopped(strlen("Changed")))) {
            isSignal = true;
        } else {
            const bool isConnections = !m_objectType->baseType().isNull()
                    && m_objectType->baseType()->internalName() == u"QQmlConnections";
            const auto methods = isConnections ? m_objectType->parentScope()->methods(signalName)
                                               : m_objectType->methods(signalName);
            for (const auto &method : methods) {
                if (method.methodType() == QQmlJSMetaMethod::Signal) {
                    isSignal = true;
                    break;
                }
            }
        }

        if (!isSignal) {
            return diagnose(QStringLiteral("Could not compile signal handler for %1: "
                                           "The signal does not exist")
                                    .arg(signalName),
                            QtWarningMsg, bindingLocation);
        }
    }

    if (!isSignal) {
        if (!isProperty) {
            return diagnose(QStringLiteral("Could not compile binding for %1: "
                                           "The property does not exist")
                                    .arg(propertyName),
                            QtWarningMsg, bindingLocation);
        }

        const auto property = m_objectType->property(propertyName);
        function.returnType = property.type();
        if (!function.returnType) {
            return diagnose(QStringLiteral("Cannot resolve property type %1 for binding on %2")
                                    .arg(property.typeName())
                                    .arg(propertyName),
                            QtWarningMsg, bindingLocation);
        }

        if (!property.bindable().isEmpty())
            function.isQPropertyBinding = true;
    }

    auto astNode =
            m_currentObject->functionsAndExpressions->slowAt(irBinding.value.compiledScriptIndex)
                    ->node;
    auto ast = astNode->asFunctionDefinition();
    if (!ast) {
        QQmlJS::AST::Statement *stmt = astNode->statementCast();
        if (!stmt) {
            Q_ASSERT(astNode->expressionCast());
            QQmlJS::AST::ExpressionNode *expr = astNode->expressionCast();
            stmt = new (m_pool) QQmlJS::AST::ExpressionStatement(expr);
        }
        auto body = new (m_pool) QQmlJS::AST::StatementList(stmt);
        body = body->finish();

        QString name = u"binding for "_qs; // ####
        ast = new (m_pool) QQmlJS::AST::FunctionDeclaration(m_pool->newString(name),
                                                                     /*formals*/ nullptr, body);
        ast->lbraceToken = astNode->firstSourceLocation();
        ast->functionToken = ast->lbraceToken;
        ast->rbraceToken = astNode->lastSourceLocation();
    }

    QQmlJS::DiagnosticMessage error;
    if (!generateFunction(QV4::Compiler::ContextType::Binding, context, ast, &function, &error)) {
        // If it's a signal and the function just returns a closure, it's harmless.
        // Otherwise promote the message to warning level.
        return diagnose(QStringLiteral("Could not compile binding for %1: %2")
                                .arg(propertyName, error.message),
                        (isSignal && error.type == QtDebugMsg) ? QtDebugMsg : QtWarningMsg,
                        error.loc);
    }

    return QQmlJSAotFunction {};
}

std::variant<QQmlJSAotFunction, QQmlJS::DiagnosticMessage>
Codegen::compileFunction(const QV4::Compiler::Context *context, const QmlIR::Function &irFunction)
{
    QQmlJS::SourceLocation functionLocation;
    functionLocation.startColumn = irFunction.location.column;
    functionLocation.startLine = irFunction.location.line;

    const QString functionName = m_document->stringAt(irFunction.nameIndex);

    Function function;
    function.qmlScope = m_scopeType;

    auto astNode = m_currentObject->functionsAndExpressions->slowAt(irFunction.index)->node;

    QQmlJS::DiagnosticMessage error;
    if (!generateFunction(QV4::Compiler::ContextType::Function, context,
                          astNode->asFunctionDefinition(), &function, &error)) {
        return diagnose(QStringLiteral("Could not compile function %1: %2")
                                .arg(functionName, error.message),
                        QtWarningMsg, error.loc);
    }

    return QQmlJSAotFunction {};
}

QQmlJSAotFunction Codegen::globalCode() const
{
    return QQmlJSAotFunction {};
}

QQmlJS::DiagnosticMessage Codegen::diagnose(const QString &message, QtMsgType type,
                                            const QQmlJS::SourceLocation &location)
{
    if (!message.isEmpty()) {
        switch (type) {
        case QtDebugMsg:
        case QtInfoMsg:
            m_logger->logInfo(message, Log_Compiler, location);
            break;
        case QtWarningMsg:
            m_logger->logWarning(message, Log_Compiler, location);
            break;
        case QtCriticalMsg:
        case QtFatalMsg:
            m_logger->logCritical(message, Log_Compiler, location);
            break;
        }
    }
    return QQmlJS::DiagnosticMessage { message, type, location };
}

bool Codegen::generateFunction(QV4::Compiler::ContextType contextType,
                               const QV4::Compiler::Context *context,
                               QQmlJS::AST::FunctionExpression *ast, Function *function,
                               QQmlJS::DiagnosticMessage *error) const
{
    const auto fail = [&](const QString &message) {
        error->loc = ast->firstSourceLocation();
        error->message = message;
        return false;
    };

    QQmlJS::AST::BoundNames arguments;
    if (ast->formals)
        arguments = ast->formals->formals();

    if (function->argumentTypes.isEmpty()) {
        for (const QQmlJS::AST::BoundName &argument : qAsConst(arguments)) {
            if (argument.typeAnnotation) {
                if (const auto type = m_typeResolver->typeFromAST(argument.typeAnnotation->type)) {
                    function->argumentTypes.append(type);
                } else {
                    return fail(QStringLiteral("Cannot resolve argument type %1")
                                .arg(argument.typeAnnotation->type->toString()));
                }
            } else {
                return fail(QStringLiteral("Functions without type annotations won't be compiled"));
            }
        }
    }

    if (!function->returnType) {
        if (ast->typeAnnotation) {
            function->returnType = m_typeResolver->typeFromAST(ast->typeAnnotation->type);
            if (!function->returnType)
                return fail(QStringLiteral("Cannot resolve return type"));
        }
    }

    function->isSignalHandler =
            !function->returnType && contextType == QV4::Compiler::ContextType::Binding;
    function->addressableScopes = m_typeResolver->objectsById();
    function->code = context->code;
    function->sourceLocations = context->sourceLocationTable.get();

    QQmlJSTypePropagator propagator(m_unitGenerator, m_typeResolver.get(), m_logger, m_typeInfo);
    QQmlJSCompilePass::InstructionAnnotations annotations = propagator.run(function, error);
    if (!error->isValid()) {
        QQmlJSShadowCheck shadowCheck(m_unitGenerator, m_typeResolver.get(), m_logger);
        shadowCheck.run(&annotations, function, error);
    }

    if (!error->isValid()) {
        QQmlJSStorageGeneralizer generalizer(m_unitGenerator, m_typeResolver.get(), m_logger);
        generalizer.run(annotations, function, error);
    }

    if (error->isValid()) {
        error->type = context->returnsClosure ? QtDebugMsg : QtWarningMsg;
        return false;
    }

    return true;
}

QT_END_NAMESPACE
