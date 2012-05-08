/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QV4IR_P_H
#define QV4IR_P_H

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

#include <private/qqmljsmemorypool_p.h>

#include <QtCore/QVector>
#include <QtCore/QString>
#include <QtCore/QBitArray>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QTextStream;
class QQmlType;

namespace QQmlJS {

namespace VM {
struct Context;
struct Value;
}

namespace IR {

struct BasicBlock;
struct Function;
struct Module;

struct Stmt;
struct Expr;

// expressions
struct Const;
struct String;
struct Name;
struct Temp;
struct Closure;
struct Unop;
struct Binop;
struct Call;
struct New;
struct Subscript;
struct Member;

// statements
struct Exp;
struct Enter;
struct Leave;
struct Move;
struct Jump;
struct CJump;
struct Ret;

enum AluOp {
    OpInvalid = 0,

    OpIfTrue,
    OpNot,
    OpUMinus,
    OpUPlus,
    OpCompl,

    OpBitAnd,
    OpBitOr,
    OpBitXor,

    OpAdd,
    OpSub,
    OpMul,
    OpDiv,
    OpMod,

    OpLShift,
    OpRShift,
    OpURShift,

    OpGt,
    OpLt,
    OpGe,
    OpLe,
    OpEqual,
    OpNotEqual,
    OpStrictEqual,
    OpStrictNotEqual,

    OpAnd,
    OpOr
};
AluOp binaryOperator(int op);
const char *opname(IR::AluOp op);

enum Type {
    InvalidType,
    UndefinedType,
    NullType,
    VoidType,
    StringType,
    UrlType,
    ColorType,
    SGAnchorLineType,
    AttachType,
    ObjectType,
    VariantType,
    VarType,

    FirstNumberType,
    BoolType = FirstNumberType,
    IntType,
    FloatType,
    NumberType
};
Type maxType(IR::Type left, IR::Type right);
bool isRealType(IR::Type type);
const char *typeName(IR::Type t);

struct ExprVisitor {
    virtual ~ExprVisitor() {}
    virtual void visitConst(Const *) {}
    virtual void visitString(String *) {}
    virtual void visitName(Name *) {}
    virtual void visitTemp(Temp *) {}
    virtual void visitClosure(Closure *) {}
    virtual void visitUnop(Unop *) {}
    virtual void visitBinop(Binop *) {}
    virtual void visitCall(Call *) {}
    virtual void visitNew(New *) {}
    virtual void visitSubscript(Subscript *) {}
    virtual void visitMember(Member *) {}
};

struct StmtVisitor {
    virtual ~StmtVisitor() {}
    virtual void visitExp(Exp *) {}
    virtual void visitEnter(Enter *) {}
    virtual void visitLeave(Leave *) {}
    virtual void visitMove(Move *) {}
    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *) {}
    virtual void visitRet(Ret *) {}
};

struct Expr {
    Type type;

    Expr(): type(InvalidType) {}
    virtual ~Expr() {}
    virtual void accept(ExprVisitor *) = 0;
    virtual Const *asConst() { return 0; }
    virtual String *asString() { return 0; }
    virtual Name *asName() { return 0; }
    virtual Temp *asTemp() { return 0; }
    virtual Closure *asClosure() { return 0; }
    virtual Unop *asUnop() { return 0; }
    virtual Binop *asBinop() { return 0; }
    virtual Call *asCall() { return 0; }
    virtual New *asNew() { return 0; }
    virtual Subscript *asSubscript() { return 0; }
    virtual Member *asMember() { return 0; }
    virtual void dump(QTextStream &out) = 0;
};

struct ExprList {
    Expr *expr;
    ExprList *next;

    void init(Expr *expr, ExprList *next = 0)
    {
        this->expr = expr;
        this->next = next;
    }
};

struct Const: Expr {
    double value;

    void init(Type type, double value)
    {
        this->type = type;
        this->value = value;
    }

    virtual void accept(ExprVisitor *v) { v->visitConst(this); }
    virtual Const *asConst() { return this; }

    virtual void dump(QTextStream &out);
};

struct String: Expr {
    const QString *value;

    void init(const QString *value)
    {
        this->type = StringType;
        this->value = value;
    }

    virtual void accept(ExprVisitor *v) { v->visitString(this); }
    virtual String *asString() { return this; }

    virtual void dump(QTextStream &out);
    static QString escape(const QString &s);
};

struct Name: Expr {
    const QString *id;
    quint32 line;
    quint32 column;

    void init(Type type, const QString *id, quint32 line, quint32 column);

    virtual void accept(ExprVisitor *v) { v->visitName(this); }
    virtual Name *asName() { return this; }

    virtual void dump(QTextStream &out);
};

struct Temp: Expr {
    unsigned index;

    void init(Type type, unsigned index)
    {
        this->type = type;
        this->index = index;
    }

    virtual void accept(ExprVisitor *v) { v->visitTemp(this); }
    virtual Temp *asTemp() { return this; }

    virtual void dump(QTextStream &out);
};

struct Closure: Expr {
    Function *value;

    void init(Type type, Function *value)
    {
        this->type = type;
        this->value = value;
    }

    virtual void accept(ExprVisitor *v) { v->visitClosure(this); }
    virtual Closure *asClosure() { return this; }

    virtual void dump(QTextStream &out);
};

struct Unop: Expr {
    AluOp op;
    Expr *expr;

    void init(AluOp op, Expr *expr)
    {
        this->type = this->typeForOp(op, expr);
        this->op = op;
        this->expr = expr;
    }

    virtual void accept(ExprVisitor *v) { v->visitUnop(this); }
    virtual Unop *asUnop() { return this; }

    virtual void dump(QTextStream &out);

private:
    static Type typeForOp(AluOp op, Expr *expr);
};

struct Binop: Expr {
    AluOp op;
    Expr *left;
    Expr *right;

    void init(AluOp op, Expr *left, Expr *right)
    {
        this->type = typeForOp(op, left, right);
        this->op = op;
        this->left = left;
        this->right = right;
    }

    virtual void accept(ExprVisitor *v) { v->visitBinop(this); }
    virtual Binop *asBinop() { return this; }

    virtual void dump(QTextStream &out);

    static Type typeForOp(AluOp op, Expr *left, Expr *right);
};

struct Call: Expr {
    Expr *base;
    ExprList *args;

    void init(Expr *base, ExprList *args)
    {
        this->type = typeForFunction(base);
        this->base = base;
        this->args = args;
    }

    Expr *onlyArgument() const {
        if (args && ! args->next)
            return args->expr;
        return 0;
    }

    virtual void accept(ExprVisitor *v) { v->visitCall(this); }
    virtual Call *asCall() { return this; }

    virtual void dump(QTextStream &out);

private:
    static Type typeForFunction(Expr *base);
};

struct New: Expr {
    Expr *base;
    ExprList *args;

    void init(Expr *base, ExprList *args)
    {
        this->type = typeForFunction(base);
        this->base = base;
        this->args = args;
    }

    Expr *onlyArgument() const {
        if (args && ! args->next)
            return args->expr;
        return 0;
    }

    virtual void accept(ExprVisitor *v) { v->visitNew(this); }
    virtual New *asNew() { return this; }

    virtual void dump(QTextStream &out);

private:
    static Type typeForFunction(Expr *base);
};

struct Subscript: Expr {
    Expr *base;
    Expr *index;

    void init(Expr *base, Expr *index)
    {
        this->type = typeForFunction(base);
        this->base = base;
        this->index = index;
    }

    virtual void accept(ExprVisitor *v) { v->visitSubscript(this); }
    virtual Subscript *asSubscript() { return this; }

    virtual void dump(QTextStream &out);

private:
    static Type typeForFunction(Expr *) { return IR::InvalidType; }
};

struct Member: Expr {
    Expr *base;
    const QString *name;

    void init(Expr *base, const QString *name)
    {
        this->type = typeForFunction(base);
        this->base = base;
        this->name = name;
    }

    virtual void accept(ExprVisitor *v) { v->visitMember(this); }
    virtual Member *asMember() { return this; }

    virtual void dump(QTextStream &out);

private:
    static Type typeForFunction(Expr *) { return IR::InvalidType; }
};

struct Stmt {
    enum Mode {
        HIR,
        MIR
    };

    struct Data {
        QVector<unsigned> uses;
        QVector<unsigned> defs;
        QBitArray liveIn;
        QBitArray liveOut;
    };

    Data *d;

    Stmt(): d(0) {}
    virtual ~Stmt() { Q_UNREACHABLE(); }
    virtual Stmt *asTerminator() { return 0; }

    virtual void accept(StmtVisitor *) = 0;
    virtual Exp *asExp() { return 0; }
    virtual Move *asMove() { return 0; }
    virtual Enter *asEnter() { return 0; }
    virtual Leave *asLeave() { return 0; }
    virtual Jump *asJump() { return 0; }
    virtual CJump *asCJump() { return 0; }
    virtual Ret *asRet() { return 0; }
    virtual void dump(QTextStream &out, Mode mode = HIR) = 0;

    void destroyData() {
        delete d;
        d = 0;
    }
};

struct Exp: Stmt {
    Expr *expr;

    void init(Expr *expr)
    {
        this->expr = expr;
    }

    virtual void accept(StmtVisitor *v) { v->visitExp(this); }
    virtual Exp *asExp() { return this; }

    virtual void dump(QTextStream &out, Mode);
};

struct Move: Stmt {
    Expr *target;
    Expr *source;
    AluOp op;

    void init(Expr *target, Expr *source, AluOp op)
    {
        this->target = target;
        this->source = source;
        this->op = op;
    }

    virtual void accept(StmtVisitor *v) { v->visitMove(this); }
    virtual Move *asMove() { return this; }

    virtual void dump(QTextStream &out, Mode);
};

struct Enter: Stmt {
    Expr *expr;

    void init(Expr *expr)
    {
        this->expr = expr;
    }

    virtual void accept(StmtVisitor *v) { v->visitEnter(this); }
    virtual Enter *asEnter() { return this; }

    virtual void dump(QTextStream &out, Mode);
};

struct Leave: Stmt {
    void init() {}

    virtual void accept(StmtVisitor *v) { v->visitLeave(this); }
    virtual Leave *asLeave() { return this; }

    virtual void dump(QTextStream &out, Mode);
};

struct Jump: Stmt {
    BasicBlock *target;

    void init(BasicBlock *target)
    {
        this->target = target;
    }

    virtual Stmt *asTerminator() { return this; }

    virtual void accept(StmtVisitor *v) { v->visitJump(this); }
    virtual Jump *asJump() { return this; }

    virtual void dump(QTextStream &out, Mode mode);
};

struct CJump: Stmt {
    Expr *cond;
    BasicBlock *iftrue;
    BasicBlock *iffalse;

    void init(Expr *cond, BasicBlock *iftrue, BasicBlock *iffalse)
    {
        this->cond = cond;
        this->iftrue = iftrue;
        this->iffalse = iffalse;
    }

    virtual Stmt *asTerminator() { return this; }

    virtual void accept(StmtVisitor *v) { v->visitCJump(this); }
    virtual CJump *asCJump() { return this; }

    virtual void dump(QTextStream &out, Mode mode);
};

struct Ret: Stmt {
    Expr *expr;
    Type type;

    void init(Expr *expr, Type type)
    {
        this->expr = expr;
        this->type = type;
    }

    virtual Stmt *asTerminator() { return this; }

    virtual void accept(StmtVisitor *v) { v->visitRet(this); }
    virtual Ret *asRet() { return this; }

    virtual void dump(QTextStream &out, Mode);
};

struct Module {
    MemoryPool pool;
    QVector<Function *> functions;

    Function *newFunction(const QString &name);
};

struct Function {
    Module *module;
    MemoryPool *pool;
    const QString *name;
    QVector<BasicBlock *> basicBlocks;
    int tempCount;
    QSet<QString> strings;
    QList<const QString *> formals;
    QList<const QString *> locals;
    void (*code)(VM::Context *);
    bool directEval;

    template <typename _Tp> _Tp *New() { return new (pool->allocate(sizeof(_Tp))) _Tp(); }

    Function(Module *module, const QString &name)
        : module(module)
        , pool(&module->pool)
        , tempCount(0)
        , code(0)
        , directEval(false)
    { this->name = newString(name); }

    ~Function();

    BasicBlock *newBasicBlock();
    const QString *newString(const QString &text);

    void RECEIVE(const QString &name) { formals.append(newString(name)); }
    void LOCAL(const QString &name) { locals.append(newString(name)); }

    inline BasicBlock *i(BasicBlock *block) { basicBlocks.append(block); return block; }

    void dump(QTextStream &out, Stmt::Mode mode = Stmt::HIR);
};

struct BasicBlock {
    Function *function;
    QVector<Stmt *> statements;
    QVector<BasicBlock *> in;
    QVector<BasicBlock *> out;
    QBitArray liveIn;
    QBitArray liveOut;
    int index;
    int offset;

    BasicBlock(Function *function): function(function), index(-1), offset(-1) {}
    ~BasicBlock() {}

    template <typename Instr> inline Instr i(Instr i) { statements.append(i); return i; }

    inline bool isEmpty() const {
        return statements.isEmpty();
    }

    inline Stmt *terminator() const {
        if (! statements.isEmpty() && statements.at(statements.size() - 1)->asTerminator() != 0)
            return statements.at(statements.size() - 1);
        return 0;
    }

    inline bool isTerminated() const {
        if (terminator() != 0)
            return true;
        return false;
    }

    unsigned newTemp();

    Temp *TEMP(unsigned index);

    Expr *CONST(Type type, double value);
    Expr *STRING(const QString *value);

    Name *NAME(const QString &id, quint32 line, quint32 column);

    Closure *CLOSURE(Function *function);

    Expr *UNOP(AluOp op, Expr *expr);
    Expr *BINOP(AluOp op, Expr *left, Expr *right);
    Expr *CALL(Expr *base, ExprList *args = 0);
    Expr *NEW(Expr *base, ExprList *args = 0);
    Expr *SUBSCRIPT(Expr *base, Expr *index);
    Expr *MEMBER(Expr *base, const QString *name);

    Stmt *EXP(Expr *expr);
    Stmt *ENTER(Expr *expr);
    Stmt *LEAVE();

    Stmt *MOVE(Expr *target, Expr *source, AluOp op = IR::OpInvalid);

    Stmt *JUMP(BasicBlock *target);
    Stmt *CJUMP(Expr *cond, BasicBlock *iftrue, BasicBlock *iffalse);
    Stmt *RET(Expr *expr, Type type);

    void dump(QTextStream &out, Stmt::Mode mode = Stmt::HIR);
};

} // end of namespace IR

} // end of namespace QQmlJS

QT_END_NAMESPACE

QT_END_HEADER

#endif // QV4IR_P_H
