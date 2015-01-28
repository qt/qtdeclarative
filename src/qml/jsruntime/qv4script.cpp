/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <private/qqmlcontextwrapper_p.h>

#include <QtCore/QDebug>
#include <QtCore/QString>

QT_BEGIN_NAMESPACE

namespace QV4 {
namespace Heap {

struct CompilationUnitHolder : Object {
    inline CompilationUnitHolder(ExecutionEngine *engine, CompiledData::CompilationUnit *unit);

    QQmlRefPointer<CompiledData::CompilationUnit> unit;
};

}

struct CompilationUnitHolder : public Object
{
    V4_OBJECT2(CompilationUnitHolder, Object)
    V4_NEEDS_DESTROY
};

inline
Heap::CompilationUnitHolder::CompilationUnitHolder(ExecutionEngine *engine, CompiledData::CompilationUnit *unit)
    : Heap::Object(engine)
    , unit(unit)
{
}

}

QT_END_NAMESPACE

using namespace QV4;

DEFINE_OBJECT_VTABLE(QmlBindingWrapper);
DEFINE_OBJECT_VTABLE(CompilationUnitHolder);

Heap::QmlBindingWrapper::QmlBindingWrapper(QV4::ExecutionContext *scope, Function *f, QV4::Object *qml)
    : Heap::FunctionObject(scope, scope->d()->engine->id_eval, /*createProto = */ false)
    , qml(qml->d())
{
    Q_ASSERT(scope->inUse());

    function = f;
    if (function)
        function->compilationUnit->addref();

    Scope s(scope);
    Scoped<QV4::QmlBindingWrapper> o(s, this);

    o->defineReadonlyProperty(scope->d()->engine->id_length, Primitive::fromInt32(1));

    ScopedContext ctx(s, s.engine->currentContext());
    o->d()->qmlContext = ctx->newQmlContext(o, qml);
    s.engine->popContext();
}

Heap::QmlBindingWrapper::QmlBindingWrapper(QV4::ExecutionContext *scope, QV4::Object *qml)
    : Heap::FunctionObject(scope, scope->d()->engine->id_eval, /*createProto = */ false)
    , qml(qml->d())
{
    Q_ASSERT(scope->inUse());

    Scope s(scope);
    Scoped<QV4::QmlBindingWrapper> o(s, this);

    o->defineReadonlyProperty(scope->d()->engine->id_length, Primitive::fromInt32(1));

    ScopedContext ctx(s, s.engine->currentContext());
    o->d()->qmlContext = ctx->newQmlContext(o, qml);
    s.engine->popContext();
}

ReturnedValue QmlBindingWrapper::call(Managed *that, CallData *)
{
    ExecutionEngine *engine = static_cast<Object *>(that)->engine();
    CHECK_STACK_LIMITS(engine);

    Scope scope(engine);
    QmlBindingWrapper *This = static_cast<QmlBindingWrapper *>(that);
    if (!This->function())
        return QV4::Encode::undefined();

    Scoped<CallContext> ctx(scope, This->d()->qmlContext);
    std::fill(ctx->d()->locals, ctx->d()->locals + ctx->d()->function->varCount(), Primitive::undefinedValue());
    engine->pushContext(ctx);
    ScopedValue result(scope, This->function()->code(engine, This->function()->codeData));
    engine->popContext();

    return result->asReturnedValue();
}

void QmlBindingWrapper::markObjects(Heap::Base *m, ExecutionEngine *e)
{
    QmlBindingWrapper::Data *wrapper = static_cast<QmlBindingWrapper::Data *>(m);
    if (wrapper->qml)
        wrapper->qml->mark(e);
    FunctionObject::markObjects(m, e);
    if (wrapper->qmlContext)
        wrapper->qmlContext->mark(e);
}

static ReturnedValue signalParameterGetter(QV4::CallContext *ctx, uint parameterIndex)
{
    QV4::Scope scope(ctx);
    QV4::Scoped<CallContext> signalEmittingContext(scope, static_cast<Heap::CallContext *>(ctx->d()->parent));
    Q_ASSERT(signalEmittingContext && signalEmittingContext->d()->type >= QV4::Heap::ExecutionContext::Type_SimpleCallContext);
    return signalEmittingContext->argument(parameterIndex);
}

Heap::FunctionObject *QmlBindingWrapper::createQmlCallableForFunction(QQmlContextData *qmlContext, QObject *scopeObject, Function *runtimeFunction, const QList<QByteArray> &signalParameters, QString *error)
{
    ExecutionEngine *engine = QQmlEnginePrivate::getV4Engine(qmlContext->engine);
    QV4::Scope valueScope(engine);
    QV4::ScopedObject qmlScopeObject(valueScope, QV4::QmlContextWrapper::qmlScope(engine, qmlContext, scopeObject));
    ScopedContext global(valueScope, valueScope.engine->rootContext());
    QV4::Scoped<QV4::QmlBindingWrapper> wrapper(valueScope, engine->memoryManager->alloc<QV4::QmlBindingWrapper>(global, qmlScopeObject));
    QV4::Scoped<CallContext> wrapperContext(valueScope, wrapper->context());

    if (!signalParameters.isEmpty()) {
        if (error)
            QQmlPropertyCache::signalParameterStringForJS(engine, signalParameters, error);
        QV4::ScopedProperty p(valueScope);
        QV4::ScopedString s(valueScope);
        int index = 0;
        foreach (const QByteArray &param, signalParameters) {
            QV4::ScopedFunctionObject g(valueScope, engine->memoryManager->alloc<QV4::IndexedBuiltinFunction>(wrapperContext, index++, signalParameterGetter));
            p->setGetter(g);
            p->setSetter(0);
            s = engine->newString(QString::fromUtf8(param));
            qmlScopeObject->insertMember(s, p, QV4::Attr_Accessor|QV4::Attr_NotEnumerable|QV4::Attr_NotConfigurable);
        }
    }

    QV4::ScopedFunctionObject function(valueScope, QV4::FunctionObject::createScriptFunction(wrapperContext, runtimeFunction));
    return function->d();
}

Script::Script(ExecutionEngine *v4, Object *qml, CompiledData::CompilationUnit *compilationUnit)
    : line(0), column(0), scope(v4->rootContext()), strictMode(false), inheritContext(true), parsed(false)
    , qml(v4, qml), vmFunction(0), parseAsBinding(true)
{
    parsed = true;

    vmFunction = compilationUnit ? compilationUnit->linkToEngine(v4) : 0;
    if (vmFunction) {
        Scope valueScope(v4);
        ScopedObject holder(valueScope, v4->memoryManager->alloc<CompilationUnitHolder>(v4, compilationUnit));
        compilationUnitHolder.set(v4, holder);
    }
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

    IR::Module module(v4->debugger != 0);

    QQmlJS::Engine ee, *engine = &ee;
    Lexer lexer(engine);
    lexer.setCode(sourceCode, line, parseAsBinding);
    Parser parser(engine);

    const bool parsed = parser.parseProgram();

    foreach (const QQmlJS::DiagnosticMessage &m, parser.diagnosticMessages()) {
        if (m.isError()) {
            valueScope.engine->throwSyntaxError(m.message, sourceFile, m.loc.startLine, m.loc.startColumn);
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
        if (inheritContext) {
            Scoped<CallContext> ctx(valueScope, scope);
            if (ctx) {
                for (Identifier * const *i = ctx->variables(), * const *ei = i + ctx->variableCount(); i < ei; ++i)
                    inheritedLocals.append(*i ? (*i)->string : QString());
            }
        }

        RuntimeCodegen cg(v4, strictMode);
        cg.generateFromProgram(sourceFile, sourceCode, program, &module, QQmlJS::Codegen::EvalCode, inheritedLocals);
        if (v4->hasException)
            return;

        QV4::Compiler::JSUnitGenerator jsGenerator(&module);
        QScopedPointer<EvalInstructionSelection> isel(v4->iselFactory->create(QQmlEnginePrivate::get(v4), v4->executableAllocator, &module, &jsGenerator));
        if (inheritContext)
            isel->setUseFastLookups(false);
        QQmlRefPointer<QV4::CompiledData::CompilationUnit> compilationUnit = isel->compile();
        vmFunction = compilationUnit->linkToEngine(v4);
        ScopedObject holder(valueScope, v4->memoryManager->alloc<CompilationUnitHolder>(v4, compilationUnit));
        compilationUnitHolder.set(v4, holder);
    }

    if (!vmFunction) {
        // ### FIX file/line number
        ScopedObject error(valueScope, v4->newSyntaxErrorObject(QStringLiteral("Syntax error")));
        v4->throwError(error);
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

        ExecutionContextSaver ctxSaver(valueScope, scope);
        ContextStateSaver stateSaver(valueScope, scope);
        scope->strictMode = vmFunction->isStrict();
        scope->lookups = vmFunction->compilationUnit->runtimeLookups;
        scope->compilationUnit = vmFunction->compilationUnit;

        return vmFunction->code(engine, vmFunction->codeData);
    } else {
        ScopedObject qmlObj(valueScope, qml.value());
        ScopedContext ctx(valueScope, scope);
        ScopedFunctionObject f(valueScope, engine->memoryManager->alloc<QmlBindingWrapper>(ctx, vmFunction, qmlObj));
        ScopedCallData callData(valueScope);
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

QQmlRefPointer<QV4::CompiledData::CompilationUnit> Script::precompile(IR::Module *module, Compiler::JSUnitGenerator *unitGenerator, ExecutionEngine *engine, const QUrl &url, const QString &source, QList<QQmlError> *reportedErrors, QQmlJS::Directives *directivesCollector)
{
    using namespace QQmlJS;
    using namespace QQmlJS::AST;

    QQmlJS::Engine ee;
    if (directivesCollector)
        ee.setDirectives(directivesCollector);
    QQmlJS::Lexer lexer(&ee);
    lexer.setCode(source, /*line*/1, /*qml mode*/false);
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
    cg.generateFromProgram(url.toString(), source, program, module, QQmlJS::Codegen::EvalCode);
    errors = cg.qmlErrors();
    if (!errors.isEmpty()) {
        if (reportedErrors)
            *reportedErrors << errors;
        return 0;
    }

    QScopedPointer<EvalInstructionSelection> isel(engine->iselFactory->create(QQmlEnginePrivate::get(engine), engine->executableAllocator, module, unitGenerator));
    isel->setUseFastLookups(false);
    return isel->compile(/*generate unit data*/false);
}

ReturnedValue Script::qmlBinding()
{
    if (!parsed)
        parse();
    ExecutionEngine *v4 = scope->engine;
    Scope valueScope(v4);
    ScopedObject qmlObj(valueScope, qml.value());
    ScopedContext ctx(valueScope, scope);
    ScopedObject v(valueScope, v4->memoryManager->alloc<QmlBindingWrapper>(ctx, vmFunction, qmlObj));
    return v.asReturnedValue();
}

QV4::ReturnedValue Script::evaluate(ExecutionEngine *engine,  const QString &script, Object *scopeObject)
{
    QV4::Scope scope(engine);
    QV4::Script qmlScript(engine, scopeObject, script, QString());

    qmlScript.parse();
    QV4::ScopedValue result(scope);
    if (!scope.engine->hasException)
        result = qmlScript.run();
    if (scope.engine->hasException) {
        scope.engine->catchException();
        return Encode::undefined();
    }
    return result->asReturnedValue();
}
