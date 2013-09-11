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
    Q_MANAGED

    Value value;
    mutable Property tmpProperty;
    StringObject(ExecutionEngine *engine, const Value &value);

    Property *getIndex(uint index) const;

    static bool deleteIndexedProperty(Managed *m, uint index);

protected:
    StringObject(InternalClass *ic);
    static Property *advanceIterator(Managed *m, ObjectIterator *it, String **name, uint *index, PropertyAttributes *attrs);
    static void markObjects(Managed *that);
};

struct StringCtor: FunctionObject
{
    StringCtor(ExecutionContext *scope);

    static Value construct(Managed *m, CallData *callData);
    static Value call(Managed *that, CallData *callData);

protected:
    static const ManagedVTable static_vtbl;
};

struct StringPrototype: StringObject
{
    StringPrototype(InternalClass *ic): StringObject(ic) {}
    void init(ExecutionEngine *engine, const Value &ctor);

    static Value method_toString(SimpleCallContext *context);
    static Value method_charAt(SimpleCallContext *context);
    static Value method_charCodeAt(SimpleCallContext *context);
    static Value method_concat(SimpleCallContext *context);
    static Value method_indexOf(SimpleCallContext *context);
    static Value method_lastIndexOf(SimpleCallContext *context);
    static Value method_localeCompare(SimpleCallContext *context);
    static Value method_match(SimpleCallContext *context);
    static Value method_replace(SimpleCallContext *ctx);
    static Value method_search(SimpleCallContext *ctx);
    static Value method_slice(SimpleCallContext *ctx);
    static Value method_split(SimpleCallContext *ctx);
    static Value method_substr(SimpleCallContext *context);
    static Value method_substring(SimpleCallContext *context);
    static Value method_toLowerCase(SimpleCallContext *ctx);
    static Value method_toLocaleLowerCase(SimpleCallContext *ctx);
    static Value method_toUpperCase(SimpleCallContext *ctx);
    static Value method_toLocaleUpperCase(SimpleCallContext *ctx);
    static Value method_fromCharCode(SimpleCallContext *context);
    static Value method_trim(SimpleCallContext *ctx);
};

}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
