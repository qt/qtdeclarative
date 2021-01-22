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
#ifndef QV4GLOBALOBJECT_H
#define QV4GLOBALOBJECT_H

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

#include "qv4global_p.h"
#include "qv4functionobject_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace Heap {

struct EvalFunction : FunctionObject {
    void init(QV4::ExecutionContext *scope);
};

}

struct Q_QML_EXPORT EvalFunction : FunctionObject
{
    V4_OBJECT2(EvalFunction, FunctionObject)

    ReturnedValue evalCall(const Value *thisObject, const Value *argv, int argc, bool directCall) const;

    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct GlobalFunctions
{
    static ReturnedValue method_parseInt(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_parseFloat(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_isNaN(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_isFinite(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_decodeURI(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_decodeURIComponent(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_encodeURI(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_encodeURIComponent(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_escape(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_unescape(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};

}

QT_END_NAMESPACE

#endif // QMLJS_OBJECTS_H
