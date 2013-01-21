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

namespace QQmlJS {
namespace VM {

struct DateObject: Object {
    Value value;
    DateObject(const Value &value): value(value) {}
    virtual QString className() { return QStringLiteral("Date"); }
    virtual DateObject *asDateObject() { return this; }
};

struct DateCtor: FunctionObject
{
    DateCtor(ExecutionContext *scope);

    virtual Value construct(ExecutionContext *ctx);
    virtual Value call(ExecutionContext *ctx);
};

struct DatePrototype: DateObject
{
    DatePrototype(): DateObject(Value::fromDouble(qSNaN())) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static double getThisDate(ExecutionContext *ctx);

    static Value method_parse(ExecutionContext *ctx);
    static Value method_UTC(ExecutionContext *ctx);
    static Value method_now(ExecutionContext *ctx);

    static Value method_toString(ExecutionContext *ctx);
    static Value method_toDateString(ExecutionContext *ctx);
    static Value method_toTimeString(ExecutionContext *ctx);
    static Value method_toLocaleString(ExecutionContext *ctx);
    static Value method_toLocaleDateString(ExecutionContext *ctx);
    static Value method_toLocaleTimeString(ExecutionContext *ctx);
    static Value method_valueOf(ExecutionContext *ctx);
    static Value method_getTime(ExecutionContext *ctx);
    static Value method_getYear(ExecutionContext *ctx);
    static Value method_getFullYear(ExecutionContext *ctx);
    static Value method_getUTCFullYear(ExecutionContext *ctx);
    static Value method_getMonth(ExecutionContext *ctx);
    static Value method_getUTCMonth(ExecutionContext *ctx);
    static Value method_getDate(ExecutionContext *ctx);
    static Value method_getUTCDate(ExecutionContext *ctx);
    static Value method_getDay(ExecutionContext *ctx);
    static Value method_getUTCDay(ExecutionContext *ctx);
    static Value method_getHours(ExecutionContext *ctx);
    static Value method_getUTCHours(ExecutionContext *ctx);
    static Value method_getMinutes(ExecutionContext *ctx);
    static Value method_getUTCMinutes(ExecutionContext *ctx);
    static Value method_getSeconds(ExecutionContext *ctx);
    static Value method_getUTCSeconds(ExecutionContext *ctx);
    static Value method_getMilliseconds(ExecutionContext *ctx);
    static Value method_getUTCMilliseconds(ExecutionContext *ctx);
    static Value method_getTimezoneOffset(ExecutionContext *ctx);
    static Value method_setTime(ExecutionContext *ctx);
    static Value method_setMilliseconds(ExecutionContext *ctx);
    static Value method_setUTCMilliseconds(ExecutionContext *ctx);
    static Value method_setSeconds(ExecutionContext *ctx);
    static Value method_setUTCSeconds(ExecutionContext *ctx);
    static Value method_setMinutes(ExecutionContext *ctx);
    static Value method_setUTCMinutes(ExecutionContext *ctx);
    static Value method_setHours(ExecutionContext *ctx);
    static Value method_setUTCHours(ExecutionContext *ctx);
    static Value method_setDate(ExecutionContext *ctx);
    static Value method_setUTCDate(ExecutionContext *ctx);
    static Value method_setMonth(ExecutionContext *ctx);
    static Value method_setUTCMonth(ExecutionContext *ctx);
    static Value method_setYear(ExecutionContext *ctx);
    static Value method_setFullYear(ExecutionContext *ctx);
    static Value method_setUTCFullYear(ExecutionContext *ctx);
    static Value method_toUTCString(ExecutionContext *ctx);
    static Value method_toISOString(ExecutionContext *ctx);
    static Value method_toJSON(ExecutionContext *ctx);
};

} // end of namespace VM
} // end of namespace QQmlJS

#endif // QV4ECMAOBJECTS_P_H
