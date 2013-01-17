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

#include "qv8jsonwrapper_p.h"
#include "qv8engine_p.h"
#include "qjsconverter_impl_p.h"

QT_BEGIN_NAMESPACE

QV8JsonWrapper::QV8JsonWrapper()
: m_engine(0)
{
}

QV8JsonWrapper::~QV8JsonWrapper()
{
}

void QV8JsonWrapper::init(QV8Engine *engine)
{
    m_engine = engine;
}

void QV8JsonWrapper::destroy()
{
}

v8::Handle<v8::Value> QV8JsonWrapper::fromJsonValue(const QJsonValue &value)
{
    if (value.isString())
        return QJSConverter::toString(value.toString());
    else if (value.isDouble())
        return v8::Number::New(value.toDouble());
    else if (value.isBool())
        return value.toBool() ? v8::True() : v8::False();
    else if (value.isArray())
        return fromJsonArray(value.toArray());
    else if (value.isObject())
        return fromJsonObject(value.toObject());
    else if (value.isNull())
        return v8::Null();
    else
        return v8::Undefined();
}

QJsonValue QV8JsonWrapper::toJsonValue(v8::Handle<v8::Value> value,
                                       V8ObjectSet &visitedObjects)
{
    if (value->IsString())
        return QJsonValue(QJSConverter::toString(value.As<v8::String>()));
    else if (value->IsNumber())
        return QJsonValue(value->NumberValue());
    else if (value->IsBoolean())
        return QJsonValue(value->BooleanValue());
    else if (value->IsArray())
        return toJsonArray(value.As<v8::Array>(), visitedObjects);
    else if (value->IsObject())
        return toJsonObject(value.As<v8::Object>(), visitedObjects);
    else if (value->IsNull())
        return QJsonValue(QJsonValue::Null);
    else
        return QJsonValue(QJsonValue::Undefined);
}

v8::Local<v8::Object> QV8JsonWrapper::fromJsonObject(const QJsonObject &object)
{
    v8::Local<v8::Object> v8object = v8::Object::New();
    for (QJsonObject::const_iterator it = object.begin(); it != object.end(); ++it)
        v8object->Set(QJSConverter::toString(it.key()), fromJsonValue(it.value()));
    return v8object;
}

QJsonObject QV8JsonWrapper::toJsonObject(v8::Handle<v8::Value> value,
                                         V8ObjectSet &visitedObjects)
{
    QJsonObject result;
    if (!value->IsObject() || value->IsArray() || value->IsFunction())
        return result;

    v8::Handle<v8::Object> v8object(value.As<v8::Object>());
    if (visitedObjects.contains(v8object)) {
        // Avoid recursion.
        // For compatibility with QVariant{List,Map} conversion, we return an
        // empty object (and no error is thrown).
        return result;
    }

    visitedObjects.insert(v8object);

    v8::Local<v8::Array> propertyNames = m_engine->getOwnPropertyNames(v8object);
    uint32_t length = propertyNames->Length();
    for (uint32_t i = 0; i < length; ++i) {
        v8::Local<v8::Value> name = propertyNames->Get(i);
        v8::Local<v8::Value> propertyValue = v8object->Get(name);
        if (!propertyValue->IsFunction())
            result.insert(QJSConverter::toString(name->ToString()),
                          toJsonValue(propertyValue, visitedObjects));
    }

    visitedObjects.remove(v8object);

    return result;
}

v8::Local<v8::Array> QV8JsonWrapper::fromJsonArray(const QJsonArray &array)
{
    int size = array.size();
    v8::Local<v8::Array> v8array = v8::Array::New(size);
    for (int i = 0; i < size; i++)
        v8array->Set(i, fromJsonValue(array.at(i)));
    return v8array;
}

QJsonArray QV8JsonWrapper::toJsonArray(v8::Handle<v8::Value> value,
                                       V8ObjectSet &visitedObjects)
{
    QJsonArray result;
    if (!value->IsArray())
        return result;

    v8::Handle<v8::Array> v8array(value.As<v8::Array>());
    if (visitedObjects.contains(v8array)) {
        // Avoid recursion.
        // For compatibility with QVariant{List,Map} conversion, we return an
        // empty array (and no error is thrown).
        return result;
    }

    visitedObjects.insert(v8array);

    uint32_t length = v8array->Length();
    for (uint32_t i = 0; i < length; ++i) {
        v8::Local<v8::Value> element = v8array->Get(i);
        if (!element->IsFunction())
            result.append(toJsonValue(element, visitedObjects));
    }

    visitedObjects.remove(v8array);

    return result;
}

QT_END_NAMESPACE
