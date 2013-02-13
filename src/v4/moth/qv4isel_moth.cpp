#include "qv4isel_util_p.h"
#include "qv4isel_moth_p.h"
#include "qv4vme_moth_p.h"
#include "qv4functionobject.h"
#include "qv4regexpobject.h"
#include "debugging.h"

using namespace QQmlJS;
using namespace QQmlJS::Moth;

namespace {

QTextStream qout(stderr, QIODevice::WriteOnly);

//#define DEBUG_TEMP_COMPRESSION
#ifdef DEBUG_TEMP_COMPRESSION
#  define DBTC(x) x
#else // !DEBUG_TEMP_COMPRESSION
#  define DBTC(x)
#endif // DEBUG_TEMP_COMPRESSION
class CompressTemps: public IR::StmtVisitor, IR::ExprVisitor
{
public:
    void run(IR::Function *function)
    {
        DBTC(qDebug() << "starting on function" << (*function->name) << "with" << function->tempCount << "temps.";)

        _seenTemps.clear();
        _nextFree = 0;
        _active.reserve(function->tempCount);
        _localCount = function->locals.size();

        QVector<int> pinned;
        foreach (IR::BasicBlock *block, function->basicBlocks) {
            if (IR::Stmt *last = block->terminator()) {
                const QBitArray &liveOut = last->d->liveOut;
                for (int i = 0, ei = liveOut.size(); i < ei; ++i) {
                    if (liveOut.at(i) && !pinned.contains(i)) {
                        pinned.append(i);
                        add(i - _localCount, _nextFree);
                    }
                }
            }
        }
        _pinnedCount = _nextFree;

        int maxUsed = _nextFree;

        foreach (IR::BasicBlock *block, function->basicBlocks) {
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

private:
    virtual void visitConst(IR::Const *) {}
    virtual void visitString(IR::String *) {}
    virtual void visitRegExp(IR::RegExp *) {}
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
        if (_seenTemps.contains(e))
            return;
        _seenTemps.insert(e);

        if (e->index < 0)
            return;
        if (e->index < _localCount) // don't optimise locals yet.
            return;

        e->index = remap(e->index - _localCount) + _localCount;
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
        s->target->accept(this);
        s->source->accept(this);
    }

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
    QSet<IR::Temp *> _seenTemps;
    IR::Stmt *_currentStatement;
    int _localCount;
    int _nextFree;
    int _pinnedCount;
};
#undef DBTC

typedef VM::Value (*ALUFunction)(const VM::Value, const VM::Value, VM::ExecutionContext*);
inline ALUFunction aluOpFunction(IR::AluOp op)
{
    switch (op) {
    case IR::OpInvalid:
        return 0;
    case IR::OpIfTrue:
        return 0;
    case IR::OpNot:
        return 0;
    case IR::OpUMinus:
        return 0;
    case IR::OpUPlus:
        return 0;
    case IR::OpCompl:
        return 0;
    case IR::OpBitAnd:
        return VM::__qmljs_bit_and;
    case IR::OpBitOr:
        return VM::__qmljs_bit_or;
    case IR::OpBitXor:
        return VM::__qmljs_bit_xor;
    case IR::OpAdd:
        return VM::__qmljs_add;
    case IR::OpSub:
        return VM::__qmljs_sub;
    case IR::OpMul:
        return VM::__qmljs_mul;
    case IR::OpDiv:
        return VM::__qmljs_div;
    case IR::OpMod:
        return VM::__qmljs_mod;
    case IR::OpLShift:
        return VM::__qmljs_shl;
    case IR::OpRShift:
        return VM::__qmljs_shr;
    case IR::OpURShift:
        return VM::__qmljs_ushr;
    case IR::OpGt:
        return VM::__qmljs_gt;
    case IR::OpLt:
        return VM::__qmljs_lt;
    case IR::OpGe:
        return VM::__qmljs_ge;
    case IR::OpLe:
        return VM::__qmljs_le;
    case IR::OpEqual:
        return VM::__qmljs_eq;
    case IR::OpNotEqual:
        return VM::__qmljs_ne;
    case IR::OpStrictEqual:
        return VM::__qmljs_se;
    case IR::OpStrictNotEqual:
        return VM::__qmljs_sne;
    case IR::OpInstanceof:
        return VM::__qmljs_instanceof;
    case IR::OpIn:
        return VM::__qmljs_in;
    case IR::OpAnd:
        return 0;
    case IR::OpOr:
        return 0;
    default:
        assert(!"Unknown AluOp");
        return 0;
    }
};

} // anonymous namespace

InstructionSelection::InstructionSelection(VM::ExecutionEngine *engine, IR::Module *module)
    : EvalInstructionSelection(engine, module)
    , _function(0)
    , _vmFunction(0)
    , _block(0)
    , _codeStart(0)
    , _codeNext(0)
    , _codeEnd(0)
{
}

InstructionSelection::~InstructionSelection()
{
}

void InstructionSelection::run(VM::Function *vmFunction, IR::Function *function)
{
    IR::BasicBlock *block;

    QHash<IR::BasicBlock *, QVector<ptrdiff_t> > patches;
    QHash<IR::BasicBlock *, ptrdiff_t> addrs;

    int codeSize = 4096;
    uchar *codeStart = new uchar[codeSize];
    uchar *codeNext = codeStart;
    uchar *codeEnd = codeStart + codeSize;

    qSwap(_function, function);
    qSwap(_vmFunction, vmFunction);
    qSwap(block, _block);
    qSwap(patches, _patches);
    qSwap(addrs, _addrs);
    qSwap(codeStart, _codeStart);
    qSwap(codeNext, _codeNext);
    qSwap(codeEnd, _codeEnd);

    // TODO: FIXME: fix the temp compression with the new temp index layout.
//    CompressTemps().run(_function);

    int locals = frameSize();
    assert(locals >= 0);

    Instruction::Push push;
    push.value = quint32(locals);
    addInstruction(push);

    foreach (_block, _function->basicBlocks) {
        _addrs.insert(_block, _codeNext - _codeStart);

        foreach (IR::Stmt *s, _block->statements)
            s->accept(this);
    }

    patchJumpAddresses();

    _vmFunction->code = VME::exec;
    _vmFunction->codeData = squeezeCode();

    qSwap(_function, function);
    qSwap(_vmFunction, vmFunction);
    qSwap(block, _block);
    qSwap(patches, _patches);
    qSwap(addrs, _addrs);
    qSwap(codeStart, _codeStart);
    qSwap(codeNext, _codeNext);
    qSwap(codeEnd, _codeEnd);

    delete[] codeStart;
}

void InstructionSelection::callValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result)
{
    Instruction::CallValue call;
    prepareCallArgs(args, call.argc, call.args);
    call.dest = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result)
{
    // call the property on the loaded base
    Instruction::CallProperty call;
    call.base = getParam(base);
    call.name = identifier(name);
    prepareCallArgs(args, call.argc, call.args);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callSubscript(IR::Temp *base, IR::Temp *index, IR::ExprList *args, IR::Temp *result)
{
    // call the property on the loaded base
    Instruction::CallElement call;
    call.base = getParam(base);
    call.index = getParam(index);
    prepareCallArgs(args, call.argc, call.args);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::constructActivationProperty(IR::Name *func,
                                                       IR::ExprList *args,
                                                       IR::Temp *result)
{
    Instruction::CreateActivationProperty create;
    create.name = identifier(*func->id);
    prepareCallArgs(args, create.argc, create.args);
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::constructProperty(IR::Temp *base, const QString &name, IR::ExprList *args, IR::Temp *result)
{
    Instruction::CreateProperty create;
    create.base = getParam(base);
    create.name = identifier(name);
    prepareCallArgs(args, create.argc, create.args);
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::constructValue(IR::Temp *value, IR::ExprList *args, IR::Temp *result)
{
    Instruction::CreateValue create;
    create.func = getParam(value);
    prepareCallArgs(args, create.argc, create.args);
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::loadThisObject(IR::Temp *temp)
{
    Instruction::LoadThis load;
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::loadConst(IR::Const *sourceConst, IR::Temp *targetTemp)
{
    assert(sourceConst);

    Instruction::LoadValue load;
    load.value = getParam(sourceConst);
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::loadString(const QString &str, IR::Temp *targetTemp)
{
    Instruction::LoadValue load;
    load.value = Instr::Param::createValue(VM::Value::fromString(identifier(str)));
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::loadRegexp(IR::RegExp *sourceRegexp, IR::Temp *targetTemp)
{
    VM::Value v = VM::Value::fromObject(engine()->newRegExpObject(
                                            *sourceRegexp->value,
                                            sourceRegexp->flags));
    _vmFunction->generatedValues.append(v);

    Instruction::LoadValue load;
    load.value = Instr::Param::createValue(v);
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::getActivationProperty(const QString &name, IR::Temp *temp)
{
    Instruction::LoadName load;
    load.name = identifier(name);
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::setActivationProperty(IR::Temp *source, const QString &targetName)
{
    Instruction::StoreName store;
    store.source = getParam(source);
    store.name = identifier(targetName);
    addInstruction(store);
}

void InstructionSelection::initClosure(IR::Closure *closure, IR::Temp *target)
{
    VM::Function *vmFunc = vmFunction(closure->value);
    assert(vmFunc);
    Instruction::LoadClosure load;
    load.value = vmFunc;
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::getProperty(IR::Temp *base, const QString &name, IR::Temp *target)
{
    Instruction::LoadProperty load;
    load.base = getParam(base);
    load.name = identifier(name);
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::setProperty(IR::Expr *source, IR::Temp *targetBase, const QString &targetName)
{
    Instruction::StoreProperty store;
    store.base = getParam(targetBase);
    store.name = identifier(targetName);
    store.source = getParam(source);
    addInstruction(store);
}

void InstructionSelection::getElement(IR::Temp *base, IR::Temp *index, IR::Temp *target)
{
    Instruction::LoadElement load;
    load.base = getParam(base);
    load.index = getParam(index);
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::setElement(IR::Expr *source, IR::Temp *targetBase, IR::Temp *targetIndex)
{
    Instruction::StoreElement store;
    store.base = getParam(targetBase);
    store.index = getParam(targetIndex);
    store.source = getParam(source);
    addInstruction(store);
}

void InstructionSelection::copyValue(IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    Instruction::MoveTemp move;
    move.source = getParam(sourceTemp);
    move.result = getResultParam(targetTemp);
    addInstruction(move);
}

void InstructionSelection::unop(IR::AluOp oper, IR::Temp *sourceTemp, IR::Temp *targetTemp)
{
    VM::Value (*op)(const VM::Value value, VM::ExecutionContext *ctx) = 0;
    switch (oper) {
    case IR::OpIfTrue: assert(!"unreachable"); break;
    case IR::OpNot: op = VM::__qmljs_not; break;
    case IR::OpUMinus: op = VM::__qmljs_uminus; break;
    case IR::OpUPlus: op = VM::__qmljs_uplus; break;
    case IR::OpCompl: op = VM::__qmljs_compl; break;
    case IR::OpIncrement: op = VM::__qmljs_increment; break;
    case IR::OpDecrement: op = VM::__qmljs_decrement; break;
    default: assert(!"unreachable"); break;
    } // switch

    if (op) {
        Instruction::Unop unop;
        unop.alu = op;
        unop.source = getParam(sourceTemp);
        unop.result = getResultParam(targetTemp);
        addInstruction(unop);
    } else {
        qWarning("  UNOP1");
    }
}

void InstructionSelection::binop(IR::AluOp oper, IR::Expr *leftSource, IR::Expr *rightSource, IR::Temp *target)
{
    Instruction::Binop binop;
    binop.alu = aluOpFunction(oper);
    binop.lhs = getParam(leftSource);
    binop.rhs = getParam(rightSource);
    binop.result = getResultParam(target);
    addInstruction(binop);
}

void InstructionSelection::inplaceNameOp(IR::AluOp oper, IR::Expr *sourceExpr, const QString &targetName)
{
    void (*op)(VM::Value value, VM::String *name, VM::ExecutionContext *ctx) = 0;
    switch (oper) {
    case IR::OpBitAnd: op = VM::__qmljs_inplace_bit_and_name; break;
    case IR::OpBitOr: op = VM::__qmljs_inplace_bit_or_name; break;
    case IR::OpBitXor: op = VM::__qmljs_inplace_bit_xor_name; break;
    case IR::OpAdd: op = VM::__qmljs_inplace_add_name; break;
    case IR::OpSub: op = VM::__qmljs_inplace_sub_name; break;
    case IR::OpMul: op = VM::__qmljs_inplace_mul_name; break;
    case IR::OpDiv: op = VM::__qmljs_inplace_div_name; break;
    case IR::OpMod: op = VM::__qmljs_inplace_mod_name; break;
    case IR::OpLShift: op = VM::__qmljs_inplace_shl_name; break;
    case IR::OpRShift: op = VM::__qmljs_inplace_shr_name; break;
    case IR::OpURShift: op = VM::__qmljs_inplace_ushr_name; break;
    default: break;
    }

    if (op) {
        Instruction::InplaceNameOp ieo;
        ieo.alu = op;
        ieo.name = identifier(targetName);
        ieo.source = getParam(sourceExpr);
        addInstruction(ieo);
    }
}

void InstructionSelection::inplaceElementOp(IR::AluOp oper, IR::Expr *sourceExpr, IR::Temp *targetBaseTemp, IR::Temp *targetIndexTemp)
{
    void (*op)(VM::Value base, VM::Value index, VM::Value value, VM::ExecutionContext *ctx) = 0;
    switch (oper) {
    case IR::OpBitAnd: op = VM::__qmljs_inplace_bit_and_element; break;
    case IR::OpBitOr: op = VM::__qmljs_inplace_bit_or_element; break;
    case IR::OpBitXor: op = VM::__qmljs_inplace_bit_xor_element; break;
    case IR::OpAdd: op = VM::__qmljs_inplace_add_element; break;
    case IR::OpSub: op = VM::__qmljs_inplace_sub_element; break;
    case IR::OpMul: op = VM::__qmljs_inplace_mul_element; break;
    case IR::OpDiv: op = VM::__qmljs_inplace_div_element; break;
    case IR::OpMod: op = VM::__qmljs_inplace_mod_element; break;
    case IR::OpLShift: op = VM::__qmljs_inplace_shl_element; break;
    case IR::OpRShift: op = VM::__qmljs_inplace_shr_element; break;
    case IR::OpURShift: op = VM::__qmljs_inplace_ushr_element; break;
    default: break;
    }

    Instruction::InplaceElementOp ieo;
    ieo.alu = op;
    ieo.base = getParam(targetBaseTemp);
    ieo.index = getParam(targetIndexTemp);
    ieo.source = getParam(sourceExpr);
    addInstruction(ieo);
}

void InstructionSelection::inplaceMemberOp(IR::AluOp oper, IR::Expr *source, IR::Temp *targetBase, const QString &targetName)
{
    void (*op)(VM::Value value, VM::Value base, VM::String *name, VM::ExecutionContext *ctx) = 0;
    switch (oper) {
    case IR::OpBitAnd: op = VM::__qmljs_inplace_bit_and_member; break;
    case IR::OpBitOr: op = VM::__qmljs_inplace_bit_or_member; break;
    case IR::OpBitXor: op = VM::__qmljs_inplace_bit_xor_member; break;
    case IR::OpAdd: op = VM::__qmljs_inplace_add_member; break;
    case IR::OpSub: op = VM::__qmljs_inplace_sub_member; break;
    case IR::OpMul: op = VM::__qmljs_inplace_mul_member; break;
    case IR::OpDiv: op = VM::__qmljs_inplace_div_member; break;
    case IR::OpMod: op = VM::__qmljs_inplace_mod_member; break;
    case IR::OpLShift: op = VM::__qmljs_inplace_shl_member; break;
    case IR::OpRShift: op = VM::__qmljs_inplace_shr_member; break;
    case IR::OpURShift: op = VM::__qmljs_inplace_ushr_member; break;
    default: break;
    }

    Instruction::InplaceMemberOp imo;
    imo.alu = op;
    imo.base = getParam(targetBase);
    imo.member = identifier(targetName);
    imo.source = getParam(source);
    addInstruction(imo);
}

void InstructionSelection::prepareCallArgs(IR::ExprList *e, quint32 &argc, quint32 &args)
{
    bool singleArgIsTemp = false;
    if (e && e->next == 0) {
        // ok, only 1 argument in the call...
        const int idx = e->expr->asTemp()->index;
        // We can only pass a reference into the stack, which holds temps that
        // are not arguments (idx >= 0) nor locals (idx >= localCound).
        singleArgIsTemp = idx >= _function->locals.size();
    }

    if (singleArgIsTemp) {
        // We pass single arguments as references to the stack, but only if it's not a local or an argument.
        argc = 1;
        args = e->expr->asTemp()->index - _function->locals.size();
    } else if (e) {
        // We need to move all the temps into the function arg array
        int argLocation = outgoingArgumentTempStart();
        assert(argLocation >= 0);
        argc = 0;
        args = argLocation;
        while (e) {
            Instruction::MoveTemp move;
            move.source = getParam(e->expr);
            move.result = Instr::Param::createTemp(argLocation);
            addInstruction(move);
            ++argLocation;
            ++argc;
            e = e->next;
        }
    } else {
        argc = 0;
        args = 0;
    }
}

void InstructionSelection::visitJump(IR::Jump *s)
{
    Instruction::Jump jump;
    jump.offset = 0;
    ptrdiff_t loc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));

    _patches[s->target].append(loc);
}

void InstructionSelection::visitCJump(IR::CJump *s)
{
    Instr::Param condition;
    if (IR::Temp *t = s->cond->asTemp()) {
        condition = getResultParam(t);
    } else if (IR::Binop *b = s->cond->asBinop()) {
        condition = getResultParam(0);
        Instruction::Binop binop;
        binop.alu = aluOpFunction(b->op);
        binop.lhs = getParam(b->left);
        binop.rhs = getParam(b->right);
        binop.result = condition;
        addInstruction(binop);
    } else {
        Q_UNIMPLEMENTED();
    }

    Instruction::CJump jump;
    jump.offset = 0;
    jump.condition = condition;
    ptrdiff_t trueLoc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));
    _patches[s->iftrue].append(trueLoc);

    if (_block->index + 1 != s->iffalse->index) {
        Instruction::Jump jump;
        jump.offset = 0;
        ptrdiff_t falseLoc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));
        _patches[s->iffalse].append(falseLoc);
    }
}

void InstructionSelection::visitRet(IR::Ret *s)
{
    Instruction::Ret ret;
    ret.result = getParam(s->expr);
    addInstruction(ret);
}

void InstructionSelection::callBuiltinInvalid(IR::Name *func, IR::ExprList *args, IR::Temp *result)
{
    Instruction::CallActivationProperty call;
    call.name = identifier(*func->id);
    prepareCallArgs(args, call.argc, call.args);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofMember(IR::Temp *base, const QString &name, IR::Temp *result)
{
    Instruction::CallBuiltinTypeofMember call;
    call.base = getParam(base);
    call.member = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result)
{
    Instruction::CallBuiltinTypeofSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofName(const QString &name, IR::Temp *result)
{
    Instruction::CallBuiltinTypeofName call;
    call.name = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofValue(IR::Temp *value, IR::Temp *result)
{
    Instruction::CallBuiltinTypeofValue call;
    call.value = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteMember(IR::Temp *base, const QString &name, IR::Temp *result)
{
    Instruction::CallBuiltinDeleteMember call;
    call.base = getParam(base);
    call.member = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result)
{
    Instruction::CallBuiltinDeleteSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteName(const QString &name, IR::Temp *result)
{
    Instruction::CallBuiltinDeleteName call;
    call.name = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteValue(IR::Temp *result)
{
    Instruction::LoadValue load;
    load.value = Instr::Param::createValue(VM::Value::fromBoolean(false));
    load.result = getResultParam(result);
    addInstruction(load);
}

void InstructionSelection::callBuiltinPostDecrementMember(IR::Temp *base, const QString &name, IR::Temp *result)
{
    Instruction::CallBuiltinPostDecMember call;
    call.base = getParam(base);
    call.member = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostDecrementSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result)
{
    Instruction::CallBuiltinPostDecSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostDecrementName(const QString &name, IR::Temp *result)
{
    Instruction::CallBuiltinPostDecName call;
    call.name = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostDecrementValue(IR::Temp *value, IR::Temp *result)
{
    Instruction::CallBuiltinPostDecValue call;
    call.value = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostIncrementMember(IR::Temp *base, const QString &name, IR::Temp *result)
{
    Instruction::CallBuiltinPostIncMember call;
    call.base = getParam(base);
    call.member = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostIncrementSubscript(IR::Temp *base, IR::Temp *index, IR::Temp *result)
{
    Instruction::CallBuiltinPostIncSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostIncrementName(const QString &name, IR::Temp *result)
{
    Instruction::CallBuiltinPostIncName call;
    call.name = identifier(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPostIncrementValue(IR::Temp *value, IR::Temp *result)
{
    Instruction::CallBuiltinPostIncValue call;
    call.value = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinThrow(IR::Temp *arg)
{
    Instruction::CallBuiltinThrow call;
    call.arg = getParam(arg);
    addInstruction(call);
}

void InstructionSelection::callBuiltinCreateExceptionHandler(IR::Temp *result, IR::Temp *)
{
    Instruction::CallBuiltinCreateExceptionHandler call;
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteExceptionHandler()
{
    Instruction::CallBuiltinDeleteExceptionHandler call;
    addInstruction(call);
}

void InstructionSelection::callBuiltinGetException(IR::Temp *result)
{
    Instruction::CallBuiltinGetException call;
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinForeachIteratorObject(IR::Temp *arg, IR::Temp *result)
{
    Instruction::CallBuiltinForeachIteratorObject call;
    call.arg = getParam(arg);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinForeachNextPropertyname(IR::Temp *arg, IR::Temp *result)
{
    Instruction::CallBuiltinForeachNextPropertyName call;
    call.arg = getParam(arg);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPushWithScope(IR::Temp *arg)
{
    Instruction::CallBuiltinPushScope call;
    call.arg = getParam(arg);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPushCatchScope(const QString &exceptionVarName)
{
    Instruction::CallBuiltinPushCatchScope call;
    call.varName = identifier(exceptionVarName);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPopScope()
{
    Instruction::CallBuiltinPopScope call;
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeclareVar(bool deletable, const QString &name)
{
    Instruction::CallBuiltinDeclareVar call;
    call.isDeletable = deletable;
    call.varName = identifier(name);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineGetterSetter(IR::Temp *object, const QString &name, IR::Temp *getter, IR::Temp *setter)
{
    Instruction::CallBuiltinDefineGetterSetter call;
    call.object = getParam(object);
    call.name = identifier(name);
    call.getter = getParam(getter);
    call.setter = getParam(setter);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineProperty(IR::Temp *object, const QString &name, IR::Temp *value)
{
    Instruction::CallBuiltinDefineProperty call;
    call.object = getParam(object);
    call.name = identifier(name);
    call.value = getParam(value);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineArrayProperty(IR::Temp *object, int index, IR::Temp *value)
{
    Instruction::CallBuiltinDefineArrayProperty call;
    call.object = getParam(object);
    call.index = index;
    call.value = getParam(value);
    addInstruction(call);
}

ptrdiff_t InstructionSelection::addInstructionHelper(Instr::Type type, Instr &instr)
{
#ifdef MOTH_THREADED_INTERPRETER
    instr.common.code = VME::instructionJumpTable()[static_cast<int>(type)];
#else
    instr.common.instructionType = type;
#endif

    int instructionSize = Instr::size(type);
    if (_codeEnd - _codeNext < instructionSize) {
        int currSize = _codeEnd - _codeStart;
        uchar *newCode = new uchar[currSize * 2];
        ::memset(newCode + currSize, 0, currSize);
        ::memcpy(newCode, _codeStart, currSize);
        _codeNext = _codeNext - _codeStart + newCode;
        delete[] _codeStart;
        _codeStart = newCode;
        _codeEnd = _codeStart + currSize * 2;
    }

    ::memcpy(_codeNext, reinterpret_cast<const char *>(&instr), instructionSize);
    ptrdiff_t ptrOffset = _codeNext - _codeStart;
    _codeNext += instructionSize;

    return ptrOffset;
}

void InstructionSelection::patchJumpAddresses()
{
    typedef QHash<IR::BasicBlock *, QVector<ptrdiff_t> >::ConstIterator PatchIt;
    for (PatchIt i = _patches.begin(), ei = _patches.end(); i != ei; ++i) {
        Q_ASSERT(_addrs.contains(i.key()));
        ptrdiff_t target = _addrs.value(i.key());

        const QVector<ptrdiff_t> &patchList = i.value();
        for (int ii = 0, eii = patchList.count(); ii < eii; ++ii) {
            ptrdiff_t patch = patchList.at(ii);

            *((ptrdiff_t *)(_codeStart + patch)) = target - patch;
        }
    }

    _patches.clear();
    _addrs.clear();
}

uchar *InstructionSelection::squeezeCode() const
{
    int codeSize = _codeNext - _codeStart;
    uchar *squeezed = new uchar[codeSize];
    ::memcpy(squeezed, _codeStart, codeSize);
    return squeezed;
}

VM::String *InstructionSelection::identifier(const QString &s)
{
    VM::String *str = engine()->newIdentifier(s);
    _vmFunction->identifiers.append(str);
    return str;
}
