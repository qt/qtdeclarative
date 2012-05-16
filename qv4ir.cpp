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

#include "qv4ir_p.h"
#include <private/qqmljsast_p.h>

#include <QtCore/qtextstream.h>
#include <QtCore/qdebug.h>
#include <math.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace IR {

const char *typeName(Type t)
{
    switch (t) {
    case InvalidType: return "invalid";
    case UndefinedType: return "undefined";
    case NullType: return "null";
    case VoidType: return "void";
    case StringType: return "string";
    case UrlType: return "QUrl";
    case ColorType: return "QColor";
    case SGAnchorLineType: return "SGAnchorLine";
    case AttachType: return "AttachType";
    case ObjectType: return "object";
    case VariantType: return "variant";
    case VarType: return "var";
    case BoolType: return "bool";
    case IntType: return "int";
    case FloatType: return "float";
    case NumberType: return "number";
    default: return "invalid";
    }
}

inline bool isNumberType(IR::Type ty)
{
    return ty >= IR::FirstNumberType;
}

inline bool isStringType(IR::Type ty)
{
    return ty == IR::StringType || ty == IR::UrlType || ty == IR::ColorType;
}

IR::Type maxType(IR::Type left, IR::Type right)
{
    if (isStringType(left) && isStringType(right)) {
        // String promotions (url to string) are more specific than
        // identity conversions (AKA left == right). That's because
        // we want to ensure we convert urls to strings in binary
        // expressions.
        return IR::StringType;
    } else if (left == right)
        return left;
    else if (isNumberType(left) && isNumberType(right)) {
        IR::Type ty = qMax(left, right);
        return ty == FloatType ? NumberType : ty; // promote floats
    } else if ((isNumberType(left) && isStringType(right)) ||
             (isNumberType(right) && isStringType(left)))
        return IR::StringType;
    else
        return IR::InvalidType;
}

bool isRealType(IR::Type type)
{
    return type == IR::NumberType || type == IR::FloatType;
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
    case QQmlJS::IR::VoidType:
        out << "void";
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
            r += QLatin1String("\\n");
        else if (ch == QLatin1Char('\r'))
            r += QLatin1String("\\r");
        else if (ch == QLatin1Char('\\'))
            r += QLatin1String("\\\\");
        else if (ch == QLatin1Char('"'))
            r += QLatin1String("\\\"");
        else if (ch == QLatin1Char('\''))
            r += QLatin1String("\\'");
        else
            r += ch;
    }
    return r;
}

void Name::init(Type type, const QString *id, quint32 line, quint32 column)
{
    this->type = type;
    this->id = id;
    this->line = line;
    this->column = column;
}

void Name::dump(QTextStream &out)
{
    out << *id;
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

Type Unop::typeForOp(AluOp op, Expr *expr)
{
    switch (op) {
    case OpIfTrue: return BoolType;
    case OpNot: return BoolType;

    case OpUMinus:
    case OpUPlus:
    case OpCompl:
        return maxType(expr->type, NumberType);

    default:
        break;
    }

    return InvalidType;
}

void Binop::dump(QTextStream &out)
{
    left->dump(out);
    out << ' ' << opname(op) << ' ';
    right->dump(out);
}

Type Binop::typeForOp(AluOp op, Expr *left, Expr *right)
{
    if (! (left && right))
        return InvalidType;

    switch (op) {
    case OpInvalid:
        return InvalidType;

    // unary operators
    case OpIfTrue:
    case OpNot:
    case OpUMinus:
    case OpUPlus:
    case OpCompl:
        return InvalidType;

    // bit fields
    case OpBitAnd:
    case OpBitOr:
    case OpBitXor:
        return IntType;

    case OpAdd:
        if (left->type == StringType)
            return StringType;
        return NumberType;

    case OpSub:
    case OpMul:
    case OpDiv:
    case OpMod:
        return NumberType;

    case OpLShift:
    case OpRShift:
    case OpURShift:
        return IntType;

    case OpAnd:
    case OpOr:
        return BoolType;

    case OpGt:
    case OpLt:
    case OpGe:
    case OpLe:
    case OpEqual:
    case OpNotEqual:
    case OpStrictEqual:
    case OpStrictNotEqual:
        return BoolType;
    } // switch

    return InvalidType;
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

Type Call::typeForFunction(Expr *)
{
    return InvalidType;
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

Type New::typeForFunction(Expr *)
{
    return InvalidType;
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

Function *Module::newFunction(const QString &name)
{
    Function *f = new Function(this, name);
    functions.append(f);
    return f;
}

Function::~Function()
{
    qDeleteAll(basicBlocks);
}

const QString *Function::newString(const QString &text)
{
    return &*strings.insert(text);
}

BasicBlock *Function::newBasicBlock()
{
    return i(new BasicBlock(this));
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
    e->init(IR::InvalidType, index);
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

Name *BasicBlock::NAME(const QString &id, quint32 line, quint32 column)
{ 
    Name *e = function->New<Name>();
    e->init(InvalidType, function->newString(id), line, column);
    return e;
}

Closure *BasicBlock::CLOSURE(Function *function)
{
    Closure *clos = function->New<Closure>();
    clos->init(IR::InvalidType, function);
    return clos;
}

Expr *BasicBlock::UNOP(AluOp op, Expr *expr) 
{ 
    Unop *e = function->New<Unop>();
    e->init(op, expr);
    return e;
}

Expr *BasicBlock::BINOP(AluOp op, Expr *left, Expr *right)
{
    if (left && right) {
        if (Const *c1 = left->asConst()) {
            if (Const *c2 = right->asConst()) {
                const IR::Type ty = Binop::typeForOp(op, left, right);

                switch (op) {
                case OpAdd: return CONST(ty, c1->value + c2->value);
                case OpAnd: return CONST(ty, c1->value ? c2->value : 0);
                case OpBitAnd: return CONST(ty, int(c1->value) & int(c2->value));
                case OpBitOr: return CONST(ty, int(c1->value) | int(c2->value));
                case OpBitXor: return CONST(ty, int(c1->value) ^ int(c2->value));
                case OpDiv: return CONST(ty, c1->value / c2->value);
                case OpEqual: return CONST(ty, c1->value == c2->value);
                case OpGe: return CONST(ty, c1->value >= c2->value);
                case OpGt: return CONST(ty, c1->value > c2->value);
                case OpLe: return CONST(ty, c1->value <= c2->value);
                case OpLShift: return CONST(ty, int(c1->value) << int(c2->value));
                case OpLt: return CONST(ty, c1->value < c2->value);
                case OpMod: return CONST(ty, ::fmod(c1->value, c2->value));
                case OpMul: return CONST(ty, c1->value * c2->value);
                case OpNotEqual: return CONST(ty, c1->value != c2->value);
                case OpOr: return CONST(ty, c1->value ? c1->value : c2->value);
                case OpRShift: return CONST(ty, int(c1->value) >> int(c2->value));
                case OpStrictEqual: return CONST(ty, c1->value == c2->value);
                case OpStrictNotEqual: return CONST(ty, c1->value != c2->value);
                case OpSub: return CONST(ty, c1->value - c2->value);
                case OpURShift: return CONST(ty, unsigned(c1->value) >> int(c2->value));

                case OpIfTrue: // unary ops
                case OpNot:
                case OpUMinus:
                case OpUPlus:
                case OpCompl:
                case OpInvalid:
                    break;
                }
            }
        } else if (op == OpAdd) {
            if (String *s1 = left->asString()) {
                if (String *s2 = right->asString()) {
                    return STRING(function->newString(*s1->value + *s2->value));
                }
            }
        }
    }

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

Expr *BasicBlock::SUBSCRIPT(Expr *base, Expr *index)
{
    Subscript *e = function->New<Subscript>();
    e->init(base, index);
    return e;
}

Expr *BasicBlock::MEMBER(Expr *base, const QString *name)
{
    Member*e = function->New<Member>();
    e->init(base, name);
    return e;
}

Stmt *BasicBlock::EXP(Expr *expr)
{ 
    Exp *s = function->New<Exp>();
    s->init(expr);
    statements.append(s);
    return s;
}

Stmt *BasicBlock::ENTER(Expr *expr)
{
    Enter *s = function->New<Enter>();
    s->init(expr);
    statements.append(s);
    return s;
}

Stmt *BasicBlock::LEAVE()
{
    Leave *s = function->New<Leave>();
    s->init();
    statements.append(s);
    return s;
}

Stmt *BasicBlock::MOVE(Expr *target, Expr *source, AluOp op)
{ 
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
    return s;
}

Stmt *BasicBlock::CJUMP(Expr *cond, BasicBlock *iftrue, BasicBlock *iffalse) 
{
    if (isTerminated())
        return 0;

    CJump *s = function->New<CJump>();
    s->init(cond, iftrue, iffalse);
    statements.append(s);
    return s;
}

Stmt *BasicBlock::RET(Expr *expr, Type type)
{
    if (isTerminated())
        return 0;

    Ret *s = function->New<Ret>();
    s->init(expr, type);
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
