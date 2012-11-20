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
#include <qmljs_engine.h>
#include <qmljs_objects.h>
#include <qv4ecmaobjects_p.h>

namespace QQmlJS {
namespace VM {

ExecutionEngine::ExecutionEngine(EValISelFactory *factory)
{
    rootContext = newContext();
    rootContext->init(this);

    id_length = identifier(QStringLiteral("length"));
    id_prototype = identifier(QStringLiteral("prototype"));
    id_constructor = identifier(QStringLiteral("constructor"));
    id_arguments = identifier(QStringLiteral("arguments"));
    id___proto__ = identifier(QStringLiteral("__proto__"));

    objectPrototype = new ObjectPrototype();
    stringPrototype = new StringPrototype(rootContext);
    numberPrototype = new NumberPrototype();
    booleanPrototype = new BooleanPrototype();
    arrayPrototype = new ArrayPrototype();
    datePrototype = new DatePrototype();
    functionPrototype = new FunctionPrototype(rootContext);
    regExpPrototype = new RegExpPrototype();
    errorPrototype = new ErrorPrototype();
    evalErrorPrototype = new EvalErrorPrototype(rootContext);
    rangeErrorPrototype = new RangeErrorPrototype(rootContext);
    referenceErrorPrototype = new ReferenceErrorPrototype(rootContext);
    syntaxErrorPrototype = new SyntaxErrorPrototype(rootContext);
    typeErrorPrototype = new TypeErrorPrototype(rootContext);
    uRIErrorPrototype = new URIErrorPrototype(rootContext);

    stringPrototype->prototype = objectPrototype;
    numberPrototype->prototype = objectPrototype;
    booleanPrototype->prototype = objectPrototype;
    arrayPrototype->prototype = objectPrototype;
    datePrototype->prototype = objectPrototype;
    functionPrototype->prototype = objectPrototype;
    regExpPrototype->prototype = objectPrototype;
    errorPrototype->prototype = objectPrototype;
    evalErrorPrototype->prototype = errorPrototype;
    rangeErrorPrototype->prototype = errorPrototype;
    referenceErrorPrototype->prototype = errorPrototype;
    syntaxErrorPrototype->prototype = errorPrototype;
    typeErrorPrototype->prototype = errorPrototype;
    uRIErrorPrototype->prototype = errorPrototype;

    objectCtor = Value::fromObject(new ObjectCtor(rootContext));
    stringCtor = Value::fromObject(new StringCtor(rootContext));
    numberCtor = Value::fromObject(new NumberCtor(rootContext));
    booleanCtor = Value::fromObject(new BooleanCtor(rootContext));
    arrayCtor = Value::fromObject(new ArrayCtor(rootContext));
    functionCtor = Value::fromObject(new FunctionCtor(rootContext, factory));
    dateCtor = Value::fromObject(new DateCtor(rootContext));
    regExpCtor = Value::fromObject(new RegExpCtor(rootContext));
    errorCtor = Value::fromObject(new ErrorCtor(rootContext));
    evalErrorCtor = Value::fromObject(new EvalErrorCtor(rootContext));
    rangeErrorCtor = Value::fromObject(new RangeErrorCtor(rootContext));
    referenceErrorCtor = Value::fromObject(new ReferenceErrorCtor(rootContext));
    syntaxErrorCtor = Value::fromObject(new SyntaxErrorCtor(rootContext));
    typeErrorCtor = Value::fromObject(new TypeErrorCtor(rootContext));
    uRIErrorCtor = Value::fromObject(new URIErrorCtor(rootContext));

    stringCtor.objectValue()->prototype = functionPrototype;
    numberCtor.objectValue()->prototype = functionPrototype;
    booleanCtor.objectValue()->prototype = functionPrototype;
    arrayCtor.objectValue()->prototype = functionPrototype;
    functionCtor.objectValue()->prototype = functionPrototype;
    dateCtor.objectValue()->prototype = functionPrototype;
    regExpCtor.objectValue()->prototype = functionPrototype;
    errorCtor.objectValue()->prototype = functionPrototype;
    evalErrorCtor.objectValue()->prototype = errorPrototype;
    rangeErrorCtor.objectValue()->prototype = errorPrototype;
    referenceErrorCtor.objectValue()->prototype = errorPrototype;
    syntaxErrorCtor.objectValue()->prototype = errorPrototype;
    typeErrorCtor.objectValue()->prototype = errorPrototype;
    uRIErrorCtor.objectValue()->prototype = errorPrototype;

    objectPrototype->init(rootContext, objectCtor);
    stringPrototype->init(rootContext, stringCtor);
    numberPrototype->init(rootContext, numberCtor);
    booleanPrototype->init(rootContext, booleanCtor);
    arrayPrototype->init(rootContext, arrayCtor);
    datePrototype->init(rootContext, dateCtor);
    functionPrototype->init(rootContext, functionCtor);
    regExpPrototype->init(rootContext, regExpCtor);
    errorPrototype->init(rootContext, errorCtor);
    evalErrorPrototype->init(rootContext, evalErrorCtor);
    rangeErrorPrototype->init(rootContext, rangeErrorCtor);
    referenceErrorPrototype->init(rootContext, referenceErrorCtor);
    syntaxErrorPrototype->init(rootContext, syntaxErrorCtor);
    typeErrorPrototype->init(rootContext, typeErrorCtor);
    uRIErrorPrototype->init(rootContext, uRIErrorCtor);

    //
    // set up the global object
    //
    VM::Object *glo = newObject(/*rootContext*/);
    globalObject = Value::fromObject(glo);
    rootContext->variableEnvironment->activation = glo;

    glo->__put__(rootContext, identifier(QStringLiteral("Object")), objectCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("String")), stringCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Number")), numberCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Boolean")), booleanCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Array")), arrayCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Function")), functionCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Date")), dateCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("RegExp")), regExpCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Error")), errorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("EvalError")), evalErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("RangeError")), rangeErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("ReferenceError")), referenceErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("SyntaxError")), syntaxErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("TypeError")), typeErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("URIError")), uRIErrorCtor);
    glo->__put__(rootContext, identifier(QStringLiteral("Math")), Value::fromObject(newMathObject(rootContext)));
    glo->__put__(rootContext, identifier(QStringLiteral("undefined")), Value::undefinedValue());
    glo->__put__(rootContext, identifier(QStringLiteral("NaN")), Value::fromDouble(nan("")));
    glo->__put__(rootContext, identifier(QStringLiteral("Infinity")), Value::fromDouble(INFINITY));
    glo->__put__(rootContext, identifier(QStringLiteral("eval")), Value::fromObject(new EvalFunction(rootContext, factory)));

    // TODO: parseInt [15.1.2.2]
    // TODO: parseFloat [15.1.2.3]
    glo->__put__(rootContext, identifier(QStringLiteral("isNaN")), Value::fromObject(new IsNaNFunction(rootContext))); // isNaN [15.1.2.4]
    glo->__put__(rootContext, identifier(QStringLiteral("isFinite")), Value::fromObject(new IsFiniteFunction(rootContext))); // isFinite [15.1.2.5]
}

ExecutionContext *ExecutionEngine::newContext()
{
    return new ExecutionContext();
}

String *ExecutionEngine::identifier(const QString &s)
{
    String *&id = identifiers[s];
    if (! id)
        id = newString(s);
    return id;
}

FunctionObject *ExecutionEngine::newNativeFunction(ExecutionContext *scope, Value (*code)(ExecutionContext *))
{
    NativeFunction *f = new NativeFunction(scope, code);
    f->prototype = scope->engine->functionPrototype;
    return f;
}

FunctionObject *ExecutionEngine::newScriptFunction(ExecutionContext *scope, IR::Function *function)
{
    ScriptFunction *f = new ScriptFunction(scope, function);
    Object *proto = scope->engine->newObject();
    proto->__put__(scope, scope->engine->id_constructor, Value::fromObject(f));
    f->__put__(scope, scope->engine->id_prototype, Value::fromObject(proto));
    f->prototype = scope->engine->functionPrototype;
    return f;
}

Object *ExecutionEngine::newObject()
{
    Object *object = new Object();
    object->prototype = objectPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newObjectCtor(ExecutionContext *ctx)
{
    return new ObjectCtor(ctx);
}

String *ExecutionEngine::newString(const QString &s)
{
    return new String(s);
}

Object *ExecutionEngine::newStringObject(const Value &value)
{
    StringObject *object = new StringObject(value);
    object->prototype = stringPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newStringCtor(ExecutionContext *ctx)
{
    return new StringCtor(ctx);
}

Object *ExecutionEngine::newNumberObject(const Value &value)
{
    NumberObject *object = new NumberObject(value);
    object->prototype = numberPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newNumberCtor(ExecutionContext *ctx)
{
    return new NumberCtor(ctx);
}

Object *ExecutionEngine::newBooleanObject(const Value &value)
{
    Object *object = new BooleanObject(value);
    object->prototype = booleanPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newBooleanCtor(ExecutionContext *ctx)
{
    return new BooleanCtor(ctx);
}

Object *ExecutionEngine::newFunctionObject(ExecutionContext *ctx)
{
    Object *object = new FunctionObject(ctx);
    object->prototype = functionPrototype;
    return object;
}

Object *ExecutionEngine::newArrayObject()
{
    ArrayObject *object = new ArrayObject();
    object->prototype = arrayPrototype;
    return object;
}

Object *ExecutionEngine::newArrayObject(const Array &value)
{
    ArrayObject *object = new ArrayObject(value);
    object->prototype = arrayPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newArrayCtor(ExecutionContext *ctx)
{
    return new ArrayCtor(ctx);
}

Object *ExecutionEngine::newDateObject(const Value &value)
{
    Object *object = new DateObject(value);
    object->prototype = datePrototype;
    return object;
}

FunctionObject *ExecutionEngine::newDateCtor(ExecutionContext *ctx)
{
    return new DateCtor(ctx);
}

Object *ExecutionEngine::newRegExpObject(const QString &pattern, int flags)
{
    bool global = (flags & IR::RegExp::RegExp_Global);
    QRegularExpression::PatternOptions options = 0;
    if (flags & IR::RegExp::RegExp_IgnoreCase)
        options |= QRegularExpression::CaseInsensitiveOption;
    if (flags & IR::RegExp::RegExp_Multiline)
        options |= QRegularExpression::MultilineOption;

    Object *object = new RegExpObject(QRegularExpression(pattern, options), global);
    object->prototype = regExpPrototype;
    return object;
}

FunctionObject *ExecutionEngine::newRegExpCtor(ExecutionContext *ctx)
{
    return new RegExpCtor(ctx);
}

Object *ExecutionEngine::newErrorObject(const Value &value)
{
    ErrorObject *object = new ErrorObject(value);
    object->prototype = errorPrototype;
    return object;
}

Object *ExecutionEngine::newMathObject(ExecutionContext *ctx)
{
    MathObject *object = new MathObject(ctx);
    object->prototype = objectPrototype;
    return object;
}

Object *ExecutionEngine::newActivationObject(DeclarativeEnvironment *ctx)
{
    return new ActivationObject(ctx);
}

Object *ExecutionEngine::newForEachIteratorObject(Object *o)
{
    return new ForEachIteratorObject(o);
}


} // namespace VM
} // namespace QQmlJS
