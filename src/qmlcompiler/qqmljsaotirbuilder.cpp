// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljsaotirbuilder_p.h"

QT_BEGIN_NAMESPACE

bool QQmlJSAOTIRBuilder::visit(QQmlJS::AST::FunctionExpression *ast)
{
    registerFunctionExpr(ast, IsQmlFunction::No);
    return true;
}

bool QQmlJSAOTIRBuilder::visit(QQmlJS::AST::FunctionDeclaration *ast)
{
    // QML functions are already collected when visiting the UiSourceElements, we want to collect
    // the remaining JS functions
    if (qmlFuncDecls.contains({ _object, ast }))
        return true;

    registerFunctionExpr(ast, IsQmlFunction::No);
    return true;
}

bool QQmlJSAOTIRBuilder::visit(QQmlJS::AST::UiSourceElement *node)
{
    if (QQmlJS::AST::FunctionExpression *funDecl = node->sourceElement->asFunctionDefinition())
        qmlFuncDecls.insert({ _object, funDecl });

    IRBuilder::visit(node);
    return true;
}

void QQmlJSAOTIRBuilder::registerFunctionExpr(QQmlJS::AST::FunctionExpression *fexp,
                                              IsQmlFunction isQmlFunction)
{
    // Ugly hack to prevent double insertion of bindings from setBindingValue
    for (auto *foe = _object->functionsAndExpressions->first; foe; foe = foe->next) {
        if (foe->node->kind == QQmlJS::AST::Node::Kind_ExpressionStatement) {
            if (fexp == static_cast<QQmlJS::AST::ExpressionStatement *>(foe->node)->expression)
                return;
        }
    }

    if (!_object->declarationsOverride)
        IRBuilder::registerFunctionExpr(fexp, isQmlFunction);
}

void QQmlJSAOTIRBuilder::setBindingValue(QV4::CompiledData::Binding *binding,
                                   QQmlJS::AST::Statement *statement,
                                   QQmlJS::AST::Node *parentNode)
{
    IRBuilder::setBindingValue(binding, statement, parentNode);
    statement->accept(this);
}

QT_END_NAMESPACE
