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
#include <qv4engine_p.h>
#include <qv4value_p.h>
#include <qv4object_p.h>
#include <qv4objectproto_p.h>
#include <qv4arrayobject_p.h>
#include <qv4booleanobject_p.h>
#include <qv4globalobject_p.h>
#include <qv4errorobject_p.h>
#include <qv4functionobject_p.h>
#include "qv4function_p.h"
#include <qv4mathobject_p.h>
#include <qv4numberobject_p.h>
#include <qv4regexpobject_p.h>
#include <qv4variantobject_p.h>
#include <qv4runtime_p.h>
#include "qv4mm_p.h"
#include <qv4argumentsobject_p.h>
#include <qv4dateobject_p.h>
#include <qv4jsonobject_p.h>
#include <qv4stringobject_p.h>
#include <qv4identifiertable_p.h>
#include <qv4unwindhelper_p.h>
#include "qv4debugging_p.h"
#include "qv4executableallocator_p.h"
#include "qv4sequenceobject_p.h"
#include "qv4qobjectwrapper_p.h"
#include "qv4qmlextensions_p.h"
#include "qv4stacktrace_p.h"

#ifdef V4_ENABLE_JIT
#include "qv4isel_masm_p.h"
#endif // V4_ENABLE_JIT

#include "qv4isel_moth_p.h"

QT_BEGIN_NAMESPACE

using namespace QV4;

static QBasicAtomicInt engineSerial = Q_BASIC_ATOMIC_INITIALIZER(1);

ExecutionEngine::ExecutionEngine(QQmlJS::EvalISelFactory *factory)
    : memoryManager(new QV4::MemoryManager)
    , executableAllocator(new QV4::ExecutableAllocator)
    , regExpAllocator(new QV4::ExecutableAllocator)
    , bumperPointerAllocator(new WTF::BumpPointerAllocator)
    , debugger(0)
    , globalObject(0)
    , globalCode(0)
    , functionsNeedSort(false)
    , m_engineId(engineSerial.fetchAndAddOrdered(1))
    , regExpCache(0)
    , m_multiplyWrappedQObjects(0)
    , m_qmlExtensions(0)
{
    MemoryManager::GCBlocker gcBlocker(memoryManager);

    if (!factory) {
#ifdef V4_ENABLE_JIT
        factory = new QQmlJS::MASM::ISelFactory;
#else // !V4_ENABLE_JIT
        factory = new QQmlJS::Moth::ISelFactory;
#endif // V4_ENABLE_JIT
    }
    iselFactory.reset(factory);

    memoryManager->setExecutionEngine(this);

    identifierTable = new IdentifierTable(this);

    emptyClass =  new (classPool.allocate(sizeof(InternalClass))) InternalClass(this);

    id_undefined = newIdentifier(QStringLiteral("undefined"));
    id_null = newIdentifier(QStringLiteral("null"));
    id_true = newIdentifier(QStringLiteral("true"));
    id_false = newIdentifier(QStringLiteral("false"));
    id_boolean = newIdentifier(QStringLiteral("boolean"));
    id_number = newIdentifier(QStringLiteral("number"));
    id_string = newIdentifier(QStringLiteral("string"));
    id_object = newIdentifier(QStringLiteral("object"));
    id_function = newIdentifier(QStringLiteral("function"));
    id_length = newIdentifier(QStringLiteral("length"));
    id_prototype = newIdentifier(QStringLiteral("prototype"));
    id_constructor = newIdentifier(QStringLiteral("constructor"));
    id_arguments = newIdentifier(QStringLiteral("arguments"));
    id_caller = newIdentifier(QStringLiteral("caller"));
    id_callee = newIdentifier(QStringLiteral("callee"));
    id_this = newIdentifier(QStringLiteral("this"));
    id___proto__ = newIdentifier(QStringLiteral("__proto__"));
    id_enumerable = newIdentifier(QStringLiteral("enumerable"));
    id_configurable = newIdentifier(QStringLiteral("configurable"));
    id_writable = newIdentifier(QStringLiteral("writable"));
    id_value = newIdentifier(QStringLiteral("value"));
    id_get = newIdentifier(QStringLiteral("get"));
    id_set = newIdentifier(QStringLiteral("set"));
    id_eval = newIdentifier(QStringLiteral("eval"));
    id_uintMax = newIdentifier(QStringLiteral("4294967295"));
    id_name = newIdentifier(QStringLiteral("name"));

    arrayClass = emptyClass->addMember(id_length, Attr_NotConfigurable|Attr_NotEnumerable);
    InternalClass *argsClass = emptyClass->addMember(id_length, Attr_NotEnumerable);
    argumentsObjectClass = argsClass->addMember(id_callee, Attr_Data|Attr_NotEnumerable);
    strictArgumentsObjectClass = argsClass->addMember(id_callee, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    strictArgumentsObjectClass = strictArgumentsObjectClass->addMember(id_caller, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    initRootContext();

    objectPrototype = new (memoryManager) ObjectPrototype(this);
    stringPrototype = new (memoryManager) StringPrototype(this);
    numberPrototype = new (memoryManager) NumberPrototype(this);
    booleanPrototype = new (memoryManager) BooleanPrototype(this);
    arrayPrototype = new (memoryManager) ArrayPrototype(rootContext);
    datePrototype = new (memoryManager) DatePrototype(this);
    functionPrototype = new (memoryManager) FunctionPrototype(rootContext);
    regExpPrototype = new (memoryManager) RegExpPrototype(this);
    errorPrototype = new (memoryManager) ErrorPrototype(this);
    evalErrorPrototype = new (memoryManager) EvalErrorPrototype(this);
    rangeErrorPrototype = new (memoryManager) RangeErrorPrototype(this);
    referenceErrorPrototype = new (memoryManager) ReferenceErrorPrototype(this);
    syntaxErrorPrototype = new (memoryManager) SyntaxErrorPrototype(this);
    typeErrorPrototype = new (memoryManager) TypeErrorPrototype(this);
    uRIErrorPrototype = new (memoryManager) URIErrorPrototype(this);

    variantPrototype = new (memoryManager) VariantPrototype(this);
    sequencePrototype = new (memoryManager) SequencePrototype(this);

    stringPrototype->prototype = objectPrototype;
    numberPrototype->prototype = objectPrototype;
    booleanPrototype->prototype = objectPrototype;
    arrayPrototype->prototype = objectPrototype;
    datePrototype->prototype = objectPrototype;
    functionPrototype->prototype = objectPrototype;
    regExpPrototype->prototype = objectPrototype;
    errorPrototype->prototype = objectPrototype;
    evalErrorPrototype->prototype = objectPrototype;
    rangeErrorPrototype->prototype = objectPrototype;
    referenceErrorPrototype->prototype = objectPrototype;
    syntaxErrorPrototype->prototype = objectPrototype;
    typeErrorPrototype->prototype = objectPrototype;
    uRIErrorPrototype->prototype = objectPrototype;

    objectCtor = Value::fromObject(new (memoryManager) ObjectCtor(rootContext));
    stringCtor = Value::fromObject(new (memoryManager) StringCtor(rootContext));
    numberCtor = Value::fromObject(new (memoryManager) NumberCtor(rootContext));
    booleanCtor = Value::fromObject(new (memoryManager) BooleanCtor(rootContext));
    arrayCtor = Value::fromObject(new (memoryManager) ArrayCtor(rootContext));
    functionCtor = Value::fromObject(new (memoryManager) FunctionCtor(rootContext));
    dateCtor = Value::fromObject(new (memoryManager) DateCtor(rootContext));
    regExpCtor = Value::fromObject(new (memoryManager) RegExpCtor(rootContext));
    errorCtor = Value::fromObject(new (memoryManager) ErrorCtor(rootContext));
    evalErrorCtor = Value::fromObject(new (memoryManager) EvalErrorCtor(rootContext));
    rangeErrorCtor = Value::fromObject(new (memoryManager) RangeErrorCtor(rootContext));
    referenceErrorCtor = Value::fromObject(new (memoryManager) ReferenceErrorCtor(rootContext));
    syntaxErrorCtor = Value::fromObject(new (memoryManager) SyntaxErrorCtor(rootContext));
    typeErrorCtor = Value::fromObject(new (memoryManager) TypeErrorCtor(rootContext));
    uRIErrorCtor = Value::fromObject(new (memoryManager) URIErrorCtor(rootContext));

    objectCtor.objectValue()->prototype = functionPrototype;
    stringCtor.objectValue()->prototype = functionPrototype;
    numberCtor.objectValue()->prototype = functionPrototype;
    booleanCtor.objectValue()->prototype = functionPrototype;
    arrayCtor.objectValue()->prototype = functionPrototype;
    functionCtor.objectValue()->prototype = functionPrototype;
    dateCtor.objectValue()->prototype = functionPrototype;
    regExpCtor.objectValue()->prototype = functionPrototype;
    errorCtor.objectValue()->prototype = functionPrototype;
    evalErrorCtor.objectValue()->prototype = functionPrototype;
    rangeErrorCtor.objectValue()->prototype = functionPrototype;
    referenceErrorCtor.objectValue()->prototype = functionPrototype;
    syntaxErrorCtor.objectValue()->prototype = functionPrototype;
    typeErrorCtor.objectValue()->prototype = functionPrototype;
    uRIErrorCtor.objectValue()->prototype = functionPrototype;

    objectPrototype->init(rootContext, objectCtor);
    stringPrototype->init(this, stringCtor);
    numberPrototype->init(rootContext, numberCtor);
    booleanPrototype->init(rootContext, booleanCtor);
    arrayPrototype->init(rootContext, arrayCtor);
    datePrototype->init(rootContext, dateCtor);
    functionPrototype->init(rootContext, functionCtor);
    regExpPrototype->init(rootContext, regExpCtor);
    errorPrototype->init(this, errorCtor);
    evalErrorPrototype->init(this, evalErrorCtor);
    rangeErrorPrototype->init(this, rangeErrorCtor);
    referenceErrorPrototype->init(this, referenceErrorCtor);
    syntaxErrorPrototype->init(this, syntaxErrorCtor);
    typeErrorPrototype->init(this, typeErrorCtor);
    uRIErrorPrototype->init(this, uRIErrorCtor);

    variantPrototype->init(this);
    sequencePrototype->init(this);

    //
    // set up the global object
    //
    globalObject = newObject(/*rootContext*/);
    rootContext->global = globalObject;
    rootContext->thisObject = Value::fromObject(globalObject);

    globalObject->defineDefaultProperty(rootContext, QStringLiteral("Object"), objectCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("String"), stringCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("Number"), numberCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("Boolean"), booleanCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("Array"), arrayCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("Function"), functionCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("Date"), dateCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("RegExp"), regExpCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("Error"), errorCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("EvalError"), evalErrorCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("RangeError"), rangeErrorCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("ReferenceError"), referenceErrorCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("SyntaxError"), syntaxErrorCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("TypeError"), typeErrorCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("URIError"), uRIErrorCtor);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("Math"), Value::fromObject(new (memoryManager) MathObject(rootContext)));
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("JSON"), Value::fromObject(new (memoryManager) JsonObject(rootContext)));

    globalObject->defineReadonlyProperty(this, QStringLiteral("undefined"), Value::undefinedValue());
    globalObject->defineReadonlyProperty(this, QStringLiteral("NaN"), Value::fromDouble(std::numeric_limits<double>::quiet_NaN()));
    globalObject->defineReadonlyProperty(this, QStringLiteral("Infinity"), Value::fromDouble(Q_INFINITY));

    evalFunction = new (memoryManager) EvalFunction(rootContext);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("eval"), Value::fromObject(evalFunction));

    globalObject->defineDefaultProperty(rootContext, QStringLiteral("parseInt"), GlobalFunctions::method_parseInt, 2);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("parseFloat"), GlobalFunctions::method_parseFloat, 1);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("isNaN"), GlobalFunctions::method_isNaN, 1);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("isFinite"), GlobalFunctions::method_isFinite, 1);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("decodeURI"), GlobalFunctions::method_decodeURI, 1);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("decodeURIComponent"), GlobalFunctions::method_decodeURIComponent, 1);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("encodeURI"), GlobalFunctions::method_encodeURI, 1);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("encodeURIComponent"), GlobalFunctions::method_encodeURIComponent, 1);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("escape"), GlobalFunctions::method_escape, 1);
    globalObject->defineDefaultProperty(rootContext, QStringLiteral("unescape"), GlobalFunctions::method_unescape, 1);
}

ExecutionEngine::~ExecutionEngine()
{
    delete debugger;
    delete m_multiplyWrappedQObjects;
    m_multiplyWrappedQObjects = 0;
    delete identifierTable;
    delete memoryManager;
    delete m_qmlExtensions;
    emptyClass->destroy();
    delete bumperPointerAllocator;
    delete regExpCache;
    UnwindHelper::deregisterFunctions(functions);
    delete regExpAllocator;
    delete executableAllocator;
}

void ExecutionEngine::enableDebugger()
{
    Q_ASSERT(!debugger);
    debugger = new Debugging::Debugger(this);
    iselFactory.reset(new QQmlJS::Moth::ISelFactory);
}

void ExecutionEngine::initRootContext()
{
    rootContext = static_cast<GlobalContext *>(memoryManager->allocContext(sizeof(GlobalContext)));
    current = rootContext;
    current->parent = 0;
    rootContext->initGlobalContext(this);
}

InternalClass *ExecutionEngine::newClass(const InternalClass &other)
{
    return new (classPool.allocate(sizeof(InternalClass))) InternalClass(other);
}

WithContext *ExecutionEngine::newWithContext(Object *with)
{
    WithContext *w = static_cast<WithContext *>(memoryManager->allocContext(sizeof(WithContext)));
    ExecutionContext *p = current;
    current = w;
    w->initWithContext(p, with);
    return w;
}

CatchContext *ExecutionEngine::newCatchContext(String *exceptionVarName, const Value &exceptionValue)
{
    CatchContext *c = static_cast<CatchContext *>(memoryManager->allocContext(sizeof(CatchContext)));
    ExecutionContext *p = current;
    current = c;
    c->initCatchContext(p, exceptionVarName, exceptionValue);
    return c;
}

CallContext *ExecutionEngine::newQmlContext(FunctionObject *f, Object *qml)
{
    CallContext *c = static_cast<CallContext *>(memoryManager->allocContext(requiredMemoryForExecutionContect(f, 0)));

    ExecutionContext *p = current;
    current = c;
    c->initQmlContext(p, qml, f);

    return c;
}

CallContext *ExecutionEngine::newCallContext(void *stackSpace, FunctionObject *f, const Value &thisObject, Value *args, int argc)
{
    CallContext *c;
    uint memory = requiredMemoryForExecutionContect(f, argc);
    if (f->needsActivation || memory > stackContextSize) {
        c = static_cast<CallContext *>(memoryManager->allocContext(memory));
    } else {
        c = (CallContext *)stackSpace;
#ifndef QT_NO_DEBUG
        c->next = (CallContext *)0x1;
#endif
    }

    ExecutionContext *p = current;
    current = c;
    c->initCallContext(p, f, args, argc, thisObject);

    return c;
}


ExecutionContext *ExecutionEngine::pushGlobalContext()
{
    GlobalContext *g = static_cast<GlobalContext *>(memoryManager->allocContext(sizeof(GlobalContext)));
    ExecutionContext *oldNext = g->next;
    *g = *rootContext;
    g->next = oldNext;
    g->parent = current;
    current = g;

    return current;
}

Function *ExecutionEngine::newFunction(const QString &name)
{
    Function *f = new Function(this, newIdentifier(name));
    functions.append(f);
    functionsNeedSort = true;
    return f;
}

FunctionObject *ExecutionEngine::newBuiltinFunction(ExecutionContext *scope, String *name, Value (*code)(SimpleCallContext *))
{
    BuiltinFunctionOld *f = new (memoryManager) BuiltinFunctionOld(scope, name, code);
    return f;
}

FunctionObject *ExecutionEngine::newScriptFunction(ExecutionContext *scope, Function *function)
{
    assert(function);

    ScriptFunction *f = new (memoryManager) ScriptFunction(scope, function);
    return f;
}

BoundFunction *ExecutionEngine::newBoundFunction(ExecutionContext *scope, FunctionObject *target, Value boundThis, const QVector<Value> &boundArgs)
{
    assert(target);

    BoundFunction *f = new (memoryManager) BoundFunction(scope, target, boundThis, boundArgs);
    return f;
}


Object *ExecutionEngine::newObject()
{
    Object *object = new (memoryManager) Object(this);
    object->prototype = objectPrototype;
    return object;
}

Object *ExecutionEngine::newObject(InternalClass *internalClass)
{
    Object *object = new (memoryManager) Object(this, internalClass);
    object->prototype = objectPrototype;
    return object;
}

String *ExecutionEngine::newString(const QString &s)
{
    return new (memoryManager) String(this, s);
}

String *ExecutionEngine::newIdentifier(const QString &text)
{
    return identifierTable->insertString(text);
}

Object *ExecutionEngine::newStringObject(const Value &value)
{
    StringObject *object = new (memoryManager) StringObject(this, value);
    object->prototype = stringPrototype;
    return object;
}

Object *ExecutionEngine::newNumberObject(const Value &value)
{
    NumberObject *object = new (memoryManager) NumberObject(this, value);
    object->prototype = numberPrototype;
    return object;
}

Object *ExecutionEngine::newBooleanObject(const Value &value)
{
    Object *object = new (memoryManager) BooleanObject(this, value);
    object->prototype = booleanPrototype;
    return object;
}

ArrayObject *ExecutionEngine::newArrayObject(int count)
{
    ArrayObject *object = new (memoryManager) ArrayObject(this);
    object->prototype = arrayPrototype;

    if (count) {
        if (count < 0x1000)
            object->arrayReserve(count);
        object->setArrayLengthUnchecked(count);
    }
    return object;
}

ArrayObject *ExecutionEngine::newArrayObject(const QStringList &list)
{
    ArrayObject *object = new (memoryManager) ArrayObject(this, list);
    object->prototype = arrayPrototype;
    return object;
}

DateObject *ExecutionEngine::newDateObject(const Value &value)
{
    DateObject *object = new (memoryManager) DateObject(this, value);
    object->prototype = datePrototype;
    return object;
}

DateObject *ExecutionEngine::newDateObject(const QDateTime &dt)
{
    DateObject *object = new (memoryManager) DateObject(this, dt);
    object->prototype = datePrototype;
    return object;
}

RegExpObject *ExecutionEngine::newRegExpObject(const QString &pattern, int flags)
{
    bool global = (flags & QQmlJS::V4IR::RegExp::RegExp_Global);
    bool ignoreCase = false;
    bool multiline = false;
    if (flags & QQmlJS::V4IR::RegExp::RegExp_IgnoreCase)
        ignoreCase = true;
    if (flags & QQmlJS::V4IR::RegExp::RegExp_Multiline)
        multiline = true;

    return newRegExpObject(RegExp::create(this, pattern, ignoreCase, multiline), global);
}

RegExpObject *ExecutionEngine::newRegExpObject(RegExp* re, bool global)
{
    RegExpObject *object = new (memoryManager) RegExpObject(this, re, global);
    object->prototype = regExpPrototype;
    return object;
}

RegExpObject *ExecutionEngine::newRegExpObject(const QRegExp &re)
{
    RegExpObject *object = new (memoryManager) RegExpObject(this, re);
    object->prototype = regExpPrototype;
    return object;
}

Object *ExecutionEngine::newErrorObject(const Value &value)
{
    ErrorObject *object = new (memoryManager) ErrorObject(this, value);
    object->prototype = errorPrototype;
    return object;
}

Object *ExecutionEngine::newSyntaxErrorObject(ExecutionContext *ctx, DiagnosticMessage *message)
{
    return new (memoryManager) SyntaxErrorObject(ctx, message);
}

Object *ExecutionEngine::newSyntaxErrorObject(const QString &message)
{
    return new (memoryManager) SyntaxErrorObject(this, message);
}


Object *ExecutionEngine::newReferenceErrorObject(const QString &message)
{
    return new (memoryManager) ReferenceErrorObject(this, message);
}

Object *ExecutionEngine::newReferenceErrorObject(const QString &message, const QString &fileName, int lineNumber)
{
    return new (memoryManager) ReferenceErrorObject(this, message, fileName, lineNumber);
}


Object *ExecutionEngine::newTypeErrorObject(const QString &message)
{
    return new (memoryManager) TypeErrorObject(this, message);
}

Object *ExecutionEngine::newRangeErrorObject(const QString &message)
{
    return new (memoryManager) RangeErrorObject(this, message);
}

Object *ExecutionEngine::newURIErrorObject(Value message)
{
    return new (memoryManager) URIErrorObject(this, message);
}

Object *ExecutionEngine::newVariantObject(const QVariant &v)
{
    return new (memoryManager) VariantObject(this, v);
}

Object *ExecutionEngine::newForEachIteratorObject(ExecutionContext *ctx, Object *o)
{
    return new (memoryManager) ForEachIteratorObject(ctx, o);
}

Object *ExecutionEngine::qmlContextObject() const
{
    ExecutionContext *ctx = current;

    if (ctx->type == QV4::ExecutionContext::Type_SimpleCallContext)
        ctx = ctx->parent;

    if (!ctx->outer)
        return 0;

    while (ctx->outer && ctx->outer->type != ExecutionContext::Type_GlobalContext)
        ctx = ctx->outer;

    assert(ctx);
    if (ctx->type != ExecutionContext::Type_QmlContext)
        return 0;

    return static_cast<CallContext *>(ctx)->activation;
}

namespace {
    struct LineNumberResolver {
        const ExecutionEngine* engine;
        QScopedPointer<QV4::NativeStackTrace> nativeTrace;

        LineNumberResolver(const ExecutionEngine *engine)
            : engine(engine)
        {
        }

        void resolve(ExecutionEngine::StackFrame *frame, ExecutionContext *context, Function *function)
        {
            if (context->interpreterInstructionPointer) {
                qptrdiff offset = *context->interpreterInstructionPointer - 1 - function->codeData;
                frame->line = function->lineNumberForProgramCounter(offset);
            } else {
                if (!nativeTrace)
                    nativeTrace.reset(new QV4::NativeStackTrace(engine->current));

                NativeFrame nativeFrame = nativeTrace->nextFrame();
                if (nativeFrame.function == function)
                    frame->line = nativeFrame.line;
            }
        }
    };
}

QVector<ExecutionEngine::StackFrame> ExecutionEngine::stackTrace(int frameLimit) const
{
    LineNumberResolver lineNumbers(this);

    QVector<StackFrame> stack;

    QV4::ExecutionContext *c = current;
    while (c && frameLimit) {
        if (CallContext *callCtx = c->asCallContext()) {
            StackFrame frame;
            if (callCtx->function->function)
                frame.source = callCtx->function->function->sourceFile;
            frame.function = callCtx->function->name->toQString();
            frame.line = -1;
            frame.column = -1;

            if (callCtx->function->function)
                lineNumbers.resolve(&frame, callCtx, callCtx->function->function);

            stack.append(frame);
            --frameLimit;
        }
        c = c->parent;
    }

    if (frameLimit && globalCode) {
        StackFrame frame;
        frame.source = globalCode->sourceFile;
        frame.function = globalCode->name->toQString();
        frame.line = -1;
        frame.column = -1;

        lineNumbers.resolve(&frame, rootContext, globalCode);

        stack.append(frame);
    }
    return stack;
}

ExecutionEngine::StackFrame ExecutionEngine::currentStackFrame() const
{
    StackFrame frame;
    frame.line = -1;
    frame.column = -1;

    QVector<StackFrame> trace = stackTrace(/*limit*/ 1);
    if (!trace.isEmpty())
        frame = trace.first();

    return frame;
}

QUrl ExecutionEngine::resolvedUrl(const QString &file)
{
    QUrl src(file);
    if (!src.isRelative())
        return src;

    QUrl base;
    QV4::ExecutionContext *c = current;
    while (c) {
        if (CallContext *callCtx = c->asCallContext()) {
            if (callCtx->function->function)
                base.setUrl(callCtx->function->function->sourceFile);
            break;
        }
        c = c->parent;
    }

    if (base.isEmpty() && globalCode)
            base.setUrl(globalCode->sourceFile);

    if (base.isEmpty())
        return src;

    return base.resolved(src);
}

void ExecutionEngine::requireArgumentsAccessors(int n)
{
    if (n <= argumentsAccessors.size())
        return;

    uint oldSize = argumentsAccessors.size();
    argumentsAccessors.resize(n);
    for (int i = oldSize; i < n; ++i) {
        FunctionObject *get = new (memoryManager) ArgumentsGetterFunction(rootContext, i);
        get->prototype = functionPrototype;
        FunctionObject *set = new (memoryManager) ArgumentsSetterFunction(rootContext, i);
        set->prototype = functionPrototype;
        Property pd = Property::fromAccessor(get, set);
        argumentsAccessors[i] = pd;
    }
}

void ExecutionEngine::markObjects()
{
    identifierTable->mark();

    globalObject->mark();

    if (globalCode)
        globalCode->mark();

    for (int i = 0; i < argumentsAccessors.size(); ++i) {
        const Property &pd = argumentsAccessors.at(i);
        if (FunctionObject *getter = pd.getter())
            getter->mark();
        if (FunctionObject *setter = pd.setter())
            setter->mark();
    }

    ExecutionContext *c = current;
    while (c) {
        c->mark();
        c = c->parent;
    }

    for (int i = 0; i < functions.size(); ++i)
        functions.at(i)->mark();

    id_length->mark();
    id_prototype->mark();
    id_constructor->mark();
    id_arguments->mark();
    id_caller->mark();
    id_this->mark();
    id___proto__->mark();
    id_enumerable->mark();
    id_configurable->mark();
    id_writable->mark();
    id_value->mark();
    id_get->mark();
    id_set->mark();
    id_eval->mark();
    id_uintMax->mark();
    id_name->mark();

    objectCtor.mark();
    stringCtor.mark();
    numberCtor.mark();
    booleanCtor.mark();
    arrayCtor.mark();
    functionCtor.mark();
    dateCtor.mark();
    regExpCtor.mark();
    errorCtor.mark();
    evalErrorCtor.mark();
    rangeErrorCtor.mark();
    referenceErrorCtor.mark();
    syntaxErrorCtor.mark();
    typeErrorCtor.mark();
    uRIErrorCtor.mark();

    objectPrototype->mark();
    stringPrototype->mark();
    numberPrototype->mark();
    booleanPrototype->mark();
    arrayPrototype->mark();
    functionPrototype->mark();
    datePrototype->mark();
    regExpPrototype->mark();
    errorPrototype->mark();
    evalErrorPrototype->mark();
    rangeErrorPrototype->mark();
    referenceErrorPrototype->mark();
    syntaxErrorPrototype->mark();
    typeErrorPrototype->mark();
    uRIErrorPrototype->mark();

    variantPrototype->mark();
    sequencePrototype->mark();

    if (m_qmlExtensions)
        m_qmlExtensions->markObjects();
}

namespace {
    bool functionSortHelper(Function *lhs, Function *rhs)
    {
        return reinterpret_cast<quintptr>(lhs->code) < reinterpret_cast<quintptr>(rhs->code);
    }

    struct FindHelper
    {
        bool operator()(Function *function, quintptr pc)
        {
            return reinterpret_cast<quintptr>(function->code) < pc
                   && (reinterpret_cast<quintptr>(function->code) + function->codeSize) < pc;
        }

        bool operator()(quintptr pc, Function *function)
        {
            return pc < reinterpret_cast<quintptr>(function->code);
        }
    };
}

Function *ExecutionEngine::functionForProgramCounter(quintptr pc) const
{
    if (functionsNeedSort) {
        qSort(functions.begin(), functions.end(), functionSortHelper);
        functionsNeedSort = false;
    }

    QVector<Function*>::ConstIterator it = qBinaryFind(functions.constBegin(), functions.constEnd(),
            pc, FindHelper());
    if (it != functions.constEnd())
        return *it;
    return 0;
}

QmlExtensions *ExecutionEngine::qmlExtensions()
{
    if (!m_qmlExtensions)
        m_qmlExtensions = new QmlExtensions;
    return m_qmlExtensions;
}

QT_END_NAMESPACE
