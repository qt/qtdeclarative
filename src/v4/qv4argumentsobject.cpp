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
#include <qv4argumentsobject.h>

namespace QQmlJS {
namespace VM {


ArgumentsObject::ArgumentsObject(ExecutionContext *context, int formalParameterCount, int actualParameterCount)
    : context(context)
{
    type = Type_ArgumentsObject;

    defineDefaultProperty(context->engine->id_length, Value::fromInt32(actualParameterCount));
    if (context->strictMode) {
        for (uint i = 0; i < context->argumentCount; ++i)
            Object::__put__(context, QString::number(i), context->arguments[i]);
        FunctionObject *thrower = context->engine->newBuiltinFunction(context, 0, __qmljs_throw_type_error);
        PropertyDescriptor pd = PropertyDescriptor::fromAccessor(thrower, thrower);
        pd.configurable = PropertyDescriptor::Disabled;
        pd.enumberable = PropertyDescriptor::Disabled;
        __defineOwnProperty__(context, QStringLiteral("callee"), &pd);
        __defineOwnProperty__(context, QStringLiteral("caller"), &pd);
    } else {
        uint numAccessors = qMin(formalParameterCount, actualParameterCount);
        context->engine->requireArgumentsAccessors(numAccessors);
        for (uint i = 0; i < (uint)numAccessors; ++i) {
            mappedArguments.append(context->argument(i));
            __defineOwnProperty__(context, i, &context->engine->argumentsAccessors.at(i));
        }
        PropertyDescriptor pd;
        pd.type = PropertyDescriptor::Data;
        pd.writable = PropertyDescriptor::Enabled;
        pd.configurable = PropertyDescriptor::Enabled;
        pd.enumberable = PropertyDescriptor::Enabled;
        for (uint i = numAccessors; i < qMin((uint)actualParameterCount, context->argumentCount); ++i) {
            pd.value = context->argument(i);
            __defineOwnProperty__(context, i, &pd);
        }
        defineDefaultProperty(context, QStringLiteral("callee"), Value::fromObject(context->function));
        isNonStrictArgumentsObject = true;
    }
}

bool ArgumentsObject::defineOwnProperty(ExecutionContext *ctx, uint index, const PropertyDescriptor *desc)
{
    PropertyDescriptor *pd = array.arrayAt(index);
    PropertyDescriptor map;
    bool isMapped = false;
    if (pd && index < (uint)mappedArguments.size())
        isMapped = pd->isAccessor() && pd->get == context->engine->argumentsAccessors.at(index).get;

    if (isMapped) {
        map = *pd;
        pd->type = PropertyDescriptor::Data;
        pd->writable = PropertyDescriptor::Enabled;
        pd->configurable = PropertyDescriptor::Enabled;
        pd->enumberable = PropertyDescriptor::Enabled;
        pd->value = mappedArguments.at(index);
    }

    isNonStrictArgumentsObject = false;
    bool strict = ctx->strictMode;
    ctx->strictMode = false;
    bool result = Object::__defineOwnProperty__(ctx, index, desc);
    ctx->strictMode = strict;
    isNonStrictArgumentsObject = true;

    if (isMapped && desc->isData()) {
        if (desc->type != PropertyDescriptor::Generic) {
            Value arg = desc->value;
            map.set->call(ctx, Value::fromObject(this), &arg, 1);
        }
        if (desc->writable != PropertyDescriptor::Disabled)
            *pd = map;
    }

    if (ctx->strictMode && !result)
        __qmljs_throw_type_error(ctx);
    return result;
}

void ArgumentsObject::markObjects()
{
    for (int i = 0; i < mappedArguments.size(); ++i) {
        Managed *m = mappedArguments.at(i).asManaged();
        if (m)
            m->mark();
    }
    Object::markObjects();
}


Value ArgumentsGetterFunction::call(ExecutionContext *ctx, Value thisObject, Value *, int)
{
    Object *that = thisObject.asObject();
    if (!that)
        __qmljs_throw_type_error(ctx);
    ArgumentsObject *o = that->asArgumentsObject();
    if (!o)
        __qmljs_throw_type_error(ctx);

    assert(index < o->context->argumentCount);
    return o->context->argument(index);
}

Value ArgumentsSetterFunction::call(ExecutionContext *ctx, Value thisObject, Value *args, int argc)
{
    Object *that = thisObject.asObject();
    if (!that)
        __qmljs_throw_type_error(ctx);
    ArgumentsObject *o = that->asArgumentsObject();
    if (!o)
        __qmljs_throw_type_error(ctx);

    assert(index < o->context->argumentCount);
    o->context->arguments[index] = argc ? args[0] : Value::undefinedValue();
    return Value::undefinedValue();
}


}
}
