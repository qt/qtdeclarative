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
#ifndef QV4JSIR_P_H
#define QV4JSIR_P_H

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

#include "private/qv4global_p.h"
#include <private/qqmljsmemorypool_p.h>
#include <private/qqmljsastfwd_p.h>
#include <private/qflagpointer_p.h>

#include <QtCore/QVector>
#include <QtCore/QString>
#include <QtCore/QBitArray>
#include <QtCore/qurl.h>
#include <qglobal.h>

#if defined(CONST) && defined(Q_OS_WIN)
# define QT_POP_CONST
# pragma push_macro("CONST")
# undef CONST // CONST conflicts with our own identifier
#endif

QT_BEGIN_NAMESPACE

class QTextStream;
class QQmlType;
class QQmlPropertyData;
class QQmlPropertyCache;
class QQmlEnginePrivate;

namespace QV4 {
struct ExecutionContext;
}

namespace QQmlJS {

inline bool isNegative(double d)
{
    uchar *dch = (uchar *)&d;
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        return (dch[0] & 0x80);
    else
        return (dch[7] & 0x80);

}

namespace V4IR {

struct BasicBlock;
struct Function;
struct Module;

struct Stmt;
struct Expr;

// expressions
struct Const;
struct String;
struct RegExp;
struct Name;
struct Temp;
struct Closure;
struct Convert;
struct Unop;
struct Binop;
struct Call;
struct New;
struct Subscript;
struct Member;

// statements
struct Exp;
struct Move;
struct Jump;
struct CJump;
struct Ret;
struct Phi;

// Flag pointer:
// * The first flag indicates whether the meta object is final.
//   If final, then none of its properties themselves need to
//   be final when considering for lookups in QML.
// * The second flag indicates whether enums should be included
//   in the lookup of properties or not. The default is false.
typedef QFlagPointer<QQmlPropertyCache> IRMetaObject;

enum AluOp {
    OpInvalid = 0,

    OpIfTrue,
    OpNot,
    OpUMinus,
    OpUPlus,
    OpCompl,
    OpIncrement,
    OpDecrement,

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

    OpInstanceof,
    OpIn,

    OpAnd,
    OpOr,

    LastAluOp = OpOr
};
AluOp binaryOperator(int op);
const char *opname(V4IR::AluOp op);

enum Type {
    UnknownType   = 0,

    MissingType   = 1 << 0,
    UndefinedType = 1 << 1,
    NullType      = 1 << 2,
    BoolType      = 1 << 3,

    SInt32Type    = 1 << 4,
    UInt32Type    = 1 << 5,
    DoubleType    = 1 << 6,
    NumberType    = SInt32Type | UInt32Type | DoubleType,

    StringType    = 1 << 7,
    QObjectType   = 1 << 8,
    VarType       = 1 << 9
};

inline bool strictlyEqualTypes(Type t1, Type t2)
{
    return t1 == t2 || ((t1 & NumberType) && (t2 & NumberType));
}

QString typeName(Type t);

struct ExprVisitor {
    virtual ~ExprVisitor() {}
    virtual void visitConst(Const *) = 0;
    virtual void visitString(String *) = 0;
    virtual void visitRegExp(RegExp *) = 0;
    virtual void visitName(Name *) = 0;
    virtual void visitTemp(Temp *) = 0;
    virtual void visitClosure(Closure *) = 0;
    virtual void visitConvert(Convert *) = 0;
    virtual void visitUnop(Unop *) = 0;
    virtual void visitBinop(Binop *) = 0;
    virtual void visitCall(Call *) = 0;
    virtual void visitNew(New *) = 0;
    virtual void visitSubscript(Subscript *) = 0;
    virtual void visitMember(Member *) = 0;
};

struct StmtVisitor {
    virtual ~StmtVisitor() {}
    virtual void visitExp(Exp *) = 0;
    virtual void visitMove(Move *) = 0;
    virtual void visitJump(Jump *) = 0;
    virtual void visitCJump(CJump *) = 0;
    virtual void visitRet(Ret *) = 0;
    virtual void visitPhi(Phi *) = 0;
};


struct MemberExpressionResolver
{
    typedef Type (*ResolveFunction)(QQmlEnginePrivate *engine, MemberExpressionResolver *resolver, Member *member);

    MemberExpressionResolver()
        : resolveMember(0), data(0), extraData(0), flags(0), isQObjectResolver(false) {}

    bool isValid() const { return !!resolveMember; }
    void clear() { *this = MemberExpressionResolver(); }

    ResolveFunction resolveMember;
    void *data; // Could be pointer to meta object, importNameSpace, etc. - depends on resolveMember implementation
    void *extraData; // Could be QQmlTypeNameCache
    unsigned int flags : 31;
    unsigned int isQObjectResolver; // neede for IR dump helpers
};

struct Expr {
    Type type;

    Expr(): type(UnknownType) {}
    virtual ~Expr() {}
    virtual void accept(ExprVisitor *) = 0;
    virtual bool isLValue() { return false; }
    virtual Const *asConst() { return 0; }
    virtual String *asString() { return 0; }
    virtual RegExp *asRegExp() { return 0; }
    virtual Name *asName() { return 0; }
    virtual Temp *asTemp() { return 0; }
    virtual Closure *asClosure() { return 0; }
    virtual Convert *asConvert() { return 0; }
    virtual Unop *asUnop() { return 0; }
    virtual Binop *asBinop() { return 0; }
    virtual Call *asCall() { return 0; }
    virtual New *asNew() { return 0; }
    virtual Subscript *asSubscript() { return 0; }
    virtual Member *asMember() { return 0; }
    virtual void dump(QTextStream &out) const = 0;
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

    virtual void dump(QTextStream &out) const;
};

struct String: Expr {
    const QString *value;

    void init(const QString *value)
    {
        this->value = value;
    }

    virtual void accept(ExprVisitor *v) { v->visitString(this); }
    virtual String *asString() { return this; }

    virtual void dump(QTextStream &out) const;
    static QString escape(const QString &s);
};

struct RegExp: Expr {
    // needs to be compatible with the flags in the lexer, and in RegExpObject
    enum Flags {
        RegExp_Global     = 0x01,
        RegExp_IgnoreCase = 0x02,
        RegExp_Multiline  = 0x04
    };

    const QString *value;
    int flags;

    void init(const QString *value, int flags)
    {
        this->value = value;
        this->flags = flags;
    }

    virtual void accept(ExprVisitor *v) { v->visitRegExp(this); }
    virtual RegExp *asRegExp() { return this; }

    virtual void dump(QTextStream &out) const;
};

struct Name: Expr {
    enum Builtin {
        builtin_invalid,
        builtin_typeof,
        builtin_delete,
        builtin_throw,
        builtin_rethrow,
        builtin_unwind_exception,
        builtin_push_catch_scope,
        builtin_foreach_iterator_object,
        builtin_foreach_next_property_name,
        builtin_push_with_scope,
        builtin_pop_scope,
        builtin_declare_vars,
        builtin_define_property,
        builtin_define_array,
        builtin_define_getter_setter,
        builtin_define_object_literal,
        builtin_setup_argument_object,
        builtin_convert_this_to_object,
        builtin_qml_id_array,
        builtin_qml_imported_scripts_object,
        builtin_qml_context_object,
        builtin_qml_scope_object
    };

    const QString *id;
    Builtin builtin;
    bool global : 1;
    bool qmlSingleton : 1;
    bool freeOfSideEffects : 1;
    quint32 line;
    quint32 column;

    void initGlobal(const QString *id, quint32 line, quint32 column);
    void init(const QString *id, quint32 line, quint32 column);
    void init(Builtin builtin, quint32 line, quint32 column);

    virtual void accept(ExprVisitor *v) { v->visitName(this); }
    virtual bool isLValue() { return true; }
    virtual Name *asName() { return this; }

    virtual void dump(QTextStream &out) const;
};

struct Temp: Expr {
    enum Kind {
        Formal = 0,
        ScopedFormal,
        Local,
        ScopedLocal,
        VirtualRegister,
        PhysicalRegister,
        StackSlot
    };

    unsigned index;
    unsigned scope : 27; // how many scopes outside the current one?
    unsigned kind  : 3;
    unsigned isArgumentsOrEval : 1;
    unsigned isReadOnly : 1;
    // Used when temp is used as base in member expression
    MemberExpressionResolver memberResolver;

    void init(unsigned kind, unsigned index, unsigned scope)
    {
        Q_ASSERT((kind == ScopedLocal && scope != 0) ||
                 (kind == ScopedFormal && scope != 0) ||
                 (scope == 0));

        this->kind = kind;
        this->index = index;
        this->scope = scope;
        this->isArgumentsOrEval = false;
        this->isReadOnly = false;
    }

    virtual void accept(ExprVisitor *v) { v->visitTemp(this); }
    virtual bool isLValue() { return !isReadOnly; }
    virtual Temp *asTemp() { return this; }

    virtual void dump(QTextStream &out) const;
};

inline bool operator==(const Temp &t1, const Temp &t2) Q_DECL_NOTHROW
{ return t1.index == t2.index && t1.scope == t2.scope && t1.kind == t2.kind && t1.type == t2.type; }

inline uint qHash(const Temp &t, uint seed = 0) Q_DECL_NOTHROW
{ return t.index ^ (t.kind | (t.scope << 3)) ^ seed; }

bool operator<(const Temp &t1, const Temp &t2) Q_DECL_NOTHROW;

struct Closure: Expr {
    int value; // index in _module->functions
    const QString *functionName;

    void init(int functionInModule, const QString *functionName)
    {
        this->value = functionInModule;
        this->functionName = functionName;
    }

    virtual void accept(ExprVisitor *v) { v->visitClosure(this); }
    virtual Closure *asClosure() { return this; }

    virtual void dump(QTextStream &out) const;
};

struct Convert: Expr {
    Expr *expr;

    void init(Expr *expr, Type type)
    {
        this->expr = expr;
        this->type = type;
    }

    virtual void accept(ExprVisitor *v) { v->visitConvert(this); }
    virtual Convert *asConvert() { return this; }

    virtual void dump(QTextStream &out) const;
};

struct Unop: Expr {
    AluOp op;
    Expr *expr;

    void init(AluOp op, Expr *expr)
    {
        this->op = op;
        this->expr = expr;
    }

    virtual void accept(ExprVisitor *v) { v->visitUnop(this); }
    virtual Unop *asUnop() { return this; }

    virtual void dump(QTextStream &out) const;
};

struct Binop: Expr {
    AluOp op;
    Expr *left; // Temp or Const
    Expr *right; // Temp or Const

    void init(AluOp op, Expr *left, Expr *right)
    {
        this->op = op;
        this->left = left;
        this->right = right;
    }

    virtual void accept(ExprVisitor *v) { v->visitBinop(this); }
    virtual Binop *asBinop() { return this; }

    virtual void dump(QTextStream &out) const;
};

struct Call: Expr {
    Expr *base; // Name, Member, Temp
    ExprList *args; // List of Temps

    void init(Expr *base, ExprList *args)
    {
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

    virtual void dump(QTextStream &out) const;
};

struct New: Expr {
    Expr *base; // Name, Member, Temp
    ExprList *args; // List of Temps

    void init(Expr *base, ExprList *args)
    {
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

    virtual void dump(QTextStream &out) const;
};

struct Subscript: Expr {
    Expr *base;
    Expr *index;

    void init(Expr *base, Expr *index)
    {
        this->base = base;
        this->index = index;
    }

    virtual void accept(ExprVisitor *v) { v->visitSubscript(this); }
    virtual bool isLValue() { return true; }
    virtual Subscript *asSubscript() { return this; }

    virtual void dump(QTextStream &out) const;
};

struct Member: Expr {
    // Used for property dependency tracking
    enum MemberKind {
        UnspecifiedMember,
        MemberOfEnum,
        MemberOfQmlScopeObject,
        MemberOfQmlContextObject
    };

    Expr *base;
    const QString *name;
    QQmlPropertyData *property;
    int attachedPropertiesIdOrEnumValue; // depending on kind
    uchar memberIsEnum : 1;
    uchar freeOfSideEffects : 1;

    // This is set for example for for QObject properties. All sorts of extra behavior
    // is defined when writing to them, for example resettable properties are reset
    // when writing undefined to them, and an exception is thrown when they're missing
    // a reset function. And then there's also Qt.binding().
    uchar inhibitTypeConversionOnWrite: 1;

    uchar kind: 3; // MemberKind

    void setEnumValue(int value) {
        kind = MemberOfEnum;
        attachedPropertiesIdOrEnumValue = value;
    }

    void setAttachedPropertiesId(int id) {
        Q_ASSERT(kind != MemberOfEnum);
        attachedPropertiesIdOrEnumValue = id;
    }

    void init(Expr *base, const QString *name, QQmlPropertyData *property = 0, uchar kind = UnspecifiedMember, int attachedPropertiesIdOrEnumValue = 0)
    {
        this->base = base;
        this->name = name;
        this->property = property;
        this->attachedPropertiesIdOrEnumValue = attachedPropertiesIdOrEnumValue;
        this->memberIsEnum = false;
        this->freeOfSideEffects = false;
        this->inhibitTypeConversionOnWrite = property != 0;
        this->kind = kind;
    }

    virtual void accept(ExprVisitor *v) { v->visitMember(this); }
    virtual bool isLValue() { return true; }
    virtual Member *asMember() { return this; }

    virtual void dump(QTextStream &out) const;
};

struct Stmt {
    enum Mode {
        HIR,
        MIR
    };

    struct Data {
        QVector<Expr *> incoming; // used by Phi nodes
    };

    Data *d;
    int id;
    AST::SourceLocation location;

    Stmt(): d(0), id(-1) {}
    virtual ~Stmt()
    {
#ifdef Q_CC_MSVC
         // MSVC complains about potential memory leaks if a destructor never returns.
#else
        Q_UNREACHABLE();
#endif
    }
    virtual Stmt *asTerminator() { return 0; }

    virtual void accept(StmtVisitor *) = 0;
    virtual Exp *asExp() { return 0; }
    virtual Move *asMove() { return 0; }
    virtual Jump *asJump() { return 0; }
    virtual CJump *asCJump() { return 0; }
    virtual Ret *asRet() { return 0; }
    virtual Phi *asPhi() { return 0; }
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
    Expr *target; // LHS - Temp, Name, Member or Subscript
    Expr *source;
    bool swap;

    void init(Expr *target, Expr *source)
    {
        this->target = target;
        this->source = source;
        this->swap = false;
    }

    virtual void accept(StmtVisitor *v) { v->visitMove(this); }
    virtual Move *asMove() { return this; }

    virtual void dump(QTextStream &out, Mode mode = HIR);
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
    Expr *cond; // Temp, Binop
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

    void init(Expr *expr)
    {
        this->expr = expr;
    }

    virtual Stmt *asTerminator() { return this; }

    virtual void accept(StmtVisitor *v) { v->visitRet(this); }
    virtual Ret *asRet() { return this; }

    virtual void dump(QTextStream &out, Mode);
};

struct Phi: Stmt {
    Temp *targetTemp;

    virtual void accept(StmtVisitor *v) { v->visitPhi(this); }
    virtual Phi *asPhi() { return this; }

    virtual void dump(QTextStream &out, Mode mode);
};

struct Q_QML_EXPORT Module {
    MemoryPool pool;
    QVector<Function *> functions;
    Function *rootFunction;
    QString fileName;
    bool isQmlModule; // implies rootFunction is always 0
    bool debugMode;

    Function *newFunction(const QString &name, Function *outer);

    Module(bool debugMode)
        : rootFunction(0)
        , isQmlModule(false)
        , debugMode(debugMode)
    {}
    ~Module();

    void setFileName(const QString &name);
};

// Map from meta property index (existence implies dependency) to notify signal index
typedef QHash<int, int> PropertyDependencyMap;

struct Function {
    Module *module;
    MemoryPool *pool;
    const QString *name;
    QVector<BasicBlock *> basicBlocks;
    int tempCount;
    int maxNumberOfArguments;
    QSet<QString> strings;
    QList<const QString *> formals;
    QList<const QString *> locals;
    QVector<Function *> nestedFunctions;
    Function *outer;

    int insideWithOrCatch;

    uint hasDirectEval: 1;
    uint usesArgumentsObject : 1;
    uint usesThis : 1;
    uint isStrict: 1;
    uint isNamedExpression : 1;
    uint hasTry: 1;
    uint hasWith: 1;
    uint unused : 25;

    // Location of declaration in source code (-1 if not specified)
    int line;
    int column;

    // Qml extension:
    QSet<int> idObjectDependencies;
    PropertyDependencyMap contextObjectPropertyDependencies;
    PropertyDependencyMap scopeObjectPropertyDependencies;

    template <typename _Tp> _Tp *New() { return new (pool->allocate(sizeof(_Tp))) _Tp(); }

    Function(Module *module, Function *outer, const QString &name)
        : module(module)
        , pool(&module->pool)
        , tempCount(0)
        , maxNumberOfArguments(0)
        , outer(outer)
        , insideWithOrCatch(0)
        , hasDirectEval(false)
        , usesArgumentsObject(false)
        , isStrict(false)
        , isNamedExpression(false)
        , hasTry(false)
        , hasWith(false)
        , unused(0)
        , line(-1)
        , column(-1)
    { this->name = newString(name); }

    ~Function();

    enum BasicBlockInsertMode {
        InsertBlock,
        DontInsertBlock
    };

    BasicBlock *newBasicBlock(BasicBlock *containingLoop, BasicBlock *catchBlock, BasicBlockInsertMode mode = InsertBlock);
    const QString *newString(const QString &text);

    void RECEIVE(const QString &name) { formals.append(newString(name)); }
    void LOCAL(const QString &name) { locals.append(newString(name)); }

    inline BasicBlock *insertBasicBlock(BasicBlock *block) { basicBlocks.append(block); return block; }

    void dump(QTextStream &out, Stmt::Mode mode = Stmt::HIR);

    void removeSharedExpressions();

    int indexOfArgument(const QStringRef &string) const;

    bool variablesCanEscape() const
    { return hasDirectEval || !nestedFunctions.isEmpty() || module->debugMode; }
};

struct BasicBlock {
    Function *function;
    BasicBlock *catchBlock;
    QVector<Stmt *> statements;
    QVector<BasicBlock *> in;
    QVector<BasicBlock *> out;
    QBitArray liveIn;
    QBitArray liveOut;
    int index;
    bool isExceptionHandler;
    AST::SourceLocation nextLocation;

    BasicBlock(Function *function, BasicBlock *containingLoop, BasicBlock *catcher)
        : function(function)
        , catchBlock(catcher)
        , index(-1)
        , isExceptionHandler(false)
        , _containingGroup(containingLoop)
        , _groupStart(false)
    {}
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

    Temp *TEMP(unsigned kind);
    Temp *ARG(unsigned index, unsigned scope);
    Temp *LOCAL(unsigned index, unsigned scope);

    Expr *CONST(Type type, double value);
    Expr *STRING(const QString *value);
    Expr *REGEXP(const QString *value, int flags);

    Name *NAME(const QString &id, quint32 line, quint32 column);
    Name *NAME(Name::Builtin builtin, quint32 line, quint32 column);

    Name *GLOBALNAME(const QString &id, quint32 line, quint32 column);

    Closure *CLOSURE(int functionInModule);

    Expr *CONVERT(Expr *expr, Type type);
    Expr *UNOP(AluOp op, Expr *expr);
    Expr *BINOP(AluOp op, Expr *left, Expr *right);
    Expr *CALL(Expr *base, ExprList *args = 0);
    Expr *NEW(Expr *base, ExprList *args = 0);
    Expr *SUBSCRIPT(Expr *base, Expr *index);
    Expr *MEMBER(Expr *base, const QString *name, QQmlPropertyData *property = 0, uchar kind = Member::UnspecifiedMember, int attachedPropertiesIdOrEnumValue = 0);

    Stmt *EXP(Expr *expr);

    Stmt *MOVE(Expr *target, Expr *source);

    Stmt *JUMP(BasicBlock *target);
    Stmt *CJUMP(Expr *cond, BasicBlock *iftrue, BasicBlock *iffalse);
    Stmt *RET(Temp *expr);

    void dump(QTextStream &out, Stmt::Mode mode = Stmt::HIR);

    void appendStatement(Stmt *statement);

    BasicBlock *containingGroup() const
    { return _containingGroup; }

    bool isGroupStart() const
    { return _groupStart; }

    void markAsGroupStart()
    { _groupStart = true; }

private:
    BasicBlock *_containingGroup;
    bool _groupStart;
};

class CloneExpr: protected V4IR::ExprVisitor
{
public:
    explicit CloneExpr(V4IR::BasicBlock *block = 0);

    void setBasicBlock(V4IR::BasicBlock *block);

    template <typename _Expr>
    _Expr *operator()(_Expr *expr)
    {
        return clone(expr);
    }

    template <typename _Expr>
    _Expr *clone(_Expr *expr)
    {
        Expr *c = expr;
        qSwap(cloned, c);
        expr->accept(this);
        qSwap(cloned, c);
        return static_cast<_Expr *>(c);
    }

    static Const *cloneConst(Const *c, Function *f)
    {
        Const *newConst = f->New<Const>();
        newConst->init(c->type, c->value);
        return newConst;
    }

    static Name *cloneName(Name *n, Function *f)
    {
        Name *newName = f->New<Name>();
        newName->type = n->type;
        newName->id = n->id;
        newName->builtin = n->builtin;
        newName->global = n->global;
        newName->qmlSingleton = n->qmlSingleton;
        newName->freeOfSideEffects = n->freeOfSideEffects;
        newName->line = n->line;
        newName->column = n->column;
        return newName;
    }

    static Temp *cloneTemp(Temp *t, Function *f)
    {
        Temp *newTemp = f->New<Temp>();
        newTemp->init(t->kind, t->index, t->scope);
        newTemp->type = t->type;
        newTemp->memberResolver = t->memberResolver;
        return newTemp;
    }

protected:
    V4IR::ExprList *clone(V4IR::ExprList *list);

    virtual void visitConst(Const *);
    virtual void visitString(String *);
    virtual void visitRegExp(RegExp *);
    virtual void visitName(Name *);
    virtual void visitTemp(Temp *);
    virtual void visitClosure(Closure *);
    virtual void visitConvert(Convert *);
    virtual void visitUnop(Unop *);
    virtual void visitBinop(Binop *);
    virtual void visitCall(Call *);
    virtual void visitNew(New *);
    virtual void visitSubscript(Subscript *);
    virtual void visitMember(Member *);

private:
    V4IR::BasicBlock *block;
    V4IR::Expr *cloned;
};

} // end of namespace IR

} // end of namespace QQmlJS

QT_END_NAMESPACE

#if defined(QT_POP_CONST)
# pragma pop_macro("CONST") // Restore peace
# undef QT_POP_CONST
#endif

#endif // QV4IR_P_H
