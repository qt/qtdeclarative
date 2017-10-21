/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
#ifndef QV4DATEOBJECT_P_H
#define QV4DATEOBJECT_P_H

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

#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include <QtCore/private/qnumeric_p.h>

QT_BEGIN_NAMESPACE

class QDateTime;

namespace QV4 {

namespace Heap {

struct DateObject : Object {
    void init()
    {
        Object::init();
        date = qt_qnan();
    }

    void init(const Value &date)
    {
        Object::init();
        this->date = date.toNumber();
    }
    void init(const QDateTime &date);
    void init(const QTime &time);

    double date;
};


struct DateCtor : FunctionObject {
    void init(QV4::ExecutionContext *scope);
};

}

struct DateObject: Object {
    V4_OBJECT2(DateObject, Object)
    Q_MANAGED_TYPE(DateObject)
    V4_PROTOTYPE(datePrototype)


    double date() const { return d()->date; }
    void setDate(double date) { d()->date = date; }

    QDateTime toQDateTime() const;
};

template<>
inline const DateObject *Value::as() const {
    return isManaged() && m()->vtable()->type == Managed::Type_DateObject ? static_cast<const DateObject *>(this) : 0;
}

struct DateCtor: FunctionObject
{
    V4_OBJECT2(DateCtor, FunctionObject)

    static ReturnedValue callAsConstructor(const FunctionObject *, const Value *argv, int argc);
    static ReturnedValue call(const FunctionObject *f, const Value *thisObject, const Value *argv, int);
};

struct DatePrototype: Object
{
    V4_PROTOTYPE(objectPrototype)

    void init(ExecutionEngine *engine, Object *ctor);

    static double getThisDate(ExecutionEngine *v4, CallData *callData);

    static ReturnedValue method_parse(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_UTC(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_now(const BuiltinFunction *, CallData *callData);

    static ReturnedValue method_toString(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_toDateString(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_toTimeString(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_toLocaleString(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_toLocaleDateString(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_toLocaleTimeString(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_valueOf(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getTime(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getYear(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getFullYear(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getUTCFullYear(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getMonth(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getUTCMonth(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getDate(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getUTCDate(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getDay(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getUTCDay(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getHours(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getUTCHours(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getMinutes(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getUTCMinutes(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getSeconds(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getUTCSeconds(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getMilliseconds(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getUTCMilliseconds(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_getTimezoneOffset(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setTime(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setMilliseconds(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setUTCMilliseconds(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setSeconds(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setUTCSeconds(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setMinutes(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setUTCMinutes(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setHours(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setUTCHours(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setDate(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setUTCDate(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setMonth(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setUTCMonth(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setYear(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setFullYear(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_setUTCFullYear(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_toUTCString(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_toISOString(const BuiltinFunction *, CallData *callData);
    static ReturnedValue method_toJSON(const BuiltinFunction *, CallData *callData);

    static void timezoneUpdated();
};

}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
