// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomreformatter_p.h"
#include "qqmldomcomments_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>

#include <QString>
#include <algorithm>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

using namespace AST;

bool ScriptFormatter::preVisit(Node *n)
{
    if (const CommentedElement *c = comments->commentForNode(n, CommentAnchor{})) {
        c->writePre(lw);
        postOps[n].append([c, this]() { c->writePost(lw); });
    }
    return true;
}
void ScriptFormatter::postVisit(Node *n)
{
    for (auto &op : postOps[n]) {
        op();
    }
    postOps.remove(n);
}

void ScriptFormatter::lnAcceptIndented(Node *node)
{
    int indent = lw.increaseIndent(1);
    lw.ensureNewline();
    accept(node);
    lw.decreaseIndent(1, indent);
}

bool ScriptFormatter::acceptBlockOrIndented(Node *ast, bool finishWithSpaceOrNewline)
{
    if (auto *es = cast<EmptyStatement *>(ast)) {
        writeOutSemicolon(es);
        return false;
    }
    if (cast<Block *>(ast)) {
        lw.lineWriter.ensureSpace();
        accept(ast);
        if (finishWithSpaceOrNewline)
            lw.lineWriter.ensureSpace();
        return true;
    } else {
        if (finishWithSpaceOrNewline)
            postOps[ast].append([this]() { this->newLine(); });
        lnAcceptIndented(ast);
        return false;
    }
}

bool ScriptFormatter::visit(ThisExpression *ast)
{
    out(ast->thisToken);
    return true;
}

bool ScriptFormatter::visit(NullExpression *ast)
{
    out(ast->nullToken);
    return true;
}
bool ScriptFormatter::visit(TrueLiteral *ast)
{
    out(ast->trueToken);
    return true;
}
bool ScriptFormatter::visit(FalseLiteral *ast)
{
    out(ast->falseToken);
    return true;
}

bool ScriptFormatter::visit(IdentifierExpression *ast)
{
    out(ast->identifierToken);
    return true;
}
bool ScriptFormatter::visit(StringLiteral *ast)
{
    // correctly handle multiline literals
    if (ast->literalToken.length == 0)
        return true;
    QStringView str = m_script->loc2Str(ast->literalToken);
    if (lw.indentNextlines && str.contains(QLatin1Char('\n'))) {
        out(str.mid(0, 1));
        lw.indentNextlines = false;
        out(str.mid(1));
        lw.indentNextlines = true;
    } else {
        out(str);
    }
    return true;
}
bool ScriptFormatter::visit(NumericLiteral *ast)
{
    out(ast->literalToken);
    return true;
}
bool ScriptFormatter::visit(RegExpLiteral *ast)
{
    out(ast->literalToken);
    return true;
}

bool ScriptFormatter::visit(ArrayPattern *ast)
{
    outWithComments(ast->lbracketToken, ast);
    int baseIndent = lw.increaseIndent(1);
    if (ast->elements) {
        accept(ast->elements);
        outWithComments(ast->commaToken, ast);
        auto lastElement = lastListElement(ast->elements);
        if (lastElement->element && cast<ObjectPattern *>(lastElement->element->initializer)) {
            newLine();
        }
    } else {
        outWithComments(ast->commaToken, ast);
    }
    lw.decreaseIndent(1, baseIndent);
    outWithComments(ast->rbracketToken, ast);
    return false;
}

bool ScriptFormatter::visit(ObjectPattern *ast)
{
    outWithComments(ast->lbraceToken, ast);
    ++expressionDepth;
    if (ast->properties) {
        lnAcceptIndented(ast->properties);
        newLine();
    }
    --expressionDepth;
    outWithComments(ast->rbraceToken, ast);
    return false;
}

bool ScriptFormatter::visit(PatternElementList *ast)
{
    for (PatternElementList *it = ast; it; it = it->next) {
        const bool isObjectInitializer =
                it->element && cast<ObjectPattern *>(it->element->initializer);
        if (isObjectInitializer)
            newLine();

        if (it->elision)
            accept(it->elision);
        if (it->element)
            accept(it->element);
        if (it->next) {
            out(",");
            lw.lineWriter.ensureSpace();
            if (isObjectInitializer)
                newLine();
        }
    }
    return false;
}

bool ScriptFormatter::visit(PatternPropertyList *ast)
{
    for (PatternPropertyList *it = ast; it; it = it->next) {
        accept(it->property);
        if (it->next) {
            out(",");
            newLine();
        }
    }
    return false;
}

// https://262.ecma-international.org/7.0/#prod-PropertyDefinition
bool ScriptFormatter::visit(AST::PatternProperty *property)
{
    if (property->type == PatternElement::Getter || property->type == PatternElement::Setter
        || property->type == PatternElement::Method) {
        // note that MethodDefinitions and FunctionDeclarations have different syntax
        // https://262.ecma-international.org/7.0/#prod-MethodDefinition
        // https://262.ecma-international.org/7.0/#prod-FunctionDeclaration
        // hence visit(FunctionDeclaration*) is not quite appropriate here
        if (property->type == PatternProperty::Getter) {
            out("get");
            lw.lineWriter.ensureSpace();
        } else if (property->type == PatternProperty::Setter) {
            out("set");
            lw.lineWriter.ensureSpace();
        }
        FunctionExpression *f = AST::cast<FunctionExpression *>(property->initializer);
        if (f->isGenerator) {
            out("*");
        }
        accept(property->name);
        out(f->lparenToken);
        accept(f->formals);
        out(f->rparenToken);
        lw.lineWriter.ensureSpace();
        out(f->lbraceToken);
        const bool scoped = f->lbraceToken.isValid();
        if (scoped)
            ++expressionDepth;
        if (f->body) {
            if (f->body->next || scoped) {
                lnAcceptIndented(f->body);
                lw.newline();
            } else {
                auto baseIndent = lw.increaseIndent(1);
                accept(f->body);
                lw.decreaseIndent(1, baseIndent);
            }
        }
        if (scoped)
            --expressionDepth;
        out(f->rbraceToken);
        return false;
    }

    // IdentifierReference[?Yield]
    accept(property->name);
    bool useInitializer = false;
    const bool bindingIdentifierExist = !property->bindingIdentifier.isEmpty();
    if (property->colonToken.isValid()) {
        // PropertyName[?Yield] : AssignmentExpression[In, ?Yield]
        out(":");
        lw.lineWriter.ensureSpace();
        useInitializer = true;
        if (bindingIdentifierExist)
            out(property->bindingIdentifier);
        if (property->bindingTarget)
            accept(property->bindingTarget);
    }

    if (property->initializer) {
        // CoverInitializedName[?Yield]
        if (bindingIdentifierExist) {
            lw.lineWriter.ensureSpace();
            out("=");
            lw.lineWriter.ensureSpace();
            useInitializer = true;
        }
        if (useInitializer)
            accept(property->initializer);
    }
    return false;
}

bool ScriptFormatter::visit(NestedExpression *ast)
{
    out(ast->lparenToken);
    int baseIndent = lw.increaseIndent(1);
    accept(ast->expression);
    lw.decreaseIndent(1, baseIndent);
    out(ast->rparenToken);
    return false;
}

bool ScriptFormatter::visit(IdentifierPropertyName *ast)
{
    out(ast->id.toString());
    return true;
}
bool ScriptFormatter::visit(StringLiteralPropertyName *ast)
{
    out(ast->propertyNameToken);
    return true;
}
bool ScriptFormatter::visit(NumericLiteralPropertyName *ast)
{
    out(QString::number(ast->id));
    return true;
}

bool ScriptFormatter::visit(TemplateLiteral *ast)
{
    // correctly handle multiline literals
    if (ast->literalToken.length != 0) {
        QStringView str = m_script->loc2Str(ast->literalToken);
        if (lw.indentNextlines && str.contains(QLatin1Char('\n'))) {
            out(str.mid(0, 1));
            lw.indentNextlines = false;
            out(str.mid(1));
            lw.indentNextlines = true;
        } else {
            out(str);
        }
    }
    accept(ast->expression);
    return true;
}

bool ScriptFormatter::visit(ArrayMemberExpression *ast)
{
    accept(ast->base);
    out(ast->optionalToken);
    out(ast->lbracketToken);
    int indent = lw.increaseIndent(1);
    accept(ast->expression);
    lw.decreaseIndent(1, indent);
    out(ast->rbracketToken);
    return false;
}

bool ScriptFormatter::visit(FieldMemberExpression *ast)
{
    accept(ast->base);
    out(ast->dotToken);
    out(ast->identifierToken);
    return false;
}

bool ScriptFormatter::visit(NewMemberExpression *ast)
{
    out("new"); // ast->newToken
    lw.lineWriter.ensureSpace();
    accept(ast->base);
    out(ast->lparenToken);
    accept(ast->arguments);
    out(ast->rparenToken);
    return false;
}

bool ScriptFormatter::visit(NewExpression *ast)
{
    out("new"); // ast->newToken
    lw.lineWriter.ensureSpace();
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(CallExpression *ast)
{
    accept(ast->base);
    out(ast->optionalToken);
    out(ast->lparenToken);
    accept(ast->arguments);
    out(ast->rparenToken);
    return false;
}

bool ScriptFormatter::visit(PostIncrementExpression *ast)
{
    accept(ast->base);
    out(ast->incrementToken);
    return false;
}

bool ScriptFormatter::visit(PostDecrementExpression *ast)
{
    accept(ast->base);
    out(ast->decrementToken);
    return false;
}

bool ScriptFormatter::visit(PreIncrementExpression *ast)
{
    out(ast->incrementToken);
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(PreDecrementExpression *ast)
{
    out(ast->decrementToken);
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(DeleteExpression *ast)
{
    out("delete"); // ast->deleteToken
    lw.lineWriter.ensureSpace();
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(VoidExpression *ast)
{
    out("void"); // ast->voidToken
    lw.lineWriter.ensureSpace();
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(TypeOfExpression *ast)
{
    out("typeof"); // ast->typeofToken
    lw.lineWriter.ensureSpace();
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(UnaryPlusExpression *ast)
{
    out(ast->plusToken);
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(UnaryMinusExpression *ast)
{
    out(ast->minusToken);
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(TildeExpression *ast)
{
    out(ast->tildeToken);
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(NotExpression *ast)
{
    out(ast->notToken);
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(BinaryExpression *ast)
{
    accept(ast->left);
    lw.lineWriter.ensureSpace();
    out(ast->operatorToken);
    lw.lineWriter.ensureSpace();
    accept(ast->right);
    return false;
}

bool ScriptFormatter::visit(ConditionalExpression *ast)
{
    accept(ast->expression);
    lw.lineWriter.ensureSpace();
    out("?"); // ast->questionToken
    lw.lineWriter.ensureSpace();
    accept(ast->ok);
    lw.lineWriter.ensureSpace();
    out(":"); // ast->colonToken
    lw.lineWriter.ensureSpace();
    accept(ast->ko);
    return false;
}

bool ScriptFormatter::visit(Block *ast)
{
    // write comments manually because we need to indent before writing a potential post comment
    const CommentedElement *c =
            comments->commentForNode(ast, CommentAnchor::from(ast->lbraceToken));
    if (c)
        c->writePre(lw);
    out(ast->lbraceToken);
    const int indent = lw.increaseIndent();
    if (c)
        c->writePost(lw);

    if (ast->statements) {
        ++expressionDepth;
        lw.lineWriter.ensureNewline();
        accept(ast->statements);
        lw.lineWriter.ensureNewline();
        --expressionDepth;
    }

    // write comments manually because we need to write a potential pre-comment before decreasing
    // the indentation
    c = comments->commentForNode(ast, CommentAnchor::from(ast->rbraceToken));
    if (c)
        c->writePre(lw);
    lw.decreaseIndent(1, indent);
    out(ast->rbraceToken);
    if (c)
        c->writePost(lw);
    return false;
}

bool ScriptFormatter::visit(VariableStatement *ast)
{
    out(ast->declarationKindToken);
    lw.lineWriter.ensureSpace();
    accept(ast->declarations);
    if (addSemicolons())
        writeOutSemicolon(ast);
    return false;
}

bool ScriptFormatter::visit(PatternElement *ast)
{
    switch (ast->type) {
    case PatternElement::Literal:
    case PatternElement::Method:
    case PatternElement::Binding:
        break;
    case PatternElement::Getter:
        out("get");
        lw.lineWriter.ensureSpace();
        break;
    case PatternElement::Setter:
        out("set");
        lw.lineWriter.ensureSpace();
        break;
    case PatternElement::SpreadElement:
        out("...");
        break;
    }

    accept(ast->bindingTarget);
    if (!ast->destructuringPattern())
        out(ast->identifierToken);
    if (ast->initializer) {
        if (ast->isVariableDeclaration() || ast->type == AST::PatternElement::Binding) {
            lw.lineWriter.ensureSpace();
            out("=");
            lw.lineWriter.ensureSpace();
        }
        accept(ast->initializer);
    }
    accept(ast->typeAnnotation);
    return false;
}

bool ScriptFormatter::visit(TypeAnnotation *ast)
{
    out(ast->colonToken);
    lw.lineWriter.ensureSpace();
    accept(ast->type);
    return false;
}

bool ScriptFormatter::visit(Type *ast)
{
    accept(ast->typeId);
    if (ast->typeArgument) {
        outWithComments(ast->lAngleBracketToken, ast);
        accept(ast->typeArgument);
        outWithComments(ast->rAngleBracketToken, ast);
    }
    return false;
}

bool ScriptFormatter::visit(UiQualifiedId *ast)
{
    for (UiQualifiedId *it = ast; it; it = it->next) {
        outWithComments(it->dotToken, it);
        outWithComments(it->identifierToken, it);
    }
    return false;
}

bool ScriptFormatter::visit(EmptyStatement *)
{
    lw.lineWriter.ensureSemicolon();
    return false;
}

bool ScriptFormatter::visit(IfStatement *ast)
{
    out(ast->ifToken);
    lw.lineWriter.ensureSpace();
    out(ast->lparenToken);
    preVisit(ast->expression);
    ast->expression->accept0(this);
    out(ast->rparenToken);
    postVisit(ast->expression);
    acceptBlockOrIndented(ast->ok, ast->ko);
    if (ast->ko) {
        out(ast->elseToken);
        if (cast<Block *>(ast->ko) || cast<IfStatement *>(ast->ko)) {
            lw.lineWriter.ensureSpace();
            accept(ast->ko);
        } else {
            lnAcceptIndented(ast->ko);
        }
    }
    return false;
}

bool ScriptFormatter::visit(DoWhileStatement *ast)
{
    out(ast->doToken);
    acceptBlockOrIndented(ast->statement, true);
    out(ast->whileToken);
    lw.lineWriter.ensureSpace();
    outWithComments(ast->lparenToken, ast);
    accept(ast->expression);
    outWithComments(ast->rparenToken, ast);
    return false;
}

bool ScriptFormatter::visit(WhileStatement *ast)
{
    out(ast->whileToken);
    lw.lineWriter.ensureSpace();
    outWithComments(ast->lparenToken, ast);
    accept(ast->expression);
    outWithComments(ast->rparenToken, ast);
    acceptBlockOrIndented(ast->statement);
    return false;
}

bool ScriptFormatter::visit(ForStatement *ast)
{
    out(ast->forToken);
    lw.lineWriter.ensureSpace();
    outWithComments(ast->lparenToken, ast);
    if (ast->initialiser) {
        accept(ast->initialiser);
    } else if (ast->declarations) {
        if (auto pe = ast->declarations->declaration) {
            out(pe->declarationKindToken);
            lw.lineWriter.ensureSpace();
        }
        bool first = true;
        for (VariableDeclarationList *it = ast->declarations; it; it = it->next) {
            if (!std::exchange(first, false)) {
                out(",");
                lw.lineWriter.ensureSpace();
            }
            accept(it->declaration);
        }
    }
    // We don't use writeOutSemicolon() here because we need a semicolon unconditionally.
    // Repeats for the second semicolon token below.
    out(u";"); // ast->firstSemicolonToken
    lw.lineWriter.ensureSpace();
    accept(ast->condition);
    out(u";"); // ast->secondSemicolonToken
    lw.lineWriter.ensureSpace();
    accept(ast->expression);
    outWithComments(ast->rparenToken, ast);
    acceptBlockOrIndented(ast->statement);
    return false;
}

bool ScriptFormatter::visit(ForEachStatement *ast)
{
    out(ast->forToken);
    lw.lineWriter.ensureSpace();
    out(ast->lparenToken);
    if (auto pe = AST::cast<PatternElement *>(ast->lhs)) {
        out(pe->declarationKindToken);
        lw.lineWriter.ensureSpace();
    }
    accept(ast->lhs);
    lw.lineWriter.ensureSpace();
    out(ast->inOfToken);
    lw.lineWriter.ensureSpace();
    accept(ast->expression);
    out(ast->rparenToken);
    acceptBlockOrIndented(ast->statement);
    return false;
}

bool ScriptFormatter::visit(ContinueStatement *ast)
{
    out(ast->continueToken);
    if (!ast->label.isNull()) {
        lw.lineWriter.ensureSpace();
        out(ast->identifierToken);
    }
    if (addSemicolons())
        writeOutSemicolon(ast);
    return false;
}

bool ScriptFormatter::visit(BreakStatement *ast)
{
    out(ast->breakToken);
    if (!ast->label.isNull()) {
        lw.lineWriter.ensureSpace();
        out(ast->identifierToken);
    }
    if (addSemicolons())
        writeOutSemicolon(ast);
    return false;
}

bool ScriptFormatter::visit(ReturnStatement *ast)
{
    out(ast->returnToken);
    if (ast->expression) {
        if (ast->returnToken.length != 0)
            lw.lineWriter.ensureSpace();
        accept(ast->expression);
    }
    if (ast->returnToken.length > 0 && addSemicolons())
        writeOutSemicolon(ast);
    return false;
}

bool ScriptFormatter::visit(YieldExpression *ast)
{
    out(ast->yieldToken);
    if (ast->isYieldStar)
        out("*");
    if (ast->expression) {
        if (ast->yieldToken.isValid())
            lw.lineWriter.ensureSpace();;
        accept(ast->expression);
    }
    return false;
}

bool ScriptFormatter::visit(ThrowStatement *ast)
{
    out(ast->throwToken);
    if (ast->expression) {
        lw.lineWriter.ensureSpace();
        accept(ast->expression);
    }
    if (addSemicolons())
        writeOutSemicolon(ast);
    return false;
}

bool ScriptFormatter::visit(WithStatement *ast)
{
    out(ast->withToken);
    lw.lineWriter.ensureSpace();
    out(ast->lparenToken);
    accept(ast->expression);
    out(ast->rparenToken);
    acceptBlockOrIndented(ast->statement);
    return false;
}

bool ScriptFormatter::visit(SwitchStatement *ast)
{
    out(ast->switchToken);
    lw.lineWriter.ensureSpace();
    out(ast->lparenToken);
    accept(ast->expression);
    out(ast->rparenToken);
    lw.lineWriter.ensureSpace();
    accept(ast->block);
    return false;
}

bool ScriptFormatter::visit(CaseBlock *ast)
{
    out(ast->lbraceToken);
    ++expressionDepth;
    newLine();
    accept(ast->clauses);
    if (ast->clauses && ast->defaultClause)
        newLine();
    accept(ast->defaultClause);
    if (ast->moreClauses)
        newLine();
    accept(ast->moreClauses);
    newLine();
    --expressionDepth;
    out(ast->rbraceToken);
    return false;
}

bool ScriptFormatter::visit(CaseClause *ast)
{
    out("case"); // ast->caseToken
    lw.lineWriter.ensureSpace();
    accept(ast->expression);
    outWithComments(ast->colonToken, ast);
    if (ast->statements)
        lnAcceptIndented(ast->statements);
    return false;
}

bool ScriptFormatter::visit(DefaultClause *ast)
{
    out(ast->defaultToken);
    out(ast->colonToken);
    lnAcceptIndented(ast->statements);
    return false;
}

bool ScriptFormatter::visit(LabelledStatement *ast)
{
    out(ast->identifierToken);
    out(":"); // ast->colonToken
    lw.lineWriter.ensureSpace();
    accept(ast->statement);
    return false;
}

bool ScriptFormatter::visit(TryStatement *ast)
{
    out("try"); // ast->tryToken
    lw.lineWriter.ensureSpace();
    accept(ast->statement);
    if (ast->catchExpression) {
        lw.lineWriter.ensureSpace();
        accept(ast->catchExpression);
    }
    if (ast->finallyExpression) {
        lw.lineWriter.ensureSpace();
        accept(ast->finallyExpression);
    }
    return false;
}

bool ScriptFormatter::visit(Catch *ast)
{
    out(ast->catchToken);
    lw.lineWriter.ensureSpace();
    out(ast->lparenToken);
    out(ast->identifierToken);
    out(")"); // ast->rparenToken
    lw.lineWriter.ensureSpace();
    accept(ast->statement);
    return false;
}

bool ScriptFormatter::visit(Finally *ast)
{
    out("finally"); // ast->finallyToken
    lw.lineWriter.ensureSpace();
    accept(ast->statement);
    return false;
}

bool ScriptFormatter::visit(FunctionDeclaration *ast)
{
    return ScriptFormatter::visit(static_cast<FunctionExpression *>(ast));
}

bool ScriptFormatter::visit(FunctionExpression *ast)
{
    if (!ast->isArrowFunction) {
        if (ast->isGenerator) {
            out("function*");
            lw.lineWriter.ensureSpace();
        } else {
            out("function");
            lw.lineWriter.ensureSpace();
        }
        outWithComments(ast->identifierToken, ast);
    }

    const bool removeParentheses = ast->isArrowFunction && ast->formals && !ast->formals->next
            && (ast->formals->element && !ast->formals->element->bindingTarget);

    // note: qmlformat removes the parentheses for "(x) => x". In that case, we still need
    // to print potential comments attached to `(` or `)` via `OnlyComments` option.
    outWithComments(ast->lparenToken, ast, removeParentheses ? OnlyComments : NoSpace);
    int baseIndent = lw.increaseIndent(1);
    accept(ast->formals);
    lw.decreaseIndent(1, baseIndent);
    outWithComments(ast->rparenToken, ast, removeParentheses ? OnlyComments : NoSpace);
    accept(ast->typeAnnotation);
    lw.lineWriter.ensureSpace();
    if (ast->isArrowFunction) {
        out("=>");
        lw.lineWriter.ensureSpace();
    }
    outWithComments(ast->lbraceToken, ast);
    if (ast->lbraceToken.length != 0)
        ++expressionDepth;
    if (ast->body) {
        if (ast->body->next || ast->lbraceToken.length != 0) {
            lnAcceptIndented(ast->body);
            newLine();
        } else {
            // print a single statement in one line. E.g. x => x * 2
            baseIndent = lw.increaseIndent(1);
            accept(ast->body);
            lw.decreaseIndent(1, baseIndent);
        }
    }
    if (ast->lbraceToken.length != 0)
        --expressionDepth;
    outWithComments(ast->rbraceToken, ast);
    return false;
}

bool ScriptFormatter::visit(Elision *ast)
{
    for (Elision *it = ast; it; it = it->next) {
        if (ast->commaToken.isValid()) {
            outWithComments(ast->commaToken, ast);
            lw.lineWriter.ensureSpace();
        }
    }
    return false;
}

bool ScriptFormatter::visit(ArgumentList *ast)
{
    for (ArgumentList *it = ast; it; it = it->next) {
        if (it->isSpreadElement)
            out("...");
        accept(it->expression);
        if (it->next) {
            out(","); // it->commaToken
            lw.lineWriter.ensureSpace();
        }
    }
    return false;
}

bool ScriptFormatter::visit(StatementList *ast)
{
    ++expressionDepth;
    for (StatementList *it = ast; it; it = it->next) {
        // ### work around parser bug: skip empty statements with wrong tokens
        if (EmptyStatement *emptyStatement = cast<EmptyStatement *>(it->statement)) {
            if (m_script->loc2Str(emptyStatement->semicolonToken) != QLatin1String(";"))
                continue;
        }

        accept(it->statement);
        if (it->next) {
            // There might be a post-comment attached to the current
            // statement or a pre-comment attached to the next
            // statmente or both.
            // If any of those are present they will take care of
            // handling the spacing between the statements so we
            // don't need to push any newline.
            auto *commentForCurrentStatement =
                    comments->commentForNode(it->statement, CommentAnchor{});
            auto *commentForNextStatement =
                    comments->commentForNode(it->next->statement, CommentAnchor{});

            if (
                (commentForCurrentStatement && !commentForCurrentStatement->postComments().empty())
                || (commentForNextStatement && !commentForNextStatement->preComments().empty())
            ) continue;

            quint32 lineDelta = it->next->firstSourceLocation().startLine
                    - it->statement->lastSourceLocation().startLine;
            lineDelta = std::clamp(lineDelta, quint32{ 1 }, quint32{ 2 });

            newLine(lineDelta);
        }
    }
    --expressionDepth;
    return false;
}

bool ScriptFormatter::visit(VariableDeclarationList *ast)
{
    for (VariableDeclarationList *it = ast; it; it = it->next) {
        accept(it->declaration);
        if (it->next) {
            out(","); // it->commaToken
            lw.lineWriter.ensureSpace();
        }
    }
    return false;
}

bool ScriptFormatter::visit(CaseClauses *ast)
{
    for (CaseClauses *it = ast; it; it = it->next) {
        accept(it->clause);
        if (it->next)
            newLine();
    }
    return false;
}

bool ScriptFormatter::visit(FormalParameterList *ast)
{
    for (FormalParameterList *it = ast; it; it = it->next) {
        accept(it->element);
        outWithComments(it->commaToken, it, SpaceBeforePostComment);
    }
    return false;
}

// to check
bool ScriptFormatter::visit(SuperLiteral *)
{
    out("super");
    return true;
}
bool ScriptFormatter::visit(ComputedPropertyName *)
{
    out("[");
    return true;
}
bool ScriptFormatter::visit(CommaExpression *el)
{
    accept(el->left);
    out(",");
    lw.lineWriter.ensureSpace();
    accept(el->right);
    return false;
}
bool ScriptFormatter::visit(ExpressionStatement *el)
{
    if (addSemicolons())
        postOps[el->expression].append([this, el]() { writeOutSemicolon(el); });
    return true;
}

// Return false because we want to omit default function calls in accept0 implementation.
bool ScriptFormatter::visit(ClassDeclaration *ast)
{
    out(ast->classToken);
    lw.lineWriter.ensureSpace();
    outWithComments(ast->identifierToken, ast);
    if (ast->heritage) {
        lw.lineWriter.ensureSpace();
        out("extends");
        lw.lineWriter.ensureSpace();
        accept(ast->heritage);
    }
    lw.lineWriter.ensureSpace();
    outWithComments(ast->lbraceToken, ast);
    int baseIndent = lw.increaseIndent();
    for (ClassElementList *it = ast->elements; it; it = it->next) {
        lw.newline();
        if (it->isStatic) {
            out("static");
            lw.lineWriter.ensureSpace();
        }
        accept(it->property);
        lw.newline();
    }
    lw.decreaseIndent(1, baseIndent);
    outWithComments(ast->rbraceToken, ast);
    return false;
}

bool ScriptFormatter::visit(AST::ImportDeclaration *ast)
{
    out(ast->importToken);
    lw.ensureSpace();
    if (!ast->moduleSpecifier.isNull()) {
        out(ast->moduleSpecifierToken);
    }
    return true;
}

bool ScriptFormatter::visit(AST::ImportSpecifier *ast)
{
    if (!ast->identifier.isNull()) {
        out(ast->identifierToken);
        lw.ensureSpace();
        out("as");
        lw.ensureSpace();
    }
    out(ast->importedBindingToken);
    return true;
}

bool ScriptFormatter::visit(AST::NameSpaceImport *ast)
{
    out(ast->starToken);
    lw.ensureSpace();
    out("as");
    lw.ensureSpace();
    out(ast->importedBindingToken);
    return true;
}

bool ScriptFormatter::visit(AST::ImportsList *ast)
{
    for (ImportsList *it = ast; it; it = it->next) {
        accept(it->importSpecifier);
        if (it->next) {
            out(",");
            lw.ensureSpace();
        }
    }
    return false;
}
bool ScriptFormatter::visit(AST::NamedImports *ast)
{
    out(ast->leftBraceToken);
    if (ast->importsList) {
        lw.ensureSpace();
    }
    return true;
}

bool ScriptFormatter::visit(AST::ImportClause *ast)
{
    if (!ast->importedDefaultBinding.isNull()) {
        out(ast->importedDefaultBindingToken);
        if (ast->nameSpaceImport || ast->namedImports) {
            out(",");
            lw.ensureSpace();
        }
    }
    return true;
}

bool ScriptFormatter::visit(AST::ExportDeclaration *ast)
{
    out(ast->exportToken);
    lw.ensureSpace();
    if (ast->exportDefault) {
        out("default");
        lw.ensureSpace();
    }
    if (ast->exportsAll()) {
        out("*");
    }
    return true;
}

bool ScriptFormatter::visit(AST::ExportClause *ast)
{
    out(ast->leftBraceToken);
    if (ast->exportsList) {
        lw.ensureSpace();
    }
    return true;
}

bool ScriptFormatter::visit(AST::ExportSpecifier *ast)
{
    out(ast->identifier);
    if (ast->exportedIdentifierToken.isValid()) {
        lw.ensureSpace();
        out("as");
        lw.ensureSpace();
        out(ast->exportedIdentifier);
    }
    return true;
}

bool ScriptFormatter::visit(AST::ExportsList *ast)
{
    for (ExportsList *it = ast; it; it = it->next) {
        accept(it->exportSpecifier);
        if (it->next) {
            out(",");
            lw.ensureSpace();
        }
    }
    return false;
}

bool ScriptFormatter::visit(AST::FromClause *ast)
{
    lw.ensureSpace();
    out(ast->fromToken);
    lw.ensureSpace();
    out(ast->moduleSpecifierToken);
    return true;
}

void ScriptFormatter::endVisit(ComputedPropertyName *)
{
    out("]");
}

void ScriptFormatter::endVisit(AST::ExportDeclaration *ast)
{
    // add a semicolon at the end of the following expressions
    // export * FromClause
    // export ExportClause FromClause ;
    if (ast->fromClause) {
        writeOutSemicolon(ast);
    }

    // add a semicolon at the end of the following expressions
    // export ExportClause ;
    if (ast->exportClause && !ast->fromClause) {
        writeOutSemicolon(ast);
    }

    // add a semicolon at the end of the following expressions
    // export default [lookahead ∉ { function, class }] AssignmentExpression;
    if (ast->exportDefault && ast->variableStatementOrDeclaration) {
        // lookahead ∉ { function, class }
        if (!(ast->variableStatementOrDeclaration->kind == Node::Kind_FunctionDeclaration
              || ast->variableStatementOrDeclaration->kind == Node::Kind_ClassDeclaration)) {
            writeOutSemicolon(ast);
        }
        // ArrowFunction in QQmlJS::AST is handled with the help of FunctionDeclaration
        // and not as part of AssignmentExpression (as per ECMA
        // https://262.ecma-international.org/7.0/#prod-AssignmentExpression)
        if (ast->variableStatementOrDeclaration->kind == Node::Kind_FunctionDeclaration
            && static_cast<AST::FunctionDeclaration *>(ast->variableStatementOrDeclaration)
                       ->isArrowFunction) {
            writeOutSemicolon(ast);
        }
    }
}

void ScriptFormatter::endVisit(AST::ExportClause *ast)
{
    if (ast->exportsList) {
        lw.ensureSpace();
    }
    out(ast->rightBraceToken);
}

void ScriptFormatter::endVisit(AST::NamedImports *ast)
{
    if (ast->importsList) {
        lw.ensureSpace();
    }
    out(ast->rightBraceToken);
}

void ScriptFormatter::endVisit(AST::ImportDeclaration *id)
{
    writeOutSemicolon(id);
}

void ScriptFormatter::throwRecursionDepthError()
{
    out("/* ERROR: Hit recursion limit  ScriptFormatter::visiting AST, rewrite failed */");
}

// This is a set of characters that are not allowed to be at the beginning of a line
// after a semicolon for ASI.

static constexpr QStringView restrictedChars = u"([/+-";

// Given an existing semicolon, can we safely remove it without changing behavior
bool ScriptFormatter::canRemoveSemicolon(AST::Node *node)
{
    const auto canRelyOnASI = [this](Node *node) {
        auto nodeLoc = node->lastSourceLocation().offset + 1;
        auto code = m_script->engine()->code();
        // Bounds check for nodeLoc
        if (qsizetype(nodeLoc) >= code.size())
            return false;
        auto startIt = code.begin() + nodeLoc;
        auto endIt = std::find_first_of(startIt, code.end(), restrictedChars.begin(),
                                        restrictedChars.end());
        // No restricted character found, then it is safe to remove the semicolon
        if (endIt == code.end())
            return true;

        // Check if there is at least one character between nodeLoc and the found character
        // that are neither space chars nor semicolons.
        bool hasOtherChars =
                std::any_of(startIt, endIt, [](QChar ch) { return !(ch.isSpace() || ch == u';'); });

        if (hasOtherChars)
            return true;

        // Check if there is no linebreak between nodeLoc and the found character
        return std::none_of(startIt, endIt, [](QChar c) { return c == u'\n'; });
    };

    // Check if the node is a statement that requires a semicolon to avoid ASI issues
    switch (node->kind) {
    case AST::Node::Kind_ExpressionStatement:
        return canRelyOnASI(cast<ExpressionStatement *>(node));
    case AST::Node::Kind_VariableStatement:
        return canRelyOnASI(cast<VariableStatement *>(node));
    case AST::Node::Kind_EmptyStatement:
        return false;
    case AST::Node::Kind_ContinueStatement:
    case AST::Node::Kind_BreakStatement:
    case AST::Node::Kind_ReturnStatement:
    case AST::Node::Kind_ThrowStatement:
    case AST::Node::Kind_ExportDeclaration:
    case AST::Node::Kind_ImportDeclaration:
    case AST::Node::Kind_FromClause:
    case AST::Node::Kind_ExportClause:
    default:
        return true;
    }
}

OutWriter &ScriptFormatter::writeOutSemicolon(AST::Node *node)
{
    if (!node)
        return lw;
    switch (lw.lineWriter.options().semicolonRule) {
    case LineWriterOptions::SemicolonRule::Essential:
        if (!canRemoveSemicolon(node))
            out(u";");
        lw.lineWriter.ensureNewline();
        return lw;
    case LineWriterOptions::SemicolonRule::Always:
        out(u";");
        return lw;
    default:
        Q_UNREACHABLE_RETURN(lw);
    }
}

void reformatAst(OutWriter &lw, const QQmlJS::Dom::ScriptExpression *const script)
{
    if (script)
        ScriptFormatter formatter(lw, script);
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
