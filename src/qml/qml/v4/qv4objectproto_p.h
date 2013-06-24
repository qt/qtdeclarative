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
#ifndef QV4ECMAOBJECTS_P_H
#define QV4ECMAOBJECTS_P_H

#include "qv4object_p.h"
#include "qv4functionobject_p.h"
#include <QtCore/qnumeric.h>

QT_BEGIN_NAMESPACE

namespace QV4 {

struct ObjectCtor: FunctionObject
{
    ObjectCtor(ExecutionContext *scope);

    static Value construct(Managed *that, Value *args, int argc);
    static Value call(Managed *that, const Value &, Value *, int);

protected:
    static const ManagedVTable static_vtbl;
};

struct ObjectPrototype: Object
{
    ObjectPrototype(ExecutionEngine *engine) : Object(engine) {}

    void init(ExecutionContext *ctx, const Value &ctor);

    static Value method_getPrototypeOf(SimpleCallContext *ctx);
    static Value method_getOwnPropertyDescriptor(SimpleCallContext *ctx);
    static Value method_getOwnPropertyNames(SimpleCallContext *context);
    static Value method_create(SimpleCallContext *ctx);
    static Value method_defineProperty(SimpleCallContext *ctx);
    static Value method_defineProperties(SimpleCallContext *ctx);
    static Value method_seal(SimpleCallContext *ctx);
    static Value method_freeze(SimpleCallContext *ctx);
    static Value method_preventExtensions(SimpleCallContext *ctx);
    static Value method_isSealed(SimpleCallContext *ctx);
    static Value method_isFrozen(SimpleCallContext *ctx);
    static Value method_isExtensible(SimpleCallContext *ctx);
    static Value method_keys(SimpleCallContext *ctx);

    static Value method_toString(SimpleCallContext *ctx);
    static Value method_toLocaleString(SimpleCallContext *ctx);
    static Value method_valueOf(SimpleCallContext *ctx);
    static Value method_hasOwnProperty(SimpleCallContext *ctx);
    static Value method_isPrototypeOf(SimpleCallContext *ctx);
    static Value method_propertyIsEnumerable(SimpleCallContext *ctx);

    static Value method_defineGetter(SimpleCallContext *ctx);
    static Value method_defineSetter(SimpleCallContext *ctx);

    static Value method_get_proto(SimpleCallContext *ctx);
    static Value method_set_proto(SimpleCallContext *ctx);

    static void toPropertyDescriptor(ExecutionContext *ctx, Value v, Property *desc, PropertyAttributes *attrs);
    static Value fromPropertyDescriptor(ExecutionContext *ctx, const Property *desc, PropertyAttributes attrs);

    static ArrayObject *getOwnPropertyNames(ExecutionEngine *v4, const Value &o);
};


}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
