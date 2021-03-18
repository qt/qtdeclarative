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

#include "qv4baselinejit_p.h"
#include "qv4baselineassembler_p.h"
#include <private/qv4lookup_p.h>
#include <private/qv4generatorobject_p.h>

QT_USE_NAMESPACE
using namespace QV4;
using namespace QV4::JIT;
using namespace QV4::Moth;

BaselineJIT::BaselineJIT(Function *function)
    : function(function)
      , as(new BaselineAssembler(&(function->compilationUnit->constants->asValue<Value>())))
{}

BaselineJIT::~BaselineJIT()
{}

void BaselineJIT::generate()
{
//    qDebug()<<"jitting" << function->name()->toQString();
    const char *code = function->codeData;
    uint len = function->compiledFunction->codeSize;

    for (unsigned i = 0, ei = function->compiledFunction->nLabelInfos; i != ei; ++i)
        labels.insert(int(function->compiledFunction->labelInfoTable()[i]));

    as->generatePrologue();
    // Make sure the ACC register is initialized and not clobbered by the caller.
    as->loadAccumulatorFromFrame();
    decode(code, len);
    as->generateEpilogue();

    as->link(function);
//    qDebug()<<"done";
}

#define STORE_IP() as->storeInstructionPointer(nextInstructionOffset())
#define STORE_ACC() as->saveAccumulatorInFrame()
#define LOAD_ACC() as->loadAccumulatorFromFrame()
#define BASELINEJIT_GENERATE_RUNTIME_CALL(function, destination) { \
    as->GENERATE_RUNTIME_CALL(function, destination); \
    if (Runtime::function::throws) \
        as->checkException(); \
    else {} } // this else prevents else statements after the macro from attaching to the if above

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
    // Don't clobber the accumulator.
    as->moveReg(srcReg, destReg);
}

void BaselineJIT::generate_LoadImport(int index)
{
    as->loadImport(index);
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
    BASELINEJIT_GENERATE_RUNTIME_CALL(RegexpLiteral, CallResultDestination::InAccumulator);
    as->storeReg(destReg);
}

void BaselineJIT::generate_LoadClosure(int value)
{
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(value, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(Closure, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_LoadName(int name)
{
    STORE_IP();
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(name, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(LoadName, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_LoadGlobalLookup(int index)
{
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(index, 2);
    as->passFunctionAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(LoadGlobalLookup, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_LoadQmlContextPropertyLookup(int index)
{
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(index, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(LoadQmlContextPropertyLookup, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_StoreNameSloppy(int name)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passAccumulatorAsArg(2);
    as->passInt32AsArg(name, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(StoreNameSloppy, CallResultDestination::Ignore);
    LOAD_ACC();
}

void BaselineJIT::generate_StoreNameStrict(int name)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passAccumulatorAsArg(2);
    as->passInt32AsArg(name, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(StoreNameStrict, CallResultDestination::Ignore);
    LOAD_ACC();
}

void BaselineJIT::generate_LoadElement(int base)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passAccumulatorAsArg(2);
    as->passJSSlotAsArg(base, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(LoadElement, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_StoreElement(int base, int index)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(4);
    as->passAccumulatorAsArg(3);
    as->passJSSlotAsArg(index, 2);
    as->passJSSlotAsArg(base, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(StoreElement, CallResultDestination::Ignore);
    LOAD_ACC();
}

void BaselineJIT::generate_LoadProperty(int name)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(name, 2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(LoadProperty, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_GetLookup(int index)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(index, 3);
    as->passAccumulatorAsArg(2);
    as->passFunctionAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(GetLookup, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_StoreProperty(int name, int base)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(4);
    as->passAccumulatorAsArg(3);
    as->passInt32AsArg(name, 2);
    as->passJSSlotAsArg(base, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(StoreProperty, CallResultDestination::Ignore);
    LOAD_ACC();
}

void BaselineJIT::generate_SetLookup(int index, int base)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(4);
    as->passAccumulatorAsArg(3);
    as->passInt32AsArg(index, 2);
    as->passJSSlotAsArg(base, 1);
    as->passFunctionAsArg(0);
    if (function->isStrict())
        BASELINEJIT_GENERATE_RUNTIME_CALL(SetLookupStrict, CallResultDestination::InAccumulator)
    else
        BASELINEJIT_GENERATE_RUNTIME_CALL(SetLookupSloppy, CallResultDestination::InAccumulator)
}

void BaselineJIT::generate_LoadSuperProperty(int property)
{
    STORE_IP();
    as->prepareCallWithArgCount(2);
    as->passJSSlotAsArg(property, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(LoadSuperProperty, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_StoreSuperProperty(int property)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passAccumulatorAsArg(2);
    as->passJSSlotAsArg(property, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(StoreSuperProperty, CallResultDestination::Ignore);
    LOAD_ACC();
}

void BaselineJIT::generate_Yield()
{
    // #####
    Q_UNREACHABLE();
}

void BaselineJIT::generate_YieldStar()
{
    // #####
    Q_UNREACHABLE();
}

void BaselineJIT::generate_Resume(int)
{
    // #####
    Q_UNREACHABLE();
}

void BaselineJIT::generate_CallValue(int name, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(argc, 3);
    as->passJSSlotAsArg(argv, 2);
    as->passJSSlotAsArg(name, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CallValue, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CallWithReceiver(int name, int thisObject, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passJSSlotAsArg(argv, 3);
    as->passJSSlotAsArg(thisObject, 2);
    as->passJSSlotAsArg(name, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CallWithReceiver, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CallProperty(int name, int base, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passJSSlotAsArg(argv, 3);
    as->passInt32AsArg(name, 2);
    as->passJSSlotAsArg(base, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CallProperty, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CallPropertyLookup(int lookupIndex, int base, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passJSSlotAsArg(argv, 3);
    as->passInt32AsArg(lookupIndex, 2);
    as->passJSSlotAsArg(base, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CallPropertyLookup, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CallElement(int base, int index, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passJSSlotAsArg(argv, 3);
    as->passJSSlotAsArg(index, 2);
    as->passJSSlotAsArg(base, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CallElement, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CallName(int name, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(argc, 3);
    as->passJSSlotAsArg(argv, 2);
    as->passInt32AsArg(name, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CallName, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CallPossiblyDirectEval(int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(argc, 2);
    as->passJSSlotAsArg(argv, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CallPossiblyDirectEval, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CallGlobalLookup(int index, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(argc, 3);
    as->passJSSlotAsArg(argv, 2);
    as->passInt32AsArg(index, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CallGlobalLookup, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CallQmlContextPropertyLookup(int index, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(argc, 3);
    as->passJSSlotAsArg(argv, 2);
    as->passInt32AsArg(index, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CallQmlContextPropertyLookup, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CallWithSpread(int func, int thisObject, int argc, int argv)
{
    STORE_IP();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passJSSlotAsArg(argv, 3);
    as->passJSSlotAsArg(thisObject, 2);
    as->passJSSlotAsArg(func, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CallWithSpread, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_TailCall(int func, int thisObject, int argc, int argv)
{
    STORE_IP();
    as->jsTailCall(func, thisObject, argc, argv);
}

void BaselineJIT::generate_Construct(int func, int argc, int argv)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passJSSlotAsArg(argv, 3);
    as->passAccumulatorAsArg(2);
    as->passJSSlotAsArg(func, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(Construct, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_ConstructWithSpread(int func, int argc, int argv)
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(5);
    as->passInt32AsArg(argc, 4);
    as->passJSSlotAsArg(argv, 3);
    as->passAccumulatorAsArg(2);
    as->passJSSlotAsArg(func, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(ConstructWithSpread, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_SetUnwindHandler(int offset)
{
    if (offset)
        labels.insert(as->setUnwindHandler(absoluteOffset(offset)));
    else
        as->clearUnwindHandler();
}

void BaselineJIT::generate_UnwindDispatch()
{
    as->unwindDispatch();
}

void BaselineJIT::generate_UnwindToLabel(int level, int offset)
{
    labels.insert(as->unwindToLabel(level, absoluteOffset(offset)));
}

void BaselineJIT::generate_DeadTemporalZoneCheck(int name)
{
    as->deadTemporalZoneCheck(nextInstructionOffset(), name);
}

void BaselineJIT::generate_ThrowException()
{
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(ThrowException, CallResultDestination::Ignore);
    as->gotoCatchException();
}

void BaselineJIT::generate_GetException() { as->getException(); }
void BaselineJIT::generate_SetException() { as->setException(); }

void BaselineJIT::generate_CreateCallContext()
{
    as->prepareCallWithArgCount(1);
    as->passCppFrameAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(PushCallContext, CallResultDestination::Ignore);
}

void BaselineJIT::generate_PushCatchContext(int index, int name) { as->pushCatchContext(index, name); }

void BaselineJIT::generate_PushWithContext()
{
    STORE_IP();
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(2);
    as->passJSSlotAsArg(CallData::Accumulator, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(PushWithContext, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_PushBlockContext(int index)
{
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(index, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(PushBlockContext, CallResultDestination::Ignore);
    as->loadAccumulatorFromFrame();
}

void BaselineJIT::generate_CloneBlockContext()
{
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CloneBlockContext, CallResultDestination::Ignore);
    as->loadAccumulatorFromFrame();
}

void BaselineJIT::generate_PushScriptContext(int index)
{
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(index, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(PushScriptContext, CallResultDestination::Ignore);
    as->loadAccumulatorFromFrame();
}

void BaselineJIT::generate_PopScriptContext()
{
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(PopScriptContext, CallResultDestination::Ignore);
    as->loadAccumulatorFromFrame();
}

void BaselineJIT::generate_PopContext() { as->popContext(); }

void BaselineJIT::generate_GetIterator(int iterator)
{
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(iterator, 2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(GetIterator, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_IteratorNext(int value, int done)
{
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(3);
    as->passJSSlotAsArg(value, 2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(IteratorNext, CallResultDestination::InAccumulator);
    as->storeReg(done);
}

void BaselineJIT::generate_IteratorNextForYieldStar(int iterator, int object)
{
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(4);
    as->passJSSlotAsArg(object, 3);
    as->passJSSlotAsArg(iterator, 2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(IteratorNextForYieldStar, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_IteratorClose(int done)
{
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(3);
    as->passJSSlotAsArg(done, 2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(IteratorClose, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_DestructureRestElement()
{
    as->saveAccumulatorInFrame();
    as->prepareCallWithArgCount(2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(DestructureRestElement, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_DeleteProperty(int base, int index)
{
    STORE_IP();
    as->prepareCallWithArgCount(4);
    as->passJSSlotAsArg(index, 3);
    as->passJSSlotAsArg(base, 2);
    as->passFunctionAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(DeleteProperty, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_DeleteName(int name)
{
    STORE_IP();
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(name, 2);
    as->passFunctionAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(DeleteName, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_TypeofName(int name)
{
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(name, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(TypeofName, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_TypeofValue()
{
    STORE_ACC();
    as->prepareCallWithArgCount(2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(TypeofValue, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_DeclareVar(int varName, int isDeletable)
{
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(varName, 2);
    as->passInt32AsArg(isDeletable, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(DeclareVar, CallResultDestination::Ignore);
    LOAD_ACC();
}

void BaselineJIT::generate_DefineArray(int argc, int args)
{
    as->prepareCallWithArgCount(3);
    as->passInt32AsArg(argc, 2);
    as->passJSSlotAsArg(args, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(ArrayLiteral, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_DefineObjectLiteral(int internalClassId, int argc, int args)
{
    as->prepareCallWithArgCount(4);
    as->passInt32AsArg(argc, 3);
    as->passJSSlotAsArg(args, 2);
    as->passInt32AsArg(internalClassId, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(ObjectLiteral, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CreateClass(int classIndex, int heritage, int computedNames)
{
    as->prepareCallWithArgCount(4);
    as->passJSSlotAsArg(computedNames, 3);
    as->passJSSlotAsArg(heritage, 2);
    as->passInt32AsArg(classIndex, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CreateClass, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CreateMappedArgumentsObject()
{
    as->prepareCallWithArgCount(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CreateMappedArgumentsObject,
                              CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CreateUnmappedArgumentsObject()
{
    as->prepareCallWithArgCount(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CreateUnmappedArgumentsObject,
                              CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CreateRestParameter(int argIndex)
{
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(argIndex, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(CreateRestParameter, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_ConvertThisToObject()
{
    STORE_ACC();
    as->prepareCallWithArgCount(2);
    as->passJSSlotAsArg(CallData::This, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(ConvertThisToObject, CallResultDestination::InAccumulator);
    as->storeReg(CallData::This);
    LOAD_ACC();
}

void BaselineJIT::generate_LoadSuperConstructor()
{
    as->prepareCallWithArgCount(2);
    as->passJSSlotAsArg(CallData::Function, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(LoadSuperConstructor, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_ToObject()
{
    STORE_ACC();
    as->prepareCallWithArgCount(2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(ToObject, CallResultDestination::InAccumulator);

}

void BaselineJIT::generate_Jump(int offset)
{
    labels.insert(as->jump(absoluteOffset(offset)));
}

void BaselineJIT::generate_JumpTrue(int offset)
{
    labels.insert(as->jumpTrue(absoluteOffset(offset)));
}

void BaselineJIT::generate_JumpFalse(int offset)
{
    labels.insert(as->jumpFalse(absoluteOffset(offset)));
}

void BaselineJIT::generate_JumpNoException(int offset)
{
    labels.insert(as->jumpNoException(absoluteOffset(offset)));
}

void BaselineJIT::generate_JumpNotUndefined(int offset)
{
    labels.insert(as->jumpNotUndefined(absoluteOffset(offset)));
}

void BaselineJIT::generate_CheckException()
{
    as->checkException();
}

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
    as->passJSSlotAsArg(lhs, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(In, CallResultDestination::InAccumulator);
}

void BaselineJIT::generate_CmpInstanceOf(int lhs)
{
    STORE_ACC();
    as->prepareCallWithArgCount(3);
    as->passAccumulatorAsArg(2);
    as->passJSSlotAsArg(lhs, 1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(Instanceof, CallResultDestination::InAccumulator);
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

void BaselineJIT::generate_Exp(int lhs) {
    STORE_IP();
    STORE_ACC();
    as->prepareCallWithArgCount(2);
    as->passAccumulatorAsArg(1);
    as->passJSSlotAsArg(lhs, 0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(Exp, CallResultDestination::InAccumulator);
}
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
//    as->callRuntime("binopContext", op, CallResultDestination::InAccumulator);
//    as->checkException();
//}

void BaselineJIT::generate_InitializeBlockDeadTemporalZone(int firstReg, int count)
{
    as->loadValue(Value::emptyValue().rawValue());
    for (int i = firstReg, end = firstReg + count; i < end; ++i)
        as->storeReg(i);
}

void BaselineJIT::generate_ThrowOnNullOrUndefined()
{
    STORE_ACC();
    as->prepareCallWithArgCount(2);
    as->passAccumulatorAsArg(1);
    as->passEngineAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(ThrowOnNullOrUndefined, CallResultDestination::Ignore);
    LOAD_ACC();
}

void BaselineJIT::generate_GetTemplateObject(int index)
{
    as->prepareCallWithArgCount(2);
    as->passInt32AsArg(index, 1);
    as->passFunctionAsArg(0);
    BASELINEJIT_GENERATE_RUNTIME_CALL(GetTemplateObject, CallResultDestination::InAccumulator);
}

ByteCodeHandler::Verdict BaselineJIT::startInstruction(Instr::Type /*instr*/)
{
    if (labels.contains(currentInstructionOffset()))
        as->addLabel(currentInstructionOffset());
    return ProcessInstruction;
}

void BaselineJIT::endInstruction(Instr::Type instr)
{
    Q_UNUSED(instr);
}
