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

#include "qv4object.h"
#include "qv4functionobject.h"

QT_BEGIN_NAMESPACE

namespace QQmlJS {
namespace VM {

struct ErrorObject: Object {
    enum ErrorType {
        Error,
        EvalError,
        RangeError,
        ReferenceError,
        SyntaxError,
        TypeError,
        URIError
    };

    ErrorObject(ExecutionEngine* engine, const Value &message);

    virtual struct SyntaxErrorObject *asSyntaxError() { return 0; }

protected:
    void setNameProperty(ExecutionContext *ctx);
};

struct EvalErrorObject: ErrorObject {
    EvalErrorObject(ExecutionContext *ctx, const Value &message);
};

struct RangeErrorObject: ErrorObject {
    RangeErrorObject(ExecutionContext *ctx, const Value &message);
    RangeErrorObject(ExecutionContext *ctx, const QString &msg);
};

struct ReferenceErrorObject: ErrorObject {
    ReferenceErrorObject(ExecutionContext *ctx, const Value &message);
    ReferenceErrorObject(ExecutionContext *ctx, const QString &msg);
};

struct SyntaxErrorObject: ErrorObject {
    SyntaxErrorObject(ExecutionContext *ctx, DiagnosticMessage *msg);
    ~SyntaxErrorObject() { delete msg; }

    virtual SyntaxErrorObject *asSyntaxError() { return this; }
    DiagnosticMessage *message() { return msg; }

private:
    DiagnosticMessage *msg;
};

struct TypeErrorObject: ErrorObject {
    TypeErrorObject(ExecutionContext *ctx, const Value &message);
    TypeErrorObject(ExecutionContext *ctx, const QString &msg);
};

struct URIErrorObject: ErrorObject {
    URIErrorObject(ExecutionContext *ctx, const Value &message);
};

struct ErrorCtor: FunctionObject
{
    ErrorCtor(ExecutionContext *scope);

    virtual Value construct(ExecutionContext *context, Value *args, int argc);
    virtual Value call(ExecutionContext *ctx, Value thisObject, Value *args, int argc);
};

struct EvalErrorCtor: ErrorCtor
{
    EvalErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx, Value *, int argc);
};

struct RangeErrorCtor: ErrorCtor
{
    RangeErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx, Value *args, int argc);
};

struct ReferenceErrorCtor: ErrorCtor
{
    ReferenceErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx, Value *args, int argc);
};

struct SyntaxErrorCtor: ErrorCtor
{
    SyntaxErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx, Value *, int);
};

struct TypeErrorCtor: ErrorCtor
{
    TypeErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx, Value *args, int argc);
};

struct URIErrorCtor: ErrorCtor
{
    URIErrorCtor(ExecutionContext *scope): ErrorCtor(scope) {}

    virtual Value construct(ExecutionContext *ctx, Value *args, int argc);
};


struct ErrorPrototype: ErrorObject
{
    // ### shouldn't be undefined
    ErrorPrototype(ExecutionEngine* engine): ErrorObject(engine, Value::undefinedValue()) {}
    void init(ExecutionContext *ctx, const Value &ctor) { init(ctx, ctor, this); }

    static void init(ExecutionContext *ctx, const Value &ctor, Object *obj);
    static Value method_toString(ExecutionContext *ctx);
};

struct EvalErrorPrototype: EvalErrorObject
{
    EvalErrorPrototype(ExecutionContext *ctx): EvalErrorObject(ctx, Value::undefinedValue()) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};

struct RangeErrorPrototype: RangeErrorObject
{
    RangeErrorPrototype(ExecutionContext *ctx): RangeErrorObject(ctx, Value::undefinedValue()) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};

struct ReferenceErrorPrototype: ReferenceErrorObject
{
    ReferenceErrorPrototype(ExecutionContext *ctx): ReferenceErrorObject(ctx, Value::undefinedValue()) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};

struct SyntaxErrorPrototype: SyntaxErrorObject
{
    SyntaxErrorPrototype(ExecutionContext *ctx): SyntaxErrorObject(ctx, 0) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};

struct TypeErrorPrototype: TypeErrorObject
{
    TypeErrorPrototype(ExecutionContext *ctx): TypeErrorObject(ctx, Value::undefinedValue()) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};

struct URIErrorPrototype: URIErrorObject
{
    URIErrorPrototype(ExecutionContext *ctx): URIErrorObject(ctx, Value::undefinedValue()) {}
    void init(ExecutionContext *ctx, const Value &ctor) { ErrorPrototype::init(ctx, ctor, this); }
};


} // end of namespace VM
} // end of namespace QQmlJS

QT_END_NAMESPACE

#endif // QV4ECMAOBJECTS_P_H
