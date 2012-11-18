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
#include <qmljs_environment.h>
#include <qmljs_objects.h>
#include <qv4ecmaobjects_p.h>

namespace QQmlJS {
namespace VM {

DeclarativeEnvironment::DeclarativeEnvironment(ExecutionEngine *e)
{
    engine = e;
    outer = 0;
    arguments = 0;
    argumentCount = 0;
    locals = 0;
    activation = 0;
    formals = 0;
    formalCount = 0;
    vars = 0;
    varCount = 0;
}

DeclarativeEnvironment::DeclarativeEnvironment(FunctionObject *f, Value *args, uint argc)
{
    outer = f->scope;
    engine = outer->engine;

    if (f->needsActivation)
        activation = engine->newActivationObject(this);
    else
        activation = 0;

    formals = f->formalParameterList;
    formalCount = f->formalParameterCount;
    arguments = args;
    argumentCount = argc;
    if (f->needsActivation || argc < formalCount){
        arguments = new Value[qMax(argc, formalCount)];
        if (argc)
            std::copy(args, args + argc, arguments);
        if (argc < formalCount)
            std::fill(arguments + argc, arguments + formalCount, Value::undefinedValue());
    }
    vars = f->varList;
    varCount = f->varCount;
    locals = varCount ? new Value[varCount] : 0;
    if (varCount)
        std::fill(locals, locals + varCount, Value::undefinedValue());
}

bool DeclarativeEnvironment::hasBinding(String *name) const
{
    for (unsigned int i = 0; i < varCount; ++i) {
        if (__qmljs_string_equal(vars[i], name))
            return true;
    }
    for (unsigned int i = 0; i < formalCount; ++i) {
        if (__qmljs_string_equal(formals[i], name))
            return true;
    }
    return deletableLocals.contains(name->toQString());
}

void DeclarativeEnvironment::createMutableBinding(String *name, bool deletable)
{
    // all non deletable vars should get created at compile time
    assert(deletable);
    assert(!hasBinding(name));

    deletableLocals.insert(name->toQString(), Value::undefinedValue());
}

void DeclarativeEnvironment::setMutableBinding(String *name, Value value, bool strict)
{
    // ### throw if strict is true, and it would change an immutable binding
    for (unsigned int i = 0; i < varCount; ++i) {
        if (__qmljs_string_equal(vars[i], name)) {
            locals[i] = value;
            return;
        }
    }
    for (unsigned int i = 0; i < formalCount; ++i) {
        if (__qmljs_string_equal(formals[i], name)) {
            arguments[i] = value;
            return;
        }
    }
    QHash<QString, Value>::iterator it = deletableLocals.find(name->toQString());
    if (it != deletableLocals.end()) {
        *it = value;
        return;
    }
    assert(false);
}

Value DeclarativeEnvironment::getBindingValue(String *name, bool strict) const
{
    for (unsigned int i = 0; i < varCount; ++i) {
        if (__qmljs_string_equal(vars[i], name))
            return locals[i];
    }
    for (unsigned int i = 0; i < formalCount; ++i) {
        if (__qmljs_string_equal(formals[i], name))
            return arguments[i];
    }
    QHash<QString, Value>::const_iterator it = deletableLocals.find(name->toQString());
    if (it != deletableLocals.end())
        return *it;

    assert(false);
}

bool DeclarativeEnvironment::deleteBinding(String *name)
{
    QHash<QString, Value>::iterator it = deletableLocals.find(name->toQString());
    if (it != deletableLocals.end()) {
        deletableLocals.erase(it);
        return true;
    }
    return !hasBinding(name);
}


void ExecutionContext::init(ExecutionEngine *eng)
{
    engine = eng;
    parent = 0;
    variableEnvironment = new DeclarativeEnvironment(eng);
    lexicalEnvironment = variableEnvironment;
    thisObject = Value::nullValue();
    result = Value::undefinedValue();
}

PropertyDescriptor *ExecutionContext::lookupPropertyDescriptor(String *name, PropertyDescriptor *tmp)
{
    for (DeclarativeEnvironment *ctx = lexicalEnvironment; ctx; ctx = ctx->outer) {
        if (ctx->activation) {
            if (PropertyDescriptor *pd = ctx->activation->__getPropertyDescriptor__(this, name, tmp))
                return pd;
        }
    }
    return 0;
}

void ExecutionContext::inplaceBitOp(Value value, String *name, BinOp op)
{
    for (DeclarativeEnvironment *ctx = lexicalEnvironment; ctx; ctx = ctx->outer) {
        if (ctx->activation) {
            if (ctx->activation->inplaceBinOp(value, name, op, this))
                return;
        }
    }
    throwReferenceError(Value::fromString(name));
}

void ExecutionContext::throwError(Value value)
{
    result = value;
    __qmljs_builtin_throw(value, this);
}

void ExecutionContext::throwError(const QString &message)
{
    Value v = Value::fromString(this, message);
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void ExecutionContext::throwTypeError()
{
    Value v = Value::fromString(this, QStringLiteral("Type error"));
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void ExecutionContext::throwUnimplemented(const QString &message)
{
    Value v = Value::fromString(this, QStringLiteral("Unimplemented ") + message);
    throwError(Value::fromObject(engine->newErrorObject(v)));
}

void ExecutionContext::throwReferenceError(Value value)
{
    String *s = value.toString(this);
    QString msg = s->toQString() + QStringLiteral(" is not defined");
    throwError(Value::fromObject(engine->newErrorObject(Value::fromString(this, msg))));
}

void ExecutionContext::initCallContext(ExecutionContext *parent, const Value that, FunctionObject *f, Value *args, unsigned argc)
{
    engine = parent->engine;
    this->parent = parent;

    variableEnvironment = new DeclarativeEnvironment(f, args, argc);
    lexicalEnvironment = variableEnvironment;

    thisObject = that;
    result = Value::undefinedValue();
}

void ExecutionContext::leaveCallContext()
{
    // ## Should rather be handled by a the activation object having a ref to the environment
    if (variableEnvironment->activation) {
        delete[] variableEnvironment->locals;
        variableEnvironment->locals = 0;
    }
}

void ExecutionContext::initConstructorContext(ExecutionContext *parent, Value that, FunctionObject *f, Value *args, unsigned argc)
{
    initCallContext(parent, that, f, args, argc);
}

void ExecutionContext::leaveConstructorContext(FunctionObject *f)
{
    wireUpPrototype(f);
    leaveCallContext();
}

void ExecutionContext::wireUpPrototype(FunctionObject *f)
{
    assert(thisObject.isObject());
    result = thisObject;

    Value proto = f->__get__(this, engine->id_prototype);
    if (proto.isObject())
        thisObject.objectValue()->prototype = proto.objectValue();
    else
        thisObject.objectValue()->prototype = engine->objectPrototype;
}


} // namespace VM
} // namespace QQmlJS
