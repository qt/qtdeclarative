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

#include "qv4object.h"
#include "qv4functionobject.h"
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

struct DateObject: Object {
    Value value;
    DateObject(ExecutionEngine *engine, const Value &value): Object(engine), value(value) { type = Type_DateObject; }
};

struct DateCtor: FunctionObject
{
    DateCtor(ExecutionContext *scope);

    static Value construct(Managed *, ExecutionContext *context, Value *args, int argc);
    static Value call(Managed *that, ExecutionContext *, const Value &, Value *, int);

protected:
    static const ManagedVTable static_vtbl;
};

struct DatePrototype: DateObject
{
    DatePrototype(ExecutionEngine *engine): DateObject(engine, Value::fromDouble(qSNaN())) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static double getThisDate(ExecutionContext *ctx);

    static Value method_parse(CallContext *ctx);
    static Value method_UTC(CallContext *ctx);
    static Value method_now(CallContext *ctx);

    static Value method_toString(CallContext *ctx);
    static Value method_toDateString(CallContext *ctx);
    static Value method_toTimeString(CallContext *ctx);
    static Value method_toLocaleString(CallContext *ctx);
    static Value method_toLocaleDateString(CallContext *ctx);
    static Value method_toLocaleTimeString(CallContext *ctx);
    static Value method_valueOf(CallContext *ctx);
    static Value method_getTime(CallContext *ctx);
    static Value method_getYear(CallContext *ctx);
    static Value method_getFullYear(CallContext *ctx);
    static Value method_getUTCFullYear(CallContext *ctx);
    static Value method_getMonth(CallContext *ctx);
    static Value method_getUTCMonth(CallContext *ctx);
    static Value method_getDate(CallContext *ctx);
    static Value method_getUTCDate(CallContext *ctx);
    static Value method_getDay(CallContext *ctx);
    static Value method_getUTCDay(CallContext *ctx);
    static Value method_getHours(CallContext *ctx);
    static Value method_getUTCHours(CallContext *ctx);
    static Value method_getMinutes(CallContext *ctx);
    static Value method_getUTCMinutes(CallContext *ctx);
    static Value method_getSeconds(CallContext *ctx);
    static Value method_getUTCSeconds(CallContext *ctx);
    static Value method_getMilliseconds(CallContext *ctx);
    static Value method_getUTCMilliseconds(CallContext *ctx);
    static Value method_getTimezoneOffset(CallContext *ctx);
    static Value method_setTime(CallContext *ctx);
    static Value method_setMilliseconds(CallContext *ctx);
    static Value method_setUTCMilliseconds(CallContext *ctx);
    static Value method_setSeconds(CallContext *ctx);
    static Value method_setUTCSeconds(CallContext *ctx);
    static Value method_setMinutes(CallContext *ctx);
    static Value method_setUTCMinutes(CallContext *ctx);
    static Value method_setHours(CallContext *ctx);
    static Value method_setUTCHours(CallContext *ctx);
    static Value method_setDate(CallContext *ctx);
    static Value method_setUTCDate(CallContext *ctx);
    static Value method_setMonth(CallContext *ctx);
    static Value method_setUTCMonth(CallContext *ctx);
    static Value method_setYear(CallContext *ctx);
    static Value method_setFullYear(CallContext *ctx);
    static Value method_setUTCFullYear(CallContext *ctx);
    static Value method_toUTCString(CallContext *ctx);
    static Value method_toISOString(CallContext *ctx);
    static Value method_toJSON(CallContext *ctx);
};

} // end of namespace VM
} // end of namespace QQmlJS

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
