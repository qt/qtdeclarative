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

#include "qv4string_p.h"
#include "qv4value_p.h"
#ifndef V4_BOOTSTRAP
#include "qv4identifiertable_p.h"
#include "qv4runtime_p.h"
#include "qv4objectproto_p.h"
#include "qv4stringobject_p.h"
#endif
#include <QtCore/QHash>

using namespace QV4;

static uint toArrayIndex(const QChar *ch, const QChar *end)
{
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
    return i;
}

#ifndef V4_BOOTSTRAP

static uint toArrayIndex(const char *ch, const char *end)
{
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
    return i;
}


DEFINE_MANAGED_VTABLE(String);

void String::markObjects(Heap::Base *that, ExecutionEngine *e)
{
    String::Data *s = static_cast<String::Data *>(that);
    if (s->largestSubLength) {
        s->left->mark(e);
        s->right->mark(e);
    }
}

bool String::isEqualTo(Managed *t, Managed *o)
{
    if (t == o)
        return true;

    if (!o->d()->vtable()->isString)
        return false;

    return static_cast<String *>(t)->isEqualTo(static_cast<String *>(o));
}


Heap::String::String(MemoryManager *mm, const QString &t)
    : mm(mm)
{
    subtype = String::StringType_Unknown;

    text = const_cast<QString &>(t).data_ptr();
    text->ref.ref();
    identifier = 0;
    stringHash = UINT_MAX;
    largestSubLength = 0;
    len = text->size;
}

Heap::String::String(MemoryManager *mm, String *l, String *r)
    : mm(mm)
{
    subtype = String::StringType_Unknown;

    left = l;
    right = r;
    stringHash = UINT_MAX;
    largestSubLength = qMax(l->largestSubLength, r->largestSubLength);
    len = l->len + r->len;

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

    if (subtype() == Heap::String::StringType_Unknown)
        d()->createHashValue();
    if (subtype() == Heap::String::StringType_ArrayIndex)
        return d()->stringHash;

    // required for UINT_MAX or numbers starting with a leading 0
    double d = RuntimeHelpers::stringToNumber(toQString());
    uint l = (uint)d;
    if (d == l)
        return l;
    *ok = false;
    return UINT_MAX;
}

void String::makeIdentifierImpl(ExecutionEngine *e) const
{
    if (d()->largestSubLength)
        d()->simplifyString();
    Q_ASSERT(!d()->largestSubLength);
    e->identifierTable->identifier(this);
}

void Heap::String::simplifyString() const
{
    Q_ASSERT(largestSubLength);

    int l = length();
    QString result(l, Qt::Uninitialized);
    QChar *ch = const_cast<QChar *>(result.constData());
    append(this, ch);
    text = result.data_ptr();
    text->ref.ref();
    identifier = 0;
    largestSubLength = 0;
    mm->growUnmanagedHeapSizeUsage(size_t(text->size) * sizeof(QChar));
}

void Heap::String::createHashValue() const
{
    if (largestSubLength)
        simplifyString();
    Q_ASSERT(!largestSubLength);
    const QChar *ch = reinterpret_cast<const QChar *>(text->data());
    const QChar *end = ch + text->size;

    // array indices get their number as hash value
    stringHash = ::toArrayIndex(ch, end);
    if (stringHash != UINT_MAX) {
        subtype = Heap::String::StringType_ArrayIndex;
        return;
    }

    uint h = 0xffffffff;
    while (ch < end) {
        h = 31 * h + ch->unicode();
        ++ch;
    }

    stringHash = h;
    subtype = Heap::String::StringType_Regular;
}

void Heap::String::append(const String *data, QChar *ch)
{
    std::vector<const String *> worklist;
    worklist.reserve(32);
    worklist.push_back(data);

    while (!worklist.empty()) {
        const String *item = worklist.back();
        worklist.pop_back();

        if (item->largestSubLength) {
            worklist.push_back(item->right);
            worklist.push_back(item->left);
        } else {
            memcpy(ch, item->text->data(), item->text->size * sizeof(QChar));
            ch += item->text->size;
        }
    }
}




uint String::createHashValue(const QChar *ch, int length)
{
    const QChar *end = ch + length;

    // array indices get their number as hash value
    uint stringHash = ::toArrayIndex(ch, end);
    if (stringHash != UINT_MAX)
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
    uint stringHash = ::toArrayIndex(ch, end);
    if (stringHash != UINT_MAX)
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
    return static_cast<const String *>(m)->d()->length();
}

#endif // V4_BOOTSTRAP

uint String::toArrayIndex(const QString &str)
{
    return ::toArrayIndex(str.constData(), str.constData() + str.length());
}

