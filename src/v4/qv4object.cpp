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
    delete memberData;
    delete sparseArray;
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
    PropertyDescriptor *pd = insertMember(name);
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
    PropertyDescriptor *pd = insertMember(name);
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
        for (int i = 0; i < memberDataSize; ++i) {
            const PropertyDescriptor &pd = memberData[i];
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
    markArrayObjects();
}

PropertyDescriptor *Object::insertMember(String *s)
{
    if (!members)
        members.reset(new PropertyTable);

    PropertyTableEntry *e = members->insert(s);
    if (e->valueIndex == UINT_MAX) {
        if (memberDataSize == memberDataAlloc) {
            memberDataAlloc = qMax((uint)8, 2*memberDataAlloc);
            PropertyDescriptor *newMemberData = new PropertyDescriptor[memberDataAlloc];
            memcpy(newMemberData, memberData, sizeof(PropertyDescriptor)*memberDataSize);
            delete memberData;
            memberData = newMemberData;
        }
        e->valueIndex = memberDataSize;
        ++memberDataSize;
    }
    return memberData + e->valueIndex;
}

// Section 8.12.1
PropertyDescriptor *Object::__getOwnProperty__(ExecutionContext *ctx, String *name)
{
    uint idx = name->asArrayIndex();
    if (idx != String::InvalidArrayIndex)
        return __getOwnProperty__(ctx, idx);

    if (members) {
        uint idx = members->find(name);
        if (idx < UINT_MAX)
            return memberData + idx;
    }
    return 0;
}

PropertyDescriptor *Object::__getOwnProperty__(ExecutionContext *ctx, uint index)
{
    PropertyDescriptor *p = arrayAt(index);
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
            uint idx = o->members->find(name);
            if (idx < UINT_MAX)
                return o->memberData + idx;
        }
        o = o->prototype;
    }
    return 0;
}

PropertyDescriptor *Object::__getPropertyDescriptor__(ExecutionContext *ctx, uint index)
{
    Object *o = this;
    while (o) {
        PropertyDescriptor *p = o->arrayAt(index);
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
            uint idx = o->members->find(name);
            if (idx < UINT_MAX) {
                if (hasProperty)
                    *hasProperty = true;
                return getValue(ctx, o->memberData + idx);
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
        PropertyDescriptor *p = o->arrayAt(index);
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
            ok = setArrayLength(l);
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
        PropertyDescriptor *p = insertMember(name);
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

    arraySet(index, value);
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

    if (members && members->find(name) != UINT_MAX)
        return true;

    return prototype ? prototype->__hasProperty__(ctx, name) : false;
}

bool Object::__hasProperty__(const ExecutionContext *ctx, uint index) const
{
    const PropertyDescriptor *p = arrayAt(index);
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
            PropertyDescriptor &pd = memberData[entry->valueIndex];
            if (pd.isConfigurable()) {
                members->remove(entry);
                // ### leaves a hole in memberData
                pd.type = PropertyDescriptor::Generic;
                pd.writable = PropertyDescriptor::Undefined;
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
    if (deleteArrayIndex(index))
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
        PropertyDescriptor *lp = memberData + ArrayObject::LengthPropertyIndex;
        assert(0 == members->find(ctx->engine->id_length));
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
            succeeded = setArrayLength(l);
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
        PropertyDescriptor *pd = insertMember(name);
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
    if (isArrayObject() && index >= arrayLength() && !memberData[ArrayObject::LengthPropertyIndex].isWritable())
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
        PropertyDescriptor *pd = arrayInsert(index);
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


void Object::copyArrayData(Object *other)
{
    arrayLen = other->arrayLen;
    arrayData = other->arrayData;
    arrayFreeList = other->arrayFreeList;
    if (other->sparseArray)
        sparseArray = new SparseArray(*other->sparseArray);
    if (isArrayObject())
        setArrayLengthUnchecked(arrayLen);
}


Value Object::arrayIndexOf(Value v, uint fromIndex, uint endIndex, ExecutionContext *ctx, Object *o)
{
    bool protoHasArray = false;
    Object *p = o;
    while ((p = p->prototype))
        if (p->arrayLength())
            protoHasArray = true;

    if (protoHasArray) {
        // lets be safe and slow
        for (uint i = fromIndex; i < endIndex; ++i) {
            bool exists;
            Value value = o->__get__(ctx, i, &exists);
            if (exists && __qmljs_strict_equal(value, v))
                return Value::fromDouble(i);
        }
    } else if (sparseArray) {
        for (SparseArrayNode *n = sparseArray->lowerBound(fromIndex); n && n->key() < endIndex; n = n->nextNode()) {
            bool exists;
            Value value = o->getValueChecked(ctx, arrayDecriptor(n->value), &exists);
            if (exists && __qmljs_strict_equal(value, v))
                return Value::fromDouble(n->key());
        }
    } else {
        if ((int) endIndex > arrayData.size())
            endIndex = arrayData.size();
        PropertyDescriptor *pd = arrayData.data() + arrayOffset;
        PropertyDescriptor *end = pd + endIndex;
        pd += fromIndex;
        while (pd < end) {
            bool exists;
            Value value = o->getValueChecked(ctx, pd, &exists);
            if (exists && __qmljs_strict_equal(value, v))
                return Value::fromDouble(pd - arrayOffset - arrayData.constData());
            ++pd;
        }
    }
    return Value::fromInt32(-1);
}

void Object::arrayConcat(const ArrayObject *other)
{
    int newLen = arrayLen + other->arrayLength();
    if (other->sparseArray)
        initSparse();
    if (sparseArray) {
        if (other->sparseArray) {
            for (const SparseArrayNode *it = other->sparseArray->begin(); it != other->sparseArray->end(); it = it->nextNode())
                arraySet(arrayLen + it->key(), other->arrayDecriptor(it->value));
        } else {
            int oldSize = arrayData.size();
            arrayData.resize(oldSize + other->arrayLength());
            memcpy(arrayData.data() + oldSize, other->arrayData.constData() + other->arrayOffset, other->arrayLength()*sizeof(PropertyDescriptor));
            for (uint i = 0; i < other->arrayLength(); ++i) {
                SparseArrayNode *n = sparseArray->insert(arrayLen + i);
                n->value = oldSize + i;
            }
        }
    } else {
        int oldSize = arrayData.size();
        arrayData.resize(oldSize + other->arrayLength());
        memcpy(arrayData.data() + oldSize, other->arrayData.constData() + other->arrayOffset, other->arrayLength()*sizeof(PropertyDescriptor));
    }
    setArrayLengthUnchecked(newLen);
}

void Object::arraySort(ExecutionContext *context, Object *thisObject, const Value &comparefn, uint len)
{
    if (sparseArray) {
        context->throwUnimplemented("Object::sort unimplemented for sparse arrays");
        return;
        delete sparseArray;
    }

    ArrayElementLessThan lessThan(context, thisObject, comparefn);
    if (len > arrayData.size() - arrayOffset)
        len = arrayData.size() - arrayOffset;
    PropertyDescriptor *begin = arrayData.begin() + arrayOffset;
    std::sort(begin, begin + len, lessThan);
}


void Object::initSparse()
{
    if (!sparseArray) {
        sparseArray = new SparseArray;
        for (int i = arrayOffset; i < arrayData.size(); ++i) {
            SparseArrayNode *n = sparseArray->insert(i - arrayOffset);
            n->value = i;
        }

        if (arrayOffset) {
            int o = arrayOffset;
            for (int i = 0; i < o - 1; ++i) {
                arrayData[i].type = PropertyDescriptor::Generic;
                arrayData[i].value = Value::fromInt32(i + 1);
            }
            arrayData[o - 1].type = PropertyDescriptor::Generic;
            arrayData[o - 1].value = Value::fromInt32(arrayData.size());
            arrayFreeList = 0;
        } else {
            arrayFreeList = arrayData.size();
        }
    }
}

void Object::setArrayLengthUnchecked(uint l)
{
    arrayLen = l;
    if (isArrayObject()) {
        // length is always the first property of an array
        PropertyDescriptor &lengthProperty = memberData[ArrayObject::LengthPropertyIndex];
        lengthProperty.value = Value::fromUInt32(l);
    }
}

bool Object::setArrayLength(uint newLen) {
    assert(isArrayObject());
    const PropertyDescriptor *lengthProperty = memberData + ArrayObject::LengthPropertyIndex;
    if (lengthProperty && !lengthProperty->isWritable())
        return false;
    uint oldLen = arrayLength();
    bool ok = true;
    if (newLen < oldLen) {
        if (sparseArray) {
            SparseArrayNode *begin = sparseArray->lowerBound(newLen);
            SparseArrayNode *it = sparseArray->end()->previousNode();
            while (1) {
                PropertyDescriptor &pd = arrayData[it->value];
                if (pd.type != PropertyDescriptor::Generic && !pd.isConfigurable()) {
                    ok = false;
                    newLen = it->key() + 1;
                    break;
                }
                pd.type = PropertyDescriptor::Generic;
                pd.value.tag = Value::_Undefined_Type;
                pd.value.int_32 = arrayFreeList;
                arrayFreeList = it->value;
                bool brk = (it == begin);
                SparseArrayNode *prev = it->previousNode();
                sparseArray->erase(it);
                if (brk)
                    break;
                it = prev;
            }
        } else {
            PropertyDescriptor *it = arrayData.data() + arrayData.size();
            const PropertyDescriptor *begin = arrayData.constData() + arrayOffset + newLen;
            while (--it >= begin) {
                if (it->type != PropertyDescriptor::Generic && !it->isConfigurable()) {
                    ok = false;
                    newLen = it - arrayData.data() + arrayOffset + 1;
                    break;
                }
            }
            arrayData.resize(newLen + arrayOffset);
        }
    } else {
        if (newLen >= 0x100000)
            initSparse();
    }
    setArrayLengthUnchecked(newLen);
    return ok;
}

void Object::markArrayObjects() const
{
    uint i = sparseArray ? 0 : arrayOffset;
    for (; i < (uint)arrayData.size(); ++i) {
        const PropertyDescriptor &pd = arrayData.at(i);
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


void ArrayObject::init(ExecutionContext *context)
{
    type = Type_ArrayObject;

    if (!members)
        members.reset(new PropertyTable());
    PropertyDescriptor *pd = insertMember(context->engine->id_length);
    assert(pd == memberData + LengthPropertyIndex);
    pd->type = PropertyDescriptor::Data;
    pd->writable = PropertyDescriptor::Enabled;
    pd->enumberable = PropertyDescriptor::Disabled;
    pd->configurable = PropertyDescriptor::Disabled;
    pd->value = Value::fromInt32(0);
}



void ForEachIteratorObject::markObjects()
{
    Object::markObjects();
    if (it.object)
        it.object->mark();
}

