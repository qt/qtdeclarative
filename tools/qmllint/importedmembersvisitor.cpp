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

#include "importedmembersvisitor.h"
#include "scopetree.h"

using namespace QQmlJS::AST;

ScopeTree *ImportedMembersVisitor::result(const QString &scopeName) const
{
    ScopeTree *result = new ScopeTree(ScopeType::QMLScope);
    result->setClassName(scopeName);
    result->setSuperclassName(m_rootObject->superclassName());
    const auto properties = m_rootObject->properties();
    for (auto property : properties) {
        if (property.isAlias()) {
            const auto it = m_objects.find(property.typeName());
            if (it != m_objects.end())
                property.setType(it->get());
            result->addProperty(property);
        } else {
            result->addProperty(property);
        }
    }

    for (const auto &method : m_rootObject->methods())
        result->addMethod(method);

    return result;
}

bool ImportedMembersVisitor::visit(UiObjectDefinition *definition)
{
    ScopeTree::Ptr scope(new ScopeTree(ScopeType::QMLScope));
    QString superType;
    for (auto segment = definition->qualifiedTypeNameId; segment; segment = segment->next) {
        if (!superType.isEmpty())
            superType.append('.');
        superType.append(segment->name.toString());
    }
    scope->setSuperclassName(superType);
    if (!m_rootObject)
        m_rootObject = scope;
    m_currentObjects.append(scope);
    return true;
}

void ImportedMembersVisitor::endVisit(UiObjectDefinition *)
{
    m_currentObjects.pop_back();
}

bool ImportedMembersVisitor::visit(UiPublicMember *publicMember)
{
    switch (publicMember->type) {
    case UiPublicMember::Signal: {
        UiParameterList *param = publicMember->parameters;
        MetaMethod method;
        method.setMethodType(MetaMethod::Signal);
        method.setMethodName(publicMember->name.toString());
        while (param) {
            method.addParameter(param->name.toString(), param->type->name.toString());
            param = param->next;
        }
        currentObject()->addMethod(method);
        break;
    }
    case UiPublicMember::Property: {
        auto typeName = publicMember->memberType->name;
        const bool isAlias = (typeName == QLatin1String("alias"));
        if (isAlias) {
            const auto expression = cast<ExpressionStatement *>(publicMember->statement);
            if (const auto idExpression = cast<IdentifierExpression *>(expression->expression))
                typeName = idExpression->name;
        }
        MetaProperty prop {
            publicMember->name.toString(),
            typeName.toString(),
            false,
            false,
            false,
            isAlias,
            0
        };
        currentObject()->addProperty(prop);
        break;
    }
    }
    return true;
}

bool ImportedMembersVisitor::visit(UiSourceElement *sourceElement)
{
    if (FunctionExpression *fexpr = sourceElement->sourceElement->asFunctionDefinition()) {
        MetaMethod method;
        method.setMethodName(fexpr->name.toString());
        method.setMethodType(MetaMethod::Method);
        FormalParameterList *parameters = fexpr->formals;
        while (parameters) {
            method.addParameter(parameters->element->bindingIdentifier.toString(), "");
            parameters = parameters->next;
        }
        currentObject()->addMethod(method);
    } else if (ClassExpression *clexpr = sourceElement->sourceElement->asClassDefinition()) {
        MetaProperty prop { clexpr->name.toString(), "", false, false, false, false, 1 };
        currentObject()->addProperty(prop);
    } else if (cast<VariableStatement *>(sourceElement->sourceElement)) {
        // nothing to do
    } else {
        const auto loc = sourceElement->firstSourceLocation();
        m_colorOut->writeUncolored(
                    "unsupportedd sourceElement at "
                    + QString::fromLatin1("%1:%2: ").arg(loc.startLine).arg(loc.startColumn)
                    + QString::number(sourceElement->sourceElement->kind));
    }
    return true;
}

bool ImportedMembersVisitor::visit(UiScriptBinding *scriptBinding)
{
    if (scriptBinding->qualifiedId->name == QLatin1String("id")) {
        const auto *statement = cast<ExpressionStatement *>(scriptBinding->statement);
        const auto *idExprension = cast<IdentifierExpression *>(statement->expression);
        m_objects.insert(idExprension->name.toString(), currentObject());
    }
    return true;
}

void ImportedMembersVisitor::throwRecursionDepthError()
{
    m_colorOut->write(QStringLiteral("Error"), Error);
    m_colorOut->write(QStringLiteral("Maximum statement or expression depth exceeded"), Error);
}
