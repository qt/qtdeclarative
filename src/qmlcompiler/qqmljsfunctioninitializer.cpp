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

#include "qqmljsfunctioninitializer_p.h"

#include <private/qqmljsmemorypool_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qfileinfo.h>

QT_BEGIN_NAMESPACE

/*!
 * \internal
 * \class QQmlJSFunctionInitializer
 *
 * QQmlJSFunctionInitializer analyzes the IR to produce an initial
 * QQmlJSCompilePass::Function for further analysis. It only looks for the
 * signature and the QML scope and doesn't visit the byte code.
 */

static QString bindingTypeDescription(QmlIR::Binding::Type type)
{
    switch (type) {
    case QmlIR::Binding::Type_Invalid:
        return u"invalid"_qs;
    case QmlIR::Binding::Type_Boolean:
        return u"a boolean"_qs;
    case QmlIR::Binding::Type_Number:
        return u"a number"_qs;
    case QmlIR::Binding::Type_String:
        return u"a string"_qs;
    case QmlIR::Binding::Type_Null:
        return u"null"_qs;
    case QmlIR::Binding::Type_Translation:
        return u"a translation"_qs;
    case QmlIR::Binding::Type_TranslationById:
        return u"a translation by id"_qs;
    case QmlIR::Binding::Type_Script:
        return u"a script"_qs;
    case QmlIR::Binding::Type_Object:
        return u"an object"_qs;
    case QmlIR::Binding::Type_AttachedProperty:
        return u"an attached property"_qs;
    case QmlIR::Binding::Type_GroupProperty:
        return u"a grouped property"_qs;
    }

    return u"nothing"_qs;
}

void QQmlJSFunctionInitializer::populateSignature(
        const QV4::Compiler::Context *context, QQmlJS::AST::FunctionExpression *ast,
        QQmlJSCompilePass::Function *function, QQmlJS::DiagnosticMessage *error)
{
    const auto signatureError = [&](const QString &message) {
        error->type = QtWarningMsg;
        error->loc = ast->firstSourceLocation();
        error->message = message;
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
                    function->argumentTypes.append(m_typeResolver->varType());
                    signatureError(u"Cannot resolve the argument type %1."_qs
                                   .arg(argument.typeAnnotation->type->toString()));
                }
            } else {
                function->argumentTypes.append(m_typeResolver->varType());
                signatureError(u"Functions without type annotations won't be compiled"_qs);
            }
        }
    }

    if (!function->returnType) {
        if (ast->typeAnnotation) {
            function->returnType = m_typeResolver->typeFromAST(ast->typeAnnotation->type);
            if (!function->returnType)
                signatureError(u"Cannot resolve return type %1"_qs.arg(
                                   QmlIR::IRBuilder::asString(ast->typeAnnotation->type->typeId)));
        }
    }

    function->addressableScopes = m_typeResolver->objectsById();
    function->code = context->code;
    function->sourceLocations = context->sourceLocationTable.get();
}

static void diagnose(
        const QString &message, QtMsgType type, const QQmlJS::SourceLocation &location,
        QQmlJS::DiagnosticMessage *error)
{
    *error = QQmlJS::DiagnosticMessage{
        message,
        type,
        location
    };
}

QQmlJSCompilePass::Function QQmlJSFunctionInitializer::run(
        const QV4::Compiler::Context *context,
        const QString &propertyName, const QmlIR::Binding &irBinding,
        QQmlJS::DiagnosticMessage *error)
{
    QQmlJS::SourceLocation bindingLocation;
    bindingLocation.startLine = irBinding.location.line();
    bindingLocation.startColumn = irBinding.location.column();

    QQmlJSCompilePass::Function function;
    function.qmlScope = m_scopeType;

    if (irBinding.type() != QmlIR::Binding::Type_Script) {
        diagnose(u"Binding is not a script binding, but %1."_qs.arg(
                         bindingTypeDescription(irBinding.type())),
                 QtDebugMsg, bindingLocation, error);
    }

    const bool isProperty = m_objectType->hasProperty(propertyName);
    if (!isProperty && QmlIR::IRBuilder::isSignalPropertyName(propertyName)) {
        const QString signalName = QmlIR::IRBuilder::signalNameFromSignalPropertyName(propertyName);

        if (signalName.endsWith(u"Changed"_qs)
                && m_objectType->hasProperty(signalName.chopped(strlen("Changed")))) {
            function.isSignalHandler = true;
        } else {
            const auto methods = m_objectType->methods(signalName);
            for (const auto &method : methods) {
                if (method.methodType() == QQmlJSMetaMethod::Signal) {
                    function.isSignalHandler = true;
                    break;
                }
            }
        }

        if (!function.isSignalHandler) {
            diagnose(u"Could not compile signal handler for %1: The signal does not exist"_qs.arg(
                         signalName),
                     QtWarningMsg, bindingLocation, error);
        }
    }


    if (!function.isSignalHandler) {
        if (!isProperty) {
            diagnose(u"Could not compile binding for %1: The property does not exist"_qs.arg(
                         propertyName),
                     QtWarningMsg, bindingLocation, error);
        }

        const auto property = m_objectType->property(propertyName);
        function.returnType = property.type();
        if (!function.returnType) {
            diagnose(u"Cannot resolve property type %1 for binding on %2"_qs.arg(
                         property.typeName(), propertyName),
                     QtWarningMsg, bindingLocation, error);
        }

        if (!property.bindable().isEmpty() && !property.isPrivate())
            function.isQPropertyBinding = true;
    }

    QQmlJS::MemoryPool pool;
    auto astNode = m_currentObject->functionsAndExpressions->slowAt(
                irBinding.value.compiledScriptIndex)->node;
    auto ast = astNode->asFunctionDefinition();
    if (!ast) {
        QQmlJS::AST::Statement *stmt = astNode->statementCast();
        if (!stmt) {
            Q_ASSERT(astNode->expressionCast());
            QQmlJS::AST::ExpressionNode *expr = astNode->expressionCast();
            stmt = new (&pool) QQmlJS::AST::ExpressionStatement(expr);
        }
        auto body = new (&pool) QQmlJS::AST::StatementList(stmt);
        body = body->finish();

        QString name = u"binding for "_qs; // ####
        ast = new (&pool) QQmlJS::AST::FunctionDeclaration(
                pool.newString(name), /*formals*/ nullptr, body);
        ast->lbraceToken = astNode->firstSourceLocation();
        ast->functionToken = ast->lbraceToken;
        ast->rbraceToken = astNode->lastSourceLocation();
    }

    populateSignature(context, ast, &function, error);
    return function;
}

QQmlJSCompilePass::Function QQmlJSFunctionInitializer::run(
        const QV4::Compiler::Context *context,
        const QString &functionName, const QmlIR::Function &irFunction,
        QQmlJS::DiagnosticMessage *error)
{
    Q_UNUSED(functionName);

    QQmlJSCompilePass::Function function;
    function.qmlScope = m_scopeType;

    auto astNode = m_currentObject->functionsAndExpressions->slowAt(irFunction.index)->node;
    auto ast = astNode->asFunctionDefinition();
    Q_ASSERT(ast);

    populateSignature(context, ast, &function, error);
    return function;
}

QT_END_NAMESPACE
