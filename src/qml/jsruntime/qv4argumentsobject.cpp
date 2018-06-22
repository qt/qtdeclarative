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
#include <qv4arrayobject_p.h>
#include <qv4alloca_p.h>
#include <qv4scopedvalue_p.h>
#include <qv4string_p.h>
#include <qv4function_p.h>
#include <qv4jscall_p.h>
#include <qv4symbol_p.h>

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

    Q_ASSERT(CalleePropertyIndex == internalClass->find(v4->id_callee()->identifier()));
    setProperty(v4, CalleePropertyIndex, context->d()->function);
    Q_ASSERT(LengthPropertyIndex == internalClass->find(v4->id_length()->identifier()));
    setProperty(v4, LengthPropertyIndex, Primitive::fromInt32(context->argc()));
    Q_ASSERT(SymbolIteratorPropertyIndex == internalClass->find(v4->symbol_iterator()->identifier()));
    setProperty(v4, SymbolIteratorPropertyIndex, *v4->arrayProtoValues());
}

void Heap::StrictArgumentsObject::init(QV4::CppStackFrame *frame)
{
    Q_ASSERT(vtable() == QV4::StrictArgumentsObject::staticVTable());
    ExecutionEngine *v4 = internalClass->engine;

    Object::init();

    Q_ASSERT(CalleePropertyIndex == internalClass->find(v4->id_callee()->identifier()));
    Q_ASSERT(SymbolIteratorPropertyIndex == internalClass->find(v4->symbol_iterator()->identifier()));
    setProperty(v4, SymbolIteratorPropertyIndex, *v4->arrayProtoValues());
    setProperty(v4, CalleePropertyIndex + QV4::Object::GetterOffset, *v4->thrower());
    setProperty(v4, CalleePropertyIndex + QV4::Object::SetterOffset, *v4->thrower());

    Scope scope(v4);
    Scoped<QV4::StrictArgumentsObject> args(scope, this);
    args->arrayReserve(frame->originalArgumentsCount);
    args->arrayPut(0, frame->originalArguments, frame->originalArgumentsCount);

    Q_ASSERT(LengthPropertyIndex == args->internalClass()->find(v4->id_length()->identifier()));
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

bool ArgumentsObject::defineOwnProperty(Managed *m, Identifier id, const Property *desc, PropertyAttributes attrs)
{
    if (!id.isArrayIndex())
        return Object::defineOwnProperty(m, id, desc, attrs);

    ArgumentsObject *a = static_cast<ArgumentsObject *>(m);
    a->fullyCreate();

    uint index = id.asArrayIndex();
    Scope scope(m);
    ScopedProperty map(scope);
    PropertyAttributes mapAttrs;
    uint numAccessors = qMin(a->d()->nFormals, a->context()->argc());
    bool isMapped = false;
    if (a->arrayData() && index < numAccessors &&
        a->arrayData()->attributes(index).isAccessor() &&
        a->arrayData()->get(index) == scope.engine->argumentsAccessors[index].getter()->asReturnedValue())
        isMapped = true;

    if (isMapped) {
        Q_ASSERT(a->arrayData());
        mapAttrs = a->arrayData()->attributes(index);
        a->arrayData()->getProperty(index, map, &mapAttrs);
        a->setArrayAttributes(index, Attr_Data);
        PropertyIndex arrayIndex{ a->arrayData(), a->arrayData()->values.values + a->arrayData()->mappedIndex(index) };
        arrayIndex.set(scope.engine, a->d()->mappedArguments->values[index]);
    }

    bool result = Object::defineOwnProperty(m, id, desc, attrs);
    if (!result)
        return false;

    if (isMapped && attrs.isData()) {
        Q_ASSERT(a->arrayData());
        ScopedFunctionObject setter(scope, map->setter());
        JSCallData jsCallData(scope, 1);
        *jsCallData->thisObject = a->asReturnedValue();
        jsCallData->args[0] = desc->value;
        setter->call(jsCallData);

        if (attrs.isWritable()) {
            a->setArrayAttributes(index, mapAttrs);
            a->arrayData()->setProperty(m->engine(), index, map);
        }
    }

    return result;
}

ReturnedValue ArgumentsObject::get(const Managed *m, Identifier id, const Value *receiver, bool *hasProperty)
{
    const ArgumentsObject *args = static_cast<const ArgumentsObject *>(m);
    if (id.isArrayIndex() && !args->fullyCreated()) {
        uint index = id.asArrayIndex();
        if (index < static_cast<uint>(args->context()->argc())) {
            if (hasProperty)
                *hasProperty = true;
            return args->context()->args()[index].asReturnedValue();
        }
    }
    return Object::get(m, id, receiver, hasProperty);
}

bool ArgumentsObject::put(Managed *m, Identifier id, const Value &value, Value *receiver)
{
    ArgumentsObject *args = static_cast<ArgumentsObject *>(m);
    if (id.isArrayIndex()) {
        uint index = id.asArrayIndex();
        if (!args->fullyCreated() && index >= static_cast<uint>(args->context()->argc()))
            args->fullyCreate();

        if (!args->fullyCreated()) {
            args->context()->setArg(index, value);
            return true;
        }
    }
    return Object::put(m, id, value, receiver);
}

bool ArgumentsObject::deleteProperty(Managed *m, Identifier id)
{
    ArgumentsObject *args = static_cast<ArgumentsObject *>(m);
    if (!args->fullyCreated())
        args->fullyCreate();
    return Object::deleteProperty(m, id);
}

PropertyAttributes ArgumentsObject::getOwnProperty(Managed *m, Identifier id, Property *p)
{
    const ArgumentsObject *args = static_cast<const ArgumentsObject *>(m);
    if (!id.isArrayIndex() || args->fullyCreated())
        return Object::getOwnProperty(m, id, p);

    uint index = id.asArrayIndex();
    uint argCount = args->context()->argc();
    if (index >= argCount)
        return PropertyAttributes();
    if (p)
        p->value = args->context()->args()[index];
    return Attr_Data;
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

qint64 ArgumentsObject::getLength(const Managed *m)
{
    const ArgumentsObject *a = static_cast<const ArgumentsObject *>(m);
    return a->propertyData(Heap::ArgumentsObject::LengthPropertyIndex)->toLength();
}
