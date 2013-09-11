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

struct Q_QML_EXPORT String : public Managed {
    enum StringType {
        StringType_Unknown,
        StringType_Regular,
        StringType_UInt,
        StringType_ArrayIndex
    };

    String() : Managed(0), identifier(0), stringHash(UINT_MAX)
    { vtbl = &static_vtbl; type = Type_String; subtype = StringType_Unknown; }
    String(ExecutionEngine *engine, const QString &text);
    ~String() { _data = 0; }

    inline bool isEqualTo(const String *other) const {
        if (this == other)
            return true;
        if (hashValue() != other->hashValue())
            return false;
        if (identifier && identifier == other->identifier)
            return true;
        if (subtype >= StringType_UInt && subtype == other->subtype)
            return true;

        return toQString() == other->toQString();
    }
    inline bool compare(const String *other) {
        return toQString() < other->toQString();
    }

    inline bool isEmpty() const { return _text.isEmpty(); }
    inline const QString &toQString() const {
        return _text;
    }

    inline unsigned hashValue() const {
        if (subtype == StringType_Unknown)
            createHashValue();

        return stringHash;
    }
    uint asArrayIndex() const {
        if (subtype == StringType_Unknown)
            createHashValue();
        if (subtype == StringType_ArrayIndex)
            return stringHash;
        return UINT_MAX;
    }
    uint toUInt(bool *ok) const;

    void makeIdentifier() {
        if (identifier)
            return;
        makeIdentifierImpl();
    }

    void makeIdentifierImpl();

    void createHashValue() const;
    static uint createHashValue(const QChar *ch, int length);
    static uint createHashValue(const char *ch, int length);

    bool startsWithUpper() const {
        return _text.length() && _text.at(0).isUpper();
    }
    int length() const {
        return _text.length();
    }

    static String *cast(const Value &v) {
        return v.asString();
    }

    ReturnedValue asReturnedValue() { return Value::fromString(this).asReturnedValue(); }

    QString _text;
    mutable Identifier *identifier;
    mutable uint stringHash;


protected:
    static void destroy(Managed *);
    static ReturnedValue get(Managed *m, String *name, bool *hasProperty);
    static ReturnedValue getIndexed(Managed *m, uint index, bool *hasProperty);
    static void put(Managed *m, String *name, const Value &value);
    static void putIndexed(Managed *m, uint index, const Value &value);
    static PropertyAttributes query(const Managed *m, String *name);
    static PropertyAttributes queryIndexed(const Managed *m, uint index);
    static bool deleteProperty(Managed *, String *);
    static bool deleteIndexedProperty(Managed *m, uint index);
    static bool isEqualTo(Managed *that, Managed *o);

    static const ManagedVTable static_vtbl;
};

}

QT_END_NAMESPACE

#endif
