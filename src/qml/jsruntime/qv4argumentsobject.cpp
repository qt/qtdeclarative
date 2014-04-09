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
#include <qv4argumentsobject_p.h>
#include <qv4alloca_p.h>
#include <qv4scopedvalue_p.h>

using namespace QV4;

DEFINE_OBJECT_VTABLE(ArgumentsObject);

ArgumentsObject::ArgumentsObject(CallContext *context)
    : Object(context->strictMode ? context->engine->strictArgumentsObjectClass : context->engine->argumentsObjectClass)
    , context(context)
    , fullyCreated(false)
{
    ExecutionEngine *v4 = context->engine;
    Scope scope(v4);
    ScopedObject protectThis(scope, this);

    setArrayType(ArrayData::Complex);

    if (context->strictMode) {
        Q_ASSERT(CalleePropertyIndex == internalClass->find(context->engine->id_callee));
        Q_ASSERT(CallerPropertyIndex == internalClass->find(context->engine->id_caller));
        propertyAt(CalleePropertyIndex)->value = v4->thrower;
        propertyAt(CalleePropertyIndex)->set = v4->thrower;
        propertyAt(CallerPropertyIndex)->value = v4->thrower;
        propertyAt(CallerPropertyIndex)->set = v4->thrower;

        arrayReserve(context->callData->argc);
        arrayPut(0, context->callData->args, context->callData->argc);
        fullyCreated = true;
    } else {
        hasAccessorProperty = 1;
        Q_ASSERT(CalleePropertyIndex == internalClass->find(context->engine->id_callee));
        memberData[CalleePropertyIndex] = context->function->asReturnedValue();
    }
    Q_ASSERT(LengthPropertyIndex == internalClass->find(context->engine->id_length));
    memberData[LengthPropertyIndex] = Primitive::fromInt32(context->realArgumentCount);

    Q_ASSERT(internalClass->vtable == staticVTable());
}

void ArgumentsObject::destroy(Managed *that)
{
    static_cast<ArgumentsObject *>(that)->~ArgumentsObject();
}

void ArgumentsObject::fullyCreate()
{
    if (fullyCreated)
        return;

    uint numAccessors = qMin((int)context->function->formalParameterCount(), context->realArgumentCount);
    uint argCount = qMin(context->realArgumentCount, context->callData->argc);
    ArrayData::realloc(this, ArrayData::Sparse, 0, argCount, true);
    context->engine->requireArgumentsAccessors(numAccessors);
    mappedArguments.ensureIndex(engine(), numAccessors);
    for (uint i = 0; i < (uint)numAccessors; ++i) {
        mappedArguments[i] = context->callData->args[i];
        arraySet(i, context->engine->argumentsAccessors[i], Attr_Accessor);
    }
    arrayPut(numAccessors, context->callData->args + numAccessors, argCount - numAccessors);
    for (uint i = numAccessors; i < argCount; ++i)
        setArrayAttributes(i, Attr_Data);

    fullyCreated = true;
}

bool ArgumentsObject::defineOwnProperty(ExecutionContext *ctx, uint index, const Property &desc, PropertyAttributes attrs)
{
    fullyCreate();

    Scope scope(ctx);
    Property *pd = arrayData->getProperty(index);
    Property map;
    PropertyAttributes mapAttrs;
    bool isMapped = false;
    uint numAccessors = qMin((int)context->function->formalParameterCount(), context->realArgumentCount);
    if (pd && index < (uint)numAccessors)
        isMapped = arrayData->attributes(index).isAccessor() && pd->getter() == context->engine->argumentsAccessors[index].getter();

    if (isMapped) {
        mapAttrs = arrayData->attributes(index);
        map.copy(*pd, mapAttrs);
        setArrayAttributes(index, Attr_Data);
        pd = arrayData->getProperty(index);
        pd->value = mappedArguments[index];
    }

    bool strict = ctx->strictMode;
    ctx->strictMode = false;
    bool result = Object::defineOwnProperty2(ctx, index, desc, attrs);
    ctx->strictMode = strict;

    if (isMapped && attrs.isData()) {
        ScopedCallData callData(scope, 1);
        callData->thisObject = this->asReturnedValue();
        callData->args[0] = desc.value;
        map.setter()->call(callData);

        if (attrs.isWritable()) {
            setArrayAttributes(index, mapAttrs);
            pd = arrayData->getProperty(index);
            pd->copy(map, mapAttrs);
        }
    }

    if (ctx->strictMode && !result)
        return ctx->throwTypeError();
    return result;
}

ReturnedValue ArgumentsObject::getIndexed(Managed *m, uint index, bool *hasProperty)
{
    ArgumentsObject *args = static_cast<ArgumentsObject *>(m);
    if (args->fullyCreated)
        return Object::getIndexed(m, index, hasProperty);

    if (index < static_cast<uint>(args->context->callData->argc)) {
        if (hasProperty)
            *hasProperty = true;
        return args->context->callData->args[index].asReturnedValue();
    }
    if (hasProperty)
        *hasProperty = false;
    return Encode::undefined();
}

void ArgumentsObject::putIndexed(Managed *m, uint index, const ValueRef value)
{
    ArgumentsObject *args = static_cast<ArgumentsObject *>(m);
    if (!args->fullyCreated && index >= static_cast<uint>(args->context->callData->argc))
        args->fullyCreate();

    if (args->fullyCreated) {
        Object::putIndexed(m, index, value);
        return;
    }

    args->context->callData->args[index] = value;
}

bool ArgumentsObject::deleteIndexedProperty(Managed *m, uint index)
{
    ArgumentsObject *args = static_cast<ArgumentsObject *>(m);
    if (!args->fullyCreated)
        args->fullyCreate();
    return Object::deleteIndexedProperty(m, index);
}

PropertyAttributes ArgumentsObject::queryIndexed(const Managed *m, uint index)
{
    const ArgumentsObject *args = static_cast<const ArgumentsObject *>(m);
    if (args->fullyCreated)
        return Object::queryIndexed(m, index);

    uint numAccessors = qMin((int)args->context->function->formalParameterCount(), args->context->realArgumentCount);
    uint argCount = qMin(args->context->realArgumentCount, args->context->callData->argc);
    if (index >= argCount)
        return PropertyAttributes();
    if (index >= numAccessors)
        return Attr_Data;
    return Attr_Accessor;
}

DEFINE_OBJECT_VTABLE(ArgumentsGetterFunction);

ReturnedValue ArgumentsGetterFunction::call(Managed *getter, CallData *callData)
{
    ExecutionEngine *v4 = getter->engine();
    Scope scope(v4);
    Scoped<ArgumentsGetterFunction> g(scope, static_cast<ArgumentsGetterFunction *>(getter));
    Scoped<ArgumentsObject> o(scope, callData->thisObject.as<ArgumentsObject>());
    if (!o)
        return v4->currentContext()->throwTypeError();

    Q_ASSERT(g->index < static_cast<unsigned>(o->context->callData->argc));
    return o->context->argument(g->index);
}

DEFINE_OBJECT_VTABLE(ArgumentsSetterFunction);

ReturnedValue ArgumentsSetterFunction::call(Managed *setter, CallData *callData)
{
    ExecutionEngine *v4 = setter->engine();
    Scope scope(v4);
    Scoped<ArgumentsSetterFunction> s(scope, static_cast<ArgumentsSetterFunction *>(setter));
    Scoped<ArgumentsObject> o(scope, callData->thisObject.as<ArgumentsObject>());
    if (!o)
        return v4->currentContext()->throwTypeError();

    Q_ASSERT(s->index < static_cast<unsigned>(o->context->callData->argc));
    o->context->callData->args[s->index] = callData->argc ? callData->args[0].asReturnedValue() : Encode::undefined();
    return Encode::undefined();
}

void ArgumentsObject::markObjects(Managed *that, ExecutionEngine *e)
{
    ArgumentsObject *o = static_cast<ArgumentsObject *>(that);
    if (o->context)
        o->context->mark(e);
    o->mappedArguments.mark(e);

    Object::markObjects(that, e);
}
