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

using namespace QV4;

ObjectIterator::ObjectIterator(Object *o, uint flags)
    : object(o)
    , current(o)
    , arrayNode(0)
    , arrayIndex(0)
    , memberIndex(0)
    , flags(flags)
{
    tmpDynamicProperty.value = Value::undefinedValue();
}

Property *ObjectIterator::next(String **name, uint *index, PropertyAttributes *attrs)
{
    Property *p = 0;
    *name = 0;
    *index = UINT_MAX;
    while (1) {
        if (!current)
            break;

        while (p = current->advanceIterator(this, name, index, attrs)) {
            // check the property is not already defined earlier in the proto chain
            if (current != object) {
                Property *pp;
                if (*name) {
                    pp = object->__getPropertyDescriptor__(*name);
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
            current = current->prototype;
        else
            current = 0;

        arrayIndex = 0;
        memberIndex = 0;
    }
    return 0;
}

Value ObjectIterator::nextPropertyName(Value *value)
{
    PropertyAttributes attrs;
    uint index;
    String *name;
    Property *p = next(&name, &index, &attrs);
    if (!p)
        return Value::nullValue();

    if (value)
        *value = object->getValue(p, attrs);

    if (name)
        return Value::fromString(name);
    assert(index < UINT_MAX);
    return Value::fromDouble(index);
}

Value ObjectIterator::nextPropertyNameAsString(Value *value)
{
    PropertyAttributes attrs;
    uint index;
    String *name;
    Property *p = next(&name, &index, &attrs);
    if (!p)
        return Value::nullValue();

    if (value)
        *value = object->getValue(p, attrs);

    if (name)
        return Value::fromString(name);
    assert(index < UINT_MAX);
    return Value::fromString(object->engine()->newString(QString::number(index)));
}
