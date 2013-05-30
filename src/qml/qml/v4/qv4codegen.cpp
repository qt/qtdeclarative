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

#include "qv4codegen_p.h"
#include "qv4util_p.h"
#include "qv4debugging_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QSet>
#include <QtCore/QBuffer>
#include <QtCore/QBitArray>
#include <QtCore/QStack>
#include <private/qqmljsast_p.h>
#include <qv4runtime_p.h>
#include <qv4context_p.h>
#include <cmath>
#include <iostream>
#include <cassert>

#ifdef CONST
#undef CONST
#endif

using namespace QQmlJS;
using namespace AST;

namespace {
QTextStream qout(stdout, QIODevice::WriteOnly);

void dfs(V4IR::BasicBlock *block,
         QSet<V4IR::BasicBlock *> *V,
         QVector<V4IR::BasicBlock *> *blocks)
{
    if (! V->contains(block)) {
        V->insert(block);

        foreach (V4IR::BasicBlock *succ, block->out)
            dfs(succ, V, blocks);

        blocks->append(block);
    }
}

struct ComputeUseDef: V4IR::StmtVisitor, V4IR::ExprVisitor
{
    V4IR::Function *_function;
    V4IR::Stmt *_stmt;

    ComputeUseDef(V4IR::Function *function)
        : _function(function)
        , _stmt(0) {}

    void operator()(V4IR::Stmt *s) {
        assert(! s->d);
        s->d = new V4IR::Stmt::Data;
        qSwap(_stmt, s);
        _stmt->accept(this);
        qSwap(_stmt, s);
    }

    virtual void visitConst(V4IR::Const *) {}
    virtual void visitString(V4IR::String *) {}
    virtual void visitRegExp(V4IR::RegExp *) {}
    virtual void visitName(V4IR::Name *) {}
    virtual void visitClosure(V4IR::Closure *) {}
    virtual void visitUnop(V4IR::Unop *e) { e->expr->accept(this); }
    virtual void visitBinop(V4IR::Binop *e) { e->left->accept(this); e->right->accept(this); }
    virtual void visitSubscript(V4IR::Subscript *e) { e->base->accept(this); e->index->accept(this); }
    virtual void visitMember(V4IR::Member *e) { e->base->accept(this); }
    virtual void visitExp(V4IR::Exp *s) { s->expr->accept(this); }
    virtual void visitEnter(V4IR::Enter *) {}
    virtual void visitLeave(V4IR::Leave *) {}
    virtual void visitJump(V4IR::Jump *) {}
    virtual void visitCJump(V4IR::CJump *s) { s->cond->accept(this); }
    virtual void visitRet(V4IR::Ret *s) { s->expr->accept(this); }
    virtual void visitTry(V4IR::Try *t) {
        if (! _stmt->d->defs.contains(t->exceptionVar->index))
            _stmt->d->defs.append(t->exceptionVar->index);
    }

    virtual void visitTemp(V4IR::Temp *e) {
        if (e->index < 0 || e->scope != 0)
            return;

        if (! _stmt->d->uses.contains(e->index))
            _stmt->d->uses.append(e->index);
    }

    virtual void visitCall(V4IR::Call *e) {
        e->base->accept(this);
        for (V4IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitNew(V4IR::New *e) {
        e->base->accept(this);
        for (V4IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitMove(V4IR::Move *s) {
        if (V4IR::Temp *t = s->target->asTemp()) {
            if (t->index >= 0 && t->scope == 0) // only collect unscoped locals and temps
                if (! _stmt->d->defs.contains(t->index))
                    _stmt->d->defs.append(t->index);
        } else {
            // source was not a temp, but maybe a sub-expression has a temp
            // (e.g. base expressions for subscripts/member-access),
            // so visit it.
            s->target->accept(this);
        }
        // whatever the target expr was, always visit the source expr to collect
        // temps there.
        s->source->accept(this);
    }
};

void liveness(V4IR::Function *function)
{
    QSet<V4IR::BasicBlock *> V;
    QVector<V4IR::BasicBlock *> blocks;

    ComputeUseDef computeUseDef(function);
    foreach (V4IR::BasicBlock *block, function->basicBlocks) {
        foreach (V4IR::Stmt *s, block->statements)
            computeUseDef(s);
    }

    dfs(function->basicBlocks.at(0), &V, &blocks);

    bool changed;
    do {
        changed = false;

        foreach (V4IR::BasicBlock *block, blocks) {
            const QBitArray previousLiveIn = block->liveIn;
            const QBitArray previousLiveOut = block->liveOut;
            QBitArray live(function->tempCount);
            foreach (V4IR::BasicBlock *succ, block->out)
                live |= succ->liveIn;
            block->liveOut = live;
            for (int i = block->statements.size() - 1; i != -1; --i) {
                V4IR::Stmt *s = block->statements.at(i);
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

static inline bool isDeadAssignment(V4IR::Stmt *stmt, int localCount)
{
    V4IR::Move *move = stmt->asMove();
    if (!move || move->op != V4IR::OpInvalid)
        return false;
    V4IR::Temp *target = move->target->asTemp();
    if (!target)
        return false;
    if (target->scope || target->index < localCount)
        return false;

    if (V4IR::Name *n = move->source->asName()) {
        if (*n->id != QStringLiteral("this"))
            return false;
    } else if (!move->source->asConst() && !move->source->asTemp()) {
        return false;
    }

    return !stmt->d->liveOut.at(target->index);
}

void removeDeadAssignments(V4IR::Function *function)
{
    const int localCount = function->locals.size();
    foreach (V4IR::BasicBlock *bb, function->basicBlocks) {
        QVector<V4IR::Stmt *> &statements = bb->statements;
        for (int i = 0; i < statements.size(); ) {
            //qout<<"removeDeadAssignments: considering ";statements.at(i)->dump(qout);qout<<"\n";qout.flush();
            if (isDeadAssignment(statements.at(i), localCount)) {
                statements.at(i)->destroyData();
                statements.remove(i);
            } else
                ++i;
        }
    }
}

void removeUnreachableBlocks(V4IR::Function *function)
{
    // TODO: change this to use a worklist.
    // FIXME: actually, use SSA and then re-implement it.

    for (int i = 1, ei = function->basicBlocks.size(); i != ei; ++i) {
        V4IR::BasicBlock *bb = function->basicBlocks[i];
        if (bb->in.isEmpty()) {
            function->basicBlocks.remove(i);
            foreach (V4IR::BasicBlock *outBB, bb->out) {
                int idx = outBB->in.indexOf(bb);
                if (idx != -1)
                    outBB->in.remove(idx);
            }
            removeUnreachableBlocks(function);
            return;
        }
    }
}

class ConstantPropagation: public V4IR::StmtVisitor, public V4IR::ExprVisitor
{
    struct Value {
        enum Type {
            InvalidType = 0,
            UndefinedType,
            NullType,
            BoolType,
            NumberType,
            ThisType,
            StringType
        } type;

        union {
            double numberValue;
            V4IR::String *stringValue;
        };

        Value()
            : type(InvalidType), stringValue(0)
        {}

        explicit Value(V4IR::String *str)
            : type(StringType), stringValue(str)
        {}

        explicit Value(Type t)
            : type(t), stringValue(0)
        {}

        Value(Type t, double val)
            : type(t), numberValue(val)
        {}

        bool isValid() const
        { return type != InvalidType; }

        bool operator<(const Value &other) const
        {
            if (type < other.type)
                return true;
            if (type == Value::NumberType && other.type == Value::NumberType) {
                if (numberValue == 0 && other.numberValue == 0)
                    return isNegative(numberValue) && !isNegative(other.numberValue);
                else
                    return numberValue < other.numberValue;
            }
            if (type == Value::BoolType && other.type == Value::BoolType)
                return numberValue < other.numberValue;
            if (type == Value::StringType && other.type == Value::StringType)
                return *stringValue->value < *other.stringValue->value;
            return false;
        }

        bool operator==(const Value &other) const
        {
            if (type != other.type)
                return false;
            if (type == Value::NumberType && other.type == Value::NumberType) {
                if (numberValue == 0 && other.numberValue == 0)
                    return isNegative(numberValue) == isNegative(other.numberValue);
                else
                    return numberValue == other.numberValue;
            }
            if (type == Value::BoolType && other.type == Value::BoolType)
                return numberValue == other.numberValue;
            if (type == Value::StringType && other.type == Value::StringType)
                return *stringValue->value == *other.stringValue->value;
            return false;
        }
    };

public:
    void run(V4IR::Function *function)
    {
        if (function->hasTry)
            return;
        localCount = function->locals.size();
        if (function->hasWith) {
            thisTemp = -1;
        } else {
            V4IR::BasicBlock *entryBlock = function->basicBlocks.at(0);
            thisTemp = entryBlock->newTemp();
            V4IR::Move *fetchThis = function->New<V4IR::Move>();
            fetchThis->init(entryBlock->TEMP(thisTemp),
                            entryBlock->NAME(QStringLiteral("this"), 0, 0),
                            V4IR::OpInvalid);
            entryBlock->statements.prepend(fetchThis);
        }

        foreach (V4IR::BasicBlock *block, function->basicBlocks) {
//            qDebug()<<"--- Starting with BB"<<block->index;
            reset();
            QVector<V4IR::Stmt *> &statements = block->statements;
            foreach (V4IR::Stmt *stmt, statements) {
//                qout<<"*** ";stmt->dump(qout);qout<<"\n";qout.flush();
                stmt->accept(this);
            }
        }
    }

protected:
    virtual void visitConst(V4IR::Const *) {}
    virtual void visitString(V4IR::String *) {}
    virtual void visitRegExp(V4IR::RegExp *) {}
    virtual void visitName(V4IR::Name *) {}
    virtual void visitClosure(V4IR::Closure *) {}
    virtual void visitUnop(V4IR::Unop *e) { e->expr->accept(this); }
    virtual void visitBinop(V4IR::Binop *e) { e->left->accept(this); e->right->accept(this); }
    virtual void visitSubscript(V4IR::Subscript *e) { e->base->accept(this); e->index->accept(this); }
    virtual void visitMember(V4IR::Member *e) { e->base->accept(this); }
    virtual void visitExp(V4IR::Exp *s) { s->expr->accept(this); }
    virtual void visitEnter(V4IR::Enter *) {}
    virtual void visitLeave(V4IR::Leave *) {}
    virtual void visitJump(V4IR::Jump *) {}
    virtual void visitCJump(V4IR::CJump *s) { s->cond->accept(this); }
    virtual void visitRet(V4IR::Ret *s) { s->expr->accept(this); }
    virtual void visitTry(V4IR::Try *) {}

    virtual void visitCall(V4IR::Call *e) {
        e->base->accept(this);
        for (V4IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitNew(V4IR::New *e) {
        e->base->accept(this);
        for (V4IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitTemp(V4IR::Temp *e) {
        if (e->scope)
            return;

        const int replacement = tempReplacement.value(e->index, -1);
        if (replacement != -1) {
//            qDebug() << "+++ Replacing" << e->index << "with" << replacement;
            e->index = replacement;
        }
    }

    virtual void visitMove(V4IR::Move *s) {
        V4IR::Temp *targetTemp = s->target->asTemp();
        if (targetTemp && targetTemp->index >= localCount && !targetTemp->scope) {
            if (s->op == V4IR::OpInvalid) {
                if (V4IR::Name *n = s->source->asName()) {
                    if (thisTemp != -1) {
                        if (*n->id == QStringLiteral("this")) {
                            check(targetTemp->index, Value(Value::ThisType));
                            return;
                        }
                    }
                } else if (V4IR::Const *c = s->source->asConst()) {
                    Value value;
                    switch (c->type) {
                    case V4IR::UndefinedType: value.type = Value::UndefinedType; break;
                    case V4IR::NullType: value.type = Value::NullType; break;
                    case V4IR::BoolType: value.type = Value::BoolType; value.numberValue = c->value == 0 ? 0 : 1; break;
                    case V4IR::NumberType: value.type = Value::NumberType; value.numberValue = c->value; break;
                    default: Q_ASSERT("unknown const type"); return;
                    }
                    check(targetTemp->index, value);
                    return;
                } else if (V4IR::String *str = s->source->asString()) {
                    check(targetTemp->index, Value(str));
                    return;
                }
            }
            invalidate(targetTemp->index, Value());
        } else {
            s->target->accept(this);
        }

        s->source->accept(this);
    }

    void invalidate(int &targetTempIndex, const Value &value)
    {
        QMap<int, Value>::iterator it = valueForTemp.find(targetTempIndex);
        if (it != valueForTemp.end()) {
            if (it.value() == value)
                return;
            tempForValue.remove(it.value());
            valueForTemp.erase(it);
        }

        QMap<int, int>::iterator it2 = tempReplacement.find(targetTempIndex);
        if (it2 != tempReplacement.end()) {
            tempReplacement.erase(it2);
        }
    }

    void check(int &targetTempIndex, const Value &value)
    {
        Q_ASSERT(value.isValid());

        invalidate(targetTempIndex, value);

        int replacementTemp = tempForValue.value(value, -1);
        if (replacementTemp == -1) {
//            qDebug() << "+++ inserting temp" << targetTempIndex;
            tempForValue.insert(value, targetTempIndex);
            valueForTemp.insert(targetTempIndex, value);
        } else {
//            qDebug() << "+++ temp" << targetTempIndex << "can be replaced with" << replacementTemp;
            tempReplacement.insert(targetTempIndex, replacementTemp);
        }
    }

    void reset()
    {
        tempForValue.clear();
        tempReplacement.clear();
        if (thisTemp != -1)
            tempForValue.insert(Value(Value::ThisType), thisTemp);
    }

private:
    QMap<Value, int> tempForValue;
    QMap<int, Value> valueForTemp;
    QMap<int, int> tempReplacement;

    int localCount;
    int thisTemp;
};

//#define DEBUG_TEMP_COMPRESSION
#ifdef DEBUG_TEMP_COMPRESSION
#  define DBTC(x) x
#else // !DEBUG_TEMP_COMPRESSION
#  define DBTC(x)
#endif // DEBUG_TEMP_COMPRESSION
class CompressTemps: public V4IR::StmtVisitor, V4IR::ExprVisitor
{
public:
    void run(V4IR::Function *function)
    {
        _nextFree = 0;
        _active.reserve(function->tempCount);
        _localCount = function->locals.size();

        DBTC(qDebug() << "starting on function" << (*function->name) << "with" << (function->tempCount - _localCount) << "temps.";)

        QVector<int> pinned;
        foreach (V4IR::BasicBlock *block, function->basicBlocks) {
            if (V4IR::Stmt *last = block->terminator()) {
                const QBitArray &liveOut = last->d->liveOut;
                for (int i = _localCount, ei = liveOut.size(); i < ei; ++i) {
                    if (liveOut.at(i) && !pinned.contains(i)) {
                        pinned.append(i);
                        DBTC(qDebug() << "Pinning:";)
                        add(i - _localCount, _nextFree);
                    }
                }
            }
        }
        _pinnedCount = _nextFree;

        int maxUsed = _nextFree;

        foreach (V4IR::BasicBlock *block, function->basicBlocks) {
            DBTC(qDebug("L%d:", block->index));

            for (int i = 0, ei = block->statements.size(); i < ei; ++i ) {
                _currentStatement = block->statements[i];
                if (i == 0)
                    expireOld();

                DBTC(_currentStatement->dump(qout);qout<<endl<<flush;)

                if (_currentStatement->d)
                    _currentStatement->accept(this);
            }
            maxUsed = std::max(maxUsed, _nextFree);
        }
        DBTC(qDebug() << "function" << (*function->name) << "uses" << maxUsed << "temps.";)
        function->tempCount = maxUsed + _localCount;
    }

protected:
    virtual void visitConst(V4IR::Const *) {}
    virtual void visitString(V4IR::String *) {}
    virtual void visitRegExp(V4IR::RegExp *) {}
    virtual void visitName(V4IR::Name *) {}
    virtual void visitClosure(V4IR::Closure *) {}
    virtual void visitUnop(V4IR::Unop *e) { e->expr->accept(this); }
    virtual void visitBinop(V4IR::Binop *e) { e->left->accept(this); e->right->accept(this); }
    virtual void visitSubscript(V4IR::Subscript *e) { e->base->accept(this); e->index->accept(this); }
    virtual void visitMember(V4IR::Member *e) { e->base->accept(this); }
    virtual void visitExp(V4IR::Exp *s) { s->expr->accept(this); }
    virtual void visitEnter(V4IR::Enter *) {}
    virtual void visitLeave(V4IR::Leave *) {}
    virtual void visitJump(V4IR::Jump *) {}
    virtual void visitCJump(V4IR::CJump *s) { s->cond->accept(this); }
    virtual void visitRet(V4IR::Ret *s) { s->expr->accept(this); }
    virtual void visitTry(V4IR::Try *t) { visitTemp(t->exceptionVar); }

    virtual void visitTemp(V4IR::Temp *e) {
        if (e->scope) // scoped local
            return;
        if (e->index < _localCount) // local or argument
            return;

        e->index = remap(e->index - _localCount) + _localCount;
    }

    virtual void visitCall(V4IR::Call *e) {
        e->base->accept(this);
        for (V4IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitNew(V4IR::New *e) {
        e->base->accept(this);
        for (V4IR::ExprList *it = e->args; it; it = it->next)
            it->expr->accept(this);
    }

    virtual void visitMove(V4IR::Move *s) {
        s->target->accept(this);
        s->source->accept(this);
    }

private:
    int remap(int tempIndex) {
        for (ActiveTemps::const_iterator i = _active.begin(), ei = _active.end(); i < ei; ++i) {
            if (i->first == tempIndex) {
                DBTC(qDebug() << "    lookup" << (tempIndex + _localCount) << "->" << (i->second + _localCount);)
                return i->second;
            }
        }

        int firstFree = expireOld();
        add(tempIndex, firstFree);
        return firstFree;
    }

    void add(int tempIndex, int firstFree) {
        if (_nextFree <= firstFree)
            _nextFree = firstFree + 1;
        _active.prepend(qMakePair(tempIndex, firstFree));
        DBTC(qDebug() << "    add" << (tempIndex + _localCount) << "->" << (firstFree+ _localCount);)
    }

    int expireOld() {
        Q_ASSERT(_currentStatement->d);

        const QBitArray &liveIn = _currentStatement->d->liveIn;
        QBitArray inUse(_nextFree);
        int i = 0;
        while (i < _active.size()) {
            const QPair<int, int> &p = _active[i];

            if (p.second < _pinnedCount) {
                inUse.setBit(p.second);
                ++i;
                continue;
            }

            if (liveIn[p.first + _localCount]) {
                inUse[p.second] = true;
                ++i;
            } else {
                DBTC(qDebug() << "    remove" << (p.first + _localCount) << "->" << (p.second + _localCount);)
                _active.remove(i);
            }
        }
        for (int i = 0, ei = inUse.size(); i < ei; ++i)
            if (!inUse[i])
                return i;
        return _nextFree;
    }

private:
    typedef QVector<QPair<int, int> > ActiveTemps;
    ActiveTemps _active;
    V4IR::Stmt *_currentStatement;
    int _localCount;
    int _nextFree;
    int _pinnedCount;
};
#undef DBTC

} // end of anonymous namespace

class Codegen::ScanFunctions: Visitor
{
    typedef QV4::TemporaryAssignment<bool> TemporaryBoolAssignment;
public:
    ScanFunctions(Codegen *cg, const QString &sourceCode)
        : _cg(cg)
        , _sourceCode(sourceCode)
        , _env(0)
        , _inFuncBody(false)
        , _allowFuncDecls(true)
    {
    }

    void operator()(Node *node)
    {
        if (node)
            node->accept(this);
    }

    inline void enterEnvironment(Node *node)
    {
        Environment *e = _cg->newEnvironment(node, _env);
        if (!e->isStrict)
            e->isStrict = _cg->_strictMode;
        _envStack.append(e);
        _env = e;
    }

    inline void leaveEnvironment()
    {
        _envStack.pop();
        _env = _envStack.isEmpty() ? 0 : _envStack.top();
    }

protected:
    using Visitor::visit;
    using Visitor::endVisit;

    void checkDirectivePrologue(SourceElements *ast)
    {
        for (SourceElements *it = ast; it; it = it->next) {
            if (StatementSourceElement *stmt = cast<StatementSourceElement *>(it->element)) {
                if (ExpressionStatement *expr = cast<ExpressionStatement *>(stmt->statement)) {
                    if (StringLiteral *strLit = cast<StringLiteral *>(expr->expression)) {
                        // Use the source code, because the StringLiteral's
                        // value might have escape sequences in it, which is not
                        // allowed.
                        if (strLit->literalToken.length < 2)
                            continue;
                        QStringRef str = _sourceCode.midRef(strLit->literalToken.offset + 1, strLit->literalToken.length - 2);
                        if (str == QStringLiteral("use strict")) {
                            _env->isStrict = true;
                        } else {
                            // TODO: give a warning.
                        }
                        continue;
                    }
                }
            }

            break;
        }
    }

    void checkName(const QStringRef &name, const SourceLocation &loc)
    {
        if (_env->isStrict) {
            if (name == QLatin1String("implements")
                    || name == QLatin1String("interface")
                    || name == QLatin1String("let")
                    || name == QLatin1String("package")
                    || name == QLatin1String("private")
                    || name == QLatin1String("protected")
                    || name == QLatin1String("public")
                    || name == QLatin1String("static")
                    || name == QLatin1String("yield")) {
                _cg->throwSyntaxError(loc, QCoreApplication::translate("qv4codegen", "Unexpected strict mode reserved word"));
            }
        }
    }
    void checkForArguments(AST::FormalParameterList *parameters)
    {
        while (parameters) {
            if (parameters->name == QStringLiteral("arguments"))
                _env->usesArgumentsObject = Environment::ArgumentsObjectNotUsed;
            parameters = parameters->next;
        }
    }

    virtual bool visit(Program *ast)
    {
        enterEnvironment(ast);
        checkDirectivePrologue(ast->elements);
        return true;
    }

    virtual void endVisit(Program *)
    {
        leaveEnvironment();
    }

    virtual bool visit(CallExpression *ast)
    {
        if (! _env->hasDirectEval) {
            if (IdentifierExpression *id = cast<IdentifierExpression *>(ast->base)) {
                if (id->name == QStringLiteral("eval")) {
                    if (_env->usesArgumentsObject == Environment::ArgumentsObjectUnknown)
                        _env->usesArgumentsObject = Environment::ArgumentsObjectUsed;
                    _env->hasDirectEval = true;
                }
            }
        }
        int argc = 0;
        for (ArgumentList *it = ast->arguments; it; it = it->next)
            ++argc;
        _env->maxNumberOfArguments = qMax(_env->maxNumberOfArguments, argc);
        return true;
    }

    virtual bool visit(NewMemberExpression *ast)
    {
        int argc = 0;
        for (ArgumentList *it = ast->arguments; it; it = it->next)
            ++argc;
        _env->maxNumberOfArguments = qMax(_env->maxNumberOfArguments, argc);
        return true;
    }

    virtual bool visit(ArrayLiteral *ast)
    {
        int index = 0;
        for (ElementList *it = ast->elements; it; it = it->next) {
            for (Elision *elision = it->elision; elision; elision = elision->next)
                ++index;
            ++index;
        }
        if (ast->elision) {
            for (Elision *elision = ast->elision->next; elision; elision = elision->next)
                ++index;
        }
        _env->maxNumberOfArguments = qMax(_env->maxNumberOfArguments, index);
        return true;
    }

    virtual bool visit(VariableDeclaration *ast)
    {
        if (_env->isStrict && (ast->name == QLatin1String("eval") || ast->name == QLatin1String("arguments")))
            _cg->throwSyntaxError(ast->identifierToken, QCoreApplication::translate("qv4codegen", "Variable name may not be eval or arguments in strict mode"));
        checkName(ast->name, ast->identifierToken);
        if (ast->name == QLatin1String("arguments"))
            _env->usesArgumentsObject = Environment::ArgumentsObjectNotUsed;
        _env->enter(ast->name.toString(), ast->expression ? Environment::VariableDefinition : Environment::VariableDeclaration);
        return true;
    }

    virtual bool visit(IdentifierExpression *ast)
    {
        checkName(ast->name, ast->identifierToken);
        if (_env->usesArgumentsObject == Environment::ArgumentsObjectUnknown && ast->name == QLatin1String("arguments"))
            _env->usesArgumentsObject = Environment::ArgumentsObjectUsed;
        return true;
    }

    virtual bool visit(ExpressionStatement *ast)
    {
        if (FunctionExpression* expr = AST::cast<AST::FunctionExpression*>(ast->expression)) {
            if (!_allowFuncDecls)
                _cg->throwSyntaxError(expr->functionToken, QCoreApplication::translate("qv4codegen", "conditional function or closure declaration"));

            enterFunction(expr, /*enterName*/ true);
            Node::accept(expr->formals, this);
            Node::accept(expr->body, this);
            leaveEnvironment();
            return false;
        } else {
            SourceLocation firstToken = ast->firstSourceLocation();
            if (_sourceCode.midRef(firstToken.offset, firstToken.length) == QStringLiteral("function")) {
                _cg->throwSyntaxError(firstToken, QCoreApplication::translate("qv4codegen", "unexpected token"));
            }
        }
        return true;
    }

    virtual bool visit(FunctionExpression *ast)
    {
        enterFunction(ast, /*enterName*/ false);
        return true;
    }

    void enterFunction(FunctionExpression *ast, bool enterName, bool isExpression = true)
    {
        if (_env->isStrict && (ast->name == QLatin1String("eval") || ast->name == QLatin1String("arguments")))
            _cg->throwSyntaxError(ast->identifierToken, QCoreApplication::translate("qv4codegen", "Function name may not be eval or arguments in strict mode"));
        enterFunction(ast, ast->name.toString(), ast->formals, ast->body, enterName ? ast : 0, isExpression);
    }

    virtual void endVisit(FunctionExpression *)
    {
        leaveEnvironment();
    }

    virtual bool visit(ObjectLiteral *ast)
    {
        int argc = 0;
        for (PropertyAssignmentList *it = ast->properties; it; it = it->next) {
            ++argc;
            if (AST::cast<AST::PropertyGetterSetter *>(it->assignment))
                ++argc;
        }
        _env->maxNumberOfArguments = qMax(_env->maxNumberOfArguments, argc);

        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, true);
        Node::accept(ast->properties, this);
        return false;
    }

    virtual bool visit(PropertyGetterSetter *ast)
    {
        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, true);
        enterFunction(ast, QString(), ast->formals, ast->functionBody, /*FunctionExpression*/0, /*isExpression*/false);
        return true;
    }

    virtual void endVisit(PropertyGetterSetter *)
    {
        leaveEnvironment();
    }

    virtual bool visit(FunctionDeclaration *ast)
    {
        enterFunction(ast, /*enterName*/ true, /*isExpression */false);
        return true;
    }

    virtual void endVisit(FunctionDeclaration *)
    {
        leaveEnvironment();
    }

    virtual bool visit(FunctionBody *ast)
    {
        TemporaryBoolAssignment inFuncBody(_inFuncBody, true);
        Node::accept(ast->elements, this);
        return false;
    }

    virtual bool visit(WithStatement *ast)
    {
        if (_env->isStrict) {
            _cg->throwSyntaxError(ast->withToken, QCoreApplication::translate("qv4codegen", "'with' statement is not allowed in strict mode"));
            return false;
        }

        return true;
    }

    virtual bool visit(IfStatement *ast) {
        Node::accept(ast->expression, this);

        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_inFuncBody);
        Node::accept(ast->ok, this);
        Node::accept(ast->ko, this);

        return false;
    }

    virtual bool visit(WhileStatement *ast) {
        Node::accept(ast->expression, this);

        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_inFuncBody);
        Node::accept(ast->statement, this);

        return false;
    }

    virtual bool visit(DoWhileStatement *ast) {
        {
            TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_env->isStrict);
            Node::accept(ast->statement, this);
        }
        Node::accept(ast->expression, this);
        return false;
    }

    virtual bool visit(ForStatement *ast) {
        Node::accept(ast->initialiser, this);
        Node::accept(ast->condition, this);
        Node::accept(ast->expression, this);

        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_env->isStrict);
        Node::accept(ast->statement, this);

        return false;
    }

    virtual bool visit(LocalForStatement *ast) {
        Node::accept(ast->declarations, this);
        Node::accept(ast->condition, this);
        Node::accept(ast->expression, this);

        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_env->isStrict);
        Node::accept(ast->statement, this);

        return false;
    }

    virtual bool visit(ForEachStatement *ast) {
        Node::accept(ast->initialiser, this);
        Node::accept(ast->expression, this);

        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_env->isStrict);
        Node::accept(ast->statement, this);

        return false;
    }

    virtual bool visit(LocalForEachStatement *ast) {
        Node::accept(ast->declaration, this);
        Node::accept(ast->expression, this);

        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, !_env->isStrict);
        Node::accept(ast->statement, this);

        return false;
    }

    virtual bool visit(Block *ast) {
        TemporaryBoolAssignment allowFuncDecls(_allowFuncDecls, _env->isStrict ? false : _allowFuncDecls);
        Node::accept(ast->statements, this);
        return false;
    }

private:
    void enterFunction(Node *ast, const QString &name, FormalParameterList *formals, FunctionBody *body, FunctionExpression *expr, bool isExpression)
    {
        bool wasStrict = false;
        if (_env) {
            _env->hasNestedFunctions = true;
            // The identifier of a function expression cannot be referenced from the enclosing environment.
            if (expr)
                _env->enter(name, Environment::FunctionDefinition, expr);
            if (name == QLatin1String("arguments"))
                _env->usesArgumentsObject = Environment::ArgumentsObjectNotUsed;
            wasStrict = _env->isStrict;
        }

        enterEnvironment(ast);
        checkForArguments(formals);

        _env->isNamedFunctionExpression = isExpression && !name.isEmpty();
        _env->formals = formals;

        if (body)
            checkDirectivePrologue(body->elements);

        if (wasStrict || _env->isStrict) {
            QStringList args;
            for (FormalParameterList *it = formals; it; it = it->next) {
                QString arg = it->name.toString();
                if (args.contains(arg))
                    _cg->throwSyntaxError(it->identifierToken, QCoreApplication::translate("qv4codegen", "Duplicate parameter name '%1' is not allowed in strict mode").arg(arg));
                if (arg == QLatin1String("eval") || arg == QLatin1String("arguments"))
                    _cg->throwSyntaxError(it->identifierToken, QCoreApplication::translate("qv4codegen", "'%1' cannot be used as parameter name in strict mode").arg(arg));
                args += arg;
            }
        }
    }

private: // fields:
    Codegen *_cg;
    const QString _sourceCode;
    Environment *_env;
    QStack<Environment *> _envStack;

    bool _inFuncBody;
    bool _allowFuncDecls;
};

Codegen::Codegen(QV4::ExecutionContext *context, bool strict)
    : _module(0)
    , _function(0)
    , _block(0)
    , _exitBlock(0)
    , _throwBlock(0)
    , _returnAddress(0)
    , _mode(GlobalCode)
    , _env(0)
    , _loop(0)
    , _labelledStatement(0)
    , _scopeAndFinally(0)
    , _context(context)
    , _strictMode(strict)
    , _debugger(context->engine->debugger)
    , _errorHandler(0)
{
}

Codegen::Codegen(ErrorHandler *errorHandler, bool strictMode)
    : _module(0)
    , _function(0)
    , _block(0)
    , _exitBlock(0)
    , _throwBlock(0)
    , _returnAddress(0)
    , _mode(GlobalCode)
    , _env(0)
    , _loop(0)
    , _labelledStatement(0)
    , _scopeAndFinally(0)
    , _context(0)
    , _strictMode(strictMode)
    , _debugger(0)
    , _errorHandler(errorHandler)
{
}

V4IR::Function *Codegen::operator()(const QString &fileName,
                                  const QString &sourceCode,
                                  Program *node,
                                  V4IR::Module *module,
                                  Mode mode,
                                  const QStringList &inheritedLocals)
{
    assert(node);

    _fileName = fileName;
    _module = module;
    _env = 0;

    ScanFunctions scan(this, sourceCode);
    scan(node);

    V4IR::Function *globalCode = defineFunction(QStringLiteral("%entry"), node, 0,
                                              node->elements, mode, inheritedLocals);
    if (_debugger) {
        if (node->elements->element) {
            SourceLocation loc = node->elements->element->firstSourceLocation();
            _debugger->setSourceLocation(globalCode, loc.startLine, loc.startColumn);
        }
    }

    foreach (V4IR::Function *function, _module->functions) {
        linearize(function);
    }

    qDeleteAll(_envMap);
    _envMap.clear();

    return globalCode;
}

V4IR::Function *Codegen::operator()(const QString &fileName,
                                  const QString &sourceCode,
                                  AST::FunctionExpression *ast,
                                  V4IR::Module *module)
{
    _fileName = fileName;
    _module = module;
    _env = 0;

    ScanFunctions scan(this, sourceCode);
    // fake a global environment
    scan.enterEnvironment(0);
    scan(ast);
    scan.leaveEnvironment();

    V4IR::Function *function = defineFunction(ast->name.toString(), ast, ast->formals, ast->body ? ast->body->elements : 0);
    if (_debugger)
        _debugger->setSourceLocation(function, ast->functionToken.startLine, ast->functionToken.startColumn);

    foreach (V4IR::Function *function, _module->functions) {
        linearize(function);
    }

    qDeleteAll(_envMap);
    _envMap.clear();

    return function;
}


void Codegen::enterEnvironment(Node *node)
{
    _env = _envMap.value(node);
    assert(_env);
}

void Codegen::leaveEnvironment()
{
    assert(_env);
    _env = _env->parent;
}

void Codegen::enterLoop(Statement *node, V4IR::BasicBlock *breakBlock, V4IR::BasicBlock *continueBlock)
{
    _loop = new Loop(node, breakBlock, continueBlock, _loop);
    _loop->labelledStatement = _labelledStatement; // consume the enclosing labelled statement
    _loop->scopeAndFinally = _scopeAndFinally;
    _labelledStatement = 0;
}

void Codegen::leaveLoop()
{
    Loop *current = _loop;
    _loop = _loop->parent;
    delete current;
}

V4IR::Expr *Codegen::member(V4IR::Expr *base, const QString *name)
{
    if (base->asTemp() /*|| base->asName()*/)
        return _block->MEMBER(base->asTemp(), name);
    else {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), base);
        return _block->MEMBER(_block->TEMP(t), name);
    }
}

V4IR::Expr *Codegen::subscript(V4IR::Expr *base, V4IR::Expr *index)
{
    if (! base->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), base);
        base = _block->TEMP(t);
    }

    if (! index->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), index);
        index = _block->TEMP(t);
    }

    assert(base->asTemp() && index->asTemp());
    return _block->SUBSCRIPT(base->asTemp(), index->asTemp());
}

V4IR::Expr *Codegen::argument(V4IR::Expr *expr)
{
    if (expr && ! expr->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    return expr;
}

// keeps references alive, converts other expressions to temps
V4IR::Expr *Codegen::reference(V4IR::Expr *expr)
{
    if (expr && !expr->asTemp() && !expr->asName() && !expr->asMember() && !expr->asSubscript()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    return expr;
}

V4IR::Expr *Codegen::unop(V4IR::AluOp op, V4IR::Expr *expr)
{
    if (V4IR::Const *c = expr->asConst()) {
        if (c->type == V4IR::NumberType) {
            switch (op) {
            case V4IR::OpNot:
                return _block->CONST(V4IR::BoolType, !c->value);
            case V4IR::OpUMinus:
                return _block->CONST(V4IR::NumberType, -c->value);
            case V4IR::OpUPlus:
                return expr;
            case V4IR::OpCompl:
                return _block->CONST(V4IR::NumberType, ~QV4::Value::toInt32(c->value));
            case V4IR::OpIncrement:
                return _block->CONST(V4IR::NumberType, c->value + 1);
            case V4IR::OpDecrement:
                return _block->CONST(V4IR::NumberType, c->value - 1);
            default:
                break;
            }
        }
    }
    if (! expr->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    assert(expr->asTemp());
    return _block->UNOP(op, expr->asTemp());
}

V4IR::Expr *Codegen::binop(V4IR::AluOp op, V4IR::Expr *left, V4IR::Expr *right)
{
    if (V4IR::Const *c1 = left->asConst()) {
        if (V4IR::Const *c2 = right->asConst()) {
            if (c1->type == V4IR::NumberType && c2->type == V4IR::NumberType) {
                switch (op) {
                case V4IR::OpAdd: return _block->CONST(V4IR::NumberType, c1->value + c2->value);
                case V4IR::OpAnd: return _block->CONST(V4IR::BoolType, c1->value ? c2->value : 0);
                case V4IR::OpBitAnd: return _block->CONST(V4IR::NumberType, int(c1->value) & int(c2->value));
                case V4IR::OpBitOr: return _block->CONST(V4IR::NumberType, int(c1->value) | int(c2->value));
                case V4IR::OpBitXor: return _block->CONST(V4IR::NumberType, int(c1->value) ^ int(c2->value));
                case V4IR::OpDiv: return _block->CONST(V4IR::NumberType, c1->value / c2->value);
                case V4IR::OpEqual: return _block->CONST(V4IR::BoolType, c1->value == c2->value);
                case V4IR::OpNotEqual: return _block->CONST(V4IR::BoolType, c1->value != c2->value);
                case V4IR::OpStrictEqual: return _block->CONST(V4IR::BoolType, c1->value == c2->value);
                case V4IR::OpStrictNotEqual: return _block->CONST(V4IR::BoolType, c1->value != c2->value);
                case V4IR::OpGe: return _block->CONST(V4IR::BoolType, c1->value >= c2->value);
                case V4IR::OpGt: return _block->CONST(V4IR::BoolType, c1->value > c2->value);
                case V4IR::OpLe: return _block->CONST(V4IR::BoolType, c1->value <= c2->value);
                case V4IR::OpLt: return _block->CONST(V4IR::BoolType, c1->value < c2->value);
                case V4IR::OpLShift: return _block->CONST(V4IR::NumberType, QV4::Value::toInt32(c1->value) << (QV4::Value::toUInt32(c2->value) & 0x1f));
                case V4IR::OpMod: return _block->CONST(V4IR::NumberType, ::fmod(c1->value, c2->value));
                case V4IR::OpMul: return _block->CONST(V4IR::NumberType, c1->value * c2->value);
                case V4IR::OpOr: return _block->CONST(V4IR::NumberType, c1->value ? c1->value : c2->value);
                case V4IR::OpRShift: return _block->CONST(V4IR::NumberType, QV4::Value::toInt32(c1->value) >> (QV4::Value::toUInt32(c2->value) & 0x1f));
                case V4IR::OpSub: return _block->CONST(V4IR::NumberType, c1->value - c2->value);
                case V4IR::OpURShift: return _block->CONST(V4IR::NumberType,QV4::Value::toUInt32(c1->value) >> (QV4::Value::toUInt32(c2->value) & 0x1f));

                case V4IR::OpInstanceof:
                case V4IR::OpIn:
                    break;

                case V4IR::OpIfTrue: // unary ops
                case V4IR::OpNot:
                case V4IR::OpUMinus:
                case V4IR::OpUPlus:
                case V4IR::OpCompl:
                case V4IR::OpIncrement:
                case V4IR::OpDecrement:
                case V4IR::OpInvalid:
                    break;
                }
            }
        }
    } else if (op == V4IR::OpAdd) {
        if (V4IR::String *s1 = left->asString()) {
            if (V4IR::String *s2 = right->asString()) {
                return _block->STRING(_function->newString(*s1->value + *s2->value));
            }
        }
    }

    if (!left->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), left);
        left = _block->TEMP(t);
    }

    if (!right->asTemp()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), right);
        right = _block->TEMP(t);
    }

    assert(left->asTemp());
    assert(right->asTemp());

    return _block->BINOP(op, left, right);
}

V4IR::Expr *Codegen::call(V4IR::Expr *base, V4IR::ExprList *args)
{
    base = reference(base);
    return _block->CALL(base, args);
}

void Codegen::move(V4IR::Expr *target, V4IR::Expr *source, V4IR::AluOp op)
{
    assert(target->isLValue());

    if (!source->asTemp() && !source->asConst() && (op != V4IR::OpInvalid || ! target->asTemp())) {
        unsigned t = _block->newTemp();
        _block->MOVE(_block->TEMP(t), source);
        source = _block->TEMP(t);
    }
    if (source->asConst() && (!target->asTemp() || op != V4IR::OpInvalid)) {
        unsigned t = _block->newTemp();
        _block->MOVE(_block->TEMP(t), source);
        source = _block->TEMP(t);
    }

    _block->MOVE(target, source, op);
}

void Codegen::cjump(V4IR::Expr *cond, V4IR::BasicBlock *iftrue, V4IR::BasicBlock *iffalse)
{
    if (! (cond->asTemp() || cond->asBinop())) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), cond);
        cond = _block->TEMP(t);
    }
    _block->CJUMP(cond, iftrue, iffalse);
}

void Codegen::accept(Node *node)
{
    if (node)
        node->accept(this);
}

void Codegen::statement(Statement *ast)
{
    _block->nextLocation = ast->firstSourceLocation();
    accept(ast);
}

void Codegen::statement(ExpressionNode *ast)
{
    if (! ast) {
        return;
    } else {
        Result r(nx);
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
        if (r.format == ex) {
            if (r->asCall()) {
                _block->EXP(*r); // the nest nx representation for calls is EXP(CALL(c..))
            } else if (r->asTemp()) {
                // there is nothing to do
            } else {
                unsigned t = _block->newTemp();
                move(_block->TEMP(t), *r);
            }
        }
    }
}

void Codegen::condition(ExpressionNode *ast, V4IR::BasicBlock *iftrue, V4IR::BasicBlock *iffalse)
{
    if (ast) {
        Result r(iftrue, iffalse);
        qSwap(_expr, r);
        accept(ast);
        qSwap(_expr, r);
        if (r.format == ex) {
            cjump(*r, r.iftrue, r.iffalse);
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

void Codegen::functionBody(FunctionBody *ast)
{
    if (ast)
        sourceElements(ast->elements);
}

void Codegen::program(Program *ast)
{
    if (ast) {
        sourceElements(ast->elements);
    }
}

void Codegen::sourceElements(SourceElements *ast)
{
    for (SourceElements *it = ast; it; it = it->next) {
        sourceElement(it->element);
    }
}

void Codegen::variableDeclaration(VariableDeclaration *ast)
{
    V4IR::Expr *initializer = 0;
    if (!ast->expression)
        return;
    Result expr = expression(ast->expression);
    assert(expr.code);
    initializer = *expr;

    if (! _env->parent || _function->insideWithOrCatch) {
        // it's global code.
        move(_block->NAME(ast->name.toString(), ast->identifierToken.startLine, ast->identifierToken.startColumn), initializer);
    } else {
        const int index = _env->findMember(ast->name.toString());
        assert(index != -1);
        move(_block->TEMP(index), initializer);
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
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(CaseBlock *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(CaseClause *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(CaseClauses *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(Catch *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(DefaultClause *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(ElementList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(Elision *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(Finally *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(FormalParameterList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(FunctionBody *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(Program *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(PropertyAssignmentList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(PropertyNameAndValue *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(PropertyGetterSetter *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(SourceElements *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(StatementList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiArrayMemberList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiImport *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiImportList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiObjectInitializer *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiObjectMemberList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiParameterList *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiProgram *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(UiQualifiedId *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(VariableDeclaration *)
{
    assert(!"unreachable");
    return false;
}

bool Codegen::visit(VariableDeclarationList *)
{
    assert(!"unreachable");
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
    V4IR::ExprList *args = 0;
    V4IR::ExprList *current = 0;
    for (ElementList *it = ast->elements; it; it = it->next) {
        for (Elision *elision = it->elision; elision; elision = elision->next) {
            V4IR::ExprList *arg = _function->New<V4IR::ExprList>();
            if (!current) {
                args = arg;
            } else {
                current->next = arg;
            }
            current = arg;
            current->expr = _block->CONST(V4IR::MissingType, 0);
        }
        Result expr = expression(it->expression);

        V4IR::ExprList *arg = _function->New<V4IR::ExprList>();
        if (!current) {
            args = arg;
        } else {
            current->next = arg;
        }
        current = arg;

        V4IR::Expr *exp = *expr;
        if (exp->asTemp() || exp->asConst()) {
            current->expr = exp;
        } else {
            unsigned value = _block->newTemp();
            move(_block->TEMP(value), exp);
            current->expr = _block->TEMP(value);
        }
    }
    for (Elision *elision = ast->elision; elision; elision = elision->next) {
        V4IR::ExprList *arg = _function->New<V4IR::ExprList>();
        if (!current) {
            args = arg;
        } else {
            current->next = arg;
        }
        current = arg;
        current->expr = _block->CONST(V4IR::MissingType, 0);
    }

    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->CALL(_block->NAME(V4IR::Name::builtin_define_array, 0, 0), args));
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

static V4IR::AluOp baseOp(int op)
{
    switch ((QSOperator::Op) op) {
    case QSOperator::InplaceAnd: return V4IR::OpBitAnd;
    case QSOperator::InplaceSub: return V4IR::OpSub;
    case QSOperator::InplaceDiv: return V4IR::OpDiv;
    case QSOperator::InplaceAdd: return V4IR::OpAdd;
    case QSOperator::InplaceLeftShift: return V4IR::OpLShift;
    case QSOperator::InplaceMod: return V4IR::OpMod;
    case QSOperator::InplaceMul: return V4IR::OpMul;
    case QSOperator::InplaceOr: return V4IR::OpBitOr;
    case QSOperator::InplaceRightShift: return V4IR::OpRShift;
    case QSOperator::InplaceURightShift: return V4IR::OpURShift;
    case QSOperator::InplaceXor: return V4IR::OpBitXor;
    default: return V4IR::OpInvalid;
    }
}

bool Codegen::visit(BinaryExpression *ast)
{
    if (ast->op == QSOperator::And) {
        if (_expr.accept(cx)) {
            V4IR::BasicBlock *iftrue = _function->newBasicBlock();
            condition(ast->left, iftrue, _expr.iffalse);
            _block = iftrue;
            condition(ast->right, _expr.iftrue, _expr.iffalse);
        } else {
            V4IR::BasicBlock *iftrue = _function->newBasicBlock();
            V4IR::BasicBlock *endif = _function->newBasicBlock();

            const unsigned r = _block->newTemp();

            move(_block->TEMP(r), *expression(ast->left));
            cjump(_block->TEMP(r), iftrue, endif);
            _block = iftrue;
            move(_block->TEMP(r), *expression(ast->right));
            _block->JUMP(endif);

            _expr.code = _block->TEMP(r);
            _block = endif;
        }
        return false;
    } else if (ast->op == QSOperator::Or) {
        if (_expr.accept(cx)) {
            V4IR::BasicBlock *iffalse = _function->newBasicBlock();
            condition(ast->left, _expr.iftrue, iffalse);
            _block = iffalse;
            condition(ast->right, _expr.iftrue, _expr.iffalse);
        } else {
            V4IR::BasicBlock *iffalse = _function->newBasicBlock();
            V4IR::BasicBlock *endif = _function->newBasicBlock();

            const unsigned r = _block->newTemp();
            move(_block->TEMP(r), *expression(ast->left));
            cjump(_block->TEMP(r), endif, iffalse);
            _block = iffalse;
            move(_block->TEMP(r), *expression(ast->right));
            _block->JUMP(endif);

            _block = endif;
            _expr.code = _block->TEMP(r);
        }
        return false;
    }

    V4IR::Expr* left = *expression(ast->left);
    throwSyntaxErrorOnEvalOrArgumentsInStrictMode(left, ast->left->lastSourceLocation());

    switch (ast->op) {
    case QSOperator::Or:
    case QSOperator::And:
        break;

    case QSOperator::Assign: {
        V4IR::Expr* right = *expression(ast->right);
        if (! (left->asTemp() || left->asName() || left->asSubscript() || left->asMember()))
            throwReferenceError(ast->operatorToken, QCoreApplication::translate("qv4codegen", "left-hand side of assignment operator is not an lvalue"));

        if (_expr.accept(nx)) {
            move(left, right);
        } else {
            const unsigned t = _block->newTemp();
            move(_block->TEMP(t), right);
            move(left, _block->TEMP(t));
            _expr.code = _block->TEMP(t);
        }
        break;
    }

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
        V4IR::Expr* right = *expression(ast->right);
        if (!left->isLValue())
            throwSyntaxError(ast->operatorToken, QCoreApplication::translate("qv4codegen", "left-hand side of inplace operator is not an lvalue"));

        if (_expr.accept(nx)) {
            move(left, right, baseOp(ast->op));
        } else {
            const unsigned t = _block->newTemp();
            move(_block->TEMP(t), right);
            move(left, _block->TEMP(t), baseOp(ast->op));
            _expr.code = left;
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
    case QSOperator::StrictNotEqual: {
        if (!left->asTemp() && !left->asConst()) {
            const unsigned t = _block->newTemp();
            move(_block->TEMP(t), left);
            left = _block->TEMP(t);
        }

        V4IR::Expr* right = *expression(ast->right);

        if (_expr.accept(cx)) {
            cjump(binop(V4IR::binaryOperator(ast->op), left, right), _expr.iftrue, _expr.iffalse);
        } else {
            V4IR::Expr *e = binop(V4IR::binaryOperator(ast->op), left, right);
            if (e->asConst() || e->asString())
                _expr.code = e;
            else {
                const unsigned t = _block->newTemp();
                move(_block->TEMP(t), e);
                _expr.code = _block->TEMP(t);
            }
        }
        break;
    }

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
        if (!left->asTemp() && !left->asConst()) {
            const unsigned t = _block->newTemp();
            move(_block->TEMP(t), left);
            left = _block->TEMP(t);
        }

        V4IR::Expr* right = *expression(ast->right);

        V4IR::Expr *e = binop(V4IR::binaryOperator(ast->op), left, right);
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
    V4IR::ExprList *args = 0, **args_it = &args;
    for (ArgumentList *it = ast->arguments; it; it = it->next) {
        Result arg = expression(it->expression);
        V4IR::Expr *actual = argument(*arg);
        *args_it = _function->New<V4IR::ExprList>();
        (*args_it)->init(actual);
        args_it = &(*args_it)->next;
    }
    _expr.code = call(*base, args);
    return false;
}

bool Codegen::visit(ConditionalExpression *ast)
{
    V4IR::BasicBlock *iftrue = _function->newBasicBlock();
    V4IR::BasicBlock *iffalse = _function->newBasicBlock();
    V4IR::BasicBlock *endif = _function->newBasicBlock();

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

bool Codegen::visit(DeleteExpression *ast)
{
    V4IR::Expr* expr = *expression(ast->expression);
    // Temporaries cannot be deleted
    if (expr->asTemp() && expr->asTemp()->index < _env->members.size()) {
        // Trying to delete a function argument might throw.
        if (_function->isStrict && expr->asTemp()->index < 0)
            throwSyntaxError(ast->deleteToken, "Delete of an unqualified identifier in strict mode.");
        _expr.code = _block->CONST(V4IR::BoolType, 0);
        return false;
    }
    if (_function->isStrict && expr->asName())
        throwSyntaxError(ast->deleteToken, "Delete of an unqualified identifier in strict mode.");

    // [[11.4.1]] Return true if it's not a reference
    if (expr->asConst() || expr->asString()) {
        _expr.code = _block->CONST(V4IR::BoolType, 1);
        return false;
    }

    // Return values from calls are also not a reference, but we have to
    // perform the call to allow for side effects.
    if (expr->asCall()) {
        _block->EXP(expr);
        _expr.code = _block->CONST(V4IR::BoolType, 1);
        return false;
    }
    if (expr->asTemp() && expr->asTemp()->index >=  _env->members.size()) {
        _expr.code = _block->CONST(V4IR::BoolType, 1);
        return false;
    }

    V4IR::ExprList *args = _function->New<V4IR::ExprList>();
    args->init(reference(expr));
    _expr.code = call(_block->NAME(V4IR::Name::builtin_delete, ast->deleteToken.startLine, ast->deleteToken.startColumn), args);
    return false;
}

bool Codegen::visit(FalseLiteral *)
{
    if (_expr.accept(cx)) {
        _block->JUMP(_expr.iffalse);
    } else {
        _expr.code = _block->CONST(V4IR::BoolType, 0);
    }
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
    V4IR::Function *function = defineFunction(ast->name.toString(), ast, ast->formals, ast->body ? ast->body->elements : 0);
    if (_debugger)
        _debugger->setSourceLocation(function, ast->functionToken.startLine, ast->functionToken.startColumn);
    _expr.code = _block->CLOSURE(function);
    return false;
}

V4IR::Expr *Codegen::identifier(const QString &name, int line, int col)
{
    uint scope = 0;
    Environment *e = _env;
    V4IR::Function *f = _function;

    while (f && e->parent) {
        if ((f->usesArgumentsObject && name == "arguments") || (!f->isStrict && f->hasDirectEval) || f->insideWithOrCatch || (f->isNamedExpression && f->name == name))
            break;
        int index = e->findMember(name);
        assert (index < e->members.size());
        if (index != -1) {
            return _block->TEMP(index, scope);
        }
        const int argIdx = f->indexOfArgument(&name);
        if (argIdx != -1)
            return _block->TEMP(-(argIdx + 1), scope);
        ++scope;
        e = e->parent;
        f = f->outer;
    }

    if (!e->parent && (!f || !f->insideWithOrCatch) && _mode != EvalCode && (!f || f->name != name))
        return _block->GLOBALNAME(name, line, col);

    // global context or with. Lookup by name
    return _block->NAME(name, line, col);

}

bool Codegen::visit(IdentifierExpression *ast)
{
    _expr.code = identifier(ast->name.toString(), ast->identifierToken.startLine, ast->identifierToken.startColumn);
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
    V4IR::Expr *expr = *base;
    if (expr && !expr->asTemp() && !expr->asName() && !expr->asMember()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }
    _expr.code = _block->NEW(expr, 0);
    return false;
}

bool Codegen::visit(NewMemberExpression *ast)
{
    Result base = expression(ast->base);
    V4IR::Expr *expr = *base;
    if (expr && !expr->asTemp() && !expr->asName() && !expr->asMember()) {
        const unsigned t = _block->newTemp();
        move(_block->TEMP(t), expr);
        expr = _block->TEMP(t);
    }

    V4IR::ExprList *args = 0, **args_it = &args;
    for (ArgumentList *it = ast->arguments; it; it = it->next) {
        Result arg = expression(it->expression);
        V4IR::Expr *actual = argument(*arg);
        *args_it = _function->New<V4IR::ExprList>();
        (*args_it)->init(actual);
        args_it = &(*args_it)->next;
    }
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->NEW(expr, args));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(NotExpression *ast)
{
    Result expr = expression(ast->expression);
    const unsigned r = _block->newTemp();
    move(_block->TEMP(r), unop(V4IR::OpNot, *expr));
    _expr.code = _block->TEMP(r);
    return false;
}

bool Codegen::visit(NullExpression *)
{
    if (_expr.accept(cx)) _block->JUMP(_expr.iffalse);
    else _expr.code = _block->CONST(V4IR::NullType, 0);

    return false;
}

bool Codegen::visit(NumericLiteral *ast)
{
    if (_expr.accept(cx)) {
        if (ast->value) _block->JUMP(_expr.iftrue);
        else _block->JUMP(_expr.iffalse);
    } else {
        _expr.code = _block->CONST(V4IR::NumberType, ast->value);
    }
    return false;
}

struct ObjectPropertyValue {
    V4IR::Expr *value;
    V4IR::Function *getter;
    V4IR::Function *setter;
};

bool Codegen::visit(ObjectLiteral *ast)
{
    QMap<QString, ObjectPropertyValue> valueMap;

    for (PropertyAssignmentList *it = ast->properties; it; it = it->next) {
        if (PropertyNameAndValue *nv = AST::cast<AST::PropertyNameAndValue *>(it->assignment)) {
            QString name = propertyName(nv->name);
            Result value = expression(nv->value);
            ObjectPropertyValue &v = valueMap[name];
            if (v.getter || v.setter || (_function->isStrict && v.value))
                throwSyntaxError(nv->lastSourceLocation(),
                                 QCoreApplication::translate("qv4codegen", "Illegal duplicate key '%1' in object literal").arg(name));

            valueMap[name].value = *value;
        } else if (PropertyGetterSetter *gs = AST::cast<AST::PropertyGetterSetter *>(it->assignment)) {
            QString name = propertyName(gs->name);
            V4IR::Function *function = defineFunction(name, gs, gs->formals, gs->functionBody ? gs->functionBody->elements : 0);
            if (_debugger)
                _debugger->setSourceLocation(function, gs->getSetToken.startLine, gs->getSetToken.startColumn);
            ObjectPropertyValue &v = valueMap[name];
            if (v.value ||
                (gs->type == PropertyGetterSetter::Getter && v.getter) ||
                (gs->type == PropertyGetterSetter::Setter && v.setter))
                throwSyntaxError(gs->lastSourceLocation(),
                                 QCoreApplication::translate("qv4codegen", "Illegal duplicate key '%1' in object literal").arg(name));
            if (gs->type == PropertyGetterSetter::Getter)
                v.getter = function;
            else
                v.setter = function;
        } else {
            Q_UNREACHABLE();
        }
    }

    V4IR::ExprList *args = 0;

    if (!valueMap.isEmpty()) {
        V4IR::ExprList *current;
        for (QMap<QString, ObjectPropertyValue>::iterator it = valueMap.begin(); it != valueMap.end(); ) {
            if (QV4::String(0, it.key()).asArrayIndex() != UINT_MAX) {
                ++it;
                continue;
            }

            if (!args) {
                args = _function->New<V4IR::ExprList>();
                current = args;
            } else {
                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
            }

            current->expr = _block->NAME(it.key(), 0, 0);

            if (it->value) {
                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->CONST(V4IR::BoolType, true);

                unsigned value = _block->newTemp();
                move(_block->TEMP(value), it->value);

                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->TEMP(value);
            } else {
                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->CONST(V4IR::BoolType, false);

                unsigned getter = _block->newTemp();
                unsigned setter = _block->newTemp();
                move(_block->TEMP(getter), it->getter ? _block->CLOSURE(it->getter) : _block->CONST(V4IR::UndefinedType, 0));
                move(_block->TEMP(setter), it->setter ? _block->CLOSURE(it->setter) : _block->CONST(V4IR::UndefinedType, 0));

                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->TEMP(getter);
                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->TEMP(setter);
            }

            it = valueMap.erase(it);
        }
    }

    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), _block->CALL(_block->NAME(V4IR::Name::builtin_define_object_literal,
         ast->firstSourceLocation().startLine, ast->firstSourceLocation().startColumn), args));

    // What's left are array entries
    if (!valueMap.isEmpty()) {
        unsigned value = 0;
        unsigned getter = 0;
        unsigned setter = 0;
        for (QMap<QString, ObjectPropertyValue>::const_iterator it = valueMap.constBegin(); it != valueMap.constEnd(); ++it) {
            V4IR::ExprList *args = _function->New<V4IR::ExprList>();
            V4IR::ExprList *current = args;
            current->expr = _block->TEMP(t);
            current->next = _function->New<V4IR::ExprList>();
            current = current->next;
            current->expr = _block->NAME(it.key(), 0, 0);
            current->next = _function->New<V4IR::ExprList>();
            current = current->next;

            if (it->value) {
                if (!value)
                    value = _block->newTemp();
                move(_block->TEMP(value), it->value);
                // __qmljs_builtin_define_property(Value object, String *name, Value val, ExecutionContext *ctx)
                current->expr = _block->TEMP(value);
                _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_define_property, 0, 0), args));
            } else {
                if (!getter) {
                    getter = _block->newTemp();
                    setter = _block->newTemp();
                }
                move(_block->TEMP(getter), it->getter ? _block->CLOSURE(it->getter) : _block->CONST(V4IR::UndefinedType, 0));
                move(_block->TEMP(setter), it->setter ? _block->CLOSURE(it->setter) : _block->CONST(V4IR::UndefinedType, 0));


                // __qmljs_builtin_define_getter_setter(Value object, String *name, Value getter, Value setter, ExecutionContext *ctx);
                current->expr = _block->TEMP(getter);
                current->next = _function->New<V4IR::ExprList>();
                current = current->next;
                current->expr = _block->TEMP(setter);
                _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_define_getter_setter, 0, 0), args));
            }
        }
    }

    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(PostDecrementExpression *ast)
{
    Result expr = expression(ast->base);
    if (!expr->isLValue())
        throwReferenceError(ast->base->lastSourceLocation(), "Invalid left-hand side expression in postfix operation");
    throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->decrementToken);

    if (_expr.accept(nx)) {
        move(*expr, unop(V4IR::OpDecrement, *expr));
    } else {
        V4IR::ExprList *args = _function->New<V4IR::ExprList>();
        args->init(*expr);
        _expr.code = call(_block->NAME(V4IR::Name::builtin_postdecrement, ast->lastSourceLocation().startLine, ast->lastSourceLocation().startColumn), args);
    }
    return false;
}

bool Codegen::visit(PostIncrementExpression *ast)
{
    Result expr = expression(ast->base);
    if (!expr->isLValue())
        throwReferenceError(ast->base->lastSourceLocation(), "Invalid left-hand side expression in postfix operation");
    throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->incrementToken);

    if (_expr.accept(nx)) {
        move(*expr, unop(V4IR::OpIncrement, *expr));
    } else {
        V4IR::ExprList *args = _function->New<V4IR::ExprList>();
        args->init(*expr);
        _expr.code = call(_block->NAME(V4IR::Name::builtin_postincrement, ast->lastSourceLocation().startLine, ast->lastSourceLocation().startColumn), args);
    }
    return false;
}

bool Codegen::visit(PreDecrementExpression *ast)
{
    Result expr = expression(ast->expression);
    throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->decrementToken);
    move(*expr, unop(V4IR::OpDecrement, *expr));
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
    throwSyntaxErrorOnEvalOrArgumentsInStrictMode(*expr, ast->incrementToken);
    move(*expr, unop(V4IR::OpIncrement, *expr));
    if (_expr.accept(nx)) {
        // nothing to do
    } else {
        _expr.code = *expr;
    }
    return false;
}

bool Codegen::visit(RegExpLiteral *ast)
{
    _expr.code = _block->REGEXP(_function->newString(ast->pattern.toString()), ast->flags);
    return false;
}

bool Codegen::visit(StringLiteral *ast)
{
    _expr.code = _block->STRING(_function->newString(ast->value.toString()));
    return false;
}

bool Codegen::visit(ThisExpression *ast)
{
    _expr.code = _block->NAME(QStringLiteral("this"), ast->thisToken.startLine, ast->thisToken.startColumn);
    return false;
}

bool Codegen::visit(TildeExpression *ast)
{
    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), unop(V4IR::OpCompl, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(TrueLiteral *)
{
    if (_expr.accept(cx)) {
        _block->JUMP(_expr.iftrue);
    } else {
        _expr.code = _block->CONST(V4IR::BoolType, 1);
    }
    return false;
}

bool Codegen::visit(TypeOfExpression *ast)
{
    Result expr = expression(ast->expression);
    V4IR::ExprList *args = _function->New<V4IR::ExprList>();
    args->init(reference(*expr));
    _expr.code = call(_block->NAME(V4IR::Name::builtin_typeof, ast->typeofToken.startLine, ast->typeofToken.startColumn), args);
    return false;
}

bool Codegen::visit(UnaryMinusExpression *ast)
{
    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), unop(V4IR::OpUMinus, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(UnaryPlusExpression *ast)
{
    Result expr = expression(ast->expression);
    const unsigned t = _block->newTemp();
    move(_block->TEMP(t), unop(V4IR::OpUPlus, *expr));
    _expr.code = _block->TEMP(t);
    return false;
}

bool Codegen::visit(VoidExpression *ast)
{
    statement(ast->expression);
    _expr.code = _block->CONST(V4IR::UndefinedType, 0);
    return false;
}

bool Codegen::visit(FunctionDeclaration * /*ast*/)
{
    _expr.accept(nx);
    return false;
}

void Codegen::linearize(V4IR::Function *function)
{
    V4IR::BasicBlock *exitBlock = function->basicBlocks.last();
    assert(exitBlock->isTerminated());
    assert(exitBlock->terminator()->asRet());

    QSet<V4IR::BasicBlock *> V;
    V.insert(exitBlock);

    QVector<V4IR::BasicBlock *> trace;

    for (int i = 0; i < function->basicBlocks.size(); ++i) {
        V4IR::BasicBlock *block = function->basicBlocks.at(i);
        if (!block->isTerminated() && (i + 1) < function->basicBlocks.size()) {
            V4IR::BasicBlock *next = function->basicBlocks.at(i + 1);
            block->JUMP(next);
        }
    }

    struct I { static void trace(V4IR::BasicBlock *block, QSet<V4IR::BasicBlock *> *V,
                                 QVector<V4IR::BasicBlock *> *output) {
            if (block == 0 || V->contains(block))
                return;

            V->insert(block);
            block->index = output->size();
            output->append(block);

            if (V4IR::Stmt *term = block->terminator()) {
                if (V4IR::Jump *j = term->asJump()) {
                    trace(j->target, V, output);
                } else if (V4IR::CJump *cj = term->asCJump()) {
                    if (! V->contains(cj->iffalse))
                        trace(cj->iffalse, V, output);
                    else
                        trace(cj->iftrue, V, output);
                } else if (V4IR::Try *t = term->asTry()) {
                    trace(t->tryBlock, V, output);
                    trace(t->catchBlock, V, output);
                }
            }

            // We could do this for each type above, but it is safer to have a
            // "catchall" here
            for (int ii = 0; ii < block->out.count(); ++ii)
                trace(block->out.at(ii), V, output);
        }
    };

    I::trace(function->basicBlocks.first(), &V, &trace);

    V.insert(exitBlock);
    exitBlock->index = trace.size();
    trace.append(exitBlock);

    QVarLengthArray<V4IR::BasicBlock*> blocksToDelete;
    foreach (V4IR::BasicBlock *b, function->basicBlocks) {
        if (!V.contains(b)) {
            foreach (V4IR::BasicBlock *out, b->out) {
                int idx = out->in.indexOf(b);
                if (idx >= 0)
                    out->in.remove(idx);
            }
            blocksToDelete.append(b);
        }
    }
    foreach (V4IR::BasicBlock *b, blocksToDelete)
        foreach (V4IR::Stmt *s, b->statements)
            s->destroyData();
    qDeleteAll(blocksToDelete);
    function->basicBlocks = trace;

    function->removeSharedExpressions();

    if (qgetenv("NO_OPT").isEmpty())
        ConstantPropagation().run(function);

#ifndef QV4_NO_LIVENESS
    liveness(function);
#endif

    if (qgetenv("NO_OPT").isEmpty()) {
        removeDeadAssignments(function);
        removeUnreachableBlocks(function);
    }

    static bool showCode = !qgetenv("SHOW_CODE").isNull();
    if (showCode) {
        QVector<V4IR::Stmt *> code;
        QHash<V4IR::Stmt *, V4IR::BasicBlock *> leader;

        foreach (V4IR::BasicBlock *block, function->basicBlocks) {
            leader.insert(block->statements.first(), block);
            foreach (V4IR::Stmt *s, block->statements) {
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
            V4IR::Stmt *s = code.at(i);

            if (V4IR::BasicBlock *bb = leader.value(s)) {
                qout << endl;
                QByteArray str;
                str.append('L');
                str.append(QByteArray::number(bb->index));
                str.append(':');
                for (int i = 66 - str.length(); i; --i)
                    str.append(' ');
                qout << str;
                qout << "// predecessor blocks:";
                foreach (V4IR::BasicBlock *in, bb->in)
                    qout << " L" << in->index;
                qout << endl;
            }
            V4IR::Stmt *n = (i + 1) < code.size() ? code.at(i + 1) : 0;
            if (n && s->asJump() && s->asJump()->target == leader.value(n)) {
                continue;
            }

            QByteArray str;
            QBuffer buf(&str);
            buf.open(QIODevice::WriteOnly);
            QTextStream out(&buf);
            s->dump(out, V4IR::Stmt::MIR);
            out.flush();

            if (s->location.isValid())
                qout << "    // line: " << s->location.startLine << " column: " << s->location.startColumn << endl;

#ifndef QV4_NO_LIVENESS
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

#  if 0
            if (! s->d->liveIn.isEmpty()) {
                qout << " // lives in:";
                for (int i = 0; i < s->d->liveIn.size(); ++i) {
                    if (s->d->liveIn.testBit(i))
                        qout << " %" << i;
                }
            }
#  else
            if (! s->d->liveOut.isEmpty()) {
                qout << " // lives out:";
                for (int i = 0; i < s->d->liveOut.size(); ++i) {
                    if (s->d->liveOut.testBit(i))
                        qout << " %" << i;
                }
            }
#  endif
#else
            qout << "    " << str;
#endif

            qout << endl;

            if (n && s->asCJump() && s->asCJump()->iffalse != leader.value(n)) {
                qout << "    goto L" << s->asCJump()->iffalse << ";" << endl;
            }
        }

        qout << "}" << endl
             << endl;
    }

    //### NOTE: after this pass, the liveness information is not correct anymore!
    if (qgetenv("NO_OPT").isEmpty())
        CompressTemps().run(function);
}

V4IR::Function *Codegen::defineFunction(const QString &name, AST::Node *ast,
                                      AST::FormalParameterList *formals,
                                      AST::SourceElements *body, Mode mode,
                                      const QStringList &inheritedLocals)
{
    qSwap(_mode, mode); // enter function code.

    ScopeAndFinally *scopeAndFinally = 0;

    enterEnvironment(ast);
    V4IR::Function *function = _module->newFunction(name, _function);
    function->sourceFile = _fileName;

    if (_debugger)
        _debugger->addFunction(function);
    V4IR::BasicBlock *entryBlock = function->newBasicBlock();
    V4IR::BasicBlock *exitBlock = function->newBasicBlock(V4IR::Function::DontInsertBlock);
    V4IR::BasicBlock *throwBlock = function->newBasicBlock();
    function->hasDirectEval = _env->hasDirectEval;
    function->usesArgumentsObject = (_env->usesArgumentsObject == Environment::ArgumentsObjectUsed);
    function->maxNumberOfArguments = _env->maxNumberOfArguments;
    function->isStrict = _env->isStrict;
    function->isNamedExpression = _env->isNamedFunctionExpression;

    // variables in global code are properties of the global context object, not locals as with other functions.
    if (_mode == FunctionCode) {
        for (Environment::MemberMap::iterator it = _env->members.begin(); it != _env->members.end(); ++it) {
            const QString &local = it.key();
            function->LOCAL(local);
            unsigned t = entryBlock->newTemp();
            (*it).index = t;
            entryBlock->MOVE(entryBlock->TEMP(t), entryBlock->CONST(V4IR::UndefinedType, 0));
        }
    } else {
        if (!_env->isStrict) {
            foreach (const QString &inheritedLocal, inheritedLocals) {
                function->LOCAL(inheritedLocal);
                unsigned tempIndex = entryBlock->newTemp();
                Environment::Member member = { Environment::UndefinedMember,
                                               static_cast<int>(tempIndex), 0 };
                _env->members.insert(inheritedLocal, member);
            }
        }

        V4IR::ExprList *args = 0;
        for (Environment::MemberMap::const_iterator it = _env->members.constBegin(); it != _env->members.constEnd(); ++it) {
            const QString &local = it.key();
            V4IR::ExprList *next = function->New<V4IR::ExprList>();
            next->expr = entryBlock->NAME(local, 0, 0);
            next->next = args;
            args = next;
        }
        if (args) {
            V4IR::ExprList *next = function->New<V4IR::ExprList>();
            next->expr = entryBlock->CONST(V4IR::BoolType, mode == EvalCode);
            next->next = args;
            args = next;

            entryBlock->EXP(entryBlock->CALL(entryBlock->NAME(V4IR::Name::builtin_declare_vars, 0, 0), args));
        }
    }

    unsigned returnAddress = entryBlock->newTemp();

    entryBlock->MOVE(entryBlock->TEMP(returnAddress), entryBlock->CONST(V4IR::UndefinedType, 0));
    exitBlock->RET(exitBlock->TEMP(returnAddress));
    V4IR::ExprList *throwArgs = function->New<V4IR::ExprList>();
    throwArgs->expr = throwBlock->TEMP(returnAddress);
    throwBlock->EXP(throwBlock->CALL(throwBlock->NAME(V4IR::Name::builtin_throw, /*line*/0, /*column*/0), throwArgs));
    throwBlock->JUMP(exitBlock);
    Loop *loop = 0;

    qSwap(_function, function);
    qSwap(_block, entryBlock);
    qSwap(_exitBlock, exitBlock);
    qSwap(_throwBlock, throwBlock);
    qSwap(_returnAddress, returnAddress);
    qSwap(_scopeAndFinally, scopeAndFinally);
    qSwap(_loop, loop);

    for (FormalParameterList *it = formals; it; it = it->next) {
        _function->RECEIVE(it->name.toString());
    }

    foreach (const Environment::Member &member, _env->members) {
        if (member.function) {
            V4IR::Function *function = defineFunction(member.function->name.toString(), member.function, member.function->formals,
                                                    member.function->body ? member.function->body->elements : 0);
            if (_debugger)
                _debugger->setSourceLocation(function, member.function->functionToken.startLine, member.function->functionToken.startColumn);
            if (! _env->parent) {
                move(_block->NAME(member.function->name.toString(), member.function->identifierToken.startLine, member.function->identifierToken.startColumn),
                     _block->CLOSURE(function));
            } else {
                assert(member.index >= 0);
                move(_block->TEMP(member.index), _block->CLOSURE(function));
            }
        }
    }

    sourceElements(body);

    _function->insertBasicBlock(_exitBlock);

    _block->JUMP(_exitBlock);

    qSwap(_function, function);
    qSwap(_block, entryBlock);
    qSwap(_exitBlock, exitBlock);
    qSwap(_throwBlock, throwBlock);
    qSwap(_returnAddress, returnAddress);
    qSwap(_scopeAndFinally, scopeAndFinally);
    qSwap(_loop, loop);

    leaveEnvironment();

    qSwap(_mode, mode);

    return function;
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

bool Codegen::visit(BreakStatement *ast)
{
    if (!_loop)
        throwSyntaxError(ast->lastSourceLocation(), QCoreApplication::translate("qv4codegen", "Break outside of loop"));
    Loop *loop = 0;
    if (ast->label.isEmpty())
        loop = _loop;
    else {
        for (loop = _loop; loop; loop = loop->parent) {
            if (loop->labelledStatement && loop->labelledStatement->label == ast->label)
                break;
        }
        if (!loop)
            throwSyntaxError(ast->lastSourceLocation(), QCoreApplication::translate("qv4codegen", "Undefined label '%1'").arg(ast->label.toString()));
    }
    unwindException(loop->scopeAndFinally);
    _block->JUMP(loop->breakBlock);
    return false;
}

bool Codegen::visit(ContinueStatement *ast)
{
    Loop *loop = 0;
    if (ast->label.isEmpty()) {
        for (loop = _loop; loop; loop = loop->parent) {
            if (loop->continueBlock)
                break;
        }
    } else {
        for (loop = _loop; loop; loop = loop->parent) {
            if (loop->labelledStatement && loop->labelledStatement->label == ast->label) {
                if (!loop->continueBlock)
                    loop = 0;
                break;
            }
        }
        if (!loop)
            throwSyntaxError(ast->lastSourceLocation(), QCoreApplication::translate("qv4codegen", "Undefined label '%1'").arg(ast->label.toString()));
    }
    if (!loop)
        throwSyntaxError(ast->lastSourceLocation(), QCoreApplication::translate("qv4codegen", "continue outside of loop"));
    unwindException(loop->scopeAndFinally);
    _block->JUMP(loop->continueBlock);
    return false;
}

bool Codegen::visit(DebuggerStatement *)
{
    Q_UNIMPLEMENTED();
    return false;
}

bool Codegen::visit(DoWhileStatement *ast)
{
    V4IR::BasicBlock *loopbody = _function->newBasicBlock();
    V4IR::BasicBlock *loopcond = _function->newBasicBlock();
    V4IR::BasicBlock *loopend = _function->newBasicBlock();

    enterLoop(ast, loopend, loopcond);

    _block->JUMP(loopbody);

    _block = loopbody;
    statement(ast->statement);
    _block->JUMP(loopcond);

    _block = loopcond;
    condition(ast->expression, loopbody, loopend);

    _block = loopend;

    leaveLoop();

    return false;
}

bool Codegen::visit(EmptyStatement *)
{
    return false;
}

bool Codegen::visit(ExpressionStatement *ast)
{
    if (_mode == EvalCode) {
        Result e = expression(ast->expression);
        if (*e)
            move(_block->TEMP(_returnAddress), *e);
    } else {
        statement(ast->expression);
    }
    return false;
}

bool Codegen::visit(ForEachStatement *ast)
{
    V4IR::BasicBlock *foreachin = _function->newBasicBlock();
    V4IR::BasicBlock *foreachbody = _function->newBasicBlock();
    V4IR::BasicBlock *foreachend = _function->newBasicBlock();

    enterLoop(ast, foreachend, foreachin);

    int objectToIterateOn = _block->newTemp();
    move(_block->TEMP(objectToIterateOn), *expression(ast->expression));
    V4IR::ExprList *args = _function->New<V4IR::ExprList>();
    args->init(_block->TEMP(objectToIterateOn));

    int iterator = _block->newTemp();
    move(_block->TEMP(iterator), _block->CALL(_block->NAME(V4IR::Name::builtin_foreach_iterator_object, 0, 0), args));

    _block->JUMP(foreachin);

    _block = foreachbody;
    int temp = _block->newTemp();
    move(*expression(ast->initialiser), _block->TEMP(temp));
    statement(ast->statement);
    _block->JUMP(foreachin);

    _block = foreachin;

    args = _function->New<V4IR::ExprList>();
    args->init(_block->TEMP(iterator));
    move(_block->TEMP(temp), _block->CALL(_block->NAME(V4IR::Name::builtin_foreach_next_property_name, 0, 0), args));
    int null = _block->newTemp();
    move(_block->TEMP(null), _block->CONST(V4IR::NullType, 0));
    cjump(_block->BINOP(V4IR::OpStrictNotEqual, _block->TEMP(temp), _block->TEMP(null)), foreachbody, foreachend);
    _block = foreachend;

    leaveLoop();
    return false;
}

bool Codegen::visit(ForStatement *ast)
{
    V4IR::BasicBlock *forcond = _function->newBasicBlock();
    V4IR::BasicBlock *forbody = _function->newBasicBlock();
    V4IR::BasicBlock *forstep = _function->newBasicBlock();
    V4IR::BasicBlock *forend = _function->newBasicBlock();

    enterLoop(ast, forend, forstep);

    statement(ast->initialiser);
    _block->JUMP(forcond);

    _block = forcond;
    if (ast->condition)
        condition(ast->condition, forbody, forend);
    else
        _block->JUMP(forbody);

    _block = forbody;
    statement(ast->statement);
    _block->JUMP(forstep);

    _block = forstep;
    statement(ast->expression);
    _block->JUMP(forcond);
    _block = forend;

    leaveLoop();

    return false;
}

bool Codegen::visit(IfStatement *ast)
{
    V4IR::BasicBlock *iftrue = _function->newBasicBlock();
    V4IR::BasicBlock *iffalse = ast->ko ? _function->newBasicBlock() : 0;
    V4IR::BasicBlock *endif = _function->newBasicBlock();
    condition(ast->expression, iftrue, ast->ko ? iffalse : endif);

    _block = iftrue;
    statement(ast->ok);
    _block->JUMP(endif);

    if (ast->ko) {
        _block = iffalse;
        statement(ast->ko);
        _block->JUMP(endif);
    }

    _block = endif;

    return false;
}

bool Codegen::visit(LabelledStatement *ast)
{
    _labelledStatement = ast;

    if (AST::cast<AST::SwitchStatement *>(ast->statement) ||
            AST::cast<AST::WhileStatement *>(ast->statement) ||
            AST::cast<AST::DoWhileStatement *>(ast->statement) ||
            AST::cast<AST::ForStatement *>(ast->statement) ||
            AST::cast<AST::ForEachStatement *>(ast->statement) ||
            AST::cast<AST::LocalForStatement *>(ast->statement) ||
            AST::cast<AST::LocalForEachStatement *>(ast->statement)) {
        statement(ast->statement); // labelledStatement will be associated with the ast->statement's loop.
    } else {
        V4IR::BasicBlock *breakBlock = _function->newBasicBlock();
        enterLoop(ast->statement, breakBlock, /*continueBlock*/ 0);
        statement(ast->statement);
        _block->JUMP(breakBlock);
        _block = breakBlock;
        leaveLoop();
    }

    return false;
}

bool Codegen::visit(LocalForEachStatement *ast)
{
    V4IR::BasicBlock *foreachin = _function->newBasicBlock();
    V4IR::BasicBlock *foreachbody = _function->newBasicBlock();
    V4IR::BasicBlock *foreachend = _function->newBasicBlock();

    enterLoop(ast, foreachend, foreachin);

    variableDeclaration(ast->declaration);

    int iterator = _block->newTemp();
    move(_block->TEMP(iterator), *expression(ast->expression));
    V4IR::ExprList *args = _function->New<V4IR::ExprList>();
    args->init(_block->TEMP(iterator));
    move(_block->TEMP(iterator), _block->CALL(_block->NAME(V4IR::Name::builtin_foreach_iterator_object, 0, 0), args));

    _block->JUMP(foreachin);

    _block = foreachbody;
    int temp = _block->newTemp();
    move(identifier(ast->declaration->name.toString()), _block->TEMP(temp));
    statement(ast->statement);
    _block->JUMP(foreachin);

    _block = foreachin;

    args = _function->New<V4IR::ExprList>();
    args->init(_block->TEMP(iterator));
    move(_block->TEMP(temp), _block->CALL(_block->NAME(V4IR::Name::builtin_foreach_next_property_name, 0, 0), args));
    int null = _block->newTemp();
    move(_block->TEMP(null), _block->CONST(V4IR::NullType, 0));
    cjump(_block->BINOP(V4IR::OpStrictNotEqual, _block->TEMP(temp), _block->TEMP(null)), foreachbody, foreachend);
    _block = foreachend;

    leaveLoop();
    return false;
}

bool Codegen::visit(LocalForStatement *ast)
{
    V4IR::BasicBlock *forcond = _function->newBasicBlock();
    V4IR::BasicBlock *forbody = _function->newBasicBlock();
    V4IR::BasicBlock *forstep = _function->newBasicBlock();
    V4IR::BasicBlock *forend = _function->newBasicBlock();

    enterLoop(ast, forend, forstep);

    variableDeclarationList(ast->declarations);
    _block->JUMP(forcond);

    _block = forcond;
    if (ast->condition)
        condition(ast->condition, forbody, forend);
    else
        _block->JUMP(forbody);

    _block = forbody;
    statement(ast->statement);
    _block->JUMP(forstep);

    _block = forstep;
    statement(ast->expression);
    _block->JUMP(forcond);
    _block = forend;

    leaveLoop();

    return false;
}

bool Codegen::visit(ReturnStatement *ast)
{
    if (_mode != FunctionCode)
        throwSyntaxError(ast->returnToken, QCoreApplication::translate("qv4codegen", "Return statement outside of function"));
    if (ast->expression) {
        Result expr = expression(ast->expression);
        move(_block->TEMP(_returnAddress), *expr);
    }
    unwindException(0);

    _block->JUMP(_exitBlock);
    return false;
}

bool Codegen::visit(SwitchStatement *ast)
{
    V4IR::BasicBlock *switchend = _function->newBasicBlock();

    if (ast->block) {
        Result lhs = expression(ast->expression);
        V4IR::BasicBlock *switchcond = _block;
        V4IR::BasicBlock *previousBlock = 0;

        QHash<Node *, V4IR::BasicBlock *> blockMap;

        enterLoop(ast, switchend, 0);

        for (CaseClauses *it = ast->block->clauses; it; it = it->next) {
            CaseClause *clause = it->clause;

            _block = _function->newBasicBlock();
            blockMap[clause] = _block;

            if (previousBlock && !previousBlock->isTerminated())
                previousBlock->JUMP(_block);

            for (StatementList *it2 = clause->statements; it2; it2 = it2->next)
                statement(it2->statement);

            previousBlock = _block;
        }

        if (ast->block->defaultClause) {
            _block = _function->newBasicBlock();
            blockMap[ast->block->defaultClause] = _block;

            if (previousBlock && !previousBlock->isTerminated())
                previousBlock->JUMP(_block);

            for (StatementList *it2 = ast->block->defaultClause->statements; it2; it2 = it2->next)
                statement(it2->statement);

            previousBlock = _block;
        }

        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next) {
            CaseClause *clause = it->clause;

            _block = _function->newBasicBlock();
            blockMap[clause] = _block;

            if (previousBlock && !previousBlock->isTerminated())
                previousBlock->JUMP(_block);

            for (StatementList *it2 = clause->statements; it2; it2 = it2->next)
                statement(it2->statement);

            previousBlock = _block;
        }

        leaveLoop();

        _block->JUMP(switchend);

        _block = switchcond;
        for (CaseClauses *it = ast->block->clauses; it; it = it->next) {
            CaseClause *clause = it->clause;
            Result rhs = expression(clause->expression);
            V4IR::BasicBlock *iftrue = blockMap[clause];
            V4IR::BasicBlock *iffalse = _function->newBasicBlock();
            cjump(binop(V4IR::OpStrictEqual, *lhs, *rhs), iftrue, iffalse);
            _block = iffalse;
        }

        for (CaseClauses *it = ast->block->moreClauses; it; it = it->next) {
            CaseClause *clause = it->clause;
            Result rhs = expression(clause->expression);
            V4IR::BasicBlock *iftrue = blockMap[clause];
            V4IR::BasicBlock *iffalse = _function->newBasicBlock();
            cjump(binop(V4IR::OpStrictEqual, *lhs, *rhs), iftrue, iffalse);
            _block = iffalse;
        }

        if (ast->block->defaultClause) {
            _block->JUMP(blockMap[ast->block->defaultClause]);
        }
    }

    _block->JUMP(switchend);

    _block = switchend;
    return false;
}

bool Codegen::visit(ThrowStatement *ast)
{
    Result expr = expression(ast->expression);
    move(_block->TEMP(_returnAddress), *expr);
    _block->JUMP(_throwBlock);
    return false;
}

bool Codegen::visit(TryStatement *ast)
{
    _function->hasTry = true;

    if (_function->isStrict && ast->catchExpression &&
            (ast->catchExpression->name == QLatin1String("eval") || ast->catchExpression->name == QLatin1String("arguments")))
        throwSyntaxError(ast->catchExpression->identifierToken, QCoreApplication::translate("qv4codegen", "Catch variable name may not be eval or arguments in strict mode"));

    V4IR::BasicBlock *tryBody = _function->newBasicBlock();
    V4IR::BasicBlock *catchBody =  _function->newBasicBlock();
    // We always need a finally body to clean up the exception handler
    V4IR::BasicBlock *finallyBody = _function->newBasicBlock();

    V4IR::BasicBlock *throwBlock = _function->newBasicBlock();
    V4IR::ExprList *throwArgs = _function->New<V4IR::ExprList>();
    throwArgs->expr = throwBlock->TEMP(_returnAddress);
    throwBlock->EXP(throwBlock->CALL(throwBlock->NAME(V4IR::Name::builtin_throw, /*line*/0, /*column*/0), throwArgs));
    throwBlock->JUMP(catchBody);
    qSwap(_throwBlock, throwBlock);

    int hasException = _block->newTemp();
    move(_block->TEMP(hasException), _block->CONST(V4IR::BoolType, false));

    // Pass the hidden "needRethrow" TEMP to the
    // builtin_delete_exception_handler, in order to have those TEMPs alive for
    // the duration of the exception handling block.
    V4IR::ExprList *finishTryArgs = _function->New<V4IR::ExprList>();
    finishTryArgs->init(_block->TEMP(hasException));

    ScopeAndFinally tcf(_scopeAndFinally, ast->finallyExpression, finishTryArgs);
    _scopeAndFinally = &tcf;

    int exception_to_rethrow  = _block->newTemp();

    _block->TRY(tryBody, catchBody,
                ast->catchExpression ? ast->catchExpression->name.toString() : QString(),
                _block->TEMP(exception_to_rethrow));

    _block = tryBody;
    statement(ast->statement);
    _block->JUMP(finallyBody);

    _block = catchBody;

    if (ast->catchExpression) {
        // check if an exception got thrown within catch. Go to finally
        // and then rethrow
        V4IR::BasicBlock *b = _function->newBasicBlock();
        _block->CJUMP(_block->TEMP(hasException), finallyBody, b);
        _block = b;
    }

    move(_block->TEMP(hasException), _block->CONST(V4IR::BoolType, true));

    if (ast->catchExpression) {
        ++_function->insideWithOrCatch;
        {
            ScopeAndFinally scope(_scopeAndFinally, ScopeAndFinally::CatchScope);
            _scopeAndFinally = &scope;
            statement(ast->catchExpression->statement);
            _scopeAndFinally = scope.parent;
        }
        --_function->insideWithOrCatch;
        move(_block->TEMP(hasException), _block->CONST(V4IR::BoolType, false));
    }
    _block->JUMP(finallyBody);

    _scopeAndFinally = tcf.parent;

    qSwap(_throwBlock, throwBlock);

    V4IR::BasicBlock *after = _function->newBasicBlock();
    _block = finallyBody;

    _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_finish_try, 0, 0), finishTryArgs));

    if (ast->finallyExpression && ast->finallyExpression->statement)
        statement(ast->finallyExpression->statement);

    V4IR::BasicBlock *rethrowBlock = _function->newBasicBlock();
    _block->CJUMP(_block->TEMP(hasException), rethrowBlock, after);
    _block = rethrowBlock;
    move(_block->TEMP(_returnAddress), _block->TEMP(exception_to_rethrow));
    _block->JUMP(_throwBlock);

    _block = after;

    return false;
}

void Codegen::unwindException(Codegen::ScopeAndFinally *outest)
{
    int savedDepthForWidthOrCatch = _function->insideWithOrCatch;
    ScopeAndFinally *scopeAndFinally = _scopeAndFinally;
    qSwap(_scopeAndFinally, scopeAndFinally);
    while (_scopeAndFinally != outest) {
        switch (_scopeAndFinally->type) {
        case ScopeAndFinally::WithScope:
            _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_pop_scope, 0, 0)));
            // fall through
        case ScopeAndFinally::CatchScope:
            _scopeAndFinally = _scopeAndFinally->parent;
            --_function->insideWithOrCatch;
            break;
        case ScopeAndFinally::TryScope: {
            _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_finish_try, 0, 0), _scopeAndFinally->finishTryArgs));
            ScopeAndFinally *tc = _scopeAndFinally;
            _scopeAndFinally = tc->parent;
            if (tc->finally && tc->finally->statement)
                statement(tc->finally->statement);
            break;
        }
        }
    }
    qSwap(_scopeAndFinally, scopeAndFinally);
    _function->insideWithOrCatch = savedDepthForWidthOrCatch;
}

bool Codegen::visit(VariableStatement *ast)
{
    variableDeclarationList(ast->declarations);
    return false;
}

bool Codegen::visit(WhileStatement *ast)
{
    V4IR::BasicBlock *whilecond = _function->newBasicBlock();
    V4IR::BasicBlock *whilebody = _function->newBasicBlock();
    V4IR::BasicBlock *whileend = _function->newBasicBlock();

    enterLoop(ast, whileend, whilecond);

    _block->JUMP(whilecond);
    _block = whilecond;
    condition(ast->expression, whilebody, whileend);

    _block = whilebody;
    statement(ast->statement);
    _block->JUMP(whilecond);

    _block = whileend;
    leaveLoop();

    return false;
}

bool Codegen::visit(WithStatement *ast)
{
    _function->hasWith = true;

    V4IR::BasicBlock *withBlock = _function->newBasicBlock();

    _block->JUMP(withBlock);
    _block = withBlock;
    int withObject = _block->newTemp();
    _block->MOVE(_block->TEMP(withObject), *expression(ast->expression));
    V4IR::ExprList *args = _function->New<V4IR::ExprList>();
    args->init(_block->TEMP(withObject));
    _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_push_with_scope, 0, 0), args));

    ++_function->insideWithOrCatch;
    {
        ScopeAndFinally scope(_scopeAndFinally);
        _scopeAndFinally = &scope;
        statement(ast->statement);
        _scopeAndFinally = scope.parent;
    }
    --_function->insideWithOrCatch;
    _block->EXP(_block->CALL(_block->NAME(V4IR::Name::builtin_pop_scope, 0, 0), 0));

    V4IR::BasicBlock *next = _function->newBasicBlock();
    _block->JUMP(next);
    _block = next;

    return false;
}

bool Codegen::visit(UiArrayBinding *)
{
    assert(!"not implemented");
    return false;
}

bool Codegen::visit(UiObjectBinding *)
{
    assert(!"not implemented");
    return false;
}

bool Codegen::visit(UiObjectDefinition *)
{
    assert(!"not implemented");
    return false;
}

bool Codegen::visit(UiPublicMember *)
{
    assert(!"not implemented");
    return false;
}

bool Codegen::visit(UiScriptBinding *)
{
    assert(!"not implemented");
    return false;
}

bool Codegen::visit(UiSourceElement *)
{
    assert(!"not implemented");
    return false;
}

void Codegen::throwSyntaxErrorOnEvalOrArgumentsInStrictMode(V4IR::Expr *expr, const SourceLocation& loc)
{
    if (!_env->isStrict)
        return;
    V4IR::Name *n = expr->asName();
    if (!n)
        return;
    if (*n->id == QLatin1String("eval") || *n->id == QLatin1String("arguments"))
        throwSyntaxError(loc, QCoreApplication::translate("qv4codegen", "Variable name may not be eval or arguments in strict mode"));
}

void Codegen::throwSyntaxError(const SourceLocation &loc, const QString &detail)
{
    QV4::DiagnosticMessage *msg = new QV4::DiagnosticMessage;
    msg->fileName = _fileName;
    msg->offset = loc.begin();
    msg->startLine = loc.startLine;
    msg->startColumn = loc.startColumn;
    msg->message = detail;
    if (_context)
        _context->throwSyntaxError(msg);
    else if (_errorHandler)
        _errorHandler->syntaxError(msg);
    else
        Q_ASSERT(!"No error handler available.");
}

void Codegen::throwReferenceError(const SourceLocation &loc, const QString &detail)
{
    if (_context)
        _context->throwReferenceError(QV4::Value::fromString(_context, detail));
    else if (_errorHandler)
        throwSyntaxError(loc, detail);
    else
        Q_ASSERT(!"No error handler available.");
}
