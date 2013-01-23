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
#include <qv4jsonobject.h>
#include <qv4objectproto.h>
#include <qjsondocument.h>
#include <qjsonobject.h>
#include <qjsonarray.h>
#include <qjsonvalue.h>

namespace QQmlJS {
namespace VM {

JsonObject::JsonObject(ExecutionContext *context)
    : Object()
{
    prototype = context->engine->objectPrototype;

    defineDefaultProperty(context, QStringLiteral("parse"), method_parse, 2);
    defineDefaultProperty(context, QStringLiteral("stringify"), method_stringify);
}


Value JsonObject::method_parse(ExecutionContext *ctx)
{
    QString jtext = ctx->argument(0).toString(ctx)->toQString();

    const QChar *ch = jtext.constData();
    const QChar *end = ch + jtext.size();

    bool simple = false;
    while (ch < end) {
        if (*ch == ' ' || *ch == '\t' || *ch == '\n' || *ch == '\r') {
            ++ch;
        } else if (*ch == '[' || *ch == '{') {
            break;
        } else {
            // simple type
            jtext.prepend('[');
            jtext.append(']');
            simple = true;
            break;
        }
    }

    QJsonParseError e;
    QJsonDocument doc = QJsonDocument::fromJson(jtext.toUtf8(), &e);
    if (e.error != QJsonParseError::NoError)
        ctx->throwSyntaxError(0);

    // iterate over the doc and convert to V4 types
    Value result;
    if (doc.isArray())
            result = convertArray(ctx, doc.array());
    else if (doc.isObject())
        result = convertObject(ctx, doc.object());
    else
        result = Value::undefinedValue();

    if (simple) {
        result = result.objectValue()->__get__(ctx, (uint)0);
    }

    return result;
}

Value JsonObject::method_stringify(ExecutionContext *ctx)
{
    Q_UNUSED(ctx);
    assert(!"Not implemented");
}

static void checkString(ExecutionContext *context, const QString &s)
{
    const QChar *ch = s.constData();
    const QChar *end = ch + s.length();
    while (ch < end) {
        if (ch->unicode() <= 0x1f)
            context->throwSyntaxError(0);
        ++ch;
    }
}

Value JsonObject::convertValue(ExecutionContext *context, const QJsonValue &value)
{
    switch (value.type()) {
    case QJsonValue::Null:
        return Value::nullValue();
    case QJsonValue::Bool:
        return Value::fromBoolean(value.toBool());
    case QJsonValue::Double:
        return Value::fromDouble(value.toDouble());
    case QJsonValue::String: {
        Value v = Value::fromString(context, value.toString());
        checkString(context, v.stringValue()->toQString());
        return v;
    }
    case QJsonValue::Array:
        return convertArray(context, value.toArray());
    case QJsonValue::Object:
        return convertObject(context, value.toObject());
    default:
        assert(!"internal error in JSON conversion");
        return Value::undefinedValue();
    }
}

Value JsonObject::convertArray(ExecutionContext *context, const QJsonArray &array)
{
    ArrayObject *o = context->engine->newArrayObject(context);
    for (int i = 0; i < array.size(); ++i) {
        QJsonValue v = array.at(i);
        o->array.set(i, convertValue(context, v));
    }
    o->array.setLengthUnchecked(array.size());
    return Value::fromObject(o);
}

Value JsonObject::convertObject(ExecutionContext *context, const QJsonObject &object)
{
    Object *o = context->engine->newObject();
    for (QJsonObject::const_iterator it = object.constBegin(); it != object.constEnd(); ++it) {
        QString key = it.key();
        checkString(context, key);
        QJsonValue v = it.value();
        o->__put__(context, key, convertValue(context, v));
    }
    return Value::fromObject(o);
}


}
}
