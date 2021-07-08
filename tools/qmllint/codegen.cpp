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

#include "codegen.h"

#include <QtQmlCompiler/private/qqmljstypepropagator_p.h>
#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>

#include <QFileInfo>

Codegen::Codegen(QQmlJSImporter *importer, const QString &fileName,
                 const QStringList &qmltypesFiles, QQmlJSLogger *logger, const QString &code)
    : m_fileName(fileName),
      m_qmltypesFiles(qmltypesFiles),
      m_importer(importer),
      m_logger(logger),
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
    m_typeResolver = std::make_unique<QQmlJSTypeResolver>(
            m_importer, document,
            QQmlJSImportVisitor::implicitImportDirectory(m_fileName,
                                                         m_importer->resourceFileMapper()),
            m_qmltypesFiles, QQmlJSTypeResolver::Indirect, QQmlJSTypeResolver::Dynamic, m_logger);
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

        return diagnose(
                QStringLiteral("Binding is not a script binding, but %1.").arg(bindingName()),
                QtDebugMsg, bindingLocation);
    }

    Function function;
    function.contextType = QV4::Compiler::ContextType::Binding;
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
            const auto methods = m_objectType->methods(signalName);
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
    function.ast = astNode->asFunctionDefinition();
    if (!function.ast) {
        QQmlJS::AST::Statement *stmt = astNode->statementCast();
        if (!stmt) {
            Q_ASSERT(astNode->expressionCast());
            QQmlJS::AST::ExpressionNode *expr = astNode->expressionCast();
            stmt = new (m_pool) QQmlJS::AST::ExpressionStatement(expr);
        }
        auto body = new (m_pool) QQmlJS::AST::StatementList(stmt);
        body = body->finish();

        QString name = "binding for "; // ####
        function.ast = new (m_pool) QQmlJS::AST::FunctionDeclaration(m_pool->newString(name),
                                                                     /*formals*/ nullptr, body);
        function.ast->lbraceToken = astNode->firstSourceLocation();
        function.ast->functionToken = function.ast->lbraceToken;
        function.ast->rbraceToken = astNode->lastSourceLocation();
    }

    if (!generateFunction(context, &function)) {
        // If it's a signal and the function just returns a closure, it's harmless.
        // Otherwise promote the message to warning level.
        return diagnose(QStringLiteral("Could not compile binding for %1: %2")
                                .arg(propertyName, function.error.message),
                        (isSignal && function.error.type == QtDebugMsg) ? QtDebugMsg : QtWarningMsg,
                        function.error.loc);
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
    function.contextType = QV4::Compiler::ContextType::Function;
    function.qmlScope = m_scopeType;

    auto astNode = m_currentObject->functionsAndExpressions->slowAt(irFunction.index)->node;
    function.ast = astNode->asFunctionDefinition();
    Q_ASSERT(function.ast);

    if (!generateFunction(context, &function)) {
        return diagnose(QStringLiteral("Could not compile function %1: %2")
                                .arg(functionName, function.error.message),
                        QtWarningMsg, function.error.loc);
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

void Codegen::instructionOffsetToSrcLocation(const QV4::Compiler::Context *context, uint offset,
                                             QQmlJS::SourceLocation *srcLoc) const
{
    auto findLine = [](const QV4::CompiledData::CodeOffsetToLine &entry, uint offset) {
        return entry.codeOffset < offset;
    };
    const QV4::CompiledData::CodeOffsetToLine *codeToLine =
            std::lower_bound(context->lineNumberMapping.constBegin(),
                             context->lineNumberMapping.constEnd(), offset + 1, findLine)
            - 1;

    QStringList lines = m_code.split(u'\n');

    srcLoc->startLine = codeToLine->line;
    srcLoc->startColumn = 0;
    srcLoc->length = 0;
    srcLoc->length = lines[srcLoc->startLine - 1].size();

    srcLoc->offset = std::accumulate(lines.begin(), lines.begin() + codeToLine->line - 1, 0,
                                     [](int a, QString s) { return s.length() + a; })
            + codeToLine->line - 1;
}

bool Codegen::generateFunction(const QV4::Compiler::Context *context, Function *function) const
{
    const auto error = [&](const QString &message) {
        QQmlJS::DiagnosticMessage msg;
        msg.loc = function->ast->firstSourceLocation();
        msg.message = message;
        function->error = msg;
        return false;
    };

    QQmlJS::AST::BoundNames arguments;
    if (function->ast->formals)
        arguments = function->ast->formals->formals();

    if (function->argumentTypes.isEmpty()) {
        for (const QQmlJS::AST::BoundName &argument : qAsConst(arguments)) {
            if (argument.typeAnnotation) {
                const auto rawType = m_typeResolver->typeFromAST(argument.typeAnnotation->type);
                if (const auto storedType = m_typeResolver->storedType(
                            rawType, QQmlJSTypeResolver::ComponentIsGeneric::Yes)) {
                    function->argumentTypes.append(storedType);
                    continue;
                } else {
                    return error(QStringLiteral("Cannot store the argument type %1.")
                                         .arg(rawType ? rawType->internalName() : "<unknown>"));
                }
            } else {
                return error(
                        QStringLiteral("Functions without type annotations won't be compiled"));
                return false;
            }
        }
    }

    QQmlJSTypePropagator propagator(m_unitGenerator, m_typeResolver.get());

    if (!function->returnType) {
        if (function->ast->typeAnnotation) {
            function->returnType = m_typeResolver->typeFromAST(function->ast->typeAnnotation->type);
            if (!function->returnType)
                return error(QStringLiteral("Cannot resolve return type"));
        }
    }

    if (function->returnType) {
        if (auto returnType = m_typeResolver->storedType(
                    function->returnType, QQmlJSTypeResolver::ComponentIsGeneric::Yes)) {
            function->returnType = returnType;
        } else {
            return error(QStringLiteral("Cannot store the return type %1.")
                                 .arg(function->returnType->internalName()));
        }
    }

    QQmlJSTypePropagator::Error propagationError;
    auto typePropagationResult = propagator.propagateTypes(
            context, arguments, function->returnType, function->qmlScope,
            !function->returnType && function->contextType == QV4::Compiler::ContextType::Binding,
            &propagationError);
    if (propagationError.isSet()) {
        QQmlJS::DiagnosticMessage msg;
        msg.type = context->returnsClosure ? QtDebugMsg : QtWarningMsg;
        instructionOffsetToSrcLocation(context, propagationError.instructionOffset, &msg.loc);
        msg.message = propagationError.message;
        function->error = msg;
        return false;
    }

    return true;
}
