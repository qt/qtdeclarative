/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qv4argumentsobject_p.h>
#include <qv4alloca_p.h>
#include <qv4scopedvalue_p.h>
#include <qv4string_p.h>
#include <qv4function_p.h>
#include <qv4jscall_p.h>

using namespace QV4;

DEFINE_OBJECT_VTABLE(ArgumentsObject);
DEFINE_OBJECT_VTABLE(StrictArgumentsObject);

void Heap::ArgumentsObject::init(QV4::CppStackFrame *frame)
{
    ExecutionEngine *v4 = internalClass->engine;

    int nFormals = frame->v4Function->nFormals;
    QV4::CallContext *context = static_cast<QV4::CallContext *>(frame->context());

    Object::init();
    fullyCreated = false;
    this->nFormals = nFormals;
    this->context.set(v4, context->d());
    Q_ASSERT(vtable() == QV4::ArgumentsObject::staticVTable());

    Q_ASSERT(CalleePropertyIndex == internalClass->find(v4->id_callee()));
    setProperty(v4, CalleePropertyIndex, context->d()->function);
    Q_ASSERT(LengthPropertyIndex == internalClass->find(v4->id_length()));
    setProperty(v4, LengthPropertyIndex, Primitive::fromInt32(context->argc()));
}

void Heap::StrictArgumentsObject::init(QV4::CppStackFrame *frame)
{
    Q_ASSERT(vtable() == QV4::StrictArgumentsObject::staticVTable());
    ExecutionEngine *v4 = internalClass->engine;

    Object::init();

    Q_ASSERT(CalleePropertyIndex == internalClass->find(v4->id_callee()));
    Q_ASSERT(CallerPropertyIndex == internalClass->find(v4->id_caller()));
    setProperty(v4, CalleePropertyIndex + QV4::Object::GetterOffset, *v4->thrower());
    setProperty(v4, CalleePropertyIndex + QV4::Object::SetterOffset, *v4->thrower());
    setProperty(v4, CallerPropertyIndex + QV4::Object::GetterOffset, *v4->thrower());
    setProperty(v4, CallerPropertyIndex + QV4::Object::SetterOffset, *v4->thrower());

    Scope scope(v4);
    Scoped<QV4::StrictArgumentsObject> args(scope, this);
    args->arrayReserve(frame->originalArgumentsCount);
    args->arrayPut(0, frame->originalArguments, frame->originalArgumentsCount);

    Q_ASSERT(LengthPropertyIndex == args->internalClass()->find(v4->id_length()));
    setProperty(v4, LengthPropertyIndex, Primitive::fromInt32(frame->originalArgumentsCount));
}

void ArgumentsObject::fullyCreate()
{
    if (fullyCreated())
        return;

    Scope scope(engine());

    int argCount = context()->argc();
    uint numAccessors = qMin(d()->nFormals, argCount);
    ArrayData::realloc(this, Heap::ArrayData::Sparse, argCount, true);
    scope.engine->requireArgumentsAccessors(numAccessors);

    Scoped<MemberData> md(scope, d()->mappedArguments);
    if (numAccessors) {
        d()->mappedArguments.set(scope.engine, md->allocate(scope.engine, numAccessors));
        for (uint i = 0; i < numAccessors; ++i) {
            d()->mappedArguments->values.set(scope.engine, i, context()->args()[i]);
            arraySet(i, scope.engine->argumentsAccessors + i, Attr_Accessor);
        }
    }
    arrayPut(numAccessors, context()->args() + numAccessors, argCount - numAccessors);
    for (int i = int(numAccessors); i < argCount; ++i)
        setArrayAttributes(i, Attr_Data);

    d()->fullyCreated = true;
}

bool ArgumentsObject::defineOwnProperty(ExecutionEngine *engine, uint index, const Property *desc, PropertyAttributes attrs)
{
    fullyCreate();

    Scope scope(engine);
    ScopedProperty map(scope);
    PropertyAttributes mapAttrs;
    uint numAccessors = qMin(d()->nFormals, context()->argc());
    bool isMapped = false;
    if (arrayData() && index < numAccessors &&
        arrayData()->attributes(index).isAccessor() &&
        arrayData()->get(index) == scope.engine->argumentsAccessors[index].getter()->asReturnedValue())
        isMapped = true;

    if (isMapped) {
        Q_ASSERT(arrayData());
        mapAttrs = arrayData()->attributes(index);
        arrayData()->getProperty(index, map, &mapAttrs);
        setArrayAttributes(index, Attr_Data);
        ArrayData::Index arrayIndex{ arrayData(), arrayData()->mappedIndex(index) };
        arrayIndex.set(scope.engine, d()->mappedArguments->values[index]);
    }

    bool result = Object::defineOwnProperty2(scope.engine, index, desc, attrs);
    if (!result) {
        return false;
    }

    if (isMapped && attrs.isData()) {
        Q_ASSERT(arrayData());
        ScopedFunctionObject setter(scope, map->setter());
        JSCallData jsCallData(scope, 1);
        *jsCallData->thisObject = this->asReturnedValue();
        jsCallData->args[0] = desc->value;
        setter->call(jsCallData);

        if (attrs.isWritable()) {
            setArrayAttributes(index, mapAttrs);
            arrayData()->setProperty(engine, index, map);
        }
    }

    return result;
}

ReturnedValue ArgumentsObject::getIndexed(const Managed *m, uint index, bool *hasProperty)
{
    const ArgumentsObject *args = static_cast<const ArgumentsObject *>(m);
    if (args->fullyCreated())
        return Object::getIndexed(m, index, hasProperty);

    if (index < static_cast<uint>(args->context()->argc())) {
        if (hasProperty)
            *hasProperty = true;
        return args->context()->args()[index].asReturnedValue();
    }
    if (hasProperty)
        *hasProperty = false;
    return Encode::undefined();
}

bool ArgumentsObject::putIndexed(Managed *m, uint index, const Value &value)
{
    ArgumentsObject *args = static_cast<ArgumentsObject *>(m);
    if (!args->fullyCreated() && index >= static_cast<uint>(args->context()->argc()))
        args->fullyCreate();

    if (args->fullyCreated())
        return Object::putIndexed(m, index, value);

    args->context()->setArg(index, value);
    return true;
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

    uint numAccessors = qMin(args->d()->nFormals, args->context()->argc());
    uint argCount = args->context()->argc();
    if (index >= argCount)
        return PropertyAttributes();
    if (index >= numAccessors)
        return Attr_Data;
    return Attr_Accessor;
}

DEFINE_OBJECT_VTABLE(ArgumentsGetterFunction);

ReturnedValue ArgumentsGetterFunction::call(const FunctionObject *getter, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = getter->engine();
    Scope scope(v4);
    const ArgumentsGetterFunction *g = static_cast<const ArgumentsGetterFunction *>(getter);
    Scoped<ArgumentsObject> o(scope, thisObject->as<ArgumentsObject>());
    if (!o)
        return v4->throwTypeError();

    Q_ASSERT(g->index() < static_cast<unsigned>(o->context()->argc()));
    return o->context()->args()[g->index()].asReturnedValue();
}

DEFINE_OBJECT_VTABLE(ArgumentsSetterFunction);

ReturnedValue ArgumentsSetterFunction::call(const FunctionObject *setter, const Value *thisObject, const Value *argv, int argc)
{
    ExecutionEngine *v4 = setter->engine();
    Scope scope(v4);
    const ArgumentsSetterFunction *s = static_cast<const ArgumentsSetterFunction *>(setter);
    Scoped<ArgumentsObject> o(scope, thisObject->as<ArgumentsObject>());
    if (!o)
        return v4->throwTypeError();

    Q_ASSERT(s->index() < static_cast<unsigned>(o->context()->argc()));
    o->context()->setArg(s->index(), argc ? argv[0] : Primitive::undefinedValue());
    return Encode::undefined();
}

uint ArgumentsObject::getLength(const Managed *m)
{
    const ArgumentsObject *a = static_cast<const ArgumentsObject *>(m);
    if (a->propertyData(Heap::ArgumentsObject::LengthPropertyIndex)->isInteger())
        return a->propertyData(Heap::ArgumentsObject::LengthPropertyIndex)->integerValue();
    return Primitive::toUInt32(a->propertyData(Heap::ArgumentsObject::LengthPropertyIndex)->doubleValue());
}
