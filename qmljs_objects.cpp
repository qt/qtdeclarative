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
#include "qv4isel_p.h"
#include "qv4ecmaobjects_p.h"
#include "qv4mm.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4ir_p.h>
#include <qv4codegen_p.h>
#include "private/qlocale_tools_p.h"

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>
#include <typeinfo>
#include <iostream>
#include <alloca.h>

using namespace QQmlJS::VM;


//
// Object
//
Object::~Object()
{
}

void Object::__put__(ExecutionContext *ctx, const QString &name, const Value &value)
{
    __put__(ctx, ctx->engine->identifier(name), value);
}

Value Object::getValue(ExecutionContext *ctx, const PropertyDescriptor *p) const
{
    if (p->isData())
        return p->value;
    if (!p->get)
        return Value::undefinedValue();

    return p->get->call(ctx, Value::fromObject(const_cast<Object *>(this)), 0, 0);
}

Value Object::getValueChecked(ExecutionContext *ctx, const PropertyDescriptor *p) const
{
    if (!p || p->type == PropertyDescriptor::Generic)
        return Value::undefinedValue();
    return getValue(ctx, p);
}

Value Object::getValueChecked(ExecutionContext *ctx, const PropertyDescriptor *p, bool *exists) const
{
    *exists = p && p->type != PropertyDescriptor::Generic;
    if (!*exists)
        return Value::undefinedValue();
    return getValue(ctx, p);
}

bool Object::inplaceBinOp(Value rhs, String *name, BinOp op, ExecutionContext *ctx)
{
    bool hasProperty = false;
    Value v = __get__(ctx, name, &hasProperty);
    if (!hasProperty)
        return false;
    Value result = op(v, rhs, ctx);
    __put__(ctx, name, result);
    return true;
}

bool Object::inplaceBinOp(Value rhs, Value index, BinOp op, ExecutionContext *ctx)
{
    uint idx = index.asArrayIndex();
    if (idx < UINT_MAX) {
        bool hasProperty = false;
        Value v = __get__(ctx, idx, &hasProperty);
        if (!hasProperty)
            return false;
        v = op(v, rhs, ctx);
        __put__(ctx, idx, v);
        return true;
    }
    String *name = index.toString(ctx);
    assert(name);
    return inplaceBinOp(rhs, name, op, ctx);
}

void Object::defineDefaultProperty(String *name, Value value)
{
    if (!members)
        members.reset(new PropertyTable());
    PropertyDescriptor *pd = members->insert(name);
    pd->type = PropertyDescriptor::Data;
    pd->writable = PropertyDescriptor::Enabled;
    pd->enumberable = PropertyDescriptor::Disabled;
    pd->configurable = PropertyDescriptor::Enabled;
    pd->value = value;
}

void Object::defineDefaultProperty(ExecutionContext *context, const QString &name, Value value)
{
    defineDefaultProperty(context->engine->identifier(name), value);
}

void Object::defineDefaultProperty(ExecutionContext *context, const QString &name, Value (*code)(ExecutionContext *), int argumentCount)
{
    Q_UNUSED(argumentCount);
    String *s = context->engine->identifier(name);
    FunctionObject* function = context->engine->newBuiltinFunction(context, s, code);
    function->defineReadonlyProperty(context->engine->id_length, Value::fromInt32(argumentCount));
    defineDefaultProperty(s, Value::fromObject(function));
}

void Object::defineReadonlyProperty(ExecutionEngine *engine, const QString &name, Value value)
{
    defineReadonlyProperty(engine->identifier(name), value);
}

void Object::defineReadonlyProperty(String *name, Value value)
{
    if (!members)
        members.reset(new PropertyTable());
    PropertyDescriptor *pd = members->insert(name);
    pd->type = PropertyDescriptor::Data;
    pd->writable = PropertyDescriptor::Disabled;
    pd->enumberable = PropertyDescriptor::Disabled;
    pd->configurable = PropertyDescriptor::Disabled;
    pd->value = value;
}

void Object::getCollectables(QVector<Object *> &objects)
{
    if (prototype)
        objects.append(prototype);

    if (members) {
        for (PropertyTable::iterator it = members->begin(), eit = members->end(); it < eit; ++it) {
            if (!(*it))
                continue;
            PropertyDescriptor &pd = (*it)->descriptor;
            if (pd.isData()) {
                if (Object *o = pd.value.asObject())
                    objects.append(o);
            } else if (pd.isAccessor()) {
                if (pd.get)
                    objects.append(pd.get);
                if (pd.set)
                    objects.append(pd.set);
            }
        }
    }
    array.getCollectables(objects);
}

// Section 8.12.1
PropertyDescriptor *Object::__getOwnProperty__(ExecutionContext *ctx, String *name)
{
    uint idx = name->asArrayIndex();
    if (idx != String::InvalidArrayIndex)
        return __getOwnProperty__(ctx, idx);

    if (members)
        return members->find(name);
    return 0;
}

PropertyDescriptor *Object::__getOwnProperty__(ExecutionContext *ctx, uint index)
{
    PropertyDescriptor *p = array.at(index);
    if(p && p->type != PropertyDescriptor::Generic)
        return p;
    if (isString)
        return static_cast<StringObject *>(this)->getIndex(ctx, index);

    return 0;
}

// Section 8.12.2
PropertyDescriptor *Object::__getPropertyDescriptor__(ExecutionContext *ctx, String *name)
{
    uint idx = name->asArrayIndex();
    if (idx != String::InvalidArrayIndex)
        return __getPropertyDescriptor__(ctx, idx);


    Object *o = this;
    while (o) {
        if (o->members) {
            if (PropertyDescriptor *p = o->members->find(name))
                return p;
        }
        o = o->prototype;
    }
    return 0;
}

PropertyDescriptor *Object::__getPropertyDescriptor__(ExecutionContext *ctx, uint index)
{
    Object *o = this;
    while (o) {
        PropertyDescriptor *p = o->array.at(index);
        if(p && p->type != PropertyDescriptor::Generic)
            return p;
        if (o->isString) {
            p = static_cast<StringObject *>(o)->getIndex(ctx, index);
            if (p)
                return p;
        }
        o = o->prototype;
    }
    return 0;
}

// Section 8.12.3
Value Object::__get__(ExecutionContext *ctx, String *name, bool *hasProperty)
{
    uint idx = name->asArrayIndex();
    if (idx != String::InvalidArrayIndex)
        return __get__(ctx, idx, hasProperty);

    if (name->isEqualTo(ctx->engine->id___proto__)) {
        if (hasProperty)
            *hasProperty = true;
        return Value::fromObject(prototype);
    }

    if (PropertyDescriptor *p = __getPropertyDescriptor__(ctx, name)) {
        if (hasProperty)
            *hasProperty = true;
        return getValue(ctx, p);
    }

    if (hasProperty)
        *hasProperty = false;
    return Value::undefinedValue();
}

Value Object::__get__(ExecutionContext *ctx, uint index, bool *hasProperty)
{
    if (const PropertyDescriptor *p = __getPropertyDescriptor__(ctx, index)) {
        if (hasProperty)
            *hasProperty = true;
        return getValue(ctx, p);
    }

    if (hasProperty)
        *hasProperty = false;
    return Value::undefinedValue();
}


// Section 8.12.5
void Object::__put__(ExecutionContext *ctx, String *name, Value value)
{
    uint idx = name->asArrayIndex();
    if (idx != String::InvalidArrayIndex)
        return __put__(ctx, idx, value);

    PropertyDescriptor *pd  = __getOwnProperty__(ctx, name);
    // clause 1
    if (pd) {
        if (pd->isAccessor()) {
                if (pd->set)
                    goto cont;
                goto reject;
        } else if (!pd->isWritable())
            goto reject;
        else if (isArray && name->isEqualTo(ctx->engine->id_length)) {
            bool ok;
            uint l = value.asArrayLength(ctx, &ok);
            if (!ok)
                ctx->throwRangeError(value);
            ok = array.setLength(l);
            if (!ok)
                goto reject;
        } else {
            pd->value = value;
        }
        return;
    } else if (!prototype) {
        if (!extensible)
            goto reject;
    } else {
        if (PropertyDescriptor *p = prototype->__getPropertyDescriptor__(ctx, name)) {
            if (p->isAccessor()) {
                if (p->set)
                    goto cont;
                goto reject;
            }
            if (!extensible)
                goto reject;
            if (!p->isWritable())
                goto reject;
        } else {
            if (!extensible)
                goto reject;
        }
    }

    cont:

    if (!members)
        members.reset(new PropertyTable());


    // clause 4
    if (!pd && prototype)
        pd = prototype->__getPropertyDescriptor__(ctx, name);

    // Clause 5
    if (pd && pd->isAccessor()) {
        assert(pd->set != 0);

        Value args[1];
        args[0] = value;
        pd->set->call(ctx, Value::fromObject(this), args, 1);
        return;
    }

    {
        PropertyDescriptor *p = members->insert(name);
        p->type = PropertyDescriptor::Data;
        p->value = value;
        p->configurable = PropertyDescriptor::Enabled;
        p->enumberable = PropertyDescriptor::Enabled;
        p->writable = PropertyDescriptor::Enabled;
        return;
    }

  reject:
    if (ctx->strictMode)
        __qmljs_throw_type_error(ctx);
}

void Object::__put__(ExecutionContext *ctx, uint index, Value value)
{
    PropertyDescriptor *pd  = __getOwnProperty__(ctx, index);
    // clause 1
    if (pd) {
        if (pd->isAccessor()) {
                if (pd->set)
                    goto cont;
                goto reject;
        } else if (!pd->isWritable())
            goto reject;
        else
            pd->value = value;
        return;
    } else if (!prototype) {
        if (!extensible)
            goto reject;
    } else {
        if (PropertyDescriptor *p = prototype->__getPropertyDescriptor__(ctx, index)) {
            if (p->isAccessor()) {
                if (p->set)
                    goto cont;
                goto reject;
            }
            if (!extensible)
                goto reject;
            if (!p->isWritable())
                goto reject;
        } else {
            if (!extensible)
                goto reject;
        }
    }

    cont:

    // clause 4
    if (!pd && prototype)
        pd = prototype->__getPropertyDescriptor__(ctx, index);

    // Clause 5
    if (pd && pd->isAccessor()) {
        assert(pd->set != 0);

        Value args[1];
        args[0] = value;
        pd->set->call(ctx, Value::fromObject(this), args, 1);
        return;
    }

    array.set(index, value);
    return;

  reject:
    if (ctx->strictMode)
        __qmljs_throw_type_error(ctx);
}

// Section 8.12.6
bool Object::__hasProperty__(const ExecutionContext *ctx, String *name) const
{
    uint idx = name->asArrayIndex();
    if (idx != String::InvalidArrayIndex)
        return __hasProperty__(ctx, idx);

    if (members && members->find(name) != 0)
        return true;

    return prototype ? prototype->__hasProperty__(ctx, name) : false;
}

bool Object::__hasProperty__(const ExecutionContext *ctx, uint index) const
{
    const PropertyDescriptor *p = array.at(index);
    if (p && p->type != PropertyDescriptor::Generic)
        return true;

    return prototype ? prototype->__hasProperty__(ctx, index) : false;
}

// Section 8.12.7
bool Object::__delete__(ExecutionContext *ctx, String *name)
{
    uint idx = name->asArrayIndex();
    if (idx != String::InvalidArrayIndex)
        return __delete__(ctx, idx);

    if (members) {
        if (PropertyTableEntry *entry = members->findEntry(name)) {
            if (entry->descriptor.isConfigurable()) {
                members->remove(entry);
                return true;
            }
            if (ctx->strictMode)
                __qmljs_throw_type_error(ctx);
            return false;
        }
    }
    return true;
}

bool Object::__delete__(ExecutionContext *ctx, uint index)
{
    if (array.deleteIndex(index))
        return true;
    if (ctx->strictMode)
        __qmljs_throw_type_error(ctx);
    return false;
}

// Section 8.12.9
bool Object::__defineOwnProperty__(ExecutionContext *ctx, String *name, PropertyDescriptor *desc)
{
    uint idx = name->asArrayIndex();
    if (idx != String::InvalidArrayIndex)
        return __defineOwnProperty__(ctx, idx, desc);

    PropertyDescriptor *current;

    if (isArray && name->isEqualTo(ctx->engine->id_length)) {
        PropertyDescriptor *lp = array.getLengthProperty();
        if (desc->isEmpty() || desc->isSubset(lp))
            return true;
        if (!lp->isWritable() || desc->type == PropertyDescriptor::Accessor || desc->isConfigurable() || desc->isEnumerable())
            goto reject;
        bool succeeded = true;
        if (desc->type == PropertyDescriptor::Data) {
            bool ok;
            uint l = desc->value.asArrayLength(ctx, &ok);
            if (!ok)
                ctx->throwRangeError(desc->value);
            succeeded = array.setLength(l);
        }
        if (desc->writable == PropertyDescriptor::Disabled)
            lp->writable = PropertyDescriptor::Disabled;
        if (!succeeded)
            goto reject;
        return true;
    }

    if (!members)
        members.reset(new PropertyTable());

    // Clause 1
    current = __getOwnProperty__(ctx, name);
    if (!current) {
        // clause 3
        if (!extensible)
            goto reject;
        // clause 4
        PropertyDescriptor *pd = members->insert(name);
        *pd = *desc;
        pd->fullyPopulated();
        return true;
    }

    return __defineOwnProperty__(ctx, current, desc);
reject:
  if (ctx->strictMode)
      __qmljs_throw_type_error(ctx);
  return false;
}

bool Object::__defineOwnProperty__(ExecutionContext *ctx, uint index, PropertyDescriptor *desc)
{
    PropertyDescriptor *current;

    // 15.4.5.1, 4b
    if (isArray && index >= array.length() && !array.getLengthProperty()->isWritable())
        goto reject;

    // Clause 1
    current = __getOwnProperty__(ctx, index);
    if (!current) {
        // clause 3
        if (!extensible)
            goto reject;
        // clause 4
        PropertyDescriptor *pd = array.insert(index);
        *pd = *desc;
        pd->fullyPopulated();
        return true;
    }

    return __defineOwnProperty__(ctx, current, desc);
reject:
  if (ctx->strictMode)
      __qmljs_throw_type_error(ctx);
  return false;
}

bool Object::__defineOwnProperty__(ExecutionContext *ctx, PropertyDescriptor *current, PropertyDescriptor *desc)
{
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
        if (desc->enumberable != PropertyDescriptor::Undefined && desc->enumberable != current->enumberable)
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
            current->writable = PropertyDescriptor::Disabled;
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
            if (desc->get && !(current->get == desc->get || (!current->get && (quintptr)desc->get == 0x1)))
                goto reject;
            if (desc->set && !(current->set == desc->set || (!current->set && (quintptr)desc->set == 0x1)))
                goto reject;
        }
    }

  accept:

    *current += *desc;
    return true;
  reject:
    if (ctx->strictMode)
        __qmljs_throw_type_error(ctx);
    return false;
}


bool Object::__defineOwnProperty__(ExecutionContext *ctx, const QString &name, PropertyDescriptor *desc)
{
    return __defineOwnProperty__(ctx, ctx->engine->identifier(name), desc);
}


Value Object::call(ExecutionContext *context, Value , Value *, int)
{
    context->throwTypeError();
    return Value::undefinedValue();
}

void ArrayObject::init(ExecutionContext *context)
{
    isArray = true;
    if (!members)
        members.reset(new PropertyTable());
    PropertyDescriptor *pd = members->insert(context->engine->id_length);
    pd->type = PropertyDescriptor::Data;
    pd->writable = PropertyDescriptor::Enabled;
    pd->enumberable = PropertyDescriptor::Disabled;
    pd->configurable = PropertyDescriptor::Disabled;
    pd->value = Value::fromInt32(0);
    array.setLengthProperty(pd);
}



void ForEachIteratorObject::getCollectables(QVector<Object *> &objects)
{
    Object::getCollectables(objects);
    if (it.object)
        objects.append(it.object);
}


Function::~Function()
{
    delete[] codeData;
}

bool FunctionObject::hasInstance(ExecutionContext *ctx, const Value &value)
{
    if (! value.isObject())
        return false;

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
    Object *obj = context->engine->newObject();
    Value proto = __get__(context, context->engine->id_prototype);
    if (proto.isObject())
        obj->prototype = proto.objectValue();

    ExecutionContext k;
    ExecutionContext *ctx = needsActivation ? context->engine->newContext() : &k;

    ctx->initCallContext(context, Value::fromObject(obj), this, args, argc);
    Value result = construct(ctx);
    ctx->leaveCallContext();

    if (result.isObject())
        return result;
    return Value::fromObject(obj);
}

Value FunctionObject::call(ExecutionContext *context, Value thisObject, Value *args, int argc)
{
    ExecutionContext k;
    ExecutionContext *ctx = needsActivation ? context->engine->newContext() : &k;

    ctx->initCallContext(context, thisObject, this, args, argc);
    Value result = call(ctx);
    ctx->leaveCallContext();
    return result;
}

Value FunctionObject::callDirect(ExecutionContext *context, Value thisObject, Value *args, int argc)
{
    ExecutionContext k;
    ExecutionContext *ctx = needsActivation ? context->engine->newContext() : &k;

    ctx->initCallContext(context, thisObject, this, args, argc);
    maybeAdjustThisObjectForDirectCall(ctx, thisObject);
    Value result = call(ctx);
    ctx->leaveCallContext();
    return result;
}

Value FunctionObject::call(ExecutionContext *ctx)
{
    Q_UNUSED(ctx);
    return Value::undefinedValue();
}

Value FunctionObject::construct(ExecutionContext *ctx)
{
    return call(ctx);
}

ScriptFunction::ScriptFunction(ExecutionContext *scope, VM::Function *function)
    : FunctionObject(scope)
    , function(function)
{
    assert(function);
    assert(function->code);

    // global function
    if (!scope)
        return;

    MemoryManager::GCBlocker gcBlocker(scope->engine->memoryManager);

    if (!function->name.isEmpty())
        name = scope->engine->identifier(function->name);
    needsActivation = function->needsActivation();
    usesArgumentsObject = function->usesArgumentsObject;
    strictMode = function->isStrict;
    formalParameterCount = function->formals.size();
    if (formalParameterCount) {
        formalParameterList = new String*[formalParameterCount];
        for (unsigned int i = 0; i < formalParameterCount; ++i) {
            formalParameterList[i] = scope->engine->identifier(function->formals.at(i));
        }
    }
    defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(formalParameterCount));

    varCount = function->locals.size();
    if (varCount) {
        varList = new String*[varCount];
        for (unsigned int i = 0; i < varCount; ++i) {
            varList[i] = scope->engine->identifier(function->locals.at(i));
        }
    }

    Object *proto = scope->engine->newObject();
    proto->defineDefaultProperty(scope->engine->id_constructor, Value::fromObject(this));
    PropertyDescriptor *pd = members->insert(scope->engine->id_prototype);
    pd->type = PropertyDescriptor::Data;
    pd->writable = PropertyDescriptor::Enabled;
    pd->enumberable = PropertyDescriptor::Disabled;
    pd->configurable = PropertyDescriptor::Disabled;
    pd->value = Value::fromObject(proto);

    prototype = scope->engine->functionPrototype;

    if (scope->strictMode) {
        FunctionObject *thrower = scope->engine->newBuiltinFunction(scope, 0, __qmljs_throw_type_error);
        PropertyDescriptor pd = PropertyDescriptor::fromAccessor(thrower, thrower);
        pd.configurable = PropertyDescriptor::Disabled;
        pd.enumberable = PropertyDescriptor::Disabled;
        __defineOwnProperty__(scope, QStringLiteral("caller"), &pd);
        __defineOwnProperty__(scope, QStringLiteral("arguments"), &pd);
    }
}

ScriptFunction::~ScriptFunction()
{
    delete[] formalParameterList;
    delete[] varList;
}

Value ScriptFunction::call(VM::ExecutionContext *ctx)
{
    assert(function->code);
    return function->code(ctx, function->codeData);
}


Value EvalFunction::call(ExecutionContext *context, Value /*thisObject*/, Value *args, int argc)
{
    if (argc < 1)
        return Value::undefinedValue();

    if (!args[0].isString())
        return args[0];

    // ### how to determine this correctly?
    bool directCall = true;

    const QString code = args[0].stringValue()->toQString();
    QQmlJS::VM::Function *f = parseSource(context, QStringLiteral("eval code"), code, QQmlJS::Codegen::EvalCode);
    if (!f)
        return Value::undefinedValue();

    bool strict = f->isStrict || context->strictMode;

    ExecutionContext k, *ctx;
    if (!directCall) {
        qDebug() << "!direct";
        // ###
    } else if (strict) {
        ctx = &k;
        ctx->initCallContext(context, context->thisObject, this, args, argc);
    } else {
        // use the surrounding context
        ctx = context;
    }

    // set the correct strict mode flag on the context
    bool cstrict = ctx->strictMode;
    ctx->strictMode = strict;

    Value result = f->code(ctx, f->codeData);

    ctx->strictMode = cstrict;

    if (strict)
        ctx->leaveCallContext();

    return result;
}

EvalFunction::EvalFunction(ExecutionContext *scope)
    : FunctionObject(scope)
{
    name = scope->engine->newString(QLatin1String("eval"));
}

QQmlJS::VM::Function *EvalFunction::parseSource(QQmlJS::VM::ExecutionContext *ctx,
                                                const QString &fileName, const QString &source,
                                                QQmlJS::Codegen::Mode mode)
{
    using namespace QQmlJS;

    MemoryManager::GCBlocker gcBlocker(ctx->engine->memoryManager);

    VM::ExecutionEngine *vm = ctx->engine;
    IR::Module module;
    VM::Function *globalCode = 0;

    {
        QQmlJS::Engine ee, *engine = &ee;
        Lexer lexer(engine);
        lexer.setCode(source, 1, false);
        Parser parser(engine);

        const bool parsed = parser.parseProgram();

        VM::DiagnosticMessage *error = 0, **errIt = &error;
        foreach (const QQmlJS::DiagnosticMessage &m, parser.diagnosticMessages()) {
            if (m.isError()) {
                *errIt = new VM::DiagnosticMessage;
                (*errIt)->fileName = fileName;
                (*errIt)->offset = m.loc.offset;
                (*errIt)->length = m.loc.length;
                (*errIt)->startLine = m.loc.startLine;
                (*errIt)->startColumn = m.loc.startColumn;
                (*errIt)->type = VM::DiagnosticMessage::Error;
                (*errIt)->message = m.message;
                errIt = &(*errIt)->next;
            } else {
                std::cerr << qPrintable(fileName) << ':' << m.loc.startLine << ':' << m.loc.startColumn
                          << ": warning: " << qPrintable(m.message) << std::endl;
            }
        }
        if (error)
            ctx->throwSyntaxError(error);

        if (parsed) {
            using namespace AST;
            Program *program = AST::cast<Program *>(parser.rootNode());
            if (!program) {
                // if parsing was successful, and we have no program, then
                // we're done...:
                return 0;
            }

            Codegen cg(ctx);
            IR::Function *globalIRCode = cg(fileName, program, &module, mode);
            QScopedPointer<EvalInstructionSelection> isel(ctx->engine->iselFactory->create(vm, &module));
            if (globalIRCode)
                globalCode = isel->vmFunction(globalIRCode);
        }

        if (! globalCode)
            // ### should be a syntax error
            __qmljs_throw_type_error(ctx);
    }

    return globalCode;
}

// parseInt [15.1.2.2]
ParseIntFunction::ParseIntFunction(ExecutionContext *scope)
    : FunctionObject(scope)
{
    name = scope->engine->newString(QLatin1String("parseInt"));
}

static inline int toInt(const QChar &qc, int R)
{
    ushort c = qc.unicode();
    int v = -1;
    if (c >= '0' && c <= '9')
        v = c - '0';
    else if (c >= 'A' && c <= 'Z')
        v = c - 'A' + 10;
    else if (c >= 'a' && c <= 'z')
        v = c - 'a' + 10;
    if (v >= 0 && v < R)
        return v;
    else
        return -1;
}

Value ParseIntFunction::call(ExecutionContext *context, Value thisObject, Value *args, int argc)
{
    Q_UNUSED(thisObject);

    Value string = (argc > 0) ? args[0] : Value::undefinedValue();
    Value radix = (argc > 1) ? args[1] : Value::undefinedValue();
    int R = radix.isUndefined() ? 0 : radix.toInt32(context);

    // [15.1.2.2] step by step:
    String *inputString = string.toString(context); // 1
    QString trimmed = inputString->toQString().trimmed(); // 2
    const QChar *pos = trimmed.constData();
    const QChar *end = pos + trimmed.length();

    int sign = 1; // 3
    if (pos != end) {
        if (*pos == QLatin1Char('-'))
            sign = -1; // 4
        if (*pos == QLatin1Char('-') || *pos == QLatin1Char('+'))
            ++pos; // 5
    }
    bool stripPrefix = true; // 7
    if (R) { // 8
        if (R < 2 || R > 36)
            return Value::fromDouble(nan("")); // 8a
        if (R != 16)
            stripPrefix = false; // 8b
    } else { // 9
        R = 10; // 9a
    }
    if (stripPrefix) { // 10
        if ((end - pos >= 2)
                && (pos[0] == QLatin1Char('0'))
                && (pos[1] == QLatin1Char('x') || pos[1] == QLatin1Char('X'))) { // 10a
            pos += 2;
            R = 16;
        }
    }
    // 11: Z is progressively built below
    // 13: this is handled by the toInt function
    if (pos == end) // 12
        return Value::fromDouble(nan(""));
    bool overflow = false;
    qint64 v_overflow;
    unsigned overflow_digit_count = 0;
    int d = toInt(*pos++, R);
    if (d == -1)
        return Value::fromDouble(nan(""));
    qint64 v = d;
    while (pos != end) {
        d = toInt(*pos++, R);
        if (d == -1)
            break;
        if (overflow) {
            if (overflow_digit_count == 0) {
                v_overflow = v;
                v = 0;
            }
            ++overflow_digit_count;
            v = v * R + d;
        } else {
            qint64 vNew = v * R + d;
            if (vNew < v) {
                overflow = true;
                --pos;
            } else {
                v = vNew;
            }
        }
    }

    if (overflow) {
        double result = (double) v_overflow * pow(R, overflow_digit_count);
        result += v;
        return Value::fromDouble(sign * result);
    } else {
        return Value::fromDouble(sign * (double) v); // 15
    }
}

// parseFloat [15.1.2.3]
ParseFloatFunction::ParseFloatFunction(ExecutionContext *scope)
    : FunctionObject(scope)
{
    name = scope->engine->newString(QLatin1String("parseFloat"));
}

Value ParseFloatFunction::call(ExecutionContext *context, Value thisObject, Value *args, int argc)
{
    Q_UNUSED(context);
    Q_UNUSED(thisObject);

    Value string = (argc > 0) ? args[0] : Value::undefinedValue();

    // [15.1.2.3] step by step:
    String *inputString = string.toString(context); // 1
    QString trimmed = inputString->toQString().trimmed(); // 2

    // 4:
    if (trimmed.startsWith(QLatin1String("Infinity"))
            || trimmed.startsWith(QLatin1String("+Infinity")))
        return Value::fromDouble(INFINITY);
    if (trimmed.startsWith("-Infinity"))
        return Value::fromDouble(-INFINITY);
    QByteArray ba = trimmed.toLatin1();
    bool ok;
    const char *begin = ba.constData();
    const char *end = 0;
    double d = qstrtod(begin, &end, &ok);
    if (end - begin == 0)
        return Value::fromDouble(nan("")); // 3
    else
        return Value::fromDouble(d);
}

/// isNaN [15.1.2.4]
IsNaNFunction::IsNaNFunction(ExecutionContext *scope)
    : FunctionObject(scope)
{
    name = scope->engine->newString(QLatin1String("isNaN"));
}

Value IsNaNFunction::call(ExecutionContext *context, Value /*thisObject*/, Value *args, int argc)
{
    const Value &v = (argc > 0) ? args[0] : Value::undefinedValue();
    if (v.integerCompatible())
        return Value::fromBoolean(false);

    double d = v.toNumber(context);
    return Value::fromBoolean(std::isnan(d));
}

/// isFinite [15.1.2.5]
IsFiniteFunction::IsFiniteFunction(ExecutionContext *scope)
    : FunctionObject(scope)
{
    name = scope->engine->newString(QLatin1String("isFinite"));
}

Value IsFiniteFunction::call(ExecutionContext *context, Value /*thisObject*/, Value *args, int argc)
{
    const Value &v = (argc > 0) ? args[0] : Value::undefinedValue();
    if (v.integerCompatible())
        return Value::fromBoolean(true);

    double d = v.toNumber(context);
    return Value::fromBoolean(std::isfinite(d));
}


RegExpObject::RegExpObject(ExecutionEngine *engine, PassRefPtr<RegExp> value, bool global)
    : value(value)
    , global(global)
{
    if (!members)
        members.reset(new PropertyTable());
    lastIndexProperty = members->insert(engine->identifier("lastIndex"));
    lastIndexProperty->type = PropertyDescriptor::Data;
    lastIndexProperty->writable = PropertyDescriptor::Enabled;
    lastIndexProperty->enumberable = PropertyDescriptor::Disabled;
    lastIndexProperty->configurable = PropertyDescriptor::Disabled;
    lastIndexProperty->value = Value::fromInt32(0);
}

Value RegExpObject::__get__(ExecutionContext *ctx, String *name, bool *hasProperty)
{
    QString n = name->toQString();
    Value v = Value::undefinedValue();
    if (n == QLatin1String("source"))
        v = Value::fromString(ctx, value->pattern());
    else if (n == QLatin1String("global"))
        v = Value::fromBoolean(global);
    else if (n == QLatin1String("ignoreCase"))
        v = Value::fromBoolean(value->ignoreCase());
    else if (n == QLatin1String("multiline"))
        v = Value::fromBoolean(value->multiLine());
    if (v.type() != Value::Undefined_Type) {
        if (hasProperty)
            *hasProperty = true;
        return v;
    }
    return Object::__get__(ctx, name, hasProperty);
}

Value ErrorObject::__get__(ExecutionContext *ctx, String *name, bool *hasProperty)
{
    QString n = name->toQString();
    if (n == QLatin1String("message")) {
        if (hasProperty)
            *hasProperty = true;
        return value;
    }
    return Object::__get__(ctx, name, hasProperty);
}

void ErrorObject::setNameProperty(ExecutionContext *ctx)
{
    defineDefaultProperty(ctx, QLatin1String("name"), Value::fromString(ctx, className()));
}

void ErrorObject::getCollectables(QVector<Object *> &objects)
{
    Object::getCollectables(objects);
    if (Object *o = value.asObject())
        objects.append(o);
}

SyntaxErrorObject::SyntaxErrorObject(ExecutionContext *ctx, DiagnosticMessage *message)
    : ErrorObject(ctx->argument(0))
    , msg(message)
{
    prototype = ctx->engine->syntaxErrorPrototype;
    if (message)
        value = Value::fromString(message->buildFullMessage(ctx));
    setNameProperty(ctx);
}


ArgumentsObject::ArgumentsObject(ExecutionContext *context, int formalParameterCount, int actualParameterCount)
    : context(context)
    , currentIndex(-1)
{
    defineDefaultProperty(context->engine->id_length, Value::fromInt32(actualParameterCount));
    if (context->strictMode) {
        for (uint i = 0; i < context->argumentCount; ++i)
            Object::__put__(context, QString::number(i), context->arguments[i]);
        FunctionObject *thrower = context->engine->newBuiltinFunction(context, 0, __qmljs_throw_type_error);
        PropertyDescriptor pd = PropertyDescriptor::fromAccessor(thrower, thrower);
        pd.configurable = PropertyDescriptor::Disabled;
        pd.enumberable = PropertyDescriptor::Disabled;
        __defineOwnProperty__(context, QStringLiteral("callee"), &pd);
        __defineOwnProperty__(context, QStringLiteral("caller"), &pd);
    } else {
        FunctionObject *get = context->engine->newBuiltinFunction(context, 0, method_getArg);
        FunctionObject *set = context->engine->newBuiltinFunction(context, 0, method_setArg);
        PropertyDescriptor pd = PropertyDescriptor::fromAccessor(get, set);
        pd.configurable = PropertyDescriptor::Enabled;
        pd.enumberable = PropertyDescriptor::Enabled;
        uint enumerableParams = qMin(formalParameterCount, actualParameterCount);
        for (uint i = 0; i < (uint)enumerableParams; ++i)
            __defineOwnProperty__(context, i, &pd);
        pd.type = PropertyDescriptor::Data;
        pd.writable = PropertyDescriptor::Enabled;
        for (uint i = enumerableParams; i < qMin((uint)actualParameterCount, context->argumentCount); ++i) {
            pd.value = context->argument(i);
            __defineOwnProperty__(context, i, &pd);
        }
        defineDefaultProperty(context, QStringLiteral("callee"), Value::fromObject(context->function));
    }
}

Value ArgumentsObject::__get__(ExecutionContext *ctx, uint index, bool *hasProperty)
{
    if (!ctx->strictMode)
        currentIndex = index;
    Value result = Object::__get__(ctx, index, hasProperty);
    currentIndex = -1;
    return result;
}

void ArgumentsObject::__put__(ExecutionContext *ctx, uint index, Value value)
{
    if (!ctx->strictMode)
        currentIndex = index;
    Object::__put__(ctx, index, value);
    currentIndex = -1;
}

Value ArgumentsObject::method_getArg(ExecutionContext *ctx)
{
    Object *that = ctx->thisObject.asObject();
    if (!that)
        __qmljs_throw_type_error(ctx);
    ArgumentsObject *args = that->asArgumentsObject();
    if (!args)
        __qmljs_throw_type_error(ctx);

    assert(ctx != args->context);
    assert(args->currentIndex >= 0 && args->currentIndex < (int)args->context->argumentCount);
    return args->context->argument(args->currentIndex);
}

Value ArgumentsObject::method_setArg(ExecutionContext *ctx)
{
    Object *that = ctx->thisObject.asObject();
    if (!that)
        __qmljs_throw_type_error(ctx);
    ArgumentsObject *args = that->asArgumentsObject();
    if (!args)
        __qmljs_throw_type_error(ctx);

    assert(ctx != args->context);
    assert(args->currentIndex >= 0 && args->currentIndex < (int)args->context->argumentCount);
    args->context->arguments[args->currentIndex] = ctx->arguments[0];
    return Value::undefinedValue();
}

BuiltinFunction::BuiltinFunction(ExecutionContext *scope, String *name, Value (*code)(ExecutionContext *))
    : FunctionObject(scope)
    , code(code)
{
    this->name = name;
}

Value BuiltinFunction::construct(ExecutionContext *ctx)
{
    ctx->throwTypeError();
    return Value::undefinedValue();
}

void BuiltinFunction::maybeAdjustThisObjectForDirectCall(ExecutionContext *context, Value thisArg)
{
    // Built-in functions allow for the this object to be null or undefined. This overrides
    // the behaviour of changing thisObject to the global object if null/undefined and allows
    // the built-in functions for example to throw a type error if null is passed.
    if (thisArg.isNull() || thisArg.isUndefined())
        context->thisObject = thisArg;
}


BoundFunction::BoundFunction(ExecutionContext *scope, FunctionObject *target, Value boundThis, const QVector<Value> &boundArgs)
    : FunctionObject(scope)
    , target(target)
    , boundThis(boundThis)
    , boundArgs(boundArgs)
{
    prototype = scope->engine->functionPrototype;

    int len = target->__get__(scope, scope->engine->id_length).toUInt32(scope);
    len -= boundArgs.size();
    if (len < 0)
        len = 0;
    defineReadonlyProperty(scope->engine->id_length, Value::fromInt32(len));

    FunctionObject *thrower = scope->engine->newBuiltinFunction(scope, 0, __qmljs_throw_type_error);
    PropertyDescriptor pd = PropertyDescriptor::fromAccessor(thrower, thrower);
    pd.configurable = PropertyDescriptor::Disabled;
    pd.enumberable = PropertyDescriptor::Disabled;
    *members->insert(scope->engine->id_arguments) = pd;
    *members->insert(scope->engine->id_caller) = pd;
}

Value BoundFunction::call(ExecutionContext *context, Value, Value *args, int argc)
{
    Value *newArgs = static_cast<Value *>(alloca(sizeof(Value)*(boundArgs.size() + argc)));
    memcpy(newArgs, boundArgs.constData(), boundArgs.size()*sizeof(Value));
    memcpy(newArgs + boundArgs.size(), args, argc*sizeof(Value));

    return target->call(context, boundThis, newArgs, boundArgs.size() + argc);
}

Value BoundFunction::construct(ExecutionContext *context, Value *args, int argc)
{
    Value *newArgs = static_cast<Value *>(alloca(sizeof(Value)*(boundArgs.size() + argc)));
    memcpy(newArgs, boundArgs.constData(), boundArgs.size()*sizeof(Value));
    memcpy(newArgs + boundArgs.size(), args, argc*sizeof(Value));

    return target->construct(context, newArgs, boundArgs.size() + argc);
}

bool BoundFunction::hasInstance(ExecutionContext *ctx, const Value &value)
{
    return target->hasInstance(ctx, value);
}

void BoundFunction::getCollectables(QVector<Object *> &objects)
{
    FunctionObject::getCollectables(objects);
    objects.append(target);
    if (Object *o = boundThis.asObject())
        objects.append(o);
    for (int i = 0; i < boundArgs.size(); ++i)
        if (Object *o = boundArgs.at(i).asObject())
            objects.append(o);
}


StringObject::StringObject(ExecutionContext *ctx, const Value &value)
    : value(value)
{
    isString = true;

    tmpProperty.type = PropertyDescriptor::Data;
    tmpProperty.enumberable = PropertyDescriptor::Enabled;
    tmpProperty.writable = PropertyDescriptor::Disabled;
    tmpProperty.configurable = PropertyDescriptor::Disabled;
    tmpProperty.value = Value::undefinedValue();

    assert(value.isString());
    defineReadonlyProperty(ctx->engine->id_length, Value::fromUInt32(value.stringValue()->toQString().length()));
}

PropertyDescriptor *StringObject::getIndex(ExecutionContext *ctx, uint index)
{
    QString str = value.stringValue()->toQString();
    if (index >= (uint)str.length())
        return 0;
    String *result = ctx->engine->newString(str.mid(index, 1));
    tmpProperty.value = Value::fromString(result);
    return &tmpProperty;
}

EvalErrorObject::EvalErrorObject(ExecutionContext *ctx)
    : ErrorObject(ctx->argument(0))
{
    setNameProperty(ctx);
    prototype = ctx->engine->evalErrorPrototype;
}

RangeErrorObject::RangeErrorObject(ExecutionContext *ctx)
    : ErrorObject(ctx->argument(0))
{
    setNameProperty(ctx);
    prototype = ctx->engine->rangeErrorPrototype;
}

RangeErrorObject::RangeErrorObject(ExecutionContext *ctx, const QString &msg)
    : ErrorObject(Value::fromString(ctx,msg))
{
    setNameProperty(ctx);
    prototype = ctx->engine->rangeErrorPrototype;
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionContext *ctx)
    : ErrorObject(ctx->argument(0))
{
    setNameProperty(ctx);
    prototype = ctx->engine->referenceErrorPrototype;
}

ReferenceErrorObject::ReferenceErrorObject(ExecutionContext *ctx, const QString &msg)
    : ErrorObject(Value::fromString(ctx,msg))
{
    setNameProperty(ctx);
    prototype = ctx->engine->referenceErrorPrototype;
}

TypeErrorObject::TypeErrorObject(ExecutionContext *ctx)
    : ErrorObject(ctx->argument(0))
{
    setNameProperty(ctx);
    prototype = ctx->engine->typeErrorPrototype;
}

TypeErrorObject::TypeErrorObject(ExecutionContext *ctx, const QString &msg)
    : ErrorObject(Value::fromString(ctx,msg))
{
    setNameProperty(ctx);
    prototype = ctx->engine->typeErrorPrototype;
}

URIErrorObject::URIErrorObject(ExecutionContext *ctx)
    : ErrorObject(ctx->argument(0))
{
    setNameProperty(ctx);
    prototype = ctx->engine->uRIErrorPrototype;
}
