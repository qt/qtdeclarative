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
#ifndef QV4STRINGOBJECT_P_H
#define QV4STRINGOBJECT_P_H

#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct StringObject: Object {
    V4_OBJECT
    Q_MANAGED_TYPE(StringObject)

    Value value;
    mutable Property tmpProperty;
    StringObject(ExecutionEngine *engine, const ValueRef value);

    Property *getIndex(uint index) const;

    static bool deleteIndexedProperty(Managed *m, uint index);

protected:
    StringObject(InternalClass *ic);
    static void advanceIterator(Managed *m, ObjectIterator *it, StringRef name, uint *index, Property *p, PropertyAttributes *attrs);
    static void markObjects(Managed *that, ExecutionEngine *e);
};

struct StringCtor: FunctionObject
{
    V4_OBJECT
    StringCtor(ExecutionContext *scope);

    static ReturnedValue construct(Managed *m, CallData *callData);
    static ReturnedValue call(Managed *that, CallData *callData);
};

struct StringPrototype: StringObject
{
    StringPrototype(InternalClass *ic): StringObject(ic) {}
    void init(ExecutionEngine *engine, ObjectRef ctor);

    static ReturnedValue method_toString(CallContext *context);
    static ReturnedValue method_charAt(CallContext *context);
    static ReturnedValue method_charCodeAt(CallContext *context);
    static ReturnedValue method_concat(CallContext *context);
    static ReturnedValue method_indexOf(CallContext *context);
    static ReturnedValue method_lastIndexOf(CallContext *context);
    static ReturnedValue method_localeCompare(CallContext *context);
    static ReturnedValue method_match(CallContext *context);
    static ReturnedValue method_replace(CallContext *ctx);
    static ReturnedValue method_search(CallContext *ctx);
    static ReturnedValue method_slice(CallContext *ctx);
    static ReturnedValue method_split(CallContext *ctx);
    static ReturnedValue method_substr(CallContext *context);
    static ReturnedValue method_substring(CallContext *context);
    static ReturnedValue method_toLowerCase(CallContext *ctx);
    static ReturnedValue method_toLocaleLowerCase(CallContext *ctx);
    static ReturnedValue method_toUpperCase(CallContext *ctx);
    static ReturnedValue method_toLocaleUpperCase(CallContext *ctx);
    static ReturnedValue method_fromCharCode(CallContext *context);
    static ReturnedValue method_trim(CallContext *ctx);
};

}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
