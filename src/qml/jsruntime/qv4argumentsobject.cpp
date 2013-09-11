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

static Value throwTypeError(SimpleCallContext *ctx)
{
    ctx->throwTypeError();
    return Value::undefinedValue();
}

DEFINE_MANAGED_VTABLE(ArgumentsObject);

ArgumentsObject::ArgumentsObject(CallContext *context)
    : Object(context->engine), context(context)
{
    vtbl = &static_vtbl;
    type = Type_ArgumentsObject;

    if (context->strictMode) {
        internalClass = engine()->strictArgumentsObjectClass;

        FunctionObject *thrower = context->engine->newBuiltinFunction(context, 0, throwTypeError);
        Property pd = Property::fromAccessor(thrower, thrower);
        assert(CalleePropertyIndex == internalClass->find(context->engine->id_callee));
        assert(CallerPropertyIndex == internalClass->find(context->engine->id_caller));
        memberData[CalleePropertyIndex] = pd;
        memberData[CallerPropertyIndex] = pd;

        arrayReserve(context->argumentCount);
        for (unsigned int i = 0; i < context->argumentCount; ++i)
            arrayData[i].value = context->arguments[i];
        arrayDataLen = context->argumentCount;
    } else {
        internalClass = engine()->argumentsObjectClass;
        Q_ASSERT(CalleePropertyIndex == internalClass->find(context->engine->id_callee));
        memberData[CalleePropertyIndex].value = Value::fromObject(context->function);
        isNonStrictArgumentsObject = true;

        uint numAccessors = qMin(context->function->formalParameterCount, context->realArgumentCount);
        uint argCount = qMin((uint)context->realArgumentCount, context->argumentCount);
        arrayReserve(argCount);
        ensureArrayAttributes();
        context->engine->requireArgumentsAccessors(numAccessors);
        for (uint i = 0; i < (uint)numAccessors; ++i) {
            mappedArguments.append(context->argument(i));
            arrayData[i] = context->engine->argumentsAccessors.at(i);
            arrayAttributes[i] = Attr_Accessor;
        }
        for (uint i = numAccessors; i < argCount; ++i) {
            arrayData[i] = Property::fromValue(context->argument(i));
            arrayAttributes[i] = Attr_Data;
        }
        arrayDataLen = argCount;
    }
    Q_ASSERT(LengthPropertyIndex == internalClass->find(context->engine->id_length));
    Property *lp = memberData + ArrayObject::LengthPropertyIndex;
    lp->value = Value::fromInt32(context->realArgumentCount);
}

void ArgumentsObject::destroy(Managed *that)
{
    static_cast<ArgumentsObject *>(that)->~ArgumentsObject();
}

bool ArgumentsObject::defineOwnProperty(ExecutionContext *ctx, uint index, const Property &desc, PropertyAttributes attrs)
{
    Scope scope(ctx);
    uint pidx = propertyIndexFromArrayIndex(index);
    Property *pd = arrayData + pidx;
    Property map;
    PropertyAttributes mapAttrs;
    bool isMapped = false;
    if (pd && index < (uint)mappedArguments.size())
        isMapped = arrayAttributes && arrayAttributes[pidx].isAccessor() && pd->getter() == context->engine->argumentsAccessors.at(index).getter();

    if (isMapped) {
        map = *pd;
        mapAttrs = arrayAttributes[pidx];
        arrayAttributes[pidx] = Attr_Data;
        pd->value = mappedArguments.at(index);
    }

    isNonStrictArgumentsObject = false;
    bool strict = ctx->strictMode;
    ctx->strictMode = false;
    bool result = Object::__defineOwnProperty__(ctx, index, desc, attrs);
    ctx->strictMode = strict;
    isNonStrictArgumentsObject = true;

    if (isMapped && attrs.isData()) {
        if (!attrs.isGeneric()) {
            ScopedCallData callData(scope, 1);
            callData->thisObject = Value::fromObject(this);
            callData->args[0] = desc.value;
            map.setter()->call(callData);
        }
        if (attrs.isWritable()) {
            *pd = map;
            arrayAttributes[pidx] = mapAttrs;
        }
    }

    if (ctx->strictMode && !result)
        ctx->throwTypeError();
    return result;
}

DEFINE_MANAGED_VTABLE(ArgumentsGetterFunction);

ReturnedValue ArgumentsGetterFunction::call(Managed *getter, CallData *callData)
{
    ArgumentsGetterFunction *g = static_cast<ArgumentsGetterFunction *>(getter);
    Object *that = callData->thisObject.asObject();
    if (!that)
        getter->engine()->current->throwTypeError();
    ArgumentsObject *o = that->asArgumentsObject();
    if (!o)
        getter->engine()->current->throwTypeError();

    assert(g->index < o->context->argumentCount);
    return o->context->argument(g->index).asReturnedValue();
}

DEFINE_MANAGED_VTABLE(ArgumentsSetterFunction);

ReturnedValue ArgumentsSetterFunction::call(Managed *setter, CallData *callData)
{
    ArgumentsSetterFunction *s = static_cast<ArgumentsSetterFunction *>(setter);
    Object *that = callData->thisObject.asObject();
    if (!that)
        setter->engine()->current->throwTypeError();
    ArgumentsObject *o = that->asArgumentsObject();
    if (!o)
        setter->engine()->current->throwTypeError();

    assert(s->index < o->context->argumentCount);
    o->context->arguments[s->index] = callData->argc ? callData->args[0] : Value::undefinedValue();
    return Value::undefinedValue().asReturnedValue();
}

void ArgumentsObject::markObjects(Managed *that)
{
    ArgumentsObject *o = static_cast<ArgumentsObject *>(that);
    o->context->mark();
    for (int i = 0; i < o->mappedArguments.size(); ++i) {
        Managed *m = o->mappedArguments.at(i).asManaged();
        if (m)
            m->mark();
    }
    Object::markObjects(that);
}
