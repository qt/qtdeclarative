/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <private/qv4value_p.h>
#include <private/qv4identifier_p.h>

QT_BEGIN_NAMESPACE

class QString;

namespace QV4 {

struct PropertyKey : private Value
{
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
    int id;

    static PropertyKey invalid() { PropertyKey key; key.setRawValue(0); return key; }
    static PropertyKey fromArrayIndex(uint idx) { PropertyKey key; key.setInt_32(static_cast<int>(idx)); return key; }
    bool isStringOrSymbol() const { return isManaged(); }
    uint asArrayIndex() const { return isManaged() ? std::numeric_limits<uint>::max() : value(); }
    uint isArrayIndex() const { return !isManaged(); }
    static PropertyKey fromStringOrSymbol(Heap::StringOrSymbol *b)
    { PropertyKey key; key.setM(reinterpret_cast<Heap::Base *>(b)); return key; }
    Heap::StringOrSymbol *asStringOrSymbol() const { return reinterpret_cast<Heap::StringOrSymbol *>(heapObject()); }

    bool isString() const {
        Heap::Base *s = heapObject();
        return s && s->internalClass->vtable->isString;
    }

    bool isSymbol() const {
        Heap::Base *s = heapObject();
        return s && s->internalClass->vtable->isString && s->internalClass->vtable->isStringOrSymbol;
    }

    // ### temporary until we transitioned Identifier to PropertyKey
    static PropertyKey fromIdentifier(Identifier id) {
        if (id.isArrayIndex())
            return PropertyKey::fromArrayIndex(id.asArrayIndex());
        return PropertyKey::fromStringOrSymbol(id.asStringOrSymbol());
    }

    Identifier toIdentifier() const {
        if (isArrayIndex())
            return Identifier::fromArrayIndex(asArrayIndex());
        return Identifier::fromStringOrSymbol(asStringOrSymbol());
    }

    Q_QML_EXPORT QString toQString() const;
    Heap::StringOrSymbol *toStringOrSymbol(ExecutionEngine *e);

    bool operator ==(const PropertyKey &other) const { return id == other.id; }
    bool operator !=(const PropertyKey &other) const { return id != other.id; }
    bool operator <(const PropertyKey &other) const { return id < other.id; }
};

}

QT_END_NAMESPACE

#endif
