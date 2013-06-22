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
#include <qv4argumentsobject_p.h>

using namespace QV4;

static Value throwTypeError(SimpleCallContext *ctx)
{
    ctx->throwTypeError();
    return Value::undefinedValue();
}

DEFINE_MANAGED_VTABLE(ArgumentsObject);

ArgumentsObject::ArgumentsObject(CallContext *context, int formalParameterCount, int actualParameterCount)
    : Object(context->engine), context(context)
{
    vtbl = &static_vtbl;
    type = Type_ArgumentsObject;

    defineDefaultProperty(context->engine->id_length, Value::fromInt32(actualParameterCount));
    if (context->strictMode) {
        for (uint i = 0; i < context->argumentCount; ++i)
            Object::put(context, QString::number(i), context->arguments[i]);
        FunctionObject *thrower = context->engine->newBuiltinFunction(context, 0, throwTypeError);
        Property pd = Property::fromAccessor(thrower, thrower);
        __defineOwnProperty__(context, QStringLiteral("callee"), pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
        __defineOwnProperty__(context, QStringLiteral("caller"), pd, Attr_Accessor|Attr_NotConfigurable|Attr_NotEnumerable);
    } else {
        uint numAccessors = qMin(formalParameterCount, actualParameterCount);
        context->engine->requireArgumentsAccessors(numAccessors);
        for (uint i = 0; i < (uint)numAccessors; ++i) {
            mappedArguments.append(context->argument(i));
            __defineOwnProperty__(context, i, context->engine->argumentsAccessors.at(i), Attr_Accessor);
        }
        for (uint i = numAccessors; i < qMin((uint)actualParameterCount, context->argumentCount); ++i) {
            Property pd = Property::fromValue(context->argument(i));
            __defineOwnProperty__(context, i, pd, Attr_Data);
        }
        defineDefaultProperty(context, QStringLiteral("callee"), Value::fromObject(context->function));
        isNonStrictArgumentsObject = true;
    }
}

void ArgumentsObject::destroy(Managed *that)
{
    static_cast<ArgumentsObject *>(that)->~ArgumentsObject();
}

bool ArgumentsObject::defineOwnProperty(ExecutionContext *ctx, uint index, const Property &desc, PropertyAttributes attrs)
{
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
            Value arg = desc.value;
            map.setter()->call(Value::fromObject(this), &arg, 1);
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

Value ArgumentsGetterFunction::call(Managed *getter, const Value &thisObject, Value *, int)
{
    ArgumentsGetterFunction *g = static_cast<ArgumentsGetterFunction *>(getter);
    Object *that = thisObject.asObject();
    if (!that)
        getter->engine()->current->throwTypeError();
    ArgumentsObject *o = that->asArgumentsObject();
    if (!o)
        getter->engine()->current->throwTypeError();

    assert(g->index < o->context->argumentCount);
    return o->context->argument(g->index);
}

DEFINE_MANAGED_VTABLE(ArgumentsSetterFunction);

Value ArgumentsSetterFunction::call(Managed *setter, const Value &thisObject, Value *args, int argc)
{
    ArgumentsSetterFunction *s = static_cast<ArgumentsSetterFunction *>(setter);
    Object *that = thisObject.asObject();
    if (!that)
        setter->engine()->current->throwTypeError();
    ArgumentsObject *o = that->asArgumentsObject();
    if (!o)
        setter->engine()->current->throwTypeError();

    assert(s->index < o->context->argumentCount);
    o->context->arguments[s->index] = argc ? args[0] : Value::undefinedValue();
    return Value::undefinedValue();
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
