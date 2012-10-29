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
#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>
#include <typeinfo>

using namespace QQmlJS::VM;

//
// Object
//
Object::~Object()
{
    delete members;
}

void Object::__put__(Context *ctx, const QString &name, const Value &value)
{
    __put__(ctx, ctx->engine->identifier(name), value);
}

void Object::__put__(Context *ctx, const QString &name, void (*code)(Context *), int count)
{
    Q_UNUSED(count);
    __put__(ctx, name, Value::fromObject(ctx->engine->newNativeFunction(ctx, code)));
}

// Section 8.12.1
PropertyDescriptor *Object::__getOwnProperty__(Context *, String *name)
{
    if (members)
        return members->find(name);
    return 0;
}

// Section 8.12.2
PropertyDescriptor *Object::__getPropertyDescriptor__(Context *ctx, String *name, PropertyDescriptor *to_fill)
{
    if (PropertyDescriptor *p = __getOwnProperty__(ctx, name))
        return p;

    if (prototype)
        return prototype->__getPropertyDescriptor__(ctx, name, to_fill);
    return 0;
}

// Section 8.12.3
Value Object::__get__(Context *ctx, String *name)
{
    if (name->isEqualTo(ctx->engine->id___proto__))
        return Value::fromObject(prototype);

    PropertyDescriptor tmp;
    if (PropertyDescriptor *p = __getPropertyDescriptor__(ctx, name, &tmp)) {
        if (p->isData())
            return p->value;
        if (!p->get)
            return Value::undefinedValue();
        FunctionObject *f = p->get->asFunctionObject();
        if (f) {
            f->call(ctx);
            return ctx->result;
        }
    }
    return Value::undefinedValue();
}

// Section 8.12.4
bool Object::__canPut__(Context *ctx, String *name)
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
void Object::__put__(Context *ctx, String *name, const Value &value, bool throwException)
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
            FunctionObject *func = pd->set->asFunctionObject();
            assert(func);

            // ### unify with callFunction method
            Context k;
            Context *c = func->needsActivation ? ctx->engine->newContext() : &k;
            Value that = Value::fromObject(this);
            Value args[1];
            args[0] = value;
            c->initCallContext(ctx, &that, func, args, 1);
            func->call(c);
            c->leaveCallContext();
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
bool Object::__hasProperty__(Context *ctx, String *name) const
{
    if (members)
        return members->find(name) != 0;

    return prototype ? prototype->__hasProperty__(ctx, name) : false;
}

// Section 8.12.7
bool Object::__delete__(Context *ctx, String *name, bool throwException)
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
bool Object::__defineOwnProperty__(Context *ctx, String *name, PropertyDescriptor *desc, bool throwException)
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

Value ArrayObject::__get__(Context *ctx, String *name)
{
    if (name->isEqualTo(ctx->engine->id_length))
        return Value::fromDouble(value.size());
    return Object::__get__(ctx, name);
}

bool FunctionObject::hasInstance(Context *ctx, const Value &value)
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

void FunctionObject::call(Context *ctx)
{
    Q_UNUSED(ctx);
}

void FunctionObject::construct(Context *ctx)
{
    ctx->thisObject = Value::fromObject(ctx->engine->newObject());
    call(ctx);
}

ScriptFunction::ScriptFunction(Context *scope, IR::Function *function)
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

void ScriptFunction::call(VM::Context *ctx)
{
    function->code(ctx, function->codeData);
}

Value RegExpObject::__get__(Context *ctx, String *name)
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


void ScriptFunction::construct(VM::Context *ctx)
{
    Object *obj = ctx->engine->newObject();
    Value proto = __get__(ctx, ctx->engine->id_prototype);
    if (proto.isObject())
        obj->prototype = proto.objectValue();
    ctx->thisObject = Value::fromObject(obj);
    function->code(ctx, function->codeData);
}

PropertyDescriptor *ActivationObject::__getPropertyDescriptor__(Context *ctx, String *name, PropertyDescriptor *to_fill)
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

Value ArgumentsObject::__get__(Context *ctx, String *name)
{
    if (name->isEqualTo(ctx->engine->id_length))
        return Value::fromDouble(context->argumentCount);
    return Object::__get__(ctx, name);
}

PropertyDescriptor *ArgumentsObject::__getPropertyDescriptor__(Context *ctx, String *name, PropertyDescriptor *to_fill)
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

    stringPrototype->prototype = objectPrototype;
    numberPrototype->prototype = objectPrototype;
    booleanPrototype->prototype = objectPrototype;
    arrayPrototype->prototype = objectPrototype;
    datePrototype->prototype = objectPrototype;
    functionPrototype->prototype = objectPrototype;
    regExpPrototype->prototype = objectPrototype;

    objectCtor = Value::fromObject(new ObjectCtor(rootContext));
    stringCtor = Value::fromObject(new StringCtor(rootContext));
    numberCtor = Value::fromObject(new NumberCtor(rootContext));
    booleanCtor = Value::fromObject(new BooleanCtor(rootContext));
    arrayCtor = Value::fromObject(new ArrayCtor(rootContext));
    functionCtor = Value::fromObject(new FunctionCtor(rootContext));
    dateCtor = Value::fromObject(new DateCtor(rootContext));
    regExpCtor = Value::fromObject(new RegExpCtor(rootContext));

    stringCtor.objectValue()->prototype = functionPrototype;
    numberCtor.objectValue()->prototype = functionPrototype;
    booleanCtor.objectValue()->prototype = functionPrototype;
    arrayCtor.objectValue()->prototype = functionPrototype;
    functionCtor.objectValue()->prototype = functionPrototype;
    dateCtor.objectValue()->prototype = functionPrototype;
    regExpCtor.objectValue()->prototype = functionPrototype;

    objectPrototype->init(rootContext, objectCtor);
    stringPrototype->init(rootContext, stringCtor);
    numberPrototype->init(rootContext, numberCtor);
    booleanPrototype->init(rootContext, booleanCtor);
    arrayPrototype->init(rootContext, arrayCtor);
    datePrototype->init(rootContext, dateCtor);
    functionPrototype->init(rootContext, functionCtor);
    regExpPrototype->init(rootContext, regExpCtor);

    //
    // set up the global object
    //
    VM::Object *glo = newObject(/*rootContext*/);
    globalObject = Value::fromObject(glo);
    rootContext->activation = Value::fromObject(glo);

    glo->__put__(rootContext, identifier(QStringLiteral("Object")), objectCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("String")), stringCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Number")), numberCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Boolean")), booleanCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Array")), arrayCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Function")), functionCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Date")), dateCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("RegExp")), regExpCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Math")), Value::fromObject(newMathObject(rootContext)));
}

Context *ExecutionEngine::newContext()
{
    return new Context();
}

String *ExecutionEngine::identifier(const QString &s)
{
    String *&id = identifiers[s];
    if (! id)
        id = newString(s);
    return id;
}

FunctionObject *ExecutionEngine::newNativeFunction(Context *scope, void (*code)(Context *))
{
    NativeFunction *f = new NativeFunction(scope, code);
    f->prototype = scope->engine->functionPrototype;
    return f;
}

FunctionObject *ExecutionEngine::newScriptFunction(Context *scope, IR::Function *function)
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

FunctionObject *ExecutionEngine::newObjectCtor(Context *ctx)
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

FunctionObject *ExecutionEngine::newStringCtor(Context *ctx)
{
    return new StringCtor(ctx);
}

Object *ExecutionEngine::newNumberObject(const Value &value)
{
    NumberObject *object = new NumberObject(value);
    object->prototype = numberPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newNumberCtor(Context *ctx)
{
    return new NumberCtor(ctx);
}

Object *ExecutionEngine::newBooleanObject(const Value &value)
{
    Object *object = new BooleanObject(value);
    object->prototype = booleanPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newBooleanCtor(Context *ctx)
{
    return new BooleanCtor(ctx);
}

Object *ExecutionEngine::newFunctionObject(Context *ctx)
{
    Object *object = new FunctionObject(ctx);
    object->prototype = functionPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newFunctionCtor(Context *ctx)
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

FunctionObject *ExecutionEngine::newArrayCtor(Context *ctx)
{
    return new ArrayCtor(ctx);
}

Object *ExecutionEngine::newDateObject(const Value &value)
{
    Object *object = new DateObject(value);
    object->prototype = datePrototype;
    return object;
}

FunctionObject *ExecutionEngine::newDateCtor(Context *ctx)
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

FunctionObject *ExecutionEngine::newRegExpCtor(Context *ctx)
{
    return new RegExpCtor(ctx);
}

Object *ExecutionEngine::newErrorObject(const Value &value)
{
    ErrorObject *object = new ErrorObject(value);
    object->prototype = objectPrototype;
    return object;
}

Object *ExecutionEngine::newMathObject(Context *ctx)
{
    MathObject *object = new MathObject(ctx);
    object->prototype = objectPrototype;
    return object;
}

Object *ExecutionEngine::newActivationObject(Context *ctx)
{
    return new ActivationObject(ctx);
}

Object *ExecutionEngine::newForEachIteratorObject(Object *o)
{
    return new ForEachIteratorObject(o);
}
