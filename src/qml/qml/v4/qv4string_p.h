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
#ifndef QV4STRING_H
#define QV4STRING_H

#include <QtCore/qstring.h>
#include "qv4managed_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct ExecutionEngine;

struct String : public Managed {
    enum StringType {
        StringType_Unknown,
        StringType_Regular,
        StringType_UInt,
        StringType_ArrayIndex
    };

    String() : Managed(0), stringHash(UINT_MAX), identifier(UINT_MAX)
    { vtbl = &static_vtbl; type = Type_String; subtype = StringType_Unknown; }
    String(ExecutionEngine *engine, const QString &text);
    ~String() { _data = 0; }

    inline bool isEqualTo(const String *other) const {
        if (this == other)
            return true;
        if (hashValue() != other->hashValue())
            return false;
        if (identifier != UINT_MAX && identifier == other->identifier)
            return true;
        if (subtype >= StringType_UInt && subtype == other->subtype)
            return true;

        return toQString() == other->toQString();
    }
    inline bool compare(const String *other) {
        return toQString() < other->toQString();
    }

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

    void makeIdentifier(const ExecutionContext *ctx) {
        if (identifier != UINT_MAX)
            return;
        makeIdentifierImpl(ctx);
    }

    void makeIdentifierImpl(const ExecutionContext *ctx);

    void createHashValue() const;
    static uint createHashValue(const QChar *ch, int length);

    QString _text;
    mutable uint stringHash;
    mutable uint identifier;

protected:
    static void destroy(Managed *);
    static Value get(Managed *m, ExecutionContext *ctx, String *name, bool *hasProperty);
    static Value getIndexed(Managed *m, ExecutionContext *ctx, uint index, bool *hasProperty);
    static void put(Managed *m, ExecutionContext *ctx, String *name, const Value &value);
    static void putIndexed(Managed *m, ExecutionContext *ctx, uint index, const Value &value);
    static PropertyAttributes query(Managed *m, ExecutionContext *ctx, String *name);
    static PropertyAttributes queryIndexed(Managed *m, ExecutionContext *ctx, uint index);
    static bool deleteProperty(Managed *m, ExecutionContext *ctx, String *name);
    static bool deleteIndexedProperty(Managed *m, ExecutionContext *ctx, uint index);

    static const ManagedVTable static_vtbl;
};

}

QT_END_NAMESPACE

#endif
