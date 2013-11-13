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

#include "qv4isel_util_p.h"
#include "qv4isel_moth_p.h"
#include "qv4vme_moth_p.h"
#include "qv4ssa_p.h"
#include <private/qv4debugging_p.h>
#include <private/qv4function_p.h>
#include <private/qv4regexpobject_p.h>
#include <private/qv4compileddata_p.h>

#undef USE_TYPE_INFO

using namespace QQmlJS;
using namespace QQmlJS::Moth;

namespace {

inline QV4::BinOp aluOpFunction(V4IR::AluOp op)
{
    switch (op) {
    case V4IR::OpInvalid:
        return 0;
    case V4IR::OpIfTrue:
        return 0;
    case V4IR::OpNot:
        return 0;
    case V4IR::OpUMinus:
        return 0;
    case V4IR::OpUPlus:
        return 0;
    case V4IR::OpCompl:
        return 0;
    case V4IR::OpBitAnd:
        return QV4::__qmljs_bit_and;
    case V4IR::OpBitOr:
        return QV4::__qmljs_bit_or;
    case V4IR::OpBitXor:
        return QV4::__qmljs_bit_xor;
    case V4IR::OpAdd:
        return 0;
    case V4IR::OpSub:
        return QV4::__qmljs_sub;
    case V4IR::OpMul:
        return QV4::__qmljs_mul;
    case V4IR::OpDiv:
        return QV4::__qmljs_div;
    case V4IR::OpMod:
        return QV4::__qmljs_mod;
    case V4IR::OpLShift:
        return QV4::__qmljs_shl;
    case V4IR::OpRShift:
        return QV4::__qmljs_shr;
    case V4IR::OpURShift:
        return QV4::__qmljs_ushr;
    case V4IR::OpGt:
        return QV4::__qmljs_gt;
    case V4IR::OpLt:
        return QV4::__qmljs_lt;
    case V4IR::OpGe:
        return QV4::__qmljs_ge;
    case V4IR::OpLe:
        return QV4::__qmljs_le;
    case V4IR::OpEqual:
        return QV4::__qmljs_eq;
    case V4IR::OpNotEqual:
        return QV4::__qmljs_ne;
    case V4IR::OpStrictEqual:
        return QV4::__qmljs_se;
    case V4IR::OpStrictNotEqual:
        return QV4::__qmljs_sne;
    case V4IR::OpInstanceof:
        return 0;
    case V4IR::OpIn:
        return 0;
    case V4IR::OpAnd:
        return 0;
    case V4IR::OpOr:
        return 0;
    default:
        assert(!"Unknown AluOp");
        return 0;
    }
};

inline bool isNumberType(V4IR::Expr *e)
{
    switch (e->type) {
    case V4IR::SInt32Type:
    case V4IR::UInt32Type:
    case V4IR::DoubleType:
        return true;
    default:
        return false;
    }
}

inline bool isIntegerType(V4IR::Expr *e)
{
    switch (e->type) {
    case V4IR::SInt32Type:
    case V4IR::UInt32Type:
        return true;
    default:
        return false;
    }
}

inline bool isBoolType(V4IR::Expr *e)
{
    return (e->type == V4IR::BoolType);
}

} // anonymous namespace

InstructionSelection::InstructionSelection(QV4::ExecutableAllocator *execAllocator, V4IR::Module *module, QV4::Compiler::JSUnitGenerator *jsGenerator)
    : EvalInstructionSelection(execAllocator, module, jsGenerator)
    , _block(0)
    , _codeStart(0)
    , _codeNext(0)
    , _codeEnd(0)
    , _currentStatement(0)
{
    compilationUnit = new CompilationUnit;
}

InstructionSelection::~InstructionSelection()
{
}

void InstructionSelection::run(int functionIndex)
{
    V4IR::Function *function = irModule->functions[functionIndex];
    V4IR::BasicBlock *block = 0, *nextBlock = 0;

    QHash<V4IR::BasicBlock *, QVector<ptrdiff_t> > patches;
    QHash<V4IR::BasicBlock *, ptrdiff_t> addrs;

    int codeSize = 4096;
    uchar *codeStart = new uchar[codeSize];
    memset(codeStart, 0, codeSize);
    uchar *codeNext = codeStart;
    uchar *codeEnd = codeStart + codeSize;

    qSwap(_function, function);
    qSwap(block, _block);
    qSwap(nextBlock, _nextBlock);
    qSwap(patches, _patches);
    qSwap(addrs, _addrs);
    qSwap(codeStart, _codeStart);
    qSwap(codeNext, _codeNext);
    qSwap(codeEnd, _codeEnd);

    V4IR::Optimizer opt(_function);
    opt.run();
    if (opt.isInSSA()) {
        opt.convertOutOfSSA();
        opt.showMeTheCode(_function);
    }
    ConvertTemps().toStackSlots(_function);

    QSet<V4IR::Jump *> removableJumps = opt.calculateOptionalJumps();
    qSwap(_removableJumps, removableJumps);

    V4IR::Stmt *cs = 0;
    qSwap(_currentStatement, cs);

    int locals = frameSize();
    assert(locals >= 0);

    V4IR::BasicBlock *exceptionHandler = 0;

    Instruction::Push push;
    push.value = quint32(locals);
    addInstruction(push);

    QVector<uint> lineNumberMappings;
    lineNumberMappings.reserve(_function->basicBlocks.size() * 2);

    for (int i = 0, ei = _function->basicBlocks.size(); i != ei; ++i) {
        _block = _function->basicBlocks[i];
        _nextBlock = (i < ei - 1) ? _function->basicBlocks[i + 1] : 0;
        _addrs.insert(_block, _codeNext - _codeStart);

        if (_block->catchBlock != exceptionHandler) {
            Instruction::SetExceptionHandler set;
            set.offset = 0;
            if (_block->catchBlock) {
                ptrdiff_t loc = addInstruction(set) + (((const char *)&set.offset) - ((const char *)&set));
                _patches[_block->catchBlock].append(loc);
            } else {
                addInstruction(set);
            }
            exceptionHandler = _block->catchBlock;
        }

        foreach (V4IR::Stmt *s, _block->statements) {
            _currentStatement = s;

            if (s->location.isValid())
                lineNumberMappings << _codeNext - _codeStart << s->location.startLine;

            s->accept(this);
        }
    }

    jsGenerator->registerLineNumberMapping(_function, lineNumberMappings);

    // TODO: patch stack size (the push instruction)
    patchJumpAddresses();

    codeRefs.insert(_function, squeezeCode());

    qSwap(_currentStatement, cs);
    qSwap(_removableJumps, removableJumps);
    qSwap(_function, function);
    qSwap(block, _block);
    qSwap(nextBlock, _nextBlock);
    qSwap(patches, _patches);
    qSwap(addrs, _addrs);
    qSwap(codeStart, _codeStart);
    qSwap(codeNext, _codeNext);
    qSwap(codeEnd, _codeEnd);

    delete[] codeStart;
}

QV4::CompiledData::CompilationUnit *InstructionSelection::backendCompileStep()
{
    compilationUnit->codeRefs.resize(irModule->functions.size());
    int i = 0;
    foreach (V4IR::Function *irFunction, irModule->functions)
        compilationUnit->codeRefs[i++] = codeRefs[irFunction];
    return compilationUnit;
}

void InstructionSelection::callValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result)
{
    Instruction::CallValue call;
    prepareCallArgs(args, call.argc);
    call.callData = callDataStart();
    call.dest = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callProperty(V4IR::Expr *base, const QString &name, V4IR::ExprList *args,
                                        V4IR::Temp *result)
{
    if (useFastLookups) {
        Instruction::CallPropertyLookup call;
        call.base = getParam(base);
        call.lookupIndex = registerGetterLookup(name);
        prepareCallArgs(args, call.argc);
        call.callData = callDataStart();
        call.result = getResultParam(result);
        addInstruction(call);
    } else {
        // call the property on the loaded base
        Instruction::CallProperty call;
        call.base = getParam(base);
        call.name = registerString(name);
        prepareCallArgs(args, call.argc);
        call.callData = callDataStart();
        call.result = getResultParam(result);
        addInstruction(call);
    }
}

void InstructionSelection::callSubscript(V4IR::Expr *base, V4IR::Expr *index, V4IR::ExprList *args,
                                         V4IR::Temp *result)
{
    // call the property on the loaded base
    Instruction::CallElement call;
    call.base = getParam(base);
    call.index = getParam(index);
    prepareCallArgs(args, call.argc);
    call.callData = callDataStart();
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::convertType(V4IR::Temp *source, V4IR::Temp *target)
{
    // FIXME: do something more useful with this info
    if (target->type & V4IR::NumberType)
        unop(V4IR::OpUPlus, source, target);
    else
        copyValue(source, target);
}

void InstructionSelection::constructActivationProperty(V4IR::Name *func,
                                                       V4IR::ExprList *args,
                                                       V4IR::Temp *result)
{
    if (useFastLookups && func->global) {
        Instruction::ConstructGlobalLookup call;
        call.index = registerGlobalGetterLookup(*func->id);
        prepareCallArgs(args, call.argc);
        call.callData = callDataStart();
        call.result = getResultParam(result);
        addInstruction(call);
        return;
    }
    Instruction::CreateActivationProperty create;
    create.name = registerString(*func->id);
    prepareCallArgs(args, create.argc);
    create.callData = callDataStart();
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::constructProperty(V4IR::Temp *base, const QString &name, V4IR::ExprList *args, V4IR::Temp *result)
{
    if (useFastLookups) {
        Instruction::ConstructPropertyLookup call;
        call.base = getParam(base);
        call.index = registerGetterLookup(name);
        prepareCallArgs(args, call.argc);
        call.callData = callDataStart();
        call.result = getResultParam(result);
        addInstruction(call);
        return;
    }
    Instruction::CreateProperty create;
    create.base = getParam(base);
    create.name = registerString(name);
    prepareCallArgs(args, create.argc);
    create.callData = callDataStart();
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::constructValue(V4IR::Temp *value, V4IR::ExprList *args, V4IR::Temp *result)
{
    Instruction::CreateValue create;
    create.func = getParam(value);
    prepareCallArgs(args, create.argc);
    create.callData = callDataStart();
    create.result = getResultParam(result);
    addInstruction(create);
}

void InstructionSelection::loadThisObject(V4IR::Temp *temp)
{
    Instruction::LoadThis load;
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::loadQmlIdObject(int id, V4IR::Temp *temp)
{
    Instruction::LoadQmlIdObject load;
    load.result = getResultParam(temp);
    load.id = id;
    addInstruction(load);
}

void InstructionSelection::loadQmlImportedScripts(V4IR::Temp *temp)
{
    Instruction::LoadQmlImportedScripts load;
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::loadQmlContextObject(V4IR::Temp *temp)
{
    Instruction::LoadQmlContextObject load;
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::loadQmlScopeObject(V4IR::Temp *temp)
{
    Instruction::LoadQmlScopeObject load;
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::loadConst(V4IR::Const *sourceConst, V4IR::Temp *targetTemp)
{
    assert(sourceConst);

    Instruction::Move move;
    move.source = getParam(sourceConst);
    move.result = getResultParam(targetTemp);
    addInstruction(move);
}

void InstructionSelection::loadString(const QString &str, V4IR::Temp *targetTemp)
{
    Instruction::LoadRuntimeString load;
    load.stringId = registerString(str);
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::loadRegexp(V4IR::RegExp *sourceRegexp, V4IR::Temp *targetTemp)
{
    Instruction::LoadRegExp load;
    load.regExpId = registerRegExp(sourceRegexp);
    load.result = getResultParam(targetTemp);
    addInstruction(load);
}

void InstructionSelection::getActivationProperty(const V4IR::Name *name, V4IR::Temp *temp)
{
    if (useFastLookups && name->global) {
        Instruction::GetGlobalLookup load;
        load.index = registerGlobalGetterLookup(*name->id);
        load.result = getResultParam(temp);
        addInstruction(load);
        return;
    }
    Instruction::LoadName load;
    load.name = registerString(*name->id);
    load.result = getResultParam(temp);
    addInstruction(load);
}

void InstructionSelection::setActivationProperty(V4IR::Expr *source, const QString &targetName)
{
    Instruction::StoreName store;
    store.source = getParam(source);
    store.name = registerString(targetName);
    addInstruction(store);
}

void InstructionSelection::initClosure(V4IR::Closure *closure, V4IR::Temp *target)
{
    int id = closure->value;
    Instruction::LoadClosure load;
    load.value = id;
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::getProperty(V4IR::Expr *base, const QString &name, V4IR::Temp *target)
{
    if (useFastLookups) {
        Instruction::GetLookup load;
        load.base = getParam(base);
        load.index = registerGetterLookup(name);
        load.result = getResultParam(target);
        addInstruction(load);
        return;
    }
    Instruction::LoadProperty load;
    load.base = getParam(base);
    load.name = registerString(name);
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::setProperty(V4IR::Expr *source, V4IR::Expr *targetBase,
                                       const QString &targetName)
{
    if (useFastLookups) {
        Instruction::SetLookup store;
        store.base = getParam(targetBase);
        store.index = registerSetterLookup(targetName);
        store.source = getParam(source);
        addInstruction(store);
        return;
    }
    Instruction::StoreProperty store;
    store.base = getParam(targetBase);
    store.name = registerString(targetName);
    store.source = getParam(source);
    addInstruction(store);
}

void InstructionSelection::setQObjectProperty(V4IR::Expr *source, V4IR::Expr *targetBase, int propertyIndex)
{
    Instruction::StoreQObjectProperty store;
    store.base = getParam(targetBase);
    store.propertyIndex = propertyIndex;
    store.source = getParam(source);
    addInstruction(store);
}

void InstructionSelection::getQObjectProperty(V4IR::Expr *base, int propertyIndex, bool captureRequired, V4IR::Temp *target)
{
    Instruction::LoadQObjectProperty load;
    load.base = getParam(base);
    load.propertyIndex = propertyIndex;
    load.result = getResultParam(target);
    load.captureRequired = captureRequired;
    addInstruction(load);
}

void InstructionSelection::getElement(V4IR::Expr *base, V4IR::Expr *index, V4IR::Temp *target)
{
    Instruction::LoadElement load;
    load.base = getParam(base);
    load.index = getParam(index);
    load.result = getResultParam(target);
    addInstruction(load);
}

void InstructionSelection::setElement(V4IR::Expr *source, V4IR::Expr *targetBase,
                                      V4IR::Expr *targetIndex)
{
    Instruction::StoreElement store;
    store.base = getParam(targetBase);
    store.index = getParam(targetIndex);
    store.source = getParam(source);
    addInstruction(store);
}

void InstructionSelection::copyValue(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
    Instruction::Move move;
    move.source = getParam(sourceTemp);
    move.result = getResultParam(targetTemp);
    if (move.source != move.result)
        addInstruction(move);
}

void InstructionSelection::swapValues(V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
    Instruction::SwapTemps swap;
    swap.left = getParam(sourceTemp);
    swap.right = getParam(targetTemp);
    addInstruction(swap);
}

void InstructionSelection::unop(V4IR::AluOp oper, V4IR::Temp *sourceTemp, V4IR::Temp *targetTemp)
{
    switch (oper) {
    case V4IR::OpIfTrue:
        Q_ASSERT(!"unreachable"); break;
    case V4IR::OpNot: {
        // ### enabling this fails in some cases, where apparently the value is not a bool at runtime
        if (0 && isBoolType(sourceTemp)) {
            Instruction::UNotBool unot;
            unot.source = getParam(sourceTemp);
            unot.result = getResultParam(targetTemp);
            addInstruction(unot);
            return;
        }
        Instruction::UNot unot;
        unot.source = getParam(sourceTemp);
        unot.result = getResultParam(targetTemp);
        addInstruction(unot);
        return;
    }
    case V4IR::OpUMinus: {
        Instruction::UMinus uminus;
        uminus.source = getParam(sourceTemp);
        uminus.result = getResultParam(targetTemp);
        addInstruction(uminus);
        return;
    }
    case V4IR::OpUPlus: {
        if (isNumberType(sourceTemp)) {
            // use a move
            Instruction::Move move;
            move.source = getParam(sourceTemp);
            move.result = getResultParam(targetTemp);
            addInstruction(move);
            return;
        }
        Instruction::UPlus uplus;
        uplus.source = getParam(sourceTemp);
        uplus.result = getResultParam(targetTemp);
        addInstruction(uplus);
        return;
    }
    case V4IR::OpCompl: {
        // ### enabling this fails in some cases, where apparently the value is not a int at runtime
        if (0 && isIntegerType(sourceTemp)) {
            Instruction::UComplInt unot;
            unot.source = getParam(sourceTemp);
            unot.result = getResultParam(targetTemp);
            addInstruction(unot);
            return;
        }
        Instruction::UCompl ucompl;
        ucompl.source = getParam(sourceTemp);
        ucompl.result = getResultParam(targetTemp);
        addInstruction(ucompl);
        return;
    }
    case V4IR::OpIncrement: {
        Instruction::Increment inc;
        inc.source = getParam(sourceTemp);
        inc.result = getResultParam(targetTemp);
        addInstruction(inc);
        return;
    }
    case V4IR::OpDecrement: {
        Instruction::Decrement dec;
        dec.source = getParam(sourceTemp);
        dec.result = getResultParam(targetTemp);
        addInstruction(dec);
        return;
    }
    default:  break;
    } // switch

    Q_ASSERT(!"unreachable");
}

void InstructionSelection::binop(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target)
{
    binopHelper(oper, leftSource, rightSource, target);
}

Param InstructionSelection::binopHelper(V4IR::AluOp oper, V4IR::Expr *leftSource, V4IR::Expr *rightSource, V4IR::Temp *target)
{
    if (isNumberType(leftSource) && isNumberType(rightSource)) {
        switch (oper) {
        case V4IR::OpAdd: {
            Instruction::AddNumberParams instr;
            instr.lhs = getParam(leftSource);
            instr.rhs = getParam(rightSource);
            instr.result = getResultParam(target);
            addInstruction(instr);
            return instr.result;
        }
        case V4IR::OpMul: {
            Instruction::MulNumberParams instr;
            instr.lhs = getParam(leftSource);
            instr.rhs = getParam(rightSource);
            instr.result = getResultParam(target);
            addInstruction(instr);
            return instr.result;
        }
        case V4IR::OpSub: {
            Instruction::SubNumberParams instr;
            instr.lhs = getParam(leftSource);
            instr.rhs = getParam(rightSource);
            instr.result = getResultParam(target);
            addInstruction(instr);
            return instr.result;
        }
        default:
            break;
        }
    }

    if (oper == V4IR::OpAdd) {
        Instruction::Add add;
        add.lhs = getParam(leftSource);
        add.rhs = getParam(rightSource);
        add.result = getResultParam(target);
        addInstruction(add);
        return add.result;
    }
    if (oper == V4IR::OpSub) {
        Instruction::Sub sub;
        sub.lhs = getParam(leftSource);
        sub.rhs = getParam(rightSource);
        sub.result = getResultParam(target);
        addInstruction(sub);
        return sub.result;
    }
    if (oper == V4IR::OpMul) {
        Instruction::Mul mul;
        mul.lhs = getParam(leftSource);
        mul.rhs = getParam(rightSource);
        mul.result = getResultParam(target);
        addInstruction(mul);
        return mul.result;
    }
    if (oper == V4IR::OpBitAnd) {
        if (leftSource->asConst())
            qSwap(leftSource, rightSource);
        if (V4IR::Const *c = rightSource->asConst()) {
            Instruction::BitAndConst bitAnd;
            bitAnd.lhs = getParam(leftSource);
            bitAnd.rhs = convertToValue(c).Value::toInt32();
            bitAnd.result = getResultParam(target);
            addInstruction(bitAnd);
            return bitAnd.result;
        }
        Instruction::BitAnd bitAnd;
        bitAnd.lhs = getParam(leftSource);
        bitAnd.rhs = getParam(rightSource);
        bitAnd.result = getResultParam(target);
        addInstruction(bitAnd);
        return bitAnd.result;
    }
    if (oper == V4IR::OpBitOr) {
        if (leftSource->asConst())
            qSwap(leftSource, rightSource);
        if (V4IR::Const *c = rightSource->asConst()) {
            Instruction::BitOrConst bitOr;
            bitOr.lhs = getParam(leftSource);
            bitOr.rhs = convertToValue(c).Value::toInt32();
            bitOr.result = getResultParam(target);
            addInstruction(bitOr);
            return bitOr.result;
        }
        Instruction::BitOr bitOr;
        bitOr.lhs = getParam(leftSource);
        bitOr.rhs = getParam(rightSource);
        bitOr.result = getResultParam(target);
        addInstruction(bitOr);
        return bitOr.result;
    }
    if (oper == V4IR::OpBitXor) {
        if (leftSource->asConst())
            qSwap(leftSource, rightSource);
        if (V4IR::Const *c = rightSource->asConst()) {
            Instruction::BitXorConst bitXor;
            bitXor.lhs = getParam(leftSource);
            bitXor.rhs = convertToValue(c).Value::toInt32();
            bitXor.result = getResultParam(target);
            addInstruction(bitXor);
            return bitXor.result;
        }
        Instruction::BitXor bitXor;
        bitXor.lhs = getParam(leftSource);
        bitXor.rhs = getParam(rightSource);
        bitXor.result = getResultParam(target);
        addInstruction(bitXor);
        return bitXor.result;
    }

    if (oper == V4IR::OpInstanceof || oper == V4IR::OpIn || oper == V4IR::OpAdd) {
        Instruction::BinopContext binop;
        if (oper == V4IR::OpInstanceof)
            binop.alu = QV4::__qmljs_instanceof;
        else if (oper == V4IR::OpIn)
            binop.alu = QV4::__qmljs_in;
        else
            binop.alu = QV4::__qmljs_add;
        binop.lhs = getParam(leftSource);
        binop.rhs = getParam(rightSource);
        binop.result = getResultParam(target);
        Q_ASSERT(binop.alu);
        addInstruction(binop);
        return binop.result;
    } else {
        Instruction::Binop binop;
        binop.alu = aluOpFunction(oper);
        binop.lhs = getParam(leftSource);
        binop.rhs = getParam(rightSource);
        binop.result = getResultParam(target);
        Q_ASSERT(binop.alu);
        addInstruction(binop);
        return binop.result;
    }
}

void InstructionSelection::prepareCallArgs(V4IR::ExprList *e, quint32 &argc, quint32 *args)
{
    int argLocation = outgoingArgumentTempStart();
    argc = 0;
    if (args)
        *args = argLocation;
    if (e) {
        // We need to move all the temps into the function arg array
        assert(argLocation >= 0);
        while (e) {
            Instruction::Move move;
            move.source = getParam(e->expr);
            move.result = Param::createTemp(argLocation);
            addInstruction(move);
            ++argLocation;
            ++argc;
            e = e->next;
        }
    }
}

void InstructionSelection::visitJump(V4IR::Jump *s)
{
    if (s->target == _nextBlock)
        return;
    if (_removableJumps.contains(s))
        return;

    Instruction::Jump jump;
    jump.offset = 0;
    ptrdiff_t loc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));

    _patches[s->target].append(loc);
}

void InstructionSelection::visitCJump(V4IR::CJump *s)
{
    Param condition;
    if (V4IR::Temp *t = s->cond->asTemp()) {
        condition = getResultParam(t);
    } else if (V4IR::Binop *b = s->cond->asBinop()) {
        condition = binopHelper(b->op, b->left, b->right, /*target*/0);
    } else {
        Q_UNIMPLEMENTED();
    }

    Instruction::CJump jump;
    jump.offset = 0;
    jump.condition = condition;

    if (s->iftrue == _nextBlock) {
        jump.invert = true;
        ptrdiff_t falseLoc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));
        _patches[s->iffalse].append(falseLoc);
    } else {
        jump.invert = false;
        ptrdiff_t trueLoc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));
        _patches[s->iftrue].append(trueLoc);

        if (s->iffalse != _nextBlock) {
            Instruction::Jump jump;
            jump.offset = 0;
            ptrdiff_t falseLoc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));
            _patches[s->iffalse].append(falseLoc);
        }
    }
}

void InstructionSelection::visitRet(V4IR::Ret *s)
{
    Instruction::Ret ret;
    ret.result = getParam(s->expr);
    addInstruction(ret);
}

void InstructionSelection::callBuiltinInvalid(V4IR::Name *func, V4IR::ExprList *args, V4IR::Temp *result)
{
    if (useFastLookups && func->global) {
        Instruction::CallGlobalLookup call;
        call.index = registerGlobalGetterLookup(*func->id);
        prepareCallArgs(args, call.argc);
        call.callData = callDataStart();
        call.result = getResultParam(result);
        addInstruction(call);
        return;
    }
    Instruction::CallActivationProperty call;
    call.name = registerString(*func->id);
    prepareCallArgs(args, call.argc);
    call.callData = callDataStart();
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofMember(V4IR::Expr *base, const QString &name,
                                                   V4IR::Temp *result)
{
    Instruction::CallBuiltinTypeofMember call;
    call.base = getParam(base);
    call.member = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofSubscript(V4IR::Expr *base, V4IR::Expr *index,
                                                      V4IR::Temp *result)
{
    Instruction::CallBuiltinTypeofSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofName(const QString &name, V4IR::Temp *result)
{
    Instruction::CallBuiltinTypeofName call;
    call.name = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinTypeofValue(V4IR::Expr *value, V4IR::Temp *result)
{
    Instruction::CallBuiltinTypeofValue call;
    call.value = getParam(value);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteMember(V4IR::Temp *base, const QString &name, V4IR::Temp *result)
{
    Instruction::CallBuiltinDeleteMember call;
    call.base = getParam(base);
    call.member = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteSubscript(V4IR::Temp *base, V4IR::Expr *index,
                                                      V4IR::Temp *result)
{
    Instruction::CallBuiltinDeleteSubscript call;
    call.base = getParam(base);
    call.index = getParam(index);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteName(const QString &name, V4IR::Temp *result)
{
    Instruction::CallBuiltinDeleteName call;
    call.name = registerString(name);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDeleteValue(V4IR::Temp *result)
{
    Instruction::Move move;
    int idx = jsUnitGenerator()->registerConstant(QV4::Encode(false));
    move.source = Param::createConstant(idx);
    move.result = getResultParam(result);
    addInstruction(move);
}

void InstructionSelection::callBuiltinThrow(V4IR::Expr *arg)
{
    Instruction::CallBuiltinThrow call;
    call.arg = getParam(arg);
    addInstruction(call);
}

void InstructionSelection::callBuiltinReThrow()
{
    if (_block->catchBlock) {
        // jump to exception handler
        Instruction::Jump jump;
        jump.offset = 0;
        ptrdiff_t loc = addInstruction(jump) + (((const char *)&jump.offset) - ((const char *)&jump));

        _patches[_block->catchBlock].append(loc);
    } else {
        Instruction::Ret ret;
        int idx = jsUnitGenerator()->registerConstant(QV4::Encode::undefined());
        ret.result = Param::createConstant(idx);
        addInstruction(ret);
    }
}

void InstructionSelection::callBuiltinUnwindException(V4IR::Temp *result)
{
    Instruction::CallBuiltinUnwindException call;
    call.result = getResultParam(result);
    addInstruction(call);
}


void InstructionSelection::callBuiltinPushCatchScope(const QString &exceptionName)
{
    Instruction::CallBuiltinPushCatchScope call;
    call.name = registerString(exceptionName);
    addInstruction(call);
}

void InstructionSelection::callBuiltinForeachIteratorObject(V4IR::Temp *arg, V4IR::Temp *result)
{
    Instruction::CallBuiltinForeachIteratorObject call;
    call.arg = getParam(arg);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinForeachNextPropertyname(V4IR::Temp *arg, V4IR::Temp *result)
{
    Instruction::CallBuiltinForeachNextPropertyName call;
    call.arg = getParam(arg);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinPushWithScope(V4IR::Temp *arg)
{
    Instruction::CallBuiltinPushScope call;
    call.arg = getParam(arg);
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
    call.varName = registerString(name);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineGetterSetter(V4IR::Temp *object, const QString &name, V4IR::Temp *getter, V4IR::Temp *setter)
{
    Instruction::CallBuiltinDefineGetterSetter call;
    call.object = getParam(object);
    call.name = registerString(name);
    call.getter = getParam(getter);
    call.setter = getParam(setter);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineProperty(V4IR::Temp *object, const QString &name,
                                                     V4IR::Expr *value)
{
    Instruction::CallBuiltinDefineProperty call;
    call.object = getParam(object);
    call.name = registerString(name);
    call.value = getParam(value);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineArray(V4IR::Temp *result, V4IR::ExprList *args)
{
    Instruction::CallBuiltinDefineArray call;
    prepareCallArgs(args, call.argc, &call.args);
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinDefineObjectLiteral(V4IR::Temp *result, V4IR::ExprList *args)
{
    int argLocation = outgoingArgumentTempStart();

    const int classId = registerJSClass(args);
    V4IR::ExprList *it = args;
    while (it) {
        it = it->next;

        bool isData = it->expr->asConst()->value;
        it = it->next;

        Instruction::Move move;
        move.source = getParam(it->expr);
        move.result = Param::createTemp(argLocation);
        addInstruction(move);
        ++argLocation;

        if (!isData) {
            it = it->next;

            Instruction::Move move;
            move.source = getParam(it->expr);
            move.result = Param::createTemp(argLocation);
            addInstruction(move);
            ++argLocation;
        }

        it = it->next;
    }

    Instruction::CallBuiltinDefineObjectLiteral call;
    call.internalClassId = classId;
    call.args = outgoingArgumentTempStart();
    call.result = getResultParam(result);
    addInstruction(call);
}

void InstructionSelection::callBuiltinSetupArgumentObject(V4IR::Temp *result)
{
    Instruction::CallBuiltinSetupArgumentsObject call;
    call.result = getResultParam(result);
    addInstruction(call);
}


void QQmlJS::Moth::InstructionSelection::callBuiltinConvertThisToObject()
{
    Instruction::CallBuiltinConvertThisToObject call;
    addInstruction(call);
}

ptrdiff_t InstructionSelection::addInstructionHelper(Instr::Type type, Instr &instr)
{
#ifdef MOTH_THREADED_INTERPRETER
    instr.common.code = VME::instructionJumpTable()[static_cast<int>(type)];
#else
    instr.common.instructionType = type;
#endif
    instr.common.breakPoint = 0;

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
    typedef QHash<V4IR::BasicBlock *, QVector<ptrdiff_t> >::ConstIterator PatchIt;
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

QByteArray InstructionSelection::squeezeCode() const
{
    int codeSize = _codeNext - _codeStart;
    QByteArray squeezed;
    squeezed.resize(codeSize);
    ::memcpy(squeezed.data(), _codeStart, codeSize);
    return squeezed;
}

Param InstructionSelection::getParam(V4IR::Expr *e) {
    assert(e);

    if (V4IR::Const *c = e->asConst()) {
        int idx = jsUnitGenerator()->registerConstant(convertToValue(c).asReturnedValue());
        return Param::createConstant(idx);
    } else if (V4IR::Temp *t = e->asTemp()) {
        switch (t->kind) {
        case V4IR::Temp::Formal:
        case V4IR::Temp::ScopedFormal: return Param::createArgument(t->index, t->scope);
        case V4IR::Temp::Local: return Param::createLocal(t->index);
        case V4IR::Temp::ScopedLocal: return Param::createScopedLocal(t->index, t->scope);
        case V4IR::Temp::StackSlot:
            return Param::createTemp(t->index);
        default:
            Q_UNREACHABLE();
            return Param();
        }
    } else {
        Q_UNIMPLEMENTED();
        return Param();
    }
}


CompilationUnit::~CompilationUnit()
{
    foreach (QV4::Function *f, runtimeFunctions)
        engine->allFunctions.remove(reinterpret_cast<quintptr>(f->codeData));
}

void CompilationUnit::linkBackendToEngine(QV4::ExecutionEngine *engine)
{
    runtimeFunctions.resize(data->functionTableSize);
    runtimeFunctions.fill(0);
    for (int i = 0 ;i < runtimeFunctions.size(); ++i) {
        const QV4::CompiledData::Function *compiledFunction = data->functionAt(i);

        QV4::Function *runtimeFunction = new QV4::Function(engine, this, compiledFunction,
                                                           &VME::exec, /*size - doesn't matter for moth*/0);
        runtimeFunction->codeData = reinterpret_cast<const uchar *>(codeRefs.at(i).constData());
        runtimeFunctions[i] = runtimeFunction;

        if (QV4::Debugging::Debugger *debugger = engine->debugger)
            debugger->setPendingBreakpoints(runtimeFunction);
    }

    foreach (QV4::Function *f, runtimeFunctions)
        engine->allFunctions.insert(reinterpret_cast<quintptr>(f->codeData), f);
}
