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

class IssueLocationWithContext
{
public:
    IssueLocationWithContext(const QString &code, const QQmlJS::SourceLocation &location) {
        int before = std::max(0,code.lastIndexOf(QLatin1Char('\n'), location.offset));
        m_beforeText = code.midRef(before + 1, int(location.offset - (before + 1)));
        m_issueText = code.midRef(location.offset, location.length);
        int after = code.indexOf(QLatin1Char('\n'), int(location.offset + location.length));
        m_afterText = code.midRef(int(location.offset + location.length),
                                  int(after - (location.offset+location.length)));
    }

    QStringRef beforeText() const { return m_beforeText; }
    QStringRef issueText() const { return m_issueText; }
    QStringRef afterText() const { return m_afterText; }

private:
    QStringRef m_beforeText;
    QStringRef m_issueText;
    QStringRef m_afterText;
};

static void writeWarning(ColorOutput *out)
{
    out->write(QLatin1String("Warning: "), Warning);
}

static const QStringList unknownBuiltins = {
    // TODO: "string" should be added to builtins.qmltypes, and the special handling below removed
    QStringLiteral("alias"),    // TODO: we cannot properly resolve aliases, yet
    QStringLiteral("QRectF"),   // TODO: should be added to builtins.qmltypes
    QStringLiteral("QFont"),    // TODO: should be added to builtins.qmltypes
    QStringLiteral("QJSValue"), // We cannot say anything intelligent about untyped JS values.
    QStringLiteral("variant"),  // Same for generic variants
};

void CheckIdentifiers::printContext(const QQmlJS::SourceLocation &location) const
{
    IssueLocationWithContext issueLocationWithContext {m_code, location};
    m_colorOut->write(issueLocationWithContext.beforeText().toString(), Normal);
    m_colorOut->write(issueLocationWithContext.issueText().toString(), Error);
    m_colorOut->write(issueLocationWithContext.afterText().toString() + QLatin1Char('\n'), Normal);
    int tabCount = issueLocationWithContext.beforeText().count(QLatin1Char('\t'));
    m_colorOut->write(QString::fromLatin1(" ").repeated(
                       issueLocationWithContext.beforeText().length() - tabCount)
                           + QString::fromLatin1("\t").repeated(tabCount)
                           + QString::fromLatin1("^").repeated(location.length)
                           + QLatin1Char('\n'), Normal);
}

bool CheckIdentifiers::checkMemberAccess(const QVector<ScopeTree::FieldMember> &members,
                                         const ScopeTree::ConstPtr &outerScope) const
{
    QStringList expectedNext;
    QString detectedRestrictiveName;
    QString detectedRestrictiveKind;

    ScopeTree::ConstPtr scope = outerScope;
    for (const ScopeTree::FieldMember &access : members) {
        if (scope.isNull()) {
            writeWarning(m_colorOut);
            m_colorOut->write(
                        QString::fromLatin1("Type \"%1\" of base \"%2\" not found when accessing member \"%3\" at %4:%5:%6.\n")
                        .arg(detectedRestrictiveKind)
                        .arg(detectedRestrictiveName)
                        .arg(access.m_name)
                        .arg(m_fileName)
                        .arg(access.m_location.startLine)
                        .arg(access.m_location.startColumn), Normal);
            printContext(access.m_location);
            return false;
        }

        const QString scopeName = scope->name().isEmpty() ? scope->className() : scope->name();

        if (!detectedRestrictiveKind.isEmpty()) {
            if (expectedNext.contains(access.m_name)) {
                expectedNext.clear();
                continue;
            }

            writeWarning(m_colorOut);
            m_colorOut->write(QString::fromLatin1(
                                   "\"%1\" is a %2. You cannot access \"%3\" on it at %4:%5:%6\n")
                                   .arg(detectedRestrictiveName)
                                   .arg(detectedRestrictiveKind)
                                   .arg(access.m_name)
                                   .arg(m_fileName)
                                   .arg(access.m_location.startLine)
                                   .arg(access.m_location.startColumn), Normal);
            printContext(access.m_location);
            return false;
        }

        const auto properties = scope->properties();
        const auto scopeIt = properties.find(access.m_name);
        if (scopeIt != properties.end()) {
            const QString typeName = access.m_parentType.isEmpty() ? scopeIt->typeName()
                                                                   : access.m_parentType;
            if (scopeIt->isList()) {
                detectedRestrictiveKind = QLatin1String("list");
                detectedRestrictiveName = access.m_name;
                expectedNext.append(QLatin1String("length"));
                continue;
            }

            if (typeName == QLatin1String("string")) {
                detectedRestrictiveKind = typeName;
                detectedRestrictiveName = access.m_name;
                expectedNext.append(QLatin1String("length"));
                continue;
            }

            if (const ScopeTree::ConstPtr type = scopeIt->type()) {
                if (access.m_parentType.isEmpty()) {
                    scope = type;
                    continue;
                }
            }

            if (unknownBuiltins.contains(typeName))
                return true;

            const auto it = m_types.find(typeName);
            if (it == m_types.end()) {
                detectedRestrictiveKind = typeName;
                detectedRestrictiveName = access.m_name;
                scope = nullptr;
            } else {
                scope = *it;
            }
            continue;
        }

        const auto methods = scope->methods();
        const auto scopeMethodIt = methods.find(access.m_name);
        if (scopeMethodIt != methods.end())
            return true; // Access to property of JS function

        const auto enums= scope->enums();
        for (const auto enumerator : enums) {
            for (const QString &key : enumerator.keys()) {
                if (access.m_name == key) {
                    detectedRestrictiveKind = QLatin1String("enum");
                    detectedRestrictiveName = access.m_name;
                    break;
                }
            }
            if (!detectedRestrictiveName.isEmpty())
                break;
        }
        if (!detectedRestrictiveName.isEmpty())
            continue;

        auto type = m_types.value(access.m_parentType.isEmpty() ? scopeName : access.m_parentType);
        bool typeFound = false;
        while (type) {
            const auto typeProperties = type->properties();
            const auto typeIt = typeProperties.find(access.m_name);
            if (typeIt != typeProperties.end()) {
                const ScopeTree::ConstPtr propType = typeIt->type();
                scope = propType ? propType : m_types.value(typeIt->typeName());
                typeFound = true;
                break;
            }

            const auto typeMethods = type->methods();
            const auto typeMethodIt = typeMethods.find(access.m_name);
            if (typeMethodIt != typeMethods.end()) {
                detectedRestrictiveName = access.m_name;
                detectedRestrictiveKind = QLatin1String("method");
                typeFound = true;
                break;
            }

            type = m_types.value(type->superclassName());
        }
        if (typeFound)
            continue;

        if (access.m_name.front().isUpper() && scope->scopeType() == ScopeType::QMLScope) {
            // may be an attached type
            const auto it = m_types.find(access.m_name);
            if (it != m_types.end() && !(*it)->attachedTypeName().isEmpty()) {
                const auto attached = m_types.find((*it)->attachedTypeName());
                if (attached != m_types.end()) {
                    scope = *attached;
                    continue;
                }
            }
        }

        writeWarning(m_colorOut);
        m_colorOut->write(QString::fromLatin1(
                               "Property \"%1\" not found on type \"%2\" at %3:%4:%5\n")
                               .arg(access.m_name)
                               .arg(scopeName)
                               .arg(m_fileName)
                               .arg(access.m_location.startLine)
                               .arg(access.m_location.startColumn), Normal);
        printContext(access.m_location);
        return false;
    }

    return true;
}

bool CheckIdentifiers::operator()(const QHash<QString, ScopeTree::ConstPtr> &qmlIDs,
                                  const ScopeTree::ConstPtr &root, const QString &rootId) const
{
    bool noUnqualifiedIdentifier = true;

    // revisit all scopes
    QQueue<ScopeTree::ConstPtr> workQueue;
    workQueue.enqueue(root);
    while (!workQueue.empty()) {
        const ScopeTree::ConstPtr currentScope = workQueue.dequeue();
        const auto unmatchedSignalHandlers = currentScope->unmatchedSignalHandlers();
        for (const auto &handler : unmatchedSignalHandlers) {
            writeWarning(m_colorOut);
            m_colorOut->write(QString::fromLatin1(
                                   "no matching signal found for handler \"%1\" at %2:%3:%4\n")
                                   .arg(handler.first).arg(m_fileName).arg(handler.second.startLine)
                                   .arg(handler.second.startColumn), Normal);
            printContext(handler.second);
        }

        const auto memberAccessChains = currentScope->memberAccessChains();
        for (auto memberAccessChain : memberAccessChains) {
            if (memberAccessChain.isEmpty())
                continue;

            const auto memberAccessBase = memberAccessChain.takeFirst();
            if (currentScope->isIdInCurrentJSScopes(memberAccessBase.m_name))
                continue;

            auto it = qmlIDs.find(memberAccessBase.m_name);
            if (it != qmlIDs.end()) {
                if (*it != nullptr) {
                    if (!checkMemberAccess(memberAccessChain, *it))
                        noUnqualifiedIdentifier = false;
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
                                noUnqualifiedIdentifier = false;
                            continue;
                        }
                    }
                }
            }

            auto qmlScope = ScopeTree::findCurrentQMLScope(currentScope);
            if (qmlScope->methods().contains(memberAccessBase.m_name)) {
                // a property of a JavaScript function
                continue;
            }

            const auto properties = qmlScope->properties();
            const auto qmlIt = properties.find(memberAccessBase.m_name);
            if (qmlIt != properties.end()) {
                if (memberAccessChain.isEmpty() || unknownBuiltins.contains(qmlIt->typeName()))
                    continue;

                if (!qmlIt->type()) {
                    writeWarning(m_colorOut);
                    m_colorOut->write(QString::fromLatin1(
                                           "Type of property \"%2\" not found at %3:%4:%5\n")
                                           .arg(memberAccessBase.m_name)
                                           .arg(m_fileName)
                                           .arg(memberAccessBase.m_location.startLine)
                                           .arg(memberAccessBase.m_location.startColumn), Normal);
                    printContext(memberAccessBase.m_location);
                    noUnqualifiedIdentifier = false;
                } else if (!checkMemberAccess(memberAccessChain, qmlIt->type())) {
                    noUnqualifiedIdentifier = false;
                }

                continue;
            }

            // TODO: Lots of builtins are missing
            if (memberAccessBase.m_name == QLatin1String("Qt"))
                continue;

            const auto typeIt = m_types.find(memberAccessBase.m_name);
            if (typeIt != m_types.end()) {
                if (!checkMemberAccess(memberAccessChain, *typeIt))
                    noUnqualifiedIdentifier = false;
                continue;
            }

            noUnqualifiedIdentifier = false;
            writeWarning(m_colorOut);
            const auto location = memberAccessBase.m_location;
            m_colorOut->write(QString::fromLatin1("unqualified access at %1:%2:%3\n")
                           .arg(m_fileName)
                           .arg(location.startLine).arg(location.startColumn),
                           Normal);

            printContext(location);

            // root(JS) --> program(qml) --> (first element)
            const auto firstElement = root->childScopes()[0]->childScopes()[0];
            if (firstElement->properties().contains(memberAccessBase.m_name)
                    || firstElement->methods().contains(memberAccessBase.m_name)
                    || firstElement->enums().contains(memberAccessBase.m_name)) {
                m_colorOut->write(QLatin1String("Note: "), Info);
                m_colorOut->write(memberAccessBase.m_name + QLatin1String(" is a member of the root element\n"), Normal );
                m_colorOut->write(QLatin1String("      You can qualify the access with its id to avoid this warning:\n"), Normal);
                if (rootId == QLatin1String("<id>")) {
                    m_colorOut->write(QLatin1String("Note: "), Warning);
                    m_colorOut->write(QLatin1String("You first have to give the root element an id\n"));
                }
                IssueLocationWithContext issueLocationWithContext {m_code, location};
                m_colorOut->write(issueLocationWithContext.beforeText().toString(), Normal);
                m_colorOut->write(rootId + QLatin1Char('.'), Hint);
                m_colorOut->write(issueLocationWithContext.issueText().toString(), Normal);
                m_colorOut->write(issueLocationWithContext.afterText() + QLatin1Char('\n'), Normal);
            } else if (currentScope->isIdInjectedFromSignal(memberAccessBase.m_name)) {
                auto methodUsages = ScopeTree::findCurrentQMLScope(currentScope)
                        ->injectedSignalIdentifiers().values(memberAccessBase.m_name);
                auto location = memberAccessBase.m_location;
                // sort the list of signal handlers by their occurrence in the source code
                // then, we select the first one whose location is after the unqualified id
                // and go one step backwards to get the one which we actually need
                std::sort(methodUsages.begin(), methodUsages.end(),
                          [](const MethodUsage &m1, const MethodUsage &m2) {
                    return m1.loc.startLine < m2.loc.startLine
                            || (m1.loc.startLine == m2.loc.startLine
                                && m1.loc.startColumn < m2.loc.startColumn);
                });
                auto oneBehindIt = std::find_if(methodUsages.begin(), methodUsages.end(),
                                                [&location](const MethodUsage &methodUsage) {
                    return location.startLine < methodUsage.loc.startLine
                            || (location.startLine == methodUsage.loc.startLine
                                && location.startColumn < methodUsage.loc.startColumn);
                });
                auto methodUsage = *(--oneBehindIt);
                m_colorOut->write(QLatin1String("Note: "), Info);
                m_colorOut->write(
                            memberAccessBase.m_name + QString::fromLatin1(
                                " is accessible in this scope because "
                                "you are handling a signal at %1:%2:%3\n")
                            .arg(m_fileName)
                            .arg(methodUsage.loc.startLine).arg(methodUsage.loc.startColumn),
                            Normal);
                m_colorOut->write(QLatin1String("Consider using a function instead\n"), Normal);
                IssueLocationWithContext context {m_code, methodUsage.loc};
                m_colorOut->write(context.beforeText() + QLatin1Char(' '));
                m_colorOut->write(QLatin1String(methodUsage.hasMultilineHandlerBody
                                             ? "function("
                                             : "("),
                               Hint);
                const auto parameters = methodUsage.method.parameterNames();
                for (int numParams = parameters.size(); numParams > 0; --numParams) {
                    m_colorOut->write(parameters.at(parameters.size() - numParams), Hint);
                    if (numParams > 1)
                        m_colorOut->write(QLatin1String(", "), Hint);
                }
                m_colorOut->write(QLatin1String(methodUsage.hasMultilineHandlerBody ? ")" : ") => "),
                               Hint);
                m_colorOut->write(QLatin1String(" {..."), Normal);
            }
            m_colorOut->write(QLatin1String("\n\n\n"), Normal);
        }
        const auto childScopes = currentScope->childScopes();
        for (auto const &childScope : childScopes)
            workQueue.enqueue(childScope);
    }
    return noUnqualifiedIdentifier;
}
