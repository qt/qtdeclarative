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
#ifndef QV4STRINGOBJECT_P_H
#define QV4STRINGOBJECT_P_H

#include "qv4object.h"
#include "qv4functionobject.h"
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

struct StringObject: Object {
    Value value;
    mutable PropertyDescriptor tmpProperty;
    StringObject(ExecutionContext *ctx, const Value &value);

    PropertyDescriptor *getIndex(const ExecutionContext *ctx, uint index) const;

protected:
    static const ManagedVTable static_vtbl;
    static void markObjects(Managed *that);
};

struct StringCtor: FunctionObject
{
    StringCtor(ExecutionContext *scope);

    static Value construct(Managed *that, ExecutionContext *context, Value *args, int argc);
    static Value call(Managed *that, ExecutionContext *, const Value &, Value *, int);

protected:
    static const ManagedVTable static_vtbl;
};

struct StringPrototype: StringObject
{
    StringPrototype(ExecutionContext *ctx): StringObject(ctx, Value::fromString(ctx, QString())) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static Value method_toString(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_charAt(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_charCodeAt(ExecutionContext *, Value thisObject, Value *argv, int argc);
    static Value method_concat(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_indexOf(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_lastIndexOf(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_localeCompare(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_match(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_replace(CallContext *ctx);
    static Value method_search(CallContext *ctx);
    static Value method_slice(CallContext *ctx);
    static Value method_split(CallContext *ctx);
    static Value method_substr(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_substring(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_toLowerCase(CallContext *ctx);
    static Value method_toLocaleLowerCase(CallContext *ctx);
    static Value method_toUpperCase(CallContext *ctx);
    static Value method_toLocaleUpperCase(CallContext *ctx);
    static Value method_fromCharCode(ExecutionContext *parentCtx, Value thisObject, Value *argv, int argc);
    static Value method_trim(CallContext *ctx);
};

} // end of namespace VM
} // end of namespace QQmlJS

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
