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

#ifndef QV4OPERATION_P_H
#define QV4OPERATION_P_H

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

#include <private/qv4ir_p.h>
#include <private/qqmljsmemorypool_p.h>

#include <QtCore/qatomic.h>

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

namespace Meta {
enum OpKind: uint16_t {
    FrameState,
    Start,
    End,

    Undefined,
    Constant,
    Parameter,
    Empty,
    Engine,
    CppFrame,
    Function,

    Jump,
    Return,
    JSTailCall,
    TailCall,
    Branch,
    IfTrue,
    IfFalse,
    Region,
    OnException,
    Phi,
    EffectPhi,
    SelectOutput,
    UnwindDispatch,
    UnwindToLabel,
    HandleUnwind,
    Throw,
    ThrowReferenceError,

    Call,

    LoadRegExp,
    ScopedLoad,
    ScopedStore,

    JSLoadElement,
    JSStoreElement,
    JSGetLookup,
    JSSetLookupStrict,
    JSSetLookupSloppy,
    JSLoadProperty,
    JSStoreProperty,
    JSLoadName,
    JSLoadGlobalLookup,
    JSStoreNameSloppy,
    JSStoreNameStrict,
    JSLoadSuperProperty,
    JSStoreSuperProperty,
    JSLoadClosure,
    JSGetIterator,
    JSIteratorNext,
    JSIteratorNextForYieldStar,
    JSIteratorClose,
    JSDeleteProperty,
    JSDeleteName,
    JSIn,
    JSInstanceOf,
    /* ok, these are qml object ops, but we don't care for now and treat them a s JS */
    QMLLoadScopeObjectProperty,
    QMLStoreScopeObjectProperty,
    QMLLoadContextObjectProperty,
    QMLStoreContextObjectProperty,
    QMLLoadIdObject,

    JSEqual,
    JSGreaterThan,
    JSGreaterEqual,
    JSLessThan,
    JSLessEqual,
    JSStrictEqual,

    JSAdd,
    JSSubtract,
    JSMultiply,
    JSDivide,
    JSModulo,
    JSExponentiate,

    JSBitAnd,
    JSBitOr,
    JSBitXor,
    JSUnsignedShiftRight,
    JSShiftRight,
    JSShiftLeft,

    JSNegate,
    JSToNumber,

    JSCallName,
    JSCallValue,
    JSCallElement,
    JSCallProperty,
    JSCallLookup,
    JSCallGlobalLookup,
    JSCallPossiblyDirectEval,
    JSCallWithReceiver,
    JSCallWithSpread,
    JSDefineObjectLiteral,
    JSDefineArray,
    JSCreateClass,
    JSConstruct,
    JSConstructWithSpread,
    /* ok, these are qml vararg calls, but we don't care for now and treat them as JS */
    QMLCallScopeObjectProperty,
    QMLCallContextObjectProperty,

    JSTypeofName,
    JSTypeofValue,
    JSDeclareVar,
    JSDestructureRestElement,
    QMLLoadContext,
    QMLLoadImportedScripts,
    JSThisToObject,
    JSCreateMappedArgumentsObject,
    JSCreateUnmappedArgumentsObject,
    JSCreateRestParameter,
    JSLoadSuperConstructor,
    JSThrowOnNullOrUndefined,
    JSGetTemplateObject,
    StoreThis,

    JSCreateCallContext,
    JSCreateCatchContext,
    JSCreateWithContext,
    JSCreateBlockContext,
    JSCloneBlockContext,
    JSCreateScriptContext,
    JSPopScriptContext,
    PopContext,

    GetException,
    SetException,

    ToObject,
    ToBoolean,

    //### do we need this? Or should a later phase generate JumpIsEmpty?
    IsEmpty,

    Alloca,
    VAAlloc,
    VAStore,
    VASeal,

    BooleanNot,
    HasException,

    // Low level, used by the register allocator and stack allocator:
    Swap,
    Move,
    KindsEnd
};
Q_NAMESPACE
Q_ENUM_NS(OpKind)
} // namespace Ops

class Operation
{
    Q_DISABLE_COPY_MOVE(Operation)

public:
    using Kind = Meta::OpKind;

    enum Flags: uint8_t {
        NoFlags              = 0,
        ThrowsFlag           = 1 << 0,
        Pure                 = 1 << 1, // no read/write side effect, cannot throw, cannot deopt, and is idempotent
        NeedsBytecodeOffsets = 1 << 2,

        CanThrow             = ThrowsFlag | NeedsBytecodeOffsets,

        HasFrameStateInput   = 1 << 3,
    };

public:
    static Operation *create(QQmlJS::MemoryPool *pool, Kind kind, uint16_t  inValueCount,
                             uint16_t inEffectCount, uint16_t  inControlCount,
                             uint16_t outValueCount, uint16_t outEffectCount,
                             uint16_t outControlCount, Type type, uint8_t flags)
    {
        return pool->New<Operation>(kind, inValueCount, inEffectCount, inControlCount,
                                    outValueCount, outEffectCount, outControlCount,
                                    type, Flags(flags));
    }

    Kind kind() const
    { return m_kind; }

    bool isConstant() const
    {
        switch (kind()) {
        case Meta::Undefined: Q_FALLTHROUGH();
        case Meta::Constant:
        case Meta::Empty:
            return true;
        default:
            return false;
        }
    }

    QString debugString() const;

    uint16_t valueInputCount() const { return m_inValueCount; }
    uint16_t effectInputCount() const { return m_inEffectCount; }
    uint16_t controlInputCount() const { return m_inControlCount; }
    uint16_t valueOutputCount() const { return m_outValueCount; }
    uint16_t effectOutputCount() const { return m_outEffectCount; }
    uint16_t controlOutputCount() const { return m_outControlCount; }

    unsigned indexOfFirstEffect() const { return m_inValueCount; }
    unsigned indexOfFirstControl() const { return m_inValueCount + m_inEffectCount; }
    unsigned indexOfFrameStateInput() const
    {
        return hasFrameStateInput() ? indexOfFirstControl() + m_inControlCount
                                    : std::numeric_limits<unsigned>::max();
    }

    Type type() const
    { return m_type; }

    bool canThrow() const
    { return m_flags & ThrowsFlag; }

    bool isPure() const
    { return m_flags & Pure; }

    bool needsBytecodeOffsets() const
    { return m_flags & NeedsBytecodeOffsets; }

    bool hasFrameStateInput() const
    { return m_flags & HasFrameStateInput; }

    unsigned totalInputCount() const
    {
        return valueInputCount() + effectInputCount() + controlInputCount() +
                (hasFrameStateInput() ? 1 : 0);
    }
    unsigned totalOutputCount() const { return valueOutputCount() + effectOutputCount() + controlOutputCount(); }

protected:
    friend class QQmlJS::MemoryPool;
    Operation(Kind kind,
              uint16_t  inValueCount, uint16_t  inEffectCount, uint16_t  inControlCount,
              uint16_t outValueCount, uint16_t outEffectCount, uint16_t outControlCount,
              Type type, uint8_t flags)
        : m_kind(kind)
        , m_inValueCount(inValueCount)
        , m_inEffectCount(inEffectCount)
        , m_inControlCount(inControlCount)
        , m_outValueCount(outValueCount)
        , m_outEffectCount(outEffectCount)
        , m_outControlCount(outControlCount)
        , m_type(type)
        , m_flags(Flags(flags))
    {
    }

    ~Operation() = default;

private:
    Kind m_kind;
    uint16_t m_inValueCount;
    uint16_t m_inEffectCount;
    uint16_t m_inControlCount;
    uint16_t m_outValueCount;
    uint16_t m_outEffectCount;
    uint16_t m_outControlCount;
    Type m_type;
    Flags m_flags;
};

template <typename Payload>
class OperationWithPayload: public Operation
{
public:
    static OperationWithPayload *create(QQmlJS::MemoryPool *pool, Kind kind,
                                        uint16_t  inValueCount, uint16_t  inEffectCount, uint16_t  inControlCount,
                                        uint16_t outValueCount, uint16_t outEffectCount, uint16_t outControlCount,
                                        Type type, Flags flags, Payload payload)
    {
        return pool->New<OperationWithPayload>(kind, inValueCount, inEffectCount, inControlCount,
                                               outValueCount, outEffectCount, outControlCount,
                                               type, flags, payload);
    }

    const Payload &payload() const
    { return m_payload; }

protected:
    friend class QQmlJS::MemoryPool;
    OperationWithPayload(Kind kind,
                         uint16_t  inValueCount, uint16_t  inEffectCount, uint16_t  inControlCount,
                         uint16_t outValueCount, uint16_t outEffectCount, uint16_t outControlCount,
                         Type type, Flags flags, Payload payload)
        : Operation(kind,
                    inValueCount, inEffectCount, inControlCount,
                    outValueCount, outEffectCount, outControlCount,
                    type, flags)
        , m_payload(payload)
    {}

    ~OperationWithPayload() = default;

private:
    Payload m_payload;
};

class ConstantPayload
{
public:
    explicit ConstantPayload(QV4::Value v)
        : m_value(v)
    {}

    QV4::Value value() const
    { return m_value; }

    static const ConstantPayload *get(const Operation &op)
    {
        if (op.kind() != Meta::Constant)
            return nullptr;

        return &static_cast<const OperationWithPayload<ConstantPayload>&>(op).payload();
    }

    QString debugString() const;
    static QString debugString(QV4::Value v);

private:
    QV4::Value m_value;
};

class ParameterPayload
{
public:
    ParameterPayload(size_t index, Function::StringId stringId)
        : m_index(index)
        , m_stringId(stringId)
    {}

    size_t parameterIndex() const
    { return m_index; }

    Function::StringId stringId() const
    { return m_stringId; }

    static const ParameterPayload *get(const Operation &op)
    {
        if (op.kind() != Meta::Parameter)
            return nullptr;

        return &static_cast<const OperationWithPayload<ParameterPayload>&>(op).payload();
    }

    QString debugString() const;

private:
    size_t m_index;
    Function::StringId m_stringId;
};

class CallPayload
{
public:
    CallPayload(Operation::Kind callee)
        : m_callee(callee)
    {}

    static const CallPayload *get(const Operation &op)
    {
        if (op.kind() != Meta::Call)
            return nullptr;

        return &static_cast<const OperationWithPayload<CallPayload>&>(op).payload();
    }

    static bool isRuntimeCall(Operation::Kind m);

    Operation::Kind callee() const { return m_callee; }
    QString debugString() const;

    unsigned argc() const { return argc(m_callee); }
    static unsigned argc(Operation::Kind callee);
    static bool needsStorageOnJSStack(Operation::Kind m, unsigned arg, const Operation *op,
                                      Type nodeType);
    static Type returnType(Operation::Kind m);
    static int engineArgumentPosition(Operation::Kind m);
    static int functionArgumentPosition(Operation::Kind m);

    static constexpr unsigned NoVarArgs = std::numeric_limits<unsigned>::max();
    static unsigned varArgsStart(Operation::Kind m);
    static bool isVarArgsCall(Operation::Kind m);
    bool isVarArgsCall() const;
    static bool lastArgumentIsOutputValue(Operation::Kind m);
    static bool changesContext(Operation::Kind m);
    static bool isPure(Operation::Kind m);
    static bool canThrow(Operation::Kind m);
    static bool takesEngineAsArg(Operation::Kind m, int arg);
    static bool takesFunctionAsArg(Operation::Kind m, int arg);
    static bool takesFrameAsArg(Operation::Kind m, int arg);
    static void *getMethodPtr(Operation::Kind m);

private:
    Operation::Kind m_callee;
};

class UnwindDispatchPayload
{
public:
    UnwindDispatchPayload(int unwindHandlerOffset, int fallthroughSuccessor)
        : m_unwindHandlerOffset(unwindHandlerOffset)
        , m_fallthroughSuccessor(fallthroughSuccessor)
    {}

    int unwindHandlerOffset() const
    { return m_unwindHandlerOffset; }

    int fallthroughSuccessor() const //### unused...
    { return m_fallthroughSuccessor; }

    static const UnwindDispatchPayload *get(const Operation &op)
    {
        if (op.kind() != Meta::UnwindDispatch)
            return nullptr;

        return &static_cast<const OperationWithPayload<UnwindDispatchPayload>&>(op).payload();
    }

    QString debugString() const;

private:
    int m_unwindHandlerOffset;
    int m_fallthroughSuccessor;
};

class HandleUnwindPayload
{
public:
    HandleUnwindPayload(int unwindHandlerOffset)
        : m_unwindHandlerOffset(unwindHandlerOffset)
    {}

    int unwindHandlerOffset() const
    { return m_unwindHandlerOffset; }

    static const HandleUnwindPayload *get(const Operation &op)
    {
        if (op.kind() != Meta::HandleUnwind)
            return nullptr;

        return &static_cast<const OperationWithPayload<HandleUnwindPayload>&>(op).payload();
    }

    QString debugString() const;

private:
    int m_unwindHandlerOffset;
};

class OperationBuilder
{
    Q_DISABLE_COPY_MOVE(OperationBuilder)

    friend class QQmlJS::MemoryPool;
    OperationBuilder(QQmlJS::MemoryPool *graphPool);

public:
    static OperationBuilder *create(QQmlJS::MemoryPool *pool);
    ~OperationBuilder() = delete;

    Operation *getConstant(QV4::Value v);
    Operation *getFrameState(uint16_t frameSize);
    Operation *getStart(uint16_t outputCount);
    Operation *getEnd(uint16_t controlInputCount);
    Operation *getParam(unsigned index, Function::StringId name);
    Operation *getRegion(unsigned nControlInputs);
    Operation *getPhi(unsigned nValueInputs);
    Operation *getEffectPhi(unsigned nEffectInputs);
    Operation *getUnwindDispatch(unsigned nControlOutputs, int unwindHandlerOffset, int fallthroughSuccessor);
    Operation *getHandleUnwind(int unwindHandlerOffset);

    template<Operation::Kind kind>
    Operation *get() {
        return staticOperation(kind);
    }

    Operation *getVASeal(uint16_t nElements);

    Operation *getJSVarArgsCall(Operation::Kind kind, uint16_t argc);
    Operation *getJSTailCall(uint16_t argc);
    Operation *getTailCall();

    Operation *getCall(Operation::Kind callee);

private:
    QQmlJS::MemoryPool *m_graphPool; // used to store per-graph nodes
    Operation *m_opFrameState = nullptr;
    static Operation *staticOperation(Operation::Kind kind);
};

} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4OPERATION_P_H
