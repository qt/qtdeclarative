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
#ifndef QV4ARRAYOBJECT_H
#define QV4ARRAYOBJECT_H

#include "qv4object.h"
#include "qv4functionobject.h"
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {


struct ArrayCtor: FunctionObject
{
    ArrayCtor(ExecutionContext *scope);

    static Value construct(Managed *, ExecutionContext *context, Value *args, int argc);
    static Value call(Managed *that, ExecutionContext *, const Value &, Value *, int);

protected:
    static const ManagedVTable static_vtbl;
};

struct ArrayPrototype: ArrayObject
{
    ArrayPrototype(ExecutionContext *context) : ArrayObject(context) {}

    void init(ExecutionContext *ctx, const Value &ctor);

    static uint getLength(ExecutionContext *ctx, Object *o);

    static Value method_isArray(CallContext *ctx);
    static Value method_toString(CallContext *ctx);
    static Value method_toLocaleString(CallContext *ctx);
    static Value method_concat(CallContext *ctx);
    static Value method_join(CallContext *ctx);
    static Value method_pop(CallContext *ctx);
    static Value method_push(CallContext *ctx);
    static Value method_reverse(CallContext *ctx);
    static Value method_shift(CallContext *ctx);
    static Value method_slice(CallContext *ctx);
    static Value method_sort(CallContext *ctx);
    static Value method_splice(CallContext *ctx);
    static Value method_unshift(CallContext *ctx);
    static Value method_indexOf(CallContext *ctx);
    static Value method_lastIndexOf(CallContext *ctx);
    static Value method_every(CallContext *ctx);
    static Value method_some(CallContext *ctx);
    static Value method_forEach(CallContext *ctx);
    static Value method_map(CallContext *ctx);
    static Value method_filter(CallContext *ctx);
    static Value method_reduce(CallContext *ctx);
    static Value method_reduceRight(CallContext *ctx);
};


} // end of namespace VM
} // end of namespace QQmlJS

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
