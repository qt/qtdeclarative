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

class Rewriter : protected BaseVisitor
{
    OutWriter &lw;
    std::shared_ptr<AstComments> comments;
    std::function<QStringView(SourceLocation)> loc2Str;
    QHash<Node *, QList<std::function<void()>>> postOps;
    int expressionDepth = 0;

    bool addSemicolons() const { return expressionDepth > 0; }

public:
    Rewriter(OutWriter &lw, std::shared_ptr<AstComments> comments,
             std::function<QStringView(SourceLocation)> loc2Str, Node *node)
        : lw(lw), comments(comments), loc2Str(loc2Str)
    {
        accept(node);
    }

protected:
    bool preVisit(Node *n) override
    {
        if (CommentedElement *c = comments->commentForNode(n)) {
            c->writePre(lw);
            postOps[n].append([c, this]() { c->writePost(lw); });
        }
        return true;
    }
    void postVisit(Node *n) override
    {
        for (auto &op : postOps[n]) {
            op();
        }
        postOps.remove(n);
    }

    void accept(Node *node) { Node::accept(node, this); }

    void lnAcceptIndented(Node *node)
    {
        int indent = lw.increaseIndent(1);
        lw.ensureNewline();
        accept(node);
        lw.decreaseIndent(1, indent);
    }

    void out(const char *str) { lw.write(QString::fromLatin1(str)); }

    void out(QStringView str) { lw.write(str); }

    void out(const SourceLocation &loc)
    {
        if (loc.length != 0)
            out(loc2Str(loc));
    }

    void newLine() { lw.ensureNewline(); }

    bool acceptBlockOrIndented(Node *ast, bool finishWithSpaceOrNewline = false)
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    // we are not supposed to handle the ui
    bool visit(UiPragmaValueList *) override
    {
        Q_ASSERT(false);
        return false;
    }
#endif
    bool visit(UiPragma *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiEnumDeclaration *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiEnumMemberList *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiImport *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiObjectDefinition *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiObjectInitializer *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiParameterList *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiPublicMember *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiObjectBinding *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiScriptBinding *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiArrayBinding *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiHeaderItemList *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiObjectMemberList *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiArrayMemberList *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiQualifiedId *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiProgram *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiSourceElement *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiVersionSpecifier *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiInlineComponent *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiAnnotation *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiAnnotationList *) override
    {
        Q_ASSERT(false);
        return false;
    }
    bool visit(UiRequired *) override
    {
        Q_ASSERT(false);
        return false;
    }

    bool visit(ThisExpression *ast) override
    {
        out(ast->thisToken);
        return true;
    }
    bool visit(NullExpression *ast) override
    {
        out(ast->nullToken);
        return true;
    }
    bool visit(TrueLiteral *ast) override
    {
        out(ast->trueToken);
        return true;
    }
    bool visit(FalseLiteral *ast) override
    {
        out(ast->falseToken);
        return true;
    }
    bool visit(IdentifierExpression *ast) override
    {
        out(ast->identifierToken);
        return true;
    }
    bool visit(StringLiteral *ast) override
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
    bool visit(NumericLiteral *ast) override
    {
        out(ast->literalToken);
        return true;
    }
    bool visit(RegExpLiteral *ast) override
    {
        out(ast->literalToken);
        return true;
    }

    bool visit(ArrayPattern *ast) override
    {
        out(ast->lbracketToken);
        int baseIndent = lw.increaseIndent(1);
        if (ast->elements)
            accept(ast->elements);
        out(ast->commaToken);
        lw.decreaseIndent(1, baseIndent);
        out(ast->rbracketToken);
        return false;
    }

    bool visit(ObjectPattern *ast) override
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

    bool visit(PatternElementList *ast) override
    {
        for (PatternElementList *it = ast; it; it = it->next) {
            if (it->elision)
                accept(it->elision);
            if (it->elision && it->element)
                out(", ");
            if (it->element)
                accept(it->element);
            if (it->next)
                out(", ");
        }
        return false;
    }

    bool visit(PatternPropertyList *ast) override
    {
        for (PatternPropertyList *it = ast; it; it = it->next) {
            PatternProperty *assignment = AST::cast<PatternProperty *>(it->property);
            if (assignment) {
                preVisit(assignment);
                const bool isStringLike = [this](const SourceLocation &loc) {
                    const auto name = loc2Str(loc);
                    if (name.first() == name.last()) {
                        if (name.first() == QStringLiteral("\'")
                            || name.first() == QStringLiteral("\""))
                            return true;
                    }
                    return false;
                }(assignment->name->propertyNameToken);

                if (isStringLike)
                    out("\"");

                accept(assignment->name);
                if (isStringLike)
                    out("\"");

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

    bool visit(NestedExpression *ast) override
    {
        out(ast->lparenToken);
        int baseIndent = lw.increaseIndent(1);
        accept(ast->expression);
        lw.decreaseIndent(1, baseIndent);
        out(ast->rparenToken);
        return false;
    }

    bool visit(IdentifierPropertyName *ast) override
    {
        out(ast->id.toString());
        return true;
    }
    bool visit(StringLiteralPropertyName *ast) override
    {
        out(ast->id.toString());
        return true;
    }
    bool visit(NumericLiteralPropertyName *ast) override
    {
        out(QString::number(ast->id));
        return true;
    }

    bool visit(TemplateLiteral *ast) override
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

    bool visit(ArrayMemberExpression *ast) override
    {
        accept(ast->base);
        out(ast->lbracketToken);
        int indent = lw.increaseIndent(1);
        accept(ast->expression);
        lw.decreaseIndent(1, indent);
        out(ast->rbracketToken);
        return false;
    }

    bool visit(FieldMemberExpression *ast) override
    {
        accept(ast->base);
        out(ast->dotToken);
        out(ast->identifierToken);
        return false;
    }

    bool visit(NewMemberExpression *ast) override
    {
        out("new "); // ast->newToken
        accept(ast->base);
        out(ast->lparenToken);
        accept(ast->arguments);
        out(ast->rparenToken);
        return false;
    }

    bool visit(NewExpression *ast) override
    {
        out("new "); // ast->newToken
        accept(ast->expression);
        return false;
    }

    bool visit(CallExpression *ast) override
    {
        accept(ast->base);
        out(ast->lparenToken);
        int baseIndent = lw.increaseIndent(1);
        accept(ast->arguments);
        lw.decreaseIndent(1, baseIndent);
        out(ast->rparenToken);
        return false;
    }

    bool visit(PostIncrementExpression *ast) override
    {
        accept(ast->base);
        out(ast->incrementToken);
        return false;
    }

    bool visit(PostDecrementExpression *ast) override
    {
        accept(ast->base);
        out(ast->decrementToken);
        return false;
    }

    bool visit(PreIncrementExpression *ast) override
    {
        out(ast->incrementToken);
        accept(ast->expression);
        return false;
    }

    bool visit(PreDecrementExpression *ast) override
    {
        out(ast->decrementToken);
        accept(ast->expression);
        return false;
    }

    bool visit(DeleteExpression *ast) override
    {
        out("delete "); // ast->deleteToken
        accept(ast->expression);
        return false;
    }

    bool visit(VoidExpression *ast) override
    {
        out("void "); // ast->voidToken
        accept(ast->expression);
        return false;
    }

    bool visit(TypeOfExpression *ast) override
    {
        out("typeof "); // ast->typeofToken
        accept(ast->expression);
        return false;
    }

    bool visit(UnaryPlusExpression *ast) override
    {
        out(ast->plusToken);
        accept(ast->expression);
        return false;
    }

    bool visit(UnaryMinusExpression *ast) override
    {
        out(ast->minusToken);
        accept(ast->expression);
        return false;
    }

    bool visit(TildeExpression *ast) override
    {
        out(ast->tildeToken);
        accept(ast->expression);
        return false;
    }

    bool visit(NotExpression *ast) override
    {
        out(ast->notToken);
        accept(ast->expression);
        return false;
    }

    bool visit(BinaryExpression *ast) override
    {
        accept(ast->left);
        out(" ");
        out(ast->operatorToken);
        out(" ");
        accept(ast->right);
        return false;
    }

    bool visit(ConditionalExpression *ast) override
    {
        accept(ast->expression);
        out(" ? "); // ast->questionToken
        accept(ast->ok);
        out(" : "); // ast->colonToken
        accept(ast->ko);
        return false;
    }

    bool visit(Block *ast) override
    {
        out(ast->lbraceToken);
        ++expressionDepth;
        lnAcceptIndented(ast->statements);
        newLine();
        --expressionDepth;
        out(ast->rbraceToken);
        return false;
    }

    bool visit(VariableStatement *ast) override
    {
        out(ast->declarationKindToken);
        out(" ");
        accept(ast->declarations);
        if (addSemicolons())
            out(";");
        return false;
    }


    void outputScope(VariableScope scope) {
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

    bool visit(PatternElement *ast) override
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

    bool visit(EmptyStatement *ast) override
    {
        out(ast->semicolonToken);
        return false;
    }

    bool visit(IfStatement *ast) override
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

    bool visit(DoWhileStatement *ast) override
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

    bool visit(WhileStatement *ast) override
    {
        out(ast->whileToken);
        out(" ");
        out(ast->lparenToken);
        accept(ast->expression);
        out(ast->rparenToken);
        acceptBlockOrIndented(ast->statement);
        return false;
    }

    bool visit(ForStatement *ast) override
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

    bool visit(ForEachStatement *ast) override
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

    bool visit(ContinueStatement *ast) override
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

    bool visit(BreakStatement *ast) override
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

    bool visit(ReturnStatement *ast) override
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

    bool visit(ThrowStatement *ast) override
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

    bool visit(WithStatement *ast) override
    {
        out(ast->withToken);
        out(" ");
        out(ast->lparenToken);
        accept(ast->expression);
        out(ast->rparenToken);
        acceptBlockOrIndented(ast->statement);
        return false;
    }

    bool visit(SwitchStatement *ast) override
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

    bool visit(CaseBlock *ast) override
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

    bool visit(CaseClause *ast) override
    {
        out("case "); // ast->caseToken
        accept(ast->expression);
        out(ast->colonToken);
        if (ast->statements)
            lnAcceptIndented(ast->statements);
        return false;
    }

    bool visit(DefaultClause *ast) override
    {
        out(ast->defaultToken);
        out(ast->colonToken);
        lnAcceptIndented(ast->statements);
        return false;
    }

    bool visit(LabelledStatement *ast) override
    {
        out(ast->identifierToken);
        out(": "); // ast->colonToken
        accept(ast->statement);
        return false;
    }

    bool visit(TryStatement *ast) override
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

    bool visit(Catch *ast) override
    {
        out(ast->catchToken);
        out(" ");
        out(ast->lparenToken);
        out(ast->identifierToken);
        out(") "); // ast->rparenToken
        accept(ast->statement);
        return false;
    }

    bool visit(Finally *ast) override
    {
        out("finally "); // ast->finallyToken
        accept(ast->statement);
        return false;
    }

    bool visit(FunctionDeclaration *ast) override
    {
        return visit(static_cast<FunctionExpression *>(ast));
    }

    bool visit(FunctionExpression *ast) override
    {
        if (!ast->isArrowFunction) {
            out("function "); // ast->functionToken
            if (!ast->name.isNull())
                out(ast->identifierToken);
        }
        out(ast->lparenToken);
        const bool needParentheses = ast->formals &&
                (ast->formals->next ||
                 (ast->formals->element && ast->formals->element->bindingTarget));
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

    bool visit(Elision *ast) override
    {
        for (Elision *it = ast; it; it = it->next) {
            if (it->next)
                out(", "); // ast->commaToken
        }
        return false;
    }

    bool visit(ArgumentList *ast) override
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

    bool visit(StatementList *ast) override
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

    bool visit(VariableDeclarationList *ast) override
    {
        for (VariableDeclarationList *it = ast; it; it = it->next) {
            accept(it->declaration);
            if (it->next)
                out(", "); // it->commaToken
        }
        return false;
    }

    bool visit(CaseClauses *ast) override
    {
        for (CaseClauses *it = ast; it; it = it->next) {
            accept(it->clause);
            if (it->next)
                newLine();
        }
        return false;
    }

    bool visit(FormalParameterList *ast) override
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
    bool visit(TypeExpression *) override { return true; }
    bool visit(SuperLiteral *) override
    {
        out("super");
        return true;
    }
    bool visit(PatternProperty *) override { return true; }
    bool visit(ComputedPropertyName *) override
    {
        out("[");
        return true;
    }
    bool visit(TaggedTemplate *) override { return true; }
    bool visit(Expression *el) override
    {
        accept(el->left);
        out(", ");
        accept(el->right);
        return false;
    }
    bool visit(ExpressionStatement *el) override
    {
        if (addSemicolons())
            postOps[el->expression].append([this]() { out(";"); });
        return true;
    }
    bool visit(YieldExpression *) override { return true; }
    bool visit(ClassExpression *) override { return true; }

    // Return false because we want to omit default function calls in accept0 implementation.
    bool visit(ClassDeclaration *ast) override
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

    bool visit(ClassElementList *) override { return true; }
    bool visit(Program *) override { return true; }
    bool visit(NameSpaceImport *) override { return true; }
    bool visit(ImportSpecifier *) override { return true; }
    bool visit(ImportsList *) override { return true; }
    bool visit(NamedImports *) override { return true; }
    bool visit(FromClause *) override { return true; }
    bool visit(ImportClause *) override { return true; }
    bool visit(ImportDeclaration *) override { return true; }
    bool visit(ExportSpecifier *) override { return true; }
    bool visit(ExportsList *) override { return true; }
    bool visit(ExportClause *) override { return true; }
    bool visit(ExportDeclaration *) override { return true; }
    bool visit(ESModule *) override { return true; }
    bool visit(DebuggerStatement *) override { return true; }
    bool visit(Type *) override { return true; }
    bool visit(TypeAnnotation *) override { return true; }

    // overridden to use BasicVisitor (and ensure warnings about new added AST)
    void endVisit(UiProgram *) override { }
    void endVisit(UiImport *) override { }
    void endVisit(UiHeaderItemList *) override { }
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
    void endVisit(UiPragmaValueList *) override { }
#endif
    void endVisit(UiPragma *) override { }
    void endVisit(UiPublicMember *) override { }
    void endVisit(UiSourceElement *) override { }
    void endVisit(UiObjectDefinition *) override { }
    void endVisit(UiObjectInitializer *) override { }
    void endVisit(UiObjectBinding *) override { }
    void endVisit(UiScriptBinding *) override { }
    void endVisit(UiArrayBinding *) override { }
    void endVisit(UiParameterList *) override { }
    void endVisit(UiObjectMemberList *) override { }
    void endVisit(UiArrayMemberList *) override { }
    void endVisit(UiQualifiedId *) override { }
    void endVisit(UiEnumDeclaration *) override { }
    void endVisit(UiEnumMemberList *) override { }
    void endVisit(UiVersionSpecifier *) override { }
    void endVisit(UiInlineComponent *) override { }
    void endVisit(UiAnnotation *) override { }
    void endVisit(UiAnnotationList *) override { }
    void endVisit(UiRequired *) override { }
    void endVisit(TypeExpression *) override { }
    void endVisit(ThisExpression *) override { }
    void endVisit(IdentifierExpression *) override { }
    void endVisit(NullExpression *) override { }
    void endVisit(TrueLiteral *) override { }
    void endVisit(FalseLiteral *) override { }
    void endVisit(SuperLiteral *) override { }
    void endVisit(StringLiteral *) override { }
    void endVisit(TemplateLiteral *) override { }
    void endVisit(NumericLiteral *) override { }
    void endVisit(RegExpLiteral *) override { }
    void endVisit(ArrayPattern *) override { }
    void endVisit(ObjectPattern *) override { }
    void endVisit(PatternElementList *) override { }
    void endVisit(PatternPropertyList *) override { }
    void endVisit(PatternElement *) override { }
    void endVisit(PatternProperty *) override { }
    void endVisit(Elision *) override { }
    void endVisit(NestedExpression *) override { }
    void endVisit(IdentifierPropertyName *) override { }
    void endVisit(StringLiteralPropertyName *) override { }
    void endVisit(NumericLiteralPropertyName *) override { }
    void endVisit(ComputedPropertyName *) override { out("]"); }
    void endVisit(ArrayMemberExpression *) override { }
    void endVisit(FieldMemberExpression *) override { }
    void endVisit(TaggedTemplate *) override { }
    void endVisit(NewMemberExpression *) override { }
    void endVisit(NewExpression *) override { }
    void endVisit(CallExpression *) override { }
    void endVisit(ArgumentList *) override { }
    void endVisit(PostIncrementExpression *) override { }
    void endVisit(PostDecrementExpression *) override { }
    void endVisit(DeleteExpression *) override { }
    void endVisit(VoidExpression *) override { }
    void endVisit(TypeOfExpression *) override { }
    void endVisit(PreIncrementExpression *) override { }
    void endVisit(PreDecrementExpression *) override { }
    void endVisit(UnaryPlusExpression *) override { }
    void endVisit(UnaryMinusExpression *) override { }
    void endVisit(TildeExpression *) override { }
    void endVisit(NotExpression *) override { }
    void endVisit(BinaryExpression *) override { }
    void endVisit(ConditionalExpression *) override { }
    void endVisit(Expression *) override { }
    void endVisit(Block *) override { }
    void endVisit(StatementList *) override { }
    void endVisit(VariableStatement *) override { }
    void endVisit(VariableDeclarationList *) override { }
    void endVisit(EmptyStatement *) override { }
    void endVisit(ExpressionStatement *) override { }
    void endVisit(IfStatement *) override { }
    void endVisit(DoWhileStatement *) override { }
    void endVisit(WhileStatement *) override { }
    void endVisit(ForStatement *) override { }
    void endVisit(ForEachStatement *) override { }
    void endVisit(ContinueStatement *) override { }
    void endVisit(BreakStatement *) override { }
    void endVisit(ReturnStatement *) override { }
    void endVisit(YieldExpression *) override { }
    void endVisit(WithStatement *) override { }
    void endVisit(SwitchStatement *) override { }
    void endVisit(CaseBlock *) override { }
    void endVisit(CaseClauses *) override { }
    void endVisit(CaseClause *) override { }
    void endVisit(DefaultClause *) override { }
    void endVisit(LabelledStatement *) override { }
    void endVisit(ThrowStatement *) override { }
    void endVisit(TryStatement *) override { }
    void endVisit(Catch *) override { }
    void endVisit(Finally *) override { }
    void endVisit(FunctionDeclaration *) override { }
    void endVisit(FunctionExpression *) override { }
    void endVisit(FormalParameterList *) override { }
    void endVisit(ClassExpression *) override { }
    void endVisit(ClassDeclaration *) override { }
    void endVisit(ClassElementList *) override { }
    void endVisit(Program *) override { }
    void endVisit(NameSpaceImport *) override { }
    void endVisit(ImportSpecifier *) override { }
    void endVisit(ImportsList *) override { }
    void endVisit(NamedImports *) override { }
    void endVisit(FromClause *) override { }
    void endVisit(ImportClause *) override { }
    void endVisit(ImportDeclaration *) override { }
    void endVisit(ExportSpecifier *) override { }
    void endVisit(ExportsList *) override { }
    void endVisit(ExportClause *) override { }
    void endVisit(ExportDeclaration *) override { }
    void endVisit(ESModule *) override { }
    void endVisit(DebuggerStatement *) override { }
    void endVisit(Type *) override { }
    void endVisit(TypeAnnotation *) override { }

    void throwRecursionDepthError() override
    {
        out("/* ERROR: Hit recursion limit visiting AST, rewrite failed */");
    }
};

void reformatAst(OutWriter &lw, std::shared_ptr<AstComments> comments,
                 const std::function<QStringView(SourceLocation)> loc2Str, AST::Node *n)
{
    if (n) {
        Rewriter rewriter(lw, comments, loc2Str, n);
    }
}

} // namespace Dom
} // namespace QQmlJS
QT_END_NAMESPACE
