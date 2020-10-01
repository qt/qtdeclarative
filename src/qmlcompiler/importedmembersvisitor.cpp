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

#include "importedmembersvisitor_p.h"

using namespace QQmlJS::AST;

QQmlJSScope::Ptr ImportedMembersVisitor::result(const QString &scopeName) const
{
    QQmlJSScope::Ptr result = QQmlJSScope::create();
    result->setIsComposite(true);
    result->setInternalName(scopeName);
    result->setBaseTypeName(m_rootObject->baseTypeName());
    const auto properties = m_rootObject->properties();
    for (auto property : properties) {
        if (property.isAlias()) {
            const auto it = m_objects.find(property.typeName());
            if (it != m_objects.end())
                property.setType(*it);
            result->addProperty(property);
        } else {
            result->addProperty(property);
        }
    }

    for (const auto &method : m_rootObject->methods())
        result->addMethod(method);

    for (const auto &enumerator : m_rootObject->enums())
        result->addEnum(enumerator);

    return result;
}

bool ImportedMembersVisitor::visit(UiObjectDefinition *definition)
{
    QQmlJSScope::Ptr scope = QQmlJSScope::create();
    QString superType;
    for (auto segment = definition->qualifiedTypeNameId; segment; segment = segment->next) {
        if (!superType.isEmpty())
            superType.append(u'.');
        superType.append(segment->name.toString());
    }
    scope->setBaseTypeName(superType);
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
        QQmlJSMetaMethod method;
        method.setMethodType(QQmlJSMetaMethod::Signal);
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
        QQmlJSMetaProperty prop {
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
        QQmlJSMetaMethod method;
        method.setMethodName(fexpr->name.toString());
        method.setMethodType(QQmlJSMetaMethod::Method);
        FormalParameterList *parameters = fexpr->formals;
        while (parameters) {
            method.addParameter(parameters->element->bindingIdentifier.toString(), QString());
            parameters = parameters->next;
        }
        currentObject()->addMethod(method);
    } else if (ClassExpression *clexpr = sourceElement->sourceElement->asClassDefinition()) {
        QQmlJSMetaProperty prop { clexpr->name.toString(), QString(), false, false, false, false, 1 };
        currentObject()->addProperty(prop);
    } else if (cast<VariableStatement *>(sourceElement->sourceElement)) {
        // nothing to do
    } else {
        const auto loc = sourceElement->firstSourceLocation();
        m_errors.append(
                    QStringLiteral("unsupportedd sourceElement at ")
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

bool ImportedMembersVisitor::visit(QQmlJS::AST::UiEnumDeclaration *uied)
{
    QQmlJSMetaEnum qmlEnum(uied->name.toString());
    for (const auto *member = uied->members; member; member = member->next)
        qmlEnum.addKey(member->member.toString());
    currentObject()->addEnum(qmlEnum);
    return true;
}

void ImportedMembersVisitor::throwRecursionDepthError()
{
    m_errors.append(QStringLiteral("Maximum statement or expression depth exceeded"));
}
