// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qv4codegen_p.h"
#include "qv4compilercontext_p.h"
#include "qv4bytecodegenerator_p.h"
#include <QtQml/private/qv4calldata_p.h>

QT_USE_NAMESPACE
using namespace QV4;
using namespace QV4::Compiler;
using namespace QQmlJS::AST;
using namespace QQmlJS;

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

bool Context::Member::requiresTDZCheck(const SourceLocation &accessLocation, bool accessAcrossContextBoundaries) const
{
    if (!isLexicallyScoped())
        return false;

    if (accessAcrossContextBoundaries)
        return true;

    if (!accessLocation.isValid() || !declarationLocation.isValid())
        return true;

    return accessLocation.begin() < declarationLocation.end();
}

bool Context::addLocalVar(
        const QString &name, Context::MemberType type, VariableScope scope,
        FunctionExpression *function, const QQmlJS::SourceLocation &declarationLocation,
        bool isInjected)
{
    // ### can this happen?
    if (name.isEmpty())
        return true;

    if (type != FunctionDefinition) {
        if (formals && formals->containsName(name))
            return (scope == VariableScope::Var);
    }
    if (!isCatchBlock || name != caughtVariable) {
        MemberMap::iterator it = members.find(name);
        if (it != members.end()) {
            if (scope != VariableScope::Var || (*it).scope != VariableScope::Var)
                return false;
            if ((*it).type <= type) {
                (*it).type = type;
                (*it).function = function;
            }
            return true;
        }
    }

    // hoist var declarations to the function level
    if (contextType == ContextType::Block && (scope == VariableScope::Var && type != MemberType::FunctionDefinition))
        return parent->addLocalVar(name, type, scope, function, declarationLocation);

    Member m;
    m.type = type;
    m.function = function;
    m.scope = scope;
    m.declarationLocation = declarationLocation;
    m.isInjected = isInjected;
    members.insert(name, m);
    return true;
}

Context::ResolvedName Context::resolveName(const QString &name, const QQmlJS::SourceLocation &accessLocation)
{
    int scope = 0;
    Context *c = this;

    ResolvedName result;

    while (c) {
        if (c->isWithBlock)
            return result;

        Context::Member m = c->findMember(name);
        if (!c->parent && m.index < 0)
            break;

        if (m.type != Context::UndefinedMember) {
            result.type = m.canEscape ? ResolvedName::Local : ResolvedName::Stack;
            result.scope = scope;
            result.index = m.index;
            result.isConst = (m.scope == VariableScope::Const);
            result.requiresTDZCheck = m.requiresTDZCheck(accessLocation, c != this) || c->isCaseBlock();
            if (c->isStrict && (name == QLatin1String("arguments") || name == QLatin1String("eval")))
                result.isArgOrEval = true;
            result.declarationLocation = m.declarationLocation;
            result.isInjected = m.isInjected;
            return result;
        }
        const int argIdx = c->findArgument(name, &result.isInjected);
        if (argIdx != -1) {
            if (c->argumentsCanEscape) {
                result.index = argIdx + c->locals.size();
                result.scope = scope;
                result.type = ResolvedName::Local;
                result.isConst = false;
                return result;
            } else {
                result.index = argIdx + sizeof(CallData) / sizeof(StaticValue) - 1;
                result.scope = 0;
                result.type = ResolvedName::Stack;
                result.isConst = false;
                return result;
            }
        }
        if (c->hasDirectEval) {
            Q_ASSERT(!c->isStrict && c->contextType != ContextType::Block);
            return result;
        }

        if (c->requiresExecutionContext)
            ++scope;
        c = c->parent;
    }

    if (!c)
        return result;

    if (c->contextType == ContextType::ESModule) {
        for (int i = 0; i < c->importEntries.size(); ++i) {
            if (c->importEntries.at(i).localName == name) {
                result.index = i;
                result.type = ResolvedName::Import;
                result.isConst = true;
                // We don't know at compile time whether the imported value is let/const or not.
                result.requiresTDZCheck = true;
                return result;
            }
        }
    }

    // ### can we relax the restrictions here?
    if (c->contextType == ContextType::Eval)
        return result;

    if (c->contextType == ContextType::Binding || c->contextType == ContextType::ScriptImportedByQML)
        result.type = ResolvedName::QmlGlobal;
    else
        result.type = ResolvedName::Global;
    return result;
}

void Context::emitBlockHeader(Codegen *codegen)
{
    using Instruction = Moth::Instruction;
    Moth::BytecodeGenerator *bytecodeGenerator = codegen->generator();

    setupFunctionIndices(bytecodeGenerator);

    if (requiresExecutionContext) {
        if (blockIndex < 0) {
            codegen->module()->blocks.append(this);
            blockIndex = codegen->module()->blocks.size() - 1;
        }

        if (contextType == ContextType::Global) {
            Instruction::PushScriptContext scriptContext;
            scriptContext.index = blockIndex;
            bytecodeGenerator->addInstruction(scriptContext);
        } else if (contextType == ContextType::Block || (contextType == ContextType::Eval && !isStrict)) {
            if (isCatchBlock) {
                Instruction::PushCatchContext catchContext;
                catchContext.index = blockIndex;
                catchContext.name = codegen->registerString(caughtVariable);
                bytecodeGenerator->addInstruction(catchContext);
            } else {
                Instruction::PushBlockContext blockContext;
                blockContext.index = blockIndex;
                bytecodeGenerator->addInstruction(blockContext);
            }
        } else if (contextType != ContextType::ESModule && contextType != ContextType::ScriptImportedByQML) {
            Instruction::CreateCallContext createContext;
            bytecodeGenerator->addInstruction(createContext);
        }
    }

    if (contextType == ContextType::Block && sizeOfRegisterTemporalDeadZone > 0) {
        Instruction::InitializeBlockDeadTemporalZone tdzInit;
        tdzInit.firstReg = registerOffset + nRegisters - sizeOfRegisterTemporalDeadZone;
        tdzInit.count = sizeOfRegisterTemporalDeadZone;
        bytecodeGenerator->addInstruction(tdzInit);
    }

    if (usesThis) {
        Q_ASSERT(!isStrict);
        // make sure we convert this to an object
        Instruction::ConvertThisToObject convert;
        bytecodeGenerator->addInstruction(convert);
    }
    if (innerFunctionAccessesThis) {
        Instruction::LoadReg load;
        load.reg = CallData::This;
        bytecodeGenerator->addInstruction(load);
        Codegen::Reference r = codegen->referenceForName(QStringLiteral("this"), true);
        r.storeConsumeAccumulator();
    }
    if (innerFunctionAccessesNewTarget) {
        Instruction::LoadReg load;
        load.reg = CallData::NewTarget;
        bytecodeGenerator->addInstruction(load);
        Codegen::Reference r = codegen->referenceForName(QStringLiteral("new.target"), true);
        r.storeConsumeAccumulator();
    }

    if (contextType == ContextType::Global || contextType == ContextType::ScriptImportedByQML || (contextType == ContextType::Eval && !isStrict)) {
        // variables in global code are properties of the global context object, not locals as with other functions.
        for (Context::MemberMap::const_iterator it = members.constBegin(), cend = members.constEnd(); it != cend; ++it) {
            if (it->isLexicallyScoped())
                continue;
            const QString &local = it.key();

            Instruction::DeclareVar declareVar;
            declareVar.isDeletable = (contextType == ContextType::Eval);
            declareVar.varName = codegen->registerString(local);
            bytecodeGenerator->addInstruction(declareVar);
        }
    }

    if (contextType == ContextType::Function || contextType == ContextType::Binding || contextType == ContextType::ESModule) {
        for (Context::MemberMap::iterator it = members.begin(), end = members.end(); it != end; ++it) {
            if (it->canEscape && it->type == Context::ThisFunctionName) {
                // move the function from the stack to the call context
                Instruction::LoadReg load;
                load.reg = CallData::Function;
                bytecodeGenerator->addInstruction(load);
                Instruction::StoreLocal store;
                store.index = it->index;
                bytecodeGenerator->addInstruction(store);
            }
        }
    }

    if (usesArgumentsObject == Context::ArgumentsObjectUsed) {
        Q_ASSERT(contextType != ContextType::Block);
        if (isStrict || (formals && !formals->isSimpleParameterList())) {
            Instruction::CreateUnmappedArgumentsObject setup;
            bytecodeGenerator->addInstruction(setup);
        } else {
            Instruction::CreateMappedArgumentsObject setup;
            bytecodeGenerator->addInstruction(setup);
        }
        codegen->referenceForName(QStringLiteral("arguments"), false).storeConsumeAccumulator();
    }

    for (const Context::Member &member : std::as_const(members)) {
        if (member.function) {
            const int function = codegen->defineFunction(member.function->name.toString(), member.function, member.function->formals, member.function->body);
            codegen->loadClosure(function);
            Codegen::Reference r = codegen->referenceForName(member.function->name.toString(), true);
            r.storeConsumeAccumulator();
        }
    }
}

void Context::emitBlockFooter(Codegen *codegen)
{
    using Instruction = Moth::Instruction;
    Moth::BytecodeGenerator *bytecodeGenerator = codegen->generator();

    if (!requiresExecutionContext)
        return;

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wmaybe-uninitialized") // the loads below are empty structs.
    if (contextType == ContextType::Global)
        bytecodeGenerator->addInstruction(Instruction::PopScriptContext());
    else if (contextType != ContextType::ESModule && contextType != ContextType::ScriptImportedByQML)
        bytecodeGenerator->addInstruction(Instruction::PopContext());
QT_WARNING_POP
}

void Context::setupFunctionIndices(Moth::BytecodeGenerator *bytecodeGenerator)
{
    if (registerOffset != -1) {
        // already computed, check for consistency
        Q_ASSERT(registerOffset == bytecodeGenerator->currentRegister());
        bytecodeGenerator->newRegisterArray(nRegisters);
        return;
    }
    Q_ASSERT(locals.size() == 0);
    Q_ASSERT(nRegisters == 0);
    registerOffset = bytecodeGenerator->currentRegister();

    QVector<Context::MemberMap::Iterator> localsInTDZ;
    const auto registerLocal = [this, &localsInTDZ](Context::MemberMap::iterator member) {
        if (member->isLexicallyScoped()) {
            localsInTDZ << member;
        } else {
            member->index = locals.size();
            locals.append(member.key());
        }
    };

    QVector<Context::MemberMap::Iterator> registersInTDZ;
    const auto allocateRegister = [bytecodeGenerator, &registersInTDZ](Context::MemberMap::iterator member) {
        if (member->isLexicallyScoped())
            registersInTDZ << member;
        else
            member->index = bytecodeGenerator->newRegister();
    };

    switch (contextType) {
    case ContextType::ESModule:
    case ContextType::Block:
    case ContextType::Function:
    case ContextType::Binding: {
        for (Context::MemberMap::iterator it = members.begin(), end = members.end(); it != end; ++it) {
            if (it->canEscape) {
                registerLocal(it);
            } else {
                if (it->type == Context::ThisFunctionName)
                    it->index = CallData::Function;
                else
                    allocateRegister(it);
            }
        }
        break;
    }
    case ContextType::Global:
    case ContextType::ScriptImportedByQML:
    case ContextType::Eval:
        for (Context::MemberMap::iterator it = members.begin(), end = members.end(); it != end; ++it) {
            if (!it->isLexicallyScoped() && (contextType == ContextType::Global || contextType == ContextType::ScriptImportedByQML || !isStrict))
                continue;
            if (it->canEscape)
                registerLocal(it);
            else
                allocateRegister(it);
        }
        break;
    }

    sizeOfLocalTemporalDeadZone = localsInTDZ.size();
    for (auto &member: std::as_const(localsInTDZ)) {
        member->index = locals.size();
        locals.append(member.key());
    }

    if (contextType == ContextType::ESModule && !localNameForDefaultExport.isEmpty()) {
        if (!members.contains(localNameForDefaultExport)) {
            // allocate a local slot for the default export, to be used in
            // CodeGen::visit(ExportDeclaration*).
            locals.append(localNameForDefaultExport);
            ++sizeOfLocalTemporalDeadZone;
        }
    }

    sizeOfRegisterTemporalDeadZone = registersInTDZ.size();
    firstTemporalDeadZoneRegister = bytecodeGenerator->currentRegister();
    for (auto &member: std::as_const(registersInTDZ))
        member->index = bytecodeGenerator->newRegister();

    nRegisters = bytecodeGenerator->currentRegister() - registerOffset;
}

QT_END_NAMESPACE
