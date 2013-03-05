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

#include "qv4regexpobject.h"
#include "qv4jsir_p.h"
#include "qv4isel_p.h"
#include "qv4objectproto.h"
#include "qv4stringobject.h"
#include "qv4mm.h"

#include <private/qqmljsengine_p.h>
#include <private/qqmljslexer_p.h>
#include <private/qqmljsparser_p.h>
#include <private/qqmljsast_p.h>
#include <qv4jsir_p.h>
#include <qv4codegen_p.h>
#include "private/qlocale_tools_p.h"

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <cassert>
#include <typeinfo>
#include <iostream>
#include "qv4alloca_p.h"

using namespace QQmlJS::VM;

DEFINE_MANAGED_VTABLE(RegExpObject);

RegExpObject::RegExpObject(ExecutionEngine *engine, PassRefPtr<RegExp> value, bool global)
    : Object(engine)
    , value(value)
    , global(global)
{
    type = Type_RegExpObject;

    PropertyDescriptor *lastIndexProperty = insertMember(engine->newIdentifier(QStringLiteral("lastIndex")));
    lastIndexProperty->type = PropertyDescriptor::Data;
    lastIndexProperty->writable = PropertyDescriptor::Enabled;
    lastIndexProperty->enumberable = PropertyDescriptor::Disabled;
    lastIndexProperty->configurable = PropertyDescriptor::Disabled;
    lastIndexProperty->value = Value::fromInt32(0);
    if (!this->value.get())
        return;
    defineReadonlyProperty(engine->newIdentifier(QStringLiteral("source")), Value::fromString(engine->newString(this->value->pattern())));
    defineReadonlyProperty(engine->newIdentifier(QStringLiteral("global")), Value::fromBoolean(global));
    defineReadonlyProperty(engine->newIdentifier(QStringLiteral("ignoreCase")), Value::fromBoolean(this->value->ignoreCase()));
    defineReadonlyProperty(engine->newIdentifier(QStringLiteral("multiline")), Value::fromBoolean(this->value->multiLine()));
}

void RegExpObject::destroy(Managed *that)
{
    static_cast<RegExpObject *>(that)->~RegExpObject();
}

PropertyDescriptor *RegExpObject::lastIndexProperty(ExecutionContext *ctx)
{
    assert(0 == internalClass->find(ctx->engine->newIdentifier(QStringLiteral("lastIndex"))));
    return &memberData[0];
}

DEFINE_MANAGED_VTABLE(RegExpCtor);

RegExpCtor::RegExpCtor(ExecutionContext *scope)
    : FunctionObject(scope)
{
    vtbl = &static_vtbl;
}

Value RegExpCtor::construct(Managed *, ExecutionContext *ctx, Value *argv, int argc)
{
    Value r = argc > 0 ? argv[0] : Value::undefinedValue();
    Value f = argc > 1 ? argv[1] : Value::undefinedValue();
    if (RegExpObject *re = r.asRegExpObject()) {
        if (!f.isUndefined())
            ctx->throwTypeError();

        RegExpObject *o = ctx->engine->newRegExpObject(re->value, re->global);
        return Value::fromObject(o);
    }

    if (r.isUndefined())
        r = Value::fromString(ctx, QString());
    else if (!r.isString())
        r = __qmljs_to_string(r, ctx);

    bool global = false;
    bool ignoreCase = false;
    bool multiLine = false;
    if (!f.isUndefined()) {
        f = __qmljs_to_string(f, ctx);
        QString str = f.stringValue()->toQString();
        for (int i = 0; i < str.length(); ++i) {
            if (str.at(i) == QChar('g') && !global) {
                global = true;
            } else if (str.at(i) == QChar('i') && !ignoreCase) {
                ignoreCase = true;
            } else if (str.at(i) == QChar('m') && !multiLine) {
                multiLine = true;
            } else {
                ctx->throwSyntaxError(0);
            }
        }
    }

    RefPtr<RegExp> re = RegExp::create(ctx->engine, r.stringValue()->toQString(), ignoreCase, multiLine);
    if (!re->isValid())
        ctx->throwSyntaxError(0);

    RegExpObject *o = ctx->engine->newRegExpObject(re, global);
    return Value::fromObject(o);
}

Value RegExpCtor::call(Managed *that, ExecutionContext *ctx, const Value &thisObject, Value *argv, int argc)
{
    if (argc > 0 && argv[0].asRegExpObject()) {
        if (argc == 1 || argv[1].isUndefined())
            return argv[0];
    }

    return construct(that, ctx, argv, argc);
}

void RegExpPrototype::init(ExecutionContext *ctx, const Value &ctor)
{
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_prototype, Value::fromObject(this));
    ctor.objectValue()->defineReadonlyProperty(ctx->engine->id_length, Value::fromInt32(2));
    defineDefaultProperty(ctx, QStringLiteral("constructor"), ctor);
    defineDefaultProperty(ctx, QStringLiteral("exec"), method_exec, 1);
    defineDefaultProperty(ctx, QStringLiteral("test"), method_test, 1);
    defineDefaultProperty(ctx, QStringLiteral("toString"), method_toString, 0);
    defineDefaultProperty(ctx, QStringLiteral("compile"), method_compile, 2);
}

Value RegExpPrototype::method_exec(ExecutionContext *ctx)
{
    RegExpObject *r = ctx->thisObject.asRegExpObject();
    if (!r)
        ctx->throwTypeError();

    Value arg = ctx->argument(0);
    arg = __qmljs_to_string(arg, ctx);
    QString s = arg.stringValue()->toQString();

    int offset = r->global ? r->lastIndexProperty(ctx)->value.toInt32(ctx) : 0;
    if (offset < 0 || offset > s.length()) {
        r->lastIndexProperty(ctx)->value = Value::fromInt32(0);
        return Value::nullValue();
    }

    uint* matchOffsets = (uint*)alloca(r->value->captureCount() * 2 * sizeof(uint));
    int result = r->value->match(s, offset, matchOffsets);
    if (result == -1) {
        r->lastIndexProperty(ctx)->value = Value::fromInt32(0);
        return Value::nullValue();
    }

    // fill in result data
    ArrayObject *array = ctx->engine->newArrayObject(ctx)->asArrayObject();
    for (int i = 0; i < r->value->captureCount(); ++i) {
        int start = matchOffsets[i * 2];
        int end = matchOffsets[i * 2 + 1];
        Value entry = Value::undefinedValue();
        if (start != -1 && end != -1)
            entry = Value::fromString(ctx, s.mid(start, end - start));
        array->push_back(entry);
    }

    array->__put__(ctx, QLatin1String("index"), Value::fromInt32(result));
    array->__put__(ctx, QLatin1String("input"), arg);

    if (r->global)
        r->lastIndexProperty(ctx)->value = Value::fromInt32(matchOffsets[1]);

    return Value::fromObject(array);
}

Value RegExpPrototype::method_test(ExecutionContext *ctx)
{
    Value r = method_exec(ctx);
    return Value::fromBoolean(!r.isNull());
}

Value RegExpPrototype::method_toString(ExecutionContext *ctx)
{
    RegExpObject *r = ctx->thisObject.asRegExpObject();
    if (!r)
        ctx->throwTypeError();

    QString result = QChar('/') + r->value->pattern();
    result += QChar('/');
    // ### 'g' option missing
    if (r->value->ignoreCase())
        result += QChar('i');
    if (r->value->multiLine())
        result += QChar('m');
    return Value::fromString(ctx, result);
}

Value RegExpPrototype::method_compile(ExecutionContext *ctx)
{
    RegExpObject *r = ctx->thisObject.asRegExpObject();
    if (!r)
        ctx->throwTypeError();

    RegExpObject *re = ctx->engine->regExpCtor.asFunctionObject()->construct(ctx, ctx->arguments, ctx->argumentCount).asRegExpObject();

    r->value = re->value;
    r->global = re->global;
    return Value::undefinedValue();
}

