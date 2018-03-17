/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4jit_p.h"
#include "qv4assembler_p.h"
#include <private/qv4lookup_p.h>

#ifdef V4_ENABLE_JIT

QT_USE_NAMESPACE
using namespace QV4;
using namespace QV4::JIT;
using namespace QV4::Moth;

ByteCodeHandler::~ByteCodeHandler()
{
}

#define DISPATCH_INSTRUCTION(name, nargs, ...) \
    generate_##name( \
        __VA_ARGS__ \
    );

#define DECODE_AND_DISPATCH(instr) \
    { \
        INSTR_##instr(MOTH_DECODE_WITH_BASE) \
        Q_UNUSED(base_ptr); \
        startInstruction(Instr::Type::instr); \
        _offset = code - start; \
        INSTR_##instr(DISPATCH) \
        endInstruction(Instr::Type::instr); \
        continue; \
    }

void ByteCodeHandler::decode(const char *code, uint len)
{
    MOTH_JUMP_TABLE;

    const char *start = code;
    const char *end = code + len;
    while (code < end) {
        MOTH_DISPATCH()

        FOR_EACH_MOTH_INSTR(DECODE_AND_DISPATCH)
    }
}

#undef DECODE_AND_DISPATCH
#undef DISPATCH_INSTRUCTION

BaselineJIT::BaselineJIT(Function *function)
    : function(function)
    , as(new Assembler(function->compilationUnit->constants))
{}

BaselineJIT::~BaselineJIT()
{}

void BaselineJIT::generate()
{
//    qDebug()<<"jitting" << function->name()->toQString();
    collectLabelsInBytecode();

    as->generatePrologue();
    decode(reinterpret_cast<const char *>(function->codeData), function->compiledFunction->codeSize);
    as->generateEpilogue();

    as->link(function);
//    qDebug()<<"done";
}

#define STORE_IP() as->storeInstructionPointer(instructionOffset())
#define STORE_ACC() as->saveAccumulatorInFrame()

void BaselineJIT::generate_Ret()
{
    as->ret();
}

void BaselineJIT::generate_Debug() { Q_UNREACHABLE(); }

void BaselineJIT::generate_LoadConst(int index)
{
    as->loadConst(index);
}

void BaselineJIT::generate_LoadZero()
{
    as->loadValue(Encode(int(0)));
}

void BaselineJIT::generate_LoadTrue()
{
    as->loadValue(Encode(true));
}

void BaselineJIT::generate_LoadFalse()
{
    as->loadValue(Encode(false));
}

void BaselineJIT::generate_LoadNull()
{
    as->loadValue(Encode::null());
}

void BaselineJIT::generate_LoadUndefined()
{
    as->loadValue(Encode::undefined());
}

void BaselineJIT::generate_LoadInt(int value)
{
    //###
    as->loadValue(Encode(value));
}

void BaselineJIT::generate_MoveConst(int constIndex, int destTemp)
{
    as->copyConst(constIndex, destTemp);
}

void BaselineJIT::generate_LoadReg(int reg)
{
    as->loadReg(reg);
}

void BaselineJIT::generate_StoreReg(int reg)
{
    as->storeReg(reg);
}

void BaselineJIT::generate_MoveReg(int srcReg, int destReg)
{
    as->loadReg(srcReg);
    as->storeReg(destReg);
}

void BaselineJIT::generate_LoadLocal(int index)
{
    as->loadLocal(index);
}

void BaselineJIT::generate_StoreLocal(int index)
{
    as->checkException();
    as->storeLocal(index);
}

void BaselineJIT::generate_LoadScopedLocal(int scope, int index)
{
    as->loadLocal(index, scope);
}

void BaselineJIT::generate_StoreScopedLocal(int scope, int index)
{
    as->checkException();
    as->storeLocal(index, scope);
}

void BaselineJIT::generate_LoadRuntimeString(int stringId)
{
    as->loadString(stringId);
}

void BaselineJIT::generate_MoveRegExp(int regExpId, int destReg)
{
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(regExpId, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_regexpLiteral, Assembler::ResultInAccumulator);
    as->storeReg(destReg);
}

void BaselineJIT::generate_LoadClosure(int value)
{
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(value, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_closure, Assembler::ResultInAccumulator);
}

void BaselineJIT::generate_LoadName(int name)
{
    STORE_IP();
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(name, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_loadName, Assembler::ResultInAccumulator);
    as->checkException();
}

static ReturnedValue loadGlobalLookupHelper(ExecutionEngine *engine, QV4::Function *f, int index)
{
    QV4::Lookup *l = f->compilationUnit->runtimeLookups + index;
    return l->globalGetter(l, engine);
}

void BaselineJIT::generate_LoadGlobalLookup(int index)
{
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(index, 2);
    as->passFunctionAsArg(1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(loadGlobalLookupHelper, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_StoreNameSloppy(int name)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passAccumulatorAsArg(2);
    as->passInt32AsArg(name, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_storeNameSloppy, Assembler::IgnoreResult);
    as->checkException();
}

void BaselineJIT::generate_StoreNameStrict(int name)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passAccumulatorAsArg(2);
    as->passInt32AsArg(name, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_storeNameStrict, Assembler::IgnoreResult);
    as->checkException();
}

void BaselineJIT::generate_LoadElement(int base, int index)
{
    STORE_IP();
    as->prepareCallWithArgCount(3);
    as->passRegAsArg(index, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_loadElement, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_LoadElementA(int base)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passAccumulatorAsArg(2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_loadElement, Assembler::ResultInAccumulator);
    as->checkException();
}

static void storeElementHelper(QV4::Function *f, const Value &base, const Value &index, const Value &value)
{
    auto engine = f->internalClass->engine;
    if (!Runtime::method_storeElement(engine, base, index, value) && f->isStrict())
        engine->throwTypeError();
}

void BaselineJIT::generate_StoreElement(int base, int index)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(4);
    as->passAccumulatorAsArg(3);
    as->passRegAsArg(index, 2);
    as->passRegAsArg(base, 1);
    as->passFunctionAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(storeElementHelper, Assembler::IgnoreResult);
    as->checkException();
}

void BaselineJIT::generate_LoadProperty(int name, int base)
{
    STORE_IP();
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(name, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_loadProperty, Assembler::ResultInAccumulator);
    as->checkException();
}
void BaselineJIT::generate_LoadPropertyA(int name)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(name, 2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_loadProperty, Assembler::ResultInAccumulator);
    as->checkException();
}

static ReturnedValue getLookupHelper(ExecutionEngine *engine, QV4::Function *f, int index, const QV4::Value &base)
{
    QV4::Lookup *l = f->compilationUnit->runtimeLookups + index;
    return l->getter(l, engine, base);
}

void BaselineJIT::generate_GetLookup(int index, int base)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passRegAsArg(base, 3);
    as->passInt32AsArg(index, 2);
    as->passFunctionAsArg(1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(getLookupHelper, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_GetLookupA(int index)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(4);
    as->passAccumulatorAsArg(3);
    as->passInt32AsArg(index, 2);
    as->passFunctionAsArg(1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(getLookupHelper, Assembler::ResultInAccumulator);
    as->checkException();
}

static void storePropertyHelper(QV4::Function *f, const Value &base, int name, const Value &value)
{
    auto engine = f->internalClass->engine;
    if (!Runtime::method_storeProperty(engine, base, name, value) && f->isStrict())
        engine->throwTypeError();
}

void BaselineJIT::generate_StoreProperty(int name, int base)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(4);
    as->passAccumulatorAsArg(3);
    as->passInt32AsArg(name, 2);
    as->passRegAsArg(base, 1);
    as->passFunctionAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(storePropertyHelper, Assembler::IgnoreResult);
    as->checkException();
}

static void setLookupHelper(QV4::Function *f, int index, QV4::Value &base, const QV4::Value &value)
{
    ExecutionEngine *engine = f->internalClass->engine;
    QV4::Lookup *l = f->compilationUnit->runtimeLookups + index;
    if (!l->setter(l, engine, base, value) && f->isStrict())
        engine->throwTypeError();
}

void BaselineJIT::generate_SetLookup(int index, int base)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(4);
    as->passAccumulatorAsArg(3);
    as->passRegAsArg(base, 2);
    as->passInt32AsArg(index, 1);
    as->passFunctionAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(setLookupHelper, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_StoreScopeObjectProperty(int base, int propertyIndex)
{
    STORE_ACC();
    as->prepareCallWithArgCount(4);
    as->passAccumulatorAsArg(3);
    as->passInt32AsArg(propertyIndex, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_storeQmlScopeObjectProperty, Assembler::IgnoreResult);
    as->checkException();
}

void BaselineJIT::generate_StoreContextObjectProperty(int base, int propertyIndex)
{
    STORE_ACC();
    as->prepareCallWithArgCount(4);
    as->passAccumulatorAsArg(3);
    as->passInt32AsArg(propertyIndex, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_storeQmlContextObjectProperty, Assembler::IgnoreResult);
    as->checkException();
}

void BaselineJIT::generate_LoadScopeObjectProperty(int propertyIndex, int base, int captureRequired)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(captureRequired, 3);
    as->passInt32AsArg(propertyIndex, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_loadQmlScopeObjectProperty, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_LoadContextObjectProperty(int propertyIndex, int base, int captureRequired)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(captureRequired, 3);
    as->passInt32AsArg(propertyIndex, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_loadQmlContextObjectProperty, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_LoadIdObject(int index, int base)
{
    STORE_IP();
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(index, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_loadQmlIdObject, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_CallValue(int name, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(argc, 3);
    as->passRegAsArg(argv, 2);
    as->passRegAsArg(name, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_callValue, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_CallProperty(int name, int base, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passRegAsArg(argv, 3);
    as->passInt32AsArg(name, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_callProperty, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_CallPropertyLookup(int lookupIndex, int base, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passRegAsArg(argv, 3);
    as->passInt32AsArg(lookupIndex, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_callPropertyLookup, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_CallElement(int base, int index, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passRegAsArg(argv, 3);
    as->passRegAsArg(index, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_callElement, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_CallName(int name, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(argc, 3);
    as->passRegAsArg(argv, 2);
    as->passInt32AsArg(name, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_callName, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_CallPossiblyDirectEval(int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(argc, 2);
    as->passRegAsArg(argv, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_callPossiblyDirectEval, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_CallGlobalLookup(int index, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(argc, 3);
    as->passRegAsArg(argv, 2);
    as->passInt32AsArg(index, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_callGlobalLookup, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_CallScopeObjectProperty(int propIdx, int base, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passRegAsArg(argv, 3);
    as->passInt32AsArg(propIdx, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_callQmlScopeObjectProperty, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_CallContextObjectProperty(int propIdx, int base, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passRegAsArg(argv, 3);
    as->passInt32AsArg(propIdx, 2);
    as->passRegAsArg(base, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_callQmlContextObjectProperty, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_SetExceptionHandler(int offset)
{
    if (offset)
        as->setExceptionHandler(instructionOffset() + offset);
    else
        as->clearExceptionHandler();
}

void BaselineJIT::generate_ThrowException()
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_throwException, Assembler::IgnoreResult);
    as->gotoCatchException();
}

void BaselineJIT::generate_GetException() { as->getException(); }
void BaselineJIT::generate_SetException() { as->setException(); }

void BaselineJIT::generate_CreateCallContext()
{
    as->prepareCallWithArgCount(1);
    as->passCppFrameAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(ExecutionContext::newCallContext, Assembler::IgnoreResult); // keeps result in return value register
    as->storeHeapObject(CallData::Context);
}

void BaselineJIT::generate_PushCatchContext(int name, int reg) { as->pushCatchContext(name, reg); }

static void pushWithContextHelper(ExecutionEngine *engine, QV4::Value *stack, int reg)
{
    QV4::Value &accumulator = stack[CallData::Accumulator];
    accumulator = accumulator.toObject(engine);
    if (engine->hasException)
        return;
    stack[reg] = stack[CallData::Context];
    ExecutionContext *c = static_cast<ExecutionContext *>(stack + CallData::Context);
    stack[CallData::Context] = Runtime::method_createWithContext(c, accumulator);
}

void BaselineJIT::generate_PushWithContext(int reg)
{
    STORE_IP();
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(reg, 2);
    as->passRegAsArg(0, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(pushWithContextHelper, Assembler::IgnoreResult);
    as->checkException();
}

void BaselineJIT::generate_PopContext(int reg) { as->popContext(reg); }

void BaselineJIT::generate_ForeachIteratorObject()
{
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_foreachIterator, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_ForeachNextPropertyName()
{
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(1);
    as->passAccumulatorAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_foreachNextPropertyName,
                              Assembler::ResultInAccumulator);
    as->checkException();
}

static ReturnedValue deleteMemberHelper(QV4::Function *function, const QV4::Value &base, int member)
{
    auto engine = function->internalClass->engine;
    if (!Runtime::method_deleteMember(engine, base, member)) {
        if (function->isStrict())
            engine->throwTypeError();
        return Encode(false);
    } else {
        return Encode(true);
    }
}

void BaselineJIT::generate_DeleteMember(int member, int base)
{
    STORE_IP();
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(member, 2);
    as->passRegAsArg(base, 1);
    as->passFunctionAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(deleteMemberHelper, Assembler::ResultInAccumulator);
    as->checkException();
}

static ReturnedValue deleteSubscriptHelper(QV4::Function *function, const QV4::Value &base, const QV4::Value &index)
{
    auto engine = function->internalClass->engine;
    if (!Runtime::method_deleteElement(engine, base, index)) {
        if (function->isStrict())
            engine->throwTypeError();
        return Encode(false);
    } else {
        return Encode(true);
    }
}

void BaselineJIT::generate_DeleteSubscript(int base, int index)
{
    STORE_IP();
    as->prepareCallWithArgCount(3);
    as->passRegAsArg(index, 2);
    as->passRegAsArg(base, 1);
    as->passFunctionAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(deleteSubscriptHelper, Assembler::ResultInAccumulator);
    as->checkException();
}

static ReturnedValue deleteNameHelper(QV4::Function *function, int name)
{
    auto engine = function->internalClass->engine;
    if (!Runtime::method_deleteName(engine, name)) {
        if (function->isStrict())
            engine->throwTypeError();
        return Encode(false);
    } else {
        return Encode(true);
    }
}

void BaselineJIT::generate_DeleteName(int name)
{
    STORE_IP();
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(name, 1);
    as->passFunctionAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(deleteNameHelper, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_TypeofName(int name)
{
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(name, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_typeofName, Assembler::ResultInAccumulator);
}

void BaselineJIT::generate_TypeofValue()
{
    STORE_ACC();
    as->prepareCallWithArgCount(2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_typeofValue, Assembler::ResultInAccumulator);
}

void BaselineJIT::generate_DeclareVar(int varName, int isDeletable)
{
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(varName, 2);
    as->passInt32AsArg(isDeletable, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_declareVar, Assembler::IgnoreResult);
}

void BaselineJIT::generate_DefineArray(int argc, int args)
{
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(argc, 2);
    as->passRegAsArg(args, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_arrayLiteral, Assembler::ResultInAccumulator);
}

void BaselineJIT::generate_DefineObjectLiteral(int internalClassId, int arrayValueCount,
                                             int arrayGetterSetterCountAndFlags, int args)
{
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(arrayGetterSetterCountAndFlags, 4);
    as->passInt32AsArg(arrayValueCount, 3);
    as->passInt32AsArg(internalClassId, 2);
    as->passRegAsArg(args, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_objectLiteral, Assembler::ResultInAccumulator);
}
void BaselineJIT::generate_CreateMappedArgumentsObject()
{
    as->prepareCallWithArgCount(1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_createMappedArgumentsObject,
                              Assembler::ResultInAccumulator);
}

void BaselineJIT::generate_CreateUnmappedArgumentsObject()
{
    as->prepareCallWithArgCount(1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_createUnmappedArgumentsObject,
                              Assembler::ResultInAccumulator);
}

static void convertThisToObjectHelper(ExecutionEngine *engine, Value *t)
{
    if (!t->isObject()) {
        if (t->isNullOrUndefined()) {
            *t = engine->globalObject->asReturnedValue();
        } else {
            *t = t->toObject(engine)->asReturnedValue();
        }
    }
}

void BaselineJIT::generate_ConvertThisToObject()
{
    as->prepareCallWithArgCount(2);
    as->passRegAsArg(CallData::This, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(convertThisToObjectHelper, Assembler::IgnoreResult);
    as->checkException();
}

void BaselineJIT::generate_Construct(int func, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(argc, 3);
    as->passRegAsArg(argv, 2);
    as->passRegAsArg(func, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_construct, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_Jump(int offset) { as->jump(instructionOffset() + offset); }
void BaselineJIT::generate_JumpTrue(int offset) { as->jumpTrue(instructionOffset() + offset); }
void BaselineJIT::generate_JumpFalse(int offset) { as->jumpFalse(instructionOffset() + offset); }

void BaselineJIT::generate_CmpEqNull() { as->cmpeqNull(); }
void BaselineJIT::generate_CmpNeNull() { as->cmpneNull(); }
void BaselineJIT::generate_CmpEqInt(int lhs) { as->cmpeqInt(lhs); }
void BaselineJIT::generate_CmpNeInt(int lhs) { as->cmpneInt(lhs); }
void BaselineJIT::generate_CmpEq(int lhs) { as->cmpeq(lhs); }
void BaselineJIT::generate_CmpNe(int lhs) { as->cmpne(lhs); }
void BaselineJIT::generate_CmpGt(int lhs) { as->cmpgt(lhs); }
void BaselineJIT::generate_CmpGe(int lhs) { as->cmpge(lhs); }
void BaselineJIT::generate_CmpLt(int lhs) { as->cmplt(lhs); }
void BaselineJIT::generate_CmpLe(int lhs) { as->cmple(lhs); }
void BaselineJIT::generate_CmpStrictEqual(int lhs) { as->cmpStrictEqual(lhs); }
void BaselineJIT::generate_CmpStrictNotEqual(int lhs) { as->cmpStrictNotEqual(lhs); }

void BaselineJIT::generate_CmpIn(int lhs)
{
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passAccumulatorAsArg(2);
    as->passRegAsArg(lhs, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_in, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_CmpInstanceOf(int lhs)
{
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passAccumulatorAsArg(2);
    as->passRegAsArg(lhs, 1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_instanceof, Assembler::ResultInAccumulator);
    as->checkException();
}

void BaselineJIT::generate_JumpStrictEqualStackSlotInt(int lhs, int rhs, int offset)
{
    as->jumpStrictEqualStackSlotInt(lhs, rhs, instructionOffset() + offset);
}

void BaselineJIT::generate_JumpStrictNotEqualStackSlotInt(int lhs, int rhs, int offset)
{
    as->jumpStrictNotEqualStackSlotInt(lhs, rhs, instructionOffset() + offset);
}

void BaselineJIT::generate_UNot() { as->unot(); }
void BaselineJIT::generate_UPlus() { as->toNumber(); }
void BaselineJIT::generate_UMinus() { as->uminus(); }
void BaselineJIT::generate_UCompl() { as->ucompl(); }
void BaselineJIT::generate_Increment() { as->inc(); }
void BaselineJIT::generate_Decrement() { as->dec(); }
void BaselineJIT::generate_Add(int lhs) { as->add(lhs); }

void BaselineJIT::generate_BitAnd(int lhs) { as->bitAnd(lhs); }
void BaselineJIT::generate_BitOr(int lhs) { as->bitOr(lhs); }
void BaselineJIT::generate_BitXor(int lhs) { as->bitXor(lhs); }
void BaselineJIT::generate_UShr(int lhs) { as->ushr(lhs); }
void BaselineJIT::generate_Shr(int lhs) { as->shr(lhs); }
void BaselineJIT::generate_Shl(int lhs) { as->shl(lhs); }

void BaselineJIT::generate_BitAndConst(int rhs) { as->bitAndConst(rhs); }
void BaselineJIT::generate_BitOrConst(int rhs) { as->bitOrConst(rhs); }
void BaselineJIT::generate_BitXorConst(int rhs) { as->bitXorConst(rhs); }
void BaselineJIT::generate_UShrConst(int rhs) { as->ushrConst(rhs); }
void BaselineJIT::generate_ShrConst(int rhs) { as->shrConst(rhs); }
void BaselineJIT::generate_ShlConst(int rhs) { as->shlConst(rhs); }

void BaselineJIT::generate_Mul(int lhs) { as->mul(lhs); }
void BaselineJIT::generate_Div(int lhs) { as->div(lhs); }
void BaselineJIT::generate_Mod(int lhs) { as->mod(lhs); }
void BaselineJIT::generate_Sub(int lhs) { as->sub(lhs); }

//void BaselineJIT::generate_BinopContext(int alu, int lhs)
//{
//    auto engine = function->internalClass->engine;
//    void *op = engine->runtime.runtimeMethods[alu];
//    STORE_ACC();
//    as->passAccumulatorAsArg(2);
//    as->passRegAsArg(lhs, 1);
//    as->passEngineAsArg(0);
//    as->callRuntime("binopContext", op, Assembler::ResultInAccumulator);
//    as->checkException();
//}

void BaselineJIT::generate_LoadQmlContext(int result)
{
    as->prepareCallWithArgCount(1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_loadQmlContext, Assembler::ResultInAccumulator);
    as->storeReg(result);
}

void BaselineJIT::generate_LoadQmlImportedScripts(int result)
{
    as->prepareCallWithArgCount(1);
    as->passEngineAsArg(0);
    JIT_GENERATE_RUNTIME_CALL(Runtime::method_loadQmlImportedScripts, Assembler::ResultInAccumulator);
    as->storeReg(result);
}

void BaselineJIT::startInstruction(Instr::Type /*instr*/)
{
    if (hasLabel())
        as->addLabel(instructionOffset());
}

void BaselineJIT::endInstruction(Instr::Type instr)
{
    Q_UNUSED(instr);
}

#define MOTH_UNUSED_ARGS0()
#define MOTH_UNUSED_ARGS1(arg) \
    Q_UNUSED(arg);
#define MOTH_UNUSED_ARGS2(arg1, arg2) \
    Q_UNUSED(arg1); \
    Q_UNUSED(arg2);
#define MOTH_UNUSED_ARGS3(arg1, arg2, arg3) \
    Q_UNUSED(arg1); \
    Q_UNUSED(arg2); \
    Q_UNUSED(arg3);
#define MOTH_UNUSED_ARGS4(arg1, arg2, arg3, arg4) \
    Q_UNUSED(arg1); \
    Q_UNUSED(arg2); \
    Q_UNUSED(arg3); \
    Q_UNUSED(arg4);

#define MOTH_MARK_ARGS_UNUSED_PLEASE(nargs, ...) \
    MOTH_EXPAND_FOR_MSVC(MOTH_UNUSED_ARGS##nargs(__VA_ARGS__))

#define MOTH_MARK_ARGS_UNUSED_INSTRUCTION(name, nargs, ...) \
    MOTH_MARK_ARGS_UNUSED_PLEASE(nargs, __VA_ARGS__)

#define MOTH_BEGIN_INSTR(instr) \
    { \
        INSTR_##instr(MOTH_DECODE_WITH_BASE) \
        INSTR_##instr(MOTH_MARK_ARGS_UNUSED) \
        Q_UNUSED(base_ptr);

#define MOTH_END_INSTR(instr) \
        continue; \
    }

void BaselineJIT::collectLabelsInBytecode()
{
    MOTH_JUMP_TABLE;

    const auto addLabel = [&](int offset) {
        Q_ASSERT(offset >= 0 && offset < static_cast<int>(function->compiledFunction->codeSize));
        labels.push_back(offset);
    };

    const char *code = reinterpret_cast<const char *>(function->codeData);
    const char *start = code;
    const char *end = code + function->compiledFunction->codeSize;
    while (code < end) {
        MOTH_DISPATCH()
        Q_UNREACHABLE();

        MOTH_BEGIN_INSTR(LoadReg)
        MOTH_END_INSTR(LoadReg)

        MOTH_BEGIN_INSTR(StoreReg)
        MOTH_END_INSTR(StoreReg)

        MOTH_BEGIN_INSTR(MoveReg)
        MOTH_END_INSTR(MoveReg)

        MOTH_BEGIN_INSTR(LoadConst)
        MOTH_END_INSTR(LoadConst)

        MOTH_BEGIN_INSTR(LoadNull)
        MOTH_END_INSTR(LoadNull)

        MOTH_BEGIN_INSTR(LoadZero)
        MOTH_END_INSTR(LoadZero)

        MOTH_BEGIN_INSTR(LoadTrue)
        MOTH_END_INSTR(LoadTrue)

        MOTH_BEGIN_INSTR(LoadFalse)
        MOTH_END_INSTR(LoadFalse)

        MOTH_BEGIN_INSTR(LoadUndefined)
        MOTH_END_INSTR(LoadUndefined)

        MOTH_BEGIN_INSTR(LoadInt)
        MOTH_END_INSTR(LoadInt)

        MOTH_BEGIN_INSTR(MoveConst)
        MOTH_END_INSTR(MoveConst)

        MOTH_BEGIN_INSTR(LoadLocal)
        MOTH_END_INSTR(LoadLocal)

        MOTH_BEGIN_INSTR(StoreLocal)
        MOTH_END_INSTR(StoreLocal)

        MOTH_BEGIN_INSTR(LoadScopedLocal)
        MOTH_END_INSTR(LoadScopedLocal)

        MOTH_BEGIN_INSTR(StoreScopedLocal)
        MOTH_END_INSTR(StoreScopedLocal)

        MOTH_BEGIN_INSTR(LoadRuntimeString)
        MOTH_END_INSTR(LoadRuntimeString)

        MOTH_BEGIN_INSTR(MoveRegExp)
        MOTH_END_INSTR(MoveRegExp)

        MOTH_BEGIN_INSTR(LoadClosure)
        MOTH_END_INSTR(LoadClosure)

        MOTH_BEGIN_INSTR(LoadName)
        MOTH_END_INSTR(LoadName)

        MOTH_BEGIN_INSTR(LoadGlobalLookup)
        MOTH_END_INSTR(LoadGlobalLookup)

        MOTH_BEGIN_INSTR(StoreNameSloppy)
        MOTH_END_INSTR(StoreNameSloppy)

        MOTH_BEGIN_INSTR(StoreNameStrict)
        MOTH_END_INSTR(StoreNameStrict)

        MOTH_BEGIN_INSTR(LoadElement)
        MOTH_END_INSTR(LoadElement)

        MOTH_BEGIN_INSTR(LoadElementA)
        MOTH_END_INSTR(LoadElement)

        MOTH_BEGIN_INSTR(StoreElement)
        MOTH_END_INSTR(StoreElement)

        MOTH_BEGIN_INSTR(LoadProperty)
        MOTH_END_INSTR(LoadProperty)

        MOTH_BEGIN_INSTR(LoadPropertyA)
        MOTH_END_INSTR(LoadElementA)

        MOTH_BEGIN_INSTR(GetLookup)
        MOTH_END_INSTR(GetLookup)

        MOTH_BEGIN_INSTR(GetLookupA)
        MOTH_END_INSTR(GetLookupA)

        MOTH_BEGIN_INSTR(StoreProperty)
        MOTH_END_INSTR(StoreProperty)

        MOTH_BEGIN_INSTR(SetLookup)
        MOTH_END_INSTR(SetLookup)

        MOTH_BEGIN_INSTR(StoreScopeObjectProperty)
        MOTH_END_INSTR(StoreScopeObjectProperty)

        MOTH_BEGIN_INSTR(LoadScopeObjectProperty)
        MOTH_END_INSTR(LoadScopeObjectProperty)

        MOTH_BEGIN_INSTR(StoreContextObjectProperty)
        MOTH_END_INSTR(StoreContextObjectProperty)

        MOTH_BEGIN_INSTR(LoadContextObjectProperty)
        MOTH_END_INSTR(LoadContextObjectProperty)

        MOTH_BEGIN_INSTR(LoadIdObject)
        MOTH_END_INSTR(LoadIdObject)

        MOTH_BEGIN_INSTR(CallValue)
        MOTH_END_INSTR(CallValue)

        MOTH_BEGIN_INSTR(CallProperty)
        MOTH_END_INSTR(CallProperty)

        MOTH_BEGIN_INSTR(CallPropertyLookup)
        MOTH_END_INSTR(CallPropertyLookup)

        MOTH_BEGIN_INSTR(CallElement)
        MOTH_END_INSTR(CallElement)

        MOTH_BEGIN_INSTR(CallName)
        MOTH_END_INSTR(CallName)

        MOTH_BEGIN_INSTR(CallPossiblyDirectEval)
        MOTH_END_INSTR(CallPossiblyDirectEval)

        MOTH_BEGIN_INSTR(CallGlobalLookup)
        MOTH_END_INSTR(CallGlobalLookup)

        MOTH_BEGIN_INSTR(CallScopeObjectProperty)
        MOTH_END_INSTR(CallScopeObjectProperty)

        MOTH_BEGIN_INSTR(CallContextObjectProperty)
        MOTH_END_INSTR(CallContextObjectProperty)

        MOTH_BEGIN_INSTR(SetExceptionHandler)
            addLabel(code - start + offset);
        MOTH_END_INSTR(SetExceptionHandler)

        MOTH_BEGIN_INSTR(ThrowException)
        MOTH_END_INSTR(ThrowException)

        MOTH_BEGIN_INSTR(GetException)
        MOTH_END_INSTR(HasException)

        MOTH_BEGIN_INSTR(SetException)
        MOTH_END_INSTR(SetExceptionFlag)

        MOTH_BEGIN_INSTR(CreateCallContext)
        MOTH_END_INSTR(CreateCallContext)

        MOTH_BEGIN_INSTR(PushCatchContext)
        MOTH_END_INSTR(PushCatchContext)

        MOTH_BEGIN_INSTR(PushWithContext)
        MOTH_END_INSTR(PushWithContext)

        MOTH_BEGIN_INSTR(PopContext)
        MOTH_END_INSTR(PopContext)

        MOTH_BEGIN_INSTR(ForeachIteratorObject)
        MOTH_END_INSTR(ForeachIteratorObject)

        MOTH_BEGIN_INSTR(ForeachNextPropertyName)
        MOTH_END_INSTR(ForeachNextPropertyName)

        MOTH_BEGIN_INSTR(DeleteMember)
        MOTH_END_INSTR(DeleteMember)

        MOTH_BEGIN_INSTR(DeleteSubscript)
        MOTH_END_INSTR(DeleteSubscript)

        MOTH_BEGIN_INSTR(DeleteName)
        MOTH_END_INSTR(DeleteName)

        MOTH_BEGIN_INSTR(TypeofName)
        MOTH_END_INSTR(TypeofName)

        MOTH_BEGIN_INSTR(TypeofValue)
        MOTH_END_INSTR(TypeofValue)

        MOTH_BEGIN_INSTR(DeclareVar)
        MOTH_END_INSTR(DeclareVar)

        MOTH_BEGIN_INSTR(DefineArray)
        MOTH_END_INSTR(DefineArray)

        MOTH_BEGIN_INSTR(DefineObjectLiteral)
        MOTH_END_INSTR(DefineObjectLiteral)

        MOTH_BEGIN_INSTR(CreateMappedArgumentsObject)
        MOTH_END_INSTR(CreateMappedArgumentsObject)

        MOTH_BEGIN_INSTR(CreateUnmappedArgumentsObject)
        MOTH_END_INSTR(CreateUnmappedArgumentsObject)

        MOTH_BEGIN_INSTR(ConvertThisToObject)
        MOTH_END_INSTR(ConvertThisToObject)

        MOTH_BEGIN_INSTR(Construct)
        MOTH_END_INSTR(Construct)

        MOTH_BEGIN_INSTR(Jump)
            addLabel(code - start + offset);
        MOTH_END_INSTR(Jump)

        MOTH_BEGIN_INSTR(JumpTrue)
            addLabel(code - start + offset);
        MOTH_END_INSTR(JumpTrue)

        MOTH_BEGIN_INSTR(JumpFalse)
            addLabel(code - start + offset);
        MOTH_END_INSTR(JumpFalse)

        MOTH_BEGIN_INSTR(CmpEqNull)
        MOTH_END_INSTR(CmpEqNull)

        MOTH_BEGIN_INSTR(CmpNeNull)
        MOTH_END_INSTR(CmpNeNull)

        MOTH_BEGIN_INSTR(CmpEqInt)
        MOTH_END_INSTR(CmpEq)

        MOTH_BEGIN_INSTR(CmpNeInt)
        MOTH_END_INSTR(CmpNeInt)

        MOTH_BEGIN_INSTR(CmpEq)
        MOTH_END_INSTR(CmpEq)

        MOTH_BEGIN_INSTR(CmpNe)
        MOTH_END_INSTR(CmpNe)

        MOTH_BEGIN_INSTR(CmpGt)
        MOTH_END_INSTR(CmpGt)

        MOTH_BEGIN_INSTR(CmpGe)
        MOTH_END_INSTR(CmpGe)

        MOTH_BEGIN_INSTR(CmpLt)
        MOTH_END_INSTR(CmpLt)

        MOTH_BEGIN_INSTR(CmpLe)
        MOTH_END_INSTR(CmpLe)

        MOTH_BEGIN_INSTR(CmpStrictEqual)
        MOTH_END_INSTR(CmpStrictEqual)

        MOTH_BEGIN_INSTR(CmpStrictNotEqual)
        MOTH_END_INSTR(CmpStrictNotEqual)

        MOTH_BEGIN_INSTR(CmpIn)
        MOTH_END_INSTR(CmpIn)

        MOTH_BEGIN_INSTR(CmpInstanceOf)
        MOTH_END_INSTR(CmpInstanceOf)

        MOTH_BEGIN_INSTR(JumpStrictEqualStackSlotInt)
            addLabel(code - start + offset);
        MOTH_END_INSTR(JumpStrictEqualStackSlotInt)

        MOTH_BEGIN_INSTR(JumpStrictNotEqualStackSlotInt)
            addLabel(code - start + offset);
        MOTH_END_INSTR(JumpStrictNotEqualStackSlotInt)

        MOTH_BEGIN_INSTR(UNot)
        MOTH_END_INSTR(UNot)

        MOTH_BEGIN_INSTR(UPlus)
        MOTH_END_INSTR(UPlus)

        MOTH_BEGIN_INSTR(UMinus)
        MOTH_END_INSTR(UMinus)

        MOTH_BEGIN_INSTR(UCompl)
        MOTH_END_INSTR(UCompl)

        MOTH_BEGIN_INSTR(Increment)
        MOTH_END_INSTR(PreIncrement)

        MOTH_BEGIN_INSTR(Decrement)
        MOTH_END_INSTR(PreDecrement)

        MOTH_BEGIN_INSTR(Add)
        MOTH_END_INSTR(Add)

        MOTH_BEGIN_INSTR(BitAnd)
        MOTH_END_INSTR(BitAnd)

        MOTH_BEGIN_INSTR(BitOr)
        MOTH_END_INSTR(BitOr)

        MOTH_BEGIN_INSTR(BitXor)
        MOTH_END_INSTR(BitXor)

        MOTH_BEGIN_INSTR(UShr)
        MOTH_END_INSTR(UShr)

        MOTH_BEGIN_INSTR(Shr)
        MOTH_END_INSTR(Shr)

        MOTH_BEGIN_INSTR(Shl)
        MOTH_END_INSTR(Shl)

        MOTH_BEGIN_INSTR(BitAndConst)
        MOTH_END_INSTR(BitAndConst)

        MOTH_BEGIN_INSTR(BitOrConst)
        MOTH_END_INSTR(BitOr)

        MOTH_BEGIN_INSTR(BitXorConst)
        MOTH_END_INSTR(BitXor)

        MOTH_BEGIN_INSTR(UShrConst)
        MOTH_END_INSTR(UShrConst)

        MOTH_BEGIN_INSTR(ShrConst)
        MOTH_END_INSTR(ShrConst)

        MOTH_BEGIN_INSTR(ShlConst)
        MOTH_END_INSTR(ShlConst)

        MOTH_BEGIN_INSTR(Mul)
        MOTH_END_INSTR(Mul)

        MOTH_BEGIN_INSTR(Div)
        MOTH_END_INSTR(Div)

        MOTH_BEGIN_INSTR(Mod)
        MOTH_END_INSTR(Mod)

        MOTH_BEGIN_INSTR(Sub)
        MOTH_END_INSTR(Sub)

        MOTH_BEGIN_INSTR(Ret)
        MOTH_END_INSTR(Ret)

        MOTH_BEGIN_INSTR(Debug)
        MOTH_END_INSTR(Debug)

        MOTH_BEGIN_INSTR(LoadQmlContext)
        MOTH_END_INSTR(LoadQmlContext)

        MOTH_BEGIN_INSTR(LoadQmlImportedScripts)
        MOTH_END_INSTR(LoadQmlImportedScripts)
    }
}
#undef MOTH_BEGIN_INSTR
#undef MOTH_END_INSTR

#endif // V4_ENABLE_JIT
