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

DEFINE_MANAGED_VTABLE(ArgumentsObject);

ArgumentsObject::ArgumentsObject(CallContext *context)
    : Object(context->engine), context(context)
{
    vtbl = &static_vtbl;
    type = Type_ArgumentsObject;

    ExecutionEngine *v4 = context->engine;
    Scope scope(v4);
    ScopedObject protectThis(scope, this);

    if (context->strictMode) {
        internalClass = v4->strictArgumentsObjectClass;

        Property pd = Property::fromAccessor(v4->thrower, v4->thrower);
        assert(CalleePropertyIndex == internalClass->find(context->engine->id_callee));
        assert(CallerPropertyIndex == internalClass->find(context->engine->id_caller));
        memberData[CalleePropertyIndex] = pd;
        memberData[CallerPropertyIndex] = pd;

        arrayReserve(context->callData->argc);
        for (int i = 0; i < context->callData->argc; ++i)
            arrayData[i].value = context->callData->args[i];
        arrayDataLen = context->callData->argc;
    } else {
        internalClass = engine()->argumentsObjectClass;
        Q_ASSERT(CalleePropertyIndex == internalClass->find(context->engine->id_callee));
        memberData[CalleePropertyIndex].value = context->function->asReturnedValue();
        isNonStrictArgumentsObject = true;

        uint numAccessors = qMin((int)context->function->formalParameterCount, context->realArgumentCount);
        uint argCount = qMin(context->realArgumentCount, context->callData->argc);
        arrayReserve(argCount);
        ensureArrayAttributes();
        context->engine->requireArgumentsAccessors(numAccessors);
        for (uint i = 0; i < (uint)numAccessors; ++i) {
            mappedArguments.append(context->callData->args[i]);
            arrayData[i] = context->engine->argumentsAccessors.at(i);
            arrayAttributes[i] = Attr_Accessor;
        }
        for (uint i = numAccessors; i < argCount; ++i) {
            arrayData[i] = Property::fromValue(context->callData->args[i]);
            arrayAttributes[i] = Attr_Data;
        }
        arrayDataLen = argCount;
    }
    Q_ASSERT(LengthPropertyIndex == internalClass->find(context->engine->id_length));
    Property *lp = memberData + ArrayObject::LengthPropertyIndex;
    lp->value = Primitive::fromInt32(context->realArgumentCount);
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
        ScopedCallData callData(scope, 1);
        callData->thisObject = this->asReturnedValue();
        callData->args[0] = desc.value;
        map.setter()->call(callData);

        if (attrs.isWritable()) {
            *pd = map;
            arrayAttributes[pidx] = mapAttrs;
        }
    }

    if (ctx->strictMode && !result)
        return ctx->throwTypeError();
    return result;
}

DEFINE_MANAGED_VTABLE(ArgumentsGetterFunction);

ReturnedValue ArgumentsGetterFunction::call(Managed *getter, CallData *callData)
{
    ExecutionEngine *v4 = getter->engine();
    Scope scope(v4);
    Scoped<ArgumentsGetterFunction> g(scope, static_cast<ArgumentsGetterFunction *>(getter));
    Scoped<ArgumentsObject> o(scope, callData->thisObject.as<ArgumentsObject>());
    if (!o)
        return v4->current->throwTypeError();

    Q_ASSERT(g->index < static_cast<unsigned>(o->context->callData->argc));
    return o->context->argument(g->index);
}

DEFINE_MANAGED_VTABLE(ArgumentsSetterFunction);

ReturnedValue ArgumentsSetterFunction::call(Managed *setter, CallData *callData)
{
    ExecutionEngine *v4 = setter->engine();
    Scope scope(v4);
    Scoped<ArgumentsSetterFunction> s(scope, static_cast<ArgumentsSetterFunction *>(setter));
    Scoped<ArgumentsObject> o(scope, callData->thisObject.as<ArgumentsObject>());
    if (!o)
        return v4->current->throwTypeError();

    Q_ASSERT(s->index < static_cast<unsigned>(o->context->callData->argc));
    o->context->callData->args[s->index] = callData->argc ? callData->args[0].asReturnedValue() : Encode::undefined();
    return Encode::undefined();
}

void ArgumentsObject::markObjects(Managed *that, ExecutionEngine *e)
{
    ArgumentsObject *o = static_cast<ArgumentsObject *>(that);
    o->context->mark();
    for (int i = 0; i < o->mappedArguments.size(); ++i)
        o->mappedArguments.at(i).mark(e);

    Object::markObjects(that, e);
}
