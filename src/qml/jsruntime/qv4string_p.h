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
#ifndef QV4STRING_H
#define QV4STRING_H

#include <QtCore/qstring.h>
#include "qv4managed_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct ExecutionEngine;
struct Identifier;

struct Q_QML_PRIVATE_EXPORT String : public Managed {
#ifndef V4_BOOTSTRAP
    // ### FIXME: Should this be a V4_OBJECT
    V4_OBJECT
    Q_MANAGED_TYPE(String)
    enum {
        IsString = true
    };

    enum StringType {
        StringType_Unknown,
        StringType_Regular,
        StringType_UInt,
        StringType_ArrayIndex
    };

    String(ExecutionEngine *engine, const QString &text);
    String(ExecutionEngine *engine, String *l, String *n);
    ~String() {
        if (!largestSubLength && !_text->ref.deref())
            QStringData::deallocate(_text);
        _data = 0;
    }

    bool equals(const StringRef other) const;
    inline bool isEqualTo(const String *other) const {
        if (this == other)
            return true;
        if (hashValue() != other->hashValue())
            return false;
        Q_ASSERT(!largestSubLength);
        if (identifier && identifier == other->identifier)
            return true;
        if (subtype >= StringType_UInt && subtype == other->subtype)
            return true;

        return toQString() == other->toQString();
    }

    inline bool compare(const String *other) {
        return toQString() < other->toQString();
    }

    inline QString toQString() const {
        if (largestSubLength)
            simplifyString();
        QStringDataPtr ptr = { _text };
        _text->ref.ref();
        return QString(ptr);
    }

    void simplifyString() const;

    inline unsigned hashValue() const {
        if (subtype == StringType_Unknown)
            createHashValue();
        Q_ASSERT(!largestSubLength);

        return stringHash;
    }
    uint asArrayIndex() const {
        if (subtype == StringType_Unknown)
            createHashValue();
        Q_ASSERT(!largestSubLength);
        if (subtype == StringType_ArrayIndex)
            return stringHash;
        return UINT_MAX;
    }
    uint toUInt(bool *ok) const;

    void makeIdentifier() const {
        if (identifier)
            return;
        makeIdentifierImpl();
    }

    void makeIdentifierImpl() const;

    void createHashValue() const;
    static uint createHashValue(const QChar *ch, int length);
    static uint createHashValue(const char *ch, int length);

    bool startsWithUpper() const {
        const String *l = this;
        while (l->largestSubLength)
            l = l->left;
        return l->_text->size && QChar::isUpper(l->_text->data()[0]);
    }
    int length() const {
        Q_ASSERT((largestSubLength && (len == left->len + right->len)) || len == (uint)_text->size);
        return len;
    }

    union {
        mutable QStringData *_text;
        mutable String *left;
    };
    union {
        mutable Identifier *identifier;
        mutable String *right;
    };
    mutable uint stringHash;
    mutable uint largestSubLength;
    uint len;


protected:
    static void destroy(Managed *);
    static void markObjects(Managed *that, ExecutionEngine *e);
    static ReturnedValue get(Managed *m, const StringRef name, bool *hasProperty);
    static ReturnedValue getIndexed(Managed *m, uint index, bool *hasProperty);
    static void put(Managed *m, const StringRef name, const ValueRef value);
    static void putIndexed(Managed *m, uint index, const ValueRef value);
    static PropertyAttributes query(const Managed *m, StringRef name);
    static PropertyAttributes queryIndexed(const Managed *m, uint index);
    static bool deleteProperty(Managed *, const StringRef);
    static bool deleteIndexedProperty(Managed *m, uint index);
    static bool isEqualTo(Managed *that, Managed *o);
    static uint getLength(const Managed *m);

private:
    QChar *recursiveAppend(QChar *ch) const;
#endif

public:
    static uint toArrayIndex(const QString &str);
};

#ifndef V4_BOOTSTRAP
template<>
inline String *value_cast(const Value &v) {
    return v.asString();
}

template<>
inline ReturnedValue value_convert<String>(ExecutionEngine *e, const Value &v)
{
    return v.toString(e)->asReturnedValue();
}

DEFINE_REF(String, Managed);
#endif

}

QT_END_NAMESPACE

#endif
