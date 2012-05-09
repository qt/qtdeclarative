#ifndef QV4CODEGEN_P_H
#define QV4CODEGEN_P_H

#include "qv4ir_p.h"
#include <private/qqmljsastvisitor_p.h>

namespace QQmlJS {

namespace AST {
class UiParameterList;
}

class Codegen: protected AST::Visitor
{
public:
    Codegen();

    void operator()(AST::Program *ast, IR::Module *module);

protected:
    enum Format { ex, cx, nx };
    struct Result {
        IR::Expr *code;
        IR::BasicBlock *iftrue;
        IR::BasicBlock *iffalse;
        Format format;
        Format requested;

        explicit Result(Format requested = ex)
            : code(0)
            , iftrue(0)
            , iffalse(0)
            , format(ex)
            , requested(requested) {}

        explicit Result(IR::BasicBlock *iftrue, IR::BasicBlock *iffalse)
            : code(0)
            , iftrue(iftrue)
            , iffalse(iffalse)
            , format(ex)
            , requested(cx) {}

        inline IR::Expr *operator*() const { Q_ASSERT(format == ex); return code; }
        inline IR::Expr *operator->() const { Q_ASSERT(format == ex); return code; }

        bool accept(Format f)
        {
            if (requested == f) {
                format = f;
                return true;
            }
            return false;
        }
    };

    struct UiMember {
    };

    struct Loop {
        IR::BasicBlock *breakBlock;
        IR::BasicBlock *continueBlock;

        Loop()
            : breakBlock(0), continueBlock(0) {}

        Loop(IR::BasicBlock *breakBlock, IR::BasicBlock *continueBlock)
            : breakBlock(breakBlock), continueBlock(continueBlock) {}
    };

    IR::Expr *member(IR::Expr *base, const QString *name);
    IR::Expr *subscript(IR::Expr *base, IR::Expr *index);
    IR::Expr *argument(IR::Expr *expr);
    IR::Expr *binop(IR::AluOp op, IR::Expr *left, IR::Expr *right);
    void move(IR::Expr *target, IR::Expr *source, IR::AluOp op = IR::OpInvalid);

    void linearize(IR::Function *function);
    void defineFunction(AST::FunctionExpression *ast, bool isDeclaration = false);
    int tempForLocalVariable(const QStringRef &string) const;

    void statement(AST::Statement *ast);
    void statement(AST::ExpressionNode *ast);
    void condition(AST::ExpressionNode *ast, IR::BasicBlock *iftrue, IR::BasicBlock *iffalse);
    Result expression(AST::ExpressionNode *ast);
    QString propertyName(AST::PropertyName *ast);
    Result sourceElement(AST::SourceElement *ast);
    UiMember uiObjectMember(AST::UiObjectMember *ast);

    void accept(AST::Node *node);

    void argumentList(AST::ArgumentList *ast);
    void caseBlock(AST::CaseBlock *ast);
    void caseClause(AST::CaseClause *ast);
    void caseClauses(AST::CaseClauses *ast);
    void catchNode(AST::Catch *ast);
    void defaultClause(AST::DefaultClause *ast);
    void elementList(AST::ElementList *ast);
    void elision(AST::Elision *ast);
    void finallyNode(AST::Finally *ast);
    void formalParameterList(AST::FormalParameterList *ast);
    void functionBody(AST::FunctionBody *ast);
    void program(AST::Program *ast);
    void propertyNameAndValueList(AST::PropertyNameAndValueList *ast);
    void sourceElements(AST::SourceElements *ast);
    void statementList(AST::StatementList *ast);
    void uiArrayMemberList(AST::UiArrayMemberList *ast);
    void uiImport(AST::UiImport *ast);
    void uiImportList(AST::UiImportList *ast);
    void uiObjectInitializer(AST::UiObjectInitializer *ast);
    void uiObjectMemberList(AST::UiObjectMemberList *ast);
    void uiParameterList(AST::UiParameterList *ast);
    void uiProgram(AST::UiProgram *ast);
    void uiQualifiedId(AST::UiQualifiedId *ast);
    void variableDeclaration(AST::VariableDeclaration *ast);
    void variableDeclarationList(AST::VariableDeclarationList *ast);

    // nodes
    virtual bool visit(AST::ArgumentList *ast);
    virtual bool visit(AST::CaseBlock *ast);
    virtual bool visit(AST::CaseClause *ast);
    virtual bool visit(AST::CaseClauses *ast);
    virtual bool visit(AST::Catch *ast);
    virtual bool visit(AST::DefaultClause *ast);
    virtual bool visit(AST::ElementList *ast);
    virtual bool visit(AST::Elision *ast);
    virtual bool visit(AST::Finally *ast);
    virtual bool visit(AST::FormalParameterList *ast);
    virtual bool visit(AST::FunctionBody *ast);
    virtual bool visit(AST::Program *ast);
    virtual bool visit(AST::PropertyNameAndValueList *ast);
    virtual bool visit(AST::SourceElements *ast);
    virtual bool visit(AST::StatementList *ast);
    virtual bool visit(AST::UiArrayMemberList *ast);
    virtual bool visit(AST::UiImport *ast);
    virtual bool visit(AST::UiImportList *ast);
    virtual bool visit(AST::UiObjectInitializer *ast);
    virtual bool visit(AST::UiObjectMemberList *ast);
    virtual bool visit(AST::UiParameterList *ast);
    virtual bool visit(AST::UiProgram *ast);
    virtual bool visit(AST::UiQualifiedId *ast);
    virtual bool visit(AST::VariableDeclaration *ast);
    virtual bool visit(AST::VariableDeclarationList *ast);

    // expressions
    virtual bool visit(AST::Expression *ast);
    virtual bool visit(AST::ArrayLiteral *ast);
    virtual bool visit(AST::ArrayMemberExpression *ast);
    virtual bool visit(AST::BinaryExpression *ast);
    virtual bool visit(AST::CallExpression *ast);
    virtual bool visit(AST::ConditionalExpression *ast);
    virtual bool visit(AST::DeleteExpression *ast);
    virtual bool visit(AST::FalseLiteral *ast);
    virtual bool visit(AST::FieldMemberExpression *ast);
    virtual bool visit(AST::FunctionExpression *ast);
    virtual bool visit(AST::IdentifierExpression *ast);
    virtual bool visit(AST::NestedExpression *ast);
    virtual bool visit(AST::NewExpression *ast);
    virtual bool visit(AST::NewMemberExpression *ast);
    virtual bool visit(AST::NotExpression *ast);
    virtual bool visit(AST::NullExpression *ast);
    virtual bool visit(AST::NumericLiteral *ast);
    virtual bool visit(AST::ObjectLiteral *ast);
    virtual bool visit(AST::PostDecrementExpression *ast);
    virtual bool visit(AST::PostIncrementExpression *ast);
    virtual bool visit(AST::PreDecrementExpression *ast);
    virtual bool visit(AST::PreIncrementExpression *ast);
    virtual bool visit(AST::RegExpLiteral *ast);
    virtual bool visit(AST::StringLiteral *ast);
    virtual bool visit(AST::ThisExpression *ast);
    virtual bool visit(AST::TildeExpression *ast);
    virtual bool visit(AST::TrueLiteral *ast);
    virtual bool visit(AST::TypeOfExpression *ast);
    virtual bool visit(AST::UnaryMinusExpression *ast);
    virtual bool visit(AST::UnaryPlusExpression *ast);
    virtual bool visit(AST::VoidExpression *ast);
    virtual bool visit(AST::FunctionDeclaration *ast);

    // property names
    virtual bool visit(AST::IdentifierPropertyName *ast);
    virtual bool visit(AST::NumericLiteralPropertyName *ast);
    virtual bool visit(AST::StringLiteralPropertyName *ast);

    // source elements
    virtual bool visit(AST::FunctionSourceElement *ast);
    virtual bool visit(AST::StatementSourceElement *ast);

    // statements
    virtual bool visit(AST::Block *ast);
    virtual bool visit(AST::BreakStatement *ast);
    virtual bool visit(AST::ContinueStatement *ast);
    virtual bool visit(AST::DebuggerStatement *ast);
    virtual bool visit(AST::DoWhileStatement *ast);
    virtual bool visit(AST::EmptyStatement *ast);
    virtual bool visit(AST::ExpressionStatement *ast);
    virtual bool visit(AST::ForEachStatement *ast);
    virtual bool visit(AST::ForStatement *ast);
    virtual bool visit(AST::IfStatement *ast);
    virtual bool visit(AST::LabelledStatement *ast);
    virtual bool visit(AST::LocalForEachStatement *ast);
    virtual bool visit(AST::LocalForStatement *ast);
    virtual bool visit(AST::ReturnStatement *ast);
    virtual bool visit(AST::SwitchStatement *ast);
    virtual bool visit(AST::ThrowStatement *ast);
    virtual bool visit(AST::TryStatement *ast);
    virtual bool visit(AST::VariableStatement *ast);
    virtual bool visit(AST::WhileStatement *ast);
    virtual bool visit(AST::WithStatement *ast);

    // ui object members
    virtual bool visit(AST::UiArrayBinding *ast);
    virtual bool visit(AST::UiObjectBinding *ast);
    virtual bool visit(AST::UiObjectDefinition *ast);
    virtual bool visit(AST::UiPublicMember *ast);
    virtual bool visit(AST::UiScriptBinding *ast);
    virtual bool visit(AST::UiSourceElement *ast);

private:
    Result _expr;
    QString _property;
    UiMember _uiMember;
    Loop _loop;
    IR::Module *_module;
    IR::Function *_function;
    IR::BasicBlock *_block;
    IR::BasicBlock *_exitBlock;
    unsigned _returnAddress;
};

} // end of namespace QQmlJS

#endif // QV4CODEGEN_P_H
