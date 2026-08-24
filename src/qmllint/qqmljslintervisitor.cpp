// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljslintervisitor_p.h"

#include <private/qqmljsutils_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;
using namespace QQmlJS::AST;

namespace QQmlJS {
/*!
   \internal
    \class QQmlJS::LinterVisitor
    Extends QQmlJSImportVisitor with extra warnings that are required for linting but unrelated to
   QQmlJSImportVisitor actual task that is constructing QQmlJSScopes. One example of such warnings
   are purely syntactic checks, or style-checks warnings that don't make sense during compilation.
 */

LinterVisitor::LinterVisitor(
        QQmlJSImporter *importer, QQmlJSLogger *logger,
        const QString &implicitImportDirectory, const QStringList &qmldirFiles,
        QQmlJS::Engine *engine)
    : QQmlJSImportVisitor(importer, logger, implicitImportDirectory, qmldirFiles)
    , m_engine(engine)
{
}

void LinterVisitor::leaveEnvironment()
{
    const auto leaveEnv = qScopeGuard([this] { QQmlJSImportVisitor::leaveEnvironment(); });

    if (m_currentScope->scopeType() != QQmlSA::ScopeType::QMLScope)
        return;

    if (auto base = m_currentScope->baseType()) {
        if (base->internalName() == u"QQmlComponent"_s) {
            const auto nChildren = std::count_if(
                    m_currentScope->childScopesBegin(), m_currentScope->childScopesEnd(),
                    [](const QQmlJSScope::ConstPtr &scope) {
                        return scope->scopeType() == QQmlSA::ScopeType::QMLScope;
                    });
            if (nChildren != 1) {
                m_logger->log("Components must have exactly one child"_L1,
                              qmlComponentChildrenCount, m_currentScope->sourceLocation());
            }
        }
    }
}

bool LinterVisitor::visit(StringLiteral *sl)
{
    QQmlJSImportVisitor::visit(sl);
    const QString s = m_logger->code().mid(sl->literalToken.begin(), sl->literalToken.length);

    if (s.contains(QLatin1Char('\r')) || s.contains(QLatin1Char('\n')) || s.contains(QChar(0x2028u))
        || s.contains(QChar(0x2029u))) {
        QString templateString;

        bool escaped = false;
        const QChar stringQuote = s[0];
        for (qsizetype i = 1; i < s.size() - 1; i++) {
            const QChar c = s[i];

            if (c == u'\\') {
                escaped = !escaped;
            } else if (escaped) {
                // If we encounter an escaped quote, unescape it since we use backticks here
                if (c == stringQuote)
                    templateString.chop(1);

                escaped = false;
            } else {
                if (c == u'`')
                    templateString += u'\\';
                if (c == u'$' && i + 1 < s.size() - 1 && s[i + 1] == u'{')
                    templateString += u'\\';
            }

            templateString += c;
        }

        QQmlJSDocumentEdit documentEdit{
            m_logger->filePath(), sl->literalToken, u"`" % templateString % u"`"
        };
        QQmlJSFixSuggestion suggestion = { "Use a template literal instead."_L1, sl->literalToken,
                                           documentEdit };
        suggestion.setAutoApplicable();
        m_logger->log(QStringLiteral("String contains unescaped line terminator which is "
                                     "deprecated."),
                      qmlMultilineStrings, sl->literalToken, true, true, suggestion);
    }
    return true;
}

bool LinterVisitor::preVisit(Node *n)
{
    m_ancestryIncludingCurrentNode.push_back(n);
    return true;
}

void LinterVisitor::postVisit(Node *n)
{
    Q_ASSERT(m_ancestryIncludingCurrentNode.back() == n);
    m_ancestryIncludingCurrentNode.pop_back();
}

Node *LinterVisitor::astParentOfVisitedNode() const
{
    if (m_ancestryIncludingCurrentNode.size() < 2)
        return nullptr;
    return m_ancestryIncludingCurrentNode[m_ancestryIncludingCurrentNode.size() - 2];
}

bool LinterVisitor::visit(CommaExpression *expression)
{
    QQmlJSImportVisitor::visit(expression);
    if (!expression->left || !expression->right)
        return true;

    // don't warn about commas in "for" statements
    if (cast<ForStatement *>(astParentOfVisitedNode()))
        return true;

    m_logger->log("Do not use comma expressions."_L1, qmlComma, expression->commaToken);
    return true;
}

static void warnAboutLiteralConstructors(NewMemberExpression *expression, QQmlJSLogger *logger)
{
    static constexpr std::array literals{ "Boolean"_L1, "Function"_L1, "JSON"_L1,
                                          "Math"_L1,    "Number"_L1,   "String"_L1 };

    const IdentifierExpression *identifier = cast<IdentifierExpression *>(expression->base);
    if (!identifier)
        return;

    if (std::find(literals.cbegin(), literals.cend(), identifier->name) != literals.cend()) {
        logger->log("Do not use '%1' as a constructor."_L1.arg(identifier->name),
                    qmlLiteralConstructor, identifier->identifierToken);
    }
    if (identifier->name == "Array"_L1 && expression->arguments && expression->arguments->next) {
        const auto fullRange = combine(expression->newToken, expression->rparenToken);
        const QList<QQmlJSDocumentEdit> edits = {
            { logger->filePath(), combine(expression->newToken, expression->lparenToken), "["_L1 },
            { logger->filePath(), expression->rparenToken, "]"_L1 },
        };
        QQmlJSFixSuggestion fix("Replace with array literal"_L1, fullRange, edits);
        fix.setAutoApplicable(true);
        logger->log("Array has confusing semantics, use an array literal ([]) instead."_L1,
                    qmlLiteralConstructor, identifier->identifierToken, true, true, fix);
    }
}

bool LinterVisitor::visit(NewMemberExpression *expression)
{
    QQmlJSImportVisitor::visit(expression);
    warnAboutLiteralConstructors(expression, m_logger);
    return true;
}

bool LinterVisitor::visit(VoidExpression *ast)
{
    QQmlJSImportVisitor::visit(ast);
    m_logger->log("Do not use void expressions."_L1, qmlVoid, ast->voidToken);
    return true;
}

static SourceLocation confusingPluses(BinaryExpression *exp)
{
    Q_ASSERT(exp->op == QSOperator::Add);

    SourceLocation location = exp->operatorToken;

    // a++ + b
    if (auto increment = cast<PostIncrementExpression *>(exp->left))
        location = combine(increment->incrementToken, location);
    // a + +b
    if (auto unary = cast<UnaryPlusExpression *>(exp->right))
        location = combine(location, unary->plusToken);
    // a + ++b
    if (auto increment = cast<PreIncrementExpression *>(exp->right))
        location = combine(location, increment->incrementToken);

    if (location == exp->operatorToken)
        return SourceLocation{};

    return location;
}

static SourceLocation confusingMinuses(BinaryExpression *exp)
{
    Q_ASSERT(exp->op == QSOperator::Sub);

    SourceLocation location = exp->operatorToken;

    // a-- - b
    if (auto decrement = cast<PostDecrementExpression *>(exp->left))
        location = combine(decrement->decrementToken, location);
    // a - -b
    if (auto unary = cast<UnaryMinusExpression *>(exp->right))
        location = combine(location, unary->minusToken);
    // a - --b
    if (auto decrement = cast<PreDecrementExpression *>(exp->right))
        location = combine(location, decrement->decrementToken);

    if (location == exp->operatorToken)
        return SourceLocation{};

    return location;
}

bool LinterVisitor::visit(BinaryExpression *exp)
{
    QQmlJSImportVisitor::visit(exp);
    switch (exp->op) {
    case QSOperator::Add:
        if (SourceLocation loc = confusingPluses(exp); loc.isValid())
            m_logger->log("Confusing pluses."_L1, qmlConfusingPluses, loc);
        break;
    case QSOperator::Sub:
        if (SourceLocation loc = confusingMinuses(exp); loc.isValid())
            m_logger->log("Confusing minuses."_L1, qmlConfusingMinuses, loc);
        break;
    default:
        break;
    }

    return true;
}

bool LinterVisitor::visit(QQmlJS::AST::UiImport *import)
{
    QQmlJSImportVisitor::visit(import);

    const auto locAndName = [](const UiImport *i) {
        if (!i->importUri)
            return std::make_pair(i->fileNameToken, i->fileName.toString());

        QQmlJS::SourceLocation l = i->importUri->firstSourceLocation();
        if (i->importIdToken.isValid())
            l = combine(l, i->importIdToken);
        else if (i->version)
            l = combine(l, i->version->minorToken);
        else
            l = combine(l, i->importUri->lastSourceLocation());

        return std::make_pair(l, i->importUri->toString());
    };

    SeenImport i(import);
    if (const auto it = m_seenImports.constFind(i); it != m_seenImports.constEnd()) {
        const auto locAndNameImport = locAndName(import);
        const auto locAndNameSeen = locAndName(it->uiImport);
        m_logger->log("Duplicate import '%1'"_L1.arg(locAndNameImport.second),
                      qmlDuplicateImport, locAndNameImport.first);
        m_logger->log("Note: previous import '%1' here"_L1.arg(locAndNameSeen.second),
                      qmlDuplicateImport, locAndNameSeen.first, true, true, {},
                      locAndName(import).first.startLine);
    }

    m_seenImports.insert(i);
    return true;
}

void LinterVisitor::handleDuplicateEnums(UiEnumMemberList *members, QStringView key,
                                         const QQmlJS::SourceLocation &location)
{
    m_logger->log(u"Enum key '%1' has already been declared"_s.arg(key), qmlDuplicateEnumEntries,
                  location);
    for (const auto *member = members; member; member = member->next) {
        if (member->member.toString() == key) {
            m_logger->log(u"Note: previous declaration of '%1' here"_s.arg(key),
                          qmlDuplicateEnumEntries, member->memberToken);
            return;
        }
    }
}

bool LinterVisitor::visit(QQmlJS::AST::UiEnumDeclaration *uied)
{
    QQmlJSImportVisitor::visit(uied);

    if (m_currentScope->isInlineComponent()) {
        m_logger->log(u"Enums declared inside of inline component are ignored."_s,
                      qmlInlineComponentEnums, uied->firstSourceLocation());
    } else if (m_currentScope->componentRootStatus() == QQmlJSScope::IsComponentRoot::No
               && !m_currentScope->isFileRootComponent()) {
        m_logger->log(u"Enum declared outside the root element. It won't be accessible."_s,
                      qmlNonRootEnums, uied->firstSourceLocation());
    }

    QHash<QStringView, const QQmlJS::AST::UiEnumMemberList *> seen;
    for (const auto *member = uied->members; member; member = member->next) {
        QStringView key = member->member;
        if (!key.front().isUpper()) {
            m_logger->log(u"Enum keys should start with an uppercase."_s, qmlEnumKeyCase,
                          member->memberToken);
        }

        if (seen.contains(key))
            handleDuplicateEnums(uied->members, key, member->memberToken);
        else
            seen[member->member] = member;

        if (uied->name == key) {
            m_logger->log("Enum entry should be named differently than the enum itself to avoid "
                          "confusion."_L1, qmlEnumEntryMatchesEnum, member->firstSourceLocation());
        }
    }

    return true;
}

static bool allCodePathsReturnInsideCase(Node *statement)
{
    using namespace AST;
    if (!statement)
        return false;

    switch (statement->kind) {
    case Node::Kind_Block: {
        return allCodePathsReturnInsideCase(cast<Block *>(statement)->statements);
    }
    case Node::Kind_BreakStatement:
        return true;
    case Node::Kind_CaseBlock: {
        const CaseBlock *caseBlock = cast<CaseBlock *>(statement);
        if (caseBlock->defaultClause)
            return allCodePathsReturnInsideCase(caseBlock->defaultClause);
        return allCodePathsReturnInsideCase(caseBlock->clauses);
    }
    case Node::Kind_CaseClause:
        return allCodePathsReturnInsideCase(cast<CaseClause *>(statement)->statements);
    case Node::Kind_CaseClauses: {
        for (CaseClauses *caseClauses = cast<CaseClauses *>(statement); caseClauses;
             caseClauses = caseClauses->next) {
            if (!allCodePathsReturnInsideCase(caseClauses->clause))
                return false;
        }
        return true;
    }
    case Node::Kind_ContinueStatement:
        // allCodePathsReturn() doesn't recurse into loops, so any encountered `continue` should
        // belong to a loop outside the switch statement.
        return true;
    case Node::Kind_DefaultClause:
        return allCodePathsReturnInsideCase(cast<DefaultClause *>(statement)->statements);
    case Node::Kind_IfStatement: {
        const auto *ifStatement = cast<IfStatement *>(statement);
        return allCodePathsReturnInsideCase(ifStatement->ok)
                && allCodePathsReturnInsideCase(ifStatement->ko);
    }
    case Node::Kind_LabelledStatement:
        return allCodePathsReturnInsideCase(cast<LabelledStatement *>(statement)->statement);
    case Node::Kind_ReturnStatement:
        return true;
    case Node::Kind_StatementList: {
        for (StatementList *list = cast<StatementList *>(statement); list; list = list->next) {
            if (allCodePathsReturnInsideCase(list->statement))
                return true;
        }
        return false;
    }
    case Node::Kind_SwitchStatement:
        return allCodePathsReturnInsideCase(cast<SwitchStatement *>(statement)->block);
    case Node::Kind_ThrowStatement:
        return true;
    case Node::Kind_TryStatement: {
        auto *tryStatement = cast<TryStatement *>(statement);
        if (allCodePathsReturnInsideCase(tryStatement->statement))
            return true;
        return allCodePathsReturnInsideCase(tryStatement->finallyExpression->statement);
    }
    case Node::Kind_WithStatement:
        return allCodePathsReturnInsideCase(cast<WithStatement *>(statement)->statement);
    default:
        break;
    }
    return false;
}

void LinterVisitor::checkCaseFallthrough(StatementList *statements, SourceLocation errorLoc,
                                         SourceLocation nextLoc)
{
    if (!statements || !nextLoc.isValid())
        return;

    if (allCodePathsReturnInsideCase(statements))
        return;

    quint32 afterLastStatement = 0;
    for (StatementList *it = statements; it; it = it->next) {
        if (!it->next) {
            afterLastStatement = it->statement->lastSourceLocation().end();
        }
    }

    const auto &comments = m_engine->comments();
    auto it = std::find_if(comments.cbegin(), comments.cend(),
                           [&](auto c) { return afterLastStatement < c.offset; });
    auto end = std::find_if(it, comments.cend(),
                            [&](auto c) { return c.offset >= nextLoc.offset; });

    for (; it != end; ++it) {
        const QString &commentText = m_engine->code().mid(it->offset, it->length);
        if (commentText.contains("fall through"_L1, Qt::CaseInsensitive)
            || commentText.contains("fall-through"_L1, Qt::CaseInsensitive)
            || commentText.contains("fallthrough"_L1, Qt::CaseInsensitive)) {
            return;
        }
    }

    m_logger->log(
            "Non-empty case block potentially falls through to the next case or default statement. "
            "Add \"// fallthrough\" at the end of the block to silence this warning."_L1,
            qmlUnterminatedCase, errorLoc);
}

bool LinterVisitor::visit(QQmlJS::AST::CaseBlock *block)
{
    QQmlJSImportVisitor::visit(block);

    std::vector<std::pair<SourceLocation, StatementList *>> clauses;
    for (CaseClauses *it = block->clauses; it; it = it->next)
        clauses.push_back({ it->clause->caseToken, it->clause->statements });
    if (block->defaultClause)
        clauses.push_back({ block->defaultClause->defaultToken, block->defaultClause->statements });
    for (CaseClauses *it = block->moreClauses; it; it = it->next)
        clauses.push_back({ it->clause->caseToken, it->clause->statements });

    // check all but the last clause for fallthrough
    for (size_t i = 0; i < clauses.size() - 1; ++i) {
        const SourceLocation nextToken = clauses[i + 1].first;
        checkCaseFallthrough(clauses[i].second, clauses[i].first, nextToken);
    }
    return true;
}

static QList<const Statement *> possibleLastStatements(const StatementList *ast);
static QList<const Statement *> possibleLastStatements(const CaseBlock *ast)
{
    QList<const Statement *> lasts;

    for (const auto *clause = ast->clauses; clause; clause = clause->next)
        lasts << possibleLastStatements(clause->clause->statements);
    if (ast->defaultClause)
        lasts << possibleLastStatements(ast->defaultClause->statements);
    for (const auto *clause = ast->moreClauses; clause; clause = clause->next)
        lasts << possibleLastStatements(clause->clause->statements);

    return lasts;
}

static QList<const Statement *> possibleLastStatements(const Statement *ast)
{
    if (const auto *s = cast<const Block *>(ast))
        return possibleLastStatements(s->statements) << s;
    if (const auto *s = cast<const BreakStatement *>(ast))
        return { s };
    if (const auto *s = cast<const ContinueStatement *>(ast))
        return { s };
    if (const auto *s = cast<const DebuggerStatement *>(ast))
        return { s };
    if (const auto *s = cast<const DoWhileStatement *>(ast))
        return possibleLastStatements(s->statement) << s;
    if (const auto *s = cast<const EmptyStatement *>(ast))
        return { s };
    if (const auto *s = cast<const ExportDeclaration *>(ast))
        return { s };
    if (const auto *s = cast<const ExpressionStatement *>(ast))
        return { s };
    if (const auto *s = cast<const ForEachStatement *>(ast))
        return possibleLastStatements(s->statement) << s;
    if (const auto *s = cast<const ForStatement *>(ast))
        return possibleLastStatements(s->statement) << s;
    if (const auto *s = cast<const IfStatement *>(ast)) {
        auto lasts = possibleLastStatements(s->ok);
        if (s->ko)
            lasts << possibleLastStatements(s->ko);
        return lasts << s;
    }
    if (const auto *s = cast<const ImportDeclaration *>(ast))
        return { s };
    if (const auto *s = cast<const LabelledStatement *>(ast))
        return possibleLastStatements(s->statement) << ast;
    if (const auto *s = cast<const ReturnStatement *>(ast))
        return { s };
    if (const auto *s = cast<const SwitchStatement *>(ast))
        return possibleLastStatements(s->block) << s;
    if (const auto *s = cast<const ThrowStatement *>(ast))
        return { s };
    if (const auto *s = cast<const TryStatement *>(ast))
        return { s };
    if (const auto *s = cast<const VariableStatement *>(ast))
        return { s };
    if (const auto *s = cast<const WhileStatement *>(ast))
        return possibleLastStatements(s->statement) << s;
    if (const auto *s = cast<const WithStatement *>(ast))
        return possibleLastStatements(s->statement) << s;

    Q_UNREACHABLE_RETURN({});
}

static QList<const Statement *> possibleLastStatements(const StatementList *ast)
{
    if (!ast)
        return {};
    for (; ast->next; ast = ast->next) { }
    const auto *statement = ast->statement;

    // Can't store FunctionDeclaration as a statement. See StatementList.
    if (cast<const FunctionDeclaration *>(statement))
        return {};

    return possibleLastStatements(static_cast<const Statement *>(statement));
}

static bool isUselessExpressionStatement_impl(const ExpressionNode *ast)
{
    switch (ast->kind) {
    case Node::Kind_CallExpression:
    case Node::Kind_DeleteExpression:
    case Node::Kind_NewExpression:
    case Node::Kind_PreDecrementExpression:
    case Node::Kind_PreIncrementExpression:
    case Node::Kind_PostDecrementExpression:
    case Node::Kind_PostIncrementExpression:
    case Node::Kind_YieldExpression:
    case Node::Kind_FunctionExpression:
        return false;
    case Node::Kind_NumericLiteral:
    case Node::Kind_StringLiteral:
    case Node::Kind_FalseLiteral:
    case Node::Kind_TrueLiteral:
    case Node::Kind_NullExpression:
    case Node::Kind_Undefined:
    case Node::Kind_RegExpLiteral:
    case Node::Kind_SuperLiteral:
    case Node::Kind_ThisExpression:
    case Node::Kind_FieldMemberExpression:
    case Node::Kind_IdentifierExpression:
    case Node::Kind_TypeOfExpression:
        return true;
    default:
        break;
    }

    if (const auto *e = cast<const NestedExpression *>(ast))
        return isUselessExpressionStatement_impl(e->expression);
    if (const auto *e = cast<const NotExpression *>(ast))
        return isUselessExpressionStatement_impl(e->expression);
    if (const auto *e = cast<const TildeExpression *>(ast))
        return isUselessExpressionStatement_impl(e->expression);
    if (const auto *e = cast<const UnaryMinusExpression *>(ast))
        return isUselessExpressionStatement_impl(e->expression);
    if (const auto *e = cast<const UnaryPlusExpression *>(ast))
        return isUselessExpressionStatement_impl(e->expression);
    if (const auto *e = cast<const ConditionalExpression *>(ast))
        return isUselessExpressionStatement_impl(e->ok) && isUselessExpressionStatement_impl(e->ko);

    if (const BinaryExpression *binary = cast<const BinaryExpression *>(ast)) {
        switch (binary->op) {
        case QSOperator::InplaceAnd:
        case QSOperator::Assign:
        case QSOperator::InplaceSub:
        case QSOperator::InplaceDiv:
        case QSOperator::InplaceExp:
        case QSOperator::InplaceAdd:
        case QSOperator::InplaceLeftShift:
        case QSOperator::InplaceMod:
        case QSOperator::InplaceMul:
        case QSOperator::InplaceOr:
        case QSOperator::InplaceRightShift:
        case QSOperator::InplaceURightShift:
        case QSOperator::InplaceXor:
            return false;
        default:
            return isUselessExpressionStatement_impl(binary->left)
                    && isUselessExpressionStatement_impl(binary->right);
        }
    }

    return false;
}

/*!
\internal

This assumes that there is no custom coercion enabled via \c Symbol.toPrimitive or similar.
*/
static bool isUselessExpressionStatement(const ExpressionStatement *ast)
{
    return isUselessExpressionStatement_impl(ast->expression);
}

void LinterVisitor::handleUselessExpressionStatement(const ExpressionStatement *ast)
{
    // property binding, signal handler, or function declaration
    const auto it = std::find_if(m_ancestryIncludingCurrentNode.crbegin(),
                                 m_ancestryIncludingCurrentNode.crend(),
                                 [](auto it) {
                                     return it->kind == Node::Kind_UiPublicMember
                                             || it->kind == Node::Kind_FunctionDeclaration
                                             || it->kind == Node::Kind_UiScriptBinding;
                                 });

    if (it == m_ancestryIncludingCurrentNode.crend())
        return;

    // A useless ExpressionStatement in *last position* inside a property binding is not useless
    const auto isLastExprStat = [](const ExpressionStatement *statement, const Statement *base) {
        const auto lasts = possibleLastStatements(base);
        return lasts.contains(statement);
    };

    if (const auto *usb = cast<UiScriptBinding *>(*it); usb && usb->qualifiedId) {
        if (usb->qualifiedId->toString() == "id"_L1)
            return;
        if (usb->qualifiedId->next)
            return; // group/attached property, give up
        if (m_savedBindingOuterScope->scopeType() == QQmlSA::ScopeType::GroupedPropertyScope)
            return; // group property, give up

        QQmlJSScope::Ptr object = m_currentScope;
        while (object && object->scopeType() != QQmlSA::ScopeType::QMLScope)
            object = object->parentScope();

        if (!object)
            return;

        if (m_propertyBindings.contains(object)) {
            for (const auto &entry : m_propertyBindings[object]) {
                if (entry.data == usb->qualifiedId->toString()) {
                    if (isLastExprStat(ast, usb->statement))
                        return;
                    else
                        break;
                }
            }
        }
    }

    const auto *upm = cast<const UiPublicMember *>(*it);
    if (upm && upm->type == AST::UiPublicMember::Property && upm->statement) {
        if (isLastExprStat(ast, upm->statement))
            return;
    }

    if (isUselessExpressionStatement(ast)) {
        m_logger->log("Expression statement has no obvious effect."_L1,
                      qmlConfusingExpressionStatement,
                      combine(ast->firstSourceLocation(), ast->lastSourceLocation()));
    }
}

bool LinterVisitor::visit(ExpressionStatement *ast)
{
    QQmlJSImportVisitor::visit(ast);
    handleUselessExpressionStatement(ast);
    return true;
}

bool LinterVisitor::safeInsertJSIdentifier(QQmlJSScope::Ptr &scope, const QString &name, const QQmlJSScope::JavaScriptIdentifier &identifier)
{
    if (scope->scopeType() == QQmlSA::ScopeType::JSLexicalScope &&
        identifier.kind == QQmlJSScope::JavaScriptIdentifier::FunctionScoped) {
        // var is generally not great, but we don't want to emit this warning if you
        // are in the single, toplevel block of a binding
        Q_ASSERT(!scope->parentScope().isNull()); // lexical scope should always have a parent
        auto parentScopeType = scope->parentScope()->scopeType();
        bool inTopLevelBindingBlockScope = parentScopeType == QQmlSA::ScopeType::BindingFunctionScope
                || parentScopeType == QQmlSA::ScopeType::SignalHandlerFunctionScope;
        if (!inTopLevelBindingBlockScope) {
            m_logger->log(u"var declaration in block scope is hoisted to function scope\n"_s
                          u"Replace it with const or let to silence the warning\n"_s,
                          qmlBlockScopeVarDeclaration, identifier.location);
        }
    } else if (scope->scopeType() == QQmlSA::ScopeType::QMLScope) {
        const QQmlJSScope *scopePtr = scope.get();
        std::pair<const QQmlJSScope*, QString> misplaced { scopePtr, name };
        if (misplacedJSIdentifiers.contains(misplaced))
            return false; // we only want to warn once
        misplacedJSIdentifiers.insert(misplaced);
        m_logger->log(u"JavaScript declarations are not allowed in QML elements"_s, qmlSyntax,
                      identifier.location);
        return false;
    }
    return QQmlJSImportVisitor::safeInsertJSIdentifier(scope, name, identifier);
}

QQmlJSImportVisitor::BindingExpressionParseResult LinterVisitor::parseBindingExpression(
        const QString &name, const QQmlJS::AST::Statement *statement,
        const QQmlJS::AST::UiPublicMember *associatedPropertyDefinition)
{
    if (statement && statement->kind == (int)AST::Node::Kind::Kind_Block) {
        const auto *block = static_cast<const AST::Block *>(statement);
        if (!block->statements && associatedPropertyDefinition) {
            m_logger->log("Unintentional empty block, use ({}) for empty object literal"_L1,
                          qmlUnintentionalEmptyBlock,
                          combine(block->lbraceToken, block->rbraceToken));
        }
    }

    return QQmlJSImportVisitor::parseBindingExpression(name, statement, associatedPropertyDefinition);
}

void LinterVisitor::handleLiteralBinding(const QQmlJSMetaPropertyBinding &binding,
                                         const UiPublicMember *associatedPropertyDefinition)
{
    if (!m_currentScope->hasOwnProperty(binding.propertyName()))
        return;

    if (!associatedPropertyDefinition->isReadonly())
        return;

    const auto &prop = m_currentScope->property(binding.propertyName());
    const auto log = [&](const QString &preferredType) {
        m_logger->log("Prefer more specific type %1 over var"_L1.arg(preferredType),
                      qmlPreferNonVarProperties, prop.sourceLocation());
    };

    if (prop.typeName() != "QVariant"_L1)
        return;

    switch (binding.bindingType()) {
    case QQmlSA::BindingType::BoolLiteral: {
        log("bool"_L1);
        break;
    }
    case QQmlSA::BindingType::NumberLiteral: {
        double v = binding.numberValue();
        auto loc = binding.sourceLocation();
        QStringView literal = QStringView(m_engine->code()).mid(loc.offset, loc.length);
        if (literal.contains(u'.') || double(int(v)) != v)
            log("real or double"_L1);
        else
            log("int"_L1);
        break;
    }
    case QQmlSA::BindingType::StringLiteral: {
        log("string"_L1);
        break;
    }
    default: {
        break;
    }
    }
}

bool LinterVisitor::visit(UiProgram *ast)
{
    const bool result = QQmlJSImportVisitor::visit(ast);

    m_renamedComponents.setScopeToName(&m_rootScopeImports.names());

    return result;
}

void LinterVisitor::checkUnusedImports()
{
    auto unusedImports = m_importLocations;
    for (const QString &type : std::as_const(m_usedTypes)) {
        const auto &importLocations = m_importTypeLocationMap.values(type);
        for (const auto &importLocation : importLocations)
            unusedImports.remove(importLocation);

        // If there are no more unused imports left we can abort early
        if (unusedImports.isEmpty())
            break;
    }

    const auto &imports = m_importStaticModuleLocationMap.values();
    for (const QQmlJS::SourceLocation &import : imports)
        unusedImports.remove(import);

    for (const auto &import : unusedImports) {
        m_logger->log(QString::fromLatin1("Unused import"), qmlUnusedImports, import);
    }
}

void LinterVisitor::endVisit(UiProgram *ast)
{
    QQmlJSImportVisitor::endVisit(ast);
    checkFileSelections();
    checkUnusedImports();
}

static constexpr QLatin1String s_method = "method"_L1;
static constexpr QLatin1String s_signal = "signal"_L1;
static constexpr QLatin1String s_property = "property"_L1;

enum OverrideInformation { WithoutOverride = 0, WithFinal = 1, WithOverride = 2 };
Q_DECLARE_FLAGS(OverrideInformations, OverrideInformation);

static void warnForMethodShadowingInBase(const QQmlJSScope::ConstPtr &base, const QString &name,
                                         const QQmlJS::SourceLocation &location,
                                         QQmlJSLogger *logger)
{
    Q_ASSERT(base);
    if (!base->hasMethod(name))
        return;

    static constexpr QLatin1String warningMessage =
            "%1 \"%2\" already exists in base type \"%3\", use a different name."_L1;
    const auto owner = QQmlJSScope::ownerOfMethod(base, name).scope;
    const bool isSignal = owner->methods(name).front().methodType() == QQmlJSMetaMethodType::Signal;
    logger->log(warningMessage.arg(isSignal ? "Signal"_L1 : "Method"_L1, name,
                                   QQmlJSUtils::getScopeName(owner, QQmlSA::ScopeType::QMLScope)),
                qmlShadow, location);
}

static void warnForPropertyShadowingInBase(const QQmlJSScope::ConstPtr &base, const QString &name,
                                           const QQmlJS::SourceLocation &location,
                                           OverrideInformations overrideFlags, QQmlJSLogger *logger)
{
    Q_ASSERT(base);
    const bool hasOverride = overrideFlags.testFlag(WithOverride);
    if (!base->hasProperty(name)) {
        if (!hasOverride)
            return;
        logger->log(
                "Member \"%1\" does not override anything. Consider removing \"override\"."_L1.arg(
                        name),
                qmlPropertyOverride, location);
        return;
    }

    const auto owner = QQmlJSScope::ownerOfProperty(base, name).scope;
    const auto shadowedProperty = owner->ownProperty(name);
    if (shadowedProperty.isFinal()) {
        logger->log(
                (!hasOverride
                         ? "Member \"%1\" shadows final member \"%1\" from base type \"%2\", use a different name."_L1
                         : "Member \"%1\" overrides final member \"%1\" from base type \"%2\", use a different name and remove the \"override\"."_L1)
                        .arg(name, QQmlJSUtils::getScopeName(owner, QQmlSA::ScopeType::QMLScope)),
                qmlPropertyOverride, location);
        return;
    }

    if (shadowedProperty.isVirtual() || shadowedProperty.isOverride()) {
        if (hasOverride || overrideFlags.testFlag(WithFinal))
            return;

        logger->log(
                "Member \"%1\" shadows member \"%1\" from base type \"%2\", use a different name or add a final or override specifier."_L1
                        .arg(name, QQmlJSUtils::getScopeName(owner, QQmlSA::ScopeType::QMLScope)),
                qmlPropertyOverride, location);
        return;
    }

    if (hasOverride) {
        logger->log(
                "Member \"%1\" overrides a non-virtual member from base type \"%2\", use a different name or mark the property as virtual in the base type."_L1
                        .arg(name, QQmlJSUtils::getScopeName(owner, QQmlSA::ScopeType::QMLScope)),
                qmlPropertyOverride, location);
        return;
    }
    logger->log("Property \"%2\" already exists in base type \"%3\", use a different name."_L1.arg(
                        name, QQmlJSUtils::getScopeName(owner, QQmlSA::ScopeType::QMLScope)),
                qmlPropertyOverride, location);
}

static void warnForDuplicates(const QQmlJSScope::ConstPtr &scope, const QString &name,
                              QLatin1String type, const QQmlJS::SourceLocation &location,
                              OverrideInformations overrideFlags, QQmlJSLogger *logger)
{
    static constexpr QLatin1String duplicateMessage =
            "Duplicated %1 name \"%2\", \"%2\" is already a %3."_L1;
    if (const auto methods = scope->ownMethods(name); !methods.isEmpty()) {
        logger->log(duplicateMessage.arg(type, name,
                                         methods.front().methodType() == QQmlSA::MethodType::Signal
                                                 ? s_signal
                                                 : s_method),
                    qmlDuplicatedName, location);
    }
    if (scope->hasOwnProperty(name))
        logger->log(duplicateMessage.arg(type, name, s_property), qmlDuplicatedName, location);

    const QQmlJSScope::ConstPtr base = scope->baseType();
    if (!base)
        return;

    warnForMethodShadowingInBase(base, name, location, logger);
    warnForPropertyShadowingInBase(base, name, location, overrideFlags, logger);
}

void LinterVisitor::handleRenamedType(UiQualifiedId *qualifiedId)
{
    m_renamedComponents.handleRenamedType(
            m_rootScopeImports.type(qualifiedId->name.toString()).scope, qualifiedId->name,
            qualifiedId->identifierToken, m_logger);
}

bool LinterVisitor::visit(Type *type)
{
    const bool result = QQmlJSImportVisitor::visit(type);

    handleRenamedType(type->typeId);

    return result;
}

void LinterVisitor::handleRecursivelyInstantiatedType(UiQualifiedId *qualifiedId)
{
    // It should be ok to reference inline components or enums inside of the current file
    if (qualifiedId->next)
        return;

    auto logWarning = [&qualifiedId, this]() {
        m_logger->log("Type \"%1\" can't be instantiated recursively"_L1.arg(qualifiedId->name),
                      qmlTypeInstantiatedRecursively, qualifiedId->identifierToken);
    };

    const QString name = qualifiedId->name.toString();
    if (m_rootScopeImports.names().contains(m_exportedRootScope, name))
        logWarning();

    // note: inline components can't be renamed via qmldir entries
    if (const auto inlineComponentName = std::get_if<InlineComponentNameType>(&m_currentRootName);
        inlineComponentName && name == *inlineComponentName) {
        logWarning();
    }
}

bool LinterVisitor::visit(QQmlJS::AST::UiPragma *pragma)
{
    const bool result = QQmlJSImportVisitor::visit(pragma);

    // The QML Engine ignores this pragma, so __don't__ set m_exportedRootScope's singleton flag with its value.
    if (pragma->name == u"Singleton")
        m_rootIsSingleton = true;

    return result;
}

void LinterVisitor::checkSingletonRoot()
{
    const bool hasQmldirSingletonEntry = m_exportedRootScope->isSingleton(); // set by importer
    const bool hasSingletonPragma = m_rootIsSingleton;

    if (hasQmldirSingletonEntry == hasSingletonPragma)
        return;

    if (hasQmldirSingletonEntry && !hasSingletonPragma) {
        m_logger->log("Type %1 declared as singleton in qmldir but missing pragma Singleton"_L1.arg(
                              m_exportedRootScope->internalName()),
                      qmlImport, QQmlJS::SourceLocation());
        return;
    }
    Q_ASSERT(!hasQmldirSingletonEntry && hasSingletonPragma);
    m_logger->log("Type %1 not declared as singleton in qmldir but using pragma Singleton"_L1.arg(
                          m_exportedRootScope->internalName()),
                  qmlImport, QQmlJS::SourceLocation());
}

bool LinterVisitor::visit(QQmlJS::AST::UiObjectDefinition *objectDefinition)
{
    handleRenamedType(objectDefinition->qualifiedTypeNameId);
    handleRecursivelyInstantiatedType(objectDefinition->qualifiedTypeNameId);
    if (!rootScopeIsValid() && !objectDefinition->qualifiedTypeNameId->name.front().isLower())
        checkSingletonRoot();

    return QQmlJSImportVisitor::visit(objectDefinition);
}

bool LinterVisitor::visit(UiPublicMember *publicMember)
{
    switch (publicMember->type) {
    case UiPublicMember::Signal: {
        const QString signalName = publicMember->name.toString();
        warnForDuplicates(m_currentScope, signalName, s_signal, publicMember->identifierToken,
                          WithoutOverride, m_logger);
        break;
    }
    case QQmlJS::AST::UiPublicMember::Property: {
        const QString propertyName = publicMember->name.toString();
        OverrideInformations flags;
        flags.setFlag(WithOverride, publicMember->isOverride());
        flags.setFlag(WithFinal, publicMember->isFinal());
        warnForDuplicates(m_currentScope, propertyName, s_property, publicMember->identifierToken,
                          flags, m_logger);
        handleRenamedType(publicMember->memberType);

        const QString typeName = publicMember->memberType->toString();
        if (typeName != "alias"_L1) {
            if (m_rootScopeImports.hasType(typeName)
                && !m_rootScopeImports.type(typeName).scope.isNull()) {
                if (m_importTypeLocationMap.contains(typeName))
                    m_usedTypes.insert(typeName);
            }
        }
        break;
    }
    }
    return QQmlJSImportVisitor::visit(publicMember);
}

bool LinterVisitor::visit(FunctionExpression *fexpr)
{
    if (m_currentScope->scopeType() == QQmlSA::ScopeType::QMLScope) {
        warnForDuplicates(m_currentScope, fexpr->name.toString(), s_method, fexpr->identifierToken,
                          WithoutOverride, m_logger);
    }
    return QQmlJSImportVisitor::visit(fexpr);
}

bool LinterVisitor::visit(FunctionDeclaration *fdecl)
{
    if (m_currentScope->scopeType() == QQmlSA::ScopeType::QMLScope) {
        warnForDuplicates(m_currentScope, fdecl->name.toString(), s_method, fdecl->identifierToken,
                          WithoutOverride, m_logger);
    }
    return QQmlJSImportVisitor::visit(fdecl);
}

/* This is a _rough_ heuristic; only meant for qmllint to avoid warnings about common constructs.
   We might want to improve it in the future if it causes issues
*/
static bool compatibilityHeuristicForFileSelector(const QQmlJSScope::ConstPtr &scope1,
                                                  const QQmlJSScope::ConstPtr &scope2)
{
    for (const auto &[propertyName, prop] : scope1->properties().asKeyValueRange())
        if (!scope2->hasProperty(propertyName))
            return false;
    for (const auto &[methodName, method] : scope1->methods().asKeyValueRange())
        if (!scope2->hasMethod(methodName))
            return false;
    return true;
}

// heuristic to check file selected files for "compability" to the unselected file.
void LinterVisitor::checkFileSelections()
{
    const QQmlJS::FileSelectorInfo info =
            m_rootScopeImports.contextualTypes().fileSelectorInfoFor(m_exportedRootScope);

    if (info.fileSelectedTypes.isEmpty() || info.mainType.isNull())
        return;

    const QString name = m_rootScopeImports.name(m_exportedRootScope);

    if (info.mainType == m_exportedRootScope) {
        // current has fileselectors -> check all fileselectors for compatiblity
        for (const auto &fileSelected : info.fileSelectedTypes) {
            if (compatibilityHeuristicForFileSelector(m_exportedRootScope,
                                                      fileSelected.type.scope)) {
                m_logger->log(
                        "Type %1 is ambiguous due to file selector usage, ignoring %2."_L1.arg(
                                name, fileSelected.type.scope->filePath()),
                        qmlImportFileSelector, m_exportedRootScope->sourceLocation());
                continue;
            }
            m_logger->log("Type %1 has a potentially incompatible file-selected variant %2."_L1.arg(
                                  name, fileSelected.type.scope->filePath()),
                          qmlImport, m_exportedRootScope->sourceLocation());
        }
        return;
    }

    // current is fileselected -> only check against "main" type for compatibility
    if (compatibilityHeuristicForFileSelector(info.mainType, m_exportedRootScope)) {
        m_logger->log(
                "File-selected type %1 is ambiguous due to file selector usage, this file will be ignored in favour of %2."_L1
                        .arg(name, info.mainType->filePath()),
                qmlImportFileSelector, m_exportedRootScope->sourceLocation());
        return;
    }
    m_logger->log("File-selected type %1 is potentially incompatible with %2."_L1.arg(
                          name, info.mainType->filePath()),
                  qmlImport, m_exportedRootScope->sourceLocation());
}

void LinterVisitor::addImportWithLocation(const QString &name, const QQmlJS::SourceLocation &loc,
                                          bool hadWarnings)
{
    if (m_importTypeLocationMap.contains(name)
        && m_importTypeLocationMap.values(name).contains(loc)) {
        return;
    }

    m_importTypeLocationMap.insert(name, loc);

    // If the import had warnings it may be "unused" because we haven't found all of its types.
    // If the type's location is not valid it's a builtin.
    // We don't need to complain about those being unused.
    if (!hadWarnings && loc.isValid())
        m_importLocations.insert(loc);
}

void LinterVisitor::addStaticImportWithLocation(const QString &name,
                                                const QQmlJS::SourceLocation &loc,
                                                bool isDependency)
{
    // Always prefer a direct import of static module to it being imported as a dependency
    if (isDependency && m_importStaticModuleLocationMap.contains(name))
        return;
    m_importStaticModuleLocationMap[name] = loc;
}

bool LinterVisitor::visit(QQmlJS::AST::IdentifierExpression *idexp)
{
    const QString name = idexp->name.toString();
    if (m_importTypeLocationMap.contains(name)) {
        m_usedTypes.insert(name);
    }

    return true;
}

void LinterVisitor::endVisit(QQmlJS::AST::FieldMemberExpression *fieldMember)
{
    // This is a rather rough approximation of "used type" but the "unused import"
    // info message doesn't have to be 100% accurate.
    const QString name = fieldMember->name.toString();
    if (m_importTypeLocationMap.contains(name)) {
        const QQmlJSImportedScope type = m_rootScopeImports.type(name);
        if (type.scope.isNull()) {
            if (m_rootScopeImports.hasType(name))
                m_usedTypes.insert(name);
        } else if (!type.scope->attachedTypeName().isEmpty()) {
            m_usedTypes.insert(name);
        }
    }
}

void LinterVisitor::checkGroupedAndAttachedScopes()
{
    for (const auto &scope : std::as_const(m_objectDefinitionScopes))
        checkGroupedAndAttachedScope(scope);
}

void LinterVisitor::checkGroupedAndAttachedScope(const QQmlJSScope::ConstPtr &scope)
{
    // These warnings do not apply for custom parsers and their children and need to be handled on a
    // case by case basis
    if (checkCustomParser(scope))
        return;

    if (!checkTypeResolved(scope))
        return;

    auto children = scope->childScopes();
    while (!children.isEmpty()) {
        auto childScope = children.takeFirst();
        const auto type = childScope->scopeType();
        switch (type) {
        case QQmlSA::ScopeType::GroupedPropertyScope:
        case QQmlSA::ScopeType::AttachedPropertyScope:
            if (!childScope->baseType() && !m_unresolvedTypes.hasSeen(childScope)) {
                // note: if we already warn about the unknown property scope then we don't have to
                // warn again about it being unresolved later
                m_logger->log(QStringLiteral("unknown %1 property scope %2.")
                                      .arg(type == QQmlSA::ScopeType::GroupedPropertyScope
                                                   ? QStringLiteral("grouped")
                                                   : QStringLiteral("attached"),
                                           childScope->internalName()),
                              qmlUnqualified, childScope->sourceLocation());
            }
            children.append(childScope->childScopes());
            break;
        default:
            break;
        }
    }
}

} // namespace QQmlJS

QT_END_NAMESPACE
