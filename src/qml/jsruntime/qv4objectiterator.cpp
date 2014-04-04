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
#include "qv4objectiterator_p.h"
#include "qv4object_p.h"
#include "qv4stringobject_p.h"
#include "qv4identifier_p.h"
#include "qv4argumentsobject_p.h"

using namespace QV4;

ObjectIterator::ObjectIterator(Value *scratch1, Value *scratch2, const ObjectRef o, uint flags)
    : object(ObjectRef::fromValuePointer(scratch1))
    , current(ObjectRef::fromValuePointer(scratch2))
    , arrayNode(0)
    , arrayIndex(0)
    , memberIndex(0)
    , flags(flags)
{
    object = o.getPointer();
    current = o.getPointer();

    if (!!object && object->asArgumentsObject()) {
        Scope scope(object->engine());
        Scoped<ArgumentsObject> (scope, object->asReturnedValue())->fullyCreate();
    }
}

ObjectIterator::ObjectIterator(Scope &scope, const ObjectRef o, uint flags)
    : object(ObjectRef::fromValuePointer(scope.alloc(1)))
    , current(ObjectRef::fromValuePointer(scope.alloc(1)))
    , arrayNode(0)
    , arrayIndex(0)
    , memberIndex(0)
    , flags(flags)
{
    object = o;
    current = o;

    if (!!object && object->asArgumentsObject()) {
        Scope scope(object->engine());
        Scoped<ArgumentsObject> (scope, object->asReturnedValue())->fullyCreate();
    }
}

void ObjectIterator::next(StringRef name, uint *index, Property *pd, PropertyAttributes *attrs)
{
    name = (String *)0;
    *index = UINT_MAX;

    if (!object) {
        *attrs = PropertyAttributes();
        return;
    }

    while (1) {
        if (!current)
            break;

        while (1) {
            current->advanceIterator(this, name, index, pd, attrs);
            if (attrs->isEmpty())
                break;
            // check the property is not already defined earlier in the proto chain
            if (current != object) {
                Object *o = object;
                bool shadowed = false;
                while (o != current) {
                    if ((!!name && o->hasOwnProperty(name)) ||
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
            current = current->prototype();
        else
            current = (Object *)0;

        arrayIndex = 0;
        memberIndex = 0;
    }
    *attrs = PropertyAttributes();
}

ReturnedValue ObjectIterator::nextPropertyName(ValueRef value)
{
    if (!object)
        return Encode::null();

    PropertyAttributes attrs;
    Property p;
    uint index;
    Scope scope(object->engine());
    ScopedString name(scope);
    next(name, &index, &p, &attrs);
    if (attrs.isEmpty())
        return Encode::null();

    value = object->getValue(&p, attrs);

    if (!!name)
        return name->asReturnedValue();
    assert(index < UINT_MAX);
    return Encode(index);
}

ReturnedValue ObjectIterator::nextPropertyNameAsString(ValueRef value)
{
    if (!object)
        return Encode::null();

    PropertyAttributes attrs;
    Property p;
    uint index;
    Scope scope(object->engine());
    ScopedString name(scope);
    next(name, &index, &p, &attrs);
    if (attrs.isEmpty())
        return Encode::null();

    value = object->getValue(&p, attrs);

    if (!!name)
        return name->asReturnedValue();
    assert(index < UINT_MAX);
    return Encode(object->engine()->newString(QString::number(index)));
}

ReturnedValue ObjectIterator::nextPropertyNameAsString()
{
    if (!object)
        return Encode::null();

    PropertyAttributes attrs;
    Property p;
    uint index;
    Scope scope(object->engine());
    ScopedString name(scope);
    next(name, &index, &p, &attrs);
    if (attrs.isEmpty())
        return Encode::null();

    if (!!name)
        return name->asReturnedValue();
    assert(index < UINT_MAX);
    return Encode(object->engine()->newString(QString::number(index)));
}


DEFINE_OBJECT_VTABLE(ForEachIteratorObject);

void ForEachIteratorObject::markObjects(Managed *that, ExecutionEngine *e)
{
    ForEachIteratorObject *o = static_cast<ForEachIteratorObject *>(that);
    o->workArea[0].mark(e);
    o->workArea[1].mark(e);
    Object::markObjects(that, e);
}
