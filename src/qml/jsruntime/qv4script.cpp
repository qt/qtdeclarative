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

#include "qv4script_p.h"
#include "qv4mm_p.h"
#include "qv4functionobject_p.h"
#include "qv4function_p.h"
#include "qv4context_p.h"
#include "qv4debugging_p.h"
#include "qv4exception_p.h"
#include "qv4scopedvalue_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>

#include <QtCore/QDebug>
#include <QtCore/QString>

using namespace QV4;

QmlBindingWrapper::QmlBindingWrapper(ExecutionContext *scope, Function *f, Object *qml)
    : FunctionObject(scope, scope->engine->id_eval)
    , qml(qml)
{
    vtbl = &static_vtbl;
    function = f;
    function->compilationUnit->ref();
    usesArgumentsObject = function->usesArgumentsObject();
    needsActivation = function->needsActivation();
    defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(1));

    qmlContext = scope->engine->current->newQmlContext(this, qml);
    scope->engine->popContext();
}

QmlBindingWrapper::QmlBindingWrapper(ExecutionContext *scope, Object *qml)
    : FunctionObject(scope, scope->engine->id_eval)
    , qml(qml)
{
    vtbl = &static_vtbl;
    function = 0;
    usesArgumentsObject = false;
    needsActivation = false;
    defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(1));

    qmlContext = scope->engine->current->newQmlContext(this, qml);
    scope->engine->popContext();
}

ReturnedValue QmlBindingWrapper::call(Managed *that, CallData *)
{
    ExecutionEngine *engine = that->engine();
    Scope scope(engine);
    QmlBindingWrapper *This = static_cast<QmlBindingWrapper *>(that);
    Q_ASSERT(This->function);

    CallContext *ctx = This->qmlContext;
    std::fill(ctx->locals, ctx->locals + ctx->function->varCount, Value::undefinedValue());
    engine->pushContext(ctx);
    ScopedValue result(scope, This->function->code(ctx, This->function->codeData));
    engine->popContext();

    return result.asReturnedValue();
}

void QmlBindingWrapper::markObjects(Managed *m)
{
    QmlBindingWrapper *wrapper = static_cast<QmlBindingWrapper*>(m);
    if (wrapper->qml)
        wrapper->qml->mark();
    FunctionObject::markObjects(m);
    wrapper->qmlContext->mark();
}

DEFINE_MANAGED_VTABLE(QmlBindingWrapper);

struct CompilationUnitHolder : public QV4::Object
{
    Q_MANAGED

    CompilationUnitHolder(ExecutionEngine *engine, CompiledData::CompilationUnit *unit)
        : Object(engine)
        , unit(unit)
    {
        unit->ref();
        vtbl = &static_vtbl;
    }
    ~CompilationUnitHolder()
    {
        unit->deref();
    }

    static void destroy(Managed *that)
    {
        static_cast<CompilationUnitHolder*>(that)->~CompilationUnitHolder();
    }

    QV4::CompiledData::CompilationUnit *unit;
};

DEFINE_MANAGED_VTABLE(CompilationUnitHolder);

Script::~Script()
{
}

void Script::parse()
{
    if (parsed)
        return;

    using namespace QQmlJS;

    parsed = true;

    ExecutionEngine *v4 = scope->engine;
    Scope valueScope(v4);

    MemoryManager::GCBlocker gcBlocker(v4->memoryManager);

    V4IR::Module module;

    QQmlJS::Engine ee, *engine = &ee;
    Lexer lexer(engine);
    lexer.setCode(sourceCode, line, parseAsBinding);
    Parser parser(engine);

    const bool parsed = parser.parseProgram();

    foreach (const QQmlJS::DiagnosticMessage &m, parser.diagnosticMessages()) {
        if (m.isError()) {
            scope->throwSyntaxError(m.message, sourceFile, m.loc.startLine, m.loc.startColumn);
        } else {
            qWarning() << sourceFile << ':' << m.loc.startLine << ':' << m.loc.startColumn
                      << ": warning: " << m.message;
        }
    }

    if (parsed) {
        using namespace AST;
        Program *program = AST::cast<Program *>(parser.rootNode());
        if (!program) {
            // if parsing was successful, and we have no program, then
            // we're done...:
            return;
        }

        QStringList inheritedLocals;
        if (inheritContext)
            for (String * const *i = scope->variables(), * const *ei = i + scope->variableCount(); i < ei; ++i)
                inheritedLocals.append(*i ? (*i)->toQString() : QString());

        RuntimeCodegen cg(scope, strictMode);
        cg.generateFromProgram(sourceFile, sourceCode, program, &module,
                               parseAsBinding ? QQmlJS::Codegen::QmlBinding : QQmlJS::Codegen::EvalCode, inheritedLocals);
        QV4::Compiler::JSUnitGenerator jsGenerator(&module);
        QScopedPointer<EvalInstructionSelection> isel(v4->iselFactory->create(v4->executableAllocator, &module, &jsGenerator));
        if (inheritContext)
            isel->setUseFastLookups(false);
        QV4::CompiledData::CompilationUnit *compilationUnit = isel->compile();
        vmFunction = compilationUnit->linkToEngine(v4);
        ScopedValue holder(valueScope, Value::fromObject(new (v4->memoryManager) CompilationUnitHolder(v4, compilationUnit)));
        compilationUnitHolder = holder;
    }

    if (!vmFunction) {
        // ### FIX file/line number
        Scoped<Object> error(valueScope, v4->newSyntaxErrorObject("Syntax error"));
        v4->current->throwError(error);
    }
}

ReturnedValue Script::run()
{
    if (!parsed)
        parse();
    if (!vmFunction)
        return Encode::undefined();

    QV4::ExecutionEngine *engine = scope->engine;
    QV4::Scope valueScope(engine);

    if (qml.isUndefined()) {
        TemporaryAssignment<Function*> savedGlobalCode(engine->globalCode, vmFunction);

        bool strict = scope->strictMode;
        Lookup *oldLookups = scope->lookups;
        CompiledData::CompilationUnit * const oldCompilationUnit = scope->compilationUnit;
        const CompiledData::Function * const oldCompiledFunction = scope->compiledFunction;
        SafeString * const oldRuntimeStrings = scope->runtimeStrings;

        scope->strictMode = vmFunction->isStrict();
        scope->lookups = vmFunction->compilationUnit->runtimeLookups;
        scope->compilationUnit = vmFunction->compilationUnit;
        scope->compiledFunction = vmFunction->compiledFunction;
        scope->runtimeStrings = vmFunction->compilationUnit->runtimeStrings;

        QV4::ScopedValue result(valueScope);
        try {
            result = vmFunction->code(scope, vmFunction->codeData);
        } catch (Exception &e) {
            scope->strictMode = strict;
            scope->lookups = oldLookups;
            scope->compilationUnit = oldCompilationUnit;
            scope->compiledFunction = oldCompiledFunction;
            scope->runtimeStrings = oldRuntimeStrings;
            throw;
        }

        scope->lookups = oldLookups;
        scope->compilationUnit = oldCompilationUnit;
        scope->compiledFunction = oldCompiledFunction;
        scope->runtimeStrings = oldRuntimeStrings;

        return result.asReturnedValue();

    } else {
        ScopedObject qmlObj(valueScope, qml.value());
        FunctionObject *f = new (engine->memoryManager) QmlBindingWrapper(scope, vmFunction, qmlObj.getPointer());
        ScopedCallData callData(valueScope, 0);
        callData->thisObject = Value::undefinedValue();
        return f->call(callData);
    }
}

Function *Script::function()
{
    if (!parsed)
        parse();
    return vmFunction;
}

ReturnedValue Script::qmlBinding()
{
    if (!parsed)
        parse();
    ExecutionEngine *v4 = scope->engine;
    Scope valueScope(v4);
    ScopedObject qmlObj(valueScope, qml.value());
    return Value::fromObject(new (v4->memoryManager) QmlBindingWrapper(scope, vmFunction, qmlObj.getPointer())).asReturnedValue();
}

QV4::ReturnedValue Script::evaluate(ExecutionEngine *engine,  const QString &script, ObjectRef scopeObject)
{
    QV4::Scope scope(engine);
    QV4::Script qmlScript(engine, scopeObject, script, QString());

    QV4::ExecutionContext *ctx = engine->current;
    try {
        qmlScript.parse();
        return qmlScript.run();
    } catch (QV4::Exception &e) {
        e.accept(ctx);
    }
    return Encode::undefined();
}
