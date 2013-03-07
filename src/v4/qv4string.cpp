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

#include "qv4string.h"
#include "qv4identifier.h"
#include "qmljs_runtime.h"
#include "qv4objectproto.h"
#include "qv4stringobject.h"
#include <QtCore/QHash>

namespace QQmlJS {
namespace VM {

static uint toArrayIndex(const QChar *ch, const QChar *end, bool *ok)
{
    *ok = false;
    uint i = ch->unicode() - '0';
    if (i > 9)
        return UINT_MAX;
    ++ch;
    // reject "01", "001", ...
    if (i == 0 && ch != end)
        return UINT_MAX;

    while (ch < end) {
        uint x = ch->unicode() - '0';
        if (x > 9)
            return UINT_MAX;
        uint n = i*10 + x;
        if (n < i)
            // overflow
            return UINT_MAX;
        i = n;
        ++ch;
    }
    *ok = true;
    return i;
}

const ManagedVTable String::static_vtbl =
{
    call,
    construct,
    0 /*markObjects*/,
    destroy,
    hasInstance,
    get,
    getIndexed,
    put,
    putIndexed,
    query,
    queryIndexed,
    deleteProperty,
    deleteIndexedProperty,
    "String",
};

void String::destroy(Managed *that)
{
    static_cast<String*>(that)->~String();
}

Value String::get(Managed *m, ExecutionContext *ctx, String *name, bool *hasProperty)
{
    String *that = static_cast<String *>(m);
    if (name == ctx->engine->id_length)
        return Value::fromInt32(that->_text.length());
    PropertyDescriptor *pd = ctx->engine->objectPrototype->__getPropertyDescriptor__(ctx, name);
    return ctx->engine->objectPrototype->getValueChecked(ctx, pd, Value::fromString(that));
}

Value String::getIndexed(Managed *m, ExecutionContext *ctx, uint index, bool *hasProperty)
{
    String *that = static_cast<String *>(m);
    if (index < that->_text.length()) {
        if (hasProperty)
            *hasProperty = true;
        return Value::fromString(ctx, that->toQString().mid(index, 1));
    }
    if (hasProperty)
        *hasProperty = false;
    return Value::undefinedValue();
}

void String::put(Managed *m, ExecutionContext *ctx, String *name, const Value &value)
{
    String *that = static_cast<String *>(m);
    Object *o = ctx->engine->newStringObject(ctx, Value::fromString(that));
    o->put(ctx, name, value);
}

void String::putIndexed(Managed *m, ExecutionContext *ctx, uint index, const Value &value)
{
    String *that = static_cast<String *>(m);
    Object *o = ctx->engine->newStringObject(ctx, Value::fromString(that));
    o->putIndexed(ctx, index, value);
}

PropertyFlags String::query(Managed *m, ExecutionContext *ctx, String *name)
{
    return PropertyFlags(0);
}

PropertyFlags String::queryIndexed(Managed *m, ExecutionContext *ctx, uint index)
{
    String *that = static_cast<String *>(m);
    return (index < that->_text.length()) ? PropertyFlags(Enumerable) : PropertyFlags(0);
}

bool String::deleteProperty(Managed *m, ExecutionContext *ctx, String *name)
{
    return false;
}

bool String::deleteIndexedProperty(Managed *m, ExecutionContext *ctx, uint index)
{
    return false;
}

uint String::toUInt(bool *ok) const
{
    *ok = true;

    if (subtype == StringType_Unknown)
        createHashValue();
    if (subtype >= StringType_UInt)
        return stringHash;

    // ### this conversion shouldn't be required
    double d = __qmljs_string_to_number(this);
    uint l = (uint)d;
    if (d == l)
        return l;
    *ok = false;
    return UINT_MAX;
}

void String::makeIdentifierImpl(const ExecutionContext *ctx)
{
    ctx->engine->identifierCache->toIdentifier(this);
}

void String::createHashValue() const
{
    const QChar *ch = _text.constData();
    const QChar *end = ch + _text.length();

    // array indices get their number as hash value
    bool ok;
    stringHash = toArrayIndex(ch, end, &ok);
    if (ok) {
        subtype = (stringHash == UINT_MAX) ? StringType_UInt : StringType_ArrayIndex;
        return;
    }

    uint h = 0xffffffff;
    while (ch < end) {
        h = 31 * h + ch->unicode();
        ++ch;
    }

    stringHash = h;
    subtype = StringType_Regular;
}

}
}
