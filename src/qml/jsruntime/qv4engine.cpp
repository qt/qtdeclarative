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
    , jsStack(new WTF::PageAllocation)
    , debugger(0)
    , globalObject(0)
    , globalCode(0)
    , m_engineId(engineSerial.fetchAndAddOrdered(1))
    , regExpCache(0)
    , m_multiplyWrappedQObjects(0)
    , m_qmlExtensions(0)
{
    MemoryManager::GCBlocker gcBlocker(memoryManager);

    if (!factory) {

#ifdef V4_ENABLE_JIT
        static const bool forceMoth = !qgetenv("QV4_FORCE_INTERPRETER").isEmpty();
        if (forceMoth)
            factory = new QQmlJS::Moth::ISelFactory;
        else
            factory = new QQmlJS::MASM::ISelFactory;
#else // !V4_ENABLE_JIT
        factory = new QQmlJS::Moth::ISelFactory;
#endif // V4_ENABLE_JIT
    }
    iselFactory.reset(factory);

    memoryManager->setExecutionEngine(this);

    // reserve 8MB for the JS stack
    *jsStack = WTF::PageAllocation::allocate(8*1024*1024, WTF::OSAllocator::JSVMStackPages, true);
    jsStackBase = (Value *)jsStack->base();
    jsStackTop = jsStackBase;

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
    id_index = newIdentifier(QStringLiteral("index"));
    id_input = newIdentifier(QStringLiteral("input"));

    ObjectPrototype *objectPrototype = new (memoryManager) ObjectPrototype(emptyClass);
    objectClass = emptyClass->changePrototype(objectPrototype);

    arrayClass = objectClass->addMember(id_length, Attr_NotConfigurable|Attr_NotEnumerable);
    ArrayPrototype *arrayPrototype = new (memoryManager) ArrayPrototype(arrayClass);
    arrayClass = arrayClass->changePrototype(arrayPrototype);

    InternalClass *argsClass = objectClass->addMember(id_length, Attr_NotEnumerable);
    argumentsObjectClass = argsClass->addMember(id_callee, Attr_Data|Attr_NotEnumerable);
    strictArgumentsObjectClass = argsClass->addMember(id_callee, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    strictArgumentsObjectClass = strictArgumentsObjectClass->addMember(id_caller, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    initRootContext();

    StringPrototype *stringPrototype = new (memoryManager) StringPrototype(objectClass);
    stringClass = emptyClass->changePrototype(stringPrototype);

    NumberPrototype *numberPrototype = new (memoryManager) NumberPrototype(objectClass);
    numberClass = emptyClass->changePrototype(numberPrototype);

    BooleanPrototype *booleanPrototype = new (memoryManager) BooleanPrototype(objectClass);
    booleanClass = emptyClass->changePrototype(booleanPrototype);

    DatePrototype *datePrototype = new (memoryManager) DatePrototype(objectClass);
    dateClass = emptyClass->changePrototype(datePrototype);

    FunctionPrototype *functionPrototype = new (memoryManager) FunctionPrototype(objectClass);
    functionClass = emptyClass->changePrototype(functionPrototype);
    uint index;
    functionWithProtoClass = functionClass->addMember(id_prototype, Attr_NotEnumerable|Attr_NotConfigurable, &index);
    Q_ASSERT(index == FunctionObject::Index_Prototype);
    protoClass = objectClass->addMember(id_constructor, Attr_NotEnumerable, &index);
    Q_ASSERT(index == FunctionObject::Index_ProtoConstructor);

    RegExpPrototype *regExpPrototype = new (memoryManager) RegExpPrototype(objectClass);
    regExpClass = emptyClass->changePrototype(regExpPrototype);
    regExpExecArrayClass = arrayClass->addMember(id_index, Attr_Data, &index);
    Q_ASSERT(index == RegExpObject::Index_ArrayIndex);
    regExpExecArrayClass = regExpExecArrayClass->addMember(id_input, Attr_Data, &index);
    Q_ASSERT(index == RegExpObject::Index_ArrayInput);

    ErrorPrototype *errorPrototype = new (memoryManager) ErrorPrototype(objectClass);
    errorClass = emptyClass->changePrototype(errorPrototype);
    EvalErrorPrototype *evalErrorPrototype = new (memoryManager) EvalErrorPrototype(errorClass);
    evalErrorClass = emptyClass->changePrototype(evalErrorPrototype);
    RangeErrorPrototype *rangeErrorPrototype = new (memoryManager) RangeErrorPrototype(errorClass);
    rangeErrorClass = emptyClass->changePrototype(rangeErrorPrototype);
    ReferenceErrorPrototype *referenceErrorPrototype = new (memoryManager) ReferenceErrorPrototype(errorClass);
    referenceErrorClass = emptyClass->changePrototype(referenceErrorPrototype);
    SyntaxErrorPrototype *syntaxErrorPrototype = new (memoryManager) SyntaxErrorPrototype(errorClass);
    syntaxErrorClass = emptyClass->changePrototype(syntaxErrorPrototype);
    TypeErrorPrototype *typeErrorPrototype = new (memoryManager) TypeErrorPrototype(errorClass);
    typeErrorClass = emptyClass->changePrototype(typeErrorPrototype);
    URIErrorPrototype *uRIErrorPrototype = new (memoryManager) URIErrorPrototype(errorClass);
    uriErrorClass = emptyClass->changePrototype(uRIErrorPrototype);

    VariantPrototype *variantPrototype = new (memoryManager) VariantPrototype(objectClass);
    variantClass = emptyClass->changePrototype(variantPrototype);

    SequencePrototype *sequencePrototype = new (memoryManager) SequencePrototype(arrayClass->changePrototype(arrayPrototype));
    sequenceClass = emptyClass->changePrototype(sequencePrototype);

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
    globalObject = newObject();
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

    QSet<QV4::CompiledData::CompilationUnit*> remainingUnits;
    qSwap(compilationUnits, remainingUnits);
    foreach (QV4::CompiledData::CompilationUnit *unit, remainingUnits)
        unit->unlink();

    delete m_qmlExtensions;
    emptyClass->destroy();
    delete bumperPointerAllocator;
    delete regExpCache;
    delete regExpAllocator;
    delete executableAllocator;
    jsStack->deallocate();
    delete jsStack;
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

FunctionObject *ExecutionEngine::newBuiltinFunction(ExecutionContext *scope, String *name, ReturnedValue (*code)(SimpleCallContext *))
{
    BuiltinFunction *f = new (memoryManager) BuiltinFunction(scope, name, code);
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
    return object;
}

Object *ExecutionEngine::newObject(InternalClass *internalClass)
{
    Object *object = new (memoryManager) Object(internalClass);
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

Returned<Object> *ExecutionEngine::newStringObject(const Value &value)
{
    StringObject *object = new (memoryManager) StringObject(this, value);
    return object->asReturned<Object>();
}

Returned<Object> *ExecutionEngine::newNumberObject(const Value &value)
{
    NumberObject *object = new (memoryManager) NumberObject(this, value);
    return object->asReturned<Object>();
}

Returned<Object> *ExecutionEngine::newBooleanObject(const Value &value)
{
    Object *object = new (memoryManager) BooleanObject(this, value);
    return object->asReturned<Object>();
}

Returned<ArrayObject> *ExecutionEngine::newArrayObject(int count)
{
    ArrayObject *object = new (memoryManager) ArrayObject(this);

    if (count) {
        if (count < 0x1000)
            object->arrayReserve(count);
        object->setArrayLengthUnchecked(count);
    }
    return object->asReturned<ArrayObject>();
}

Returned<ArrayObject> *ExecutionEngine::newArrayObject(const QStringList &list)
{
    ArrayObject *object = new (memoryManager) ArrayObject(this, list);
    return object->asReturned<ArrayObject>();
}

Returned<ArrayObject> *ExecutionEngine::newArrayObject(InternalClass *ic)
{
    ArrayObject *object = new (memoryManager) ArrayObject(ic);
    return object->asReturned<ArrayObject>();
}


Returned<DateObject> *ExecutionEngine::newDateObject(const Value &value)
{
    DateObject *object = new (memoryManager) DateObject(this, value);
    return object->asReturned<DateObject>();
}

Returned<DateObject> *ExecutionEngine::newDateObject(const QDateTime &dt)
{
    DateObject *object = new (memoryManager) DateObject(this, dt);
    return object->asReturned<DateObject>();
}

Returned<RegExpObject> *ExecutionEngine::newRegExpObject(const QString &pattern, int flags)
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

Returned<RegExpObject> *ExecutionEngine::newRegExpObject(RegExp* re, bool global)
{
    RegExpObject *object = new (memoryManager) RegExpObject(this, re, global);
    return object->asReturned<RegExpObject>();
}

Returned<RegExpObject> *ExecutionEngine::newRegExpObject(const QRegExp &re)
{
    RegExpObject *object = new (memoryManager) RegExpObject(this, re);
    return object->asReturned<RegExpObject>();
}

Returned<Object> *ExecutionEngine::newErrorObject(const Value &value)
{
    ErrorObject *object = new (memoryManager) ErrorObject(errorClass, value);
    return object->asReturned<Object>();
}

Returned<Object> *ExecutionEngine::newSyntaxErrorObject(const QString &message)
{
    Object *error = new (memoryManager) SyntaxErrorObject(this, Value::fromString(this, message));
    return error->asReturned<Object>();
}

Returned<Object> *ExecutionEngine::newSyntaxErrorObject(const QString &message, const QString &fileName, int line, int column)
{
    Object *error = new (memoryManager) SyntaxErrorObject(this, message, fileName, line, column);
    return error->asReturned<Object>();
}


Returned<Object> *ExecutionEngine::newReferenceErrorObject(const QString &message)
{
    Object *o = new (memoryManager) ReferenceErrorObject(this, message);
    return o->asReturned<Object>();
}

Returned<Object> *ExecutionEngine::newReferenceErrorObject(const QString &message, const QString &fileName, int lineNumber, int columnNumber)
{
    Object *o = new (memoryManager) ReferenceErrorObject(this, message, fileName, lineNumber, columnNumber);
    return o->asReturned<Object>();
}


Returned<Object> *ExecutionEngine::newTypeErrorObject(const QString &message)
{
    Object *o = new (memoryManager) TypeErrorObject(this, message);
    return o->asReturned<Object>();
}

Returned<Object> *ExecutionEngine::newRangeErrorObject(const QString &message)
{
    Object *o = new (memoryManager) RangeErrorObject(this, message);
    return o->asReturned<Object>();
}

Returned<Object> *ExecutionEngine::newURIErrorObject(Value message)
{
    Object *o = new (memoryManager) URIErrorObject(this, message);
    return o->asReturned<Object>();
}

Returned<Object> *ExecutionEngine::newVariantObject(const QVariant &v)
{
    Object *o = new (memoryManager) VariantObject(this, v);
    return o->asReturned<Object>();
}

Returned<Object> *ExecutionEngine::newForEachIteratorObject(ExecutionContext *ctx, Object *o)
{
    Object *obj = new (memoryManager) ForEachIteratorObject(ctx, o);
    return obj->asReturned<Object>();
}

Returned<Object> *ExecutionEngine::qmlContextObject() const
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

    return static_cast<CallContext *>(ctx)->activation->asReturned<Object>();
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
                frame.source = callCtx->function->function->sourceFile();
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
        frame.source = globalCode->sourceFile();
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
                base.setUrl(callCtx->function->function->sourceFile());
            break;
        }
        c = c->parent;
    }

    if (base.isEmpty() && globalCode)
        base.setUrl(globalCode->sourceFile());

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
        FunctionObject *set = new (memoryManager) ArgumentsSetterFunction(rootContext, i);
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
    id_index->mark();
    id_input->mark();

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

    if (m_qmlExtensions)
        m_qmlExtensions->markObjects();

    emptyClass->markObjects();

    for (QSet<CompiledData::CompilationUnit*>::ConstIterator it = compilationUnits.constBegin(), end = compilationUnits.constEnd();
         it != end; ++it)
        (*it)->markObjects();
}

namespace {
    struct FindHelper
    {
        bool operator()(Function *function, quintptr pc)
        {
            return reinterpret_cast<quintptr>(function->codePtr) < pc
                   && (reinterpret_cast<quintptr>(function->codePtr) + function->codeSize) < pc;
        }

        bool operator()(quintptr pc, Function *function)
        {
            return pc < reinterpret_cast<quintptr>(function->codePtr);
        }
    };
}

Function *ExecutionEngine::functionForProgramCounter(quintptr pc) const
{
    // ### Use this code path instead of the "else" when the number of compilation units went down to
    // one per (qml) file.
#if 0
    for (QSet<QV4::CompiledData::CompilationUnit*>::ConstIterator unitIt = compilationUnits.constBegin(), unitEnd = compilationUnits.constEnd();
         unitIt != unitEnd; ++unitIt) {
        const QVector<Function*> &functions = (*unitIt)->runtimeFunctionsSortedByAddress;
        QVector<Function*>::ConstIterator it = qBinaryFind(functions.constBegin(),
                                                           functions.constEnd(),
                                                           pc, FindHelper());
        if (it != functions.constEnd())
            return *it;
    }
    return 0;
#else
    QMap<quintptr, Function*>::ConstIterator it = allFunctions.lowerBound(pc);
    if (it != allFunctions.begin() && allFunctions.count() > 0)
        --it;
    if (it == allFunctions.end())
        return 0;

    if (pc < it.key() || pc >= it.key() + (*it)->codeSize)
        return 0;
    return *it;
#endif
}

QmlExtensions *ExecutionEngine::qmlExtensions()
{
    if (!m_qmlExtensions)
        m_qmlExtensions = new QmlExtensions;
    return m_qmlExtensions;
}

QT_END_NAMESPACE
