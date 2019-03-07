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

#ifndef QV4GRAPHBUILDER_P_H
#define QV4GRAPHBUILDER_P_H

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

#include <private/qv4global_p.h>
#include <private/qv4bytecodehandler_p.h>
#include <private/qv4ir_p.h>
#include "qv4graph_p.h"

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace IR {

// The graph builder walks the byte-code, and produces a graph. The graph is a digraph, where the
// nodes have operations, and the edges are dependencies (or inputs).
class GraphBuilder: protected Moth::ByteCodeHandler
{
    Q_DISABLE_COPY_MOVE(GraphBuilder)

    enum { NoTraceSlot = -1 };

    struct LabelInfo { //### extend this to also capture the amount of slots that are live
        LabelInfo() = default;
        LabelInfo(unsigned label) : labelOffset(label) {}
        unsigned labelOffset = 0;
    };

public:
    static void buildGraph(IR::Function *function);

    class InterpreterEnvironment;

    void setEnv(InterpreterEnvironment *newEnv)
    { m_currentEnv = newEnv; }

    InterpreterEnvironment *env() const
    { return m_currentEnv; }

private:
    GraphBuilder(IR::Function *function);
    ~GraphBuilder() override = default;

    void startGraph();
    void endGraph();

    Node *bindAcc(Node *n);
    Node *createAndLinkNode(Operation *op, Node *operands[], size_t opCount, bool incomplete = false);
    Node *createNode(Operation *op, bool incomplete = false);
    Node *createNode(Operation *op, Node *n1);
    Node *createNode(Operation *op, Node *n1, Node *n2);
    Node *createNode(Operation *op, Node *n1, Node *n2, Node *n3);
    Node *createNode(Operation *op, Node *n1, Node *n2, Node *n3, Node *n4);
    Node *createRegion(unsigned nControlInputs);
    Node *createIfTrue();
    Node *createIfFalse();
    Node *createConstant(int v);
    Node *createPhi(unsigned nInputs, Node *input, Node *control);
    Node *createEffectPhi(unsigned nInputs, Node *input, Node *control);
    Node *createHandleUnwind(int offset);
    Node *mergeControl(Node *c1, Node *c2);
    Node *mergeEffect(Node *e1, Node *e2, Node *control);
    Node *mergeValue(Node *v1, Node *v2, Node *control);

    Node *createToBoolean(Node *input);

    using VarArgNodes = QVarLengthArray<Node *, 32>;
    void populate(VarArgNodes &args, int argc, int argv);

    void queueFunctionExit(Node *exitNode);

    Function *function() const
    { return m_func; }

    Graph *graph()
    { return m_graph; }

    Node *mergeIntoSuccessor(int offset);

    OperationBuilder *opBuilder() const
    { return m_graph->opBuilder(); }

    int absoluteOffset(int offset) const
    { return offset + nextInstructionOffset(); }

    const LabelInfo *labelInfoAt(unsigned offset) const;
    const LabelInfo *isLoopStart(unsigned offset) const;
    void handleLoopStart(const LabelInfo &labelInfo);
    void startUnwinding();

protected: // ByteCodeHandler
    void generate_Ret() override;
    void generate_Debug() override;
    void generate_LoadConst(int index) override;
    void generate_LoadZero() override;
    void generate_LoadTrue() override;
    void generate_LoadFalse() override;
    void generate_LoadNull() override;
    void generate_LoadUndefined() override;
    void generate_LoadInt(int value) override;
    void generate_MoveConst(int constIndex, int destTemp) override;
    void generate_LoadReg(int reg) override;
    void generate_StoreReg(int reg) override;
    void generate_MoveReg(int srcReg, int destReg) override;
    void generate_LoadImport(int index) override;
    void generate_LoadLocal(int index, int traceSlot) override;
    void generate_StoreLocal(int index) override;
    void generate_LoadScopedLocal(int scope, int index, int traceSlot) override;
    void generate_StoreScopedLocal(int scope, int index) override;
    void generate_LoadRuntimeString(int stringId) override;
    void generate_MoveRegExp(int regExpId, int destReg) override;
    void generate_LoadClosure(int value) override;
    void generate_LoadName(int name, int traceSlot) override;
    void generate_LoadGlobalLookup(int index, int traceSlot) override;
    void generate_StoreNameSloppy(int name) override;
    void generate_StoreNameStrict(int name) override;
    void generate_LoadElement(int base, int traceSlot) override;
    void generate_StoreElement(int base, int index, int traceSlot) override;
    void generate_LoadProperty(int name, int traceSlot) override;
    void generate_GetLookup(int index, int traceSlot) override;
    void generate_StoreProperty(int name, int base) override;
    void generate_SetLookup(int index, int base) override;
    void generate_LoadSuperProperty(int property) override;
    void generate_StoreSuperProperty(int property) override;
    void generate_StoreScopeObjectProperty(int base,
                                           int propertyIndex) override;
    void generate_StoreContextObjectProperty(int base,
                                             int propertyIndex) override;
    void generate_LoadScopeObjectProperty(int propertyIndex, int base,
                                          int captureRequired) override;
    void generate_LoadContextObjectProperty(int propertyIndex, int base,
                                            int captureRequired) override;
    void generate_LoadIdObject(int index, int base) override;
    void generate_Yield() override;
    void generate_YieldStar() override;
    void generate_Resume(int offset) override;
    void finalizeCall(Operation::Kind kind, VarArgNodes &args, int argc, int argv);
    void generate_CallValue(int name, int argc, int argv, int traceSlot) override;
    void generate_CallWithReceiver(int name, int thisObject, int argc, int argv,
                                   int traceSlot) override;
    void generate_CallProperty(int name, int base, int argc, int argv, int traceSlot) override;
    void generate_CallPropertyLookup(int lookupIndex, int base, int argc, int argv,
                                     int traceSlot) override;
    void generate_CallElement(int base, int index, int argc, int argv, int traceSlot) override;
    void generate_CallName(int name, int argc, int argv, int traceSlot) override;
    void generate_CallPossiblyDirectEval(int argc, int argv, int traceSlot) override;
    void generate_CallGlobalLookup(int index, int argc, int argv, int traceSlot) override;
    void generate_CallScopeObjectProperty(int propIdx, int base, int argc, int argv,
                                          int traceSlot) override;
    void generate_CallContextObjectProperty(int propIdx, int base, int argc, int argv,
                                            int traceSlot) override;
    void generate_SetUnwindHandler(int offset) override;
    void generate_UnwindDispatch() override;
    void generate_UnwindToLabel(int level, int offset) override;
    void generate_DeadTemporalZoneCheck(int name) override;
    void generate_ThrowException() override;
    void generate_GetException() override;
    void generate_SetException() override;
    void generate_CreateCallContext() override;
    void generate_PushCatchContext(int index, int name) override;
    void generate_PushWithContext() override;
    void generate_PushBlockContext(int index) override;
    void generate_CloneBlockContext() override;
    void generate_PushScriptContext(int index) override;
    void generate_PopScriptContext() override;
    void generate_PopContext() override;
    void generate_GetIterator(int iterator) override;
    void generate_IteratorNextAndFriends_TrailingStuff(Node *iterationNode, int resultSlot);
    void generate_IteratorNext(int value, int done) override;
    void generate_IteratorNextForYieldStar(int iterator, int object) override;
    void generate_IteratorClose(int done) override;
    void generate_DestructureRestElement() override;
    void generate_DeleteProperty(int base, int index) override;
    void generate_DeleteName(int name) override;
    void generate_TypeofName(int name) override;
    void generate_TypeofValue() override;
    void generate_DeclareVar(int varName, int isDeletable) override;
    void generate_DefineArray(int argc, int argv) override;
    void generate_DefineObjectLiteral(int internalClassId, int argc, int argv) override;
    void generate_CreateClass(int classIndex, int heritage, int computedNames) override;
    void generate_CreateMappedArgumentsObject() override;
    void generate_CreateUnmappedArgumentsObject() override;
    void generate_CreateRestParameter(int argIndex) override;
    void generate_ConvertThisToObject() override;
    void generate_LoadSuperConstructor() override;
    void generate_ToObject() override;
    void generate_CallWithSpread(int func, int thisObject, int argc, int argv,
                                 int traceSlot) override;
    void generate_TailCall(int func, int thisObject, int argc, int argv) override;
    void generate_Construct(int func, int argc, int argv) override;
    void generate_ConstructWithSpread(int func, int argc, int argv) override;
    void generate_Jump(int offset) override;
    void generate_JumpTrue(int traceSlot, int offset) override;
    void generate_JumpFalse(int traceSlot, int offset) override;
    void generate_JumpFalse(Node *condition, int traceSlot, int offset);
    void generate_JumpNoException(int offset) override;
    void generate_JumpNotUndefined(int offset) override;
    void generate_CmpEqNull() override;
    void generate_CmpNeNull() override;
    void generate_CmpEqInt(int lhs) override;
    void generate_CmpNeInt(int lhs) override;
    void generate_CmpEq(int lhs) override;
    void generate_CmpNe(int lhs) override;
    void generate_CmpGt(int lhs) override;
    void generate_CmpGe(int lhs) override;
    void generate_CmpLt(int lhs) override;
    void generate_CmpLe(int lhs) override;
    void generate_CmpStrictEqual(int lhs) override;
    void generate_CmpStrictNotEqual(int lhs) override;
    void generate_CmpIn(int lhs) override;
    void generate_CmpInstanceOf(int lhs) override;
    void generate_UNot() override;
    void generate_UPlus(int traceSlot) override;
    void generate_UMinus(int traceSlot) override;
    void generate_UCompl() override;
    void generate_Increment(int traceSlot) override;
    void generate_Decrement(int traceSlot) override;
    void generate_Add(int lhs, int traceSlot) override;
    void generate_BitAnd(int lhs) override;
    void generate_BitOr(int lhs) override;
    void generate_BitXor(int lhs) override;
    void generate_UShr(int lhs) override;
    void generate_Shr(int lhs) override;
    void generate_Shl(int lhs) override;
    void generate_BitAndConst(int rhs) override;
    void generate_BitOrConst(int rhs) override;
    void generate_BitXorConst(int rhs) override;
    void generate_UShrConst(int rhs) override;
    void generate_ShrConst(int rhs) override;
    void generate_ShlConst(int rhs) override;
    void generate_Exp(int lhs) override;
    void generate_Mul(int lhs, int traceSlot) override;
    void generate_Div(int lhs) override;
    void generate_Mod(int lhs, int traceSlot) override;
    void generate_Sub(int lhs, int traceSlot) override;
    void generate_LoadQmlContext(int result) override;
    void generate_LoadQmlImportedScripts(int result) override;
    void generate_InitializeBlockDeadTemporalZone(int firstReg, int count) override;
    void generate_ThrowOnNullOrUndefined() override;
    void generate_GetTemplateObject(int index) override;

    Verdict startInstruction(Moth::Instr::Type instr) override;
    void endInstruction(Moth::Instr::Type instr) override;

private:
    IR::Function *m_func;
    Graph *m_graph;
    InterpreterEnvironment *m_currentEnv;
    std::vector<Node *> m_exitControls;
    QHash<int, InterpreterEnvironment *> m_envForOffset;
    std::vector<LabelInfo> m_labelInfos;
    int m_currentUnwindHandlerOffset = 0;
};

} // namespace IR
} // namespace QV4

QT_END_NAMESPACE

#endif // QV4GRAPHBUILDER_P_H
