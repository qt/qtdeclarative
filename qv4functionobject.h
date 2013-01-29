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
#ifndef QV4FUNCTIONOBJECT_H
#define QV4FUNCTIONOBJECT_H

#include "qmljs_runtime.h"
#include "qmljs_engine.h"
#include "qmljs_environment.h"
#include "qv4object.h"
#include "qv4array.h"
#include "qv4string.h"
#include "qv4codegen_p.h"
#include "qv4isel_p.h"
#include "qv4managed.h"
#include "qv4propertydescriptor.h"
#include "qv4propertytable.h"
#include "qv4objectiterator.h"
#include "qv4regexp.h"

#include <QtCore/QString>
#include <QtCore/QHash>
#include <QtCore/QScopedPointer>
#include <cstdio>
#include <cassert>

namespace QQmlJS {

namespace VM {

struct Value;
struct Function;
struct Object;
struct ObjectIterator;
struct BooleanObject;
struct NumberObject;
struct StringObject;
struct ArrayObject;
struct DateObject;
struct FunctionObject;
struct RegExpObject;
struct ErrorObject;
struct ArgumentsObject;
struct ExecutionContext;
struct ExecutionEngine;
class MemoryManager;

struct ObjectPrototype;
struct StringPrototype;
struct NumberPrototype;
struct BooleanPrototype;
struct ArrayPrototype;
struct FunctionPrototype;
struct DatePrototype;
struct RegExpPrototype;
struct ErrorPrototype;
struct EvalErrorPrototype;
struct RangeErrorPrototype;
struct ReferenceErrorPrototype;
struct SyntaxErrorPrototype;
struct TypeErrorPrototype;
struct URIErrorPrototype;

struct Function {
    String *name;

    VM::Value (*code)(VM::ExecutionContext *, const uchar *);
    const uchar *codeData;
    JSC::MacroAssemblerCodeRef codeRef;

    QVector<String *> formals;
    QVector<String *> locals;
    QVector<Value> generatedValues;
    QVector<String *> identifiers;

    bool hasNestedFunctions  : 1;
    bool hasDirectEval       : 1;
    bool usesArgumentsObject : 1;
    bool isStrict            : 1;

    Function(String *name)
        : name(name)
        , code(0)
        , codeData(0)
        , hasNestedFunctions(0)
        , hasDirectEval(false)
        , usesArgumentsObject(false)
        , isStrict(false)
    {}
    ~Function();

    inline bool needsActivation() const { return hasNestedFunctions || hasDirectEval || usesArgumentsObject; }

    void mark();
};

struct FunctionObject: Object {
    ExecutionContext *scope;
    String *name;
    String * const *formalParameterList;
    String * const *varList;
    unsigned int formalParameterCount;
    unsigned int varCount;

    FunctionObject(ExecutionContext *scope);

    virtual QString className() { return QStringLiteral("Function"); }
    virtual bool hasInstance(ExecutionContext *ctx, const Value &value);

    virtual Value construct(ExecutionContext *context, Value *args, int argc);
    virtual Value call(ExecutionContext *context, Value thisObject, Value *args, int argc);

    virtual struct ScriptFunction *asScriptFunction() { return 0; }

    virtual void markObjects();

protected:
    virtual Value call(ExecutionContext *ctx);
    virtual Value construct(ExecutionContext *ctx);
};

struct FunctionCtor: FunctionObject
{
    FunctionCtor(ExecutionContext *scope);

    virtual Value construct(ExecutionContext *ctx);
    virtual Value call(ExecutionContext *ctx);
};

struct FunctionPrototype: FunctionObject
{
    FunctionPrototype(ExecutionContext *ctx): FunctionObject(ctx) {}
    void init(ExecutionContext *ctx, const Value &ctor);

    static Value method_toString(ExecutionContext *ctx);
    static Value method_apply(ExecutionContext *ctx);
    static Value method_call(ExecutionContext *ctx);
    static Value method_bind(ExecutionContext *ctx);
};

struct BuiltinFunction: FunctionObject {
    Value (*code)(ExecutionContext *);

    BuiltinFunction(ExecutionContext *scope, String *name, Value (*code)(ExecutionContext *));
    virtual Value call(ExecutionContext *ctx) { return code(ctx); }
    virtual Value construct(ExecutionContext *ctx);
};

struct ScriptFunction: FunctionObject {
    VM::Function *function;

    ScriptFunction(ExecutionContext *scope, VM::Function *function);
    virtual ~ScriptFunction();

    virtual Value call(ExecutionContext *ctx);

    virtual ScriptFunction *asScriptFunction() { return this; }

    virtual void markObjects();
};

struct BoundFunction: FunctionObject {
    FunctionObject *target;
    Value boundThis;
    QVector<Value> boundArgs;

    BoundFunction(ExecutionContext *scope, FunctionObject *target, Value boundThis, const QVector<Value> &boundArgs);
    virtual ~BoundFunction() {}

    virtual Value call(ExecutionContext *context, Value thisObject, Value *args, int argc);
    virtual Value construct(ExecutionContext *context, Value *args, int argc);
    virtual bool hasInstance(ExecutionContext *ctx, const Value &value);
    virtual void markObjects();
};

} // namespace VM
} // namespace QQmlJS

#endif // QMLJS_OBJECTS_H
