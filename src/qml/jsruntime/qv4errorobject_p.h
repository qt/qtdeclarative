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
#ifndef QV4ERROROBJECT_H
#define QV4ERROROBJECT_H

#include "qv4object_p.h"
#include "qv4functionobject_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct SyntaxErrorObject;

struct ErrorObject: Object {
    V4_OBJECT
    Q_MANAGED_TYPE(ErrorObject)
    enum {
        IsErrorObject = true
    };

    enum ErrorType {
        Error,
        EvalError,
        RangeError,
        ReferenceError,
        SyntaxError,
        TypeError,
        URIError
    };

    ErrorObject(InternalClass *ic);
    ErrorObject(InternalClass *ic, const ValueRef message, ErrorType t = Error);
    ErrorObject(InternalClass *ic, const QString &message, ErrorType t = Error);
    ErrorObject(InternalClass *ic, const QString &message, const QString &fileName, int line, int column, ErrorType t = Error);

    SyntaxErrorObject *asSyntaxError();

    StackTrace stackTrace;
    String *stack;

    static ReturnedValue method_get_stack(CallContext *ctx);
    static void markObjects(Managed *that, ExecutionEngine *e);
    static void destroy(Managed *that) { static_cast<ErrorObject *>(that)->~ErrorObject(); }
};

template<>
inline ErrorObject *value_cast(const Value &v) {
    return v.asErrorObject();
}

struct EvalErrorObject: ErrorObject {
    EvalErrorObject(ExecutionEngine *engine, const ValueRef message);
};

struct RangeErrorObject: ErrorObject {
    RangeErrorObject(ExecutionEngine *engine, const ValueRef message);
    RangeErrorObject(ExecutionEngine *engine, const QString &msg);
};

struct ReferenceErrorObject: ErrorObject {
    ReferenceErrorObject(ExecutionEngine *engine, const ValueRef message);
    ReferenceErrorObject(ExecutionEngine *engine, const QString &msg);
    ReferenceErrorObject(ExecutionEngine *engine, const QString &msg, const QString &fileName, int lineNumber, int columnNumber);
};

struct SyntaxErrorObject: ErrorObject {
    V4_OBJECT
    SyntaxErrorObject(ExecutionEngine *engine, const ValueRef msg);
    SyntaxErrorObject(ExecutionEngine *engine, const QString &msg, const QString &fileName, int lineNumber, int columnNumber);
};

struct TypeErrorObject: ErrorObject {
    TypeErrorObject(ExecutionEngine *engine, const ValueRef message);
    TypeErrorObject(ExecutionEngine *engine, const QString &msg);
};

struct URIErrorObject: ErrorObject {
    URIErrorObject(ExecutionEngine *engine, const ValueRef message);
};

struct ErrorCtor: FunctionObject
{
    V4_OBJECT
    ErrorCtor(ExecutionContext *scope);
    ErrorCtor(ExecutionContext *scope, const QString &name);

    static ReturnedValue construct(Managed *, CallData *callData);
    static ReturnedValue call(Managed *that, CallData *callData);
};

struct EvalErrorCtor: ErrorCtor
{
    V4_OBJECT
    EvalErrorCtor(ExecutionContext *scope);

    static ReturnedValue construct(Managed *m, CallData *callData);
};

struct RangeErrorCtor: ErrorCtor
{
    V4_OBJECT
    RangeErrorCtor(ExecutionContext *scope);

    static ReturnedValue construct(Managed *m, CallData *callData);
};

struct ReferenceErrorCtor: ErrorCtor
{
    V4_OBJECT
    ReferenceErrorCtor(ExecutionContext *scope);

    static ReturnedValue construct(Managed *m, CallData *callData);
};

struct SyntaxErrorCtor: ErrorCtor
{
    V4_OBJECT
    SyntaxErrorCtor(ExecutionContext *scope);

    static ReturnedValue construct(Managed *m, CallData *callData);
};

struct TypeErrorCtor: ErrorCtor
{
    V4_OBJECT
    TypeErrorCtor(ExecutionContext *scope);

    static ReturnedValue construct(Managed *m, CallData *callData);
};

struct URIErrorCtor: ErrorCtor
{
    V4_OBJECT
    URIErrorCtor(ExecutionContext *scope);

    static ReturnedValue construct(Managed *m, CallData *callData);
};


struct ErrorPrototype: ErrorObject
{
    // ### shouldn't be undefined
    ErrorPrototype(InternalClass *ic): ErrorObject(ic) {}
    void init(ExecutionEngine *engine, ObjectRef ctor) { init(engine, ctor, this); }

    static void init(ExecutionEngine *engine, ObjectRef ctor, Object *obj);
    static ReturnedValue method_toString(CallContext *ctx);
};

struct EvalErrorPrototype: ErrorObject
{
    EvalErrorPrototype(InternalClass *ic): ErrorObject(ic) { setVTable(staticVTable()); }
    void init(ExecutionEngine *engine, ObjectRef ctor) { ErrorPrototype::init(engine, ctor, this); }
};

struct RangeErrorPrototype: ErrorObject
{
    RangeErrorPrototype(InternalClass *ic): ErrorObject(ic) { setVTable(staticVTable()); }
    void init(ExecutionEngine *engine, ObjectRef ctor) { ErrorPrototype::init(engine, ctor, this); }
};

struct ReferenceErrorPrototype: ErrorObject
{
    ReferenceErrorPrototype(InternalClass *ic): ErrorObject(ic) { setVTable(staticVTable()); }
    void init(ExecutionEngine *engine, ObjectRef ctor) { ErrorPrototype::init(engine, ctor, this); }
};

struct SyntaxErrorPrototype: ErrorObject
{
    SyntaxErrorPrototype(InternalClass *ic): ErrorObject(ic) { setVTable(staticVTable()); }
    void init(ExecutionEngine *engine, ObjectRef ctor) { ErrorPrototype::init(engine, ctor, this); }
};

struct TypeErrorPrototype: ErrorObject
{
    TypeErrorPrototype(InternalClass *ic): ErrorObject(ic) { setVTable(staticVTable()); }
    void init(ExecutionEngine *engine, ObjectRef ctor) { ErrorPrototype::init(engine, ctor, this); }
};

struct URIErrorPrototype: ErrorObject
{
    URIErrorPrototype(InternalClass *ic): ErrorObject(ic) { setVTable(staticVTable()); }
    void init(ExecutionEngine *engine, ObjectRef ctor) { ErrorPrototype::init(engine, ctor, this); }
};


inline SyntaxErrorObject *ErrorObject::asSyntaxError()
{
    return subtype == SyntaxError ? static_cast<SyntaxErrorObject *>(this) : 0;
}

}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
