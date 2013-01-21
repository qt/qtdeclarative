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
#ifndef QMLJS_ENVIRONMENT_H
#define QMLJS_ENVIRONMENT_H

#include <qmljs_runtime.h>

namespace QQmlJS {
namespace VM {

struct Value;
struct Object;
struct ExecutionEngine;
struct ExecutionContext;
struct DeclarativeEnvironment;

struct DiagnosticMessage
{
    enum { Error, Warning };

    QString fileName;
    quint32 offset;
    quint32 length;
    quint32 startLine;
    unsigned startColumn: 31;
    unsigned type: 1;
    QString message;
    DiagnosticMessage *next;

    DiagnosticMessage();
    ~DiagnosticMessage();
    String *buildFullMessage(ExecutionContext *ctx) const;
};

struct ExecutionContext
{
    ExecutionEngine *engine;
    ExecutionContext *parent;
    Value thisObject;

    FunctionObject *function;

    Value *arguments;
    unsigned int argumentCount;
    Value *locals;

    String **formals() const;
    unsigned int formalCount() const;
    String **variables() const;
    unsigned int variableCount() const;
    ExecutionContext *outer() const;

    bool strictMode;

    Object *activation;
    struct With {
        Object *object;
        With *next;
    } *withObject;

    void init(ExecutionEngine *e);
    void destroy();

    bool hasBinding(String *name) const;
    void createMutableBinding(String *name, bool deletable);
    bool setMutableBinding(ExecutionContext *scope, String *name, Value value);
    Value getBindingValue(ExecutionContext *scope, String *name, bool strict) const;
    bool deleteBinding(ExecutionContext *ctx, String *name);

    void pushWithObject(Object *with);
    void popWithObject();

    void initCallContext(ExecutionContext *parent, const Value that, FunctionObject *f, Value *args, unsigned argc);
    void leaveCallContext();

    void wireUpPrototype();

    void throwError(Value value);
    void throwError(const QString &message);
    void throwSyntaxError(DiagnosticMessage *message);
    void throwTypeError();
    void throwReferenceError(Value value);
    void throwRangeError(Value value);
    void throwUnimplemented(const QString &message);

    void setProperty(String *name, Value value);
    Value getProperty(String *name);
    Value getPropertyNoThrow(String *name);
    Value getPropertyAndBase(String *name, Object **base);
    void inplaceBitOp(Value value, String *name, BinOp op);
    bool deleteProperty(String *name);

    inline Value argument(unsigned int index = 0)
    {
        if (index < argumentCount)
            return arguments[index];
        return Value::undefinedValue();
    }
};



} // namespace VM
} // namespace QQmlJS

#endif
