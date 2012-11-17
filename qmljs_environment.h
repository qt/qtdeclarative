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

struct ExecutionContext {
    ExecutionEngine *engine;
    ExecutionContext *parent;
    Object *activation;
    Value thisObject;
    Value *arguments;
    unsigned int argumentCount;
    Value *locals;
    Value result;
    String **formals;
    unsigned int formalCount;
    String **vars;
    unsigned int varCount;

    PropertyDescriptor *lookupPropertyDescriptor(String *name, PropertyDescriptor *tmp);
    void inplaceBitOp(Value value, String *name, BinOp op);

    inline Value argument(unsigned int index = 0)
    {
        if (index < argumentCount)
            return arguments[index];
        return Value::undefinedValue();
    }

    void init(ExecutionEngine *eng);

    void initCallContext(ExecutionContext *parent, const Value that, FunctionObject *f, Value *args, unsigned argc);
    void leaveCallContext();

    void initConstructorContext(ExecutionContext *parent, Value that, FunctionObject *f, Value *args, unsigned argc);
    void leaveConstructorContext(FunctionObject *f);
    void wireUpPrototype(FunctionObject *f);

    void throwError(Value value);
    void throwError(const QString &message);
    void throwTypeError();
    void throwReferenceError(Value value);
    void throwUnimplemented(const QString &message);
};


} // namespace VM
} // namespace QQmlJS

#endif
