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

#include "qv4object.h"
#include "qv4ir_p.h"
#include "qv4isel_p.h"
#include "qv4objectproto.h"
#include "qv4stringobject.h"
#include "qv4argumentsobject.h"
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
    __put__(ctx, ctx->engine->newString(name), value);
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

void Object::inplaceBinOp(Value rhs, String *name, BinOp op, ExecutionContext *ctx)
{
    bool hasProperty = false;
    Value v = __get__(ctx, name, &hasProperty);
    Value result = op(v, rhs, ctx);
    __put__(ctx, name, result);
}

void Object::inplaceBinOp(Value rhs, Value index, BinOp op, ExecutionContext *ctx)
{
    uint idx = index.asArrayIndex();
    if (idx < UINT_MAX) {
        bool hasProperty = false;
        Value v = __get__(ctx, idx, &hasProperty);
        v = op(v, rhs, ctx);
        __put__(ctx, idx, v);
        return;
    }
    String *name = index.toString(ctx);
    assert(name);
    inplaceBinOp(rhs, name, op, ctx);
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
    defineDefaultProperty(context->engine->newIdentifier(name), value);
}

void Object::defineDefaultProperty(ExecutionContext *context, const QString &name, Value (*code)(ExecutionContext *), int argumentCount)
{
    Q_UNUSED(argumentCount);
    String *s = context->engine->newIdentifier(name);
    FunctionObject* function = context->engine->newBuiltinFunction(context, s, code);
    function->defineReadonlyProperty(context->engine->id_length, Value::fromInt32(argumentCount));
    defineDefaultProperty(s, Value::fromObject(function));
}

void Object::defineDefaultProperty(ExecutionContext *context, const QString &name, Value (*code)(ExecutionContext *, Value, Value *, int), int argumentCount)
{
    Q_UNUSED(argumentCount);
    String *s = context->engine->newIdentifier(name);
    FunctionObject* function = context->engine->newBuiltinFunction(context, s, code);
    function->defineReadonlyProperty(context->engine->id_length, Value::fromInt32(argumentCount));
    defineDefaultProperty(s, Value::fromObject(function));
}

void Object::defineReadonlyProperty(ExecutionEngine *engine, const QString &name, Value value)
{
    defineReadonlyProperty(engine->newIdentifier(name), value);
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

void Object::markObjects()
{
    if (prototype)
        prototype->mark();

    if (members) {
        for (PropertyTable::iterator it = members->begin(), eit = members->end(); it < eit; ++it) {
            if (!(*it))
                continue;
            (*it)->name->mark();
        }
        for (int i = 0; i < (uint)members->values.size(); ++i) {
            const PropertyDescriptor &pd = members->values.at(i);
            if (pd.isData()) {
                if (Managed *m = pd.value.asManaged())
                    m->mark();
             } else if (pd.isAccessor()) {
                if (pd.get)
                    pd.get->mark();
                if (pd.set)
                    pd.set->mark();
            }
        }
    }
    array.markObjects();
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
    if (isStringObject())
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
        if (o->isStringObject()) {
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

    name->makeIdentifier(ctx);

    if (name->isEqualTo(ctx->engine->id___proto__)) {
        if (hasProperty)
            *hasProperty = true;
        return Value::fromObject(prototype);
    }

    Object *o = this;
    while (o) {
        if (o->members) {
            if (PropertyDescriptor *p = o->members->find(name)) {
                if (hasProperty)
                    *hasProperty = true;
                return getValue(ctx, p);
            }
        }
        o = o->prototype;
    }

    if (hasProperty)
        *hasProperty = false;
    return Value::undefinedValue();
}

Value Object::__get__(ExecutionContext *ctx, uint index, bool *hasProperty)
{
    PropertyDescriptor *pd = 0;
    Object *o = this;
    while (o) {
        PropertyDescriptor *p = o->array.at(index);
        if (p && p->type != PropertyDescriptor::Generic) {
            pd = p;
            break;
        }
        if (o->isStringObject()) {
            p = static_cast<StringObject *>(o)->getIndex(ctx, index);
            if (p) {
                pd = p;
                break;
            }
        }
        o = o->prototype;
    }

    if (pd) {
        if (hasProperty)
            *hasProperty = true;
        return getValue(ctx, pd);
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

    name->makeIdentifier(ctx);

    PropertyDescriptor *pd  = __getOwnProperty__(ctx, name);
    // clause 1
    if (pd) {
        if (pd->isAccessor()) {
                if (pd->set)
                    goto cont;
                goto reject;
        } else if (!pd->isWritable())
            goto reject;
        else if (isArrayObject() && name->isEqualTo(ctx->engine->id_length)) {
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

    name->makeIdentifier(ctx);

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

    name->makeIdentifier(ctx);

    if (members) {
        if (PropertyTableEntry *entry = members->findEntry(name)) {
            if (members->values[entry->valueIndex].isConfigurable()) {
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
bool Object::__defineOwnProperty__(ExecutionContext *ctx, String *name, const PropertyDescriptor *desc)
{
    uint idx = name->asArrayIndex();
    if (idx != String::InvalidArrayIndex)
        return __defineOwnProperty__(ctx, idx, desc);

    name->makeIdentifier(ctx);

    PropertyDescriptor *current;

    if (isArrayObject() && name->isEqualTo(ctx->engine->id_length)) {
        PropertyDescriptor *lp = members->values.data(); // length is always the first property
        assert(lp == members->find(ctx->engine->id_length));
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

bool Object::__defineOwnProperty__(ExecutionContext *ctx, uint index, const PropertyDescriptor *desc)
{
    PropertyDescriptor *current;

    // 15.4.5.1, 4b
    if (isArrayObject() && index >= array.length() && !members->values.at(0).isWritable())
        goto reject;

    if (isNonStrictArgumentsObject)
        return static_cast<ArgumentsObject *>(this)->defineOwnProperty(ctx, index, desc);

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

bool Object::__defineOwnProperty__(ExecutionContext *ctx, PropertyDescriptor *current, const PropertyDescriptor *desc)
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


bool Object::__defineOwnProperty__(ExecutionContext *ctx, const QString &name, const PropertyDescriptor *desc)
{
    return __defineOwnProperty__(ctx, ctx->engine->newString(name), desc);
}


void ArrayObject::init(ExecutionContext *context)
{
    type = Type_ArrayObject;

    if (!members)
        members.reset(new PropertyTable());
    PropertyDescriptor *pd = members->insert(context->engine->id_length);
    pd->type = PropertyDescriptor::Data;
    pd->writable = PropertyDescriptor::Enabled;
    pd->enumberable = PropertyDescriptor::Disabled;
    pd->configurable = PropertyDescriptor::Disabled;
    pd->value = Value::fromInt32(0);
    array.setArrayObject(this);
    assert(pd = members->values.data());
}



void ForEachIteratorObject::markObjects()
{
    Object::markObjects();
    if (it.object)
        it.object->mark();
}

