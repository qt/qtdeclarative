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
#include "qv4objectiterator_p.h"
#include "qv4object_p.h"
#include "qv4stringobject_p.h"
#include "qv4identifier_p.h"

using namespace QV4;

ObjectIterator::ObjectIterator(Object *o, uint flags)
    : object(o)
    , internalClass(o ? o->internalClass : 0)
    , current(o)
    , arrayNode(0)
    , arrayIndex(0)
    , memberIndex(0)
    , flags(flags)
    , dynamicProperties(0)
    , dynamicPropertyIndex(0)
{
    tmpDynamicProperty.value = Value::undefinedValue();
    if (current) {
        if (current->asStringObject())
            this->flags |= CurrentIsString;
        if (current->dynamicPropertyEnumerator)
            dynamicProperties = current->dynamicPropertyEnumerator(current).asArrayObject();
    }
}

Property *ObjectIterator::next(String **name, uint *index, PropertyAttributes *attrs)
{
    Property *p = 0;
    *name = 0;
    *index = UINT_MAX;
    while (1) {
        if (!current)
            break;

        if (flags & CurrentIsString) {
            StringObject *s = static_cast<StringObject *>(current);
            uint slen = s->value.stringValue()->toQString().length();
            while (arrayIndex < slen) {
                *index = arrayIndex;
                ++arrayIndex;
                if (attrs)
                    *attrs = s->arrayAttributes ? s->arrayAttributes[arrayIndex] : PropertyAttributes(Attr_NotWritable|Attr_NotConfigurable);
                return s->__getOwnProperty__(*index);
            }
            flags &= ~CurrentIsString;
            arrayNode = current->sparseArrayBegin();
            // iterate until we're past the end of the string
            while (arrayNode && arrayNode->key() < slen)
                arrayNode = arrayNode->nextNode();
        }

        if (!arrayIndex)
            arrayNode = current->sparseArrayBegin();

        // sparse arrays
        if (arrayNode) {
            while (arrayNode != current->sparseArrayEnd()) {
                int k = arrayNode->key();
                uint pidx = arrayNode->value;
                p = current->arrayData + pidx;
                arrayNode = arrayNode->nextNode();
                PropertyAttributes a = current->arrayAttributes ? current->arrayAttributes[pidx] : PropertyAttributes(Attr_Data);
                if (!(flags & EnumberableOnly) || a.isEnumerable()) {
                    arrayIndex = k + 1;
                    *index = k;
                    if (attrs)
                        *attrs = a;
                    return p;
                }
            }
            arrayNode = 0;
            arrayIndex = UINT_MAX;
        }
        // dense arrays
        while (arrayIndex < current->arrayDataLen) {
            uint pidx = current->propertyIndexFromArrayIndex(arrayIndex);
            p = current->arrayData + pidx;
            PropertyAttributes a = current->arrayAttributes ? current->arrayAttributes[pidx] : PropertyAttributes(Attr_Data);
            ++arrayIndex;
            if ((!current->arrayAttributes || !current->arrayAttributes[pidx].isGeneric())
                 && (!(flags & EnumberableOnly) || a.isEnumerable())) {
                *index = arrayIndex - 1;
                if (attrs)
                    *attrs = a;
                return p;
            }
        }

        if (dynamicProperties) {
            const int len = dynamicProperties->arrayLength();
            while (dynamicPropertyIndex < len) {
                bool exists = false;
                String *n = dynamicProperties->getIndexed(dynamicPropertyIndex).asString();
                ++dynamicPropertyIndex;
                if (!n)
                    continue;
                PropertyAttributes a;
                if (current->dynamicPropertyQuery)
                    a = current->dynamicPropertyQuery(current, n);
                if (!(flags & EnumberableOnly) || a.isEnumerable()) {
                    *name = n;
                    if (attrs)
                        *attrs = a;
                    tmpDynamicProperty.value = current->get(n);
                    return &tmpDynamicProperty;
                }
            }
            dynamicProperties = 0;
        }

        if (memberIndex == internalClass->size) {
            if (flags & WithProtoChain)
                current = current->prototype;
            else
                current = 0;
            if (current && current->asStringObject())
                flags |= CurrentIsString;
            else
                flags &= ~CurrentIsString;

            internalClass = current ? current->internalClass : 0;

            arrayIndex = 0;
            memberIndex = 0;
            dynamicProperties = 0;
            if (current && current->dynamicPropertyEnumerator)
                dynamicProperties = current->dynamicPropertyEnumerator(current).asArrayObject();
            dynamicPropertyIndex = 0;
            continue;
        }
        String *n = internalClass->nameMap.at(memberIndex);
        assert(n);
        // ### check that it's not a repeated attribute

        p = current->memberData + memberIndex;
        PropertyAttributes a = internalClass->propertyData[memberIndex];
        ++memberIndex;
        if (!(flags & EnumberableOnly) || a.isEnumerable()) {
            *name = n;
            if (attrs)
                *attrs = a;
            return p;
        }
    }
    return 0;
}

Value ObjectIterator::nextPropertyName()
{
    uint index;
    String *name;
    next(&name, &index);
    if (name)
        return Value::fromString(name);
    if (index < UINT_MAX)
        return Value::fromDouble(index);
    return Value::nullValue();
}

Value ObjectIterator::nextPropertyNameAsString()
{
    uint index;
    String *name;
    next(&name, &index);
    if (name)
        return Value::fromString(name);
    if (index < UINT_MAX)
        return __qmljs_to_string(Value::fromDouble(index), object->internalClass->engine->current);
    return Value::nullValue();
}
