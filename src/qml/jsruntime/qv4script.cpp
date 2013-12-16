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
#include "qv4scopedvalue_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <private/qqmlengine_p.h>
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>

#include <QtCore/QDebug>
#include <QtCore/QString>

using namespace QV4;

QmlBindingWrapper::QmlBindingWrapper(ExecutionContext *scope, Function *f, ObjectRef qml)
    : FunctionObject(scope, scope->engine->id_eval)
    , qml(qml)
    , qmlContext(0)
{
    Q_ASSERT(scope->inUse);

    setVTable(&static_vtbl);
    function = f;
    function->compilationUnit->ref();
    needsActivation = function->needsActivation();

    Scope s(scope);
    ScopedValue protectThis(s, this);

    defineReadonlyProperty(scope->engine->id_length, Primitive::fromInt32(1));

    qmlContext = scope->engine->currentContext()->newQmlContext(this, qml);
    scope->engine->popContext();
}

QmlBindingWrapper::QmlBindingWrapper(ExecutionContext *scope, ObjectRef qml)
    : FunctionObject(scope, scope->engine->id_eval)
    , qml(qml)
    , qmlContext(0)
{
    Q_ASSERT(scope->inUse);

    setVTable(&static_vtbl);
    function = 0;
    needsActivation = false;

    Scope s(scope);
    ScopedValue protectThis(s, this);

    defineReadonlyProperty(scope->engine->id_length, Primitive::fromInt32(1));

    qmlContext = scope->engine->currentContext()->newQmlContext(this, qml);
    scope->engine->popContext();
}

ReturnedValue QmlBindingWrapper::call(Managed *that, CallData *)
{
    ExecutionEngine *engine = that->engine();
    CHECK_STACK_LIMITS(engine);

    Scope scope(engine);
    QmlBindingWrapper *This = static_cast<QmlBindingWrapper *>(that);
    Q_ASSERT(This->function);

    CallContext *ctx = This->qmlContext;
    std::fill(ctx->locals, ctx->locals + ctx->function->varCount, Primitive::undefinedValue());
    engine->pushContext(ctx);
    ScopedValue result(scope, This->function->code(ctx, This->function->codeData));
    engine->popContext();

    return result.asReturnedValue();
}

void QmlBindingWrapper::markObjects(Managed *m, ExecutionEngine *e)
{
    QmlBindingWrapper *wrapper = static_cast<QmlBindingWrapper*>(m);
    if (wrapper->qml)
        wrapper->qml->mark(e);
    FunctionObject::markObjects(m, e);
    if (wrapper->qmlContext)
        wrapper->qmlContext->mark(e);
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
        setVTable(&static_vtbl);
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

Script::Script(ExecutionEngine *v4, ObjectRef qml, CompiledData::CompilationUnit *compilationUnit)
    : line(0), column(0), scope(v4->rootContext), strictMode(false), inheritContext(true), parsed(false)
    , qml(qml.asReturnedValue()), vmFunction(0), parseAsBinding(true)
{
    parsed = true;

    if (compilationUnit) {
        vmFunction = compilationUnit->linkToEngine(v4);
        Q_ASSERT(vmFunction);
        Scope valueScope(v4);
        ScopedValue holder(valueScope, new (v4->memoryManager) CompilationUnitHolder(v4, compilationUnit));
        compilationUnitHolder = holder;
    } else
        vmFunction = 0;
}

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

    V4IR::Module module(v4->debugger != 0);

    QQmlJS::Engine ee, *engine = &ee;
    Lexer lexer(engine);
    lexer.setCode(sourceCode, line, parseAsBinding);
    Parser parser(engine);

    const bool parsed = parser.parseProgram();

    foreach (const QQmlJS::DiagnosticMessage &m, parser.diagnosticMessages()) {
        if (m.isError()) {
            scope->throwSyntaxError(m.message, sourceFile, m.loc.startLine, m.loc.startColumn);
            return;
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
        cg.generateFromProgram(sourceFile, sourceCode, program, &module, QQmlJS::Codegen::EvalCode, inheritedLocals);
        if (v4->hasException)
            return;

        QV4::Compiler::JSUnitGenerator jsGenerator(&module);
        QScopedPointer<EvalInstructionSelection> isel(v4->iselFactory->create(QQmlEnginePrivate::get(v4), v4->executableAllocator, &module, &jsGenerator));
        if (inheritContext)
            isel->setUseFastLookups(false);
        QV4::CompiledData::CompilationUnit *compilationUnit = isel->compile();
        vmFunction = compilationUnit->linkToEngine(v4);
        ScopedValue holder(valueScope, new (v4->memoryManager) CompilationUnitHolder(v4, compilationUnit));
        compilationUnitHolder = holder;
    }

    if (!vmFunction) {
        // ### FIX file/line number
        Scoped<Object> error(valueScope, v4->newSyntaxErrorObject(QStringLiteral("Syntax error")));
        v4->currentContext()->throwError(error);
    }
}

ReturnedValue Script::run()
{
    struct ContextStateSaver {
        ExecutionContext *savedContext;
        bool strictMode;
        Lookup *lookups;
        CompiledData::CompilationUnit *compilationUnit;

        ContextStateSaver(ExecutionContext *context)
            : savedContext(context)
            , strictMode(context->strictMode)
            , lookups(context->lookups)
            , compilationUnit(context->compilationUnit)
        {}

        ~ContextStateSaver()
        {
            savedContext->strictMode = strictMode;
            savedContext->lookups = lookups;
            savedContext->compilationUnit = compilationUnit;
        }
    };

    if (!parsed)
        parse();
    if (!vmFunction)
        return Encode::undefined();

    QV4::ExecutionEngine *engine = scope->engine;
    QV4::Scope valueScope(engine);

    if (qml.isUndefined()) {
        TemporaryAssignment<Function*> savedGlobalCode(engine->globalCode, vmFunction);

        ExecutionContextSaver ctxSaver(scope);
        ContextStateSaver stateSaver(scope);
        scope->strictMode = vmFunction->isStrict();
        scope->lookups = vmFunction->compilationUnit->runtimeLookups;
        scope->compilationUnit = vmFunction->compilationUnit;

        return vmFunction->code(scope, vmFunction->codeData);
    } else {
        ScopedObject qmlObj(valueScope, qml.value());
        FunctionObject *f = new (engine->memoryManager) QmlBindingWrapper(scope, vmFunction, qmlObj);
        ScopedCallData callData(valueScope, 0);
        callData->thisObject = Primitive::undefinedValue();
        return f->call(callData);
    }
}

Function *Script::function()
{
    if (!parsed)
        parse();
    return vmFunction;
}

CompiledData::CompilationUnit *Script::precompile(ExecutionEngine *engine, const QUrl &url, const QString &source, QList<QQmlError> *reportedErrors)
{
    using namespace QQmlJS;
    using namespace QQmlJS::AST;

    QQmlJS::V4IR::Module module(engine->debugger != 0);

    QQmlJS::Engine ee;
    QQmlJS::Lexer lexer(&ee);
    lexer.setCode(source, /*line*/1, /*qml mode*/true);
    QQmlJS::Parser parser(&ee);

    parser.parseProgram();

    QList<QQmlError> errors;

    foreach (const QQmlJS::DiagnosticMessage &m, parser.diagnosticMessages()) {
        if (m.isWarning()) {
            qWarning("%s:%d : %s", qPrintable(url.toString()), m.loc.startLine, qPrintable(m.message));
            continue;
        }

        QQmlError error;
        error.setUrl(url);
        error.setDescription(m.message);
        error.setLine(m.loc.startLine);
        error.setColumn(m.loc.startColumn);
        errors << error;
    }

    if (!errors.isEmpty()) {
        if (reportedErrors)
            *reportedErrors << errors;
        return 0;
    }

    Program *program = AST::cast<Program *>(parser.rootNode());
    if (!program) {
        // if parsing was successful, and we have no program, then
        // we're done...:
        return 0;
    }

    QQmlJS::Codegen cg(/*strict mode*/false);
    cg.generateFromProgram(url.toString(), source, program, &module, QQmlJS::Codegen::EvalCode);
    errors = cg.errors();
    if (!errors.isEmpty()) {
        if (reportedErrors)
            *reportedErrors << cg.errors();
        return 0;
    }

    Compiler::JSUnitGenerator jsGenerator(&module);
    QScopedPointer<QQmlJS::EvalInstructionSelection> isel(engine->iselFactory->create(QQmlEnginePrivate::get(engine), engine->executableAllocator, &module, &jsGenerator));
    isel->setUseFastLookups(false);
    return isel->compile();
}

ReturnedValue Script::qmlBinding()
{
    if (!parsed)
        parse();
    ExecutionEngine *v4 = scope->engine;
    Scope valueScope(v4);
    ScopedObject qmlObj(valueScope, qml.value());
    ScopedObject v(valueScope, new (v4->memoryManager) QmlBindingWrapper(scope, vmFunction, qmlObj));
    return v.asReturnedValue();
}

QV4::ReturnedValue Script::evaluate(ExecutionEngine *engine,  const QString &script, ObjectRef scopeObject)
{
    QV4::Scope scope(engine);
    QV4::Script qmlScript(engine, scopeObject, script, QString());

    QV4::ExecutionContext *ctx = engine->currentContext();
    qmlScript.parse();
    QV4::ScopedValue result(scope);
    if (!scope.engine->hasException)
        result = qmlScript.run();
    if (scope.engine->hasException) {
        ctx->catchException();
        return Encode::undefined();
    }
    return result.asReturnedValue();
}
