// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
// Qt-Security score:significant

#include "qqmljslintervisitor_p.h"
#include "qqmljsutils_p.h"

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

        QQmlJSFixSuggestion suggestion = { "Use a template literal instead."_L1, sl->literalToken,
                                           u"`" % templateString % u"`" };
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
                      qmlDuplicateImport, locAndNameSeen.first, true, true, {}, {},
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
        m_logger->log(u"Enums declared inside of inline component are ignored."_s, qmlSyntax,
                      uied->firstSourceLocation());
    } else if (m_currentScope->componentRootStatus() == QQmlJSScope::IsComponentRoot::No
               && !m_currentScope->isFileRootComponent()) {
        m_logger->log(u"Enum declared outside the root element. It won't be accessible."_s,
                      qmlNonRootEnums, uied->firstSourceLocation());
    }

    QHash<QStringView, const QQmlJS::AST::UiEnumMemberList *> seen;
    for (const auto *member = uied->members; member; member = member->next) {
        QStringView key = member->member;
        if (!key.front().isUpper()) {
            m_logger->log(u"Enum keys should start with an uppercase."_s, qmlSyntax,
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
        if (commentText.contains("fall through"_L1)
            || commentText.contains("fall-through"_L1)
            || commentText.contains("fallthrough"_L1)) {
            return;
        }
    }

    m_logger->log("Unterminated non-empty case block"_L1, qmlUnterminatedCase, errorLoc);
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

/*!
\internal

This assumes that there is no custom coercion enabled via \c Symbol.toPrimitive or similar.
*/
static bool isUselessExpressionStatement(ExpressionNode *ast)
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
    default:
        break;
    };
    BinaryExpression *binary = cast<BinaryExpression *>(ast);
    if (!binary)
        return false;

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
        return true;
    }
    Q_UNREACHABLE_RETURN(true);
}

static bool canHaveUselessExpressionStatement(Node *parent)
{
    return parent->kind != Node::Kind_UiScriptBinding && parent->kind != Node::Kind_UiPublicMember;
}

bool LinterVisitor::visit(ExpressionStatement *ast)
{
    QQmlJSImportVisitor::visit(ast);

    if (canHaveUselessExpressionStatement(astParentOfVisitedNode())
        && isUselessExpressionStatement(ast->expression)) {
        m_logger->log("Expression statement has no obvious effect."_L1,
                      qmlConfusingExpressionStatement,
                      combine(ast->firstSourceLocation(), ast->lastSourceLocation()));
    }

    return true;
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

static constexpr QLatin1String s_method = "method"_L1;
static constexpr QLatin1String s_signal = "signal"_L1;
static constexpr QLatin1String s_property = "property"_L1;

static void warnForDuplicates(const QQmlJSScope::ConstPtr &scope, const QString &name,
                              QLatin1String type, const QQmlJS::SourceLocation &location,
                              QQmlJSLogger *logger)
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

    static constexpr QLatin1String warningMessage =
            "%1 \"%2\" already exists in base type \"%3\", use a different name."_L1;

    if (scope->hasMethod(name)) {
        const auto owner = QQmlJSScope::ownerOfMethod(scope, name).scope;
        const bool isSignal =
                owner->methods(name).front().methodType() == QQmlJSMetaMethodType::Signal;
        logger->log(
                warningMessage.arg(isSignal ? "Signal"_L1 : "Method"_L1, name,
                                   QQmlJSUtils::getScopeName(owner, QQmlSA::ScopeType::QMLScope)),
                qmlShadow, location);
    }
    if (scope->hasProperty(name)) {
        const auto owner = QQmlJSScope::ownerOfProperty(scope, name).scope;
        logger->log(
                warningMessage.arg("Property"_L1, name,
                                   QQmlJSUtils::getScopeName(owner, QQmlSA::ScopeType::QMLScope)),
                qmlShadow, location);
    }
}

bool LinterVisitor::visit(UiPublicMember *publicMember)
{
    switch (publicMember->type) {
    case UiPublicMember::Signal: {
        const QString signalName = publicMember->name.toString();
        warnForDuplicates(m_currentScope, signalName, s_signal, publicMember->identifierToken,
                          m_logger);
        break;
    }
    case QQmlJS::AST::UiPublicMember::Property: {
        const QString propertyName = publicMember->name.toString();
        warnForDuplicates(m_currentScope, propertyName, s_property, publicMember->identifierToken,
                          m_logger);
        break;
    }
    }
    return QQmlJSImportVisitor::visit(publicMember);
}

bool LinterVisitor::visit(FunctionExpression *fexpr)
{
    if (m_currentScope->scopeType() == QQmlSA::ScopeType::QMLScope) {
        warnForDuplicates(m_currentScope, fexpr->name.toString(), s_method, fexpr->identifierToken,
                          m_logger);
    }
    return QQmlJSImportVisitor::visit(fexpr);
}

bool LinterVisitor::visit(FunctionDeclaration *fdecl)
{
    if (m_currentScope->scopeType() == QQmlSA::ScopeType::QMLScope) {
        warnForDuplicates(m_currentScope, fdecl->name.toString(), s_method, fdecl->identifierToken,
                          m_logger);
    }
    return QQmlJSImportVisitor::visit(fdecl);
}

} // namespace QQmlJS

QT_END_NAMESPACE
