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

#include "qv4compilercontext_p.h"
#include "qv4compilercontrolflow_p.h"
#include "qv4bytecodegenerator_p.h"

QT_USE_NAMESPACE
using namespace QV4;
using namespace QV4::Compiler;
using namespace QQmlJS::AST;

QT_BEGIN_NAMESPACE

Context *Module::newContext(Node *node, Context *parent, ContextType contextType)
{
    Q_ASSERT(!contextMap.contains(node));

    Context *c = new Context(parent, contextType);
    if (node) {
        SourceLocation loc = node->firstSourceLocation();
        c->line = loc.startLine;
        c->column = loc.startColumn;
    }

    contextMap.insert(node, c);

    if (!parent)
        rootContext = c;
    else {
        parent->nestedContexts.append(c);
        c->isStrict = parent->isStrict;
    }

    return c;
}

bool Context::forceLookupByName()
{
    ControlFlow *flow = controlFlow;
    while (flow) {
        if (flow->needsLookupByName)
            return true;
        flow = flow->parent;
    }
    return false;
}

bool Context::addLocalVar(const QString &name, Context::MemberType type, VariableScope scope, FunctionExpression *function)
{
    if (name.isEmpty())
        return true;

    if (type != FunctionDefinition) {
        if (formals && formals->containsName(name))
            return (scope == QQmlJS::AST::VariableScope::Var);
    }
    MemberMap::iterator it = members.find(name);
    if (it != members.end()) {
        if (scope != QQmlJS::AST::VariableScope::Var || (*it).scope != QQmlJS::AST::VariableScope::Var)
            return false;
        if ((*it).type <= type) {
            (*it).type = type;
            (*it).function = function;
        }
        return true;
    }
    Member m;
    m.type = type;
    m.function = function;
    m.scope = scope;
    members.insert(name, m);
    return true;
}

Context::ResolvedName Context::resolveName(const QString &name)
{
    int scope = 0;
    Context *c = this;

    ResolvedName result;

    while (c->parent) {
        if (c->forceLookupByName())
            return result;

        Context::Member m = c->findMember(name);
        if (m.type != Context::UndefinedMember) {
            result.type = m.canEscape ? ResolvedName::Local : ResolvedName::Stack;
            result.scope = scope;
            result.index = m.index;
            if (c->isStrict && (name == QLatin1String("arguments") || name == QLatin1String("eval")))
                result.isArgOrEval = true;
            return result;
            Q_ASSERT(result.type != ResolvedName::Stack || result.scope == 0);
        }
        const int argIdx = c->findArgument(name);
        if (argIdx != -1) {
            if (c->argumentsCanEscape) {
                result.index = argIdx + c->locals.size();
                result.scope = scope;
                result.type = ResolvedName::Local;
                return result;
            } else {
                Q_ASSERT(scope == 0);
                result.index = argIdx + sizeof(CallData)/sizeof(Value) - 1;
                result.scope = 0;
                result.type = ResolvedName::Stack;
                return result;
            }
        }
        if (!c->isStrict && c->hasDirectEval)
            return result;

        if (c->requiresExecutionContext)
            ++scope;
        c = c->parent;
    }

    // ### can we relax the restrictions here?
    if (c->forceLookupByName() || type == ContextType::Eval || c->type == ContextType::Binding)
        return result;

    result.type = ResolvedName::Global;
    return result;
}

void Context::emitHeaderBytecode(Codegen *codegen)
{
    using Instruction = Moth::Instruction;
    Moth::BytecodeGenerator *bytecodeGenerator = codegen->generator();

    bool allVarsEscape = hasWith || hasTry || hasDirectEval;
    if (requiresExecutionContext ||
        type == ContextType::Binding) { // we don't really need this for bindings, but we do for signal handlers, and we don't know if the code is a signal handler or not.
        Instruction::CreateCallContext createContext;
        bytecodeGenerator->addInstruction(createContext);
    }
    if (usesThis && !isStrict) {
        // make sure we convert this to an object
        Instruction::ConvertThisToObject convert;
        bytecodeGenerator->addInstruction(convert);
    }

    // variables in global code are properties of the global context object, not locals as with other functions.
    if (type == ContextType::Function || type == ContextType::Binding) {
        for (Context::MemberMap::iterator it = members.begin(), end = members.end(); it != end; ++it) {
            const QString &local = it.key();
            if (allVarsEscape)
                it->canEscape = true;
            if (it->canEscape) {
                it->index = locals.size();
                locals.append(local);
                if (it->type == Context::ThisFunctionName) {
                    // move the name from the stack to the call context
                    Instruction::LoadReg load;
                    load.reg = CallData::Function;
                    bytecodeGenerator->addInstruction(load);
                    Instruction::StoreLocal store;
                    store.index = it->index;
                    bytecodeGenerator->addInstruction(store);
                }
            } else {
                if (it->type == Context::ThisFunctionName)
                    it->index = CallData::Function;
                else
                    it->index = bytecodeGenerator->newRegister();
            }
        }
    } else {
        for (Context::MemberMap::const_iterator it = members.constBegin(), cend = members.constEnd(); it != cend; ++it) {
            const QString &local = it.key();

            Instruction::DeclareVar declareVar;
            declareVar.isDeletable = false;
            declareVar.varName = codegen->registerString(local);
            bytecodeGenerator->addInstruction(declareVar);
        }
    }

    if (usesArgumentsObject == Context::ArgumentsObjectUsed) {
        if (isStrict || (formals && !formals->isSimpleParameterList())) {
            Instruction::CreateUnmappedArgumentsObject setup;
            bytecodeGenerator->addInstruction(setup);
        } else {
            Instruction::CreateMappedArgumentsObject setup;
            bytecodeGenerator->addInstruction(setup);
        }
        codegen->referenceForName(QStringLiteral("arguments"), false).storeConsumeAccumulator();
    }
}

QT_END_NAMESPACE
