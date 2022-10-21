/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "checkidentifiers.h"

#include <QtQmlCompiler/private/qcoloroutput_p.h>

#include <QtCore/qqueue.h>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/qsharedpointer.h>
#include <stack>

static const QStringList unknownBuiltins = {
    QStringLiteral("alias"),    // TODO: we cannot properly resolve aliases, yet
    QStringLiteral("QJSValue"), // We cannot say anything intelligent about untyped JS values.
    QStringLiteral("QVariant")  // Same for generic variants
};

template<typename Visitor>
static bool walkRelatedScopes(QQmlJSScope::ConstPtr rootType, const Visitor &visit)
{
    if (rootType.isNull())
        return false;
    std::stack<QQmlJSScope::ConstPtr> stack;
    QDuplicateTracker<QQmlJSScope::ConstPtr> seenTypes;
    stack.push(rootType);

    while (!stack.empty()) {
        const auto type = stack.top();
        stack.pop();

        // If we've seen this type before (can be caused by self attaching types), ignore it
        if (seenTypes.hasSeen(type))
            continue;

        if (visit(type))
            return true;

        if (auto attachedType = type->attachedType())
            stack.push(attachedType);

        if (auto baseType = type->baseType())
            stack.push(baseType);

        // Push extension type last. It overrides the base type.
        if (auto extensionType = type->extensionType())
            stack.push(extensionType);
    }

    return false;
}

void CheckIdentifiers::checkMemberAccess(const QVector<FieldMember> &members,
                                         const QQmlJSScope::ConstPtr &outerScope,
                                         const QQmlJSMetaProperty *prop) const
{

    QStringList expectedNext;
    QString detectedRestrictiveName;
    QString detectedRestrictiveKind;

    if (prop != nullptr && prop->isList()) {
        detectedRestrictiveKind = QLatin1String("list");
        expectedNext.append(QLatin1String("length"));
    }

    QQmlJSScope::ConstPtr scope = outerScope;
    for (qsizetype i = 0; i < members.size(); i++) {
        const FieldMember &access = members.at(i);

        if (scope.isNull()) {
            m_logger->log(
                        QString::fromLatin1("Type \"%1\" of base \"%2\" not found when accessing member \"%3\"")
                        .arg(detectedRestrictiveKind)
                        .arg(detectedRestrictiveName)
                        .arg(access.m_name), Log_Type, access.m_location);
            return;
        }

        if (!detectedRestrictiveKind.isEmpty()) {
            if (expectedNext.contains(access.m_name)) {
                expectedNext.clear();
                continue;
            }

            m_logger->log(
                        QLatin1String("\"%1\" is a %2. You cannot access \"%3\" it from here")
                        .arg(detectedRestrictiveName)
                        .arg(detectedRestrictiveKind)
                        .arg(access.m_name), Log_Type, access.m_location);
            return;
        }

        if (unknownBuiltins.contains(scope->internalName()))
            return;

        const auto property = scope->property(access.m_name);
        if (!property.propertyName().isEmpty()) {
            const auto binding = scope->propertyBinding(access.m_name);
            const QString typeName = access.m_parentType.isEmpty()
                    ? (binding.hasValue() ? binding.valueTypeName() : property.typeName())
                    : access.m_parentType;

            if (property.isList()) {
                detectedRestrictiveKind = QLatin1String("list");
                detectedRestrictiveName = access.m_name;
                expectedNext.append(QLatin1String("length"));
                continue;
            }

            if (access.m_parentType.isEmpty()) {
                if (binding.hasValue())
                    scope = binding.value();
                else
                    scope = property.type();

                if (scope.isNull()) {
                    // Properties should always have a type. Otherwise something
                    // was missing from the import already.
                    detectedRestrictiveKind = typeName;
                    detectedRestrictiveName = access.m_name;
                }
                continue;
            }

            if (unknownBuiltins.contains(typeName))
                return;

            const auto it = m_types.find(typeName);
            if (it == m_types.end()) {
                detectedRestrictiveKind = typeName;
                detectedRestrictiveName = access.m_name;
                scope = QQmlJSScope::ConstPtr();
            } else {
                scope = it->scope;
            }

            continue;
        }

        if (scope->hasMethod(access.m_name))
            return; // Access to property of JS function

        auto checkEnums = [&](const QQmlJSScope::ConstPtr &scope) {
            if (scope->hasEnumeration(access.m_name)) {
                detectedRestrictiveKind = QLatin1String("enum");
                detectedRestrictiveName = access.m_name;
                expectedNext.append(scope->enumeration(access.m_name).keys());
                return true;
            }

            if (scope->hasEnumerationKey(access.m_name)) {
                detectedRestrictiveKind = QLatin1String("enum");
                detectedRestrictiveName = access.m_name;
                return true;
            }

            return false;
        };

        checkEnums(scope);

        if (!detectedRestrictiveName.isEmpty())
            continue;

        QQmlJSScope::ConstPtr rootType;
        if (!access.m_parentType.isEmpty())
            rootType = m_types.value(access.m_parentType).scope;
        else
            rootType = scope;

        bool typeFound =
                walkRelatedScopes(rootType, [&](QQmlJSScope::ConstPtr type) {
                    const auto typeProperties = type->ownProperties();
                    const auto typeIt = typeProperties.find(access.m_name);
                    if (typeIt != typeProperties.end()) {
                        scope = typeIt->type();
                        return true;
                    }

                    const auto typeMethods = type->ownMethods();
                    const auto typeMethodIt = typeMethods.find(access.m_name);
                    if (typeMethodIt != typeMethods.end()) {
                        detectedRestrictiveName = access.m_name;
                        detectedRestrictiveKind = QLatin1String("method");
                        return true;
                    }

                    return checkEnums(type);
                });
        if (typeFound)
            continue;

        if (access.m_name.front().isUpper() && scope->scopeType() == QQmlJSScope::QMLScope) {
            // may be an attached type

            auto it = m_types.find(access.m_name);

            // Something was found but it wasn't the attached type we were looking for, it could be a prefix
            if (it != m_types.end() && !it->scope && i+1 < members.length()) {
                // See whether this is due to us getting the prefixed property in two accesses (i.e. "T" and "Item")
                // by checking again with a fixed name.
                it = m_types.find(access.m_name + QLatin1Char('.') + members[++i].m_name);

                if (it == m_types.end() || !it->scope || it->scope->attachedTypeName().isEmpty())
                    --i;
            }

            if (it != m_types.end() && it->scope && !it->scope->attachedTypeName().isEmpty()) {
                if (const auto attached = it->scope->attachedType()) {
                    scope = attached;
                    continue;
                }
            }
        }

        m_logger->log(QLatin1String(
                          "Property \"%1\" not found on type \"%2\"")
                      .arg(access.m_name)
                      .arg(scope->internalName().isEmpty()
                           ? scope->baseTypeName() : scope->internalName()), Log_Type, access.m_location);
        return;
    }
}

void CheckIdentifiers::operator()(
        const QQmlJSScopesById &qmlIDs,
        const QHash<QQmlJS::SourceLocation, QQmlJSMetaSignalHandler> &signalHandlers,
        const MemberAccessChains &memberAccessChains, const QQmlJSScope::ConstPtr &root,
        const QString &rootId) const
{
    // revisit all scopes
    QQueue<QQmlJSScope::ConstPtr> workQueue;
    workQueue.enqueue(root);
    while (!workQueue.empty()) {
        const QQmlJSScope::ConstPtr currentScope = workQueue.dequeue();

        const auto scopeMemberAccessChains = memberAccessChains[currentScope];
        for (auto memberAccessChain : scopeMemberAccessChains) {
            if (memberAccessChain.isEmpty())
                continue;

            auto memberAccessBase = memberAccessChain.takeFirst();
            const auto jsId = currentScope->findJSIdentifier(memberAccessBase.m_name);
            if (jsId.has_value() && jsId->kind != QQmlJSScope::JavaScriptIdentifier::Injected) {
                if (memberAccessBase.m_location.end() < jsId->location.begin()) {
                    // TODO: Is there a more fitting category?
                    m_logger->log(
                            QStringLiteral("Variable \"%1\" is used here before its declaration. "
                                           "The declaration is at %4:%5.")
                                    .arg(memberAccessBase.m_name)
                                    .arg(jsId->location.startLine)
                                    .arg(jsId->location.startColumn),
                            Log_Type, memberAccessBase.m_location);
                }
                continue;
            }

            if (!memberAccessBase.m_name.isEmpty()) {
                auto scope = qmlIDs.scope(memberAccessBase.m_name, currentScope);
                if (!scope.isNull()) {
                    checkMemberAccess(memberAccessChain, scope);
                    continue;
                } else if (!memberAccessChain.isEmpty()) {
                    // It could be a qualified type name
                    const QString scopedName = memberAccessChain.first().m_name;
                    if (scopedName.front().isUpper()) {
                        const QString qualified = memberAccessBase.m_name + QLatin1Char('.')
                                + scopedName;
                        const auto typeIt = m_types.find(qualified);
                        if (typeIt != m_types.end()) {
                            memberAccessChain.takeFirst();
                            checkMemberAccess(memberAccessChain, typeIt->scope);
                            continue;
                        }
                    }
                }
            }

            auto qmlScope = QQmlJSScope::findCurrentQMLScope(currentScope);
            if (qmlScope->hasMethod(memberAccessBase.m_name)) {
                // a property of a JavaScript function, or a method
                auto methods = qmlScope->methods(memberAccessBase.m_name);
                const QQmlJSMetaMethod &method = methods.constFirst();
                const auto &annotations = method.annotations();
                auto deprecationAnn = std::find_if(annotations.constBegin(), annotations.constEnd(), [](const QQmlJSAnnotation& annotation) {
                    return annotation.isDeprecation();
                });

                // Once we encountered one possible method that is not deprecated,
                // we can assume that the one beyond that is not what was being referenced
                if (deprecationAnn == annotations.constEnd())
                    continue;

                QQQmlJSDeprecation deprecation = deprecationAnn->deprecation();

                QString message = QStringLiteral("Method \"%1(%2)\" is deprecated")
                        .arg(memberAccessBase.m_name, method.parameterNames().join(QStringLiteral(", ")));

                if (!deprecation.reason.isEmpty())
                    message.append(QStringLiteral(" (Reason: %1)").arg(deprecation.reason));

                m_logger->log(message, Log_Deprecation, memberAccessBase.m_location);
                continue;
            }

            const auto property = qmlScope->property(memberAccessBase.m_name);
            if (!property.propertyName().isEmpty()) {
                for (const QQmlJSAnnotation &annotation : property.annotations()) {
                    if (annotation.isDeprecation()) {
                        QQQmlJSDeprecation deprecation = annotation.deprecation();

                        QString message = QStringLiteral("Property \"%1\" is deprecated")
                                .arg(memberAccessBase.m_name);

                        if (!deprecation.reason.isEmpty())
                            message.append(QStringLiteral(" (Reason: %1)").arg(deprecation.reason));

                        m_logger->log(message, Log_Deprecation, memberAccessBase.m_location);
                    }
                }

                if (memberAccessChain.isEmpty() || unknownBuiltins.contains(property.typeName()))
                    continue;

                const auto binding = qmlScope->propertyBinding(memberAccessBase.m_name);
                if (binding.hasValue()) {
                    checkMemberAccess(memberAccessChain, binding.value(), &property);
                } else if (!property.type()) {
                    m_logger->log(QString::fromLatin1(
                                      "Type of property \"%2\" not found")
                                  .arg(memberAccessBase.m_name), Log_Type, memberAccessBase.m_location);
                } else {
                    checkMemberAccess(memberAccessChain, property.type(), &property);
                }

                continue;
            }

            const QString baseName = memberAccessBase.m_name;
            auto typeIt = m_types.find(memberAccessBase.m_name);
            bool baseIsPrefixed = false;
            while (typeIt != m_types.end() && typeIt->scope.isNull()) {
                // This is a namespaced import. Check with the full name.
                if (!memberAccessChain.isEmpty()) {
                    auto location = memberAccessBase.m_location;
                    memberAccessBase = memberAccessChain.takeFirst();
                    memberAccessBase.m_name.prepend(baseName + u'.');
                    location.length = memberAccessBase.m_location.offset - location.offset
                            + memberAccessBase.m_location.length;
                    memberAccessBase.m_location = location;
                    typeIt = m_types.find(memberAccessBase.m_name);
                    baseIsPrefixed = true;
                }
            }

            if (typeIt != m_types.end() && !typeIt->scope.isNull()) {
                checkMemberAccess(memberAccessChain, typeIt->scope);
                continue;
            }

            // If we're in a custom parser component (or one of their children) we cannot be sure
            // that this is really an unqualified access. We have to err on the side of producing
            // false negatives for the sake of usability.
            if (qmlScope->isInCustomParserParent()) {
                // We can handle Connections properly
                if (qmlScope->baseType()
                    && qmlScope->baseType()->internalName() != u"QQmlConnections"_qs)
                    continue;
            }

            const auto location = memberAccessBase.m_location;

            if (baseIsPrefixed) {
                m_logger->log(
                            QLatin1String("Type not found in namespace"),
                            Log_Type, location);
            } else {
                m_logger->log(
                            QLatin1String("Unqualified access"),
                            Log_UnqualifiedAccess, location);
            }

            // root(JS) --> (first element)
            const auto firstElement = root->childScopes()[0];

            QColorOutput &colorOut = m_logger->colorOutput();

            if (!m_logger->isCategoryDisabled(Log_UnqualifiedAccess) &&
                    (firstElement->hasProperty(memberAccessBase.m_name)
                    || firstElement->hasMethod(memberAccessBase.m_name)
                    || firstElement->hasEnumeration(memberAccessBase.m_name))) {

                colorOut.writePrefixedMessage(
                            memberAccessBase.m_name
                            + QLatin1String(" is a member of the root element\n")
                            + QLatin1String("      You can qualify the access with its id "
                                            "to avoid this warning:\n"),
                            QtInfoMsg, QStringLiteral("Note"));
                IssueLocationWithContext issueLocationWithContext {m_code, location};
                colorOut.write(issueLocationWithContext.beforeText().toString());
                colorOut.write(rootId + QLatin1Char('.'), QtDebugMsg);
                colorOut.write(issueLocationWithContext.issueText().toString());
                colorOut.write(issueLocationWithContext.afterText() + QLatin1String("\n\n"));

                if (rootId == QLatin1String("<id>")) {
                    colorOut.writePrefixedMessage(
                                QLatin1String("You first have to give the root element an id\n"),
                                QtInfoMsg, QStringLiteral("Note"));
                }

                colorOut.write(QLatin1String("\n\n\n"));
            } else if (jsId.has_value()
                       && jsId->kind == QQmlJSScope::JavaScriptIdentifier::Injected) {
                const QQmlJSScope::JavaScriptIdentifier id = jsId.value();
                colorOut.writePrefixedMessage(
                            memberAccessBase.m_name + QString::fromLatin1(
                                " is accessible in this scope because "
                                "you are handling a signal at %1:%2:%3\n")
                            .arg(m_fileName)
                            .arg(id.location.startLine).arg(id.location.startColumn),
                            QtInfoMsg, QStringLiteral("Note"));
                colorOut.write(QLatin1String("Consider using a function instead\n"));
                IssueLocationWithContext context {m_code, id.location};
                colorOut.write(context.beforeText() + QLatin1Char(' '));

                const auto handler = signalHandlers[id.location];

                colorOut.write(QLatin1String(handler.isMultiline ? "function(" : "("), QtInfoMsg);
                const auto parameters = handler.signalParameters;
                for (int numParams = parameters.size(); numParams > 0; --numParams) {
                    colorOut.write(parameters.at(parameters.size() - numParams), QtInfoMsg);
                    if (numParams > 1)
                        colorOut.write(QLatin1String(", "), QtInfoMsg);
                }
                colorOut.write(QLatin1String(handler.isMultiline ? ")" : ") => "), QtInfoMsg);
                colorOut.write(QLatin1String(" {...\n\n\n"));
            }
        }
        const auto childScopes = currentScope->childScopes();
        for (auto const &childScope : childScopes)
            workQueue.enqueue(childScope);
    }
}
