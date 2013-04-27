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
#ifndef QV4DATEOBJECT_P_H
#define QV4DATEOBJECT_P_H

#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

class QDateTime;

namespace QV4 {

struct DateObject: Object {
    Value value;
    DateObject(ExecutionEngine *engine, const Value &value): Object(engine), value(value) { type = Type_DateObject; }
    DateObject(ExecutionEngine *engine, const QDateTime &value);

    QDateTime toQDateTime() const;
};

struct QV4_JS_CLASS(DatePrototype): DateObject
{
    QV4_ANNOTATE(argc 7)

    DatePrototype(ExecutionEngine *engine): DateObject(engine, Value::fromDouble(qSNaN())) {}
    void init(ExecutionContext *ctx, const Value &ctor);
    void initClass(ExecutionEngine *ctx, const Value &ctor);
    static Object *newConstructor(ExecutionContext *scope);

    static double getThisDate(ExecutionContext *ctx);

    static Value ctor_method_construct(Managed *, ExecutionContext *context, Value *args, int argc);
    static Value ctor_method_call(Managed *that, ExecutionContext *, const Value &, Value *, int);

    static Value ctor_method_parse(SimpleCallContext *ctx) QV4_ARGC(1);
    static Value ctor_method_UTC(SimpleCallContext *ctx) QV4_ARGC(7);
    static Value ctor_method_now(SimpleCallContext *ctx);

    static Value method_toString(SimpleCallContext *ctx);
    static Value method_toDateString(SimpleCallContext *ctx);
    static Value method_toTimeString(SimpleCallContext *ctx);
    static Value method_toLocaleString(SimpleCallContext *ctx);
    static Value method_toLocaleDateString(SimpleCallContext *ctx);
    static Value method_toLocaleTimeString(SimpleCallContext *ctx);
    static Value method_valueOf(SimpleCallContext *ctx);
    static Value method_getTime(SimpleCallContext *ctx);
    static Value method_getYear(SimpleCallContext *ctx);
    static Value method_getFullYear(SimpleCallContext *ctx);
    static Value method_getUTCFullYear(SimpleCallContext *ctx);
    static Value method_getMonth(SimpleCallContext *ctx);
    static Value method_getUTCMonth(SimpleCallContext *ctx);
    static Value method_getDate(SimpleCallContext *ctx);
    static Value method_getUTCDate(SimpleCallContext *ctx);
    static Value method_getDay(SimpleCallContext *ctx);
    static Value method_getUTCDay(SimpleCallContext *ctx);
    static Value method_getHours(SimpleCallContext *ctx);
    static Value method_getUTCHours(SimpleCallContext *ctx);
    static Value method_getMinutes(SimpleCallContext *ctx);
    static Value method_getUTCMinutes(SimpleCallContext *ctx);
    static Value method_getSeconds(SimpleCallContext *ctx);
    static Value method_getUTCSeconds(SimpleCallContext *ctx);
    static Value method_getMilliseconds(SimpleCallContext *ctx);
    static Value method_getUTCMilliseconds(SimpleCallContext *ctx);
    static Value method_getTimezoneOffset(SimpleCallContext *ctx);
    static Value method_setTime(SimpleCallContext *ctx) QV4_ARGC(1);
    static Value method_setMilliseconds(SimpleCallContext *ctx) QV4_ARGC(1);
    static Value method_setUTCMilliseconds(SimpleCallContext *ctx) QV4_ARGC(1);
    static Value method_setSeconds(SimpleCallContext *ctx) QV4_ARGC(2);
    static Value method_setUTCSeconds(SimpleCallContext *ctx) QV4_ARGC(2);
    static Value method_setMinutes(SimpleCallContext *ctx) QV4_ARGC(3);
    static Value method_setUTCMinutes(SimpleCallContext *ctx) QV4_ARGC(3);
    static Value method_setHours(SimpleCallContext *ctx) QV4_ARGC(4);
    static Value method_setUTCHours(SimpleCallContext *ctx) QV4_ARGC(4);
    static Value method_setDate(SimpleCallContext *ctx) QV4_ARGC(1);
    static Value method_setUTCDate(SimpleCallContext *ctx) QV4_ARGC(1);
    static Value method_setMonth(SimpleCallContext *ctx) QV4_ARGC(2);
    static Value method_setUTCMonth(SimpleCallContext *ctx) QV4_ARGC(2);
    static Value method_setYear(SimpleCallContext *ctx) QV4_ARGC(1);
    static Value method_setFullYear(SimpleCallContext *ctx) QV4_ARGC(3);
    static Value method_setUTCFullYear(SimpleCallContext *ctx) QV4_ARGC(3);
    static Value method_toUTCString(SimpleCallContext *ctx) QV4_ANNOTATE(alias toGMTString);
    static Value method_toISOString(SimpleCallContext *ctx);
    static Value method_toJSON(SimpleCallContext *ctx) QV4_ARGC(1);
};

}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
