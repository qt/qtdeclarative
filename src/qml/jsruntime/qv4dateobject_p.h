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
#ifndef QV4DATEOBJECT_P_H
#define QV4DATEOBJECT_P_H

#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

class QDateTime;

namespace QV4 {

struct DateObject: Object {
    Q_MANAGED
    SafeValue value;
    DateObject(ExecutionEngine *engine, const ValueRef date): Object(engine->dateClass) {
        vtbl = &static_vtbl;
        type = Type_DateObject;
        value = date;
    }
    DateObject(ExecutionEngine *engine, const QDateTime &value);

    QDateTime toQDateTime() const;

protected:
    DateObject(InternalClass *ic): Object(ic) {
        vtbl = &static_vtbl;
        type = Type_DateObject;
        value = Primitive::fromDouble(qSNaN());
    }
};

struct DateCtor: FunctionObject
{
    Q_MANAGED
    DateCtor(ExecutionContext *scope);

    static ReturnedValue construct(Managed *, CallData *callData);
    static ReturnedValue call(Managed *that, CallData *);
};

struct DatePrototype: DateObject
{
    DatePrototype(InternalClass *ic): DateObject(ic) {}
    void init(ExecutionEngine *engine, ObjectRef ctor);

    static double getThisDate(ExecutionContext *ctx);

    static ReturnedValue method_parse(SimpleCallContext *ctx);
    static ReturnedValue method_UTC(SimpleCallContext *ctx);
    static ReturnedValue method_now(SimpleCallContext *ctx);

    static ReturnedValue method_toString(SimpleCallContext *ctx);
    static ReturnedValue method_toDateString(SimpleCallContext *ctx);
    static ReturnedValue method_toTimeString(SimpleCallContext *ctx);
    static ReturnedValue method_toLocaleString(SimpleCallContext *ctx);
    static ReturnedValue method_toLocaleDateString(SimpleCallContext *ctx);
    static ReturnedValue method_toLocaleTimeString(SimpleCallContext *ctx);
    static ReturnedValue method_valueOf(SimpleCallContext *ctx);
    static ReturnedValue method_getTime(SimpleCallContext *ctx);
    static ReturnedValue method_getYear(SimpleCallContext *ctx);
    static ReturnedValue method_getFullYear(SimpleCallContext *ctx);
    static ReturnedValue method_getUTCFullYear(SimpleCallContext *ctx);
    static ReturnedValue method_getMonth(SimpleCallContext *ctx);
    static ReturnedValue method_getUTCMonth(SimpleCallContext *ctx);
    static ReturnedValue method_getDate(SimpleCallContext *ctx);
    static ReturnedValue method_getUTCDate(SimpleCallContext *ctx);
    static ReturnedValue method_getDay(SimpleCallContext *ctx);
    static ReturnedValue method_getUTCDay(SimpleCallContext *ctx);
    static ReturnedValue method_getHours(SimpleCallContext *ctx);
    static ReturnedValue method_getUTCHours(SimpleCallContext *ctx);
    static ReturnedValue method_getMinutes(SimpleCallContext *ctx);
    static ReturnedValue method_getUTCMinutes(SimpleCallContext *ctx);
    static ReturnedValue method_getSeconds(SimpleCallContext *ctx);
    static ReturnedValue method_getUTCSeconds(SimpleCallContext *ctx);
    static ReturnedValue method_getMilliseconds(SimpleCallContext *ctx);
    static ReturnedValue method_getUTCMilliseconds(SimpleCallContext *ctx);
    static ReturnedValue method_getTimezoneOffset(SimpleCallContext *ctx);
    static ReturnedValue method_setTime(SimpleCallContext *ctx);
    static ReturnedValue method_setMilliseconds(SimpleCallContext *ctx);
    static ReturnedValue method_setUTCMilliseconds(SimpleCallContext *ctx);
    static ReturnedValue method_setSeconds(SimpleCallContext *ctx);
    static ReturnedValue method_setUTCSeconds(SimpleCallContext *ctx);
    static ReturnedValue method_setMinutes(SimpleCallContext *ctx);
    static ReturnedValue method_setUTCMinutes(SimpleCallContext *ctx);
    static ReturnedValue method_setHours(SimpleCallContext *ctx);
    static ReturnedValue method_setUTCHours(SimpleCallContext *ctx);
    static ReturnedValue method_setDate(SimpleCallContext *ctx);
    static ReturnedValue method_setUTCDate(SimpleCallContext *ctx);
    static ReturnedValue method_setMonth(SimpleCallContext *ctx);
    static ReturnedValue method_setUTCMonth(SimpleCallContext *ctx);
    static ReturnedValue method_setYear(SimpleCallContext *ctx);
    static ReturnedValue method_setFullYear(SimpleCallContext *ctx);
    static ReturnedValue method_setUTCFullYear(SimpleCallContext *ctx);
    static ReturnedValue method_toUTCString(SimpleCallContext *ctx);
    static ReturnedValue method_toISOString(SimpleCallContext *ctx);
    static ReturnedValue method_toJSON(SimpleCallContext *ctx);

    static void timezoneUpdated();
};

}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
