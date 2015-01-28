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
#include "qv4objectiterator_p.h"
#include "qv4object_p.h"
#include "qv4stringobject_p.h"
#include "qv4identifier_p.h"
#include "qv4argumentsobject_p.h"

using namespace QV4;

ObjectIterator::ObjectIterator(ExecutionEngine *e, Value *scratch1, Value *scratch2, Object *o, uint flags)
    : engine(e)
    , object(scratch1)
    , current(scratch2)
    , arrayNode(0)
    , arrayIndex(0)
    , memberIndex(0)
    , flags(flags)
{
    init(o);
}

ObjectIterator::ObjectIterator(Scope &scope, Object *o, uint flags)
    : engine(scope.engine)
    , object(scope.alloc(1))
    , current(scope.alloc(1))
    , arrayNode(0)
    , arrayIndex(0)
    , memberIndex(0)
    , flags(flags)
{
    init(o);
}

void ObjectIterator::init(Object *o)
{
    object->m = o ? o->m : 0;
    current->m = o ? o->m : 0;

#if QT_POINTER_SIZE == 4
    object->tag = QV4::Value::Managed_Type;
    current->tag = QV4::Value::Managed_Type;
#endif

    if (object->as<ArgumentsObject>()) {
        Scope scope(engine);
        Scoped<ArgumentsObject> (scope, object->asReturnedValue())->fullyCreate();
    }
}

void ObjectIterator::next(Heap::String **name, uint *index, Property *pd, PropertyAttributes *attrs)
{
    *name = 0;
    *index = UINT_MAX;

    if (!object->asObject()) {
        *attrs = PropertyAttributes();
        return;
    }
    Scope scope(engine);
    ScopedObject o(scope);
    ScopedString n(scope);

    while (1) {
        if (!current->asObject())
            break;

        while (1) {
            current->asObject()->advanceIterator(this, name, index, pd, attrs);
            if (attrs->isEmpty())
                break;
            // check the property is not already defined earlier in the proto chain
            if (current->heapObject() != object->heapObject()) {
                o = object->asObject();
                n = *name;
                bool shadowed = false;
                while (o->asObject()->d() != current->heapObject()) {
                    if ((!!n && o->hasOwnProperty(n)) ||
                        (*index != UINT_MAX && o->hasOwnProperty(*index))) {
                        shadowed = true;
                        break;
                    }
                    o = o->prototype();
                }
                if (shadowed)
                    continue;
            }
            return;
        }

        if (flags & WithProtoChain)
            current->m = current->objectValue()->prototype();
        else
            current->m = (Heap::Base *)0;

        arrayIndex = 0;
        memberIndex = 0;
    }
    *attrs = PropertyAttributes();
}

ReturnedValue ObjectIterator::nextPropertyName(Value *value)
{
    if (!object->asObject())
        return Encode::null();

    PropertyAttributes attrs;
    uint index;
    Scope scope(engine);
    ScopedProperty p(scope);
    ScopedString name(scope);
    next(name.getRef(), &index, p, &attrs);
    if (attrs.isEmpty())
        return Encode::null();

    *value = object->objectValue()->getValue(p, attrs);

    if (!!name)
        return name->asReturnedValue();
    assert(index < UINT_MAX);
    return Encode(index);
}

ReturnedValue ObjectIterator::nextPropertyNameAsString(Value *value)
{
    if (!object->asObject())
        return Encode::null();

    PropertyAttributes attrs;
    uint index;
    Scope scope(engine);
    ScopedProperty p(scope);
    ScopedString name(scope);
    next(name.getRef(), &index, p, &attrs);
    if (attrs.isEmpty())
        return Encode::null();

    *value = object->objectValue()->getValue(p, attrs);

    if (!!name)
        return name->asReturnedValue();
    assert(index < UINT_MAX);
    return Encode(engine->newString(QString::number(index)));
}

ReturnedValue ObjectIterator::nextPropertyNameAsString()
{
    if (!object->asObject())
        return Encode::null();

    PropertyAttributes attrs;
    uint index;
    Scope scope(engine);
    ScopedProperty p(scope);
    ScopedString name(scope);
    next(name.getRef(), &index, p, &attrs);
    if (attrs.isEmpty())
        return Encode::null();

    if (!!name)
        return name->asReturnedValue();
    Q_ASSERT(index < UINT_MAX);
    return Encode(engine->newString(QString::number(index)));
}


DEFINE_OBJECT_VTABLE(ForEachIteratorObject);

void ForEachIteratorObject::markObjects(Heap::Base *that, ExecutionEngine *e)
{
    ForEachIteratorObject::Data *o = static_cast<ForEachIteratorObject::Data *>(that);
    o->workArea[0].mark(e);
    o->workArea[1].mark(e);
    Object::markObjects(that, e);
}
