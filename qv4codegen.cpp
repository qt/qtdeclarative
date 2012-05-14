#include "qv4codegen_p.h"

#include <QtCore/QStringList>
#include <QtCore/QSet>
#include <QtCore/QBuffer>
#include <QtCore/QBitArray>
#include <private/qqmljsast_p.h>
#include <iostream>
#include <cassert>

using namespace QQmlJS;
using namespace AST;

namespace {
QTextStream qout(stdout, QIODevice::WriteOnly);

void edge(IR::BasicBlock *source, IR::BasicBlock *target)
{
    if (! source->out.contains(target))
        source->out.append(target);

    if (! target->in.contains(source))
        target->in.append(source);
}

void dfs(IR::BasicBlock *block,
         QSet<IR::BasicBlock *> *V,
         QVector<IR::BasicBlock *> *blocks)
{
    if (! V->contains(block)) {
        V->insert(block);

        foreach (IR::BasicBlock *succ, block->out)
            dfs(succ, V, blocks);

        blocks->append(block);
    }
}

struct ComputeUseDef: IR::StmtVisitor, IR::ExprVisitor
{
    IR::Function *_function;
    IR::Stmt *_stmt;

    ComputeUseDef(IR::Function *function)
        : _function(function)
        , _stmt(0) {}

    void operator()(IR::Stmt *s) {
        Q_ASSERT(! s->d);
        s->d = new IR::Stmt::Data;
        qSwap(_stmt, s);
        _stmt->accept(this);
        qSwap(_stmt, s);
    }

    virtual void visitConst(IR::Const *) {}
    virtual void visitString(IR::String *) {}
    virtual void visitName(IR::Name *) {}
    virtual void visitClosure(IR::Closure *) {}
    virtual void visitUnop(IR::Unop *e) { e->expr->accept(this); }
    virtual void visitBinop(IR::Binop *e) { e->left->accept(this); e->right->accept(this); }
    virtual void visitSubscript(IR::Subscript *e) { e->base->accept(this); e->index->accept(this); }
    virtual void visitMember(IR::Member *e) { e->base->accept(this); }
    virtual void visitExp(IR::Exp *s) { s->expr->accept(this); }
    virtual void visitEnter(IR::Enter *) {}
    virtual void visitLeave(IR::Leave *) {}
    virtual void visitJump(IR::Jump *) {}
    virtual void visitCJump(IR::CJump *s) { s->cond->accept(this); }
    virtual void visitRet(IR::Ret *s) { s->expr->accept(this); }

    virtual void visitTemp(IR::Temp *e) {
        if (! _stmt->d->uses.contains(e->index))
            _stmt->d->uses.append(e->index);
    }

    virtual void visitCall(IR::Call *e) {
        e->base->accept(this);
        for (IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitNew(IR::New *e) {
        e->base->accept(this);
        for (IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitMove(IR::Move *s) {
        if (IR::Temp *t = s->target->asTemp()) {
            if (! _stmt->d->defs.contains(t->index))
                _stmt->d->defs.append(t->index);
        } else {
            s->target->accept(this);
        }
        s->source->accept(this);
    }
};

void liveness(IR::Function *function)
{
    QSet<IR::BasicBlock *> V;
    QVector<IR::BasicBlock *> blocks;

    //
    // compute the CFG
    //
    foreach (IR::BasicBlock *block, function->basicBlocks) {
        if (IR::Stmt *term = block->terminator()) {
            if (IR::Jump *j = term->asJump())
                edge(block, j->target);
            else if (IR::CJump *cj = term->asCJump()) {
                edge(block, cj->iftrue);
                edge(block, cj->iffalse);
            }
        }
    }

    ComputeUseDef computeUseDef(function);
    foreach (IR::BasicBlock *block, function->basicBlocks) {
        foreach (IR::Stmt *s, block->statements)
            computeUseDef(s);
    }

    dfs(function->basicBlocks.first(), &V, &blocks);

    bool changed;
    do {
        changed = false;

        foreach (IR::BasicBlock *block, blocks) {
            const QBitArray previousLiveIn = block->liveIn;
            const QBitArray previousLiveOut = block->liveOut;
            QBitArray live(function->tempCount);
            foreach (IR::BasicBlock *succ, block->out)
                live |= succ->liveIn;
            block->liveOut = live;
            for (int i = block->statements.size() - 1; i != -1; --i) {
                IR::Stmt *s = block->statements.at(i);
                s->d->liveOut = live;
                foreach (unsigned d, s->d->defs)
                    live.clearBit(d);
                foreach (unsigned u, s->d->uses)
                    live.setBit(u);
                s->d->liveIn = live;
            }
            block->liveIn = live;
            if (! changed) {
                if (previousLiveIn != block->liveIn || previousLiveOut != block->liveOut)
                    changed = true;
            }
        }
    } while (changed);
}

struct ScanFunctionBody: Visitor
{
    using Visitor::visit;

    // search for locals
    QList<QStringRef> locals;
    int maxNumberOfArguments;
    bool hasDirectEval;
    bool hasNestedFunctions;

    ScanFunctionBody()
        : maxNumberOfArguments(0)
        , hasDirectEval(false)
        , hasNestedFunctions(false)
    {
    }

    void operator()(Node *node) {
        hasDirectEval = false;
        locals.clear();
        if (node)
            node->accept(this);
    }

protected:
    virtual bool visit(CallExpression *ast)
    {
        if (! hasDirectEval) {
            if (IdentifierExpression *id = cast<IdentifierExpression *>(ast->base)) {
                if (id->name == QLatin1String("eval")) {
                    hasDirectEval = true;
                }
            }
        }
        int argc = 0;
        for (AST::ArgumentList *it = ast->arguments; it; it = it->next)
            ++argc;
        maxNumberOfArguments = qMax(maxNumberOfArguments, argc);
        return true;
    }

    virtual bool visit(NewMemberExpression *ast)
    {
        int argc = 0;
        for (AST::ArgumentList *it = ast->arguments; it; it = it->next)
            ++argc;
        maxNumberOfArguments = qMax(maxNumberOfArguments, argc);
        return true;
    }

    virtual bool visit(VariableDeclaration *ast)
    {
        if (! locals.contains(ast->name))
            locals.append(ast->name);
        return true;
    }

    virtual bool visit(FunctionExpression *ast)
    {
        hasNestedFunctions = true;
        if (! locals.contains(ast->name))
            locals.append(ast->name);
        return false;
    }

    virtual bool visit(FunctionDeclaration *ast)
    {
        hasNestedFunctions = true;
        if (! locals.contains(ast->name))
            locals.append(ast->name);
        return false;
    }
};

} // end of anonymous namespace

Codegen::Codegen()
    : _function(0)
    , _block(0)
    , _exitBlock(0)
    , _returnAddress(0)
{
}

void Codegen::operator()(AST::Program *node, IR::Module *module)
{
    _module = module;

    ScanFunctionBody globalCodeInfo;
    globalCodeInfo(node);

    IR::Function *globalCode = _module->newFunction(QLatin1String("%entry"));
    globalCode->hasDirectEval = globalCodeInfo.hasDirectEval;
    globalCode->hasNestedFunctions = true; // ### FIXME: initialize it with globalCodeInfo.hasNestedFunctions;
    globalCode->maxNumberOfArguments = globalCodeInfo.maxNumberOfArguments;
    _function = globalCode;
    _block = _function->newBasicBlock();
    _exitBlock = _function->newBasicBlock();
    _returnAddress = _block->newTemp();
    move(_block->TEMP(_returnAddress), _block->CONST(IR::UndefinedType, 0));
    _exitBlock->RET(_exitBlock->TEMP(_returnAddress), IR::UndefinedType);

    program(node);

    if (! _block->isTerminated()) {
        _block->JUMP(_exitBlock);
    }

    foreach (IR::Function *function, _module->functions) {
        linearize(function);
    }
}

IR::Expr *Codegen::member(IR::Expr *base, const QString *name)
{
    if (base->asTemp() /*|| base->asName()*/)
        return _block->MEMBER(base, name);
    else {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), base);
        return _block->MEMBER(_block->TEMP(t), name);
    }
}

IR::Expr *Codegen::subscript(IR::Expr *base, IR::Expr *index)
{
    if (base->asTemp() || base->asName())
        return _block->SUBSCRIPT(base, index);
    else {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), base);
        return _block->SUBSCRIPT(_block->TEMP(t), index);
    }
}

IR::Expr *Codegen::argument(IR::Expr *expr)
{
    if (expr && ! expr->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    return expr;
}

IR::Expr *Codegen::binop(IR::AluOp op, IR::Expr *left, IR::Expr *right)
{
    if (left && ! left->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), left);
        left = _block->TEMP(t);
    }

    if (right && ! right->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), right);
        right = _block->TEMP(t);
    }

    return _block->BINOP(op, left, right);
}

IR::Expr *Codegen::call(IR::Expr *base, IR::ExprList *args)
{
    if (base->asMember() || base->asName() || base->asTemp())
        return _block->CALL(base, args);
    const unsigned t = _block->newTemp();
    _block->MOVE(_block->TEMP(t), base);
    return _block->CALL(_block->TEMP(t), args);
}

void Codegen::move(IR::Expr *target, IR::Expr *source, IR::AluOp op)
{
    if (target->asMember()) {
        if (! (source->asTemp() || source->asConst())) {
            const unsigned t = _block->newTemp();
            _block->MOVE(_block->TEMP(t), source);
            _block->MOVE(target, _block->TEMP(t), op);
            return;
        }
    } else if (! target->asTemp())  {
        if (! (source->asConst() || source->asTemp() || source->asName() || source->asSubscript())) {
            const unsigned t = _block->newTemp();
            _block->MOVE(_block->TEMP(t), source);
            _block->MOVE(target, _block->TEMP(t), op);
            return;
        }
    }
    _block->MOVE(target, source, op);
}

void Codegen::accept(Node *node)
{
    if (node)
        node->accept(this);
}

void Codegen::statement(Statement *ast)
{
    accept(ast);
}

void Codegen::statement(ExpressionNode *ast)
{
    if (ast) {
        Result r(nx);
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
        if (r.format == ex) {
            _block->EXP(*r);
        }
    }
}

void Codegen::condition(ExpressionNode *ast, IR::BasicBlock *iftrue, IR::BasicBlock *iffalse)
{
    if (ast) {
        Result r(iftrue, iffalse);
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
        if (r.format == ex) {
            _block->CJUMP(*r, r.iftrue, r.iffalse);
        }
    }
}

Codegen::Result Codegen::expression(ExpressionNode *ast)
{
    Result r;
    if (ast) {
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
    }
    return r;
}

QString Codegen::propertyName(PropertyName *ast)
{
    QString p;
    if (ast) {
        qSwap(_property, p);
        accept(ast);
        qSwap(_property, p);
    }
    return p;
}

Codegen::Result Codegen::sourceElement(SourceElement *ast)
{
    Result r(nx);
    if (ast) {
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
    }
    return r;
}

Codegen::UiMember Codegen::uiObjectMember(UiObjectMember *ast)
{
    UiMember m;
    if (ast) {
        qSwap(_uiMember, m);
        accept(ast);
        qSwap(_uiMember, m);
    }
    return m;
}

void Codegen::argumentList(ArgumentList *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::caseBlock(CaseBlock *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::caseClause(CaseClause *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::caseClauses(CaseClauses *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::catchNode(Catch *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::defaultClause(DefaultClause *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::elementList(ElementList *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::elision(Elision *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::finallyNode(Finally *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::formalParameterList(FormalParameterList *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::functionBody(FunctionBody *ast)
{
    if (ast)
        sourceElements(ast->elements);
}

void Codegen::program(Program *ast)
{
    if (ast)
        sourceElements(ast->elements);
}

void Codegen::propertyNameAndValueList(PropertyNameAndValueList *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::sourceElements(SourceElements *ast)
{
    for (SourceElements *it = ast; it; it = it->next) {
        sourceElement(it->element);
    }
}

void Codegen::statementList(StatementList *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::uiArrayMemberList(UiArrayMemberList *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::uiImport(UiImport *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::uiImportList(UiImportList *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::uiObjectInitializer(UiObjectInitializer *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::uiObjectMemberList(UiObjectMemberList *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::uiParameterList(UiParameterList *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::uiProgram(UiProgram *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::uiQualifiedId(UiQualifiedId *)
{
    Q_ASSERT(!"not implemented");
}

void Codegen::variableDeclaration(VariableDeclaration *ast)
{
    if (ast->expression) {
        Result expr = expression(ast->expression);

        if (! expr.code)
            expr.code = _block->CONST(IR::UndefinedType, 0);

        if (! _function->needsActivation()) {
            const int index = tempForLocalVariable(ast->name);
            if (index != -1) {
                move(_block->TEMP(index), *expr);
                return;
            }
        } else {
            move(_block->NAME(ast->name.toString(), ast->identifierToken.startLine, ast->identifierToken.startColumn), *expr);
        }
    }
}

void Codegen::variableDeclarationList(VariableDeclarationList *ast)
{
    for (VariableDeclarationList *it = ast; it; it = it->next) {
        variableDeclaration(it->declaration);
    }
}


bool Codegen::visit(ArgumentList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(CaseBlock *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(CaseClause *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(CaseClauses *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(Catch *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(DefaultClause *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(ElementList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(Elision *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(Finally *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(FormalParameterList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(FunctionBody *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(Program *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(PropertyNameAndValueList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(SourceElements *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(StatementList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiArrayMemberList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiImport *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiImportList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiObjectInitializer *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiObjectMemberList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiParameterList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiProgram *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(UiQualifiedId *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(VariableDeclaration *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(VariableDeclarationList *)
{
    Q_ASSERT(!"unreachable");
    return false;
}

bool Codegen::visit(Expression *ast)
{
    statement(ast->left);
    accept(ast->right);
    return false;
}

bool Codegen::visit(ArrayLiteral *ast)
{
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->NEW(_block->NAME(QLatin1String("Array"), ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn)));
    int index = 0;
    for (ElementList *it = ast->elements; it; it = it->next) {
        for (Elision *elision = it->elision; elision; elision = elision->next)
            ++index;
        Result expr = expression(it->expression);
        move(subscript(_block->TEMP(t), _block->CONST(IR::NumberType, index)), *expr);
        ++index;
    }
    for (Elision *elision = ast->elision; elision; elision = elision->next)
        ++index; // ### set the size of the array
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(ArrayMemberExpression *ast)
{
    Result base = expression(ast->base);
    Result index = expression(ast->expression);
    _expr.code = subscript(*base, *index);
    return false;
}

static IR::AluOp baseOp(int op)
{
    switch ((QSOperator::Op) op) {
    case QSOperator::InplaceAnd: return IR::OpAnd;
    case QSOperator::InplaceSub: return IR::OpSub;
    case QSOperator::InplaceDiv: return IR::OpDiv;
    case QSOperator::InplaceAdd: return IR::OpAdd;
    case QSOperator::InplaceLeftShift: return IR::OpLShift;
    case QSOperator::InplaceMod: return IR::OpMod;
    case QSOperator::InplaceMul: return IR::OpMul;
    case QSOperator::InplaceOr: return IR::OpBitOr;
    case QSOperator::InplaceRightShift: return IR::OpRShift;
    case QSOperator::InplaceURightShift: return IR::OpURShift;
    case QSOperator::InplaceXor: return IR::OpBitXor;
    default: return IR::OpInvalid;
    }
}

bool Codegen::visit(BinaryExpression *ast)
{
    if (ast->op == QSOperator::And) {
        if (_expr.accept(cx)) {
            IR::BasicBlock *iftrue = _function->newBasicBlock();
            condition(ast->left, iftrue, _expr.iffalse);
            _block = iftrue;
            condition(ast->right, _expr.iftrue, _expr.iffalse);
        } else {
            IR::BasicBlock *iftrue = _function->newBasicBlock();
            IR::BasicBlock *iffalse = _function->newBasicBlock();
            IR::BasicBlock *endif = _function->newBasicBlock();

            const unsigned r = _block->newTemp();

            condition(ast->left, iftrue, iffalse);
            _block = iffalse;

            move(_block->TEMP(r), _block->CONST(IR::BoolType, 0));
            _block->JUMP(endif);

            _block = iftrue;
            move(_block->TEMP(r), *expression(ast->right));
            if (! _block->isTerminated())
                _block->JUMP(endif);

            _expr.code = _block->TEMP(r);
            _block = endif;
        }
        return false;
    } else if (ast->op == QSOperator::Or) {
        if (_expr.accept(cx)) {
            IR::BasicBlock *iffalse = _function->newBasicBlock();
            condition(ast->left, _expr.iftrue, iffalse);
            _block = iffalse;
            condition(ast->right, _expr.iftrue, _expr.iffalse);
        } else {
            IR::BasicBlock *iffalse = _function->newBasicBlock();
            IR::BasicBlock *endif = _function->newBasicBlock();

            const unsigned r = _block->newTemp();
            move(_block->TEMP(r), *expression(ast->left));
            _block->CJUMP(_block->TEMP(r), endif, iffalse);
            _block = iffalse;
            move(_block->TEMP(r), *expression(ast->right));
            if (! _block->isTerminated())
                _block->JUMP(endif);

            _block = endif;
            _expr.code = _block->TEMP(r);
        }
        return false;
    }

    Result left = expression(ast->left);
    Result right = expression(ast->right);

    switch (ast->op) {
    case QSOperator::Or:
    case QSOperator::And:
        break;

    case QSOperator::Assign:
        if (_expr.accept(nx)) {
            move(*left, *right);
        } else {
            const unsigned t = _block->newTemp();
            move(_block->TEMP(t), *right);
            move(*left, _block->TEMP(t));
            _expr.code = _block->TEMP(t);
        }
        break;

    case QSOperator::InplaceAnd:
    case QSOperator::InplaceSub:
    case QSOperator::InplaceDiv:
    case QSOperator::InplaceAdd:
    case QSOperator::InplaceLeftShift:
    case QSOperator::InplaceMod:
    case QSOperator::InplaceMul:
    case QSOperator::InplaceOr:
    case QSOperator::InplaceRightShift:
    case QSOperator::InplaceURightShift:
    case QSOperator::InplaceXor: {
        move(*left, *right, baseOp(ast->op));
        if (_expr.accept(nx)) {
            // nothing to do
        } else {
            _expr.code = *left;
        }
        break;
    }

    case QSOperator::In:
    case QSOperator::InstanceOf:
    case QSOperator::Equal:
    case QSOperator::NotEqual:
    case QSOperator::Ge:
    case QSOperator::Gt:
    case QSOperator::Le:
    case QSOperator::Lt:
    case QSOperator::StrictEqual:
    case QSOperator::StrictNotEqual:
        if (false && _expr.accept(cx)) {
            _block->CJUMP(binop(IR::binaryOperator(ast->op), *left, *right), _expr.iftrue, _expr.iffalse);
        } else {
            IR::Expr *e = binop(IR::binaryOperator(ast->op), *left, *right);
            if (e->asConst() || e->asString())
                _expr.code = e;
            else {
                const unsigned t = _block->newTemp();
                move(_block->TEMP(t), e);
                _expr.code = _block->TEMP(t);
            }
        }
        break;

    case QSOperator::Add:
    case QSOperator::BitAnd:
    case QSOperator::BitOr:
    case QSOperator::BitXor:
    case QSOperator::Div:
    case QSOperator::LShift:
    case QSOperator::Mod:
    case QSOperator::Mul:
    case QSOperator::RShift:
    case QSOperator::Sub:
    case QSOperator::URShift: {
        IR::Expr *e = binop(IR::binaryOperator(ast->op), *left, *right);
        if (e->asConst() || e->asString())
            _expr.code = e;
        else {
            const unsigned t = _block->newTemp();
            move(_block->TEMP(t), e);
            _expr.code = _block->TEMP(t);
        }
        break;
    }

    } // switch

    return false;
}

bool Codegen::visit(CallExpression *ast)
{
    Result base = expression(ast->base);
    IR::ExprList *args = 0, **args_it = &args;
    for (ArgumentList *it = ast->arguments; it; it = it->next) {
        Result arg = expression(it->expression);
        IR::Expr *actual = argument(*arg);
        *args_it = _function->New<IR::ExprList>();
        (*args_it)->init(actual);
        args_it = &(*args_it)->next;
    }
    _expr.code = call(*base, args);
    return false;
}

bool Codegen::visit(ConditionalExpression *ast)
{
    IR::BasicBlock *iftrue = _function->newBasicBlock();
    IR::BasicBlock *iffalse = _function->newBasicBlock();
    IR::BasicBlock *endif = _function->newBasicBlock();

    const unsigned t = _block->newTemp();

    condition(ast->expression, iftrue, iffalse);

    _block = iftrue;
    move(_block->TEMP(t), *expression(ast->ok));
    _block->JUMP(endif);

    _block = iffalse;
    move(_block->TEMP(t), *expression(ast->ko));
    _block->JUMP(endif);

    _block = endif;

    _expr.code = _block->TEMP(t);

    return false;
}

bool Codegen::visit(DeleteExpression *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(FalseLiteral *)
{
    _expr.code = _block->CONST(IR::BoolType, 0);
    return false;
}

bool Codegen::visit(FieldMemberExpression *ast)
{
    Result base = expression(ast->base);
    _expr.code = member(*base, _function->newString(ast->name.toString()));
    return false;
}

bool Codegen::visit(FunctionExpression *ast)
{
    defineFunction(ast, false);
    return false;
}

bool Codegen::visit(IdentifierExpression *ast)
{
    if (! _function->needsActivation()) {
        int index = tempForLocalVariable(ast->name);
        if (index != -1) {
            _expr.code = _block->TEMP(index);
            return false;
        }
    }

    _expr.code = _block->NAME(ast->name.toString(),
                              ast->identifierToken.startLine,
                              ast->identifierToken.startColumn);

    return false;
}

bool Codegen::visit(NestedExpression *ast)
{
    accept(ast->expression);
    return false;
}

bool Codegen::visit(NewExpression *ast)
{
    Result base = expression(ast->expression);
    _expr.code = _block->NEW(*base, 0);
    return false;
}

bool Codegen::visit(NewMemberExpression *ast)
{
    Result base = expression(ast->base);
    IR::ExprList *args = 0, **args_it = &args;
    for (ArgumentList *it = ast->arguments; it; it = it->next) {
        Result arg = expression(it->expression);
        IR::Expr *actual = argument(*arg);
        *args_it = _function->New<IR::ExprList>();
        (*args_it)->init(actual);
        args_it = &(*args_it)->next;
    }
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->NEW(*base, args));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(NotExpression *ast)
{
    Result expr = expression(ast->expression);
    const unsigned r = _block->newTemp();
    move(_block->TEMP(r), _block->UNOP(IR::OpNot, *expr));
    _expr.code = _block->TEMP(r);
    return false;
}

bool Codegen::visit(NullExpression *)
{
    _expr.code = _block->CONST(IR::NullType, 0);
    return false;
}

bool Codegen::visit(NumericLiteral *ast)
{
    _expr.code = _block->CONST(IR::NumberType, ast->value);
    return false;
}

bool Codegen::visit(ObjectLiteral *ast)
{
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->NEW(_block->NAME(QLatin1String("Object"), ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn)));
    for (PropertyNameAndValueList *it = ast->properties; it; it = it->next) {
        QString name = propertyName(it->name);
        Result value = expression(it->value);
        move(member(_block->TEMP(t), _function->newString(name)), *value);
    }
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(PostDecrementExpression *ast)
{
    Result expr = expression(ast->base);
    if (_expr.accept(nx)) {
        move(*expr, _block->CONST(IR::NumberType, 1), IR::OpSub);
    } else {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), *expr);
        move(*expr, _block->CONST(IR::NumberType, 1), IR::OpSub);
        _expr.code = _block->TEMP(t);
    }
    return false;
}

bool Codegen::visit(PostIncrementExpression *ast)
{
    Result expr = expression(ast->base);
    if (_expr.accept(nx)) {
        move(*expr, _block->CONST(IR::NumberType, 1), IR::OpAdd);
    } else {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), *expr);
        move(*expr, _block->CONST(IR::NumberType, 1), IR::OpAdd);
        _expr.code = _block->TEMP(t);
    }
    return false;
}

bool Codegen::visit(PreDecrementExpression *ast)
{
    Result expr = expression(ast->expression);
    move(*expr, _block->CONST(IR::NumberType, 1), IR::OpSub);
    if (_expr.accept(nx)) {
        // nothing to do
    } else {
        _expr.code = *expr;
    }
    return false;
}

bool Codegen::visit(PreIncrementExpression *ast)
{
    Result expr = expression(ast->expression);
    move(*expr, _block->CONST(IR::NumberType, 1), IR::OpAdd);

    if (_expr.accept(nx)) {
        // nothing to do
    } else {
        _expr.code = *expr;
    }
    return false;
}

bool Codegen::visit(RegExpLiteral *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(StringLiteral *ast)
{
    _expr.code = _block->STRING(_function->newString(ast->value.toString()));
    return false;
}

bool Codegen::visit(ThisExpression *ast)
{
    _expr.code = _block->NAME(QLatin1String("this"), ast->thisToken.startLine, ast->thisToken.startColumn);
    return false;
}

bool Codegen::visit(TildeExpression *ast)
{
    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->UNOP(IR::OpCompl, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(TrueLiteral *)
{
    _expr.code = _block->CONST(IR::BoolType, 1);
    return false;
}

bool Codegen::visit(TypeOfExpression *ast)
{
    Result expr = expression(ast->expression);
    IR::ExprList *args = _function->New<IR::ExprList>();
    args->init(argument(*expr));
    _expr.code = call(_block->NAME(QLatin1String("typeof"), ast->typeofToken.startLine, ast->typeofToken.startColumn), args);
    return false;
}

bool Codegen::visit(UnaryMinusExpression *ast)
{
    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->UNOP(IR::OpUMinus, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(UnaryPlusExpression *ast)
{
    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->UNOP(IR::OpUPlus, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(VoidExpression *ast)
{
    statement(ast->expression); // ### CHECK
    return false;
}

bool Codegen::visit(FunctionDeclaration *ast)
{
    defineFunction(ast);
    return false;
}

void Codegen::linearize(IR::Function *function)
{
    IR::BasicBlock *entryBlock = function->basicBlocks.at(0);
    IR::BasicBlock *exitBlock = function->basicBlocks.at(1);

    Q_UNUSED(entryBlock);

    QSet<IR::BasicBlock *> V;
    V.insert(exitBlock);

    QVector<IR::BasicBlock *> trace;

    for (int i = 0; i < function->basicBlocks.size(); ++i) {
        IR::BasicBlock *block = function->basicBlocks.at(i);
        if (block->statements.isEmpty()) {
            if ((i + 1) < function->basicBlocks.size()) {
                IR::BasicBlock *next = function->basicBlocks.at(i + 1);
                block->JUMP(next);
            }
        }
    }

    foreach (IR::BasicBlock *block, function->basicBlocks) {
        while (block) {
            if (V.contains(block))
                break;

            V.insert(block);
            block->index = trace.size();
            trace.append(block);

            if (IR::Stmt *term = block->terminator()) {
                block = 0;

                if (IR::Jump *j = term->asJump()) {
                    block = j->target;
                } else if (IR::CJump *cj = term->asCJump()) {
                    if (! V.contains(cj->iffalse))
                        block = cj->iffalse;
                    else
                        block = cj->iftrue;
                }
            }
        }
    }
    exitBlock->index = trace.size();
    trace.append(exitBlock);
    function->basicBlocks = trace;

    liveness(function);

    static bool showCode = !qgetenv("SHOW_CODE").isNull();
    if (showCode) {
        QVector<IR::Stmt *> code;
        QHash<IR::Stmt *, IR::BasicBlock *> leader;

        foreach (IR::BasicBlock *block, function->basicBlocks) {
            leader.insert(block->statements.first(), block);
            foreach (IR::Stmt *s, block->statements) {
                code.append(s);
            }
        }

        QString name;
        if (function->name && !function->name->isEmpty())
            name = *function->name;
        else
            name.sprintf("%p", function);

        qout << "function " << name << "(";
        for (int i = 0; i < function->formals.size(); ++i) {
            if (i != 0)
                qout << ", ";
            qout << *function->formals.at(i);
        }
        qout << ")" << endl
             << "{" << endl;

        foreach (const QString *local, function->locals) {
            qout << "    var " << *local << ';' << endl;
        }

        for (int i = 0; i < code.size(); ++i) {
            IR::Stmt *s = code.at(i);

            if (IR::BasicBlock *bb = leader.value(s)) {
                qout << endl;
                qout << 'L' << bb->index << ':' << endl;
            }
            IR::Stmt *n = (i + 1) < code.size() ? code.at(i + 1) : 0;
            if (n && s->asJump() && s->asJump()->target == leader.value(n)) {
                continue;
            }

            QByteArray str;
            QBuffer buf(&str);
            buf.open(QIODevice::WriteOnly);
            QTextStream out(&buf);
            s->dump(out, IR::Stmt::MIR);
            out.flush();

            for (int i = 60 - str.size(); i >= 0; --i)
                str.append(' ');

            qout << "    " << str;

            //        if (! s->uses.isEmpty()) {
            //            qout << " // uses:";
            //            foreach (unsigned use, s->uses) {
            //                qout << " %" << use;
            //            }
            //        }

            //        if (! s->defs.isEmpty()) {
            //            qout << " // defs:";
            //            foreach (unsigned def, s->defs) {
            //                qout << " %" << def;
            //            }
            //        }

            if (! s->d->liveOut.isEmpty()) {
                qout << " // lives:";
                for (int i = 0; i < s->d->liveOut.size(); ++i) {
                    if (s->d->liveOut.testBit(i))
                        qout << " %" << i;
                }
            }

            qout << endl;

            if (n && s->asCJump() && s->asCJump()->iffalse != leader.value(n)) {
                qout << "    goto L" << s->asCJump()->iffalse << ";" << endl;
            }
        }

        qout << "}" << endl
             << endl;
    }

    foreach (IR::BasicBlock *block, function->basicBlocks) {
        foreach (IR::Stmt *s, block->statements)
            s->destroyData();
    }
}

void Codegen::defineFunction(FunctionExpression *ast, bool /*isDeclaration*/)
{
    ScanFunctionBody functionInfo;
    functionInfo(ast->body);

    IR::Function *function = _module->newFunction(ast->name.toString());
    IR::BasicBlock *entryBlock = function->newBasicBlock();
    IR::BasicBlock *exitBlock = function->newBasicBlock();
    function->hasDirectEval = functionInfo.hasDirectEval;
    function->maxNumberOfArguments = functionInfo.maxNumberOfArguments;

    if (! functionInfo.hasDirectEval) {
        for (int i = 0; i < functionInfo.locals.size(); ++i) {
            unsigned t = entryBlock->newTemp();
            Q_ASSERT(t == unsigned(i));
            entryBlock->MOVE(entryBlock->TEMP(t), entryBlock->CONST(IR::UndefinedType, 0));
        }
    }

    unsigned returnAddress = entryBlock->newTemp();

    entryBlock->MOVE(entryBlock->TEMP(returnAddress), entryBlock->CONST(IR::UndefinedType, 0));
    exitBlock->RET(_block->TEMP(returnAddress), IR::InvalidType);

    qSwap(_function, function);
    qSwap(_block, entryBlock);
    qSwap(_exitBlock, exitBlock);
    qSwap(_returnAddress, returnAddress);

    for (FormalParameterList *it = ast->formals; it; it = it->next) {
        _function->RECEIVE(it->name.toString());
    }

    foreach (const QStringRef &local, functionInfo.locals) {
        _function->LOCAL(local.toString());
    }

    functionBody(ast->body);

    if (! _block->isTerminated())
        _block->JUMP(_exitBlock);

    qSwap(_function, function);
    qSwap(_block, entryBlock);
    qSwap(_exitBlock, exitBlock);
    qSwap(_returnAddress, returnAddress);

    if (_expr.accept(nx)) {
        // nothing to do
    } else {
        _expr.code = _block->CLOSURE(function);
    }
}

int Codegen::tempForLocalVariable(const QStringRef &string) const
{
    for (int i = 0; i < _function->locals.size(); ++i) {
        if (*_function->locals.at(i) == string)
            return i;
    }
    return -1;
}

bool Codegen::visit(IdentifierPropertyName *ast)
{
    _property = ast->id.toString();
    return false;
}

bool Codegen::visit(NumericLiteralPropertyName *ast)
{
    _property = QString::number(ast->id, 'g', 16);
    return false;
}

bool Codegen::visit(StringLiteralPropertyName *ast)
{
    _property = ast->id.toString();
    return false;
}

bool Codegen::visit(FunctionSourceElement *ast)
{
    statement(ast->declaration);
    return false;
}

bool Codegen::visit(StatementSourceElement *ast)
{
    statement(ast->statement);
    return false;
}

bool Codegen::visit(Block *ast)
{
    for (StatementList *it = ast->statements; it; it = it->next) {
        statement(it->statement);
    }
    return false;
}

bool Codegen::visit(BreakStatement *)
{
    _block->JUMP(_loop.breakBlock);
    return false;
}

bool Codegen::visit(ContinueStatement *)
{
    _block->JUMP(_loop.continueBlock);
    return false;
}

bool Codegen::visit(DebuggerStatement *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(DoWhileStatement *ast)
{
    IR::BasicBlock *loopbody = _function->newBasicBlock();
    IR::BasicBlock *loopend = _function->newBasicBlock();

    Loop loop(loopend, loopbody);
    qSwap(_loop, loop);

    if (_block->isTerminated())
        _block->JUMP(loopbody);
    _block = loopbody;
    statement(ast->statement);
    condition(ast->expression, loopbody, loopend);
    _block = loopend;

    qSwap(_loop, loop);
    return false;
}

bool Codegen::visit(EmptyStatement *)
{
    return false;
}

bool Codegen::visit(ExpressionStatement *ast)
{
    statement(ast->expression);
    return false;
}

bool Codegen::visit(ForEachStatement *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(ForStatement *ast)
{
    IR::BasicBlock *forcond = _function->newBasicBlock();
    IR::BasicBlock *forbody = _function->newBasicBlock();
    IR::BasicBlock *forstep = _function->newBasicBlock();
    IR::BasicBlock *forend = _function->newBasicBlock();

    Loop loop(forend, forstep);
    qSwap(_loop, loop);

    statement(ast->initialiser);
    if (! _block->isTerminated())
        _block->JUMP(forcond);

    _block = forcond;
    condition(ast->condition, forbody, forend);

    _block = forbody;
    statement(ast->statement);
    if (! _block->isTerminated())
        _block->JUMP(forstep);

    _block = forstep;
    statement(ast->expression);
    if (! _block->isTerminated())
        _block->JUMP(forcond);
    _block = forend;

    qSwap(_loop, loop);

    return false;
}

bool Codegen::visit(IfStatement *ast)
{
    IR::BasicBlock *iftrue = _function->newBasicBlock();
    IR::BasicBlock *iffalse = ast->ko ? _function->newBasicBlock() : 0;
    IR::BasicBlock *endif = _function->newBasicBlock();
    condition(ast->expression, iftrue, ast->ko ? iffalse : endif);

    _block = iftrue;
    statement(ast->ok);
    if (! _block->isTerminated())
        _block->JUMP(endif);

    if (ast->ko) {
        _block = iffalse;
        statement(ast->ko);
        if (! _block->isTerminated())
            _block->JUMP(endif);
    }

    _block = endif;

    return false;
}

bool Codegen::visit(LabelledStatement *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(LocalForEachStatement *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(LocalForStatement *ast)
{
    IR::BasicBlock *forcond = _function->newBasicBlock();
    IR::BasicBlock *forbody = _function->newBasicBlock();
    IR::BasicBlock *forstep = _function->newBasicBlock();
    IR::BasicBlock *forend = _function->newBasicBlock();

    Loop loop(forend, forstep);
    qSwap(_loop, loop);

    variableDeclarationList(ast->declarations);
    if (! _block->isTerminated())
        _block->JUMP(forcond);

    _block = forcond;
    condition(ast->condition, forbody, forend);

    _block = forbody;
    statement(ast->statement);
    if (! _block->isTerminated())
        _block->JUMP(forstep);

    _block = forstep;
    statement(ast->expression);
    if (! _block->isTerminated())
        _block->JUMP(forcond);
    _block = forend;

    qSwap(_loop, loop);
    return false;
}

bool Codegen::visit(ReturnStatement *ast)
{
    if (ast->expression) {
        Result expr = expression(ast->expression);
        move(_block->TEMP(_returnAddress), *expr);
    }

    _block->JUMP(_exitBlock);
    return false;
}

bool Codegen::visit(SwitchStatement *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(ThrowStatement *)
{
    //Q_ASSERT(!"not implemented");
    _expr.code = _block->CONST(IR::UndefinedType, 0);
    return false;
}

bool Codegen::visit(TryStatement *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(VariableStatement *ast)
{
    variableDeclarationList(ast->declarations);
    return false;
}

bool Codegen::visit(WhileStatement *ast)
{
    IR::BasicBlock *whilecond = _function->newBasicBlock();
    IR::BasicBlock *whilebody = _function->newBasicBlock();
    IR::BasicBlock *whileend = _function->newBasicBlock();

    Loop loop(whileend, whilecond);
    qSwap(_loop, loop);

    _block->JUMP(whilecond);
    _block = whilecond;
    condition(ast->expression, whilebody, whileend);

    _block = whilebody;
    statement(ast->statement);
    if (! _block->isTerminated())
        _block->JUMP(whilecond);

    _block = whileend;
    qSwap(_loop, loop);

    return false;
}

bool Codegen::visit(WithStatement *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(UiArrayBinding *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(UiObjectBinding *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(UiObjectDefinition *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(UiPublicMember *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(UiScriptBinding *)
{
    Q_ASSERT(!"not implemented");
    return false;
}

bool Codegen::visit(UiSourceElement *)
{
    Q_ASSERT(!"not implemented");
    return false;
}
