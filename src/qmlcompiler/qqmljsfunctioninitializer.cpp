// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljsfunctioninitializer_p.h"

#include <private/qqmljsmemorypool_p.h>

#include <QtCore/qloggingcategory.h>
#include <QtCore/qfileinfo.h>

#include <QtQml/private/qqmlsignalnames_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

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
        return u"invalid"_s;
    case QmlIR::Binding::Type_Boolean:
        return u"a boolean"_s;
    case QmlIR::Binding::Type_Number:
        return u"a number"_s;
    case QmlIR::Binding::Type_String:
        return u"a string"_s;
    case QmlIR::Binding::Type_Null:
        return u"null"_s;
    case QmlIR::Binding::Type_Translation:
        return u"a translation"_s;
    case QmlIR::Binding::Type_TranslationById:
        return u"a translation by id"_s;
    case QmlIR::Binding::Type_Script:
        return u"a script"_s;
    case QmlIR::Binding::Type_Object:
        return u"an object"_s;
    case QmlIR::Binding::Type_AttachedProperty:
        return u"an attached property"_s;
    case QmlIR::Binding::Type_GroupProperty:
        return u"a grouped property"_s;
    }

    return u"nothing"_s;
}

void QQmlJSFunctionInitializer::populateSignature(
        const QV4::Compiler::Context *context, QQmlJS::AST::FunctionExpression *ast,
        QQmlJSCompilePass::Function *function)
{
    const auto signatureError = [&](const QString &message) {
        m_logger->logCompileError(message, ast->firstSourceLocation());
        function->isFullyTyped = false;
    };

    QQmlJS::AST::BoundNames arguments;
    if (ast->formals)
        arguments = ast->formals->formals();

    // If the function has no arguments and no return type annotation we assume it's untyped.
    // You can annotate it to return void to make it typed.
    // Otherwise we first assume it's typed and reset the flag if we detect a problem.
    function->isFullyTyped = m_typeResolver->canCallJSFunctions()
            && (!arguments.isEmpty() || ast->typeAnnotation);

    if (function->argumentTypes.isEmpty()) {
        bool alreadyWarnedAboutMissingAnnotations = false;
        for (const QQmlJS::AST::BoundName &argument : std::as_const(arguments)) {
            if (argument.typeAnnotation) {
                if (const auto type = m_typeResolver->typeFromAST(argument.typeAnnotation->type)) {
                    function->argumentTypes.append(m_typeResolver->namedType(type));
                } else {
                    function->argumentTypes.append(
                            m_typeResolver->namedType(m_typeResolver->varType()));
                    signatureError(u"Cannot resolve the argument type %1."_s
                                   .arg(argument.typeAnnotation->type->toString()));
                }
            } else {
                if (!alreadyWarnedAboutMissingAnnotations) {
                    alreadyWarnedAboutMissingAnnotations = true;
                    signatureError(u"Functions without type annotations won't be compiled"_s);
                }
                function->argumentTypes.append(
                        m_typeResolver->namedType(m_typeResolver->varType()));
            }
        }
    } else {
        for (qsizetype i = 0, end = arguments.size(); i != end; ++i) {
            const QQmlJS::AST::BoundName &argument = arguments[i];
            if (argument.typeAnnotation) {
                if (const auto type = m_typeResolver->typeFromAST(argument.typeAnnotation->type)) {
                    if (!function->argumentTypes[i].contains(type)) {
                        signatureError(u"Type annotation %1 on signal handler "
                                        "contradicts signal argument type %2"_s
                                       .arg(argument.typeAnnotation->type->toString(),
                                            function->argumentTypes[i].descriptiveName()));
                    }
                }
            }
        }
    }

    if (!function->returnType.isValid()) {
        if (ast->typeAnnotation) {
            function->returnType = m_typeResolver->namedType(
                    m_typeResolver->typeFromAST(ast->typeAnnotation->type));
            if (!function->returnType.isValid())
                signatureError(u"Cannot resolve return type %1"_s.arg(
                                   QmlIR::IRBuilder::asString(ast->typeAnnotation->type->typeId)));
        }
    }

    for (int i = QQmlJSCompilePass::FirstArgument + function->argumentTypes.size();
         i < context->registerCountInFunction; ++i) {
        function->registerTypes.append(m_typeResolver->namedType(m_typeResolver->voidType()));
    }

    function->addressableScopes = m_typeResolver->objectsById();
    function->code = context->code;
    function->sourceLocations = context->sourceLocationTable.get();
}

static void diagnose(
        const QString &message, const QQmlJS::SourceLocation &location, QQmlJSLogger *logger)
{
    logger->logCompileError(message, location);
}

QQmlJSCompilePass::Function QQmlJSFunctionInitializer::run(
        const QV4::Compiler::Context *context, const QString &propertyName,
        QQmlJS::AST::Node *astNode, const QmlIR::Binding &irBinding)
{
    QQmlJS::SourceLocation bindingLocation;
    bindingLocation.startLine = irBinding.location.line();
    bindingLocation.startColumn = irBinding.location.column();

    QQmlJSCompilePass::Function function;
    function.qmlScope = m_typeResolver->registerContentPool()->createType(
            m_scopeType, QQmlJSRegisterContent::InvalidLookupIndex,
            QQmlJSRegisterContent::ScopeObject);

    // If we find a problem before we can determine whether it's a signal handler,
    // that's bad and warrants a warning, even if it then turns out to be a signal handler.
    const auto setIsSignalHandler = [&]() {
        function.isSignalHandler = true;

        // If the function is a signal handler and just returns a closure, it's harmless.
        // Otherwise it's a warning.
        m_logger->setCompileErrorSeverity(context->returnsClosure
                                                  ? QQmlJS::WarningSeverity::Info
                                                  : QQmlJS::WarningSeverity::Warning);
    };

    if (irBinding.type() != QmlIR::Binding::Type_Script) {
        diagnose(u"Binding is not a script binding, but %1."_s.arg(
                     bindingTypeDescription(QmlIR::Binding::Type(quint32(irBinding.type())))),
                 bindingLocation, m_logger);
    }

    function.isProperty = m_objectType->hasProperty(propertyName);
    if (function.isProperty) {
        const auto property = m_objectType->property(propertyName);
        if (const QQmlJSScope::ConstPtr propertyType = property.type()) {
            function.returnType = m_typeResolver->namedType(propertyType->isListProperty()
                ? m_typeResolver->qObjectListType()
                : QQmlJSScope::ConstPtr(property.type()));
        } else {
            diagnose(u"Cannot resolve property type %1 for binding on %2."_s
                             .arg(property.typeName(), propertyName),
                     bindingLocation, m_logger);
        }

        if (!property.bindable().isEmpty() && !property.isPrivate())
            function.isQPropertyBinding = true;
    } else if (QQmlSignalNames::isHandlerName(propertyName)) {
        if (auto actualPropertyName =
                QQmlSignalNames::changedHandlerNameToPropertyName(propertyName);
                actualPropertyName && m_objectType->hasProperty(*actualPropertyName)) {
            setIsSignalHandler();
        } else {
            auto signalName = QQmlSignalNames::handlerNameToSignalName(propertyName);
            const auto methods = m_objectType->methods(*signalName);
            for (const auto &method : methods) {
                if (method.isCloned())
                    continue;
                if (method.methodType() == QQmlJSMetaMethodType::Signal) {
                    setIsSignalHandler();
                    const auto arguments = method.parameters();
                    for (qsizetype i = 0, end = arguments.size(); i < end; ++i) {
                        const auto &type = arguments[i].type();
                        if (type.isNull()) {
                            diagnose(u"Cannot resolve the argument type %1."_s.arg(
                                             arguments[i].typeName()),
                                     bindingLocation, m_logger);
                            function.argumentTypes.append(
                                    m_typeResolver->namedType(m_typeResolver->varType()));
                        } else {
                            function.argumentTypes.append(m_typeResolver->namedType(type));
                        }
                    }
                    break;
                }
            }
            if (!function.isSignalHandler) {
                diagnose(u"Could not find signal \"%1\"."_s.arg(*signalName),
                         bindingLocation, m_logger);
            }
        }
    } else {
        QString message = u"Could not find property \"%1\"."_s.arg(propertyName);
        if (m_objectType->isNameDeferred(propertyName)) {
            // If the property doesn't exist but the name is deferred, then
            // it's deferred via the presence of immediate names. Those are
            // most commonly used to enable generalized grouped properties.
            message += u" You may want use ID-based grouped properties here.";
        }

        diagnose(message, bindingLocation, m_logger);
    }

    QQmlJS::MemoryPool pool;
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

        QString name = u"binding for "_s; // ####
        ast = new (&pool) QQmlJS::AST::FunctionDeclaration(
                pool.newString(std::move(name)), /*formals*/ nullptr, body);
        ast->lbraceToken = astNode->firstSourceLocation();
        ast->functionToken = ast->lbraceToken;
        ast->rbraceToken = astNode->lastSourceLocation();
    }

    populateSignature(context, ast, &function);
    return function;
}

QQmlJSCompilePass::Function QQmlJSFunctionInitializer::run(
        const QV4::Compiler::Context *context, const QString &functionName,
        QQmlJS::AST::Node *astNode)
{
    Q_UNUSED(functionName);

    QQmlJSCompilePass::Function function;
    function.qmlScope = m_typeResolver->registerContentPool()->createType(
            m_scopeType, QQmlJSRegisterContent::InvalidLookupIndex,
            QQmlJSRegisterContent::ScopeObject);

    auto ast = astNode->asFunctionDefinition();
    Q_ASSERT(ast);

    populateSignature(context, ast, &function);
    return function;
}

QT_END_NAMESPACE
