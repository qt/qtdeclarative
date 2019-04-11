/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "qv4operation_p.h"
#include "qv4runtimesupport_p.h"

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace IR {

OperationBuilder::OperationBuilder(QQmlJS::MemoryPool *graphPool)
    : m_graphPool(graphPool)
{}

OperationBuilder *OperationBuilder::create(QQmlJS::MemoryPool *pool)
{
    return pool->New<OperationBuilder>(pool);
}

Operation *OperationBuilder::getConstant(Value v)
{
    Type t;
    switch (v.type()) {
    case Value::Undefined_Type: t = Type::undefinedType(); break;
    case Value::Integer_Type: t = Type::int32Type(); break;
    case Value::Boolean_Type: t = Type::booleanType(); break;
    case Value::Null_Type: t = Type::nullType(); break;
    case Value::Double_Type: t = Type::doubleType(); break;
    case Value::Managed_Type: t = Type::objectType(); break;
    default:
        if (v.isEmpty())
            t = Type::emptyType();
        else
            Q_UNREACHABLE();
    }
    return OperationWithPayload<ConstantPayload>::create(
                m_graphPool, Meta::Constant, 0, 0, 0, 1, 0, 0, t, Operation::NoFlags,
                ConstantPayload(v));
}

Operation *OperationBuilder::getParam(unsigned index, const Function::StringId name)
{
    return OperationWithPayload<ParameterPayload>::create(m_graphPool, Meta::Parameter,
                                                          1, 0, 0,
                                                          1, 0, 0,
                                                          Type::anyType(), Operation::NoFlags,
                                                          ParameterPayload(index, name));
}

Operation *OperationBuilder::getRegion(unsigned nControlInputs) //### cache common operands in the static pool
{
    return Operation::create(m_graphPool, Meta::Region,
                             0, 0, uint16_t(nControlInputs),
                             0, 0, 1,
                             Type(), Operation::NoFlags);
}

Operation *OperationBuilder::getPhi(unsigned nValueInputs) //### cache common operands in the static pool
{
    return Operation::create(m_graphPool, Meta::Phi,
                             uint16_t(nValueInputs), 0, 1,
                             1, 0, 0,
                             Type::anyType(), Operation::NoFlags);
}

Operation *OperationBuilder::getEffectPhi(unsigned nEffectInputs) //### cache common operands in the static pool
{
    return Operation::create(m_graphPool, Meta::EffectPhi,
                             0, uint16_t(nEffectInputs), 1,
                             0, 1, 0,
                             Type(), Operation::NoFlags);
}

Operation *OperationBuilder::getUnwindDispatch(unsigned nContinuations, int unwindHandlerOffset,
                                               int fallthroughSuccessor)
{
    return OperationWithPayload<UnwindDispatchPayload>::create(
                m_graphPool, Meta::UnwindDispatch,
                0, 1, 1, 0, nContinuations, nContinuations,
                Type(), Operation::NoFlags,
                UnwindDispatchPayload(unwindHandlerOffset,
                                      fallthroughSuccessor));
}

Operation *OperationBuilder::getHandleUnwind(int unwindHandlerOffset)
{
    return OperationWithPayload<HandleUnwindPayload>::create(m_graphPool, Meta::HandleUnwind,
                                                             0, 1, 1, 0, 1, 1,
                                                             Type(), Operation::NoFlags,
                                                             HandleUnwindPayload(unwindHandlerOffset));
}

Operation *OperationBuilder::getFrameState(uint16_t frameSize)
{
    if (m_opFrameState == nullptr)
        m_opFrameState = Operation::create(m_graphPool, Meta::FrameState,
                                          frameSize, 0, 0, 0, 0, 1,
                                          Type(), Operation::NoFlags);
    else
        Q_ASSERT(frameSize == m_opFrameState->valueInputCount());

    return m_opFrameState;
}

Operation *OperationBuilder::getStart(uint16_t outputCount)
{
    return Operation::create(m_graphPool, Meta::Start,
                             0, 0, 0,
                             outputCount, 1, 1,
                             Type(), Operation::NoFlags);
}

Operation *OperationBuilder::getEnd(uint16_t controlInputCount)
{
    return Operation::create(m_graphPool, Meta::End,
                             0, 0, controlInputCount,
                             0, 0, 0,
                             Type(), Operation::NoFlags);
}

inline Operation *createOperation(Operation::Kind kind, QQmlJS::MemoryPool *staticPool)
{
    auto get = [&](uint16_t inValueCount, uint16_t inEffectCount, uint16_t inControlCount,
                   uint16_t outValueCount, uint16_t outEffectCount, uint16_t outControlCount,
                   Type (*typeCreator)(), uint8_t flags) {
        return Operation::create(staticPool, kind, inValueCount, inEffectCount, inControlCount,
                                 outValueCount, outEffectCount, outControlCount, typeCreator(),
                                 flags);
    };

    using K = Operation::Kind;
    using F = Operation::Flags;
    const auto none    = &Type::noneType;
    const auto any     = &Type::anyType;
    const auto number  = &Type::numberType;
    const auto boolean = &Type::booleanType;

    switch (kind) {
    case K::Undefined:
        return OperationWithPayload<ConstantPayload>::create(
                staticPool, K::Undefined, 0, 0, 0, 1, 0, 0, Type::undefinedType(), F::NoFlags,
                ConstantPayload(Primitive::undefinedValue()));
    case K::Empty:
        return OperationWithPayload<ConstantPayload>::create(
                staticPool, K::Constant, 0, 0, 0, 1, 0, 0, Type::emptyType(), Operation::NoFlags,
                ConstantPayload(Primitive::emptyValue()));
    case K::Engine:                          return get(1, 0, 0, 1, 0, 0, none,    F::NoFlags);
    case K::CppFrame:                        return get(1, 0, 0, 1, 0, 0, none,    F::NoFlags);
    case K::Function:                        return get(1, 0, 0, 1, 0, 0, none,    F::NoFlags);
    case K::Jump:                            return get(0, 0, 1, 0, 0, 1, none,    F::NoFlags);
    case K::Return:                          return get(1, 1, 1, 0, 0, 1, none,    F::NoFlags);
    case K::Branch:                          return get(1, 0, 1, 0, 0, 2, none,    F::HasFrameStateInput | F::NeedsBytecodeOffsets);
    case K::IfTrue:                          return get(0, 0, 1, 0, 0, 1, none,    F::NoFlags);
    case K::IfFalse:                         return get(0, 0, 1, 0, 0, 1, none,    F::NoFlags);
    case K::SelectOutput:                    return get(3, 1, 1, 1, 1, 1, any,     F::NoFlags);
    case K::Throw:                           return get(1, 1, 1, 0, 1, 1, any,     F::NeedsBytecodeOffsets);
    case K::OnException:                     return get(0, 0, 1, 0, 0, 1, none,    F::NoFlags);
    case K::ThrowReferenceError:             return get(1, 1, 1, 0, 1, 1, any,     F::NeedsBytecodeOffsets);
    case K::UnwindToLabel:                   return get(2, 1, 1, 0, 1, 1, none,    F::NoFlags);
    case K::LoadRegExp:                      return get(1, 0, 0, 1, 0, 0, any,     F::NoFlags);
    case K::ScopedLoad:                      return get(2, 1, 0, 1, 1, 0, any,     F::NoFlags);
    case K::ScopedStore:                     return get(3, 1, 0, 0, 1, 0, none,    F::NoFlags);
    case K::JSLoadElement:                   return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSGetLookup:                     return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSLoadProperty:                  return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSStoreElement:                  return get(3, 1, 1, 0, 1, 2, none,    F::CanThrow);
    case K::JSSetLookupStrict:               return get(3, 1, 1, 0, 1, 2, none,    F::CanThrow);
    case K::JSSetLookupSloppy:               return get(3, 1, 1, 0, 1, 2, none,    F::CanThrow);
    case K::JSStoreProperty:                 return get(3, 1, 1, 0, 1, 2, none,    F::CanThrow);
    case K::JSLoadName:                      return get(1, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSLoadGlobalLookup:              return get(1, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSStoreNameSloppy:               return get(2, 1, 1, 0, 1, 2, none,    F::CanThrow);
    case K::JSStoreNameStrict:               return get(2, 1, 1, 0, 1, 2, none,    F::CanThrow);
    case K::JSLoadSuperProperty:             return get(1, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSStoreSuperProperty:            return get(2, 1, 1, 0, 1, 2, any,     F::CanThrow);
    case K::JSLoadClosure:                   return get(1, 1, 0, 1, 1, 0, any,     F::Pure);
    case K::JSGetIterator:                   return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);

    // special case: see GraphBuilder::generate_IteratorNext
    case K::JSIteratorNext:                  return get(2, 1, 1, 2, 1, 1, any,     F::NoFlags);

    // special case: see GraphBuilder::generate_IteratorNext
    case K::JSIteratorNextForYieldStar:      return get(3, 1, 1, 2, 1, 1, any,     F::NoFlags);

    case K::JSIteratorClose:                 return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSDeleteProperty:                return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSDeleteName:                    return get(1, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSIn:                            return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSInstanceOf:                    return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::QMLLoadQmlContextPropertyLookup: return get(1, 1, 1, 1, 1, 2, any,     F::CanThrow);

    case K::JSEqual:                         return get(2, 1, 1, 1, 1, 2, boolean, F::CanThrow);
    case K::JSGreaterThan:                   return get(2, 1, 1, 1, 1, 2, boolean, F::CanThrow);
    case K::JSGreaterEqual:                  return get(2, 1, 1, 1, 1, 2, boolean, F::CanThrow);
    case K::JSLessThan:                      return get(2, 1, 1, 1, 1, 2, boolean, F::CanThrow);
    case K::JSLessEqual:                     return get(2, 1, 1, 1, 1, 2, boolean, F::CanThrow);
    case K::JSStrictEqual:                   return get(2, 1, 1, 1, 1, 2, boolean, F::CanThrow);

    case K::JSAdd:                           return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSSubtract:                      return get(2, 1, 1, 1, 1, 2, number,  F::CanThrow);
    case K::JSMultiply:                      return get(2, 1, 1, 1, 1, 2, number,  F::CanThrow);
    case K::JSDivide:                        return get(2, 1, 1, 1, 1, 2, number,  F::CanThrow);
    case K::JSModulo:                        return get(2, 1, 1, 1, 1, 2, number,  F::CanThrow);
    case K::JSExponentiate:                  return get(2, 1, 1, 1, 1, 2, number,  F::CanThrow);

    case K::JSBitAnd:                        return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSBitOr:                         return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSBitXor:                        return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSUnsignedShiftRight:            return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSShiftRight:                    return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSShiftLeft:                     return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);

    case K::JSNegate:                        return get(1, 1, 1, 1, 1, 2, number,  F::CanThrow);
    case K::JSToNumber:                      return get(1, 1, 1, 1, 1, 2, number,  F::CanThrow);
    case K::Alloca:                          return get(1, 0, 0, 1, 0, 0, none,    F::NoFlags);

    //### it is questionable if VAAlloc/VASeal need effect edges
    case K::VAAlloc:                         return get(1, 1, 0, 1, 1, 0, none,    F::NoFlags);

    case K::VAStore:                         return get(3, 0, 0, 1, 0, 0, none,    F::NoFlags);

    case K::JSTypeofName:                    return get(1, 1, 0, 1, 1, 0, any,     F::NoFlags);
    case K::JSTypeofValue:                   return get(1, 0, 0, 1, 0, 0, any,     F::Pure);
    case K::JSDeclareVar:                    return get(2, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::JSDestructureRestElement:        return get(1, 1, 1, 1, 1, 2, any,     F::CanThrow);

    case K::JSCreateCallContext:             return get(0, 1, 1, 0, 1, 1, none,    F::NoFlags);
    case K::JSCreateCatchContext:            return get(2, 1, 1, 1, 1, 1, none,    F::NoFlags);
    case K::JSCreateWithContext:             return get(1, 1, 1, 1, 1, 1, any,     F::NoFlags);
    case K::JSCreateBlockContext:            return get(1, 1, 1, 1, 1, 1, none,    F::NoFlags);
    case K::JSCloneBlockContext:             return get(0, 1, 1, 0, 1, 1, none,    F::NoFlags);
    case K::JSCreateScriptContext:           return get(1, 1, 1, 1, 1, 1, none,    F::NoFlags);
    case K::JSPopScriptContext:              return get(0, 1, 1, 1, 1, 1, none,    F::NoFlags);
    case K::PopContext:                      return get(0, 1, 1, 0, 1, 1, none,    F::NoFlags);

    case K::JSThisToObject:                  return get(1, 1, 1, 0, 1, 2, any,     F::NoFlags);
    case K::JSCreateMappedArgumentsObject:   return get(0, 1, 0, 1, 1, 0, any,     F::NoFlags);
    case K::JSCreateUnmappedArgumentsObject: return get(0, 1, 0, 1, 1, 0, any,     F::NoFlags);
    case K::JSCreateRestParameter:           return get(1, 0, 0, 1, 0, 0, any,     F::NoFlags);
    case K::JSLoadSuperConstructor:          return get(1, 1, 1, 1, 1, 2, any,     F::NoFlags);
    case K::JSThrowOnNullOrUndefined:        return get(1, 1, 1, 0, 1, 2, none,    F::CanThrow);
    case K::JSGetTemplateObject:             return get(1, 0, 0, 1, 0, 0, any,     F::NoFlags);
    case K::StoreThis:                       return get(1, 1, 0, 1, 1, 0, any,     F::NoFlags);

    case K::GetException:                    return get(0, 1, 0, 1, 1, 0, any,     F::NoFlags);
    case K::SetException:                    return get(1, 1, 0, 0, 1, 0, any,     F::NoFlags);

    case K::ToObject:                        return get(1, 1, 1, 1, 1, 2, any,     F::CanThrow);
    case K::ToBoolean:                       return get(1, 0, 0, 1, 0, 0, boolean, F::Pure);

    case K::IsEmpty:                         return get(1, 0, 0, 1, 0, 0, boolean, F::Pure);

    case K::BooleanNot:                      return get(1, 0, 0, 1, 0, 0, boolean, F::NoFlags);
    case K::HasException:                    return get(1, 1, 0, 1, 1, 0, boolean, F::NoFlags);

    case K::Swap:                            return get(0, 0, 0, 0, 0, 0, none,    F::NoFlags);
    case K::Move:                            return get(1, 0, 0, 1, 0, 0, none,    F::NoFlags);

    default: // Non-static operations:
        return nullptr;
    }
}

Operation *OperationBuilder::staticOperation(Operation::Kind kind)
{
    static QAtomicPointer<Operation *> ops;
    if (Operation **staticOps = ops.load())
        return staticOps[kind];

    static QAtomicInt initializing = 0;
    if (initializing.testAndSetOrdered(0, 1)) {
        // This is safe now, because we can only run this piece of code once during the life time
        // of the application as we can only change initializing from 0 to 1 once.
        Operation **staticOps = new Operation *[Meta::KindsEnd];
        static QQmlJS::MemoryPool pool;
        for (int i = 0; i < Meta::KindsEnd; ++i)
            staticOps[i] = createOperation(Operation::Kind(i), &pool);
        bool success = ops.testAndSetOrdered(nullptr, staticOps);
        Q_ASSERT(success);
    } else {
        // Unfortunately we need to busy wait now until the other thread finishes the static
        // initialization;
        while (!ops.load()) {}
    }

    return ops.load()[kind];
}

Operation *OperationBuilder::getJSVarArgsCall(Operation::Kind kind, uint16_t argc)
{
    return Operation::create(m_graphPool, kind,
                             argc, 1, 1, 1, 1, 2,
                             Type::anyType(), Operation::CanThrow);
}

Operation *OperationBuilder::getJSTailCall(uint16_t argc)
{
    return Operation::create(m_graphPool, Meta::JSTailCall,
                             argc, 1, 1, 0, 0, 1,
                             Type(), Operation::NoFlags);
}

Operation *OperationBuilder::getTailCall()
{
    // special varargs call, takes cppframe, engine, func, thisObject, argv, argc
    return Operation::create(m_graphPool, Meta::TailCall,
                             6, 1, 1, 0, 0, 1,
                             Type(), Operation::NoFlags);
}

Operation *OperationBuilder::getCall(Operation::Kind callee)
{
    const bool canThrow = CallPayload::canThrow(callee);
    const Type retTy = CallPayload::returnType(callee);
    uint16_t nControlInputs = 0;
    uint16_t nControlOutputs = 0;
    if (canThrow) {
        nControlInputs = 1;
        nControlOutputs += 2;
    }
    if (CallPayload::changesContext(callee)) {
        nControlInputs = 1;
        nControlOutputs = std::max<uint16_t>(nControlInputs, 1);
    }
    if (callee == Meta::Throw || callee == Meta::ThrowReferenceError ||
            callee == Meta::JSIteratorNext || callee == Meta::JSIteratorNextForYieldStar) {
        nControlInputs = 1;
        nControlOutputs = 1;
    }
    Operation::Flags flags = Operation::NoFlags;
    if (canThrow)
        flags = Operation::Flags(flags | Operation::CanThrow);
    if (CallPayload::isPure(callee))
        flags = Operation::Flags(flags | Operation::Pure);
    const uint16_t nEffects = (flags & Operation::Pure) ? 0 : 1;
    const uint16_t nValueOutputs = retTy.isNone() ? 0 : 1;
    const uint16_t nValueInputs = CallPayload::argc(callee);

    return OperationWithPayload<CallPayload>::create(
                m_graphPool, Meta::Call,
                nValueInputs, nEffects, nControlInputs,
                nValueOutputs, nEffects, nControlOutputs,
                retTy, flags,
                CallPayload(callee));
}

Operation *OperationBuilder::getVASeal(uint16_t nElements)
{
    return Operation::create(m_graphPool, Meta::VASeal,
                             nElements + 1, 1, 0, 1, 1, 0,
                             Type::anyType(), Operation::NoFlags);
}

QString Operation::debugString() const
{
    switch (kind()) {

    case Meta::Constant:
        return QStringLiteral("Constant[%1]").arg(ConstantPayload::get(*this)->debugString());
    case Meta::Parameter:
        return QStringLiteral("Parameter[%1]").arg(ParameterPayload::get(*this)->debugString());
    case Meta::Call:
        return QStringLiteral("Call[%1]").arg(CallPayload::get(*this)->debugString());
    case Meta::UnwindDispatch:
        return QStringLiteral("UnwindDispatch[%1]").arg(UnwindDispatchPayload::get(*this)
                                                                ->debugString());
    case Meta::HandleUnwind:
        return QStringLiteral("HandleUnwind[%1]").arg(HandleUnwindPayload::get(*this)
                                                              ->debugString());

    default:
        return QString::fromLatin1(QMetaEnum::fromType<Meta::OpKind>().valueToKey(kind()));
    }
}

QString ConstantPayload::debugString() const
{
    return debugString(m_value);
}

QString ConstantPayload::debugString(QV4::Value v)
{
    if (v.isManaged())
        return QString::asprintf("Ptr: %p", v.heapObject());
    if (v.isEmpty())
        return QStringLiteral("empty");
    return v.toQStringNoThrow();
}

QString ParameterPayload::debugString() const
{
    return QStringLiteral("%1").arg(m_index);
}

QString UnwindDispatchPayload::debugString() const
{
    return QStringLiteral("%1, %2").arg(QString::number(m_fallthroughSuccessor),
                                    QString::number(m_unwindHandlerOffset));
}

QString HandleUnwindPayload::debugString() const
{
    return QStringLiteral("%1").arg(m_unwindHandlerOffset);
}

static Type translateType(RuntimeSupport::ArgumentType t)
{
    switch (t) {
    case RuntimeSupport::ArgumentType::Int: return Type::int32Type();
    case RuntimeSupport::ArgumentType::Bool: return Type::booleanType();
    case RuntimeSupport::ArgumentType::Void: return Type();
    case RuntimeSupport::ArgumentType::Engine: return Type::rawPointerType();
    case RuntimeSupport::ArgumentType::ValueRef: return Type::anyType();
    case RuntimeSupport::ArgumentType::ValueArray: return Type::anyType();
    case RuntimeSupport::ArgumentType::ReturnedValue: return Type::anyType();
    default: Q_UNREACHABLE();
    }
}

template<template<typename Operation> class M /* MetaOperation */, typename ReturnValue>
static ReturnValue operateOnRuntimeCall(Operation::Kind kind, bool abortOnMissingCall = true)
{
    using K = Operation::Kind;
    using R = Runtime;

    switch (kind) {
    case K::Throw:                           return M<R::ThrowException>::doIt();
    case K::ThrowReferenceError:             return M<R::ThrowReferenceError>::doIt();

    case K::JSEqual:                         return M<R::CompareEqual>::doIt();
    case K::JSGreaterThan:                   return M<R::CompareGreaterThan>::doIt();
    case K::JSGreaterEqual:                  return M<R::CompareGreaterEqual>::doIt();
    case K::JSLessThan:                      return M<R::CompareLessThan>::doIt();
    case K::JSLessEqual:                     return M<R::CompareLessEqual>::doIt();
    case K::JSStrictEqual:                   return M<R::CompareStrictEqual>::doIt();

    case K::JSBitAnd:                        return M<R::BitAnd>::doIt();
    case K::JSBitOr:                         return M<R::BitOr>::doIt();
    case K::JSBitXor:                        return M<R::BitXor>::doIt();
    case K::JSUnsignedShiftRight:            return M<R::UShr>::doIt();
    case K::JSShiftRight:                    return M<R::Shr>::doIt();
    case K::JSShiftLeft:                     return M<R::Shl>::doIt();

    case K::JSAdd:                           return M<R::Add>::doIt();
    case K::JSSubtract:                      return M<R::Sub>::doIt();
    case K::JSMultiply:                      return M<R::Mul>::doIt();
    case K::JSDivide:                        return M<R::Div>::doIt();
    case K::JSModulo:                        return M<R::Mod>::doIt();
    case K::JSExponentiate:                  return M<R::Exp>::doIt();

    case K::ToBoolean:                       return M<R::ToBoolean>::doIt();
    case K::ToObject:                        return M<R::ToObject>::doIt();

    case K::JSNegate:                        return M<R::UMinus>::doIt();
    case K::JSToNumber:                      return M<R::ToNumber>::doIt();

    case K::JSLoadName:                      return M<R::LoadName>::doIt();
    case K::JSLoadElement:                   return M<R::LoadElement>::doIt();
    case K::JSStoreElement:                  return M<R::StoreElement>::doIt();
    case K::JSGetLookup:                     return M<R::GetLookup>::doIt();
    case K::JSSetLookupStrict:               return M<R::SetLookupStrict>::doIt();
    case K::JSSetLookupSloppy:               return M<R::SetLookupSloppy>::doIt();
    case K::JSLoadProperty:                  return M<R::LoadProperty>::doIt();
    case K::JSStoreProperty:                 return M<R::StoreProperty>::doIt();
    case K::JSLoadGlobalLookup:              return M<R::LoadGlobalLookup>::doIt();
    case K::JSStoreNameSloppy:               return M<R::StoreNameSloppy>::doIt();
    case K::JSStoreNameStrict:               return M<R::StoreNameStrict>::doIt();
    case K::JSLoadSuperProperty:             return M<R::LoadSuperProperty>::doIt();
    case K::JSStoreSuperProperty:            return M<R::StoreSuperProperty>::doIt();
    case K::JSLoadClosure:                   return M<R::Closure>::doIt();
    case K::JSGetIterator:                   return M<R::GetIterator>::doIt();
    case K::JSIteratorNext:                  return M<R::IteratorNext>::doIt();
    case K::JSIteratorNextForYieldStar:      return M<R::IteratorNextForYieldStar>::doIt();
    case K::JSIteratorClose:                 return M<R::IteratorClose>::doIt();
    case K::JSDeleteProperty:                return M<R::DeleteProperty>::doIt();
    case K::JSDeleteName:                    return M<R::DeleteName>::doIt();
    case K::JSIn:                            return M<R::In>::doIt();
    case K::JSInstanceOf:                    return M<R::Instanceof>::doIt();
    case K::QMLLoadQmlContextPropertyLookup: return M<R::LoadQmlContextPropertyLookup>::doIt();

    case K::JSTypeofName:                    return M<R::TypeofName>::doIt();
    case K::JSTypeofValue:                   return M<R::TypeofValue>::doIt();
    case K::JSDeclareVar:                    return M<R::DeclareVar>::doIt();
    case K::JSDestructureRestElement:        return M<R::DestructureRestElement>::doIt();
    case K::JSThisToObject:                  return M<R::ConvertThisToObject>::doIt();
    case K::JSCreateMappedArgumentsObject:   return M<R::CreateMappedArgumentsObject>::doIt();
    case K::JSCreateUnmappedArgumentsObject: return M<R::CreateUnmappedArgumentsObject>::doIt();
    case K::JSCreateRestParameter:           return M<R::CreateRestParameter>::doIt();
    case K::JSLoadSuperConstructor:          return M<R::LoadSuperConstructor>::doIt();
    case K::JSThrowOnNullOrUndefined:        return M<R::ThrowOnNullOrUndefined>::doIt();

    case K::JSCreateCallContext:             return M<R::PushCallContext>::doIt();
    case K::JSCreateCatchContext:            return M<R::PushCatchContext>::doIt();
    case K::JSCreateWithContext:             return M<R::PushWithContext>::doIt();
    case K::JSCreateBlockContext:            return M<R::PushBlockContext>::doIt();
    case K::JSCloneBlockContext:             return M<R::CloneBlockContext>::doIt();
    case K::JSCreateScriptContext:           return M<R::PushScriptContext>::doIt();
    case K::JSPopScriptContext:              return M<R::PopScriptContext>::doIt();

    case K::LoadRegExp:                      return M<R::RegexpLiteral>::doIt();
    case K::JSGetTemplateObject:             return M<R::GetTemplateObject>::doIt();

    case K::JSCallName:                      return M<R::CallName>::doIt();
    case K::JSCallValue:                     return M<R::CallValue>::doIt();
    case K::JSCallElement:                   return M<R::CallElement>::doIt();
    case K::JSCallLookup:                    return M<R::CallPropertyLookup>::doIt();
    case K::JSCallProperty:                  return M<R::CallProperty>::doIt();
    case K::JSCallGlobalLookup:              return M<R::CallGlobalLookup>::doIt();
    case K::JSCallPossiblyDirectEval:        return M<R::CallPossiblyDirectEval>::doIt();
    case K::JSCallWithReceiver:              return M<R::CallWithReceiver>::doIt();
    case K::JSDefineObjectLiteral:           return M<R::ObjectLiteral>::doIt();
    case K::JSDefineArray:                   return M<R::ArrayLiteral>::doIt();
    case K::JSCallWithSpread:                return M<R::CallWithSpread>::doIt();
    case K::JSConstruct:                     return M<R::Construct>::doIt();
    case K::JSConstructWithSpread:           return M<R::ConstructWithSpread>::doIt();
    case K::JSTailCall:                      return M<R::TailCall>::doIt();
    case K::JSCreateClass:                   return M<R::CreateClass>::doIt();
    default:
        if (abortOnMissingCall)
            Q_UNREACHABLE();
        else
            return ReturnValue();
    }
}

template<typename Method>
struct IsRuntimeMethodOperation
{
    static constexpr bool doIt() { return true; }
};

bool CallPayload::isRuntimeCall(Operation::Kind m)
{
    return operateOnRuntimeCall<IsRuntimeMethodOperation, bool>(m, false);
}

QString CallPayload::debugString() const
{
    return QString::fromLatin1(QMetaEnum::fromType<Meta::OpKind>().valueToKey(m_callee));
}

template<typename Method>
struct MethodArgcOperation
{
    static constexpr unsigned doIt() { return RuntimeSupport::argumentCount<Method>(); }
};

unsigned CallPayload::argc(Operation::Kind callee)
{
    return operateOnRuntimeCall<MethodArgcOperation, unsigned>(callee);
}

template<typename Method> struct MethodArg1TyOperation { static constexpr RuntimeSupport::ArgumentType doIt() { return RuntimeSupport::arg1Type<Method>(); } };
template<typename Method> struct MethodArg2TyOperation { static constexpr RuntimeSupport::ArgumentType doIt() { return RuntimeSupport::arg2Type<Method>(); } };
template<typename Method> struct MethodArg3TyOperation { static constexpr RuntimeSupport::ArgumentType doIt() { return RuntimeSupport::arg3Type<Method>(); } };
template<typename Method> struct MethodArg4TyOperation { static constexpr RuntimeSupport::ArgumentType doIt() { return RuntimeSupport::arg4Type<Method>(); } };
template<typename Method> struct MethodArg5TyOperation { static constexpr RuntimeSupport::ArgumentType doIt() { return RuntimeSupport::arg5Type<Method>(); } };
template<typename Method> struct MethodArg6TyOperation { static constexpr RuntimeSupport::ArgumentType doIt() { return RuntimeSupport::arg6Type<Method>(); } };

static RuntimeSupport::ArgumentType untranslatedArgumentType(Operation::Kind m, unsigned arg)
{
    if (m == Meta::JSTailCall) {
        if (arg < 4)
            return RuntimeSupport::ArgumentType::ValueRef;
        else
            return RuntimeSupport::ArgumentType::Invalid;
    }

    switch (arg) {
    case 0: return operateOnRuntimeCall<MethodArg1TyOperation, RuntimeSupport::ArgumentType>(m);
    case 1: return operateOnRuntimeCall<MethodArg2TyOperation, RuntimeSupport::ArgumentType>(m);
    case 2: return operateOnRuntimeCall<MethodArg3TyOperation, RuntimeSupport::ArgumentType>(m);
    case 3: return operateOnRuntimeCall<MethodArg4TyOperation, RuntimeSupport::ArgumentType>(m);
    case 4: return operateOnRuntimeCall<MethodArg5TyOperation, RuntimeSupport::ArgumentType>(m);
    case 5: return operateOnRuntimeCall<MethodArg6TyOperation, RuntimeSupport::ArgumentType>(m);
    default: return RuntimeSupport::ArgumentType::Invalid;
    }
}

bool CallPayload::needsStorageOnJSStack(Operation::Kind m, unsigned arg, const Operation *op,
                                        Type nodeType)
{
    auto argTy = untranslatedArgumentType(m, arg);
    if (argTy == RuntimeSupport::ArgumentType::ValueArray)
        return true;
    if (argTy != RuntimeSupport::ArgumentType::ValueRef)
        return false;

    if (op->kind() == Meta::Constant)
        return true;

    return !nodeType.isObject() && !nodeType.isRawPointer() && !nodeType.isAny();
}

template<typename Method>
struct MethodRetTyOperation
{
    static constexpr RuntimeSupport::ArgumentType doIt() { return RuntimeSupport::retType<Method>(); }
};

Type CallPayload::returnType(Operation::Kind m)
{
    if (m == Meta::JSTailCall)
        return Type();

    auto t = operateOnRuntimeCall<MethodRetTyOperation, RuntimeSupport::ArgumentType>(m);
    return translateType(t);
}

static int firstArgumentPositionForType(Operation::Kind m, RuntimeSupport::ArgumentType type)
{
    if (operateOnRuntimeCall<MethodArg1TyOperation, RuntimeSupport::ArgumentType>(m) == type)
        return 1;
    if (operateOnRuntimeCall<MethodArg2TyOperation, RuntimeSupport::ArgumentType>(m) == type)
        return 2;
    if (operateOnRuntimeCall<MethodArg3TyOperation, RuntimeSupport::ArgumentType>(m) == type)
        return 3;
    if (operateOnRuntimeCall<MethodArg4TyOperation, RuntimeSupport::ArgumentType>(m) == type)
        return 4;
    if (operateOnRuntimeCall<MethodArg5TyOperation, RuntimeSupport::ArgumentType>(m) == type)
        return 5;
    if (operateOnRuntimeCall<MethodArg6TyOperation, RuntimeSupport::ArgumentType>(m) == type)
        return 6;
    return -1;
}

unsigned CallPayload::varArgsStart(Operation::Kind m)
{
    if (m == Meta::JSTailCall)
        return 4 - 1;

    int pos = firstArgumentPositionForType(m, RuntimeSupport::ArgumentType::ValueArray) - 1;
    Q_ASSERT(pos >= 0);
    return pos;
}

bool CallPayload::isVarArgsCall(Operation::Kind m)
{
    if (m == Meta::JSTailCall)
        return true;
    if (lastArgumentIsOutputValue(m))
        return false;
    return firstArgumentPositionForType(m, RuntimeSupport::ArgumentType::ValueArray) != -1;
}

bool CallPayload::isVarArgsCall() const
{
    return isVarArgsCall(m_callee);
}

template<typename Method>
struct MethodsLastArgumentIsOutputValue
{
    static constexpr bool doIt() { return Method::lastArgumentIsOutputValue; }
};

bool CallPayload::lastArgumentIsOutputValue(Operation::Kind m)
{
    return operateOnRuntimeCall<MethodsLastArgumentIsOutputValue, bool>(m);
}

template<typename Method>
struct MethodChangesContext
{
    static constexpr bool doIt() { return Method::changesContext; }
};

bool CallPayload::changesContext(Operation::Kind m)
{
    return operateOnRuntimeCall<MethodChangesContext, bool>(m);
}

template<typename Method>
struct MethodIsPure
{
    static constexpr bool doIt() { return Method::pure; }
};

bool CallPayload::isPure(Operation::Kind m)
{
    return operateOnRuntimeCall<MethodIsPure, bool>(m);
}

template<typename Method>
struct MethodCanThrow
{
    static constexpr bool doIt() { return Method::throws; }
};

bool CallPayload::canThrow(Operation::Kind m)
{
    switch (m) {
    case Meta::Throw: Q_FALLTHROUGH();
    case Meta::ThrowReferenceError:
        // the execution path following these instructions is already linked up to the exception handler
        return false;
    case Meta::JSIteratorNext: Q_FALLTHROUGH();
    case Meta::JSIteratorNextForYieldStar:
        // special case: see GraphBuilder::generate_IteratorNext
        return false;
    default:
        return operateOnRuntimeCall<MethodCanThrow, bool>(m);
    }
}

bool CallPayload::takesEngineAsArg(Operation::Kind m, int arg)
{
    return untranslatedArgumentType(m, arg) == RuntimeSupport::ArgumentType::Engine;
}

bool CallPayload::takesFunctionAsArg(Operation::Kind m, int arg)
{
    return untranslatedArgumentType(m, arg) == RuntimeSupport::ArgumentType::Function;
}

bool CallPayload::takesFrameAsArg(Operation::Kind m, int arg)
{
    return untranslatedArgumentType(m, arg) == RuntimeSupport::ArgumentType::Frame;
}

template<typename Method>
struct GetMethodPtr
{
    static constexpr void *doIt() { return reinterpret_cast<void *>(&Method::call); }
};

void *CallPayload::getMethodPtr(Operation::Kind m)
{
    return operateOnRuntimeCall<GetMethodPtr, void *>(m);
}

} // IR namespace
} // QV4 namespace
QT_END_NAMESPACE
