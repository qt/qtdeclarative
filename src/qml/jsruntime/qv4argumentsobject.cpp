/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qv4argumentsobject_p.h>
#include <qv4alloca_p.h>
#include <qv4scopedvalue_p.h>

using namespace QV4;

DEFINE_OBJECT_VTABLE(ArgumentsObject);

Heap::ArgumentsObject::ArgumentsObject(QV4::CallContext *context)
    : Heap::Object(context->d()->strictMode ? context->d()->engine->strictArgumentsObjectClass : context->d()->engine->argumentsObjectClass,
                   context->d()->engine->objectPrototype.asObject())
    , context(context->d())
    , fullyCreated(false)
{
    Q_ASSERT(vtable == QV4::ArgumentsObject::staticVTable());

    ExecutionEngine *v4 = context->d()->engine;
    Scope scope(v4);
    Scoped<QV4::ArgumentsObject> args(scope, this);

    args->setArrayType(Heap::ArrayData::Complex);

    if (context->d()->strictMode) {
        Q_ASSERT(CalleePropertyIndex == args->internalClass()->find(context->d()->engine->id_callee));
        Q_ASSERT(CallerPropertyIndex == args->internalClass()->find(context->d()->engine->id_caller));
        args->propertyAt(CalleePropertyIndex)->value = v4->thrower;
        args->propertyAt(CalleePropertyIndex)->set = v4->thrower;
        args->propertyAt(CallerPropertyIndex)->value = v4->thrower;
        args->propertyAt(CallerPropertyIndex)->set = v4->thrower;

        args->arrayReserve(context->argc());
        args->arrayPut(0, context->args(), context->argc());
        args->d()->fullyCreated = true;
    } else {
        Q_ASSERT(CalleePropertyIndex == args->internalClass()->find(context->d()->engine->id_callee));
        args->memberData()->data[CalleePropertyIndex] = context->d()->function->asReturnedValue();
    }
    Q_ASSERT(LengthPropertyIndex == args->internalClass()->find(context->d()->engine->id_length));
    args->memberData()->data[LengthPropertyIndex] = Primitive::fromInt32(context->d()->callData->argc);
}

void ArgumentsObject::fullyCreate()
{
    if (fullyCreated())
        return;

    uint numAccessors = qMin((int)context()->function->formalParameterCount(), context()->callData->argc);
    uint argCount = context()->callData->argc;
    ArrayData::realloc(this, Heap::ArrayData::Sparse, argCount, true);
    context()->engine->requireArgumentsAccessors(numAccessors);

    Scope scope(engine());
    Scoped<MemberData> md(scope, d()->mappedArguments);
    if (!md || md->size() < numAccessors)
        d()->mappedArguments = md->reallocate(engine(), d()->mappedArguments, numAccessors);
    for (uint i = 0; i < (uint)numAccessors; ++i) {
        mappedArguments()->data[i] = context()->callData->args[i];
        arraySet(i, context()->engine->argumentsAccessors + i, Attr_Accessor);
    }
    arrayPut(numAccessors, context()->callData->args + numAccessors, argCount - numAccessors);
    for (uint i = numAccessors; i < argCount; ++i)
        setArrayAttributes(i, Attr_Data);

    d()->fullyCreated = true;
}

bool ArgumentsObject::defineOwnProperty(ExecutionEngine *engine, uint index, const Property *desc, PropertyAttributes attrs)
{
    fullyCreate();

    Scope scope(engine);
    Property *pd = arrayData() ? arrayData()->getProperty(index) : 0;
    ScopedProperty map(scope);
    PropertyAttributes mapAttrs;
    bool isMapped = false;
    uint numAccessors = qMin((int)context()->function->formalParameterCount(), context()->callData->argc);
    if (pd && index < (uint)numAccessors)
        isMapped = arrayData()->attributes(index).isAccessor() &&
                pd->getter() == context()->engine->argumentsAccessors[index].getter();

    if (isMapped) {
        Q_ASSERT(arrayData());
        mapAttrs = arrayData()->attributes(index);
        map->copy(pd, mapAttrs);
        setArrayAttributes(index, Attr_Data);
        pd = arrayData()->getProperty(index);
        pd->value = mappedArguments()->data[index];
    }

    bool strict = engine->currentContext()->strictMode;
    engine->currentContext()->strictMode = false;
    bool result = Object::defineOwnProperty2(scope.engine, index, desc, attrs);
    engine->currentContext()->strictMode = strict;

    if (isMapped && attrs.isData()) {
        Q_ASSERT(arrayData());
        ScopedFunctionObject setter(scope, map->setter());
        ScopedCallData callData(scope, 1);
        callData->thisObject = this->asReturnedValue();
        callData->args[0] = desc->value;
        setter->call(callData);

        if (attrs.isWritable()) {
            setArrayAttributes(index, mapAttrs);
            pd = arrayData()->getProperty(index);
            pd->copy(map, mapAttrs);
        }
    }

    if (engine->currentContext()->strictMode && !result)
        return engine->throwTypeError();
    return result;
}

ReturnedValue ArgumentsObject::getIndexed(Managed *m, uint index, bool *hasProperty)
{
    ArgumentsObject *args = static_cast<ArgumentsObject *>(m);
    if (args->fullyCreated())
        return Object::getIndexed(m, index, hasProperty);

    if (index < static_cast<uint>(args->context()->callData->argc)) {
        if (hasProperty)
            *hasProperty = true;
        return args->context()->callData->args[index].asReturnedValue();
    }
    if (hasProperty)
        *hasProperty = false;
    return Encode::undefined();
}

void ArgumentsObject::putIndexed(Managed *m, uint index, const Value &value)
{
    ArgumentsObject *args = static_cast<ArgumentsObject *>(m);
    if (!args->fullyCreated() && index >= static_cast<uint>(args->context()->callData->argc))
        args->fullyCreate();

    if (args->fullyCreated()) {
        Object::putIndexed(m, index, value);
        return;
    }

    args->context()->callData->args[index] = value;
}

bool ArgumentsObject::deleteIndexedProperty(Managed *m, uint index)
{
    ArgumentsObject *args = static_cast<ArgumentsObject *>(m);
    if (!args->fullyCreated())
        args->fullyCreate();
    return Object::deleteIndexedProperty(m, index);
}

PropertyAttributes ArgumentsObject::queryIndexed(const Managed *m, uint index)
{
    const ArgumentsObject *args = static_cast<const ArgumentsObject *>(m);
    if (args->fullyCreated())
        return Object::queryIndexed(m, index);

    uint numAccessors = qMin((int)args->context()->function->formalParameterCount(), args->context()->callData->argc);
    uint argCount = args->context()->callData->argc;
    if (index >= argCount)
        return PropertyAttributes();
    if (index >= numAccessors)
        return Attr_Data;
    return Attr_Accessor;
}

DEFINE_OBJECT_VTABLE(ArgumentsGetterFunction);

ReturnedValue ArgumentsGetterFunction::call(Managed *getter, CallData *callData)
{
    ExecutionEngine *v4 = static_cast<ArgumentsGetterFunction *>(getter)->engine();
    Scope scope(v4);
    Scoped<ArgumentsGetterFunction> g(scope, static_cast<ArgumentsGetterFunction *>(getter));
    Scoped<ArgumentsObject> o(scope, callData->thisObject.as<ArgumentsObject>());
    if (!o)
        return v4->throwTypeError();

    Q_ASSERT(g->index() < static_cast<unsigned>(o->context()->callData->argc));
    return o->context()->callData->args[g->index()].asReturnedValue();
}

DEFINE_OBJECT_VTABLE(ArgumentsSetterFunction);

ReturnedValue ArgumentsSetterFunction::call(Managed *setter, CallData *callData)
{
    ExecutionEngine *v4 = static_cast<ArgumentsSetterFunction *>(setter)->engine();
    Scope scope(v4);
    Scoped<ArgumentsSetterFunction> s(scope, static_cast<ArgumentsSetterFunction *>(setter));
    Scoped<ArgumentsObject> o(scope, callData->thisObject.as<ArgumentsObject>());
    if (!o)
        return v4->throwTypeError();

    Q_ASSERT(s->index() < static_cast<unsigned>(o->context()->callData->argc));
    o->context()->callData->args[s->index()] = callData->argc ? callData->args[0].asReturnedValue() : Encode::undefined();
    return Encode::undefined();
}

void ArgumentsObject::markObjects(Heap::Base *that, ExecutionEngine *e)
{
    ArgumentsObject::Data *o = static_cast<ArgumentsObject::Data *>(that);
    if (o->context)
        o->context->mark(e);
    if (o->mappedArguments)
        o->mappedArguments->mark(e);

    Object::markObjects(that, e);
}
