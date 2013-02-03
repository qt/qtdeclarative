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
#include "qv4objectiterator.h"
#include "qv4object.h"
#include "qv4stringobject.h"

namespace QQmlJS {
namespace VM {

ObjectIterator::ObjectIterator(ExecutionContext *context, Object *o, uint flags)
    : context(context)
    , object(o)
    , current(o)
    , arrayNode(0)
    , arrayIndex(0)
    , tableIndex(0)
    , flags(flags)
{
    if (current && current->asStringObject())
        this->flags |= CurrentIsString;
}

PropertyDescriptor *ObjectIterator::next(String **name, uint *index)
{
    PropertyDescriptor *p = 0;
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
                return s->__getOwnProperty__(context, *index);
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
                p = current->arrayAt(k);
                arrayNode = arrayNode->nextNode();
                if (p && (!(flags & EnumberableOnly) || p->isEnumerable())) {
                    arrayIndex = k + 1;
                    *index = k;
                    return p;
                }
            }
            arrayNode = 0;
            arrayIndex = UINT_MAX;
        }
        // dense arrays
        while (arrayIndex < current->arrayDataLen) {
            p = current->arrayAt(arrayIndex);
            ++arrayIndex;
            if (p && p->type != PropertyDescriptor::Generic && (!(flags & EnumberableOnly) || p->isEnumerable())) {
                *index = arrayIndex - 1;
                return p;
            }
        }

        if (!current->members || tableIndex >= (uint)current->members->_propertyCount) {
            if (flags & WithProtoChain)
                current = current->prototype;
            else
                current = 0;
            if (current && current->asStringObject())
                flags |= CurrentIsString;
            else
                flags &= ~CurrentIsString;


            arrayIndex = 0;
            tableIndex = 0;
            continue;
        }
        PropertyTableEntry *pt = current->members->_properties[tableIndex];
        ++tableIndex;
        // ### check that it's not a repeated attribute
        if (pt) {
            PropertyDescriptor *pd = current->memberData + pt->valueIndex;
            if (!(flags & EnumberableOnly) || pd->isEnumerable()) {
                *name = pt->name;
                p = pd;
                return p;
            }
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
        return __qmljs_to_string(Value::fromDouble(index), context);
    return Value::nullValue();
}

}
}

