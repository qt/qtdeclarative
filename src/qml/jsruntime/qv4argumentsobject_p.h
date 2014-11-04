/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

namespace Heap {

struct ArgumentsGetterFunction : FunctionObject {
    inline ArgumentsGetterFunction(QV4::ExecutionContext *scope, uint index);
    uint index;
};

struct ArgumentsSetterFunction : FunctionObject {
    inline ArgumentsSetterFunction(QV4::ExecutionContext *scope, uint index);
    uint index;
};

struct ArgumentsObject : Object {
    enum {
        LengthPropertyIndex = 0,
        CalleePropertyIndex = 1,
        CallerPropertyIndex = 3
    };
    ArgumentsObject(QV4::CallContext *context);
    QV4::CallContext *context;
    bool fullyCreated;
    MemberData *mappedArguments;
};

}

struct ArgumentsGetterFunction: FunctionObject
{
    V4_OBJECT2(ArgumentsGetterFunction, FunctionObject)

    uint index() const { return d()->index; }
    static ReturnedValue call(Managed *that, CallData *d);
};

inline
Heap::ArgumentsGetterFunction::ArgumentsGetterFunction(QV4::ExecutionContext *scope, uint index)
    : Heap::FunctionObject(scope)
    , index(index)
{
    setVTable(QV4::ArgumentsGetterFunction::staticVTable());
}

struct ArgumentsSetterFunction: FunctionObject
{
    V4_OBJECT2(ArgumentsSetterFunction, FunctionObject)

    uint index() const { return d()->index; }
    static ReturnedValue call(Managed *that, CallData *callData);
};

inline
Heap::ArgumentsSetterFunction::ArgumentsSetterFunction(QV4::ExecutionContext *scope, uint index)
    : Heap::FunctionObject(scope)
    , index(index)
{
    setVTable(QV4::ArgumentsSetterFunction::staticVTable());
}


struct ArgumentsObject: Object {
    V4_OBJECT2(ArgumentsObject, Object)
    Q_MANAGED_TYPE(ArgumentsObject)

    CallContext *context() const { return d()->context; }
    bool fullyCreated() const { return d()->fullyCreated; }
    Heap::MemberData *mappedArguments() { return d()->mappedArguments; }

    static bool isNonStrictArgumentsObject(Managed *m) {
        return m->internalClass()->vtable->type == Type_ArgumentsObject &&
                !static_cast<ArgumentsObject *>(m)->context()->d()->strictMode;
    }

    bool defineOwnProperty(ExecutionContext *ctx, uint index, const Property &desc, PropertyAttributes attrs);
    static ReturnedValue getIndexed(Managed *m, uint index, bool *hasProperty);
    static void putIndexed(Managed *m, uint index, const ValueRef value);
    static bool deleteIndexedProperty(Managed *m, uint index);
    static PropertyAttributes queryIndexed(const Managed *m, uint index);
    static void markObjects(Heap::Base *that, ExecutionEngine *e);

    void fullyCreate();
};

}

QT_END_NAMESPACE

#endif

