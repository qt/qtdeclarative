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

#include "dumpastvisitor.h"

#include <QtQml/private/qqmljslexer_p.h>

DumpAstVisitor::DumpAstVisitor(QQmlJS::Engine *engine, Node *rootNode, CommentAstVisitor *comment)
    : m_engine(engine), m_comment(comment)
{
    // Add all completely orphaned comments
    m_result += getOrphanedComments(nullptr);

    m_scope_properties.push(ScopeProperties {});

    rootNode->accept(this);

    // We need to get rid of one new-line so our output doesn't append an empty line
    m_result.chop(1);

    // Remove trailing whitespace
    QStringList lines = m_result.split("\n");
    for (QString& line : lines) {
        while (line.endsWith(" "))
            line.chop(1);
    }

    m_result = lines.join("\n");
}

bool DumpAstVisitor::preVisit(Node *el)
{
    UiObjectMember *m = el->uiObjectMemberCast();
    if (m != 0)
        Node::accept(m->annotations, this);
    return true;
}

static QString parseUiQualifiedId(UiQualifiedId *id)
{
    QString name = id->name.toString();
    for (auto *item = id->next; item != nullptr; item = item->next) {
        name += "." + item->name;
    }

    return name;
}

static QString operatorToString(int op)
{
    switch (op)
    {
    case QSOperator::Add: return "+";
    case QSOperator::And: return "&&";
    case QSOperator::InplaceAnd: return "&=";
    case QSOperator::Assign: return "=";
    case QSOperator::BitAnd: return "&";
    case QSOperator::BitOr: return "|";
    case QSOperator::BitXor: return "^";
    case QSOperator::InplaceSub: return "-=";
    case QSOperator::Div: return "/";
    case QSOperator::InplaceDiv: return "/=";
    case QSOperator::Equal: return "==";
    case QSOperator::Exp: return "**";
    case QSOperator::InplaceExp: return "**=";
    case QSOperator::Ge: return ">=";
    case QSOperator::Gt: return ">";
    case QSOperator::In: return "in";
    case QSOperator::InplaceAdd: return "+=";
    case QSOperator::InstanceOf: return "instanceof";
    case QSOperator::Le: return "<=";
    case QSOperator::LShift: return "<<";
    case QSOperator::InplaceLeftShift: return "<<=";
    case QSOperator::Lt: return "<";
    case QSOperator::Mod: return "%";
    case QSOperator::InplaceMod: return "%=";
    case QSOperator::Mul: return "*";
    case QSOperator::InplaceMul: return "*=";
    case QSOperator::NotEqual: return "!=";
    case QSOperator::Or: return "||";
    case QSOperator::InplaceOr: return "|=";
    case QSOperator::RShift: return ">>";
    case QSOperator::InplaceRightShift: return ">>=";
    case QSOperator::StrictEqual: return "===";
    case QSOperator::StrictNotEqual: return "!==";
    case QSOperator::Sub: return "-";
    case QSOperator::URShift: return ">>>";
    case QSOperator::InplaceURightShift: return ">>>=";
    case QSOperator::InplaceXor: return "^=";
    case QSOperator::As: return "as";
    case QSOperator::Coalesce: return "??";
    case QSOperator::Invalid:
    default:
        return "INVALID";
    }
}

QString DumpAstVisitor::formatComment(const Comment &comment) const
{
    QString result;

    bool useMultilineComment = comment.isMultiline() && !comment.isSyntheticMultiline();

    if (useMultilineComment)
        result += "/*";
    else
        result += "//";

    result += comment.m_text;

    if (comment.isSyntheticMultiline())
        result = result.replace("\n","\n" + formatLine("//", false));

    if (comment.m_location == Comment::Location::Back_Inline)
        result.prepend(" ");

    if (useMultilineComment)
        result += "*/";

    return result;
}

QString DumpAstVisitor::getComment(Node *node, Comment::Location location) const
{
    const auto& comments = m_comment->attachedComments();
    if (!comments.contains(node))
        return "";

    auto comment = comments[node];

    if (comment.m_location != location)
        return "";

    return formatComment(comment);
}

QString DumpAstVisitor::getListItemComment(SourceLocation srcLocation,
                                           Comment::Location location) const {
    const auto& comments = m_comment->listComments();

    if (!comments.contains(srcLocation.begin()))
        return "";

    auto comment = comments[srcLocation.begin()];

    if (comment.m_location != location)
        return "";

    return formatComment(comment);
}

QString DumpAstVisitor::getOrphanedComments(Node *node) const {
    const auto& orphans = m_comment->orphanComments()[node];

    if (orphans.size() == 0)
        return "";

    QString result = "";

    for (const Comment& orphan : orphans) {
        result += formatLine(formatComment(orphan));
    }

    result += "\n";

    return result;
}

QString DumpAstVisitor::parseArgumentList(ArgumentList *list)
{
    QString result = "";

    for (auto *item = list; item != nullptr; item = item->next)
        result += parseExpression(item->expression) + (item->next != nullptr ? ", " : "");

    return result;
}

QString DumpAstVisitor::parseUiParameterList(UiParameterList *list) {
    QString result = "";

    for (auto *item = list; item != nullptr; item = item->next)
        result += parseUiQualifiedId(item->type) + " " + item->name + (item->next != nullptr ? ", " : "");

    return result;
}

QString DumpAstVisitor::parsePatternElement(PatternElement *element, bool scope)
{
    switch (element->type)
    {
    case PatternElement::Literal:
        return parseExpression(element->initializer);
    case PatternElement::Binding: {
        QString result = "";
        QString expr = parseExpression(element->initializer);

        if (scope) {
            switch (element->scope) {
                case VariableScope::NoScope:
                break;
            case VariableScope::Let:
                result = "let ";
                break;
            case VariableScope::Const:
                result = "const ";
                break;
            case VariableScope::Var:
                result = "var ";
                break;
            }
        }

        if (element->bindingIdentifier.isEmpty())
            result += parseExpression(element->bindingTarget);
        else
            result += element->bindingIdentifier.toString();

        if (element->typeAnnotation != nullptr)
            result += ": " + parseType(element->typeAnnotation->type);

        if (!expr.isEmpty())
            result += " = "+expr;

        return result;
    }
    default:
        m_error = true;
        return "pe_unknown";
    }
}

QString escapeString(QString string)
{
    // Handle escape sequences
    string = string.replace("\r", "\\r").replace("\n", "\\n").replace("\t", "\\t")
            .replace("\b","\\b").replace("\v", "\\v").replace("\f", "\\f");

    // Escape backslash
    string = string.replace("\\", "\\\\");

    // Escape "
    string = string.replace("\"", "\\\"");

    return "\"" + string + "\"";
}

QString DumpAstVisitor::parsePatternElementList(PatternElementList *list)
{
    QString result = "";

    for (auto *item = list; item != nullptr; item = item->next)
        result += parsePatternElement(item->element) + (item->next != nullptr ? ", " : "");

    return result;
}

QString DumpAstVisitor::parseFormalParameterList(FormalParameterList *list)
{
    QString result = "";

    for (auto *item = list; item != nullptr; item = item->next)
        result += parsePatternElement(item->element) + (item->next != nullptr ? ", " : "");

    return result;
}

QString DumpAstVisitor::parsePatternProperty(PatternProperty *property)
{
    switch (property->type) {
    case PatternElement::Getter:
        return "get "+parseFunctionExpression(cast<FunctionExpression *>(property->initializer), true);
    case PatternElement::Setter:
        return "set "+parseFunctionExpression(cast<FunctionExpression *>(property->initializer), true);
    default:
        if (property->name->kind == Node::Kind_ComputedPropertyName) {
            return "["+parseExpression(cast<ComputedPropertyName *>(property->name)->expression)+"]: "+parsePatternElement(property, false);
        } else {
            return escapeString(property->name->asString())+": "+parsePatternElement(property, false);
        }
    }
}

QString DumpAstVisitor::parsePatternPropertyList(PatternPropertyList *list)
{
    QString result = "";

    for (auto *item = list; item != nullptr; item = item->next) {
        result += formatLine(parsePatternProperty(item->property) + (item->next != nullptr ? "," : ""));
    }

    return result;
}

QString DumpAstVisitor::parseFunctionExpression(FunctionExpression *functExpr, bool omitFunction)
{
    m_indentLevel++;
    QString result;

    if (!functExpr->isArrowFunction) {
        result += omitFunction ? "" : "function";

        if (functExpr->isGenerator)
            result += "*";

        if (!functExpr->name.isEmpty())
            result += (omitFunction ? "" : " ") + functExpr->name;

        result += "("+parseFormalParameterList(functExpr->formals)+")";

        if (functExpr->typeAnnotation != nullptr)
            result += " : " + parseType(functExpr->typeAnnotation->type);

        result += " {\n" + parseStatementList(functExpr->body);
    } else {
         result += "("+parseFormalParameterList(functExpr->formals)+")";

         if (functExpr->typeAnnotation != nullptr)
             result += " : " + parseType(functExpr->typeAnnotation->type);

         result += " => {\n" + parseStatementList(functExpr->body);
    }

    m_indentLevel--;

    result += formatLine("}", false);

    return result;

}

QString DumpAstVisitor::parseType(Type *type) {
    QString result = parseUiQualifiedId(type->typeId);

    if (type->typeArguments != nullptr) {
        TypeArgumentList *list = cast<TypeArgumentList *>(type->typeArguments);

        result += "<";

        for (auto *item = list; item != nullptr; item = item->next) {
            result += parseType(item->typeId) + (item->next != nullptr ? ", " : "");
        }

        result += ">";
    }

    return result;
}

QString DumpAstVisitor::parseExpression(ExpressionNode *expression)
{
    if (expression == nullptr)
        return "";

    switch (expression->kind)
    {
    case Node::Kind_ArrayPattern:
        return "["+parsePatternElementList(cast<ArrayPattern *>(expression)->elements)+"]";
    case Node::Kind_IdentifierExpression:
        return cast<IdentifierExpression*>(expression)->name.toString();
    case Node::Kind_FieldMemberExpression: {
        auto *fieldMemberExpr = cast<FieldMemberExpression *>(expression);
        QString result = parseExpression(fieldMemberExpr->base);

        // If we're operating on a numeric literal, always put it in braces
        if (fieldMemberExpr->base->kind == Node::Kind_NumericLiteral)
            result = "(" + result + ")";

        result += "." + fieldMemberExpr->name.toString();

        return result;
    }
    case Node::Kind_ArrayMemberExpression: {
        auto *arrayMemberExpr = cast<ArrayMemberExpression *>(expression);
        return parseExpression(arrayMemberExpr->base)
                + "[" + parseExpression(arrayMemberExpr->expression) + "]";
    }
    case Node::Kind_NestedExpression:
        return "("+parseExpression(cast<NestedExpression *>(expression)->expression)+")";
    case Node::Kind_TrueLiteral:
        return "true";
    case Node::Kind_FalseLiteral:
        return "false";
    case Node::Kind_FunctionExpression:
    {
        auto *functExpr = cast<FunctionExpression *>(expression);
        return parseFunctionExpression(functExpr);
    }
    case Node::Kind_NullExpression:
        return "null";
    case Node::Kind_ThisExpression:
        return "this";
    case Node::Kind_PostIncrementExpression:
        return parseExpression(cast<PostIncrementExpression *>(expression)->base)+"++";
    case Node::Kind_PreIncrementExpression:
        return "++"+parseExpression(cast<PreIncrementExpression *>(expression)->expression);
    case Node::Kind_PostDecrementExpression:
        return parseExpression(cast<PostDecrementExpression *>(expression)->base)+"--";
    case Node::Kind_PreDecrementExpression:
        return "--"+parseExpression(cast<PreDecrementExpression *>(expression)->expression);
    case Node::Kind_NumericLiteral:
        return QString::number(cast<NumericLiteral *>(expression)->value);
    case Node::Kind_TemplateLiteral: {
        auto firstSrcLoc = cast<TemplateLiteral *>(expression)->firstSourceLocation();
        auto lastSrcLoc = cast<TemplateLiteral *>(expression)->lastSourceLocation();
        return m_engine->code().mid(static_cast<int>(firstSrcLoc.begin()),
                                    static_cast<int>(lastSrcLoc.end() - firstSrcLoc.begin()));
    }
    case Node::Kind_StringLiteral: {
        auto srcLoc = cast<StringLiteral *>(expression)->firstSourceLocation();
        return m_engine->code().mid(static_cast<int>(srcLoc.begin()),
                                    static_cast<int>(srcLoc.end() - srcLoc.begin()));
    }
    case Node::Kind_BinaryExpression: {
        auto *binExpr = expression->binaryExpressionCast();
        return parseExpression(binExpr->left) + " " + operatorToString(binExpr->op)
                + " " + parseExpression(binExpr->right);
    }
    case Node::Kind_CallExpression: {
        auto *callExpr = cast<CallExpression *>(expression);

        return parseExpression(callExpr->base) + "(" + parseArgumentList(callExpr->arguments) + ")";
    }
    case Node::Kind_NewExpression:
        return "new "+parseExpression(cast<NewExpression *>(expression)->expression);
    case Node::Kind_NewMemberExpression: {
        auto *newMemberExpression = cast<NewMemberExpression *>(expression);
        return "new "+parseExpression(newMemberExpression->base)
                + "(" +parseArgumentList(newMemberExpression->arguments)+")";
    }
    case Node::Kind_DeleteExpression:
        return "delete " + parseExpression(cast<DeleteExpression *>(expression)->expression);
    case Node::Kind_VoidExpression:
        return "void " + parseExpression(cast<VoidExpression *>(expression)->expression);
    case Node::Kind_TypeOfExpression:
        return "typeof " + parseExpression(cast<TypeOfExpression *>(expression)->expression);
    case Node::Kind_UnaryPlusExpression:
        return "+" + parseExpression(cast<UnaryPlusExpression *>(expression)->expression);
    case Node::Kind_UnaryMinusExpression:
        return "-" + parseExpression(cast<UnaryMinusExpression *>(expression)->expression);
    case Node::Kind_NotExpression:
        return "!" + parseExpression(cast<NotExpression *>(expression)->expression);
    case Node::Kind_TildeExpression:
        return "~" + parseExpression(cast<TildeExpression *>(expression)->expression);
    case Node::Kind_ConditionalExpression: {
        auto *condExpr = cast<ConditionalExpression *>(expression);

        QString result = "";

        result += parseExpression(condExpr->expression) + " ? ";
        result += parseExpression(condExpr->ok) + " : ";
        result += parseExpression(condExpr->ko);

        return result;
    }
    case Node::Kind_YieldExpression: {
        auto *yieldExpr = cast<YieldExpression*>(expression);

        QString result = "yield";

        if (yieldExpr->isYieldStar)
            result += "*";

        if (yieldExpr->expression)
            result += " " + parseExpression(yieldExpr->expression);

        return result;
    }
    case Node::Kind_ObjectPattern: {
        auto *objectPattern = cast<ObjectPattern*>(expression);
        QString result = "{\n";

        m_indentLevel++;
        result += parsePatternPropertyList(objectPattern->properties);
        m_indentLevel--;

        result += formatLine("}", false);

        return result;
    }
    case Node::Kind_Expression: {
        auto* expr = cast<Expression*>(expression);
        return parseExpression(expr->left)+", "+parseExpression(expr->right);
    }
    case Node::Kind_Type: {
        auto* type = reinterpret_cast<Type*>(expression);
        return parseType(type);
    }
    case Node::Kind_RegExpLiteral: {
        auto* regexpLiteral = cast<RegExpLiteral*>(expression);
        QString result = "/"+regexpLiteral->pattern+"/";

        if (regexpLiteral->flags & QQmlJS::Lexer::RegExp_Unicode)
            result += "u";
        if (regexpLiteral->flags & QQmlJS::Lexer::RegExp_Global)
            result += "g";
        if (regexpLiteral->flags & QQmlJS::Lexer::RegExp_Multiline)
            result += "m";
        if (regexpLiteral->flags & QQmlJS::Lexer::RegExp_Sticky)
            result += "y";
        if (regexpLiteral->flags & QQmlJS::Lexer::RegExp_IgnoreCase)
            result += "i";

        return result;
    }
    default:
        m_error = true;
        return "unknown_expression_"+QString::number(expression->kind);
    }
}

QString DumpAstVisitor::parseVariableDeclarationList(VariableDeclarationList *list)
{
    QString result = "";

    for (auto *item = list; item != nullptr; item = item->next) {
        result += parsePatternElement(item->declaration, (item == list))
                + (item->next != nullptr ? ", " : "");
    }

    return result;
}

QString DumpAstVisitor::parseCaseBlock(CaseBlock *block)
{
    QString result = "{\n";

    for (auto *item = block->clauses; item != nullptr; item = item->next) {
        result += formatLine("case "+parseExpression(item->clause->expression)+":");
        m_indentLevel++;
        result += parseStatementList(item->clause->statements);
        m_indentLevel--;
    }

    if (block->defaultClause) {
        result += formatLine("default:");
        m_indentLevel++;
        result += parseStatementList(block->defaultClause->statements);
        m_indentLevel--;
    }

    result += formatLine("}", false);

    return result;
}

QString DumpAstVisitor::parseExportSpecifier(ExportSpecifier *specifier)
{
    QString result = specifier->identifier.toString();

    if (!specifier->exportedIdentifier.isEmpty())
        result += " as " + specifier->exportedIdentifier;

    return result;
}

QString DumpAstVisitor::parseExportsList(ExportsList *list)
{
    QString result = "";

    for (auto *item = list; item != nullptr; item = item->next) {
        result += formatLine(parseExportSpecifier(item->exportSpecifier)
                             + (item->next != nullptr ? "," : ""));
    }

    return result;
}

bool needsSemicolon(int kind)
{
    switch (kind) {
    case Node::Kind_ForStatement:
    case Node::Kind_ForEachStatement:
    case Node::Kind_IfStatement:
    case Node::Kind_SwitchStatement:
    case Node::Kind_WhileStatement:
    case Node::Kind_DoWhileStatement:
    case Node::Kind_TryStatement:
    case Node::Kind_WithStatement:
        return false;
    default:
        return true;
    }
}

QString DumpAstVisitor::parseBlock(Block *block, bool hasNext, bool allowBraceless)
{
    bool hasOneLine =
            (block->statements != nullptr && block->statements->next == nullptr) && allowBraceless;

    QString result = hasOneLine ? "\n" : "{\n";
    m_indentLevel++;
    result += parseStatementList(block->statements);
    m_indentLevel--;

    if (hasNext)
        result += formatLine(hasOneLine ? "" : "} ", false);

    if (!hasNext && !hasOneLine)
        result += formatLine("}", false);

    if (block->statements) {
        m_blockNeededBraces |= !needsSemicolon(block->statements->statement->kind)
                || (block->statements->next != nullptr);
    } else {
        m_blockNeededBraces = true;
    }

    return result;
}

QString DumpAstVisitor::parseStatement(Statement *statement, bool blockHasNext,
                                       bool blockAllowBraceless)
{
    if (statement == nullptr)
        return "";

    switch (statement->kind)
    {
    case Node::Kind_EmptyStatement:
        return "";
    case Node::Kind_ExpressionStatement:
        return parseExpression(cast<ExpressionStatement *>(statement)->expression);
    case Node::Kind_VariableStatement:
        return parseVariableDeclarationList(cast<VariableStatement *>(statement)->declarations);
    case Node::Kind_ReturnStatement:
        return "return "+parseExpression(cast<ReturnStatement *>(statement)->expression);
    case Node::Kind_ContinueStatement:
        return "continue";
    case Node::Kind_BreakStatement:
        return "break";
    case Node::Kind_SwitchStatement: {
        auto *switchStatement = cast<SwitchStatement *>(statement);

        QString result = "switch ("+parseExpression(switchStatement->expression)+") ";

        result += parseCaseBlock(switchStatement->block);

        return result;
    }
    case Node::Kind_IfStatement: {
        auto *ifStatement = cast<IfStatement *>(statement);

        m_blockNeededBraces = !blockAllowBraceless;

        QString ifFalse = parseStatement(ifStatement->ko, false, true);
        QString ifTrue = parseStatement(ifStatement->ok, !ifFalse.isEmpty(), true);

        bool ifTrueBlock = ifStatement->ok->kind == Node::Kind_Block;
        bool ifFalseBlock = ifStatement->ko
                ? (ifStatement->ko->kind == Node::Kind_Block || ifStatement->ko->kind == Node::Kind_IfStatement)
                : false;

        if (m_blockNeededBraces) {
            ifFalse = parseStatement(ifStatement->ko, false, false);
            ifTrue = parseStatement(ifStatement->ok, !ifFalse.isEmpty(), false);
        }

        if (ifStatement->ok->kind != Node::Kind_Block)
            ifTrue += ";";

        if (ifStatement->ko && ifStatement->ko->kind != Node::Kind_Block && ifStatement->ko->kind != Node::Kind_IfStatement)
            ifFalse += ";";

        QString result = "if (" + parseExpression(ifStatement->expression) + ")";

        if (m_blockNeededBraces) {
            if (ifStatement->ok->kind != Node::Kind_Block) {
                QString result = "{\n";
                m_indentLevel++;
                result += formatLine(ifTrue);
                m_indentLevel--;
                result += formatLine("} ", false);
                ifTrue = result;
                ifTrueBlock = true;
            }

            if (ifStatement->ko && ifStatement->ko->kind != Node::Kind_Block && ifStatement->ko->kind != Node::Kind_IfStatement) {
                QString result = "{\n";
                m_indentLevel++;
                result += formatLine(ifFalse);
                m_indentLevel--;
                result += formatLine("} ", false);
                ifFalse = result;
                ifFalseBlock = true;
            }
        }

        if (ifTrueBlock) {
            result += " " + ifTrue;
        } else {
            result += "\n";
            m_indentLevel++;
            result += formatLine(ifTrue);
            m_indentLevel--;
        }

        if (!ifFalse.isEmpty())
        {
            if (ifTrueBlock)
                result += "else";
            else
                result += formatLine("else", false);

            if (ifFalseBlock) {
                // Blocks generate an extra newline that we don't want here.
                if (!m_blockNeededBraces && ifFalse.endsWith(QLatin1String("\n")))
                    ifFalse.chop(1);

                result += " " + ifFalse;
            } else {
                result += "\n";
                m_indentLevel++;
                result += formatLine(ifFalse, false);
                m_indentLevel--;
            }
        }

        return result;
    }
    case Node::Kind_ForStatement: {
        auto *forStatement = cast<ForStatement *>(statement);

        QString expr = parseExpression(forStatement->expression);
        QString result = "for (";

        result += parseVariableDeclarationList(forStatement->declarations);

        result += "; ";

        result += parseExpression(forStatement->condition) + "; ";
        result += parseExpression(forStatement->expression)+")";

        const QString statement = parseStatement(forStatement->statement);

        if (!statement.isEmpty())
            result += " "+statement;
        else
            result += ";";

        return result;
    }
    case Node::Kind_ForEachStatement: {
        auto *forEachStatement = cast<ForEachStatement *>(statement);

        QString result = "for (";

        PatternElement *patternElement = cast<PatternElement *>(forEachStatement->lhs);

        if (patternElement != nullptr)
            result += parsePatternElement(patternElement);
        else
            result += parseExpression(forEachStatement->lhs->expressionCast());

        switch (forEachStatement->type)
        {
        case ForEachType::In:
            result += " in ";
            break;
        case ForEachType::Of:
            result += " of ";
            break;
        }

        result += parseExpression(forEachStatement->expression) + ")";

        const QString statement = parseStatement(forEachStatement->statement);

        if (!statement.isEmpty())
            result += " "+statement;
        else
            result += ";";

        return result;
    }
    case Node::Kind_WhileStatement: {
        auto *whileStatement = cast<WhileStatement *>(statement);

        m_blockNeededBraces = false;

        auto statement = parseStatement(whileStatement->statement, false, true);

        QString result = "while ("+parseExpression(whileStatement->expression) + ")";

        if (!statement.isEmpty())
            result += (m_blockNeededBraces ? " " : "") + statement;
        else
            result += ";";

        return result;
    }
    case Node::Kind_DoWhileStatement: {
        auto *doWhileStatement = cast<DoWhileStatement *>(statement);
        return "do " + parseBlock(cast<Block *>(doWhileStatement->statement), true, false)
                + "while (" + parseExpression(doWhileStatement->expression) + ")";
    }
    case Node::Kind_TryStatement: {
        auto *tryStatement = cast<TryStatement *>(statement);

        Catch *catchExpr = tryStatement->catchExpression;
        Finally *finallyExpr = tryStatement->finallyExpression;

        QString result;

        result += "try " + parseBlock(cast<Block *>(tryStatement->statement), true, false);

        result += "catch (" + parsePatternElement(catchExpr->patternElement, false) + ") "
                + parseBlock(cast<Block *>(catchExpr->statement), finallyExpr, false);

        if (finallyExpr) {
            result += "finally " + parseBlock(cast<Block *>(tryStatement->statement), false, false);
        }

        return result;
    }
    case Node::Kind_Block: {
        return parseBlock(cast<Block *>(statement), blockHasNext, blockAllowBraceless);
    }
    case Node::Kind_ThrowStatement:
        return "throw "+parseExpression(cast<ThrowStatement *>(statement)->expression);
    case Node::Kind_LabelledStatement: {
        auto *labelledStatement = cast<LabelledStatement *>(statement);
        QString result = labelledStatement->label+":\n";
        result += formatLine(parseStatement(labelledStatement->statement), false);

        return result;
    }
    case Node::Kind_WithStatement: {
        auto *withStatement = cast<WithStatement *>(statement);
        return "with (" + parseExpression(withStatement->expression) + ") "
                + parseStatement(withStatement->statement);
    }
    case Node::Kind_DebuggerStatement: {
        return "debugger";
    }
    case Node::Kind_ExportDeclaration:
        m_error = true;
        return "export_decl_unsupported";
    case Node::Kind_ImportDeclaration:
        m_error = true;
        return "import_decl_unsupported";
    default:
        m_error = true;
        return "unknown_statement_"+QString::number(statement->kind);
    }
}

QString DumpAstVisitor::parseStatementList(StatementList *list)
{
    QString result = "";

    if (list == nullptr)
        return "";

    result += getOrphanedComments(list);

    for (auto *item = list; item != nullptr; item = item->next) {
        QString statement = parseStatement(item->statement->statementCast(), false, true);
        if (statement.isEmpty())
            continue;

        QString commentFront = getComment(item->statement, Comment::Location::Front);
        QString commentBackInline = getComment(item->statement, Comment::Location::Back_Inline);

        if (!commentFront.isEmpty())
            result += formatLine(commentFront);

        result += formatLine(statement + (needsSemicolon(item->statement->kind) ? ";" : "")
                             + commentBackInline);
    }

    return result;
}

bool DumpAstVisitor::visit(UiPublicMember *node) {

    QString commentFront = getComment(node, Comment::Location::Front);
    QString commentBackInline = getComment(node, Comment::Location::Back_Inline);

    switch (node->type)
    {
    case UiPublicMember::Signal:
        if (scope().m_firstSignal) {
            if (scope().m_firstOfAll)
                scope().m_firstOfAll = false;
            else
                addNewLine();

            scope().m_firstSignal = false;
        }

        addLine(commentFront);
        addLine("signal "+node->name.toString()+"("+parseUiParameterList(node->parameters) + ")"
                + commentBackInline);
        break;
    case UiPublicMember::Property: {
        if (scope().m_firstProperty) {
            if (scope().m_firstOfAll)
                scope().m_firstOfAll = false;
            else
                addNewLine();

            scope().m_firstProperty = false;
        }

        const bool is_required = node->requiredToken.isValid();
        const bool is_default = node->defaultToken.isValid();
        const bool is_readonly = node->readonlyToken.isValid();
        const bool has_type_modifier = node->typeModifierToken.isValid();

        QString prefix = "";
        QString statement = parseStatement(node->statement);

        if (!statement.isEmpty())
            statement.prepend(": ");

        if (is_required)
            prefix += "required ";

        if (is_default)
            prefix += "default ";

        if (is_readonly)
            prefix += "readonly ";

        QString member_type = parseUiQualifiedId(node->memberType);

        if (has_type_modifier)
            member_type = node->typeModifier + "<" + member_type + ">";

        addLine(commentFront);
        if (is_readonly && statement.isEmpty()
                && scope().m_bindings.contains(node->name.toString())) {
            m_result += formatLine(prefix + "property " + member_type + " ", false);

            scope().m_pendingBinding = true;
        } else {
            addLine(prefix + "property " + member_type + " "
                    + node->name+statement + commentBackInline);
        }
        break;
    }
    }

    return true;
}

QString DumpAstVisitor::generateIndent() const {
    constexpr int IDENT_WIDTH = 4;

    QString indent = "";
    for (int i = 0; i < IDENT_WIDTH*m_indentLevel; i++)
        indent += " ";

    return indent;
}

QString DumpAstVisitor::formatLine(QString line, bool newline) const {
    QString result = generateIndent() + line;
    if (newline)
        result += "\n";

    return result;
}

void DumpAstVisitor::addNewLine(bool always) {
    if (!always && m_result.endsWith("\n\n"))
        return;

    m_result += "\n";
}

void DumpAstVisitor::addLine(QString line) {
    // addLine does not support empty lines, use addNewLine(true) for that
    if (line.isEmpty())
        return;

    m_result += formatLine(line);
}

QHash<QString, UiObjectMember*> findBindings(UiObjectMemberList *list) {
    QHash<QString, UiObjectMember*> bindings;

    // This relies on RestructureASTVisitor having run beforehand

    for (auto *item = list; item != nullptr; item = item->next) {
        switch (item->member->kind) {
        case Node::Kind_UiPublicMember: {
            UiPublicMember *member = cast<UiPublicMember *>(item->member);

            if (member->type != UiPublicMember::Property)
                continue;

            bindings[member->name.toString()] = nullptr;

            break;
        }
        case Node::Kind_UiObjectBinding: {
            UiObjectBinding *binding = cast<UiObjectBinding *>(item->member);

            const QString name = parseUiQualifiedId(binding->qualifiedId);

            if (bindings.contains(name))
                bindings[name] = binding;

            break;
        }
        case Node::Kind_UiArrayBinding: {
            UiArrayBinding *binding = cast<UiArrayBinding *>(item->member);

            const QString name = parseUiQualifiedId(binding->qualifiedId);

            if (bindings.contains(name))
                bindings[name] = binding;

            break;
        }
        case Node::Kind_UiScriptBinding:
            // We can ignore UiScriptBindings since those are actually properly attached to the property
            break;
        }
    }

    return bindings;
}

bool DumpAstVisitor::visit(UiInlineComponent *node)
{
    m_component_name = node->name.toString();
    return true;
}

bool DumpAstVisitor::visit(UiObjectDefinition *node) {
    if (scope().m_firstObject) {
        if (scope().m_firstOfAll)
            scope().m_firstOfAll = false;
        else
            addNewLine();

        scope().m_firstObject = false;
    }

    addLine(getComment(node, Comment::Location::Front));
    addLine(getComment(node, Comment::Location::Front_Inline));

    QString component = "";

    if (!m_component_name.isEmpty()) {
        component = "component "+m_component_name+": ";
        m_component_name = "";
    }

    addLine(component + parseUiQualifiedId(node->qualifiedTypeNameId) + " {");

    m_indentLevel++;

    ScopeProperties props;
    props.m_bindings = findBindings(node->initializer->members);
    m_scope_properties.push(props);

    m_result += getOrphanedComments(node);

    return true;
}

void DumpAstVisitor::endVisit(UiObjectDefinition *node) {
    m_indentLevel--;

    m_scope_properties.pop();

    bool need_comma = scope().m_inArrayBinding && scope().m_lastInArrayBinding != node;

    addLine(need_comma ? "}," : "}");
    addLine(getComment(node, Comment::Location::Back));
    if (!scope().m_inArrayBinding)
        addNewLine();
}

bool DumpAstVisitor::visit(UiEnumDeclaration *node)  {

    addNewLine();

    addLine(getComment(node, Comment::Location::Front));
    addLine("enum " + node->name + " {");
    m_indentLevel++;
    m_result += getOrphanedComments(node);

    return true;
}

void DumpAstVisitor::endVisit(UiEnumDeclaration *) {
    m_indentLevel--;
    addLine("}");

    addNewLine();
}

bool DumpAstVisitor::visit(UiEnumMemberList *node) {
    for (auto *members = node; members != nullptr; members = members->next) {

        addLine(getListItemComment(members->memberToken, Comment::Location::Front));

        QString line = members->member.toString();

        if (members->valueToken.isValid())
            line += " = "+QString::number(members->value);

        if (members->next != nullptr)
            line += ",";

        line += getListItemComment(members->memberToken, Comment::Location::Back_Inline);

        addLine(line);
    }

    return true;
}

bool DumpAstVisitor::visit(UiScriptBinding *node) {
    if (scope().m_firstBinding) {
        if (scope().m_firstOfAll)
            scope().m_firstOfAll = false;
        else
            addNewLine();

        if (parseUiQualifiedId(node->qualifiedId) != "id")
            scope().m_firstBinding = false;
    }

    addLine(getComment(node, Comment::Location::Front));

    bool multiline = !needsSemicolon(node->statement->kind);

    if (multiline) {
        m_indentLevel++;
    }

    QString statement = parseStatement(node->statement);

    if (multiline) {
        statement = "{\n" + formatLine(statement);
        m_indentLevel--;
        statement += formatLine("}", false);
    }

    QString result = parseUiQualifiedId(node->qualifiedId) + ":";

    if (!statement.isEmpty())
        result += " "+statement;
    else
        result += ";";

    result += getComment(node, Comment::Location::Back_Inline);

    addLine(result);

    return true;
}

bool DumpAstVisitor::visit(UiArrayBinding *node) {
    if (!scope().m_pendingBinding && scope().m_firstBinding) {
        if (scope().m_firstOfAll)
            scope().m_firstOfAll = false;
        else
            addNewLine();

        scope().m_firstBinding = false;
    }

    if (scope().m_pendingBinding) {
        m_result += parseUiQualifiedId(node->qualifiedId)+ ": [\n";
        scope().m_pendingBinding = false;
    } else {
        addLine(getComment(node, Comment::Location::Front));
        addLine(parseUiQualifiedId(node->qualifiedId)+ ": [");
    }

    m_indentLevel++;

    ScopeProperties props;
    props.m_inArrayBinding = true;

    for (auto *item = node->members; item != nullptr; item = item->next) {
        if (item->next == nullptr)
            props.m_lastInArrayBinding = item->member;
    }

    m_scope_properties.push(props);

    m_result += getOrphanedComments(node);

    return true;
}

void DumpAstVisitor::endVisit(UiArrayBinding *) {
    m_indentLevel--;
    m_scope_properties.pop();
    addLine("]");
}

bool DumpAstVisitor::visit(FunctionDeclaration *node) {
    if (scope().m_firstFunction) {
        if (scope().m_firstOfAll)
            scope().m_firstOfAll = false;
        else
            addNewLine();

        scope().m_firstFunction = false;
    }

    addLine(getComment(node, Comment::Location::Front));

    QString head = "function";

    if (node->isGenerator)
        head += "*";

    head += " "+node->name+"("+parseFormalParameterList(node->formals)+")";

    if (node->typeAnnotation != nullptr)
        head += " : " + parseType(node->typeAnnotation->type);

    head += " {";

    addLine(head);
    m_indentLevel++;

    return true;
}

void DumpAstVisitor::endVisit(FunctionDeclaration *node)
{
    m_result += parseStatementList(node->body);
    m_indentLevel--;
    addLine("}");
    addNewLine();
}

bool DumpAstVisitor::visit(UiObjectBinding *node) {
    if (!scope().m_pendingBinding && scope().m_firstObject) {
        if (scope().m_firstOfAll)
            scope().m_firstOfAll = false;
        else
            addNewLine();

        scope().m_firstObject = false;
    }

    QString name = parseUiQualifiedId(node->qualifiedTypeNameId);

    QString result = name;

    ScopeProperties props;
    props.m_bindings = findBindings(node->initializer->members);
    m_scope_properties.push(props);

    if (node->hasOnToken)
        result += " on "+parseUiQualifiedId(node->qualifiedId);
    else
        result.prepend(parseUiQualifiedId(node->qualifiedId) + ": ");

    if (scope().m_pendingBinding) {
        m_result += result + " {\n";

        scope().m_pendingBinding = false;
    } else {
        addNewLine();
        addLine(getComment(node, Comment::Location::Front));
        addLine(getComment(node, Comment::Location::Front_Inline));
        addLine(result + " {");
    }

    m_indentLevel++;

    return true;
}

void DumpAstVisitor::endVisit(UiObjectBinding *node) {
    m_indentLevel--;
    m_scope_properties.pop();

    addLine("}");
    addLine(getComment(node, Comment::Location::Back));

    addNewLine();
}

bool DumpAstVisitor::visit(UiImport *node) {
    scope().m_firstOfAll = false;

    addLine(getComment(node, Comment::Location::Front));

    QString result = "import ";

    if (!node->fileName.isEmpty())
        result += escapeString(node->fileName.toString());
    else
        result += parseUiQualifiedId(node->importUri);

    if (node->version) {
        result += " " + QString::number(node->version->majorVersion) + "."
                + QString::number(node->version->minorVersion);
    }

    if (node->asToken.isValid()) {
        result +=" as " + node->importId;
    }

    result += getComment(node, Comment::Location::Back_Inline);

    addLine(result);

    return true;
}

bool DumpAstVisitor::visit(UiPragma *node) {
    scope().m_firstOfAll = false;

    addLine(getComment(node, Comment::Location::Front));
    QString result = "pragma "+ node->name;
    result += getComment(node, Comment::Location::Back_Inline);

    addLine(result);

    return true;
}

bool DumpAstVisitor::visit(UiAnnotation *node)
{
    if (scope().m_firstObject) {
        if (scope().m_firstOfAll)
            scope().m_firstOfAll = false;
        else
            addNewLine();

        scope().m_firstObject = false;
    }

    addLine(getComment(node, Comment::Location::Front));
    addLine(QLatin1String("@") + parseUiQualifiedId(node->qualifiedTypeNameId) + " {");

    m_indentLevel++;

    ScopeProperties props;
    props.m_bindings = findBindings(node->initializer->members);
    m_scope_properties.push(props);

    m_result += getOrphanedComments(node);

    return true;
}

void DumpAstVisitor::endVisit(UiAnnotation *node) {
    m_indentLevel--;

    m_scope_properties.pop();

    addLine("}");
    addLine(getComment(node, Comment::Location::Back));
}
