// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qv4string_p.h"
#include "qv4value_p.h"
#include "qv4identifiertable_p.h"
#include "qv4runtime_p.h"
#include <QtQml/private/qv4mm_p.h>
#include <QtCore/QHash>
#include <QtCore/private/qnumeric_p.h>

using namespace QV4;

void Heap::StringOrSymbol::markObjects(Heap::Base *that, MarkStack *markStack)
{
    StringOrSymbol *s = static_cast<StringOrSymbol *>(that);
    Heap::StringOrSymbol *id = s->identifier.asStringOrSymbol();
    if (id)
        id->mark(markStack);
}

void Heap::String::markObjects(Heap::Base *that, MarkStack *markStack)
{
    StringOrSymbol::markObjects(that, markStack);
    String *s = static_cast<String *>(that);
    if (s->subtype < StringType_Complex)
        return;

    ComplexString *cs = static_cast<ComplexString *>(s);
    if (cs->subtype == StringType_AddedString) {
        cs->left->mark(markStack);
        cs->right->mark(markStack);
    } else {
        Q_ASSERT(cs->subtype == StringType_SubString);
        cs->left->mark(markStack);
    }
}

DEFINE_MANAGED_VTABLE(StringOrSymbol);
DEFINE_MANAGED_VTABLE(String);


bool String::virtualIsEqualTo(Managed *t, Managed *o)
{
    if (t == o)
        return true;

    if (!o->vtable()->isString)
        return false;

    return static_cast<String *>(t)->isEqualTo(static_cast<String *>(o));
}


void Heap::String::init(const QString &t)
{
    QString mutableText(t);
    StringOrSymbol::init(mutableText.data_ptr());
    subtype = String::StringType_Unknown;
}

void Heap::ComplexString::init(String *l, String *r)
{
    StringOrSymbol::init();
    subtype = String::StringType_AddedString;

    left = l;
    right = r;
    len = left->length() + right->length();
    if (left->subtype >= StringType_Complex)
        largestSubLength = static_cast<ComplexString *>(left)->largestSubLength;
    else
        largestSubLength = left->length();
    if (right->subtype >= StringType_Complex)
        largestSubLength = qMax(largestSubLength, static_cast<ComplexString *>(right)->largestSubLength);
    else
        largestSubLength = qMax(largestSubLength, right->length());

    // make sure we don't get excessive depth in our strings
    if (len > 256 && len >= 2*largestSubLength)
        simplifyString();
}

void Heap::ComplexString::init(Heap::String *ref, int from, int len)
{
    Q_ASSERT(ref->length() >= from + len);
    StringOrSymbol::init();

    subtype = String::StringType_SubString;

    left = ref;
    this->from = from;
    this->len = len;
}

void Heap::StringOrSymbol::destroy()
{
    if (subtype < Heap::String::StringType_AddedString) {
        internalClass->engine->memoryManager->changeUnmanagedHeapSizeUsage(
                    qptrdiff(-text()->size) * qptrdiff(sizeof(QChar)));
    }
    text().~QStringPrivate();
    Base::destroy();
}

uint String::toUInt(bool *ok) const
{
    *ok = true;

    if (subtype() >= Heap::String::StringType_Unknown)
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

void String::createPropertyKeyImpl() const
{
    if (d()->subtype >= Heap::String::StringType_AddedString)
        d()->simplifyString();
    Q_ASSERT(d()->subtype < Heap::String::StringType_AddedString);
    engine()->identifierTable->asPropertyKey(this);
}

void Heap::String::simplifyString() const
{
    Q_ASSERT(subtype >= StringType_AddedString);

    int l = length();
    QString result(l, Qt::Uninitialized);
    QChar *ch = const_cast<QChar *>(result.constData());
    append(this, ch);
    text() = result.data_ptr();
    const ComplexString *cs = static_cast<const ComplexString *>(this);
    identifier = PropertyKey::invalid();
    cs->left = cs->right = nullptr;

    internalClass->engine->memoryManager->changeUnmanagedHeapSizeUsage(
                qptrdiff(text().size) * qptrdiff(sizeof(QChar)));
    subtype = StringType_Unknown;
}

bool Heap::String::startsWithUpper() const
{
    if (subtype == StringType_AddedString)
        return static_cast<const Heap::ComplexString *>(this)->left->startsWithUpper();

    const Heap::String *str = this;
    int offset = 0;
    if (subtype == StringType_SubString) {
        const ComplexString *cs = static_cast<const Heap::ComplexString *>(this);
        if (!cs->len)
            return false;
        // simplification here is not ideal, but hopefully not a common case.
        if (cs->left->subtype >= Heap::String::StringType_Complex)
            cs->left->simplifyString();
        str = cs->left;
        offset = cs->from;
    }
    Q_ASSERT(str->subtype < Heap::String::StringType_Complex);
    return str->text().size > offset && QChar::isUpper(str->text().data()[offset]);
}

void Heap::String::append(const String *data, QChar *ch)
{
    // in-order visitation with explicit stack
    // where leaf nodes are "real" strings that get appended to ch

    enum StatusTag : bool { NotVisited, Visited };
    using Pointer = QTaggedPointer<const String, StatusTag>;

    std::vector<Pointer> worklist;
    worklist.reserve(32);
    worklist.push_back(Pointer(data));

    while (!worklist.empty()) {
        Pointer item = worklist.back();
        if (item.tag() == Visited) {
            Q_ASSERT(item->subtype == StringType_AddedString);
            const ComplexString *cs = static_cast<const ComplexString *>(item.data());
            worklist.pop_back();
            worklist.push_back(Pointer(cs->right));
            continue;
        }

        if (item->subtype == StringType_AddedString) {
            // we need to keep the node in the worklist, as we still need to handle "right"
            worklist.back().setTag(Visited);
            const ComplexString *cs = static_cast<const ComplexString *>(item.data());
            worklist.push_back(Pointer(cs->left));
        } else if (item->subtype == StringType_SubString) {
            worklist.pop_back();
            const ComplexString *cs = static_cast<const ComplexString *>(item.data());
            memcpy(ch, cs->left->toQString().constData() + cs->from, cs->len*sizeof(QChar));
            ch += cs->len;
        } else {
            worklist.pop_back();
            memcpy(static_cast<void *>(ch), item->text().data(), item->text().size * sizeof(QChar));
            ch += item->text().size;
        }
    }
}

void Heap::StringOrSymbol::createHashValue() const
{
    if (subtype >= StringType_AddedString) {
        Q_ASSERT(internalClass->vtable->isString);
        static_cast<const Heap::String *>(this)->simplifyString();
    }
    Q_ASSERT(subtype < StringType_AddedString);
    const QChar *ch = reinterpret_cast<const QChar *>(text().data());
    const QChar *end = ch + text().size;
    stringHash = QV4::String::calculateHashValue(ch, end, &subtype);
}

qint64 String::virtualGetLength(const Managed *m)
{
    return static_cast<const String *>(m)->d()->length();
}
