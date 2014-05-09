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
#ifndef QV4ARGUMENTSOBJECTS_H
#define QV4ARGUMENTSOBJECTS_H

#include "qv4object_p.h"
#include "qv4functionobject_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct ArgumentsGetterFunction: FunctionObject
{
    V4_OBJECT
    uint index;

    ArgumentsGetterFunction(ExecutionContext *scope, uint index)
        : FunctionObject(scope), index(index) {
        setVTable(staticVTable());
    }

    static ReturnedValue call(Managed *that, CallData *d);
};

struct ArgumentsSetterFunction: FunctionObject
{
    V4_OBJECT
    uint index;

    ArgumentsSetterFunction(ExecutionContext *scope, uint index)
        : FunctionObject(scope), index(index) {
        setVTable(staticVTable());
    }

    static ReturnedValue call(Managed *that, CallData *callData);
};


struct ArgumentsObject: Object {
    V4_OBJECT
    Q_MANAGED_TYPE(ArgumentsObject)
    CallContext *context;
    bool fullyCreated;
    Members mappedArguments;
    ArgumentsObject(CallContext *context);
    ~ArgumentsObject() {}

    static bool isNonStrictArgumentsObject(Managed *m) {
        return m->internalClass->vtable->type == Type_ArgumentsObject &&
                !static_cast<ArgumentsObject *>(m)->context->strictMode;
    }

    enum {
        LengthPropertyIndex = 0,
        CalleePropertyIndex = 1,
        CallerPropertyIndex = 3
    };
    bool defineOwnProperty(ExecutionContext *ctx, uint index, const Property &desc, PropertyAttributes attrs);
    static ReturnedValue getIndexed(Managed *m, uint index, bool *hasProperty);
    static void putIndexed(Managed *m, uint index, const ValueRef value);
    static bool deleteIndexedProperty(Managed *m, uint index);
    static PropertyAttributes queryIndexed(const Managed *m, uint index);
    static void markObjects(Managed *that, ExecutionEngine *e);
    static void destroy(Managed *);

    void fullyCreate();
};

}

QT_END_NAMESPACE

#endif

