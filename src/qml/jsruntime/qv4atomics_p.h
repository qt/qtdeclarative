/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/
#ifndef QV4ATOMICS_H
#define QV4ATOMICS_H

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

struct Atomics : Object {
    void init();
};

}

struct Atomics : Object
{
    V4_OBJECT2(Atomics, Object)

    static ReturnedValue method_add(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_and(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_compareExchange(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_exchange(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_isLockFree(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_load(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_or(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_store(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_sub(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_wait(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_wake(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_xor(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


} // namespace QV4

QT_END_NAMESPACE

#endif
