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

#include <QtCore/qloggingcategory.h>

#include "qv4graphbuilder_p.h"
#include "qv4function_p.h"
#include "qv4lookup_p.h"
#include "qv4stackframe_p.h"
#include "qv4operation_p.h"

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace IR {

Q_LOGGING_CATEGORY(lcIRGraphBuilder, "qt.v4.ir.graphbuilder")

using MemoryPool = QQmlJS::MemoryPool;

namespace {
template <typename T, size_t N>
char (&ArraySizeHelper(T (&array)[N]))[N];
template <typename T, size_t N>
char (&ArraySizeHelper(const T (&array)[N]))[N];

template <typename Array>
inline size_t arraySize(const Array &array)
{
    Q_UNUSED(array); // for MSVC
    return sizeof(ArraySizeHelper(array));
}
} // anonymous namespace

class GraphBuilder::InterpreterEnvironment
{
public:
    struct FrameState: public QQmlJS::FixedPoolArray<Node *>
    {
        FrameState(MemoryPool *pool, int totalSlotCount)
            : FixedPoolArray(pool, totalSlotCount)
        {}

        Node *&unwindHandlerOffset()
        { return at(size() - 1); }

        static FrameState *create(MemoryPool *pool, int jsSlotCount)
        {
            auto totalSlotCount = jsSlotCount;
            auto fs = pool->New<FrameState>(pool, totalSlotCount);
            return fs;
        }

        static FrameState *clone(MemoryPool *pool, FrameState *other)
        {
            FrameState *fs = create(pool, other->size());

            for (int i = 0, ei = other->size(); i != ei; ++i)
                fs->at(i) = other->at(i);

            return fs;
        }
    };

public:
    InterpreterEnvironment(GraphBuilder *graphBuilder, Node *controlDependency)
        : m_graphBuilder(graphBuilder)
        , m_effectDependency(controlDependency)
        , m_controlDependency(controlDependency)
        , m_currentFrame(nullptr)
    {}

    void createEnvironment()
    {
        Function *f = function();
        QV4::Function *v4Function = f->v4Function();
        const size_t nRegisters = v4Function->compiledFunction->nRegisters;

        // 1 extra slot for the unwindHandlerOffset
        m_currentFrame = FrameState::create(graph()->pool(), int(nRegisters + 1));
    }

    void setupStartEnvironment()
    {
        Function *f = function();
        QV4::Function *v4Function = f->v4Function();
        const size_t nFormals = v4Function->compiledFunction->nFormals;
        const size_t nRegisters = v4Function->compiledFunction->nRegisters;

        createEnvironment();

        Node *startNode = graph()->startNode();
        auto opB = opBuilder();
        auto create = [&](int index, const char *name) {
            m_currentFrame->at(index) = graph()->createNode(
                        opB->getParam(index, f->addString(QLatin1String(name))), startNode);
        };
        create(0, "%function");
        create(1, "%context");
        create(2, "%acc");
        create(3, "%this");
        create(4, "%newTarget");
        create(5, "%argc");
        const quint32_le *formalNameIdx = v4Function->compiledFunction->formalsTable();
        for (size_t i = 0; i < nFormals; ++i, ++formalNameIdx) {
            const int slot = int(CallData::HeaderSize() + i);
            Q_ASSERT(*formalNameIdx <= quint32(std::numeric_limits<int>::max()));
            auto op = opB->getParam(
                        slot,
                        f->addString(v4Function->compilationUnit->stringAt(int(*formalNameIdx))));
            Node *argNode = graph()->createNode(op, startNode);
            m_currentFrame->at(slot) = argNode;
        }
        Node *undefinedNode = graph()->undefinedNode();
        Node *emptyNode = graph()->emptyNode();
        const auto firstDeadZoneRegister
                = v4Function->compiledFunction->firstTemporalDeadZoneRegister;
        const auto registerDeadZoneSize
                = v4Function->compiledFunction->sizeOfRegisterTemporalDeadZone;
        for (size_t i = CallData::HeaderSize() + nFormals; i < nRegisters; ++i) {
            const bool isDead = i >= firstDeadZoneRegister
                    && i < size_t(firstDeadZoneRegister + registerDeadZoneSize);
            m_currentFrame->at(int(i)) = isDead ? emptyNode : undefinedNode;
        }
        setUnwindHandlerOffset(0);
    }

    Function *function() const { return m_graphBuilder->function(); }
    Graph *graph() const { return function()->graph(); }
    OperationBuilder *opBuilder() const { return graph()->opBuilder(); }
    GraphBuilder *graphBuilder() const { return m_graphBuilder; }

    Node *bindAcc(Node *node)
    {
        bindNodeToSlot(node, CallData::Accumulator);
        return node;
    }

    Node *accumulator() const
    { return slot(CallData::Accumulator); }

    Node *bindNodeToSlot(Node *node, int slot)
    {
        m_currentFrame->at(size_t(slot)) = node;
        return node;
    }

    Node *slot(int slot) const
    { return m_currentFrame->at(slot); }

    int slotCount() const
    { return m_currentFrame->size(); }

    Node *effectDependency() const
    { return m_effectDependency; }

    void setEffectDependency(Node *newNode)
    { m_effectDependency = newNode; }

    Node *controlDependency() const
    { return m_controlDependency; }

    void setControlDependency(Node *newNode)
    { m_controlDependency = newNode; }

    Node *createFrameState()
    {
        return graph()->createNode(graphBuilder()->opBuilder()->getFrameState(slotCount()),
                                   m_currentFrame->begin(), slotCount());
    }

    Node *merge(InterpreterEnvironment *other);

    InterpreterEnvironment *copy() const
    {
        auto *newEnv = graph()->pool()->New<InterpreterEnvironment>(graphBuilder(),
                                                                    controlDependency());
        newEnv->setEffectDependency(effectDependency());
        newEnv->m_currentFrame = FrameState::clone(graph()->pool(), m_currentFrame);
        return newEnv;
    }

    int unwindHandlerOffset() const
    {
        auto uhOp = m_currentFrame->unwindHandlerOffset()->operation();
        Q_ASSERT(uhOp->kind() == Meta::Constant);
        return ConstantPayload::get(*uhOp)->value().int_32();
    }

    void setUnwindHandlerOffset(int newOffset)
    { m_currentFrame->unwindHandlerOffset() = graphBuilder()->createConstant(newOffset); }

    FrameState *frameState() const
    { return m_currentFrame; }

private:
    GraphBuilder *m_graphBuilder;
    Node *m_effectDependency;
    Node *m_controlDependency;
    FrameState *m_currentFrame;
};

namespace {
class InterpreterSubEnvironment final
{
    Q_DISABLE_COPY_MOVE(InterpreterSubEnvironment)

public:
    explicit InterpreterSubEnvironment(GraphBuilder *builder)
        : m_builder(builder)
        , m_parent(builder->env()->copy())
    {}

    ~InterpreterSubEnvironment()
    { m_builder->setEnv(m_parent); }

private:
    GraphBuilder *m_builder;
    GraphBuilder::InterpreterEnvironment *m_parent;
};
} // anonymous namespace

Node *GraphBuilder::InterpreterEnvironment::merge(InterpreterEnvironment *other)
{
    Q_ASSERT(m_currentFrame->size() == other->m_currentFrame->size());

    auto gb = graphBuilder();
    Node *mergedControl = gb->mergeControl(controlDependency(), other->controlDependency());
    setControlDependency(mergedControl);
    setEffectDependency(gb->mergeEffect(effectDependency(), other->effectDependency(), mergedControl));

    // insert/update phi nodes, but not for the unwind handler:
    for (int i = 0, ei = m_currentFrame->size() - 1; i != ei; ++i) {
        //### use lifeness info to trim this!
        m_currentFrame->at(i) = gb->mergeValue(m_currentFrame->at(i),
                                              other->m_currentFrame->at(i),
                                              mergedControl);
    }
    Q_ASSERT(unwindHandlerOffset() >= 0); // specifically: don't crash
    return mergedControl;
}

void GraphBuilder::buildGraph(IR::Function *function)
{
    const char *code = function->v4Function()->codeData;
    uint len = function->v4Function()->compiledFunction->codeSize;

    GraphBuilder builder(function);
    builder.startGraph();

    InterpreterEnvironment initial(&builder, function->graph()->startNode());
    initial.setupStartEnvironment();
    builder.setEnv(&initial);
    builder.graph()->setInitialFrameState(initial.createFrameState());
    builder.decode(code, len);
    builder.endGraph();
};

GraphBuilder::GraphBuilder(IR::Function *function)
    : m_func(function)
    , m_graph(function->graph())
    , m_currentEnv(nullptr)
{
    for (unsigned i = 0, ei = m_func->v4Function()->compiledFunction->nLabelInfos; i != ei; ++i) {
        unsigned label = m_func->v4Function()->compiledFunction->labelInfoTable()[i];
        m_labelInfos.emplace_back(label);
        if (lcIRGraphBuilder().isDebugEnabled()) {
            const LabelInfo &li = m_labelInfos.back();
            qCDebug(lcIRGraphBuilder) << "Loop start at" << li.labelOffset;
        }
    }
}

void GraphBuilder::startGraph()
{
    size_t nValuesOut = 1 + CallData::HeaderSize()
            + m_func->v4Function()->compiledFunction->nFormals;
    Node *start = m_graph->createNode(opBuilder()->getStart(uint16_t(nValuesOut)), nullptr, 0);
    m_func->nodeInfo(start)->setBytecodeOffsets(0, 0);
    m_graph->setStartNode(start);
    m_graph->setEngineNode(m_graph->createNode(opBuilder()->get<Meta::Engine>(), &start, 1));
    auto frame = m_graph->createNode(opBuilder()->get<Meta::CppFrame>(), &start, 1);
    m_graph->setCppFrameNode(frame);
    m_graph->setFunctionNode(m_graph->createNode(opBuilder()->get<Meta::Function>(),
                                                 &frame, 1));
}

void GraphBuilder::endGraph()
{
    const auto inputCount = uint16_t(m_exitControls.size());
    Node **inputs = &m_exitControls.front();
    Q_ASSERT(m_graph->endNode() == nullptr);
    m_graph->setEndNode(m_graph->createNode(opBuilder()->getEnd(inputCount), inputs, inputCount));
}

Node *GraphBuilder::bindAcc(Node *n)
{
    return env()->bindAcc(n);
}

/* IMPORTANT!!!
 *
 * This might change the success environment, so don't call:
 *   env()->bindAcc(createNode(...))
 * because the binding should only happen on success, but the call to env() will get the
 * environment from *before* the new success environment was created. Instead, do:
 *   bindAcc(createNode(....))
 */
Node *GraphBuilder::createAndLinkNode(Operation *op, Node *operands[], size_t opCount,
                                      bool incomplete)
{
    Q_ASSERT(op->effectInputCount() < 2);
    Q_ASSERT(op->controlInputCount() < 2);

    QVarLengthArray<Node *, 32> inputs(static_cast<int>(opCount));
    std::copy_n(operands, opCount, inputs.data());

    if (op->effectInputCount() == 1)
        inputs.append(env()->effectDependency());
    if (op->controlInputCount() == 1)
        inputs.append(env()->controlDependency());
    if (op->hasFrameStateInput())
        inputs.append(env()->createFrameState());

    Node *node = m_graph->createNode(op, inputs.data(), inputs.size(), incomplete);

    if (op->needsBytecodeOffsets()) {
        m_func->nodeInfo(node)->setBytecodeOffsets(currentInstructionOffset(),
                                                   nextInstructionOffset());
    }

    if (op->effectOutputCount() > 0)
        env()->setEffectDependency(node);
    if (op->controlOutputCount() > 0)
        env()->setControlDependency(node);

    if (op->canThrow() && env()->unwindHandlerOffset()) {
        InterpreterSubEnvironment successEnv(this);
        Node *control = env()->controlDependency();
        control = m_graph->createNode(opBuilder()->get<Meta::OnException>(), &control, 1);
        env()->setControlDependency(control);
        auto unwindHandlerOffset = env()->unwindHandlerOffset();
        mergeIntoSuccessor(unwindHandlerOffset);
    }

    return node;
}

Node *GraphBuilder::createNode(Operation *op, bool incomplete)
{
    return createAndLinkNode(op, nullptr, 0, incomplete);
}

Node *GraphBuilder::createNode(Operation *op, Node *n1)
{
    Node *buf[] = { n1 };
    return createAndLinkNode(op, buf, arraySize(buf));
}

Node *GraphBuilder::createNode(Operation *op, Node *n1, Node *n2)
{
    Node *buf[] = { n1, n2 };
    return createAndLinkNode(op, buf, arraySize(buf));
}

Node *GraphBuilder::createNode(Operation *op, Node *n1, Node *n2, Node *n3)
{
    Node *buf[] = { n1, n2, n3 };
    return createAndLinkNode(op, buf, arraySize(buf));
}

Node *GraphBuilder::createNode(Operation *op, Node *n1, Node *n2, Node *n3, Node *n4)
{
    Node *buf[] = { n1, n2, n3, n4 };
    return createAndLinkNode(op, buf, arraySize(buf));
}

Node *GraphBuilder::createRegion(unsigned nControlInputs)
{
    return createNode(opBuilder()->getRegion(nControlInputs), true);
}

Node *GraphBuilder::createIfTrue()
{
    return createNode(opBuilder()->get<Meta::IfTrue>());
}

Node *GraphBuilder::createIfFalse()
{
    return createNode(opBuilder()->get<Meta::IfFalse>());
}

Node *GraphBuilder::createConstant(int v)
{
    return m_graph->createNode(opBuilder()->getConstant(Primitive::fromInt32(v)));
}

Node *GraphBuilder::createPhi(unsigned nInputs, Node *input, Node *control)
{
    auto phiOp = opBuilder()->getPhi(nInputs);
    QVarLengthArray<Node *, 32> buffer(int(nInputs + 1));
    std::fill_n(buffer.data(), nInputs, input);
    buffer[int(nInputs)] = control;
    return m_graph->createNode(phiOp, buffer.data(), nInputs + 1, true);
}

Node *GraphBuilder::createEffectPhi(unsigned nInputs, Node *input, Node *control)
{
    auto phiOp = opBuilder()->getEffectPhi(nInputs);
    QVarLengthArray<Node *, 32> buffer(int(nInputs + 1));
    std::fill_n(buffer.data(), nInputs, input);
    buffer[int(nInputs)] = control;
    return m_graph->createNode(phiOp, buffer.data(), nInputs + 1, true);
}

Node *GraphBuilder::createHandleUnwind(int offset)
{
    return createNode(opBuilder()->getHandleUnwind(offset));
}

Node *GraphBuilder::mergeControl(Node *c1, Node *c2)
{
    if (c1->operation()->kind() == Meta::Region) {
        const unsigned nInputs = c1->operation()->controlInputCount() + 1;
        c1->addInput(m_graph->pool(), c2);
        c1->setOperation(opBuilder()->getRegion(nInputs));
        return c1;
    }
    auto op = opBuilder()->getRegion(2);
    Node *inputs[] = { c1, c2 };
    return m_graph->createNode(op, inputs, 2);
}

Node *GraphBuilder::mergeEffect(Node *e1, Node *e2, Node *control)
{
    const unsigned nInputs = control->operation()->controlInputCount();
    if (e1->operation()->kind() == Meta::EffectPhi && e1->controlInput() == control) {
        e1->insertInput(m_graph->pool(), nInputs - 1, e2);
        e1->setOperation(opBuilder()->getEffectPhi(nInputs));
        return e1;
    }

    if (e1 != e2) {
        Node *phi = createEffectPhi(nInputs, e1, control);
        phi->replaceInput(nInputs - 1, e2);
        return phi;
    }

    return e1;
}

Node *GraphBuilder::mergeValue(Node *v1, Node *v2, Node *control)
{
    const unsigned nInputs = control->operation()->controlInputCount();
    if (v1->operation()->kind() == Meta::Phi && v1->controlInput() == control) {
        v1->insertInput(m_graph->pool(), nInputs - 1, v2);
        v1->setOperation(opBuilder()->getPhi(nInputs));
        return v1;
    }

    if (v1 != v2) {
        Node *phi = createPhi(nInputs, v1, control);
        phi->replaceInput(nInputs - 1, v2);
        return phi;
    }

    return v1;
}

Node *GraphBuilder::createToBoolean(Node *input)
{
    return createNode(opBuilder()->get<Meta::ToBoolean>(), input);
}

void GraphBuilder::populate(VarArgNodes &args, int argc, int argv)
{
    for (int i = 0; i < argc; ++i)
        args.append(env()->slot(argv + i));
    Q_ASSERT(argc >= 0 && argc <= std::numeric_limits<uint16_t>::max());
}

void GraphBuilder::queueFunctionExit(Node *exitNode)
{
    m_exitControls.push_back(exitNode);
    setEnv(nullptr);
}

Node *GraphBuilder::mergeIntoSuccessor(int offset)
{
    InterpreterEnvironment *&successorEnvironment = m_envForOffset[offset];

    Node *region = nullptr;
    if (successorEnvironment == nullptr) {
        region = createRegion(1);
        successorEnvironment = env();
    } else {
        // Merge any values which are live coming into the successor.
        region = successorEnvironment->merge(env());
    }
    setEnv(nullptr);
    return region;
}

const GraphBuilder::LabelInfo *GraphBuilder::labelInfoAt(unsigned offset) const
{
    for (const LabelInfo &li : m_labelInfos) {
        if (li.labelOffset == offset)
            return &li;
    }
    return nullptr;
}

const GraphBuilder::LabelInfo *GraphBuilder::isLoopStart(unsigned offset) const
{
    if (auto li = labelInfoAt(offset)) {
        //### in the future, check if this is a loop start, or some other label
        return li;
    }

    return nullptr;
}

void GraphBuilder::handleLoopStart(const LabelInfo &labelInfo)
{
    Q_ASSERT(env() != nullptr);

    // We unconditionally insert a region node with phi nodes here. Now there might already be
    // such a node, (e.g. the region after an if-then-else), but for simplicity we ignore that.
    // A subsequent pass will fold/remove chains of Region nodes.
    //### FIXME: add a DCE pass

    const auto offset = int(labelInfo.labelOffset);
    Node *control = createRegion(1);
    env()->setControlDependency(control);
    Node *effect = createEffectPhi(1, env()->effectDependency(), control);
    env()->setEffectDependency(effect);

    // insert/update phi nodes, but not for the unwind handler:
    for (int i = 0, ei = env()->slotCount() - 1; i != ei; ++i) {
        //### use lifeness info to trim this further!
        if (i == CallData::Accumulator)
            continue; // should never be alive on loop entry
        env()->bindNodeToSlot(createPhi(1, env()->slot(i), control), i);
    }

    m_envForOffset.insert(offset, env()->copy());
}

void GraphBuilder::startUnwinding()
{
    if (int target = env()->unwindHandlerOffset()) {
        mergeIntoSuccessor(target);
    } else {
        bindAcc(graph()->undefinedNode());
        generate_Ret();
    }
}

void GraphBuilder::generate_Ret()
{
    Node* control = createNode(opBuilder()->get<Meta::Return>(), env()->accumulator());
    queueFunctionExit(control);
}

void GraphBuilder::generate_Debug() { Q_UNREACHABLE(); }

void GraphBuilder::generate_LoadConst(int index)
{
    auto func = function()->v4Function();
    Value v = func->compilationUnit->constants[index];
    bindAcc(createNode(opBuilder()->getConstant(v)));
}

void GraphBuilder::generate_LoadZero()
{
    bindAcc(createConstant(0));
}

void GraphBuilder::generate_LoadTrue()
{
    bindAcc(m_graph->trueConstant());
}

void GraphBuilder::generate_LoadFalse()
{
    bindAcc(m_graph->falseConstant());
}

void GraphBuilder::generate_LoadNull()
{
    bindAcc(m_graph->nullNode());
}

void GraphBuilder::generate_LoadUndefined()
{
    bindAcc(m_graph->undefinedNode());
}

void GraphBuilder::generate_LoadInt(int value)
{
    bindAcc(m_graph->createNode(opBuilder()->getConstant(Primitive::fromInt32(value))));
}

void GraphBuilder::generate_MoveConst(int constIndex, int destTemp)
{
    auto func = function()->v4Function();
    Value v = func->compilationUnit->constants[constIndex];
    env()->bindNodeToSlot(createNode(opBuilder()->getConstant(v)), destTemp);
}

void GraphBuilder::generate_LoadReg(int reg)
{
    bindAcc(env()->slot(reg));
}

void GraphBuilder::generate_StoreReg(int reg)
{
    Node *n = env()->accumulator();
    if (reg == CallData::This)
        n = createNode(opBuilder()->get<Meta::StoreThis>(), n);
    env()->bindNodeToSlot(n, reg);
}

void GraphBuilder::generate_MoveReg(int srcReg, int destReg)
{
    env()->bindNodeToSlot(env()->slot(srcReg), destReg);
}

void GraphBuilder::generate_LoadImport(int index)
{
    auto func = function()->v4Function();
    Value v = *func->compilationUnit->imports[index];
    bindAcc(createNode(opBuilder()->getConstant(v)));
}

void GraphBuilder::generate_LoadLocal(int index, int /*traceSlot*/)
{
    bindAcc(createNode(opBuilder()->get<Meta::ScopedLoad>(),
                       createConstant(0),
                       createConstant(index)));
}

void GraphBuilder::generate_StoreLocal(int index)
{
    createNode(opBuilder()->get<Meta::ScopedStore>(),
               createConstant(0),
               createConstant(index),
               env()->accumulator());
}

void GraphBuilder::generate_LoadScopedLocal(int scope, int index, int /*traceSlot*/)
{
    bindAcc(createNode(opBuilder()->get<Meta::ScopedLoad>(),
                       createConstant(scope),
                       createConstant(index)));
}

void GraphBuilder::generate_StoreScopedLocal(int scope, int index)
{
    createNode(opBuilder()->get<Meta::ScopedStore>(),
               createConstant(scope),
               createConstant(index),
               env()->accumulator());
}

void GraphBuilder::generate_LoadRuntimeString(int stringId)
{
    auto func = function()->v4Function();
    Value v = Value::fromHeapObject(func->compilationUnit->runtimeStrings[stringId]);
    bindAcc(createNode(opBuilder()->getConstant(v)));
}

void GraphBuilder::generate_MoveRegExp(int regExpId, int destReg)
{
    env()->bindNodeToSlot(createNode(opBuilder()->get<Meta::LoadRegExp>(),
                                     createConstant(regExpId)), destReg);
}

void GraphBuilder::generate_LoadClosure(int value)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSLoadClosure>(),
                       createConstant(value)));
}

void GraphBuilder::generate_LoadName(int name, int /*traceSlot*/)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSLoadName>(),
                       createConstant(name)));
}

void GraphBuilder::generate_LoadGlobalLookup(int index, int /*traceSlot*/)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSLoadGlobalLookup>(), createConstant(index)));
}

void GraphBuilder::generate_StoreNameSloppy(int name)
{
    createNode(opBuilder()->get<Meta::JSStoreNameSloppy>(), createConstant(name), env()->accumulator());
}

void GraphBuilder::generate_StoreNameStrict(int name)
{
    createNode(opBuilder()->get<Meta::JSStoreNameStrict>(), createConstant(name), env()->accumulator());
}

void GraphBuilder::generate_LoadElement(int base, int /*traceSlot*/)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSLoadElement>(),
                       env()->slot(base),
                       env()->accumulator()));
}

void GraphBuilder::generate_StoreElement(int base, int index, int /*traceSlot*/)
{
    createNode(opBuilder()->get<Meta::JSStoreElement>(),
               env()->slot(base),
               env()->slot(index),
               env()->accumulator());
}

void GraphBuilder::generate_LoadProperty(int name, int /*traceSlot*/)
{
    Node *n = createNode(opBuilder()->get<Meta::JSLoadProperty>(),
                         env()->accumulator(),
                         createConstant(name));
    bindAcc(n);
}

void GraphBuilder::generate_GetLookup(int index, int /*traceSlot*/)
{
    Node *n = createNode(opBuilder()->get<Meta::JSGetLookup>(),
                         env()->accumulator(),
                         createConstant(index));
    bindAcc(n);
}

void GraphBuilder::generate_StoreProperty(int name, int base)
{
    createNode(opBuilder()->get<Meta::JSStoreProperty>(),
               env()->slot(base),
               createConstant(name),
               env()->accumulator());
}

void GraphBuilder::generate_SetLookup(int index, int base)
{

    function()->v4Function()->isStrict()
            ? createNode(opBuilder()->get<Meta::JSSetLookupStrict>(), env()->slot(base),
                         createConstant(index), env()->accumulator())
            : createNode(opBuilder()->get<Meta::JSSetLookupSloppy>(), env()->slot(base),
                         createConstant(index), env()->accumulator());
}

void GraphBuilder::generate_LoadSuperProperty(int property)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSLoadSuperProperty>(),
                       env()->slot(property)));
}

void GraphBuilder::generate_StoreSuperProperty(int property)
{
    createNode(opBuilder()->get<Meta::JSStoreSuperProperty>(),
               createConstant(property),
               env()->accumulator());
}

void GraphBuilder::generate_StoreScopeObjectProperty(int base, int propertyIndex)
{
    createNode(opBuilder()->get<Meta::QMLStoreScopeObjectProperty>(),
               env()->slot(base),
               createConstant(propertyIndex),
               env()->accumulator());
}

void GraphBuilder::generate_StoreContextObjectProperty(int base, int propertyIndex)
{
    createNode(opBuilder()->get<Meta::QMLStoreContextObjectProperty>(),
               env()->slot(base),
               createConstant(propertyIndex),
               env()->accumulator());
}

void GraphBuilder::generate_LoadScopeObjectProperty(int propertyIndex, int base,
                                                    int captureRequired)
{
    bindAcc(createNode(opBuilder()->get<Meta::QMLLoadScopeObjectProperty>(),
                       env()->slot(base),
                       createConstant(propertyIndex),
                       createConstant(captureRequired)));
}

void GraphBuilder::generate_LoadContextObjectProperty(int propertyIndex, int base,
                                                      int captureRequired)
{
    bindAcc(createNode(opBuilder()->get<Meta::QMLLoadContextObjectProperty>(),
                       env()->slot(base),
                       createConstant(propertyIndex),
                       createConstant(captureRequired)));
}

void GraphBuilder::generate_LoadIdObject(int index, int base)
{
    bindAcc(createNode(opBuilder()->get<Meta::QMLLoadIdObject>(),
                       env()->slot(base),
                       createConstant(index)));
}

void GraphBuilder::generate_Yield() { Q_UNREACHABLE(); }
void GraphBuilder::generate_YieldStar() { Q_UNREACHABLE(); }
void GraphBuilder::generate_Resume(int /*offset*/) { Q_UNREACHABLE(); }

void GraphBuilder::finalizeCall(Operation::Kind kind, VarArgNodes &args, int argc, int argv)
{
    populate(args, argc, argv);
    bindAcc(createAndLinkNode(opBuilder()->getJSVarArgsCall(kind, uint16_t(args.size())),
                              args.data(), size_t(args.size())));
}

void GraphBuilder::generate_CallValue(int name, int argc, int argv, int /*traceSlot*/)
{
    VarArgNodes args;
    args.append(env()->slot(name));
    finalizeCall(Meta::JSCallValue, args, argc, argv);
}

void GraphBuilder::generate_CallWithReceiver(int name, int thisObject, int argc, int argv,
                                             int /*traceSlot*/)
{
    VarArgNodes args;
    args.append(env()->slot(name));
    args.append(env()->slot(thisObject));
    finalizeCall(Meta::JSCallWithReceiver, args, argc, argv);
}

void GraphBuilder::generate_CallProperty(int name, int base, int argc, int argv, int /*traceSlot*/)
{
    VarArgNodes args;
    args.append(env()->slot(base));
    args.append(createConstant(name));
    finalizeCall(Meta::JSCallProperty, args, argc, argv);
}

void GraphBuilder::generate_CallPropertyLookup(int lookupIndex, int base, int argc, int argv,
                                               int /*traceSlot*/)
{
    VarArgNodes args;
    args.append(env()->slot(base));
    args.append(createConstant(lookupIndex));
    finalizeCall(Meta::JSCallLookup, args, argc, argv);
}

void GraphBuilder::generate_CallElement(int base, int index, int argc, int argv, int /*traceSlot*/)
{
    VarArgNodes args;
    args.append(env()->slot(base));
    args.append(env()->slot(index));
    finalizeCall(Meta::JSCallElement, args, argc, argv);
}

void GraphBuilder::generate_CallName(int name, int argc, int argv, int /*traceSlot*/)
{
    VarArgNodes args;
    args.append(createConstant(name));
    finalizeCall(Meta::JSCallName, args, argc, argv);
}

void GraphBuilder::generate_CallPossiblyDirectEval(int argc, int argv, int /*traceSlot*/)
{
    VarArgNodes args;
    finalizeCall(Meta::JSCallPossiblyDirectEval, args, argc, argv);
}

void GraphBuilder::generate_CallGlobalLookup(int index, int argc, int argv, int /*traceSlot*/)
{
    VarArgNodes args;
    args.append(createConstant(index));
    finalizeCall(Meta::JSCallGlobalLookup, args, argc, argv);
}

void GraphBuilder::generate_CallScopeObjectProperty(int propIdx, int base, int argc, int argv,
                                                    int /*traceSlot*/)
{
    VarArgNodes args;
    args.append(env()->slot(base));
    args.append(createConstant(propIdx));
    finalizeCall(Meta::QMLCallScopeObjectProperty, args, argc, argv);
}

void GraphBuilder::generate_CallContextObjectProperty(int propIdx, int base, int argc, int argv,
                                                      int /*traceSlot*/)
{
    VarArgNodes args;
    args.append(env()->slot(base));
    args.append(createConstant(propIdx));
    finalizeCall(Meta::QMLCallContextObjectProperty, args, argc, argv);
}

void GraphBuilder::generate_SetUnwindHandler(int offset)
{
    m_currentUnwindHandlerOffset = offset ? absoluteOffset(offset) : 0;
    env()->setUnwindHandlerOffset(m_currentUnwindHandlerOffset);
}

void GraphBuilder::generate_UnwindDispatch()
{
    auto e = createNode(opBuilder()->get<Meta::HasException>(), graph()->engineNode());
    createNode(opBuilder()->get<Meta::Branch>(), e);
    {
        InterpreterSubEnvironment subEnvironment(this);
        createIfTrue();
        startUnwinding();
    }

    createIfFalse();

    const auto unwindHandlerOffset = env()->unwindHandlerOffset();
    const auto fallthroughSuccessor = nextInstructionOffset();
    auto nContinuations = m_func->unwindLabelOffsets().size() + 1;
    if (unwindHandlerOffset)
        ++nContinuations;
    Q_ASSERT(nContinuations <= std::numeric_limits<unsigned>::max());
    createNode(opBuilder()->getUnwindDispatch(unsigned(nContinuations), unwindHandlerOffset,
                                              fallthroughSuccessor));

    {
        InterpreterSubEnvironment fallthroughEnv(this);
        mergeIntoSuccessor(fallthroughSuccessor);
    }

    if (unwindHandlerOffset) {
        InterpreterSubEnvironment unwindHandlerEnv(this);
        createHandleUnwind(unwindHandlerOffset);
        mergeIntoSuccessor(unwindHandlerOffset);
    }

    for (int unwindLabelOffset : m_func->unwindLabelOffsets()) {
        if (unwindLabelOffset <= currentInstructionOffset())
            continue;
        InterpreterSubEnvironment unwindLabelEnv(this);
        createHandleUnwind(unwindLabelOffset);
        mergeIntoSuccessor(unwindLabelOffset);
    }

    setEnv(nullptr);
}

void GraphBuilder::generate_UnwindToLabel(int level, int offset)
{
    //### For de-optimization, the relative offset probably also needs to be stored
    int unwinder = absoluteOffset(offset);
    createNode(opBuilder()->get<Meta::UnwindToLabel>(),
               createConstant(level),
               createConstant(unwinder));
    m_func->addUnwindLabelOffset(unwinder);
    startUnwinding();
}

void GraphBuilder::generate_DeadTemporalZoneCheck(int name)
{
    Node *check = createNode(opBuilder()->get<Meta::IsEmpty>(), env()->accumulator());
    createNode(opBuilder()->get<Meta::Branch>(), check);

    { //### it's probably better to handle this by de-optimizing
        InterpreterSubEnvironment subEnvironment(this);
        createIfTrue();
        createNode(opBuilder()->get<Meta::ThrowReferenceError>(),
                   createConstant(name));
        startUnwinding();
    }

    createIfFalse();
}

void GraphBuilder::generate_ThrowException()
{
    createNode(opBuilder()->get<Meta::Throw>(), env()->accumulator());
    startUnwinding();
}

void GraphBuilder::generate_GetException()
{
    bindAcc(createNode(opBuilder()->get<Meta::GetException>()));
}

void GraphBuilder::generate_SetException()
{
    createNode(opBuilder()->get<Meta::SetException>(),
               env()->accumulator());
}

void GraphBuilder::generate_CreateCallContext()
{
    createNode(opBuilder()->get<Meta::JSCreateCallContext>());
}

void GraphBuilder::generate_PushCatchContext(int index, int name)
{
    createNode(opBuilder()->get<Meta::JSCreateCatchContext>(),
               createConstant(index),
               createConstant(name));
}

void GraphBuilder::generate_PushWithContext()
{
    bindAcc(createNode(opBuilder()->get<Meta::JSCreateWithContext>(),
                       env()->accumulator()));
}

void GraphBuilder::generate_PushBlockContext(int index)
{
    createNode(opBuilder()->get<Meta::JSCreateBlockContext>(),
               createConstant(index));
}

void GraphBuilder::generate_CloneBlockContext()
{
    createNode(opBuilder()->get<Meta::JSCloneBlockContext>());
}

void GraphBuilder::generate_PushScriptContext(int index)
{
    createNode(opBuilder()->get<Meta::JSCreateScriptContext>(), createConstant(index));
}

void GraphBuilder::generate_PopScriptContext()
{
    createNode(opBuilder()->get<Meta::JSPopScriptContext>());
}

void GraphBuilder::generate_PopContext()
{
    createNode(opBuilder()->get<Meta::PopContext>());
}

void GraphBuilder::generate_GetIterator(int iterator)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSGetIterator>(),
                       env()->accumulator(),
                       createConstant(iterator)));
}

void GraphBuilder::generate_IteratorNextAndFriends_TrailingStuff(Node *iterationNode,
                                                                 int resultSlot)
{
    // See generate_IteratorNext for why this method exists.

    // check that no-one messed around with the operation and made it throwing
    Q_ASSERT(iterationNode->operation()->controlOutputCount() == 1);

    // check that it's in the effect chain, because HasException relies on that
    Q_ASSERT(iterationNode->operation()->effectOutputCount() == 1);

    env()->bindNodeToSlot(createNode(opBuilder()->get<Meta::SelectOutput>(),
                                     iterationNode,
                                     createConstant(1),
                                     graph()->undefinedNode()),
                          resultSlot);
    // Note: the following will NOT set the accumulator, because it contains the return value of
    // the runtime call!
    Node *ehCheck = createNode(opBuilder()->get<Meta::HasException>(), graph()->engineNode());
    createNode(opBuilder()->get<Meta::Branch>(), ehCheck);

    { // EH path:
        InterpreterSubEnvironment subEnvironment(this);
        createIfTrue();
        if (auto ehOffset = env()->unwindHandlerOffset()) {
            // Ok, there is an exception handler, so go there:
            mergeIntoSuccessor(ehOffset);
        } else {
            // No Exception Handler, so keep the exception set in the engine, and leave the function
            // a.s.a.p.:
            bindAcc(graph()->undefinedNode());
            generate_Ret();
        }
    }

    // Normal control flow:
    createIfFalse();
}

void GraphBuilder::generate_IteratorNext(int value, int done)
{
    // The way we model exceptions in the graph is that a runtime function will either succeed and
    // return a value, or it fails and throws an exception. If it throws, the return value is not
    // used because the method did not complete normally, and therefore it might be tainted.
    //
    // This is a problem for (and only for) IteratorNext and IteratorNextForYieldStart.
    //
    // What would happen in the normal case, is that the return value (done) is not used/assigned
    // when IteratorNext throws, because the exception handling path is chosen. However, the
    // interpreter *does* assign it, and will only check for an exception *after* that assignment.
    //
    // So, in order to work around this odd-duck behavior, we mark the operation as NoThrow,
    // override the runtime method and flag it to not throw, and insert extra exception check nodes
    // after the SelectOutput that follows the IteratorNext(ForYieldStar).
    //
    // Also note that the IteratorNext and IteratorNextForYieldStar are the only operations that
    // have an inout parameter, and thus require a SelectOutput node to retrieve this.

    Node *n = createNode(opBuilder()->get<Meta::JSIteratorNext>(),
                         env()->accumulator(),
                         graph()->undefinedNode());
    bindAcc(n);
    env()->bindNodeToSlot(n, done);
    generate_IteratorNextAndFriends_TrailingStuff(n, value);
}

void GraphBuilder::generate_IteratorNextForYieldStar(int iterator, int object)
{
    // Please, PLEASE read the comment in generate_IteratorNext.
    Node *n = createNode(opBuilder()->get<Meta::JSIteratorNextForYieldStar>(),
                         env()->accumulator(),
                         env()->slot(iterator),
                         graph()->undefinedNode());
    // Note: the following is a tiny bit different from what generate_IteratorNext does.
    bindAcc(n);
    generate_IteratorNextAndFriends_TrailingStuff(n, object);
}

void GraphBuilder::generate_IteratorClose(int done)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSIteratorClose>(),
                       env()->accumulator(),
                       env()->slot(done)));
}

void GraphBuilder::generate_DestructureRestElement()
{
    bindAcc(createNode(opBuilder()->get<Meta::JSDestructureRestElement>(),
                       env()->accumulator()));
}

void GraphBuilder::generate_DeleteProperty(int base, int index)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSDeleteProperty>(),
                       env()->slot(base),
                       env()->slot(index)));
}

void GraphBuilder::generate_DeleteName(int name)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSDeleteName>(),
                       createConstant(name)));
}

void GraphBuilder::generate_TypeofName(int name)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSTypeofName>(),
                       createConstant(name)));
}

void GraphBuilder::generate_TypeofValue()
{
    bindAcc(createNode(opBuilder()->get<Meta::JSTypeofValue>(), env()->accumulator()));
}

void GraphBuilder::generate_DeclareVar(int varName, int isDeletable)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSDeclareVar>(),
                       createConstant(isDeletable),
                       createConstant(varName)));
}

void GraphBuilder::generate_DefineArray(int argc, int argv)
{
    VarArgNodes args;
    finalizeCall(Meta::JSDefineArray, args, argc, argv);
}

void GraphBuilder::generate_DefineObjectLiteral(int internalClassId, int argc, int argv)
{
    VarArgNodes args;
    args.append(createConstant(internalClassId));
    finalizeCall(Meta::JSDefineObjectLiteral, args, argc, argv);
}

void GraphBuilder::generate_CreateClass(int classIndex, int heritage, int computedNames)
{
    int argc = 0;
    int argv = computedNames;

    const QV4::CompiledData::Class *cls = function()->v4Function()->compilationUnit->unitData()
                                                  ->classAt(classIndex);
    const CompiledData::Method *methods = cls->methodTable();
    for (uint i = 0; i < cls->nStaticMethods + cls->nMethods; ++i) {
        if (methods[i].name == std::numeric_limits<unsigned>::max())
            ++argc;
    }

    VarArgNodes args;
    args.append(createConstant(classIndex));
    args.append(env()->slot(heritage));
    finalizeCall(Meta::JSCreateClass, args, argc, argv);
}

void GraphBuilder::generate_CreateMappedArgumentsObject()
{
    bindAcc(createNode(opBuilder()->get<Meta::JSCreateMappedArgumentsObject>()));
}

void GraphBuilder::generate_CreateUnmappedArgumentsObject()
{
    bindAcc(createNode(opBuilder()->get<Meta::JSCreateUnmappedArgumentsObject>()));
}

void GraphBuilder::generate_CreateRestParameter(int argIndex)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSCreateRestParameter>(),
                       createConstant(argIndex)));
}

void GraphBuilder::generate_ConvertThisToObject()
{
    Node* control = createNode(opBuilder()->get<Meta::JSThisToObject>(),
                               env()->slot(CallData::This));
    env()->bindNodeToSlot(control, CallData::This);
}

void GraphBuilder::generate_LoadSuperConstructor()
{
    bindAcc(createNode(opBuilder()->get<Meta::JSLoadSuperConstructor>(),
                       env()->slot(CallData::Function)));
}

void GraphBuilder::generate_ToObject()
{
    bindAcc(createNode(opBuilder()->get<Meta::ToObject>(),
                       env()->accumulator()));
}

void GraphBuilder::generate_CallWithSpread(int func, int thisObject, int argc, int argv,
                                           int /*traceSlot*/)
{
    VarArgNodes args;
    args.append(env()->slot(func));
    args.append(env()->slot(thisObject));
    finalizeCall(Meta::JSCallWithSpread, args, argc, argv);
}

void GraphBuilder::generate_TailCall(int func, int thisObject, int argc, int argv)
{
    VarArgNodes args;
    args.append(env()->slot(func));
    args.append(env()->slot(thisObject));
    populate(args, argc, argv);
    Node *n = createAndLinkNode(opBuilder()->getJSTailCall(uint16_t(args.size())), args.data(),
                                size_t(args.size()));
    queueFunctionExit(n);
}

void GraphBuilder::generate_Construct(int func, int argc, int argv)
{
    VarArgNodes args;
    args.append(env()->slot(func));
    args.append(env()->accumulator());
    finalizeCall(Meta::JSConstruct, args, argc, argv);
}

void GraphBuilder::generate_ConstructWithSpread(int func, int argc, int argv)
{
    VarArgNodes args;
    args.append(env()->slot(func));
    args.append(env()->accumulator());
    finalizeCall(Meta::JSConstructWithSpread, args, argc, argv);
}

void GraphBuilder::generate_Jump(int offset)
{
    auto jumpTarget = absoluteOffset(offset);
    mergeIntoSuccessor(jumpTarget);
}

void GraphBuilder::generate_JumpTrue(int /*traceSlot*/, int offset)
{
    createNode(opBuilder()->get<Meta::Branch>(), createToBoolean(env()->accumulator()));

    {
        InterpreterSubEnvironment subEnvironment(this);
        auto jumpTarget = absoluteOffset(offset);
        createIfTrue();
        mergeIntoSuccessor(jumpTarget);
    }

    createIfFalse();
}

void GraphBuilder::generate_JumpFalse(int traceSlot, int offset)
{
    generate_JumpFalse(env()->accumulator(), traceSlot, offset);
}

void GraphBuilder::generate_JumpFalse(Node *condition, int /*traceSlot*/, int offset)
{
    createNode(opBuilder()->get<Meta::Branch>(), createToBoolean(condition));

    {
        InterpreterSubEnvironment subEnvironment(this);
        auto jumpTarget = absoluteOffset(offset);
        createIfFalse();
        mergeIntoSuccessor(jumpTarget);
    }

    createIfTrue();
}

void GraphBuilder::generate_JumpNoException(int offset)
{
    auto e = createNode(opBuilder()->get<Meta::HasException>(), graph()->engineNode());
    createNode(opBuilder()->get<Meta::Branch>(), e);

    {
        InterpreterSubEnvironment subEnvironment(this);
        auto jumpTarget = absoluteOffset(offset);
        createIfFalse();
        mergeIntoSuccessor(jumpTarget);
    }

    createIfTrue();
}

void GraphBuilder::generate_JumpNotUndefined(int offset)
{
    Node *condition = createNode(opBuilder()->get<Meta::JSStrictEqual>(),
                                 env()->accumulator(),
                                 graph()->undefinedNode());
    generate_JumpFalse(condition, NoTraceSlot, offset);
}

void GraphBuilder::generate_CmpEqNull()
{
    bindAcc(createNode(opBuilder()->get<Meta::JSEqual>(),
                       env()->accumulator(),
                       graph()->nullNode()));
}

void GraphBuilder::generate_CmpNeNull()
{
    generate_CmpEqNull();
    bindAcc(createNode(opBuilder()->get<Meta::BooleanNot>(),
                       env()->accumulator()));
}

void GraphBuilder::generate_CmpEqInt(int lhs)
{
    auto left = createConstant(lhs);
    Node* control = createNode(opBuilder()->get<Meta::JSEqual>(),
                               left,
                               env()->accumulator());
    bindAcc(control);
}

void GraphBuilder::generate_CmpNeInt(int lhs)
{
    generate_CmpEqInt(lhs);
    bindAcc(createNode(opBuilder()->get<Meta::BooleanNot>(),
                       env()->accumulator()));
}

void GraphBuilder::generate_CmpEq(int lhs)
{
    Node* control = createNode(opBuilder()->get<Meta::JSEqual>(),
                               env()->slot(lhs),
                               env()->accumulator());
    bindAcc(control);
}

void GraphBuilder::generate_CmpNe(int lhs)
{
    generate_CmpEq(lhs);
    bindAcc(createNode(opBuilder()->get<Meta::BooleanNot>(),
                       env()->accumulator()));
}

void GraphBuilder::generate_CmpGt(int lhs)
{
    Node* control = createNode(opBuilder()->get<Meta::JSGreaterThan>(),
                               env()->slot(lhs),
                               env()->accumulator());
    bindAcc(control);
}

void GraphBuilder::generate_CmpGe(int lhs)
{
    Node* control = createNode(opBuilder()->get<Meta::JSGreaterEqual>(),
                               env()->slot(lhs),
                               env()->accumulator());
    bindAcc(control);
}

void GraphBuilder::generate_CmpLt(int lhs)
{
    Node* control = createNode(opBuilder()->get<Meta::JSLessThan>(),
                               env()->slot(lhs),
                               env()->accumulator());
    bindAcc(control);
}

void GraphBuilder::generate_CmpLe(int lhs)
{
    Node* control = createNode(opBuilder()->get<Meta::JSLessEqual>(),
                               env()->slot(lhs),
                               env()->accumulator());
    bindAcc(control);
}

void GraphBuilder::generate_CmpStrictEqual(int lhs)
{
    Node* control = createNode(opBuilder()->get<Meta::JSStrictEqual>(),
                               env()->slot(lhs),
                               env()->accumulator());
    bindAcc(control);
}

void GraphBuilder::generate_CmpStrictNotEqual(int lhs)
{
    generate_CmpStrictEqual(lhs);
    bindAcc(createNode(opBuilder()->get<Meta::BooleanNot>(),
                       env()->accumulator()));
}

void GraphBuilder::generate_CmpIn(int lhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSIn>(), env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_CmpInstanceOf(int lhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSInstanceOf>(), env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_UNot()
{
    bindAcc(createNode(opBuilder()->get<Meta::BooleanNot>(),
                       createToBoolean(env()->accumulator())));
}

void GraphBuilder::generate_UPlus()
{
    Node* control = createNode(opBuilder()->get<Meta::JSToNumber>(),
                               env()->accumulator());
    bindAcc(control);
}

void GraphBuilder::generate_UMinus(int /*traceSlot*/)
{
    Node* control = createNode(opBuilder()->get<Meta::JSNegate>(),
                               env()->accumulator());
    bindAcc(control);
}

void GraphBuilder::generate_UCompl()
{
    bindAcc(createNode(opBuilder()->get<Meta::JSBitXor>(),
                       env()->accumulator(),
                       createConstant(-1)));
}

void GraphBuilder::generate_Increment(int /*traceSlot*/)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSAdd>(),
                       env()->accumulator(),
                       createConstant(1)));
}


void GraphBuilder::generate_Decrement(int /*traceSlot*/)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSSubtract>(),
                       env()->accumulator(),
                       createConstant(1)));
}

void GraphBuilder::generate_Add(int lhs, int /*traceSlot*/)
{
    Node* control = createNode(opBuilder()->get<Meta::JSAdd>(),
                               env()->slot(lhs),
                               env()->accumulator());
    bindAcc(control);
}

void GraphBuilder::generate_BitAnd(int lhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSBitAnd>(),
                       env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_BitOr(int lhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSBitOr>(),
                       env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_BitXor(int lhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSBitXor>(),
                       env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_UShr(int lhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSUnsignedShiftRight>(),
                       env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_Shr(int lhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSShiftRight>(),
                       env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_Shl(int lhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSShiftLeft>(),
                       env()->slot(lhs),
                       env()->accumulator()));
}


void GraphBuilder::generate_BitAndConst(int rhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSBitAnd>(),
                       env()->accumulator(),
                       createConstant(rhs)));
}

void GraphBuilder::generate_BitOrConst(int rhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSBitOr>(),
                       env()->accumulator(),
                       createConstant(rhs)));
}

void GraphBuilder::generate_BitXorConst(int rhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSBitXor>(),
                       env()->accumulator(),
                       createConstant(rhs)));
}

void GraphBuilder::generate_UShrConst(int rhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSUnsignedShiftRight>(),
                       env()->accumulator(),
                       createConstant(rhs)));
}

void GraphBuilder::generate_ShrConst(int rhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSShiftRight>(),
                       env()->accumulator(),
                       createConstant(rhs)));
}

void GraphBuilder::generate_ShlConst(int rhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSShiftLeft>(),
                       env()->accumulator(),
                       createConstant(rhs)));
}

void GraphBuilder::generate_Exp(int lhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSExponentiate>(),
                       env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_Mul(int lhs, int /*traceSlot*/)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSMultiply>(),
                       env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_Div(int lhs)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSDivide>(),
                       env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_Mod(int lhs, int /*traceSlot*/)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSModulo>(),
                       env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_Sub(int lhs, int /*traceSlot*/)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSSubtract>(),
                       env()->slot(lhs),
                       env()->accumulator()));
}

void GraphBuilder::generate_LoadQmlContext(int result)
{
    env()->bindNodeToSlot(createNode(opBuilder()->get<Meta::QMLLoadContext>()), result);
}

void GraphBuilder::generate_LoadQmlImportedScripts(int result)
{
    env()->bindNodeToSlot(createNode(opBuilder()->get<Meta::QMLLoadImportedScripts>()),
                          result);
}

void GraphBuilder::generate_InitializeBlockDeadTemporalZone(int firstReg, int count)
{
    for (int reg = firstReg; reg < firstReg + count; ++reg)
        env()->bindNodeToSlot(graph()->emptyNode(), reg);
}

void GraphBuilder::generate_ThrowOnNullOrUndefined()
{
    createNode(opBuilder()->get<Meta::JSThrowOnNullOrUndefined>(),
               env()->accumulator());
}

void GraphBuilder::generate_GetTemplateObject(int index)
{
    bindAcc(createNode(opBuilder()->get<Meta::JSGetTemplateObject>(),
                       createConstant(index)));
}

GraphBuilder::Verdict GraphBuilder::startInstruction(Moth::Instr::Type /*instr*/)
{
    // This handles a couple of cases on how flow control can end up at this instruction.

    const auto off = currentInstructionOffset();
    if (auto newEnv = m_envForOffset[off]) {
        // Ok, there was a jump from before to this point (which registered an environment), so we
        // have two options:
        if (env() != newEnv && env() != nullptr) {
            // There is a current environment different from the environment active when we took the
            // jump. This happens with e.g. an if-then-else:
            //
            //     acc = condition
            //     JumpFalse else-block
            //     ... then block
            //     Jump end-if
            //   else-block:
            //     ... else block
            //   end-if:
            //     .. some instruction   <--- we're here
            //
            // in that case we merge the after-else environment into the after-then environment:
            newEnv->merge(env());
        } else {
            // There is not a current environment. This can happen with e.g. a loop:
            //  loop-start:
            //     acc = condition
            //     JumpFalse loop-end
            //     ... loop body
            //     Jump loop-start
            //  loop-end:
            //     .... some instruction   <--- we're here
            //
            // The last jump of the loop will clear the environment, so at this point we only have
            // the environment registered by the JumpFalse. This is the asy case: no merges, just
            // take the registered environment unchanged.
        }

        // Leave the merged environment as-is, and continue with a copy. We cannot change the
        // registered environment in case this point also happens to be a loop start.
        setEnv(newEnv->copy());
    }

    if (env() == nullptr) {
        // Ok, there is no environment, meaning nobody jumped to this instruction, and the previous
        // instruction doesn't let control flow end up here. So, this is dead code.
        // This can happen for JS like:
        //
        //   if (condition) {
        //     return something
        //   } else {
        //     return somethingElse
        //   }
        //   someCode   <--- we're here
        return SkipInstruction;
    }

    const LabelInfo *info = isLoopStart(off);
    if (info && env()) {
        // Ok, this instruction is the start of a loop, meaning there will be a jump backwards to
        // this point. Make sure there is a Region node with Phi nodes here.
        handleLoopStart(*info);
    }

    return ProcessInstruction;
}

void GraphBuilder::endInstruction(Moth::Instr::Type /*instr*/) {}

} // IR namespace
} // QV4 namespace
QT_END_NAMESPACE
