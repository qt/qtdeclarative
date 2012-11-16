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


#include "qmljs_objects.h"
#include "qv4ir_p.h"
#include "qv4ecmaobjects_p.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4ir_p.h>
#include <qv4codegen_p.h>
#include <qv4isel_masm_p.h>

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>
#include <typeinfo>
#include <iostream>

using namespace QQmlJS::VM;

//
// Object
//
Object::~Object()
{
    delete members;
}

void Object::__put__(ExecutionContext *ctx, const QString &name, const Value &value)
{
    __put__(ctx, ctx->engine->identifier(name), value);
}

void Object::__put__(ExecutionContext *ctx, const QString &name, void (*code)(ExecutionContext *), int count)
{
    Q_UNUSED(count);
    __put__(ctx, name, Value::fromObject(ctx->engine->newNativeFunction(ctx, code)));
}

Value Object::getValue(ExecutionContext *ctx, PropertyDescriptor *p) const
{
    if (p->isData())
        return p->value;
    if (!p->get)
        return Value::undefinedValue();

    p->get->call(ctx, Value::fromObject(const_cast<Object *>(this)), 0, 0);
    return ctx->result;
}

bool Object::inplaceBinOp(Value rhs, String *name, BinOp op, ExecutionContext *ctx)
{
    PropertyDescriptor to_fill;
    PropertyDescriptor *pd = __getPropertyDescriptor__(ctx, name, &to_fill);
    if (!pd)
        return false;
    Value result = op(getValue(ctx, pd), rhs, ctx);
    __put__(ctx, name, result);
    return true;
}

bool Object::inplaceBinOp(Value rhs, Value index, BinOp op, ExecutionContext *ctx)
{
    String *name = index.toString(ctx);
    assert(name);
    return inplaceBinOp(rhs, name, op, ctx);
}

// Section 8.12.1
PropertyDescriptor *Object::__getOwnProperty__(ExecutionContext *, String *name)
{
    if (members)
        return members->find(name);
    return 0;
}

// Section 8.12.2
PropertyDescriptor *Object::__getPropertyDescriptor__(ExecutionContext *ctx, String *name, PropertyDescriptor *to_fill)
{
    if (PropertyDescriptor *p = __getOwnProperty__(ctx, name))
        return p;

    if (prototype)
        return prototype->__getPropertyDescriptor__(ctx, name, to_fill);
    return 0;
}

// Section 8.12.3
Value Object::__get__(ExecutionContext *ctx, String *name)
{
    if (name->isEqualTo(ctx->engine->id___proto__))
        return Value::fromObject(prototype);

    PropertyDescriptor tmp;
    if (PropertyDescriptor *p = __getPropertyDescriptor__(ctx, name, &tmp))
        return getValue(ctx, p);

    return Value::undefinedValue();
}

// Section 8.12.4
bool Object::__canPut__(ExecutionContext *ctx, String *name)
{
    if (PropertyDescriptor *p = __getOwnProperty__(ctx, name)) {
        if (p->isAccessor())
            return p->get != 0;
        return p->isWritable();
    }

    if (! prototype)
        return extensible;

    PropertyDescriptor tmp;
    if (PropertyDescriptor *p = prototype->__getPropertyDescriptor__(ctx, name, &tmp)) {
        if (p->isAccessor())
            return p->get != 0;
        if (!extensible)
            return false;
        return p->isWritable();
    } else {
        return extensible;
    }
    return true;
}

// Section 8.12.5
void Object::__put__(ExecutionContext *ctx, String *name, const Value &value, bool throwException)
{
    // clause 1
    if (!__canPut__(ctx, name))
        goto reject;

    if (!members)
        members = new PropertyTable();

    {
        // Clause 2
        PropertyDescriptor *pd = __getOwnProperty__(ctx, name);
        // Clause 3
        if (pd && pd->isData()) {
            // spec says to call [[DefineOwnProperty]] with { [[Value]]: value }

            // ### to simplify and speed up we should expand the relevant parts here (clauses 6,7,9,10,12,13)
            PropertyDescriptor desc = PropertyDescriptor::fromValue(value);
            __defineOwnProperty__(ctx, name, &desc, throwException);
            return;
        }

        // clause 4
        PropertyDescriptor tmp;
        if (prototype)
            pd = prototype->__getPropertyDescriptor__(ctx, name, &tmp);

        // Clause 5
        if (pd && pd->isAccessor()) {
            assert(pd->set != 0);

            Value args[1];
            args[0] = value;
            pd->set->call(ctx, Value::fromObject(this), args, 1);
            return;
        }

        PropertyDescriptor *p = members->insert(name);
        *p = PropertyDescriptor::fromValue(value);
        p->configurable = PropertyDescriptor::Set;
        p->enumberable = PropertyDescriptor::Set;
        p->writable = PropertyDescriptor::Set;
    }

  reject:
    if (throwException)
        __qmljs_throw_type_error(ctx);
}

// Section 8.12.6
bool Object::__hasProperty__(ExecutionContext *ctx, String *name) const
{
    if (members)
        return members->find(name) != 0;

    return prototype ? prototype->__hasProperty__(ctx, name) : false;
}

// Section 8.12.7
bool Object::__delete__(ExecutionContext *ctx, String *name, bool throwException)
{
    if (members) {
        if (PropertyTableEntry *entry = members->findEntry(name)) {
            if (entry->descriptor.isConfigurable()) {
                members->remove(entry);
                return true;
            }
            if (throwException)
                __qmljs_throw_type_error(ctx);
            return false;
        }
    }
    return true;
}

// Section 8.12.9
bool Object::__defineOwnProperty__(ExecutionContext *ctx, String *name, PropertyDescriptor *desc, bool throwException)
{
    if (!members)
        members = new PropertyTable();

    // Clause 1
    PropertyDescriptor *current = __getOwnProperty__(ctx, name);
    if (!current) {
        // clause 3
        if (!extensible)
            goto reject;
        // clause 4
        *current = *desc;
        current->fullyPopulated();
        return true;
    }

    // clause 5
    if (desc->isEmpty())
        return true;

    // clause 6
    if (desc->isSubset(current))
        return true;

    // clause 7
    if (!current->isConfigurable()) {
        if (desc->isConfigurable())
            goto reject;
        if (desc->enumberable != PropertyDescriptor::Unset && desc->enumberable != current->enumberable)
            goto reject;
    }

    // clause 8
    if (desc->isGeneric())
        goto accept;

    // clause 9
    if (current->isData() != desc->isData()) {
        // 9a
        if (!current->isConfigurable())
            goto reject;
        if (current->isData()) {
            // 9b
            current->type = PropertyDescriptor::Accessor;
            current->writable = PropertyDescriptor::Undefined;
            current->get = 0;
            current->set = 0;
        } else {
            // 9c
            current->type = PropertyDescriptor::Data;
            current->writable = PropertyDescriptor::Unset;
            current->value = Value::undefinedValue();
        }
    } else if (current->isData() && desc->isData()) { // clause 10
        if (!current->isConfigurable() && !current->isWritable()) {
            if (desc->isWritable() || !current->value.sameValue(desc->value))
                goto reject;
        }
    } else { // clause 10
        assert(current->isAccessor() && desc->isAccessor());
        if (!current->isConfigurable()) {
            if (current->get != desc->get || current->set != desc->set)
                goto reject;
        }
    }

  accept:

    *current += *desc;
    return true;
  reject:
    if (throwException)
        __qmljs_throw_type_error(ctx);
    return false;
}

String *ForEachIteratorObject::nextPropertyName()
{
    PropertyTableEntry *p = 0;
    while (1) {
        if (!current)
            return 0;

        // ### index array data as well
        ++tableIndex;
        if (!current->members || tableIndex > current->members->_propertyCount) {
            current = current->prototype;
            tableIndex = -1;
            continue;
        }
        p = current->members->_properties[tableIndex];
        // ### check that it's not a repeated attribute
        if (p && p->descriptor.isEnumerable())
            return p->name;
    }
}

Value ArrayObject::__get__(ExecutionContext *ctx, String *name)
{
    if (name->isEqualTo(ctx->engine->id_length))
        return Value::fromDouble(value.size());
    return Object::__get__(ctx, name);
}

bool ArrayObject::inplaceBinOp(Value rhs, Value index, BinOp op, ExecutionContext *ctx)
{
    if (index.isNumber()) {
        const quint32 idx = index.toUInt32(ctx);
        Value v = value.at(idx);
        v = op(v, rhs, ctx);
        value.assign(idx, v);
        return true;
    }
    return Object::inplaceBinOp(rhs, index, op, ctx);
}

bool FunctionObject::hasInstance(ExecutionContext *ctx, const Value &value)
{
    if (! value.isObject()) {
        ctx->throwTypeError();
        return false;
    }

    Value o = __get__(ctx, ctx->engine->id_prototype);
    if (! o.isObject()) {
        ctx->throwTypeError();
        return false;
    }

    Object *v = value.objectValue();
    while (v) {
        v = v->prototype;

        if (! v)
            break;
        else if (o.objectValue() == v)
            return true;
    }

    return false;
}

Value FunctionObject::construct(ExecutionContext *context, Value *args, int argc)
{
    ExecutionContext k;
    ExecutionContext *ctx = needsActivation ? context->engine->newContext() : &k;
    ctx->initConstructorContext(context, Value::nullValue(), this, args, argc);
    construct(ctx);
    ctx->leaveConstructorContext(this);
    return ctx->result;
}

Value FunctionObject::call(ExecutionContext *context, Value thisObject, Value *args, int argc, bool strictMode)
{
    ExecutionContext k;
    ExecutionContext *ctx = needsActivation ? context->engine->newContext() : &k;

    if (!strictMode && !thisObject.isObject()) {
        if (thisObject.isUndefined() || thisObject.isNull())
            thisObject = context->engine->globalObject;
        else
            thisObject = __qmljs_to_object(thisObject, context);
    }
    ctx->initCallContext(context, thisObject, this, args, argc);
    call(ctx);
    ctx->leaveCallContext();
    return ctx->result;
}

void FunctionObject::call(ExecutionContext *ctx)
{
    Q_UNUSED(ctx);
}

void FunctionObject::construct(ExecutionContext *ctx)
{
    ctx->thisObject = Value::fromObject(ctx->engine->newObject());
    call(ctx);
}

ScriptFunction::ScriptFunction(ExecutionContext *scope, IR::Function *function)
    : FunctionObject(scope)
    , function(function)
{
    if (function->name)
        name = scope->engine->identifier(*function->name);
    needsActivation = function->needsActivation();
    formalParameterCount = function->formals.size();
    if (formalParameterCount) {
        formalParameterList = new String*[formalParameterCount];
        for (unsigned int i = 0; i < formalParameterCount; ++i) {
            formalParameterList[i] = scope->engine->identifier(*function->formals.at(i));
        }
    }

    varCount = function->locals.size();
    if (varCount) {
        varList = new String*[varCount];
        for (unsigned int i = 0; i < varCount; ++i) {
            varList[i] = scope->engine->identifier(*function->locals.at(i));
        }
    }
}

ScriptFunction::~ScriptFunction()
{
    delete[] formalParameterList;
    delete[] varList;
}

void ScriptFunction::call(VM::ExecutionContext *ctx)
{
    function->code(ctx, function->codeData);
}


Value EvalFunction::call(ExecutionContext *context, Value thisObject, Value *args, int argc, bool strictMode)
{
    Value s = context->argument(0);
    if (!s.isString()) {
        context->result = s;
        return s;
    }
    const QString code = context->argument(0).stringValue()->toQString();

    // ### how to determine this correctly
    bool directCall = true;

    ExecutionContext k, *ctx;
    if (!directCall) {
        // ###
    } else if (strictMode) {
        ctx = &k;
        ctx->initCallContext(context, context->thisObject, this, args, argc);
    } else {
        ctx = context;
    }
    // ##### inline and do this in the correct scope
    evaluate(ctx, QStringLiteral("eval code"), code, /*useInterpreter*/ false, QQmlJS::Codegen::EvalCode);

    if (strictMode)
        ctx->leaveCallContext();
}

static inline bool protect(const void *addr, size_t size)
{
    size_t pageSize = sysconf(_SC_PAGESIZE);
    size_t iaddr = reinterpret_cast<size_t>(addr);
    size_t roundAddr = iaddr & ~(pageSize - static_cast<size_t>(1));
    int mode = PROT_READ | PROT_WRITE | PROT_EXEC;
    return mprotect(reinterpret_cast<void*>(roundAddr), size + (iaddr - roundAddr), mode) == 0;
}


int EvalFunction::evaluate(QQmlJS::VM::ExecutionContext *ctx, const QString &fileName,
                           const QString &source, bool useInterpreter,
                           QQmlJS::Codegen::Mode mode)
{
    using namespace QQmlJS;

    VM::ExecutionEngine *vm = ctx->engine;
    IR::Module module;
    IR::Function *globalCode = 0;

    const size_t codeSize = 400 * getpagesize();
    uchar *code = 0;
    if (posix_memalign((void**)&code, 16, codeSize))
        assert(!"memalign failed");
    assert(code);
    assert(! (size_t(code) & 15));

    {
        QQmlJS::Engine ee, *engine = &ee;
        Lexer lexer(engine);
        lexer.setCode(source, 1, false);
        Parser parser(engine);

        const bool parsed = parser.parseProgram();

        foreach (const DiagnosticMessage &m, parser.diagnosticMessages()) {
            std::cerr << qPrintable(fileName) << ':' << m.loc.startLine << ':' << m.loc.startColumn
                      << ": error: " << qPrintable(m.message) << std::endl;
        }

        if (parsed) {
            using namespace AST;
            Program *program = AST::cast<Program *>(parser.rootNode());

            Codegen cg;
            globalCode = cg(program, &module, mode);

//            if (useInterpreter) {
//                Moth::InstructionSelection isel(vm, &module, code);
//                foreach (IR::Function *function, module.functions)
//                    isel(function);
//            } else
            {
                foreach (IR::Function *function, module.functions) {
                    MASM::InstructionSelection isel(vm, &module, code);
                    isel(function);
                }

                if (! protect(code, codeSize))
                    Q_UNREACHABLE();
            }
        }

        if (! globalCode)
            return EXIT_FAILURE;
    }

    if (!ctx->activation)
        ctx->activation = new QQmlJS::VM::Object();

    foreach (const QString *local, globalCode->locals) {
        ctx->activation->__put__(ctx, *local, QQmlJS::VM::Value::undefinedValue());
    }

    if (mode == Codegen::GlobalCode) {
        void * buf = __qmljs_create_exception_handler(ctx);
        if (setjmp(*(jmp_buf *)buf)) {
            if (VM::ErrorObject *e = ctx->result.asErrorObject())
                std::cerr << "Uncaught exception: " << qPrintable(e->value.toString(ctx)->toQString()) << std::endl;
            else
                std::cerr << "Uncaught exception: " << qPrintable(ctx->result.toString(ctx)->toQString()) << std::endl;
            return EXIT_FAILURE;
        }
    }

//    if (useInterpreter) {
//        Moth::VME vme;
//        vme(ctx, code);
//    } else
    {
        globalCode->code(ctx, globalCode->codeData);
    }

    if (! ctx->result.isUndefined()) {
        if (! qgetenv("SHOW_EXIT_VALUE").isEmpty())
            std::cout << "exit value: " << qPrintable(ctx->result.toString(ctx)->toQString()) << std::endl;
    }

    return EXIT_SUCCESS;
}


Value RegExpObject::__get__(ExecutionContext *ctx, String *name)
{
    QString n = name->toQString();
    if (n == QLatin1String("source"))
        return Value::fromString(ctx, value.pattern());
    else if (n == QLatin1String("global"))
        return Value::fromBoolean(global);
    else if (n == QLatin1String("ignoreCase"))
        return Value::fromBoolean(value.patternOptions() & QRegularExpression::CaseInsensitiveOption);
    else if (n == QLatin1String("multiline"))
        return Value::fromBoolean(value.patternOptions() & QRegularExpression::MultilineOption);
    else if (n == QLatin1String("lastIndex"))
        return lastIndex;
    return Object::__get__(ctx, name);
}

Value ErrorObject::__get__(ExecutionContext *ctx, String *name)
{
    QString n = name->toQString();
    if (n == QLatin1String("message"))
        return value;
    return Object::__get__(ctx, name);
}

void ErrorObject::setNameProperty(ExecutionContext *ctx)
{
    __put__(ctx, QLatin1String("name"), Value::fromString(ctx, className()));
}

void ScriptFunction::construct(VM::ExecutionContext *ctx)
{
    Object *obj = ctx->engine->newObject();
    Value proto = __get__(ctx, ctx->engine->id_prototype);
    if (proto.isObject())
        obj->prototype = proto.objectValue();
    ctx->thisObject = Value::fromObject(obj);
    function->code(ctx, function->codeData);
}

PropertyDescriptor *ActivationObject::__getPropertyDescriptor__(ExecutionContext *ctx, String *name, PropertyDescriptor *to_fill)
{
    if (context) {
        for (unsigned int i = 0; i < context->varCount; ++i) {
            String *var = context->vars[i];
            if (__qmljs_string_equal(var, name)) {
                *to_fill = PropertyDescriptor::fromValue(context->locals[i]);
                to_fill->writable = PropertyDescriptor::Set;
                to_fill->enumberable = PropertyDescriptor::Set;
                return to_fill;
            }
        }
        for (unsigned int i = 0; i < context->formalCount; ++i) {
            String *formal = context->formals[i];
            if (__qmljs_string_equal(formal, name)) {
                *to_fill = PropertyDescriptor::fromValue(context->arguments[i]);
                to_fill->writable = PropertyDescriptor::Set;
                to_fill->enumberable = PropertyDescriptor::Set;
                return to_fill;
            }
        }
        if (name->isEqualTo(ctx->engine->id_arguments)) {
            if (arguments.isUndefined()) {
                arguments = Value::fromObject(new ArgumentsObject(ctx));
                arguments.objectValue()->prototype = ctx->engine->objectPrototype;
            }

            *to_fill = PropertyDescriptor::fromValue(arguments);
            to_fill->writable = PropertyDescriptor::Unset;
            to_fill->enumberable = PropertyDescriptor::Unset;
            return to_fill;
        }
    }

    return Object::__getPropertyDescriptor__(ctx, name, to_fill);
}

Value ArgumentsObject::__get__(ExecutionContext *ctx, String *name)
{
    if (name->isEqualTo(ctx->engine->id_length))
        return Value::fromDouble(context->argumentCount);
    return Object::__get__(ctx, name);
}

PropertyDescriptor *ArgumentsObject::__getPropertyDescriptor__(ExecutionContext *ctx, String *name, PropertyDescriptor *to_fill)
{
    if (context) {
        const quint32 i = Value::fromString(name).toUInt32(ctx);
        if (i < context->argumentCount) {
            *to_fill = PropertyDescriptor::fromValue(context->arguments[i]);
            to_fill->writable = PropertyDescriptor::Unset;
            to_fill->enumberable = PropertyDescriptor::Unset;
            return to_fill;
        }
    }

    return Object::__getPropertyDescriptor__(ctx, name, to_fill);
}

ExecutionEngine::ExecutionEngine()
{
    rootContext = newContext();
    rootContext->init(this);

    id_length = identifier(QStringLiteral("length"));
    id_prototype = identifier(QStringLiteral("prototype"));
    id_constructor = identifier(QStringLiteral("constructor"));
    id_arguments = identifier(QStringLiteral("arguments"));
    id___proto__ = identifier(QStringLiteral("__proto__"));

    objectPrototype = new ObjectPrototype();
    stringPrototype = new StringPrototype(rootContext);
    numberPrototype = new NumberPrototype();
    booleanPrototype = new BooleanPrototype();
    arrayPrototype = new ArrayPrototype();
    datePrototype = new DatePrototype();
    functionPrototype = new FunctionPrototype(rootContext);
    regExpPrototype = new RegExpPrototype();
    errorPrototype = new ErrorPrototype();
    evalErrorPrototype = new EvalErrorPrototype(rootContext);
    rangeErrorPrototype = new RangeErrorPrototype(rootContext);
    referenceErrorPrototype = new ReferenceErrorPrototype(rootContext);
    syntaxErrorPrototype = new SyntaxErrorPrototype(rootContext);
    typeErrorPrototype = new TypeErrorPrototype(rootContext);
    uRIErrorPrototype = new URIErrorPrototype(rootContext);

    stringPrototype->prototype = objectPrototype;
    numberPrototype->prototype = objectPrototype;
    booleanPrototype->prototype = objectPrototype;
    arrayPrototype->prototype = objectPrototype;
    datePrototype->prototype = objectPrototype;
    functionPrototype->prototype = objectPrototype;
    regExpPrototype->prototype = objectPrototype;
    errorPrototype->prototype = objectPrototype;
    evalErrorPrototype->prototype = errorPrototype;
    rangeErrorPrototype->prototype = errorPrototype;
    referenceErrorPrototype->prototype = errorPrototype;
    syntaxErrorPrototype->prototype = errorPrototype;
    typeErrorPrototype->prototype = errorPrototype;
    uRIErrorPrototype->prototype = errorPrototype;

    objectCtor = Value::fromObject(new ObjectCtor(rootContext));
    stringCtor = Value::fromObject(new StringCtor(rootContext));
    numberCtor = Value::fromObject(new NumberCtor(rootContext));
    booleanCtor = Value::fromObject(new BooleanCtor(rootContext));
    arrayCtor = Value::fromObject(new ArrayCtor(rootContext));
    functionCtor = Value::fromObject(new FunctionCtor(rootContext));
    dateCtor = Value::fromObject(new DateCtor(rootContext));
    regExpCtor = Value::fromObject(new RegExpCtor(rootContext));
    errorCtor = Value::fromObject(new ErrorCtor(rootContext));
    evalErrorCtor = Value::fromObject(new EvalErrorCtor(rootContext));
    rangeErrorCtor = Value::fromObject(new RangeErrorCtor(rootContext));
    referenceErrorCtor = Value::fromObject(new ReferenceErrorCtor(rootContext));
    syntaxErrorCtor = Value::fromObject(new SyntaxErrorCtor(rootContext));
    typeErrorCtor = Value::fromObject(new TypeErrorCtor(rootContext));
    uRIErrorCtor = Value::fromObject(new URIErrorCtor(rootContext));

    stringCtor.objectValue()->prototype = functionPrototype;
    numberCtor.objectValue()->prototype = functionPrototype;
    booleanCtor.objectValue()->prototype = functionPrototype;
    arrayCtor.objectValue()->prototype = functionPrototype;
    functionCtor.objectValue()->prototype = functionPrototype;
    dateCtor.objectValue()->prototype = functionPrototype;
    regExpCtor.objectValue()->prototype = functionPrototype;
    errorCtor.objectValue()->prototype = functionPrototype;
    evalErrorCtor.objectValue()->prototype = errorPrototype;
    rangeErrorCtor.objectValue()->prototype = errorPrototype;
    referenceErrorCtor.objectValue()->prototype = errorPrototype;
    syntaxErrorCtor.objectValue()->prototype = errorPrototype;
    typeErrorCtor.objectValue()->prototype = errorPrototype;
    uRIErrorCtor.objectValue()->prototype = errorPrototype;

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
    VM::Object *glo = newObject(/*rootContext*/);
    globalObject = Value::fromObject(glo);
    rootContext->activation = glo;

    glo->__put__(rootContext, identifier(QStringLiteral("Object")), objectCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("String")), stringCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Number")), numberCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Boolean")), booleanCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Array")), arrayCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Function")), functionCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Date")), dateCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("RegExp")), regExpCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Error")), errorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("EvalError")), evalErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("RangeError")), rangeErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("ReferenceError")), referenceErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("SyntaxError")), syntaxErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("TypeError")), typeErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("URIError")), uRIErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Math")), Value::fromObject(newMathObject(rootContext)));
    glo->__put__(rootContext, identifier(QStringLiteral("undefined")), Value::undefinedValue());
    glo->__put__(rootContext, identifier(QStringLiteral("NaN")), Value::fromDouble(nan("")));
    glo->__put__(rootContext, identifier(QStringLiteral("Infinity")), Value::fromDouble(INFINITY));
    glo->__put__(rootContext, identifier(QStringLiteral("eval")), Value::fromObject(new EvalFunction(rootContext)));


}

ExecutionContext *ExecutionEngine::newContext()
{
    return new ExecutionContext();
}

String *ExecutionEngine::identifier(const QString &s)
{
    String *&id = identifiers[s];
    if (! id)
        id = newString(s);
    return id;
}

FunctionObject *ExecutionEngine::newNativeFunction(ExecutionContext *scope, void (*code)(ExecutionContext *))
{
    NativeFunction *f = new NativeFunction(scope, code);
    f->prototype = scope->engine->functionPrototype;
    return f;
}

FunctionObject *ExecutionEngine::newScriptFunction(ExecutionContext *scope, IR::Function *function)
{
    ScriptFunction *f = new ScriptFunction(scope, function);
    Object *proto = scope->engine->newObject();
    proto->__put__(scope, scope->engine->id_constructor, Value::fromObject(f));
    f->__put__(scope, scope->engine->id_prototype, Value::fromObject(proto));
    f->prototype = scope->engine->functionPrototype;
    return f;
}

Object *ExecutionEngine::newObject()
{
    Object *object = new Object();
    object->prototype = objectPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newObjectCtor(ExecutionContext *ctx)
{
    return new ObjectCtor(ctx);
}

String *ExecutionEngine::newString(const QString &s)
{
    return new String(s);
}

Object *ExecutionEngine::newStringObject(const Value &value)
{
    StringObject *object = new StringObject(value);
    object->prototype = stringPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newStringCtor(ExecutionContext *ctx)
{
    return new StringCtor(ctx);
}

Object *ExecutionEngine::newNumberObject(const Value &value)
{
    NumberObject *object = new NumberObject(value);
    object->prototype = numberPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newNumberCtor(ExecutionContext *ctx)
{
    return new NumberCtor(ctx);
}

Object *ExecutionEngine::newBooleanObject(const Value &value)
{
    Object *object = new BooleanObject(value);
    object->prototype = booleanPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newBooleanCtor(ExecutionContext *ctx)
{
    return new BooleanCtor(ctx);
}

Object *ExecutionEngine::newFunctionObject(ExecutionContext *ctx)
{
    Object *object = new FunctionObject(ctx);
    object->prototype = functionPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newFunctionCtor(ExecutionContext *ctx)
{
    return new FunctionCtor(ctx);
}

Object *ExecutionEngine::newArrayObject()
{
    ArrayObject *object = new ArrayObject();
    object->prototype = arrayPrototype;
    return object;
}

Object *ExecutionEngine::newArrayObject(const Array &value)
{
    ArrayObject *object = new ArrayObject(value);
    object->prototype = arrayPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newArrayCtor(ExecutionContext *ctx)
{
    return new ArrayCtor(ctx);
}

Object *ExecutionEngine::newDateObject(const Value &value)
{
    Object *object = new DateObject(value);
    object->prototype = datePrototype;
    return object;
}

FunctionObject *ExecutionEngine::newDateCtor(ExecutionContext *ctx)
{
    return new DateCtor(ctx);
}

Object *ExecutionEngine::newRegExpObject(const QString &pattern, int flags)
{
    bool global = (flags & IR::RegExp::RegExp_Global);
    QRegularExpression::PatternOptions options = 0;
    if (flags & IR::RegExp::RegExp_IgnoreCase)
        options |= QRegularExpression::CaseInsensitiveOption;
    if (flags & IR::RegExp::RegExp_Multiline)
        options |= QRegularExpression::MultilineOption;

    Object *object = new RegExpObject(QRegularExpression(pattern, options), global);
    object->prototype = regExpPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newRegExpCtor(ExecutionContext *ctx)
{
    return new RegExpCtor(ctx);
}

Object *ExecutionEngine::newErrorObject(const Value &value)
{
    ErrorObject *object = new ErrorObject(value);
    object->prototype = errorPrototype;
    return object;
}

Object *ExecutionEngine::newMathObject(ExecutionContext *ctx)
{
    MathObject *object = new MathObject(ctx);
    object->prototype = objectPrototype;
    return object;
}

Object *ExecutionEngine::newActivationObject(ExecutionContext *ctx)
{
    return new ActivationObject(ctx);
}

Object *ExecutionEngine::newForEachIteratorObject(Object *o)
{
    return new ForEachIteratorObject(o);
}
