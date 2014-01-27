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
#ifndef QV4REGEXPOBJECT_H
#define QV4REGEXPOBJECT_H

#include "qv4runtime_p.h"
#include "qv4engine_p.h"
#include "qv4context_p.h"
#include "qv4functionobject_p.h"
#include "qv4string_p.h"
#include "qv4codegen_p.h"
#include "qv4isel_p.h"
#include "qv4managed_p.h"
#include "qv4property_p.h"
#include "qv4objectiterator_p.h"
#include "qv4regexp_p.h"

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QScopedPointer>
#include <cstdio>
#include <cassert>

QT_BEGIN_NAMESPACE

namespace QV4 {

class RegExp;

struct RegExpObject: Object {
    V4_OBJECT
    Q_MANAGED_TYPE(RegExpObject)
    // needs to be compatible with the flags in qv4jsir_p.h
    enum Flags {
        RegExp_Global     = 0x01,
        RegExp_IgnoreCase = 0x02,
        RegExp_Multiline  = 0x04
    };

    enum {
        Index_ArrayIndex = ArrayObject::LengthPropertyIndex + 1,
        Index_ArrayInput = Index_ArrayIndex + 1
    };

    RegExp* value;
    Property *lastIndexProperty(ExecutionContext *ctx);
    bool global;

    RegExpObject(ExecutionEngine *engine, RegExpRef value, bool global);
    RegExpObject(ExecutionEngine *engine, const QRegExp &re);
    ~RegExpObject() {}

    void init(ExecutionEngine *engine);

    QRegExp toQRegExp() const;
    QString toString() const;
    QString source() const;
    uint flags() const;

protected:
    RegExpObject(InternalClass *ic);
    static void destroy(Managed *that);
    static void markObjects(Managed *that, ExecutionEngine *e);
};

DEFINE_REF(RegExp, Object);

struct RegExpCtor: FunctionObject
{
    V4_OBJECT
    RegExpCtor(ExecutionContext *scope);

    Value lastMatch;
    StringValue lastInput;
    int lastMatchStart;
    int lastMatchEnd;
    void clearLastMatch();

    static ReturnedValue construct(Managed *m, CallData *callData);
    static ReturnedValue call(Managed *that, CallData *callData);
    static void markObjects(Managed *that, ExecutionEngine *e);
};

struct RegExpPrototype: RegExpObject
{
    RegExpPrototype(InternalClass *ic): RegExpObject(ic) {}
    void init(ExecutionEngine *engine, ObjectRef ctor);

    static ReturnedValue method_exec(CallContext *ctx);
    static ReturnedValue method_test(CallContext *ctx);
    static ReturnedValue method_toString(CallContext *ctx);
    static ReturnedValue method_compile(CallContext *ctx);

    template <int index>
    static ReturnedValue method_get_lastMatch_n(CallContext *ctx);
    static ReturnedValue method_get_lastParen(CallContext *ctx);
    static ReturnedValue method_get_input(CallContext *ctx);
    static ReturnedValue method_get_leftContext(CallContext *ctx);
    static ReturnedValue method_get_rightContext(CallContext *ctx);
};

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
