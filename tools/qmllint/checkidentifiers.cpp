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
#include "qcoloroutput.h"

#include <QtCore/qqueue.h>
#include <QtCore/qsharedpointer.h>
#include <stack>

class IssueLocationWithContext
{
public:
    IssueLocationWithContext(const QString &code, const QQmlJS::SourceLocation &location) {
        int before = qMax(0,code.lastIndexOf(QLatin1Char('\n'), location.offset));
        m_beforeText = QStringView{code}.mid(before + 1, int(location.offset - (before + 1)));
        m_issueText = QStringView{code}.mid(location.offset, location.length);
        int after = code.indexOf(QLatin1Char('\n'), int(location.offset + location.length));
        m_afterText = QStringView{code}.mid(int(location.offset + location.length),
                                  int(after - (location.offset+location.length)));
    }

    QStringView beforeText() const { return m_beforeText; }
    QStringView issueText() const { return m_issueText; }
    QStringView afterText() const { return m_afterText; }

private:
    QStringView m_beforeText;
    QStringView m_issueText;
    QStringView m_afterText;
};

static const QStringList unknownBuiltins = {
    QStringLiteral("alias"),    // TODO: we cannot properly resolve aliases, yet
    QStringLiteral("QJSValue"), // We cannot say anything intelligent about untyped JS values.

    // Same for generic variants
    QStringLiteral("variant"),
    QStringLiteral("var")
};

void CheckIdentifiers::printContext(
        const QString &code, ColorOutput *output, const QQmlJS::SourceLocation &location)
{
    IssueLocationWithContext issueLocationWithContext { code, location };
    output->write(issueLocationWithContext.beforeText().toString(), Normal);
    output->write(issueLocationWithContext.issueText().toString(), Error);
    output->write(issueLocationWithContext.afterText().toString() + QLatin1Char('\n'), Normal);
    int tabCount = issueLocationWithContext.beforeText().count(QLatin1Char('\t'));
    output->write(QString::fromLatin1(" ").repeated(
                       issueLocationWithContext.beforeText().length() - tabCount)
                           + QString::fromLatin1("\t").repeated(tabCount)
                           + QString::fromLatin1("^").repeated(location.length)
                           + QLatin1Char('\n'), Normal);
}

template<typename Visitor>
static bool walkRelatedScopes(QQmlJSScope::ConstPtr rootType, const Visitor &visit)
{
    if (rootType.isNull())
        return false;
    std::stack<QQmlJSScope::ConstPtr> stack;
    stack.push(rootType);

    while (!stack.empty()) {
        const auto type = stack.top();
        stack.pop();

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

bool CheckIdentifiers::checkMemberAccess(const QVector<FieldMember> &members,
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
    for (const FieldMember &access : members) {
        if (scope.isNull()) {
            m_colorOut->writePrefixedMessage(
                        QString::fromLatin1("Type \"%1\" of base \"%2\" not found when accessing member \"%3\" at %4:%5:%6.\n")
                        .arg(detectedRestrictiveKind)
                        .arg(detectedRestrictiveName)
                        .arg(access.m_name)
                        .arg(m_fileName)
                        .arg(access.m_location.startLine)
                        .arg(access.m_location.startColumn), Warning);
            printContext(m_code, m_colorOut, access.m_location);
            return false;
        }

        if (!detectedRestrictiveKind.isEmpty()) {
            if (expectedNext.contains(access.m_name)) {
                expectedNext.clear();
                continue;
            }

            m_colorOut->writePrefixedMessage(QString::fromLatin1(
                                   "\"%1\" is a %2. You cannot access \"%3\" on it at %4:%5:%6\n")
                                   .arg(detectedRestrictiveName)
                                   .arg(detectedRestrictiveKind)
                                   .arg(access.m_name)
                                   .arg(m_fileName)
                                   .arg(access.m_location.startLine)
                                   .arg(access.m_location.startColumn), Warning);
            printContext(m_code, m_colorOut, access.m_location);
            return false;
        }

        const auto property = scope->property(access.m_name);
        if (!property.propertyName().isEmpty()) {
            const QString typeName = access.m_parentType.isEmpty() ? property.typeName()
                                                                   : access.m_parentType;
            if (property.isList()) {
                detectedRestrictiveKind = QLatin1String("list");
                detectedRestrictiveName = access.m_name;
                expectedNext.append(QLatin1String("length"));
                continue;
            }

            if (typeName == QLatin1String("QString")) {
                detectedRestrictiveKind = typeName;
                detectedRestrictiveName = access.m_name;
                expectedNext.append(QLatin1String("length"));
                continue;
            }

            if (access.m_parentType.isEmpty()) {
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
                return true;

            const auto it = m_types.find(typeName);
            if (it == m_types.end()) {
                detectedRestrictiveKind = typeName;
                detectedRestrictiveName = access.m_name;
                scope = QQmlJSScope::ConstPtr();
            } else {
                scope = *it;
            }

            continue;
        }

        if (scope->hasMethod(access.m_name))
            return true; // Access to property of JS function

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
            rootType = m_types.value(access.m_parentType);
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
            const auto it = m_types.find(access.m_name);
            if (it != m_types.end() && *it && !(*it)->attachedTypeName().isEmpty()) {
                if (const auto attached = (*it)->attachedType()) {
                    scope = attached;
                    continue;
                }
            }
        }

        m_colorOut->writePrefixedMessage(QString::fromLatin1(
                               "Property \"%1\" not found on type \"%2\" at %3:%4:%5\n")
                               .arg(access.m_name)
                               .arg(scope->internalName().isEmpty()
                                    ? scope->baseTypeName() : scope->internalName())
                               .arg(m_fileName)
                               .arg(access.m_location.startLine)
                               .arg(access.m_location.startColumn), Warning);
        printContext(m_code, m_colorOut, access.m_location);
        return false;
    }

    return true;
}

bool CheckIdentifiers::operator()(
        const QHash<QString, QQmlJSScope::ConstPtr> &qmlIDs,
        const QHash<QQmlJS::SourceLocation, SignalHandler> &signalHandlers,
        const MemberAccessChains &memberAccessChains,
        const QQmlJSScope::ConstPtr &root, const QString &rootId) const
{
    bool identifiersClean = true;

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
                    m_colorOut->writePrefixedMessage(
                                QStringLiteral(
                                    "Variable \"%1\" is used before its declaration at %2:%3. "
                                    "The declaration is at %4:%5.\n")
                                           .arg(memberAccessBase.m_name)
                                           .arg(memberAccessBase.m_location.startLine)
                                           .arg(memberAccessBase.m_location.startColumn)
                                           .arg(jsId->location.startLine)
                                           .arg(jsId->location.startColumn), Warning);
                    printContext(m_code, m_colorOut, memberAccessBase.m_location);
                    identifiersClean = false;
                }
                continue;
            }

            auto it = qmlIDs.find(memberAccessBase.m_name);
            if (it != qmlIDs.end()) {
                if (!it->isNull()) {
                    if (!checkMemberAccess(memberAccessChain, *it))
                        identifiersClean = false;
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
                            if (!checkMemberAccess(memberAccessChain, *typeIt))
                                identifiersClean = false;
                            continue;
                        }
                    }
                }
            }

            auto qmlScope = QQmlJSScope::findCurrentQMLScope(currentScope);
            if (qmlScope->hasMethod(memberAccessBase.m_name)) {
                // a property of a JavaScript function, or a method
                continue;
            }

            const auto property = qmlScope->property(memberAccessBase.m_name);
            if (!property.propertyName().isEmpty()) {
                if (memberAccessChain.isEmpty() || unknownBuiltins.contains(property.typeName()))
                    continue;

                if (!property.type()) {
                    m_colorOut->writePrefixedMessage(QString::fromLatin1(
                                           "Type of property \"%2\" not found at %3:%4:%5\n")
                                           .arg(memberAccessBase.m_name)
                                           .arg(m_fileName)
                                           .arg(memberAccessBase.m_location.startLine)
                                           .arg(memberAccessBase.m_location.startColumn), Warning);
                    printContext(m_code, m_colorOut, memberAccessBase.m_location);
                    identifiersClean = false;
                } else if (!checkMemberAccess(memberAccessChain, property.type(), &property)) {
                    identifiersClean = false;
                }

                continue;
            }

            const QString baseName = memberAccessBase.m_name;
            auto typeIt = m_types.find(memberAccessBase.m_name);
            bool baseIsPrefixed = false;
            while (typeIt != m_types.end() && typeIt->isNull()) {
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

            if (typeIt != m_types.end() && !typeIt->isNull()) {
                if (!checkMemberAccess(memberAccessChain, *typeIt))
                    identifiersClean = false;
                continue;
            }

            identifiersClean = false;
            const auto location = memberAccessBase.m_location;

            if (baseIsPrefixed) {
                m_colorOut->writePrefixedMessage(
                            QString::fromLatin1("type not found in namespace at %1:%2:%3\n")
                               .arg(m_fileName)
                               .arg(location.startLine).arg(location.startColumn),
                               Warning);
            } else {
                m_colorOut->writePrefixedMessage(
                            QString::fromLatin1("unqualified access at %1:%2:%3\n")
                               .arg(m_fileName)
                               .arg(location.startLine).arg(location.startColumn),
                               Warning);
            }

            printContext(m_code, m_colorOut, location);

            // root(JS) --> (first element)
            const auto firstElement = root->childScopes()[0];
            if (firstElement->hasProperty(memberAccessBase.m_name)
                    || firstElement->hasMethod(memberAccessBase.m_name)
                    || firstElement->hasEnumeration(memberAccessBase.m_name)) {
                m_colorOut->writePrefixedMessage(
                            memberAccessBase.m_name
                            + QLatin1String(" is a member of the root element\n")
                            + QLatin1String("      You can qualify the access with its id "
                                            "to avoid this warning:\n"),
                            Info, QStringLiteral("Note"));
                if (rootId == QLatin1String("<id>")) {
                    m_colorOut->writePrefixedMessage(
                                QLatin1String("You first have to give the root element an id\n"),
                                Warning, QStringLiteral("Note"));
                }
                IssueLocationWithContext issueLocationWithContext {m_code, location};
                m_colorOut->write(issueLocationWithContext.beforeText().toString(), Normal);
                m_colorOut->write(rootId + QLatin1Char('.'), Hint);
                m_colorOut->write(issueLocationWithContext.issueText().toString(), Normal);
                m_colorOut->write(issueLocationWithContext.afterText() + QLatin1Char('\n'), Normal);
            } else if (jsId.has_value()
                       && jsId->kind == QQmlJSScope::JavaScriptIdentifier::Injected) {
                const QQmlJSScope::JavaScriptIdentifier id = jsId.value();
                m_colorOut->writePrefixedMessage(
                            memberAccessBase.m_name + QString::fromLatin1(
                                " is accessible in this scope because "
                                "you are handling a signal at %1:%2:%3\n")
                            .arg(m_fileName)
                            .arg(id.location.startLine).arg(id.location.startColumn),
                            Info, QStringLiteral("Note"));
                m_colorOut->write(QLatin1String("Consider using a function instead\n"), Normal);
                IssueLocationWithContext context {m_code, id.location};
                m_colorOut->write(context.beforeText() + QLatin1Char(' '));

                const auto handler = signalHandlers[id.location];

                m_colorOut->write(QLatin1String(handler.isMultiline ? "function(" : "("), Hint);
                const auto parameters = handler.signal.parameterNames();
                for (int numParams = parameters.size(); numParams > 0; --numParams) {
                    m_colorOut->write(parameters.at(parameters.size() - numParams), Hint);
                    if (numParams > 1)
                        m_colorOut->write(QLatin1String(", "), Hint);
                }
                m_colorOut->write(QLatin1String(handler.isMultiline ? ")" : ") => "), Hint);
                m_colorOut->write(QLatin1String(" {..."), Normal);
            }
            m_colorOut->write(QLatin1String("\n\n\n"), Normal);
        }
        const auto childScopes = currentScope->childScopes();
        for (auto const &childScope : childScopes)
            workQueue.enqueue(childScope);
    }
    return identifiersClean;
}
