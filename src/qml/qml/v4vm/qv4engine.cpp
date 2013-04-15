/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the V4VM module of the Qt Toolkit.
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
#include <qv4mathobject_p.h>
#include <qv4numberobject_p.h>
#include <qv4regexpobject_p.h>
#include <qv4runtime_p.h>
#include "qv4mm_p.h"
#include <qv4argumentsobject_p.h>
#include <qv4dateobject_p.h>
#include <qv4jsonobject_p.h>
#include <qv4stringobject_p.h>
#include <qv4identifier_p.h>
#include <qv4unwindhelper_p.h>
#include "qv4isel_masm_p.h"
#include "qv4debugging_p.h"
#include "qv4executableallocator_p.h"

namespace QQmlJS {
namespace VM {

ExecutionEngine::ExecutionEngine(EvalISelFactory *factory)
    : memoryManager(new QQmlJS::VM::MemoryManager)
    , executableAllocator(new QQmlJS::VM::ExecutableAllocator)
    , debugger(0)
    , globalObject(0)
    , globalCode(0)
    , externalResourceComparison(0)
    , regExpCache(0)
{
    MemoryManager::GCBlocker gcBlocker(memoryManager);

    if (!factory)
        factory = new MASM::ISelFactory;
    iselFactory.reset(factory);

    memoryManager->setExecutionEngine(this);

    identifierCache = new Identifiers(this);

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
    id_this = newIdentifier(QStringLiteral("this"));
    id___proto__ = newIdentifier(QStringLiteral("__proto__"));
    id_enumerable = newIdentifier(QStringLiteral("enumerable"));
    id_configurable = newIdentifier(QStringLiteral("configurable"));
    id_writable = newIdentifier(QStringLiteral("writable"));
    id_value = newIdentifier(QStringLiteral("value"));
    id_get = newIdentifier(QStringLiteral("get"));
    id_set = newIdentifier(QStringLiteral("set"));
    id_eval = newIdentifier(QStringLiteral("eval"));

    emptyClass = new InternalClass(this);
    arrayClass = emptyClass->addMember(id_length, Attr_NotConfigurable|Attr_NotEnumerable);
    initRootContext();

    objectPrototype = new (memoryManager) ObjectPrototype(this);
    stringPrototype = new (memoryManager) StringPrototype(rootContext);
    numberPrototype = new (memoryManager) NumberPrototype(this);
    booleanPrototype = new (memoryManager) BooleanPrototype(this);
    arrayPrototype = new (memoryManager) ArrayPrototype(rootContext);
    datePrototype = new (memoryManager) DatePrototype(this);
    functionPrototype = new (memoryManager) FunctionPrototype(rootContext);
    regExpPrototype = new (memoryManager) RegExpPrototype(this);
    errorPrototype = new (memoryManager) ErrorPrototype(rootContext);
    evalErrorPrototype = new (memoryManager) EvalErrorPrototype(rootContext);
    rangeErrorPrototype = new (memoryManager) RangeErrorPrototype(rootContext);
    referenceErrorPrototype = new (memoryManager) ReferenceErrorPrototype(rootContext);
    syntaxErrorPrototype = new (memoryManager) SyntaxErrorPrototype(rootContext);
    typeErrorPrototype = new (memoryManager) TypeErrorPrototype(rootContext);
    uRIErrorPrototype = new (memoryManager) URIErrorPrototype(rootContext);

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
    stringPrototype->init(rootContext, stringCtor);
    numberPrototype->init(rootContext, numberCtor);
    booleanPrototype->init(rootContext, booleanCtor);
    arrayPrototype->init(rootContext, arrayCtor);
    datePrototype->init(rootContext, dateCtor);
    functionPrototype->init(rootContext, functionCtor);
    regExpPrototype->init(rootContext, regExpCtor);
    errorPrototype->init(rootContext, errorCtor);
    evalErrorPrototype->init(rootContext, evalErrorCtor);
    rangeErrorPrototype->init(rootContext, rangeErrorCtor);
    referenceErrorPrototype->init(rootContext, referenceErrorCtor);
    syntaxErrorPrototype->init(rootContext, syntaxErrorCtor);
    typeErrorPrototype->init(rootContext, typeErrorCtor);
    uRIErrorPrototype->init(rootContext, uRIErrorCtor);

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
    delete regExpCache;
    UnwindHelper::deregisterFunctions(functions);
    qDeleteAll(functions);
    delete memoryManager;
    delete executableAllocator;
}

void ExecutionEngine::initRootContext()
{
    rootContext = static_cast<GlobalContext *>(memoryManager->allocContext(sizeof(GlobalContext)));
    current = rootContext;
    current->parent = 0;
    rootContext->init(this);
}

WithContext *ExecutionEngine::newWithContext(Object *with)
{
    ExecutionContext *p = current;
    WithContext *w = static_cast<WithContext *>(memoryManager->allocContext(sizeof(WithContext)));
    w->parent = current;
    current = w;

    w->init(p, with);
    return w;
}

CatchContext *ExecutionEngine::newCatchContext(String *exceptionVarName, const Value &exceptionValue)
{
    ExecutionContext *p = current;
    CatchContext *c = static_cast<CatchContext *>(memoryManager->allocContext(sizeof(CatchContext)));
    c->parent = current;
    current = c;

    c->init(p, exceptionVarName, exceptionValue);
    return c;
}

CallContext *ExecutionEngine::newCallContext(FunctionObject *f, const Value &thisObject, Value *args, int argc)
{
    CallContext *c = static_cast<CallContext *>(memoryManager->allocContext(requiredMemoryForExecutionContect(f, argc)));
    c->parent = current;
    current = c;

    c->function = f;
    c->thisObject = thisObject;
    c->arguments = args;
    c->argumentCount = argc;
    c->initCallContext(this);

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
    c->parent = current;
    current = c;

    c->function = f;
    c->thisObject = thisObject;
    c->arguments = args;
    c->argumentCount = argc;
    c->initCallContext(this);

    return c;
}


ExecutionContext *ExecutionEngine::pushGlobalContext()
{
    GlobalContext *g = static_cast<GlobalContext *>(memoryManager->allocContext(sizeof(GlobalContext)));
    *g = *rootContext;
    g->parent = current;
    current = g;

    return current;
}

Function *ExecutionEngine::newFunction(const QString &name)
{
    VM::Function *f = new VM::Function(newIdentifier(name));
    functions.append(f);
    return f;
}

FunctionObject *ExecutionEngine::newBuiltinFunction(ExecutionContext *scope, String *name, Value (*code)(SimpleCallContext *))
{
    BuiltinFunctionOld *f = new (memoryManager) BuiltinFunctionOld(scope, name, code);
    return f;
}

FunctionObject *ExecutionEngine::newScriptFunction(ExecutionContext *scope, VM::Function *function)
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

String *ExecutionEngine::newString(const QString &s)
{
    return new (memoryManager) String(s);
}

String *ExecutionEngine::newIdentifier(const QString &text)
{
    return identifierCache->insert(text);
}

Object *ExecutionEngine::newStringObject(ExecutionContext *ctx, const Value &value)
{
    StringObject *object = new (memoryManager) StringObject(ctx, value);
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

Object *ExecutionEngine::newFunctionObject(ExecutionContext *ctx)
{
    Object *object = new (memoryManager) FunctionObject(ctx);
    object->prototype = functionPrototype;
    return object;
}

ArrayObject *ExecutionEngine::newArrayObject(ExecutionContext *ctx)
{
    ArrayObject *object = new (memoryManager) ArrayObject(ctx);
    object->prototype = arrayPrototype;
    return object;
}

Object *ExecutionEngine::newDateObject(const Value &value)
{
    Object *object = new (memoryManager) DateObject(this, value);
    object->prototype = datePrototype;
    return object;
}

RegExpObject *ExecutionEngine::newRegExpObject(const QString &pattern, int flags)
{
    bool global = (flags & V4IR::RegExp::RegExp_Global);
    bool ignoreCase = false;
    bool multiline = false;
    if (flags & V4IR::RegExp::RegExp_IgnoreCase)
        ignoreCase = true;
    if (flags & V4IR::RegExp::RegExp_Multiline)
        multiline = true;

    return newRegExpObject(RegExp::create(this, pattern, ignoreCase, multiline), global);
}

RegExpObject *ExecutionEngine::newRegExpObject(RegExp* re, bool global)
{
    RegExpObject *object = new (memoryManager) RegExpObject(this, re, global);
    object->prototype = regExpPrototype;
    return object;
}

Object *ExecutionEngine::newErrorObject(const Value &value)
{
    ErrorObject *object = new (memoryManager) ErrorObject(rootContext, value);
    object->prototype = errorPrototype;
    return object;
}

Object *ExecutionEngine::newSyntaxErrorObject(ExecutionContext *ctx, DiagnosticMessage *message)
{
    return new (memoryManager) SyntaxErrorObject(ctx, message);
}

Object *ExecutionEngine::newReferenceErrorObject(ExecutionContext *ctx, const QString &message)
{
    return new (memoryManager) ReferenceErrorObject(ctx, message);
}

Object *ExecutionEngine::newTypeErrorObject(ExecutionContext *ctx, const QString &message)
{
    return new (memoryManager) TypeErrorObject(ctx, message);
}

Object *ExecutionEngine::newRangeErrorObject(ExecutionContext *ctx, const QString &message)
{
    return new (memoryManager) RangeErrorObject(ctx, message);
}

Object *ExecutionEngine::newURIErrorObject(ExecutionContext *ctx, Value message)
{
    return new (memoryManager) URIErrorObject(ctx, message);
}

Object *ExecutionEngine::newForEachIteratorObject(ExecutionContext *ctx, Object *o)
{
    return new (memoryManager) ForEachIteratorObject(ctx, o);
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
    identifierCache->mark();

    globalObject->mark();

    if (globalCode)
        globalCode->mark();

    for (int i = 0; i < argumentsAccessors.size(); ++i) {
        const Property &pd = argumentsAccessors.at(i);
        pd.getter()->mark();
        pd.setter()->mark();
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
}

Value ExecutionEngine::run(Function *function, ExecutionContext *ctx)
{
    if (!ctx)
        ctx = rootContext;

    TemporaryAssignment<Function*>(globalCode, function);

    // ### Would be better to have a SavedExecutionState object that
    // saves this and restores it in the destructor (to survive an exception).
    ctx->strictMode = function->isStrict;
    ctx->lookups = function->lookups;

    if (debugger)
        debugger->aboutToCall(0, ctx);
    QQmlJS::VM::Value result = function->code(ctx, function->codeData);
    if (debugger)
        debugger->justLeft(ctx);
    return result;
}

} // namespace VM
} // namespace QQmlJS
