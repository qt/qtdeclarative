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

#include "qv4string_p.h"
#include "qv4value_inl_p.h"
#ifndef V4_BOOTSTRAP
#include "qv4identifiertable_p.h"
#include "qv4runtime_p.h"
#include "qv4objectproto_p.h"
#include "qv4stringobject_p.h"
#endif
#include <QtCore/QHash>

using namespace QV4;

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

#ifndef V4_BOOTSTRAP

static uint toArrayIndex(const char *ch, const char *end, bool *ok)
{
    *ok = false;
    uint i = *ch - '0';
    if (i > 9)
        return UINT_MAX;
    ++ch;
    // reject "01", "001", ...
    if (i == 0 && ch != end)
        return UINT_MAX;

    while (ch < end) {
        uint x = *ch - '0';
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


const ObjectVTable String::static_vtbl =
{
    DEFINE_MANAGED_VTABLE_INT(String),
    0,
    0,
    get,
    getIndexed,
    put,
    putIndexed,
    query,
    queryIndexed,
    deleteProperty,
    deleteIndexedProperty,
    0 /*getLookup*/,
    0 /*setLookup*/,
    0,
    0 /*advanceIterator*/,
};

void String::destroy(Managed *that)
{
    static_cast<String*>(that)->~String();
}

void String::markObjects(Managed *that, ExecutionEngine *e)
{
    String *s = static_cast<String *>(that);
    if (s->largestSubLength) {
        s->left->mark(e);
        s->right->mark(e);
    }
}

ReturnedValue String::get(Managed *m, const StringRef name, bool *hasProperty)
{
    ExecutionEngine *v4 = m->engine();
    Scope scope(v4);
    ScopedString that(scope, static_cast<String *>(m));

    if (name->equals(v4->id_length)) {
        if (hasProperty)
            *hasProperty = true;
        return Primitive::fromInt32(that->_text->size).asReturnedValue();
    }
    PropertyAttributes attrs;
    Property *pd = v4->stringObjectClass->prototype->__getPropertyDescriptor__(name, &attrs);
    if (!pd || attrs.isGeneric()) {
        if (hasProperty)
            *hasProperty = false;
        return Primitive::undefinedValue().asReturnedValue();
    }
    if (hasProperty)
        *hasProperty = true;
    return v4->stringObjectClass->prototype->getValue(that, pd, attrs);
}

ReturnedValue String::getIndexed(Managed *m, uint index, bool *hasProperty)
{
    ExecutionEngine *engine = m->engine();
    Scope scope(engine);
    ScopedString that(scope, static_cast<String *>(m));

    if (index < static_cast<uint>(that->_text->size)) {
        if (hasProperty)
            *hasProperty = true;
        return Encode(engine->newString(that->toQString().mid(index, 1)));
    }
    PropertyAttributes attrs;
    Property *pd = engine->stringObjectClass->prototype->__getPropertyDescriptor__(index, &attrs);
    if (!pd || attrs.isGeneric()) {
        if (hasProperty)
            *hasProperty = false;
        return Primitive::undefinedValue().asReturnedValue();
    }
    if (hasProperty)
        *hasProperty = true;
    return engine->stringObjectClass->prototype->getValue(that, pd, attrs);
}

void String::put(Managed *m, const StringRef name, const ValueRef value)
{
    Scope scope(m->engine());
    if (scope.hasException())
        return;
    ScopedString that(scope, static_cast<String *>(m));
    Scoped<Object> o(scope, that->engine()->newStringObject(that));
    o->put(name, value);
}

void String::putIndexed(Managed *m, uint index, const ValueRef value)
{
    Scope scope(m->engine());
    if (scope.hasException())
        return;

    ScopedString that(scope, static_cast<String *>(m));
    Scoped<Object> o(scope, that->engine()->newStringObject(that));
    o->putIndexed(index, value);
}

PropertyAttributes String::query(const Managed *m, StringRef name)
{
    uint idx = name->asArrayIndex();
    if (idx != UINT_MAX)
        return queryIndexed(m, idx);
    return Attr_Invalid;
}

PropertyAttributes String::queryIndexed(const Managed *m, uint index)
{
    const String *that = static_cast<const String *>(m);
    return (index < static_cast<uint>(that->_text->size)) ? Attr_NotConfigurable|Attr_NotWritable : Attr_Invalid;
}

bool String::deleteProperty(Managed *, const StringRef)
{
    return false;
}

bool String::deleteIndexedProperty(Managed *, uint)
{
    return false;
}

bool String::isEqualTo(Managed *t, Managed *o)
{
    if (t == o)
        return true;

    if (!o->internalClass->vtable->isString)
        return false;

    String *that = static_cast<String *>(t);
    String *other = static_cast<String *>(o);
    if (that->hashValue() != other->hashValue())
        return false;
    if (that->identifier && that->identifier == other->identifier)
        return true;
    if (that->subtype >= StringType_UInt && that->subtype == other->subtype)
        return true;

    return that->toQString() == other->toQString();
}


String::String(ExecutionEngine *engine, const QString &text)
    : Managed(engine->stringClass), _text(const_cast<QString &>(text).data_ptr())
    , identifier(0), stringHash(UINT_MAX)
    , largestSubLength(0)
{
    _text->ref.ref();
    len = _text->size;
    subtype = StringType_Unknown;
}

String::String(ExecutionEngine *engine, String *l, String *r)
    : Managed(engine->stringClass)
    , left(l), right(r)
    , stringHash(UINT_MAX), largestSubLength(qMax(l->largestSubLength, r->largestSubLength))
    , len(l->len + r->len)
{
    subtype = StringType_Unknown;

    if (!l->largestSubLength && l->len > largestSubLength)
        largestSubLength = l->len;
    if (!r->largestSubLength && r->len > largestSubLength)
        largestSubLength = r->len;

    // make sure we don't get excessive depth in our strings
    if (len > 256 && len >= 2*largestSubLength)
        simplifyString();
}

uint String::toUInt(bool *ok) const
{
    *ok = true;

    if (subtype == StringType_Unknown)
        createHashValue();
    if (subtype >= StringType_UInt)
        return stringHash;

    // ### this conversion shouldn't be required
    double d = RuntimeHelpers::stringToNumber(toQString());
    uint l = (uint)d;
    if (d == l)
        return l;
    *ok = false;
    return UINT_MAX;
}

bool String::equals(const StringRef other) const
{
    if (this == other.getPointer())
        return true;
    if (hashValue() != other->hashValue())
        return false;
    if (identifier && identifier == other->identifier)
        return true;
    if (subtype >= StringType_UInt && subtype == other->subtype)
        return true;

    return toQString() == other->toQString();
}

void String::makeIdentifierImpl() const
{
    if (largestSubLength)
        simplifyString();
    Q_ASSERT(!largestSubLength);
    engine()->identifierTable->identifier(this);
}

void String::simplifyString() const
{
    Q_ASSERT(largestSubLength);

    int l = length();
    QString result(l, Qt::Uninitialized);
    QChar *ch = const_cast<QChar *>(result.constData());
    recursiveAppend(ch);
    _text = result.data_ptr();
    _text->ref.ref();
    identifier = 0;
    largestSubLength = 0;
}

QChar *String::recursiveAppend(QChar *ch) const
{
    if (largestSubLength) {
        ch = left->recursiveAppend(ch);
        ch = right->recursiveAppend(ch);
    } else {
        memcpy(ch, _text->data(), _text->size*sizeof(QChar));
        ch += _text->size;
    }
    return ch;
}


void String::createHashValue() const
{
    if (largestSubLength)
        simplifyString();
    Q_ASSERT(!largestSubLength);
    const QChar *ch = reinterpret_cast<const QChar *>(_text->data());
    const QChar *end = ch + _text->size;

    // array indices get their number as hash value
    bool ok;
    stringHash = ::toArrayIndex(ch, end, &ok);
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

uint String::createHashValue(const QChar *ch, int length)
{
    const QChar *end = ch + length;

    // array indices get their number as hash value
    bool ok;
    uint stringHash = ::toArrayIndex(ch, end, &ok);
    if (ok)
        return stringHash;

    uint h = 0xffffffff;
    while (ch < end) {
        h = 31 * h + ch->unicode();
        ++ch;
    }

    return h;
}

uint String::createHashValue(const char *ch, int length)
{
    const char *end = ch + length;

    // array indices get their number as hash value
    bool ok;
    uint stringHash = ::toArrayIndex(ch, end, &ok);
    if (ok)
        return stringHash;

    uint h = 0xffffffff;
    while (ch < end) {
        if ((uchar)(*ch) >= 0x80)
            return UINT_MAX;
        h = 31 * h + *ch;
        ++ch;
    }

    return h;
}

uint String::getLength(const Managed *m)
{
    return static_cast<const String *>(m)->length();
}

#endif // V4_BOOTSTRAP

uint String::toArrayIndex(const QString &str)
{
    bool ok;
    return ::toArrayIndex(str.constData(), str.constData() + str.length(), &ok);
}

