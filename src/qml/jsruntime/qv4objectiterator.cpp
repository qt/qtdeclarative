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

ObjectIterator::ObjectIterator(SafeObject *scratch1, SafeObject *scratch2, const ObjectRef o, uint flags)
    : object(*scratch1)
    , current(*scratch2)
    , arrayNode(0)
    , arrayIndex(0)
    , memberIndex(0)
    , flags(flags)
{
    object = o;
    current = o;
    tmpDynamicProperty.value = Primitive::undefinedValue();

    if (object && object->isNonStrictArgumentsObject) {
        Scope scope(object->engine());
        Scoped<ArgumentsObject> (scope, object->asReturnedValue())->fullyCreate();
    }
}

ObjectIterator::ObjectIterator(Scope &scope, const ObjectRef o, uint flags)
    : object(*static_cast<SafeObject *>(scope.alloc(1)))
    , current(*static_cast<SafeObject *>(scope.alloc(1)))
    , arrayNode(0)
    , arrayIndex(0)
    , memberIndex(0)
    , flags(flags)
{
    object = o;
    current = o;
    tmpDynamicProperty.value = Primitive::undefinedValue();

    if (object && object->isNonStrictArgumentsObject) {
        Scope scope(object->engine());
        Scoped<ArgumentsObject> (scope, object->asReturnedValue())->fullyCreate();
    }
}

Property *ObjectIterator::next(StringRef name, uint *index, PropertyAttributes *attrs)
{
    name = (String *)0;
    *index = UINT_MAX;
    if (!object)
        return 0;

    Property *p = 0;
    while (1) {
        if (!current)
            break;

        while ((p = current->advanceIterator(this, name, index, attrs))) {
            // check the property is not already defined earlier in the proto chain
            if (current != object) {
                Property *pp;
                if (name) {
                    pp = object->__getPropertyDescriptor__(name);
                } else {
                    assert (*index != UINT_MAX);
                    pp = object->__getPropertyDescriptor__(*index);
                }
                if (pp != p)
                    continue;
            }
            return p;
        }

        if (flags & WithProtoChain)
            current = current->prototype();
        else
            current = (Object *)0;

        arrayIndex = 0;
        memberIndex = 0;
    }
    return 0;
}

ReturnedValue ObjectIterator::nextPropertyName(ValueRef value)
{
    if (!object)
        return Encode::null();

    PropertyAttributes attrs;
    uint index;
    Scope scope(object->engine());
    ScopedString name(scope);
    Property *p = next(name, &index, &attrs);
    if (!p)
        return Encode::null();

    value = object->getValue(p, attrs);

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
    uint index;
    Scope scope(object->engine());
    ScopedString name(scope);
    Property *p = next(name, &index, &attrs);
    if (!p)
        return Encode::null();

    value = object->getValue(p, attrs);

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
    uint index;
    Scope scope(object->engine());
    ScopedString name(scope);
    Property *p = next(name, &index, &attrs);
    if (!p)
        return Encode::null();

    if (!!name)
        return name->asReturnedValue();
    assert(index < UINT_MAX);
    return Encode(object->engine()->newString(QString::number(index)));
}


DEFINE_MANAGED_VTABLE(ForEachIteratorObject);

void ForEachIteratorObject::markObjects(Managed *that, ExecutionEngine *e)
{
    ForEachIteratorObject *o = static_cast<ForEachIteratorObject *>(that);
    o->workArea[0].mark(e);
    o->workArea[1].mark(e);
    Object::markObjects(that, e);
}
