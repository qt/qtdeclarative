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
#ifndef QV4ARGUMENTSOBJECTS_H
#define QV4ARGUMENTSOBJECTS_H

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

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

#define ArgumentsGetterFunctionMembers(class, Member) \
    Member(class, NoMark, uint, index)

DECLARE_HEAP_OBJECT(ArgumentsGetterFunction, FunctionObject) {
    DECLARE_MARKOBJECTS(ArgumentsGetterFunction);
    inline void init(QV4::ExecutionContext *scope, uint index);
};

#define ArgumentsSetterFunctionMembers(class, Member) \
    Member(class, NoMark, uint, index)

DECLARE_HEAP_OBJECT(ArgumentsSetterFunction, FunctionObject) {
    DECLARE_MARKOBJECTS(ArgumentsSetterFunction);
    inline void init(QV4::ExecutionContext *scope, uint index);
};

#define ArgumentsObjectMembers(class, Member) \
    Member(class, Pointer, CallContext *, context) \
    Member(class, Pointer, MemberData *, mappedArguments) \
    Member(class, NoMark, bool, fullyCreated) \
    Member(class, NoMark, int, nFormals)

DECLARE_HEAP_OBJECT(ArgumentsObject, Object) {
    DECLARE_MARKOBJECTS(ArgumentsObject);
    enum {
        LengthPropertyIndex = 0,
        CalleePropertyIndex = 1
    };
    void init(CppStackFrame *frame);
};

#define StrictArgumentsObjectMembers(class, Member)

DECLARE_HEAP_OBJECT(StrictArgumentsObject, Object) {
    enum {
        LengthPropertyIndex = 0,
        CalleePropertyIndex = 1,
        CallerPropertyIndex = 3
    };
    void init(CppStackFrame *frame);
};

}

struct ArgumentsGetterFunction: FunctionObject
{
    V4_OBJECT2(ArgumentsGetterFunction, FunctionObject)

    uint index() const { return d()->index; }
    static ReturnedValue call(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

inline void
Heap::ArgumentsGetterFunction::init(QV4::ExecutionContext *scope, uint index)
{
    Heap::FunctionObject::init(scope);
    this->index = index;
}

struct ArgumentsSetterFunction: FunctionObject
{
    V4_OBJECT2(ArgumentsSetterFunction, FunctionObject)

    uint index() const { return d()->index; }
    static ReturnedValue call(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

inline void
Heap::ArgumentsSetterFunction::init(QV4::ExecutionContext *scope, uint index)
{
    Heap::FunctionObject::init(scope);
    this->index = index;
}


struct ArgumentsObject: Object {
    V4_OBJECT2(ArgumentsObject, Object)
    Q_MANAGED_TYPE(ArgumentsObject)

    Heap::CallContext *context() const { return d()->context; }
    bool fullyCreated() const { return d()->fullyCreated; }

    static bool isNonStrictArgumentsObject(Managed *m) {
        return m->d()->vtable() == staticVTable();
    }

    bool defineOwnProperty(ExecutionEngine *engine, uint index, const Property *desc, PropertyAttributes attrs);
    static ReturnedValue getIndexed(const Managed *m, uint index, bool *hasProperty);
    static bool putIndexed(Managed *m, uint index, const Value &value);
    static bool deleteIndexedProperty(Managed *m, uint index);
    static PropertyAttributes queryIndexed(const Managed *m, uint index);
    static uint getLength(const Managed *m);

    void fullyCreate();

};

struct StrictArgumentsObject : Object {
    V4_OBJECT2(StrictArgumentsObject, Object)
    Q_MANAGED_TYPE(ArgumentsObject)
};

}

QT_END_NAMESPACE

#endif

