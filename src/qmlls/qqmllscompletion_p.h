// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLLSCOMPLETION_H
#define QQMLLSCOMPLETION_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qqmlcompletioncontextstrings_p.h"
#include "qqmllsutils_p.h"

#include <QtLanguageServer/private/qlanguageserverspectypes_p.h>
#include <QtQmlDom/private/qqmldomexternalitems_p.h>
#include <QtQmlDom/private/qqmldomtop_p.h>
#include <QtCore/private/qduplicatetracker_p.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QQmlLSCompletionLog)

enum QQmlLSUtilsAppendOption { AppendSemicolon, AppendNothing };


class QQmlLSCompletion
{
    using DomItem = QQmlJS::Dom::DomItem;
public:
    // TODO: constructor with build paths to load plugins

    using CompletionItem = QLspSpecification::CompletionItem;
    QList<CompletionItem> completions(const DomItem &currentItem,
                                      const CompletionContextStrings &ctx) const;

    static CompletionItem makeSnippet(QUtf8StringView qualifier, QUtf8StringView label,
                                      QUtf8StringView insertText);

    static CompletionItem makeSnippet(QUtf8StringView label, QUtf8StringView insertText);

private:
    struct QQmlLSCompletionPosition
    {
        DomItem itemAtPosition;
        CompletionContextStrings cursorPosition;
        qsizetype offset() const { return cursorPosition.offset(); }
    };

    bool betweenLocations(QQmlJS::SourceLocation left, const QQmlLSCompletionPosition &positionInfo,
                          QQmlJS::SourceLocation right) const;
    bool afterLocation(QQmlJS::SourceLocation left,
                       const QQmlLSCompletionPosition &positionInfo) const;
    bool beforeLocation(const QQmlLSCompletionPosition &ctx, QQmlJS::SourceLocation right) const;
    bool ctxBeforeStatement(const QQmlLSCompletionPosition &positionInfo,
                            const DomItem &parentForContext,
                            QQmlJS::Dom::FileLocationRegion firstRegion) const;
    bool isCaseOrDefaultBeforeCtx(const DomItem &currentClause,
                                  const QQmlLSCompletionPosition &positionInfo,
                                  QQmlJS::Dom::FileLocationRegion keywordRegion) const;
    DomItem previousCaseOfCaseBlock(const DomItem &parentForContext,
                                    const QQmlLSCompletionPosition &positionInfo) const;

    QList<CompletionItem> idsCompletions(const DomItem &component) const;

    QList<CompletionItem> suggestReachableTypes(const DomItem &context,
                                                QQmlJS::Dom::LocalSymbolsTypes typeCompletionType,
                                                QLspSpecification::CompletionItemKind kind) const;

    QList<CompletionItem> suggestJSStatementCompletion(const DomItem &currentItem) const;
    QList<CompletionItem> suggestCaseAndDefaultStatementCompletion() const;
    QList<CompletionItem> suggestVariableDeclarationStatementCompletion(
            QQmlLSUtilsAppendOption option = AppendSemicolon) const;

    QList<CompletionItem> suggestJSExpressionCompletion(const DomItem &context) const;

    QList<CompletionItem> suggestBindingCompletion(const DomItem &itemAtPosition) const;

    QList<CompletionItem>
    insideImportCompletionHelper(const DomItem &file,
                                 const QQmlLSCompletionPosition &positionInfo) const;

    QList<CompletionItem> jsIdentifierCompletion(const QQmlJSScope::ConstPtr &scope,
                                                 QDuplicateTracker<QString> *usedNames) const;

    QList<CompletionItem> methodCompletion(const QQmlJSScope::ConstPtr &scope,
                                           QDuplicateTracker<QString> *usedNames) const;
    QList<CompletionItem> propertyCompletion(const QQmlJSScope::ConstPtr &scope,
                                             QDuplicateTracker<QString> *usedNames) const;
    QList<CompletionItem> enumerationCompletion(const QQmlJSScope::ConstPtr &scope,
                                                QDuplicateTracker<QString> *usedNames) const;
    QList<CompletionItem> enumerationValueCompletionHelper(const QStringList &enumeratorKeys) const;

    QList<CompletionItem> enumerationValueCompletion(const QQmlJSScope::ConstPtr &scope,
                                                     const QString &enumeratorName) const;

    static bool cursorInFrontOfItem(const DomItem &parentForContext,
                                    const QQmlLSCompletionPosition &positionInfo);
    static bool cursorAfterColon(const DomItem &currentItem,
                                 const QQmlLSCompletionPosition &positionInfo);
    QList<CompletionItem>
    insidePragmaCompletion(QQmlJS::Dom::DomItem currentItem,
                           const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideQmlObjectCompletion(const DomItem &parentForContext,
                              const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insidePropertyDefinitionCompletion(const DomItem &currentItem,
                                       const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideBindingCompletion(const DomItem &currentItem,
                            const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideImportCompletion(const DomItem &currentItem,
                           const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideQmlFileCompletion(const DomItem &currentItem,
                            const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    suggestContinueAndBreakStatementIfNeeded(const DomItem &itemAtPosition) const;
    QList<CompletionItem>
    insideScriptLiteralCompletion(const DomItem &currentItem,
                                  const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insideCallExpression(const DomItem &currentItem,
                                               const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insideIfStatement(const DomItem &currentItem,
                                            const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insideReturnStatement(const DomItem &currentItem,
                                                const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insideWhileStatement(const DomItem &currentItem,
                                               const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideDoWhileStatement(const DomItem &parentForContext,
                           const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideForStatementCompletion(const DomItem &parentForContext,
                                 const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideForEachStatement(const DomItem &parentForContext,
                           const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insideSwitchStatement(const DomItem &parentForContext,
                                                const QQmlLSCompletionPosition positionInfo) const;
    QList<CompletionItem> insideCaseClause(const DomItem &parentForContext,
                                           const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insideCaseBlock(const DomItem &parentForContext,
                                          const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insideDefaultClause(const DomItem &parentForContext,
                                              const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideBinaryExpressionCompletion(const DomItem &parentForContext,
                                     const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insideScriptPattern(const DomItem &parentForContext,
                                              const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideVariableDeclarationEntry(const DomItem &parentForContext,
                                   const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insideThrowStatement(const DomItem &parentForContext,
                                               const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideLabelledStatement(const DomItem &parentForContext,
                            const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideContinueStatement(const DomItem &parentForContext,
                            const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insideBreakStatement(const DomItem &parentForContext,
                                               const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideConditionalExpression(const DomItem &parentForContext,
                                const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insideUnaryExpression(const DomItem &parentForContext,
                                                const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> insidePostExpression(const DomItem &parentForContext,
                                               const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem>
    insideParenthesizedExpression(const DomItem &parentForContext,
                                  const QQmlLSCompletionPosition &positionInfo) const;
    QList<CompletionItem> signalHandlerCompletion(const QQmlJSScope::ConstPtr &scope,
                                                  QDuplicateTracker<QString> *usedNames) const;

    // TODO: split + move to plugin
    QList<CompletionItem> suggestQuickSnippetsCompletion(const DomItem &itemAtPosition) const;
};

QT_END_NAMESPACE

#endif // QQMLLSCOMPLETION_H
