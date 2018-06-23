/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4object_p.h"
#include "qv4objectproto_p.h"
#include "qv4stringobject_p.h"
#include "qv4argumentsobject_p.h"
#include <private/qv4mm_p.h>
#include "qv4lookup_p.h"
#include "qv4scopedvalue_p.h"
#include "qv4memberdata_p.h"
#include "qv4objectiterator_p.h"
#include "qv4identifier_p.h"
#include "qv4string_p.h"
#include "qv4identifiertable_p.h"
#include "qv4jscall_p.h"
#include "qv4symbol_p.h"

#include <stdint.h>

using namespace QV4;

DEFINE_OBJECT_VTABLE(Object);

void Object::setInternalClass(Heap::InternalClass *ic)
{
    d()->internalClass.set(engine(), ic);
    if (ic->isUsedAsProto)
        ic->updateProtoUsage(d());
    Q_ASSERT(ic && ic->vtable);
    uint nInline = d()->vtable()->nInlineProperties;
    if (ic->size <= nInline)
        return;
    bool hasMD = d()->memberData != nullptr;
    uint requiredSize = ic->size - nInline;
    if (!(hasMD && requiredSize) || (hasMD && d()->memberData->values.size < requiredSize))
        d()->memberData.set(ic->engine, MemberData::allocate(ic->engine, requiredSize, d()->memberData));
}

void Object::getProperty(uint index, Property *p, PropertyAttributes *attrs) const
{
    p->value = *propertyData(index);
    *attrs = internalClass()->propertyData.at(index);
    if (attrs->isAccessor())
        p->set = *propertyData(index + SetterOffset);
}

void Object::setProperty(uint index, const Property *p)
{
    setProperty(index, p->value);
    if (internalClass()->propertyData.at(index).isAccessor())
        setProperty(index + SetterOffset, p->set);
}

void Heap::Object::setUsedAsProto()
{
    internalClass.set(internalClass->engine, internalClass->asProtoClass());
}

ReturnedValue Object::getValue(const Value &thisObject, const Value &v, PropertyAttributes attrs)
{
    if (!attrs.isAccessor())
        return v.asReturnedValue();
    const QV4::FunctionObject *f = v.as<FunctionObject>();
    if (!f)
        return Encode::undefined();

    Scope scope(f->engine());
    JSCallData jsCallData(scope);
    *jsCallData->thisObject = thisObject;
    return f->call(jsCallData);
}

bool Object::putValue(uint memberIndex, const Value &value)
{
    Heap::InternalClass *ic = internalClass();
    if (ic->engine->hasException)
        return false;

    PropertyAttributes attrs = ic->propertyData[memberIndex];

    if (attrs.isAccessor()) {
        const FunctionObject *set = propertyData(memberIndex + SetterOffset)->as<FunctionObject>();
        if (set) {
            Scope scope(ic->engine);
            ScopedFunctionObject setter(scope, set);
            JSCallData jsCallData(scope, 1);
            jsCallData->args[0] = value;
            *jsCallData->thisObject = this;
            setter->call(jsCallData);
            return !ic->engine->hasException;
        }
        return false;
    }

    if (!attrs.isWritable())
        return false;

    setProperty(memberIndex, value);
    return true;
}

void Object::defineDefaultProperty(const QString &name, const Value &value, PropertyAttributes attributes)
{
    ExecutionEngine *e = engine();
    Scope scope(e);
    ScopedString s(scope, e->newIdentifier(name));
    defineDefaultProperty(s, value, attributes);
}

void Object::defineDefaultProperty(const QString &name, jsCallFunction code,
                                   int argumentCount, PropertyAttributes attributes)
{
    ExecutionEngine *e = engine();
    Scope scope(e);
    ScopedString s(scope, e->newIdentifier(name));
    ScopedFunctionObject function(scope, FunctionObject::createBuiltinFunction(e, s, code, argumentCount));
    defineDefaultProperty(s, function, attributes);
}

void Object::defineDefaultProperty(StringOrSymbol *nameOrSymbol, jsCallFunction code,
                                   int argumentCount, PropertyAttributes attributes)
{
    ExecutionEngine *e = engine();
    Scope scope(e);
    ScopedFunctionObject function(scope, FunctionObject::createBuiltinFunction(e, nameOrSymbol, code, argumentCount));
    defineDefaultProperty(nameOrSymbol, function, attributes);
}

void Object::defineAccessorProperty(const QString &name, jsCallFunction getter, jsCallFunction setter)
{
    ExecutionEngine *e = engine();
    Scope scope(e);
    ScopedString s(scope, e->newIdentifier(name));
    defineAccessorProperty(s, getter, setter);
}

void Object::defineAccessorProperty(StringOrSymbol *name, jsCallFunction getter, jsCallFunction setter)
{
    ExecutionEngine *v4 = engine();
    QV4::Scope scope(v4);
    ScopedProperty p(scope);
    QString n = name->toQString();
    if (n.at(0) == QLatin1Char('@'))
        n = QChar::fromLatin1('[') + n.midRef(1) + QChar::fromLatin1(']');
    if (getter) {
        ScopedString getName(scope, v4->newString(QString::fromLatin1("get ") + n));
        p->setGetter(ScopedFunctionObject(scope, FunctionObject::createBuiltinFunction(v4, getName, getter, 0)));
    } else {
        p->setGetter(nullptr);
    }
    if (setter) {
        ScopedString setName(scope, v4->newString(QString::fromLatin1("set ") + n));
        p->setSetter(ScopedFunctionObject(scope, FunctionObject::createBuiltinFunction(v4, setName, setter, 0)));
    } else {
        p->setSetter(nullptr);
    }
    insertMember(name, p, QV4::Attr_Accessor|QV4::Attr_NotEnumerable);
}



void Object::defineReadonlyProperty(const QString &name, const Value &value)
{
    QV4::ExecutionEngine *e = engine();
    Scope scope(e);
    ScopedString s(scope, e->newIdentifier(name));
    defineReadonlyProperty(s, value);
}

void Object::defineReadonlyProperty(String *name, const Value &value)
{
    insertMember(name, value, Attr_ReadOnly);
}

void Object::defineReadonlyConfigurableProperty(const QString &name, const Value &value)
{
    QV4::ExecutionEngine *e = engine();
    Scope scope(e);
    ScopedString s(scope, e->newIdentifier(name));
    defineReadonlyConfigurableProperty(s, value);
}

void Object::defineReadonlyConfigurableProperty(StringOrSymbol *name, const Value &value)
{
    insertMember(name, value, Attr_ReadOnly_ButConfigurable);
}

void Object::addSymbolSpecies()
{
    Scope scope(engine());
    ScopedProperty p(scope);
    p->setGetter(scope.engine->getSymbolSpecies());
    p->setSetter(nullptr);
    insertMember(scope.engine->symbol_species(), p, QV4::Attr_Accessor|QV4::Attr_NotWritable|QV4::Attr_NotEnumerable);
}

void Heap::Object::markObjects(Heap::Base *b, MarkStack *stack)
{
    Base::markObjects(b, stack);
    Object *o = static_cast<Object *>(b);
    if (o->memberData)
        o->memberData->mark(stack);
    if (o->arrayData)
        o->arrayData->mark(stack);
    uint nInline = o->vtable()->nInlineProperties;
    Value *v = reinterpret_cast<Value *>(o) + o->vtable()->inlinePropertyOffset;
    const Value *end = v + nInline;
    while (v < end) {
        v->mark(stack);
        ++v;
    }
}

void Object::insertMember(StringOrSymbol *s, const Property *p, PropertyAttributes attributes)
{
    uint idx;
    s->makeIdentifier();
    Heap::InternalClass::addMember(this, s->identifier(), attributes, &idx);

    if (attributes.isAccessor()) {
        setProperty(idx + GetterOffset, p->value);
        setProperty(idx + SetterOffset, p->set);
    } else {
        setProperty(idx, p->value);
    }
}

void Object::setPrototypeUnchecked(const Object *p)
{
    setInternalClass(internalClass()->changePrototype(p ? p->d() : nullptr));
}

// Section 8.12.2
PropertyIndex Object::getValueOrSetter(Identifier id, PropertyAttributes *attrs)
{
    if (id.isArrayIndex()) {
        uint index = id.asArrayIndex();
        Heap::Object *o = d();
        while (o) {
            if (o->arrayData) {
                uint idx = o->arrayData->mappedIndex(index);
                if (idx != UINT_MAX) {
                    *attrs = o->arrayData->attributes(index);
                    return { o->arrayData , o->arrayData->values.values + (attrs->isAccessor() ? idx + SetterOffset : idx) };
                }
            }
            if (o->vtable()->type == Type_StringObject) {
                if (index < static_cast<const Heap::StringObject *>(o)->length()) {
                    // this is an evil hack, but it works, as the method is only ever called from put,
                    // where we don't use the returned pointer there for non writable attributes
                    *attrs = (Attr_NotWritable|Attr_NotConfigurable);
                    return { reinterpret_cast<Heap::ArrayData *>(0x1), nullptr };
                }
            }
            o = o->prototype();
        }
    } else {
        Heap::Object *o = d();
        while (o) {
            uint idx = o->internalClass->find(id);
            if (idx < UINT_MAX) {
                *attrs = o->internalClass->propertyData[idx];
                return o->writablePropertyData(attrs->isAccessor() ? idx + SetterOffset : idx );
            }

            o = o->prototype();
        }
    }
    *attrs = Attr_Invalid;
    return { nullptr, nullptr };
}

ReturnedValue Object::callAsConstructor(const FunctionObject *f, const Value *, int)
{
    return f->engine()->throwTypeError();
}

ReturnedValue Object::call(const FunctionObject *f, const Value *, const Value *, int)
{
    return f->engine()->throwTypeError();
}

ReturnedValue Object::get(const Managed *m, Identifier id, const Value *receiver, bool *hasProperty)
{
    if (id.isArrayIndex())
        return static_cast<const Object *>(m)->internalGetIndexed(id.asArrayIndex(), receiver, hasProperty);
    Scope scope(m);
    Scoped<StringOrSymbol> name(scope, id.asStringOrSymbol());
    return static_cast<const Object *>(m)->internalGet(name, receiver, hasProperty);
}

bool Object::put(Managed *m, Identifier id, const Value &value, Value *receiver)
{
    return static_cast<Object *>(m)->internalPut(id, value, receiver);
}

bool Object::deleteProperty(Managed *m, Identifier id)
{
    return static_cast<Object *>(m)->internalDeleteProperty(id);
}

void Object::advanceIterator(Managed *m, ObjectIterator *it, Value *name, uint *index, Property *pd, PropertyAttributes *attrs)
{
    Object *o = static_cast<Object *>(m);
    name->setM(nullptr);
    *index = UINT_MAX;

    if (o->arrayData()) {
        if (!it->arrayIndex)
            it->arrayNode = o->sparseBegin();

        // sparse arrays
        if (it->arrayNode) {
            while (it->arrayNode != o->sparseEnd()) {
                int k = it->arrayNode->key();
                uint pidx = it->arrayNode->value;
                Heap::SparseArrayData *sa = o->d()->arrayData.cast<Heap::SparseArrayData>();
                const Property *p = reinterpret_cast<const Property *>(sa->values.data() + pidx);
                it->arrayNode = it->arrayNode->nextNode();
                PropertyAttributes a = sa->attrs ? sa->attrs[pidx] : Attr_Data;
                if (!(it->flags & ObjectIterator::EnumerableOnly) || a.isEnumerable()) {
                    it->arrayIndex = k + 1;
                    *index = k;
                    *attrs = a;
                    pd->copy(p, a);
                    return;
                }
            }
            it->arrayNode = nullptr;
            it->arrayIndex = UINT_MAX;
        }
        // dense arrays
        while (it->arrayIndex < o->d()->arrayData->values.size) {
            Heap::SimpleArrayData *sa = o->d()->arrayData.cast<Heap::SimpleArrayData>();
            const Value &val = sa->data(it->arrayIndex);
            PropertyAttributes a = o->arrayData()->attributes(it->arrayIndex);
            ++it->arrayIndex;
            if (!val.isEmpty()
                && (!(it->flags & ObjectIterator::EnumerableOnly) || a.isEnumerable())) {
                *index = it->arrayIndex - 1;
                *attrs = a;
                pd->value = val;
                return;
            }
        }
    }

    while (it->memberIndex < o->internalClass()->size) {
        Identifier n = o->internalClass()->nameMap.at(it->memberIndex);
        if (!n.isStringOrSymbol() || !n.asStringOrSymbol()->internalClass->vtable->isString) {
            // accessor properties have a dummy entry with n == 0
            // symbol entries are supposed to be skipped
            ++it->memberIndex;
            continue;
        }

        int idx = it->memberIndex;
        PropertyAttributes a = o->internalClass()->propertyData[it->memberIndex];
        ++it->memberIndex;
        if (!(it->flags & ObjectIterator::EnumerableOnly) || a.isEnumerable()) {
            name->setM(n.asStringOrSymbol());
            *attrs = a;
            pd->value = *o->propertyData(idx);
            if (a.isAccessor())
                pd->set = *o->propertyData(idx + SetterOffset);
            return;
        }
    }

    *attrs = PropertyAttributes();
}

// Section 8.12.3
ReturnedValue Object::internalGet(StringOrSymbol *name, const Value *receiver, bool *hasProperty) const
{
    Q_ASSERT(name->asArrayIndex() == UINT_MAX);

    name->makeIdentifier();
    Identifier id = name->identifier();

    Heap::Object *o = d();
    while (o) {
        uint idx = o->internalClass->find(id);
        if (idx < UINT_MAX) {
            if (hasProperty)
                *hasProperty = true;
            return Object::getValue(*receiver, *o->propertyData(idx), o->internalClass->propertyData.at(idx));
        }

        o = o->prototype();
    }

    if (hasProperty)
        *hasProperty = false;
    return Encode::undefined();
}

ReturnedValue Object::internalGetIndexed(uint index, const Value *receiver, bool *hasProperty) const
{
    PropertyAttributes attrs;
    Scope scope(engine());
    ScopedObject o(scope, this);
    ScopedProperty pd(scope);
    bool exists = false;
    while (o) {
        if (o->arrayData() && o->arrayData()->getProperty(index, pd, &attrs)) {
            exists = true;
            break;
        }
        if (o->isStringObject()) {
            ScopedString str(scope, static_cast<StringObject *>(o.getPointer())->getIndex(index));
            if (str) {
                attrs = (Attr_NotWritable|Attr_NotConfigurable);
                if (hasProperty)
                    *hasProperty = true;
                return str.asReturnedValue();
            }
        }
        o = o->getPrototypeOf();
    }

    if (exists) {
        if (hasProperty)
            *hasProperty = true;
        return Object::getValue(*receiver, pd->value, attrs);
    }

    if (hasProperty)
        *hasProperty = false;
    return Encode::undefined();
}


// Section 8.12.5
bool Object::internalPut(Identifier id, const Value &value, Value *receiver)
{
    ExecutionEngine *engine = this->engine();
    if (engine->hasException)
        return false;

    uint index = id.asArrayIndex();
    Scope scope(engine);

    PropertyAttributes attrs;
    PropertyIndex propertyIndex{nullptr, nullptr};

    if (index != UINT_MAX) {
        if (arrayData())
            propertyIndex = arrayData()->getValueOrSetter(index, &attrs);

        if (propertyIndex.isNull() && isStringObject()) {
            if (index < static_cast<StringObject *>(this)->length())
                // not writable
                return false;
        }
    } else {
        uint member = internalClass()->find(id);
        if (member < UINT_MAX) {
            attrs = internalClass()->propertyData[member];
            propertyIndex = d()->writablePropertyData(attrs.isAccessor() ? member + SetterOffset : member);
        }
    }

    // clause 1
    if (!propertyIndex.isNull()) {
        if (attrs.isAccessor()) {
            if (propertyIndex->as<FunctionObject>())
                goto cont;
            return false;
        } else if (!attrs.isWritable())
            return false;
        else if (isArrayObject() && id == engine->id_length()->identifier()) {
            bool ok;
            uint l = value.asArrayLength(&ok);
            if (!ok) {
                engine->throwRangeError(value);
                return false;
            }
            ok = setArrayLength(l);
            if (!ok)
                return false;
        } else {
            propertyIndex.set(engine, value);
        }
        return true;
    } else if (!getPrototypeOf()) {
        if (!isExtensible())
            return false;
    } else {
        // clause 4
        propertyIndex = ScopedObject(scope, getPrototypeOf())->getValueOrSetter(id, &attrs);
        if (!propertyIndex.isNull()) {
            if (attrs.isAccessor()) {
                if (!propertyIndex->as<FunctionObject>())
                    return false;
            } else if (!isExtensible() || !attrs.isWritable()) {
                return false;
            }
        } else if (!isExtensible()) {
            return false;
        }
    }

    cont:

    // Clause 5
    if (!propertyIndex.isNull() && attrs.isAccessor()) {
        Q_ASSERT(propertyIndex->as<FunctionObject>());

        Scope scope(engine);
        ScopedFunctionObject setter(scope, *propertyIndex);
        JSCallData jsCallData(scope, 1);
        jsCallData->args[0] = value;
        *jsCallData->thisObject = *receiver;
        setter->call(jsCallData);
        return !engine->hasException;
    }

    if (index != UINT_MAX) {
        arraySet(index, value);
    } else {
        Scoped<StringOrSymbol> name(scope, id.asStringOrSymbol());
        insertMember(name, value);
    }
    return true;
}

// Section 8.12.7
bool Object::internalDeleteProperty(Identifier id)
{
    if (internalClass()->engine->hasException)
        return false;

    if (id.isArrayIndex()) {
        uint index = id.asArrayIndex();
        Scope scope(engine());
        if (scope.engine->hasException)
            return false;

        Scoped<ArrayData> ad(scope, arrayData());
        if (!ad || ad->vtable()->del(this, index))
            return true;

        return false;
    }

    uint memberIdx = internalClass()->find(id);
    if (memberIdx != UINT_MAX) {
        if (internalClass()->propertyData[memberIdx].isConfigurable()) {
            Heap::InternalClass::removeMember(this, id);
            return true;
        }
        return false;
    }

    return true;
}

bool Object::internalDefineOwnProperty(ExecutionEngine *engine, uint index, StringOrSymbol *member, const Property *p, PropertyAttributes attrs)
{
    // clause 5
    if (attrs.isEmpty())
        return true;

    Scope scope(engine);
    ScopedProperty current(scope);
    PropertyAttributes cattrs;
    if (member) {
        getProperty(index, current, &cattrs);
        cattrs = internalClass()->propertyData[index];
    } else if (arrayData()) {
        arrayData()->getProperty(index, current, &cattrs);
        cattrs = arrayData()->attributes(index);
    }

    // clause 6
    if (p->isSubset(attrs, current, cattrs))
        return true;

    // clause 7
    if (!cattrs.isConfigurable()) {
        if (attrs.isConfigurable())
            return false;
        if (attrs.hasEnumerable() && attrs.isEnumerable() != cattrs.isEnumerable())
            return false;
    }

    // clause 8
    if (attrs.isGeneric() || current->value.isEmpty())
        goto accept;

    // clause 9
    if (cattrs.isData() != attrs.isData()) {
        // 9a
        if (!cattrs.isConfigurable())
            return false;
        if (cattrs.isData()) {
            // 9b
            cattrs.setType(PropertyAttributes::Accessor);
            cattrs.clearWritable();
            if (!member) {
                // need to convert the array and the slot
                initSparseArray();
                Q_ASSERT(arrayData());
                setArrayAttributes(index, cattrs);
            }
            current->setGetter(nullptr);
            current->setSetter(nullptr);
        } else {
            // 9c
            cattrs.setType(PropertyAttributes::Data);
            cattrs.setWritable(false);
            if (!member) {
                // need to convert the array and the slot
                setArrayAttributes(index, cattrs);
            }
            current->value = Primitive::undefinedValue();
        }
    } else if (cattrs.isData() && attrs.isData()) { // clause 10
        if (!cattrs.isConfigurable() && !cattrs.isWritable()) {
            if (attrs.isWritable() || !current->value.sameValue(p->value))
                return false;
        }
    } else { // clause 10
        Q_ASSERT(cattrs.isAccessor() && attrs.isAccessor());
        if (!cattrs.isConfigurable()) {
            if (!p->value.isEmpty() && current->value.rawValue() != p->value.rawValue())
                return false;
            if (!p->set.isEmpty() && current->set.rawValue() != p->set.rawValue())
                return false;
        }
    }

  accept:

    current->merge(cattrs, p, attrs);
    if (member) {
        Heap::InternalClass::changeMember(this, member->identifier(), cattrs);
        setProperty(index, current);
    } else {
        setArrayAttributes(index, cattrs);
        arrayData()->setProperty(scope.engine, index, current);
    }
    return true;
}

void Object::copyArrayData(Object *other)
{
    Q_ASSERT(isArrayObject());
    Scope scope(engine());

    if (other->protoHasArray() || ArgumentsObject::isNonStrictArgumentsObject(other) ||
        (other->arrayType() == Heap::ArrayData::Sparse && other->arrayData()->attrs)) {
        uint len = other->getLength();
        Q_ASSERT(len);

        ScopedValue v(scope);
        for (uint i = 0; i < len; ++i) {
            arraySet(i, (v = other->get(i)));
        }
    } else if (!other->arrayData()) {
        ;
    } else {
        Q_ASSERT(!arrayData() && other->arrayData());
        ArrayData::realloc(this, static_cast<ArrayData::Type>(other->d()->arrayData->type),
                           other->d()->arrayData->values.alloc, false);
        if (other->arrayType() == Heap::ArrayData::Sparse) {
            Heap::ArrayData *od = other->d()->arrayData;
            Heap::ArrayData *dd = d()->arrayData;
            dd->sparse = new SparseArray(*od->sparse);
        } else {
            Heap::ArrayData *dd = d()->arrayData;
            dd->values.size = other->d()->arrayData->values.size;
            dd->offset = other->d()->arrayData->offset;
        }
        // ### need a write barrier
        memcpy(d()->arrayData->values.values, other->d()->arrayData->values.values, other->d()->arrayData->values.alloc*sizeof(Value));
    }
    setArrayLengthUnchecked(other->getLength());
}

qint64 Object::getLength(const Managed *m)
{
    Scope scope(static_cast<const Object *>(m)->engine());
    ScopedValue v(scope, static_cast<Object *>(const_cast<Managed *>(m))->get(scope.engine->id_length()));
    return v->toLength();
}

// 'var' is 'V' in 15.3.5.3.
ReturnedValue Object::instanceOf(const Object *typeObject, const Value &var)
{
    QV4::ExecutionEngine *engine = typeObject->internalClass()->engine;

    // 15.3.5.3, Assume F is a Function object.
    const FunctionObject *function = typeObject->as<FunctionObject>();
    if (!function)
        return engine->throwTypeError();

    Heap::FunctionObject *f = function->d();
    if (function->isBoundFunction())
        f = function->cast<BoundFunction>()->target();

    // 15.3.5.3, 1: HasInstance can only be used on an object
    const Object *lhs = var.as<Object>();
    if (!lhs)
        return Encode(false);

    // 15.3.5.3, 2
    const Object *o = f->protoProperty();
    if (!o) // 15.3.5.3, 3
        return engine->throwTypeError();

    Heap::Object *v = lhs->d();

    // 15.3.5.3, 4
    while (v) {
        // 15.3.5.3, 4, a
        v = v->prototype();

        // 15.3.5.3, 4, b
        if (!v)
            break; // will return false

        // 15.3.5.3, 4, c
        else if (o->d() == v)
            return Encode(true);
    }

    return Encode(false);
}

bool Object::hasProperty(const Managed *m, Identifier id)
{
    Scope scope(m->engine());
    ScopedObject o(scope, m);
    ScopedProperty p(scope);
    while (o) {
        if (o->getOwnProperty(id, p) != Attr_Invalid)
            return true;

        o = o->getPrototypeOf();
    }

    return false;
}

PropertyAttributes Object::getOwnProperty(Managed *m, Identifier id, Property *p)
{
    PropertyAttributes attrs;
    Object *o = static_cast<Object *>(m);
    if (id.isArrayIndex()) {
        uint index = id.asArrayIndex();
        if (o->arrayData()) {
            if (o->arrayData()->getProperty(index, p, &attrs))
                return attrs;
        }
    } else {
        Q_ASSERT(id.asStringOrSymbol());

        uint member = o->internalClass()->find(id);
        if (member < UINT_MAX) {
            attrs = o->internalClass()->propertyData[member];
            if (p) {
                p->value = *o->propertyData(member);
                if (attrs.isAccessor())
                    p->set = *o->propertyData(member + SetterOffset);
            }
            return attrs;
        }
    }

    return Attr_Invalid;
}

bool Object::defineOwnProperty(Managed *m, Identifier id, const Property *p, PropertyAttributes attrs)
{
    Object *o = static_cast<Object *>(m);
    Scope scope(o);

    if (id.isArrayIndex()) {
        uint index = id.asArrayIndex();

        bool hasProperty = false;

        if (o->arrayData()) {
            hasProperty = o->arrayData()->mappedIndex(index) != UINT_MAX;
            if (!hasProperty && o->isStringObject())
                hasProperty = (index < static_cast<StringObject *>(o)->length());
        }

        if (!hasProperty) {
            if (!o->isExtensible())
                return false;

            ScopedProperty pp(scope);
            pp->copy(p, attrs);
            pp->fullyPopulated(&attrs);
            if (attrs == Attr_Data) {
                ScopedValue v(scope, pp->value);
                o->arraySet(index, v);
            } else {
                o->arraySet(index, pp, attrs);
            }
            return true;
        }

        return o->internalDefineOwnProperty(scope.engine, index, nullptr, p, attrs);
    }

    uint memberIndex = o->internalClass()->find(id);
    Scoped<StringOrSymbol> name(scope, id.asStringOrSymbol());

    if (memberIndex == UINT_MAX) {
        if (!o->isExtensible())
            return false;

        ScopedProperty pd(scope);
        pd->copy(p, attrs);
        pd->fullyPopulated(&attrs);
        o->insertMember(name, pd, attrs);
        return true;
    }

    return o->internalDefineOwnProperty(scope.engine, memberIndex, name, p, attrs);
}

bool Object::isExtensible(const Managed *m)
{
    return m->d()->internalClass->extensible;
}

bool Object::preventExtensions(Managed *m)
{
    Q_ASSERT(m->isObject());
    Object *o = static_cast<Object *>(m);
    o->setInternalClass(o->internalClass()->nonExtensible());
    return true;
}

Heap::Object *Object::getPrototypeOf(const Managed *m)
{
    return m->internalClass()->prototype;
}

bool Object::setPrototypeOf(Managed *m, const Object *proto)
{
    Q_ASSERT(m->isObject());
    Object *o = static_cast<Object *>(m);
    Heap::Object *current = o->internalClass()->prototype;
    Heap::Object *protod = proto ? proto->d() : nullptr;
    if (current == protod)
        return true;
    if (!o->internalClass()->extensible)
        return false;
    Heap::Object *p = protod;
    while (p) {
        if (p == o->d())
            return false;
        if (reinterpret_cast<const ObjectVTable *>(p->vtable())->getPrototypeOf !=
                reinterpret_cast<const ObjectVTable *>(Object::staticVTable())->getPrototypeOf)
            break;
        p = p->prototype();
    }
    o->setInternalClass(o->internalClass()->changePrototype(protod));
    return true;
}

bool Object::setArrayLength(uint newLen)
{
    Q_ASSERT(isArrayObject());
    if (!internalClass()->propertyData[Heap::ArrayObject::LengthPropertyIndex].isWritable())
        return false;
    uint oldLen = getLength();
    bool ok = true;
    if (newLen < oldLen) {
        if (arrayData()) {
            uint l = arrayData()->vtable()->truncate(this, newLen);
            if (l != newLen)
                ok = false;
            newLen = l;
        }
    } else {
        if (newLen >= 0x100000)
            initSparseArray();
    }
    setArrayLengthUnchecked(newLen);
    return ok;
}

void Object::initSparseArray()
{
    if (arrayType() == Heap::ArrayData::Sparse)
        return;

    ArrayData::realloc(this, Heap::ArrayData::Sparse, 0, false);
}


DEFINE_OBJECT_VTABLE(ArrayObject);

void Heap::ArrayObject::init(const QStringList &list)
{
    Object::init();
    commonInit();
    Scope scope(internalClass->engine);
    ScopedObject a(scope, this);

    // Converts a QStringList to JS.
    // The result is a new Array object with length equal to the length
    // of the QStringList, and the elements being the QStringList's
    // elements converted to JS Strings.
    int len = list.count();
    a->arrayReserve(len);
    ScopedValue v(scope);
    for (int ii = 0; ii < len; ++ii)
        a->arrayPut(ii, (v = scope.engine->newString(list.at(ii))));
    a->setArrayLengthUnchecked(len);
}

qint64 ArrayObject::getLength(const Managed *m)
{
    const ArrayObject *a = static_cast<const ArrayObject *>(m);
    return a->propertyData(Heap::ArrayObject::LengthPropertyIndex)->toLength();
}

QStringList ArrayObject::toQStringList() const
{
    QStringList result;

    QV4::ExecutionEngine *engine = internalClass()->engine;
    Scope scope(engine);
    ScopedValue v(scope);

    uint length = getLength();
    for (uint i = 0; i < length; ++i) {
        v = const_cast<ArrayObject *>(this)->get(i);
        result.append(v->toQStringNoThrow());
    }
    return result;
}

bool ArrayObject::defineOwnProperty(Managed *m, Identifier id, const Property *p, PropertyAttributes attrs)
{
    Q_ASSERT(m->isArrayObject());
    ArrayObject *a = static_cast<ArrayObject *>(m);

    if (id.isArrayIndex()) {
        uint index = id.asArrayIndex();
        uint len = a->getLength();
        if (index >= len && !a->internalClass()->propertyData[Heap::ArrayObject::LengthPropertyIndex].isWritable())
            return false;

        bool succeeded = Object::defineOwnProperty(m, id, p, attrs);
        if (!succeeded)
            return false;

        if (index >= len)
            a->setArrayLengthUnchecked(index + 1);

        return true;
    }

    ExecutionEngine *engine = m->engine();
    if (id == engine->id_length()->identifier()) {
        Scope scope(engine);
        Q_ASSERT(Heap::ArrayObject::LengthPropertyIndex == a->internalClass()->find(engine->id_length()->identifier()));
        ScopedProperty lp(scope);
        PropertyAttributes cattrs;
        a->getProperty(Heap::ArrayObject::LengthPropertyIndex, lp, &cattrs);
        if (attrs.isEmpty() || p->isSubset(attrs, lp, cattrs))
            return true;
        if (!cattrs.isWritable() || attrs.type() == PropertyAttributes::Accessor || attrs.isConfigurable() || attrs.isEnumerable())
            return false;
        bool succeeded = true;
        if (attrs.type() == PropertyAttributes::Data) {
            bool ok;
            uint l = p->value.asArrayLength(&ok);
            if (!ok) {
                ScopedValue v(scope, p->value);
                engine->throwRangeError(v);
                return false;
            }
            succeeded = a->setArrayLength(l);
        }
        if (attrs.hasWritable() && !attrs.isWritable()) {
            cattrs.setWritable(false);
            Heap::InternalClass::changeMember(a, engine->id_length()->identifier(), cattrs);
        }
        if (!succeeded)
            return false;
        return true;
    }
    return Object::defineOwnProperty(m, id, p, attrs);
}
