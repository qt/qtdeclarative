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

#include "findwarnings.h"

#include <QtQmlCompiler/private/qqmljsscope_p.h>
#include <QtQmlCompiler/private/qqmljstypedescriptionreader_p.h>
#include <QtQmlCompiler/private/qqmljstypereader_p.h>

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmlimportresolver_p.h>

#include <QtCore/qfile.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qscopedvaluerollback.h>

bool FindWarningVisitor::visit(QQmlJS::AST::UiObjectDefinition *uiod)
{
    QQmlJSImportVisitor::visit(uiod);

    const QString name = m_currentScope->baseTypeName();
    if (name.endsWith(u"Connections"_qs)) {
        QString target;
        auto member = uiod->initializer->members;
        while (member) {
            if (member->member->kind == QQmlJS::AST::Node::Kind_UiScriptBinding) {
                auto asBinding = static_cast<QQmlJS::AST::UiScriptBinding *>(member->member);
                if (asBinding->qualifiedId->name == u"target"_qs) {
                    if (asBinding->statement->kind == QQmlJS::AST::Node::Kind_ExpressionStatement) {
                        auto expr = static_cast<QQmlJS::AST::ExpressionStatement *>(
                                            asBinding->statement)
                                            ->expression;
                        if (auto idexpr =
                                    QQmlJS::AST::cast<QQmlJS::AST::IdentifierExpression *>(expr)) {
                            target = idexpr->name.toString();
                        } else {
                            // more complex expressions are not supported
                        }
                    }
                    break;
                }
            }
            member = member->next;
        }
        QQmlJSScope::ConstPtr targetScope;
        if (target.isEmpty()) {
            // no target set, connection comes from parentF
            QQmlJSScope::Ptr scope = m_currentScope;
            do {
                if (auto parentScope = scope->parentScope(); !parentScope.isNull())
                    scope = parentScope;
                else
                    break;
            } while (scope->scopeType() != QQmlJSScope::QMLScope);
            targetScope = m_rootScopeImports.value(scope->baseTypeName());
        } else {
            // there was a target, check if we already can find it
            auto scopeIt = m_scopesById.find(target);
            if (scopeIt != m_scopesById.end()) {
                targetScope = *scopeIt;
            } else {
                m_outstandingConnections.push_back({ target, m_currentScope, uiod });
                return false; // visit children later once target is known
            }
        }
        for (const auto scope = targetScope; targetScope; targetScope = targetScope->baseType()) {
            const auto connectionMethods = targetScope->ownMethods();
            for (const auto &method : connectionMethods)
                m_currentScope->addOwnMethod(method);
        }
    }

    m_objectDefinitionScopes << m_currentScope;
    return true;
}

void FindWarningVisitor::endVisit(QQmlJS::AST::UiObjectDefinition *uiod)
{
    auto childScope = m_currentScope;
    QQmlJSImportVisitor::endVisit(uiod);

    if (m_currentScope == m_globalScope
        || m_currentScope->baseTypeName() == QStringLiteral("Component")) {
        return;
    }

    QString parentPropertyName = childScope->parentPropertyName();
    if (parentPropertyName.isEmpty())
        return;

    auto property = childScope->property(parentPropertyName);
    property.setType(QQmlJSScope::ConstPtr(m_currentScope));

    if (childScope->hasOwnProperty(parentPropertyName)) {
        Q_ASSERT(childScope->ownProperty(parentPropertyName).index() >= 0);
    } else {
        // it's a new property, so must adjust the index. the index is
        // "outdated" as it's a relative index of scope, not childScope (or
        // it might even be -1 in theory but this is likely an error)
        property.setIndex(childScope->ownProperties().size());
    }

    // TODO: This is bad. We shouldn't add a new property but rather amend the existing one.
    childScope->addOwnProperty(property);
}

FindWarningVisitor::FindWarningVisitor(QQmlJSImporter *importer, QQmlJSLogger *logger,
                                       QStringList qmltypesFiles,
                                       QList<QQmlJS::SourceLocation> comments)
    : QQmlJSImportVisitor(
            importer, logger,
            implicitImportDirectory(logger->fileName(), importer->resourceFileMapper()),
            qmltypesFiles)
{
    parseComments(comments);
}

void FindWarningVisitor::parseComments(const QList<QQmlJS::SourceLocation> &comments)
{
    QHash<int, QSet<QQmlJSLoggerCategory>> disablesPerLine;
    QHash<int, QSet<QQmlJSLoggerCategory>> enablesPerLine;
    QHash<int, QSet<QQmlJSLoggerCategory>> oneLineDisablesPerLine;

    const QString code = m_logger->code();
    const QStringList lines = code.split(u'\n');

    for (const auto &loc : comments) {
        const QString comment = code.mid(loc.offset, loc.length);
        if (!comment.startsWith(u" qmllint ") && !comment.startsWith(u"qmllint "))
            continue;

        QStringList words = comment.split(u' ');
        if (words.constFirst().isEmpty())
            words.removeFirst();

        const QString command = words.at(1);

        QSet<QQmlJSLoggerCategory> categories;
        for (qsizetype i = 2; i < words.size(); i++) {
            const QString category = words.at(i);
            const auto option = m_logger->options().constFind(category);
            if (option != m_logger->options().constEnd())
                categories << option->m_category;
            else
                m_logger->logWarning(
                        u"qmllint directive on unknown category \"%1\""_qs.arg(category),
                        Log_Syntax, loc);
        }

        if (categories.isEmpty()) {
            for (const auto &option : m_logger->options())
                categories << option.m_category;
        }

        if (command == u"disable"_qs) {
            const QString line = lines[loc.startLine - 1];
            const QString preComment = line.left(line.indexOf(comment) - 2);

            bool lineHasContent = false;
            for (qsizetype i = 0; i < preComment.size(); i++) {
                if (!preComment[i].isSpace()) {
                    lineHasContent = true;
                    break;
                }
            }

            if (lineHasContent)
                oneLineDisablesPerLine[loc.startLine] |= categories;
            else
                disablesPerLine[loc.startLine] |= categories;
        } else if (command == u"enable"_qs) {
            enablesPerLine[loc.startLine + 1] |= categories;
        } else {
            m_logger->logWarning(u"Invalid qmllint directive \"%1\" provided"_qs.arg(command),
                                 Log_Syntax, loc);
        }
    }

    if (disablesPerLine.isEmpty() && oneLineDisablesPerLine.isEmpty())
        return;

    QSet<QQmlJSLoggerCategory> currentlyDisabled;
    for (qsizetype i = 1; i <= lines.length(); i++) {
        currentlyDisabled.unite(disablesPerLine[i]).subtract(enablesPerLine[i]);

        currentlyDisabled.unite(oneLineDisablesPerLine[i]);

        if (!currentlyDisabled.isEmpty())
            m_logger->ignoreWarnings(i, currentlyDisabled);

        currentlyDisabled.subtract(oneLineDisablesPerLine[i]);
    }
}

bool FindWarningVisitor::check()
{
    // now that all ids are known, revisit any Connections whose target were perviously unknown
    for (auto const &outstandingConnection: m_outstandingConnections) {
        auto targetScope = m_scopesById[outstandingConnection.targetName];
        if (outstandingConnection.scope) {
            for (const auto scope = targetScope; targetScope;
                 targetScope = targetScope->baseType()) {
                const auto connectionMethods = targetScope->ownMethods();
                for (const auto &method : connectionMethods)
                    outstandingConnection.scope->addOwnMethod(method);
            }
        }
        QScopedValueRollback<QQmlJSScope::Ptr> rollback(m_currentScope,
                                                        outstandingConnection.scope);
        outstandingConnection.uiod->initializer->accept(this);
    }

    return !m_logger->hasWarnings() && !m_logger->hasErrors();
}
