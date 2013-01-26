/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
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

#include <private/qqmljsast_p.h>
#include <private/qqmljsengine_p.h>
#include <private/qqmlscript_p.h>
#include <private/qqmlimport_p.h>
#include <private/qqmlengine_p.h>
#include <private/qv4compiler_p.h>

#include <private/qqmlpool_p.h>
#include <QtCore/qvarlengtharray.h>

// #define DEBUG_IR_STRUCTURE

#ifdef CONST
#  undef CONST
#endif

QT_BEGIN_NAMESPACE

class QTextStream;
class QQmlType;

namespace QQmlJS {

namespace IR {

struct BasicBlock;
struct Function;

struct Stmt;
struct Expr;

// expressions
struct Const;
struct String;
struct Name;
struct Temp;
struct Unop;
struct Binop;
struct Call;

// statements
struct Exp;
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
    JSValueType,

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
    virtual void visitUnop(Unop *) {}
    virtual void visitBinop(Binop *) {}
    virtual void visitCall(Call *) {}
};

struct StmtVisitor {
    virtual ~StmtVisitor() {}
    virtual void visitExp(Exp *) {}
    virtual void visitMove(Move *) {}
    virtual void visitJump(Jump *) {}
    virtual void visitCJump(CJump *) {}
    virtual void visitRet(Ret *) {}
};

struct Expr: QQmlPool::POD {
    Type type;

    Expr(): type(InvalidType) {}
    virtual ~Expr() {}
    virtual void accept(ExprVisitor *) = 0;
    virtual Const *asConst() { return 0; }
    virtual String *asString() { return 0; }
    virtual Name *asName() { return 0; }
    virtual Temp *asTemp() { return 0; }
    virtual Unop *asUnop() { return 0; }
    virtual Binop *asBinop() { return 0; }
    virtual Call *asCall() { return 0; }
    virtual void dump(QTextStream &out) = 0;
};

struct ExprList: QQmlPool::POD {
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
    QStringRef value;

    void init(const QStringRef &value)
    {
        this->type = StringType;
        this->value = value;
    }

    virtual void accept(ExprVisitor *v) { v->visitString(this); }
    virtual String *asString() { return this; }

    virtual void dump(QTextStream &out);
    static QString escape(const QStringRef &s);
};

enum BuiltinSymbol {
    NoBuiltinSymbol,
    MathSinBultinFunction,
    MathCosBultinFunction,
    MathRoundBultinFunction,
    MathFloorBultinFunction,
    MathCeilBuiltinFunction,
    MathAbsBuiltinFunction,
    MathMaxBuiltinFunction,
    MathMinBuiltinFunction,

    MathPIBuiltinConstant
};

struct Name: Expr {
    enum Symbol {
        Unbound,
        IdObject,        // This is a load of a id object.  Storage will always be IdStorage
        AttachType,      // This is a load of an attached object
        SingletonObject, // This is a load of a singleton object
        Object,          // XXX what is this for?
        Property,        // This is a load of a regular property
        Slot             // XXX what is this for?
    };

    enum Storage {
        MemberStorage, // This is a property of a previously fetched object
        IdStorage,     // This is a load of a id object.  Symbol will always be IdObject
        RootStorage,   // This is a property of the root object
        ScopeStorage   // This is a property of the scope object
    };

    Name *base;
    const QString *id;
    Symbol symbol;
    union {
        void *ptr;
        const QQmlType *declarativeType;
        const QQmlScript::Object *idObject;
    };

    QQmlMetaObject meta;
    QQmlPropertyData *property;
    Storage storage;
    BuiltinSymbol builtin;
    quint16 line;
    quint16 column;

    void init(Name *base, Type type, const QString *id, Symbol symbol, quint16 line, quint16 column);

    inline bool is(Symbol s) const { return s == symbol; }
    inline bool isNot(Symbol s) const { return s != symbol; }

    virtual void accept(ExprVisitor *v) { v->visitName(this); }
    virtual Name *asName() { return this; }

    virtual void dump(QTextStream &out);
};

struct Temp: Expr {
    int index;

    void init(Type type, int index)
    {
        this->type = type;
        this->index = index;
    }

    virtual void accept(ExprVisitor *v) { v->visitTemp(this); }
    virtual Temp *asTemp() { return this; }

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

struct Stmt: QQmlPool::POD {
    enum Mode {
        HIR,
        MIR
    };

    virtual ~Stmt() {}
    virtual Stmt *asTerminator() { return 0; }

    virtual void accept(StmtVisitor *) = 0;
    virtual Exp *asExp() { return 0; }
    virtual Move *asMove() { return 0; }
    virtual Jump *asJump() { return 0; }
    virtual CJump *asCJump() { return 0; }
    virtual Ret *asRet() { return 0; }
    virtual void dump(QTextStream &out, Mode mode = HIR) = 0;
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
    bool isMoveForReturn;

    void init(Expr *target, Expr *source, bool isMoveForReturn)
    {
        this->target = target;
        this->source = source;
        this->isMoveForReturn = isMoveForReturn;
    }

    virtual void accept(StmtVisitor *v) { v->visitMove(this); }
    virtual Move *asMove() { return this; }

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
    quint16 line;
    quint16 column;

    void init(Expr *expr, Type type, quint16 line, quint16 column)
    {
        this->expr = expr;
        this->type = type;
        this->line = line;
        this->column = column;
    }

    virtual Stmt *asTerminator() { return this; }

    virtual void accept(StmtVisitor *v) { v->visitRet(this); }
    virtual Ret *asRet() { return this; }

    virtual void dump(QTextStream &out, Mode);
};

struct Function {
    QQmlPool *pool;
    QVarLengthArray<BasicBlock *, 8> basicBlocks;
    int tempCount;

    Function(QQmlPool *pool)
      : pool(pool), tempCount(0) {}

    virtual ~Function();

    BasicBlock *newBasicBlock();
    QString *newString(const QString &text);

    inline BasicBlock *i(BasicBlock *block) { basicBlocks.append(block); return block; }

    virtual void dump(QTextStream &out);
};

struct BasicBlock {
    Function *function;
    int index;
    int offset;
    QVarLengthArray<Stmt *, 32> statements;

    BasicBlock(Function *function, int index): function(function), index(index), offset(-1) {}
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

    Temp *TEMP(Type type, int index);
    Temp *TEMP(Type type);

    Expr *CONST(Type type, double value);
    Expr *STRING(const QStringRef &value);

    Name *NAME(const QString &id, quint16 line, quint16 column);
    Name *NAME(Name *base, const QString &id, quint16 line, quint16 column);
    Name *SYMBOL(Type type, const QString &id, const QQmlMetaObject &meta, QQmlPropertyData *property, Name::Storage storage, quint16 line, quint16 column);
    Name *SYMBOL(Name *base, Type type, const QString &id, const QQmlMetaObject &meta, QQmlPropertyData *property, quint16 line, quint16 column);
    Name *SYMBOL(Name *base, Type type, const QString &id, const QQmlMetaObject &meta, QQmlPropertyData *property, Name::Storage storage, quint16 line, quint16 column);
    Name *ID_OBJECT(const QString &id, const QQmlScript::Object *object, quint16 line, quint16 column);
    Name *ATTACH_TYPE(const QString &id, const QQmlType *attachType, Name::Storage storage, quint16 line, quint16 column);
    Name *SINGLETON_OBJECT(const QString &id, const QQmlMetaObject &meta, Name::Storage storage, quint16 line, quint16 column);

    Expr *UNOP(AluOp op, Expr *expr);
    Expr *BINOP(AluOp op, Expr *left, Expr *right);
    Expr *CALL(Expr *base, ExprList *args);

    Stmt *EXP(Expr *expr);
    Stmt *MOVE(Expr *target, Expr *source, bool = false);

    Stmt *JUMP(BasicBlock *target);
    Stmt *CJUMP(Expr *cond, BasicBlock *iftrue, BasicBlock *iffalse);
    Stmt *RET(Expr *expr, Type type, quint16 line, quint16 column);

    void dump(QTextStream &out);
};

#ifdef DEBUG_IR_STRUCTURE
struct IRDump : public ExprVisitor,
                public StmtVisitor 
{
public:
    IRDump();

    void expression(QQmlJS::IR::Expr *);
    void basicblock(QQmlJS::IR::BasicBlock *);
    void statement(QQmlJS::IR::Stmt *);
    void function(QQmlJS::IR::Function *);
protected:

    const char *indent();

    //
    // expressions
    //
    virtual void visitConst(QQmlJS::IR::Const *e);
    virtual void visitString(QQmlJS::IR::String *e);
    virtual void visitName(QQmlJS::IR::Name *e);
    virtual void visitTemp(QQmlJS::IR::Temp *e);
    virtual void visitUnop(QQmlJS::IR::Unop *e);
    virtual void visitBinop(QQmlJS::IR::Binop *e);
    virtual void visitCall(QQmlJS::IR::Call *e);

    //
    // statements
    //
    virtual void visitExp(QQmlJS::IR::Exp *s);
    virtual void visitMove(QQmlJS::IR::Move *s);
    virtual void visitJump(QQmlJS::IR::Jump *s);
    virtual void visitCJump(QQmlJS::IR::CJump *s);
    virtual void visitRet(QQmlJS::IR::Ret *s);

private:
    int indentSize;
    QByteArray indentData;
    void inc();
    void dec();
};
#endif

} // end of namespace IR

} // end of namespace QQmlJS

QT_END_NAMESPACE

#endif // QV4IR_P_H
