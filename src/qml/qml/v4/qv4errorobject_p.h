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
#ifndef QV4ERROROBJECT_H
#define QV4ERROROBJECT_H

#include "qv4object_p.h"
#include "qv4functionobject_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

struct SyntaxErrorObject;

struct ErrorObject: Object {
    Q_MANAGED

    enum ErrorType {
        Error,
        EvalError,
        RangeError,
        ReferenceError,
        SyntaxError,
        TypeError,
        URIError
    };

    ErrorObject(ExecutionEngine *engine, const Value &message, ErrorType t = Error);

    SyntaxErrorObject *asSyntaxError();

    ExecutionEngine::StackTrace stackTrace;
    String *stack;

    static Value method_get_stack(SimpleCallContext *ctx);
    static void markObjects(Managed *that);
    static void destroy(Managed *that) { static_cast<ErrorObject *>(that)->~ErrorObject(); }
};

struct EvalErrorObject: ErrorObject {
    EvalErrorObject(ExecutionEngine *engine, const Value &message);
};

struct RangeErrorObject: ErrorObject {
    RangeErrorObject(ExecutionEngine *engine, const Value &message);
    RangeErrorObject(ExecutionEngine *engine, const QString &msg);
};

struct ReferenceErrorObject: ErrorObject {
    ReferenceErrorObject(ExecutionEngine *engine, const Value &message);
    ReferenceErrorObject(ExecutionEngine *engine, const QString &msg);
    ReferenceErrorObject(ExecutionEngine *engine, const QString &msg, const QString &fileName, int lineNumber);
};

struct SyntaxErrorObject: ErrorObject {
    SyntaxErrorObject(ExecutionEngine *engine, const Value &msg);
    SyntaxErrorObject(ExecutionEngine *engine, const QString &msg);
    SyntaxErrorObject(ExecutionContext *ctx, DiagnosticMessage *msg);
    ~SyntaxErrorObject() { delete msg; }
    static void destroy(Managed *that) { static_cast<SyntaxErrorObject *>(that)->~SyntaxErrorObject(); }

    DiagnosticMessage *message() { return msg; }

private:
    DiagnosticMessage *msg;
protected:
    static const ManagedVTable static_vtbl;
};

struct TypeErrorObject: ErrorObject {
    TypeErrorObject(ExecutionEngine *engine, const Value &message);
    TypeErrorObject(ExecutionEngine *engine, const QString &msg);
};

struct URIErrorObject: ErrorObject {
    URIErrorObject(ExecutionEngine *engine, const Value &message);
};

struct ErrorCtor: FunctionObject
{
    ErrorCtor(ExecutionContext *scope);
    ErrorCtor(ExecutionContext *scope, String *name);

    static Value construct(Managed *, Value *args, int argc);
    static Value call(Managed *that, ExecutionContext *, const Value &, Value *, int);

protected:
    static const ManagedVTable static_vtbl;
};

struct EvalErrorCtor: ErrorCtor
{
    EvalErrorCtor(ExecutionContext *scope);

    static Value construct(Managed *m, Value *args, int argc);

protected:
    static const ManagedVTable static_vtbl;
};

struct RangeErrorCtor: ErrorCtor
{
    RangeErrorCtor(ExecutionContext *scope);

    static Value construct(Managed *m, Value *args, int argc);

protected:
    static const ManagedVTable static_vtbl;
};

struct ReferenceErrorCtor: ErrorCtor
{
    ReferenceErrorCtor(ExecutionContext *scope);

    static Value construct(Managed *m, Value *args, int argc);

protected:
    static const ManagedVTable static_vtbl;
};

struct SyntaxErrorCtor: ErrorCtor
{
    SyntaxErrorCtor(ExecutionContext *scope);

    static Value construct(Managed *m, Value *args, int argc);

protected:
    static const ManagedVTable static_vtbl;
};

struct TypeErrorCtor: ErrorCtor
{
    TypeErrorCtor(ExecutionContext *scope);

    static Value construct(Managed *m, Value *args, int argc);

protected:
    static const ManagedVTable static_vtbl;
};

struct URIErrorCtor: ErrorCtor
{
    URIErrorCtor(ExecutionContext *scope);

    static Value construct(Managed *m, Value *args, int argc);

protected:
    static const ManagedVTable static_vtbl;
};


struct ErrorPrototype: ErrorObject
{
    // ### shouldn't be undefined
    ErrorPrototype(ExecutionEngine *engine): ErrorObject(engine, Value::undefinedValue()) {}
    void init(ExecutionEngine *engine, const Value &ctor) { init(engine, ctor, this); }

    static void init(ExecutionEngine *engine, const Value &ctor, Object *obj);
    static Value method_toString(SimpleCallContext *ctx);
};

struct EvalErrorPrototype: EvalErrorObject
{
    EvalErrorPrototype(ExecutionEngine *engine): EvalErrorObject(engine, Value::undefinedValue()) { vtbl = &static_vtbl; }
    void init(ExecutionEngine *engine, const Value &ctor) { ErrorPrototype::init(engine, ctor, this); }
};

struct RangeErrorPrototype: RangeErrorObject
{
    RangeErrorPrototype(ExecutionEngine *engine): RangeErrorObject(engine, Value::undefinedValue()) { vtbl = &static_vtbl; }
    void init(ExecutionEngine *engine, const Value &ctor) { ErrorPrototype::init(engine, ctor, this); }
};

struct ReferenceErrorPrototype: ReferenceErrorObject
{
    ReferenceErrorPrototype(ExecutionEngine *engine): ReferenceErrorObject(engine, Value::undefinedValue()) { vtbl = &static_vtbl; }
    void init(ExecutionEngine *engine, const Value &ctor) { ErrorPrototype::init(engine, ctor, this); }
};

struct SyntaxErrorPrototype: SyntaxErrorObject
{
    SyntaxErrorPrototype(ExecutionEngine *engine): SyntaxErrorObject(engine, 0) { vtbl = &static_vtbl; }
    void init(ExecutionEngine *engine, const Value &ctor) { ErrorPrototype::init(engine, ctor, this); }
};

struct TypeErrorPrototype: TypeErrorObject
{
    TypeErrorPrototype(ExecutionEngine *engine): TypeErrorObject(engine, Value::undefinedValue()) { vtbl = &static_vtbl; }
    void init(ExecutionEngine *engine, const Value &ctor) { ErrorPrototype::init(engine, ctor, this); }
};

struct URIErrorPrototype: URIErrorObject
{
    URIErrorPrototype(ExecutionEngine *engine): URIErrorObject(engine, Value::undefinedValue()) { vtbl = &static_vtbl; }
    void init(ExecutionEngine *engine, const Value &ctor) { ErrorPrototype::init(engine, ctor, this); }
};


inline SyntaxErrorObject *ErrorObject::asSyntaxError()
{
    return subtype == SyntaxError ? static_cast<SyntaxErrorObject *>(this) : 0;
}

}

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
