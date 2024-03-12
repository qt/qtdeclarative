// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmldomreformatter_p.h"
#include "qqmldomcomments_p.h"

#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>

#include <QString>

#include <limits>

QT_BEGIN_NAMESPACE
namespace QQmlJS {
namespace Dom {

using namespace AST;

bool ScriptFormatter::preVisit(Node *n)
{
    if (CommentedElement *c = comments->commentForNode(n)) {
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
    if (cast<Block *>(ast)) {
        out(" ");
        accept(ast);
        if (finishWithSpaceOrNewline)
            out(" ");
        return true;
    } else {
        if (finishWithSpaceOrNewline)
            postOps[ast].append([this]() { this->newLine(); });
        lnAcceptIndented(ast);
        return false;
    }
}

void ScriptFormatter::outputScope(VariableScope scope)
{
    switch (scope) {
    case VariableScope::Const:
        out("const ");
        break;
    case VariableScope::Let:
        out("let ");
        break;
    case VariableScope::Var:
        out("var ");
        break;
    default:
        break;
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
    QStringView str = loc2Str(ast->literalToken);
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
    out(ast->lbracketToken);
    int baseIndent = lw.increaseIndent(1);
    if (ast->elements) {
        accept(ast->elements);
        out(ast->commaToken);
        auto lastElement = lastListElement(ast->elements);
        if (lastElement->element && cast<ObjectPattern *>(lastElement->element->initializer)) {
            newLine();
        }
    } else {
        out(ast->commaToken);
    }
    lw.decreaseIndent(1, baseIndent);
    out(ast->rbracketToken);
    return false;
}

bool ScriptFormatter::visit(ObjectPattern *ast)
{
    out(ast->lbraceToken);
    ++expressionDepth;
    if (ast->properties) {
        lnAcceptIndented(ast->properties);
        newLine();
    }
    --expressionDepth;
    out(ast->rbraceToken);
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
        if (it->elision && it->element)
            out(", ");
        if (it->element)
            accept(it->element);
        if (it->next) {
            out(", ");
            if (isObjectInitializer)
                newLine();
        }
    }
    return false;
}

bool ScriptFormatter::visit(PatternPropertyList *ast)
{
    for (PatternPropertyList *it = ast; it; it = it->next) {
        PatternProperty *assignment = AST::cast<PatternProperty *>(it->property);
        if (assignment) {
            preVisit(assignment);
            accept(assignment->name);
            bool useInitializer = false;
            const bool bindingIdentifierExist = !assignment->bindingIdentifier.isEmpty();
            if (assignment->colonToken.length > 0) {
                out(": ");
                useInitializer = true;
                if (bindingIdentifierExist)
                    out(assignment->bindingIdentifier);
                if (assignment->bindingTarget)
                    accept(assignment->bindingTarget);
            }

            if (assignment->initializer) {
                if (bindingIdentifierExist) {
                    out(" = ");
                    useInitializer = true;
                }
                if (useInitializer)
                    accept(assignment->initializer);
            }

            if (it->next) {
                out(",");
                newLine();
            }
            postVisit(assignment);
            continue;
        }

        PatternPropertyList *getterSetter = AST::cast<PatternPropertyList *>(it->next);
        if (getterSetter && getterSetter->property) {
            switch (getterSetter->property->type) {
            case PatternElement::Getter:
                out("get");
                break;
            case PatternElement::Setter:
                out("set");
                break;
            default:
                break;
            }

            accept(getterSetter->property->name);
            out("(");
            // accept(getterSetter->formals);  // TODO
            out(")");
            out(" {");
            // accept(getterSetter->functionBody);  // TODO
            out(" }");
        }
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
        QStringView str = loc2Str(ast->literalToken);
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
    out("new "); // ast->newToken
    accept(ast->base);
    out(ast->lparenToken);
    accept(ast->arguments);
    out(ast->rparenToken);
    return false;
}

bool ScriptFormatter::visit(NewExpression *ast)
{
    out("new "); // ast->newToken
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(CallExpression *ast)
{
    accept(ast->base);
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
    out("delete "); // ast->deleteToken
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(VoidExpression *ast)
{
    out("void "); // ast->voidToken
    accept(ast->expression);
    return false;
}

bool ScriptFormatter::visit(TypeOfExpression *ast)
{
    out("typeof "); // ast->typeofToken
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
    out(" ");
    out(ast->operatorToken);
    out(" ");
    accept(ast->right);
    return false;
}

bool ScriptFormatter::visit(ConditionalExpression *ast)
{
    accept(ast->expression);
    out(" ? "); // ast->questionToken
    accept(ast->ok);
    out(" : "); // ast->colonToken
    accept(ast->ko);
    return false;
}

bool ScriptFormatter::visit(Block *ast)
{
    out(ast->lbraceToken);
    if (ast->statements) {
        ++expressionDepth;
        lnAcceptIndented(ast->statements);
        newLine();
        --expressionDepth;
    }
    out(ast->rbraceToken);
    return false;
}

bool ScriptFormatter::visit(VariableStatement *ast)
{
    out(ast->declarationKindToken);
    out(" ");
    accept(ast->declarations);
    if (addSemicolons())
        out(";");
    return false;
}

bool ScriptFormatter::visit(PatternElement *ast)
{
    if (ast->isForDeclaration) {
        outputScope(ast->scope);
    }
    switch (ast->type) {
    case PatternElement::Literal:
    case PatternElement::Method:
    case PatternElement::Binding:
        break;
    case PatternElement::Getter:
        out("get ");
        break;
    case PatternElement::Setter:
        out("set ");
        break;
    case PatternElement::SpreadElement:
        out("...");
        break;
    }

    accept(ast->bindingTarget);
    if (!ast->destructuringPattern())
        out(ast->identifierToken);
    if (ast->initializer) {
        if (ast->isVariableDeclaration() || ast->type == AST::PatternElement::Binding)
            out(" = ");
        accept(ast->initializer);
    }
    return false;
}

bool ScriptFormatter::visit(EmptyStatement *ast)
{
    out(ast->semicolonToken);
    return false;
}

bool ScriptFormatter::visit(IfStatement *ast)
{
    out(ast->ifToken);
    out(" ");
    out(ast->lparenToken);
    preVisit(ast->expression);
    ast->expression->accept0(this);
    out(ast->rparenToken);
    postVisit(ast->expression);
    acceptBlockOrIndented(ast->ok, ast->ko);
    if (ast->ko) {
        out(ast->elseToken);
        if (cast<Block *>(ast->ko) || cast<IfStatement *>(ast->ko)) {
            out(" ");
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
    out(" ");
    out(ast->lparenToken);
    accept(ast->expression);
    out(ast->rparenToken);
    return false;
}

bool ScriptFormatter::visit(WhileStatement *ast)
{
    out(ast->whileToken);
    out(" ");
    out(ast->lparenToken);
    accept(ast->expression);
    out(ast->rparenToken);
    acceptBlockOrIndented(ast->statement);
    return false;
}

bool ScriptFormatter::visit(ForStatement *ast)
{
    out(ast->forToken);
    out(" ");
    out(ast->lparenToken);
    if (ast->initialiser) {
        accept(ast->initialiser);
    } else if (ast->declarations) {
        outputScope(ast->declarations->declaration->scope);
        accept(ast->declarations);
    }
    out("; "); // ast->firstSemicolonToken
    accept(ast->condition);
    out("; "); // ast->secondSemicolonToken
    accept(ast->expression);
    out(ast->rparenToken);
    acceptBlockOrIndented(ast->statement);
    return false;
}

bool ScriptFormatter::visit(ForEachStatement *ast)
{
    out(ast->forToken);
    out(" ");
    out(ast->lparenToken);
    accept(ast->lhs);
    out(" ");
    out(ast->inOfToken);
    out(" ");
    accept(ast->expression);
    out(ast->rparenToken);
    acceptBlockOrIndented(ast->statement);
    return false;
}

bool ScriptFormatter::visit(ContinueStatement *ast)
{
    out(ast->continueToken);
    if (!ast->label.isNull()) {
        out(" ");
        out(ast->identifierToken);
    }
    if (addSemicolons())
        out(";");
    return false;
}

bool ScriptFormatter::visit(BreakStatement *ast)
{
    out(ast->breakToken);
    if (!ast->label.isNull()) {
        out(" ");
        out(ast->identifierToken);
    }
    if (addSemicolons())
        out(";");
    return false;
}

bool ScriptFormatter::visit(ReturnStatement *ast)
{
    out(ast->returnToken);
    if (ast->expression) {
        if (ast->returnToken.length != 0)
            out(" ");
        accept(ast->expression);
    }
    if (ast->returnToken.length > 0 && addSemicolons())
        out(";");
    return false;
}

bool ScriptFormatter::visit(ThrowStatement *ast)
{
    out(ast->throwToken);
    if (ast->expression) {
        out(" ");
        accept(ast->expression);
    }
    if (addSemicolons())
        out(";");
    return false;
}

bool ScriptFormatter::visit(WithStatement *ast)
{
    out(ast->withToken);
    out(" ");
    out(ast->lparenToken);
    accept(ast->expression);
    out(ast->rparenToken);
    acceptBlockOrIndented(ast->statement);
    return false;
}

bool ScriptFormatter::visit(SwitchStatement *ast)
{
    out(ast->switchToken);
    out(" ");
    out(ast->lparenToken);
    accept(ast->expression);
    out(ast->rparenToken);
    out(" ");
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
    out("case "); // ast->caseToken
    accept(ast->expression);
    out(ast->colonToken);
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
    out(": "); // ast->colonToken
    accept(ast->statement);
    return false;
}

bool ScriptFormatter::visit(TryStatement *ast)
{
    out("try "); // ast->tryToken
    accept(ast->statement);
    if (ast->catchExpression) {
        out(" ");
        accept(ast->catchExpression);
    }
    if (ast->finallyExpression) {
        out(" ");
        accept(ast->finallyExpression);
    }
    return false;
}

bool ScriptFormatter::visit(Catch *ast)
{
    out(ast->catchToken);
    out(" ");
    out(ast->lparenToken);
    out(ast->identifierToken);
    out(") "); // ast->rparenToken
    accept(ast->statement);
    return false;
}

bool ScriptFormatter::visit(Finally *ast)
{
    out("finally "); // ast->finallyToken
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
        out("function "); // ast->functionToken
        if (!ast->name.isNull())
            out(ast->identifierToken);
    }
    out(ast->lparenToken);
    const bool needParentheses = ast->formals
            && (ast->formals->next
                || (ast->formals->element && ast->formals->element->bindingTarget));
    if (ast->isArrowFunction && needParentheses)
        out("(");
    int baseIndent = lw.increaseIndent(1);
    accept(ast->formals);
    lw.decreaseIndent(1, baseIndent);
    if (ast->isArrowFunction && needParentheses)
        out(")");
    out(ast->rparenToken);
    if (ast->isArrowFunction && !ast->formals)
        out("()");
    out(" ");
    if (ast->isArrowFunction)
        out("=> ");
    out(ast->lbraceToken);
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
    out(ast->rbraceToken);
    return false;
}

bool ScriptFormatter::visit(Elision *ast)
{
    for (Elision *it = ast; it; it = it->next) {
        if (it->next)
            out(", "); // ast->commaToken
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
            out(", "); // it->commaToken
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
            if (loc2Str(emptyStatement->semicolonToken) != QLatin1String(";"))
                continue;
        }

        accept(it->statement);
        if (it->next)
            newLine();
    }
    --expressionDepth;
    return false;
}

bool ScriptFormatter::visit(VariableDeclarationList *ast)
{
    for (VariableDeclarationList *it = ast; it; it = it->next) {
        accept(it->declaration);
        if (it->next)
            out(", "); // it->commaToken
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
        // compare FormalParameterList::finish
        if (auto id = it->element->bindingIdentifier.toString(); !id.isEmpty())
            out(id);
        if (it->element->bindingTarget)
            accept(it->element->bindingTarget);
        if (it->next)
            out(", ");
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
bool ScriptFormatter::visit(Expression *el)
{
    accept(el->left);
    out(", ");
    accept(el->right);
    return false;
}
bool ScriptFormatter::visit(ExpressionStatement *el)
{
    if (addSemicolons())
        postOps[el->expression].append([this]() { out(";"); });
    return true;
}

// Return false because we want to omit default function calls in accept0 implementation.
bool ScriptFormatter::visit(ClassDeclaration *ast)
{
    preVisit(ast);
    out(ast->classToken);
    out(" ");
    out(ast->name);
    if (ast->heritage) {
        out(" extends ");
        accept(ast->heritage);
    }
    out(" {");
    int baseIndent = lw.increaseIndent();
    for (ClassElementList *it = ast->elements; it; it = it->next) {
        PatternProperty *property = it->property;
        lw.newline();
        preVisit(property);
        if (it->isStatic)
            out("static ");
        if (property->type == PatternProperty::Getter)
            out("get ");
        else if (property->type == PatternProperty::Setter)
            out("set ");
        FunctionExpression *f = AST::cast<FunctionExpression *>(property->initializer);
        const bool scoped = f->lbraceToken.length != 0;
        out(f->functionToken);
        out(f->lparenToken);
        accept(f->formals);
        out(f->rparenToken);
        out(f->lbraceToken);
        if (scoped)
            ++expressionDepth;
        if (f->body) {
            if (f->body->next || scoped) {
                lnAcceptIndented(f->body);
                lw.newline();
            } else {
                baseIndent = lw.increaseIndent(1);
                accept(f->body);
                lw.decreaseIndent(1, baseIndent);
            }
        }
        if (scoped)
            --expressionDepth;
        out(f->rbraceToken);
        lw.newline();
        postVisit(property);
    }
    lw.decreaseIndent(1, baseIndent);
    out("}");
    postVisit(ast);
    return false;
}

void ScriptFormatter::endVisit(ComputedPropertyName *)
{
    out("]");
}

void ScriptFormatter::throwRecursionDepthError()
{
    out("/* ERROR: Hit recursion limit  ScriptFormatter::visiting AST, rewrite failed */");
}

void reformatAst(OutWriter &lw, const std::shared_ptr<AstComments> &comments,
                 const std::function<QStringView(SourceLocation)> &loc2Str, AST::Node *n)
{
    if (n) {
        ScriptFormatter formatter(lw, comments, loc2Str, n);
    }
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
