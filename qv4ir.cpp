/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
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

#include "qv4ir_p.h"
#include <private/qqmljsast_p.h>

#include <QtCore/qtextstream.h>
#include <QtCore/qdebug.h>
#include <cmath>
#include <cassert>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace IR {

const char *typeName(Type t)
{
    switch (t) {
    case UndefinedType: return "undefined";
    case NullType: return "null";
    case BoolType: return "bool";
    case NumberType: return "number";
    default: return "invalid";
    }
}

const char *opname(AluOp op)
{
    switch (op) {
    case OpInvalid: return "?";

    case OpIfTrue: return "(bool)";
    case OpNot: return "!";
    case OpUMinus: return "-";
    case OpUPlus: return "+";
    case OpCompl: return "~";
    case OpIncrement: return "++";
    case OpDecrement: return "--";

    case OpBitAnd: return "&";
    case OpBitOr: return "|";
    case OpBitXor: return "^";

    case OpAdd: return "+";
    case OpSub: return "-";
    case OpMul: return "*";
    case OpDiv: return "/";
    case OpMod: return "%";

    case OpLShift: return "<<";
    case OpRShift: return ">>";
    case OpURShift: return ">>>";

    case OpGt: return ">";
    case OpLt: return "<";
    case OpGe: return ">=";
    case OpLe: return "<=";
    case OpEqual: return "==";
    case OpNotEqual: return "!=";
    case OpStrictEqual: return "===";
    case OpStrictNotEqual: return "!==";

    case OpInstanceof: return "instanceof";
    case OpIn: return "in";

    case OpAnd: return "&&";
    case OpOr: return "||";

    default: return "?";

    } // switch
}

AluOp binaryOperator(int op)
{
    switch (static_cast<QSOperator::Op>(op)) {
    case QSOperator::Add: return OpAdd;
    case QSOperator::And: return OpAnd;
    case QSOperator::BitAnd: return OpBitAnd;
    case QSOperator::BitOr: return OpBitOr;
    case QSOperator::BitXor: return OpBitXor;
    case QSOperator::Div: return OpDiv;
    case QSOperator::Equal: return OpEqual;
    case QSOperator::Ge: return OpGe;
    case QSOperator::Gt: return OpGt;
    case QSOperator::Le: return OpLe;
    case QSOperator::LShift: return OpLShift;
    case QSOperator::Lt: return OpLt;
    case QSOperator::Mod: return OpMod;
    case QSOperator::Mul: return OpMul;
    case QSOperator::NotEqual: return OpNotEqual;
    case QSOperator::Or: return OpOr;
    case QSOperator::RShift: return OpRShift;
    case QSOperator::StrictEqual: return OpStrictEqual;
    case QSOperator::StrictNotEqual: return OpStrictNotEqual;
    case QSOperator::Sub: return OpSub;
    case QSOperator::URShift: return OpURShift;
    case QSOperator::InstanceOf: return OpInstanceof;
    case QSOperator::In: return OpIn;
    default: return OpInvalid;
    }
}

void Const::dump(QTextStream &out)
{
    switch (type) {
    case QQmlJS::IR::UndefinedType:
        out << "undefined";
        break;
    case QQmlJS::IR::NullType:
        out << "null";
        break;
    case QQmlJS::IR::BoolType:
        out << (value ? "true" : "false");
        break;
    default:
        out << QString::number(value, 'g', 16);
        break;
    }
}

void String::dump(QTextStream &out)
{
    out << '"' << escape(*value) << '"';
}

QString String::escape(const QString &s)
{
    QString r;
    for (int i = 0; i < s.length(); ++i) {
        const QChar ch = s.at(i);
        if (ch == QLatin1Char('\n'))
            r += QStringLiteral("\\n");
        else if (ch == QLatin1Char('\r'))
            r += QStringLiteral("\\r");
        else if (ch == QLatin1Char('\\'))
            r += QStringLiteral("\\\\");
        else if (ch == QLatin1Char('"'))
            r += QStringLiteral("\\\"");
        else if (ch == QLatin1Char('\''))
            r += QStringLiteral("\\'");
        else
            r += ch;
    }
    return r;
}

void RegExp::dump(QTextStream &out)
{
    char f[3];
    int i = 0;
    if (flags & RegExp_Global)
        f[i++] = 'g';
    if (flags & RegExp_IgnoreCase)
        f[i++] = 'i';
    if (flags & RegExp_Multiline)
        f[i++] = 'm';
    f[i] = 0;

    out << '/' << *value << '/' << f;
}

void Name::init(const QString *id, quint32 line, quint32 column)
{
    this->id = id;
    this->builtin = builtin_invalid;
    this->line = line;
    this->column = column;
}

void Name::init(Builtin builtin, quint32 line, quint32 column)
{
    this->id = 0;
    this->builtin = builtin;
    this->line = line;
    this->column = column;
}

static const char *builtin_to_string(Name::Builtin b)
{
    switch (b) {
    case Name::builtin_invalid:
        return "builtin_invalid";
    case Name::builtin_typeof:
        return "builtin_typeof";
    case Name::builtin_delete:
        return "builtin_delete";
    case Name::builtin_throw:
        return "builtin_throw";
    case Name::builtin_create_exception_handler:
        return "builtin_create_exception_handler";
    case Name::builtin_delete_exception_handler:
        return "builtin_delete_exception_handler";
    case Name::builtin_get_exception:
        return "builtin_get_exception";
    case IR::Name::builtin_foreach_iterator_object:
        return "builtin_foreach_iterator_object";
    case IR::Name::builtin_foreach_next_property_name:
        return "builtin_foreach_next_property_name";
    case IR::Name::builtin_push_with:
        return "builtin_push_with";
    case IR::Name::builtin_pop_with:
        return "builtin_pop_with";
    case IR::Name::builtin_declare_vars:
        return "builtin_declare_vars";
    case IR::Name::builtin_define_property:
        return "builtin_define_property";
    case IR::Name::builtin_define_getter_setter:
        return "builtin_define_getter_setter";
    }
    return "builtin_(###FIXME)";
};


void Name::dump(QTextStream &out)
{
    if (id)
        out << *id;
    else
        out << builtin_to_string(builtin);
}

void Temp::dump(QTextStream &out)
{
    if (index < 0) {
        out << '#' << -(index + 1); // negative and 1-based.
    } else {
        out << '%' << index; // temp
    }
}

void Closure::dump(QTextStream &out)
{
    QString name = value->name ? *value->name : QString();
    if (name.isEmpty())
        name.sprintf("%p", value);
    out << "closure(" << name << ')';
}

void Unop::dump(QTextStream &out)
{
    out << opname(op);
    expr->dump(out);
}

void Binop::dump(QTextStream &out)
{
    left->dump(out);
    out << ' ' << opname(op) << ' ';
    right->dump(out);
}

void Call::dump(QTextStream &out)
{
    base->dump(out);
    out << '(';
    for (ExprList *it = args; it; it = it->next) {
        if (it != args)
            out << ", ";
        it->expr->dump(out);
    }
    out << ')';
}

void New::dump(QTextStream &out)
{
    out << "new ";
    base->dump(out);
    out << '(';
    for (ExprList *it = args; it; it = it->next) {
        if (it != args)
            out << ", ";
        it->expr->dump(out);
    }
    out << ')';
}

void Subscript::dump(QTextStream &out)
{
    base->dump(out);
    out << '[';
    index->dump(out);
    out << ']';
}

void Member::dump(QTextStream &out)
{
    base->dump(out);
    out << '.' << *name;
}

void Exp::dump(QTextStream &out, Mode)
{
    out << "(void) ";
    expr->dump(out);
    out << ';';
}

void Enter::dump(QTextStream &out, Mode)
{
    out << "%enter(";
    expr->dump(out);
    out << ");";
}

void Leave::dump(QTextStream &out, Mode)
{
    out << "%leave";
    out << ';';
}

void Move::dump(QTextStream &out, Mode)
{
    target->dump(out);
    out << ' ';
    if (op != OpInvalid)
        out << opname(op);
    out << "= ";
//    if (source->type != target->type)
//        out << typeName(source->type) << "_to_" << typeName(target->type) << '(';
    source->dump(out);
//    if (source->type != target->type)
//        out << ')';
    out << ';';
}

void Jump::dump(QTextStream &out, Mode mode)
{
    Q_UNUSED(mode);
    out << "goto " << 'L' << target->index << ';';
}

void CJump::dump(QTextStream &out, Mode mode)
{
    Q_UNUSED(mode);
    out << "if (";
    cond->dump(out);
    if (mode == HIR)
        out << ") goto " << 'L' << iftrue->index << "; else goto " << 'L' << iffalse->index << ';';
    else
        out << ") goto " << 'L' << iftrue->index << ";";
}

void Ret::dump(QTextStream &out, Mode)
{
    out << "return";
    if (expr) {
        out << ' ';
        expr->dump(out);
    }
    out << ';';
}

Function *Module::newFunction(const QString &name, Function *outer)
{
    Function *f = new Function(this, name);
    functions.append(f);
    if (!outer) {
        assert(!rootFunction);
        rootFunction = f;
    } else {
        outer->nestedFunctions.append(f);
    }
    return f;
}

Module::~Module()
{
    foreach (Function *f, functions) {
        delete f;
    }
}

Function::~Function()
{
    // destroy the Stmt::Data blocks manually, because memory pool cleanup won't
    // call the Stmt destructors.
    foreach (IR::BasicBlock *b, basicBlocks)
        foreach (IR::Stmt *s, b->statements)
            s->destroyData();

    qDeleteAll(basicBlocks);
    pool = 0;
    module = 0;
}


const QString *Function::newString(const QString &text)
{
    return &*strings.insert(text);
}

BasicBlock *Function::newBasicBlock(BasicBlockInsertMode mode)
{
    BasicBlock *block = new BasicBlock(this);
    return mode == InsertBlock ? insertBasicBlock(block) : block;
}

void Function::dump(QTextStream &out, Stmt::Mode mode)
{
    QString n = name ? *name : QString();
    if (n.isEmpty())
        n.sprintf("%p", this);
    out << "function " << n << "() {" << endl;
    foreach (const QString *formal, formals)
        out << "\treceive " << *formal << ';' << endl;
    foreach (const QString *local, locals)
        out << "\tlocal " << *local << ';' << endl;
    foreach (BasicBlock *bb, basicBlocks)
        bb->dump(out, mode);
    out << '}' << endl;
}

unsigned BasicBlock::newTemp()
{
    return function->tempCount++;
}

Temp *BasicBlock::TEMP(int index)
{ 
    Temp *e = function->New<Temp>();
    e->init(index);
    return e;
}

Expr *BasicBlock::CONST(Type type, double value)
{ 
    Const *e = function->New<Const>();
    e->init(type, value);
    return e;
}

Expr *BasicBlock::STRING(const QString *value)
{
    String *e = function->New<String>();
    e->init(value);
    return e;
}

Expr *BasicBlock::REGEXP(const QString *value, int flags)
{
    RegExp *e = function->New<RegExp>();
    e->init(value, flags);
    return e;
}

Name *BasicBlock::NAME(const QString &id, quint32 line, quint32 column)
{ 
    Name *e = function->New<Name>();
    e->init(function->newString(id), line, column);
    return e;
}

Name *BasicBlock::NAME(Name::Builtin builtin, quint32 line, quint32 column)
{
    Name *e = function->New<Name>();
    e->init(builtin, line, column);
    return e;
}

Closure *BasicBlock::CLOSURE(Function *function)
{
    Closure *clos = function->New<Closure>();
    clos->init(function);
    return clos;
}

Expr *BasicBlock::UNOP(AluOp op, Temp *expr)
{ 
    Unop *e = function->New<Unop>();
    e->init(op, expr);
    return e;
}

Expr *BasicBlock::BINOP(AluOp op, Expr *left, Expr *right)
{
    Binop *e = function->New<Binop>();
    e->init(op, left, right);
    return e;
}

Expr *BasicBlock::CALL(Expr *base, ExprList *args)
{ 
    Call *e = function->New<Call>();
    e->init(base, args);
    return e;
}

Expr *BasicBlock::NEW(Expr *base, ExprList *args)
{
    New *e = function->New<New>();
    e->init(base, args);
    return e;
}

Expr *BasicBlock::SUBSCRIPT(Temp *base, Temp *index)
{
    Subscript *e = function->New<Subscript>();
    e->init(base, index);
    return e;
}

Expr *BasicBlock::MEMBER(Temp *base, const QString *name)
{
    Member*e = function->New<Member>();
    e->init(base, name);
    return e;
}

Stmt *BasicBlock::EXP(Expr *expr)
{ 
    if (isTerminated())
        return 0;

    Exp *s = function->New<Exp>();
    s->init(expr);
    statements.append(s);
    return s;
}

Stmt *BasicBlock::ENTER(Expr *expr)
{
    if (isTerminated())
        return 0;

    Enter *s = function->New<Enter>();
    s->init(expr);
    statements.append(s);
    return s;
}

Stmt *BasicBlock::LEAVE()
{
    if (isTerminated())
        return 0;

    Leave *s = function->New<Leave>();
    s->init();
    statements.append(s);
    return s;
}

Stmt *BasicBlock::MOVE(Expr *target, Expr *source, AluOp op)
{ 
    if (isTerminated())
        return 0;

    Move *s = function->New<Move>();
    s->init(target, source, op);
    statements.append(s);
    return s;
}

Stmt *BasicBlock::JUMP(BasicBlock *target) 
{
    if (isTerminated())
        return 0;

    Jump *s = function->New<Jump>();
    s->init(target);
    statements.append(s);

    assert(! out.contains(target));
    out.append(target);

    assert(! target->in.contains(this));
    target->in.append(this);

    return s;
}

Stmt *BasicBlock::CJUMP(Expr *cond, BasicBlock *iftrue, BasicBlock *iffalse) 
{
    if (isTerminated())
        return 0;

    if (iftrue == iffalse) {
        MOVE(TEMP(newTemp()), cond);
        return JUMP(iftrue);
    }

    CJump *s = function->New<CJump>();
    s->init(cond, iftrue, iffalse);
    statements.append(s);

    assert(! out.contains(iftrue));
    out.append(iftrue);

    assert(! iftrue->in.contains(this));
    iftrue->in.append(this);

    assert(! out.contains(iffalse));
    out.append(iffalse);

    assert(! iffalse->in.contains(this));
    iffalse->in.append(this);

    return s;
}

Stmt *BasicBlock::RET(Temp *expr)
{
    if (isTerminated())
        return 0;

    Ret *s = function->New<Ret>();
    s->init(expr);
    statements.append(s);
    return s;
}

void BasicBlock::dump(QTextStream &out, Stmt::Mode mode)
{
    out << 'L' << index << ':' << endl;
    foreach (Stmt *s, statements) {
        out << '\t';
        s->dump(out, mode);
        out << endl;
    }
}

} // end of namespace IR
} // end of namespace QQmlJS

QT_END_NAMESPACE
