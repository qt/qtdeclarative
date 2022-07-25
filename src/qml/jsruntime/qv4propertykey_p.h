// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#ifndef QV4PROPERTYKEY_H
#define QV4PROPERTYKEY_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qv4global_p.h>
#include <QtCore/qhashfunctions.h>

QT_BEGIN_NAMESPACE

class QString;

namespace QV4 {

struct PropertyKey
{
private:
    // Property keys are Strings, Symbols or unsigned integers.
    // For convenience we derive them from Values, allowing us to store them
    // on the JS stack
    //
    // They do however behave somewhat different than a Value:
    // * If the key is a String, the pointer to the string is stored in the identifier
    // table and thus unique.
    // * If the key is a Symbol it simply points to the referenced symbol object
    // * if the key is an array index (a uint < UINT_MAX), it's encoded as an
    // integer value
    quint64 val;

    // Important: Always keep this in sync with the definitions for Integers and heap objects in Value
    static const quint64 ArrayIndexMask = 0x3800000000000ull;
    enum {
        IsManagedOrUndefined_Shift = 64-15,
    };
    inline bool isManaged() const { return (val >> IsManagedOrUndefined_Shift) == 0; }
    inline quint32 value() const { return val & quint64(~quint32(0)); }

#if QT_POINTER_SIZE == 8
    QML_NEARLY_ALWAYS_INLINE Heap::StringOrSymbol *m() const
    {
        Heap::StringOrSymbol *b;
        memcpy(&b, &val, 8);
        return b;
    }
    QML_NEARLY_ALWAYS_INLINE void setM(Heap::StringOrSymbol *b)
    {
        memcpy(&val, &b, 8);
    }
#elif QT_POINTER_SIZE == 4
    QML_NEARLY_ALWAYS_INLINE Heap::StringOrSymbol *m() const
    {
        Q_STATIC_ASSERT(sizeof(Heap::StringOrSymbol*) == sizeof(quint32));
        Heap::StringOrSymbol *b;
        quint32 v = value();
        memcpy(&b, &v, 4);
        return b;
    }
    QML_NEARLY_ALWAYS_INLINE void setM(Heap::StringOrSymbol *b)
    {
        quint32 v;
        memcpy(&v, &b, 4);
        val = v;
    }
#endif

public:
    static PropertyKey invalid() { PropertyKey key; key.val = 0; return key; }
    static PropertyKey fromArrayIndex(uint idx) { PropertyKey key; key.val = ArrayIndexMask | static_cast<quint64>(idx); return key; }
    bool isStringOrSymbol() const { return isManaged() && val != 0; }
    uint asArrayIndex() const { Q_ASSERT(isArrayIndex()); return static_cast<uint>(val & 0xffffffff); }
    uint isArrayIndex() const { return !isManaged() && val != 0; }
    bool isValid() const { return val != 0; }
    static PropertyKey fromStringOrSymbol(Heap::StringOrSymbol *b)
    { PropertyKey key; key.setM(b); return key; }
    Heap::StringOrSymbol *asStringOrSymbol() const {
        if (!isManaged())
            return nullptr;
        return m();
    }

    Q_QML_PRIVATE_EXPORT bool isString() const;
    Q_QML_PRIVATE_EXPORT bool isSymbol() const;
    bool isCanonicalNumericIndexString() const;

    Q_QML_PRIVATE_EXPORT QString toQString() const;
    Heap::StringOrSymbol *toStringOrSymbol(ExecutionEngine *e);
    quint64 id() const { return val; }
    static PropertyKey fromId(quint64 id) {
        PropertyKey key; key.val = id; return key;
    }

    enum FunctionNamePrefix {
        None,
        Getter,
        Setter
    };
    Heap::String *asFunctionName(ExecutionEngine *e, FunctionNamePrefix prefix) const;

    bool operator ==(const PropertyKey &other) const { return val == other.val; }
    bool operator !=(const PropertyKey &other) const { return val != other.val; }
    bool operator <(const PropertyKey &other) const { return val < other.val; }
    friend size_t qHash(const PropertyKey &key, size_t seed = 0) { return qHash(key.val, seed); }
};

}

QT_END_NAMESPACE

#endif
