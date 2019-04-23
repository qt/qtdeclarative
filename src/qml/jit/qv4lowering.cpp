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

#include <QLoggingCategory>

#include "qv4lowering_p.h"
#include "qv4graph_p.h"

QT_REQUIRE_CONFIG(qml_tracing);

QT_BEGIN_NAMESPACE
namespace QV4 {
namespace IR {

Q_LOGGING_CATEGORY(lcLower, "qt.v4.ir.lowering")

GenericLowering::GenericLowering(Function &f)
    : m_function(f)
{}

void GenericLowering::lower()
{
    NodeWorkList worklist(graph());
    // The order doesn't really matter for generic lowering, as long as it's done in 1 pass, and
    // have any clean-up done afterwards.
    worklist.enqueueAllInputs(graph()->endNode());

    while (Node *n = worklist.dequeueNextNodeForVisiting()) {
        worklist.enqueueAllInputs(n);

        if (!CallPayload::isRuntimeCall(n->opcode()))
            continue;

        if (CallPayload::isVarArgsCall(n->opcode()))
            replaceWithVarArgsCall(n);
        else
            replaceWithCall(n);
    }
}

void GenericLowering::replaceWithCall(Node *n)
{
    auto newOp = opBuilder()->getCall(n->opcode());

    QVarLengthArray<Node *, 32> args;
    if (CallPayload::takesEngineAsArg(n->opcode(), 0))
        args.append(graph()->engineNode());
    if (CallPayload::takesFunctionAsArg(n->opcode(), args.size()))
        args.append(graph()->functionNode());
    if (CallPayload::takesFrameAsArg(n->opcode(), args.size()))
        args.append(graph()->cppFrameNode());
    const int extraLeadingArguments = args.size();

    for (unsigned arg = 0, earg = n->inputCount(); arg != earg; ++arg) {
        Node *input = n->input(arg);
        if (input->opcode() == Meta::FrameState)
            continue;

        if (arg >= n->operation()->valueInputCount()) {
            // effect or control input
            args.append(input);
            continue;
        }

        if (CallPayload::needsStorageOnJSStack(n->opcode(), args.size(), input->operation(),
                                               function().nodeInfo(input)->type()))
            input = graph()->createNode(opBuilder()->get<Meta::Alloca>(), input);

        args.append(input);
    }

    Node *newCall = graph()->createNode(newOp, args.data(), args.size());

    qCDebug(lcLower) << "replacing node" << n->id() << n->operation()->debugString()
                     << "with node" << newCall->id() << newOp->debugString();
    qCDebug(lcLower) << "... old node #inputs:" << n->inputCount();
    qCDebug(lcLower) << "... old node #uses:" << n->useCount();

    function().nodeInfo(newCall)->setType(CallPayload::returnType(n->opcode()));
    n->replaceAllUsesWith(newCall);
    n->kill();

    qCDebug(lcLower) << "... new node #inputs:" << newCall->inputCount();
    qCDebug(lcLower) << "... new node #uses:" << newCall->useCount();

    for (Node *use : newCall->uses()) {
        // fix-up indices for SelectOutput:
        if (use->opcode() == Meta::SelectOutput) {
            const int oldIndex = ConstantPayload::get(*use->input(1)->operation())->value().int_32();
            const int newIndex = oldIndex + extraLeadingArguments;
            use->replaceInput(1, graph()->createConstantIntNode(newIndex));
            use->replaceInput(2, newCall->input(newIndex));
            break;
        }
    }
}

void GenericLowering::replaceWithVarArgsCall(Node *n)
{
    const bool isTailCall = n->opcode() == Meta::JSTailCall;
    Operation *newOp = isTailCall ? opBuilder()->getTailCall()
                                  : opBuilder()->getCall(n->opcode());

    //### optimize this for 0 and 1 argument: we don't need to create a VarArgs array for these cases

    const unsigned varArgsStart = CallPayload::varArgsStart(n->opcode()) - 1; // subtract 1 because the runtime calls all take the engine argument as arg0, which isn't in the graph before lowering.
    Node *vaAlloc = graph()->createNode(
                opBuilder()->get<Meta::VAAlloc>(),
                graph()->createConstantIntNode(n->operation()->valueInputCount() - varArgsStart),
                n->effectInput());
    QVarLengthArray<Node *, 32> vaSealIn;
    vaSealIn.append(vaAlloc);
    for (unsigned i = varArgsStart, ei = n->operation()->valueInputCount(); i != ei; ++i) {
        vaSealIn.append(graph()->createNode(opBuilder()->get<Meta::VAStore>(), vaAlloc,
                                            graph()->createConstantIntNode(vaSealIn.size() - 1),
                                            n->input(i)));
    }
    vaSealIn.append(vaAlloc);
    Node *vaSeal = graph()->createNode(opBuilder()->getVASeal(vaSealIn.size() - 2),
                                 vaSealIn.data(),
                                 vaSealIn.size());
    QVarLengthArray<Node *, 8> callArgs;
    if (isTailCall)
        callArgs.append(graph()->cppFrameNode());
    callArgs.append(graph()->engineNode());
    for (unsigned i = 0; i != varArgsStart; ++i) {
        Node *input = n->input(i);
        if (CallPayload::needsStorageOnJSStack(n->opcode(), callArgs.size(), input->operation(),
                                               function().nodeInfo(input)->type()))
            input = graph()->createNode(opBuilder()->get<Meta::Alloca>(), input);
        callArgs.append(input);
    }
    callArgs.append(vaSeal); // args
    if (n->opcode() != Meta::JSCreateClass) // JSCreateClass is the odd duck
        callArgs.append(graph()->createConstantIntNode(vaSealIn.size() - 2)); // argc
    callArgs.append(vaSeal); // effect
    callArgs.append(n->controlInput(0)); // control flow
    Node *newCall = graph()->createNode(newOp, callArgs.data(), unsigned(callArgs.size()));

    qCDebug(lcLower) << "replacing node" << n->id() << n->operation()->debugString()
                     << "with node" << newCall->id() << newOp->debugString();
    qCDebug(lcLower) << "... old node #inputs:" << n->inputCount();
    qCDebug(lcLower) << "... old node #uses:" << n->useCount();

    n->replaceAllUsesWith(newCall);
    n->kill();

    qCDebug(lcLower) << "... new node #inputs:" << newCall->inputCount();
    qCDebug(lcLower) << "... new node #uses:" << newCall->useCount();
}

bool GenericLowering::allUsesAsUnboxedBool(Node *n)
{
    for (Node *use : n->uses()) {
        if (use->operation()->kind() != Meta::Branch)
            return false;
    }

    return true;
}

} // IR namespace
} // QV4 namespace
QT_END_NAMESPACE
