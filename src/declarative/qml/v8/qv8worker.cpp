/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv8worker_p.h"

QT_BEGIN_NAMESPACE

// We allow the following JavaScript types to be passed between the main and 
// the secondary thread:
//    + undefined
//    + null
//    + Boolean
//    + String
//    + Function 
//    + Array
//    + "Simple" Objects
//    + Number
//    + Date
//    + RegExp
// <quint8 type><quint24 size><data>

enum Type {
    WorkerUndefined,
    WorkerNull,
    WorkerTrue,
    WorkerFalse,
    WorkerString,
    WorkerFunction,
    WorkerArray,
    WorkerObject,
    WorkerInt32,
    WorkerUint32,
    WorkerNumber,
    WorkerDate,
    WorkerRegexp
};

static inline quint32 valueheader(Type type, quint32 size = 0)
{
    return quint8(type) << 24 | (size & 0xFFFFFF);
}

static inline Type headertype(quint32 header)
{
    return (Type)(header >> 24);
}

static inline quint32 headersize(quint32 header)
{
    return header & 0xFFFFFF;
}

static inline void push(QByteArray &data, quint32 value)
{
    data.append((const char *)&value, sizeof(quint32));
}

static inline void push(QByteArray &data, double value)
{
    data.append((const char *)&value, sizeof(double));
}

static inline void reserve(QByteArray &data, int extra)
{
    data.reserve(data.size() + extra);
}

static inline quint32 popUint32(const char *&data)
{
    quint32 rv = *((quint32 *)data);
    data += sizeof(quint32);
    return rv;
}

static inline double popDouble(const char *&data)
{
    double rv = *((double *)data);
    data += sizeof(double);
    return rv;
}

// XXX double check exception safety

#include <QDebug>
#define ALIGN(size) (((size) + 3) & ~3)
void QV8Worker::serialize(QByteArray &data, v8::Handle<v8::Value> v, QV8Engine *engine)
{
    if (v.IsEmpty()) {
    } else if (v->IsUndefined()) {
        push(data, valueheader(WorkerUndefined));
    } else if (v->IsNull()) {
        push(data, valueheader(WorkerNull));
    } else if (v->IsTrue()) {
        push(data, valueheader(WorkerTrue));
    } else if (v->IsFalse()) {
        push(data, valueheader(WorkerFalse));
    } else if (v->IsString()) {
        v8::Handle<v8::String> string = v->ToString();
        int length = string->Length() + 1;
        if (length > 0xFFFFFF) {
            push(data, valueheader(WorkerUndefined));
            return;
        }
        int utf16size = ALIGN(length * sizeof(uint16_t));

        reserve(data, utf16size + sizeof(quint32));
        push(data, valueheader(WorkerString, length));
        
        int offset = data.size();
        data.resize(data.size() + utf16size);
        char *buffer = data.data() + offset;

        string->Write((uint16_t*)buffer);
    } else if (v->IsFunction()) {
        // XXX
        push(data, valueheader(WorkerUndefined));
    } else if (v->IsArray()) {
        v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(v);
        uint32_t length = array->Length();
        if (length > 0xFFFFFF) {
            push(data, valueheader(WorkerUndefined));
            return;
        }
        reserve(data, sizeof(quint32) + length * sizeof(quint32));
        push(data, valueheader(WorkerArray, length));
        for (uint32_t ii = 0; ii < length; ++ii)
            serialize(data, array->Get(ii), engine);
    } else if (v->IsInt32()) {
        reserve(data, 2 * sizeof(quint32));
        push(data, valueheader(WorkerInt32));
        push(data, (quint32)v->Int32Value());
    } else if (v->IsUint32()) {
        reserve(data, 2 * sizeof(quint32));
        push(data, valueheader(WorkerUint32));
        push(data, v->Uint32Value());
    } else if (v->IsNumber()) {
        reserve(data, sizeof(quint32) + sizeof(double));
        push(data, valueheader(WorkerNumber));
        push(data, v->NumberValue());
    } else if (v->IsDate()) {
        reserve(data, sizeof(quint32) + sizeof(double));
        push(data, valueheader(WorkerDate));
        push(data, v8::Handle<v8::Date>::Cast(v)->NumberValue());
    } else if (v->IsRegExp()) {
        v8::Handle<v8::RegExp> regexp = v8::Handle<v8::RegExp>::Cast(v);
        quint32 flags = regexp->GetFlags();
        v8::Local<v8::String> source = regexp->GetSource();

        int length = source->Length() + 1;
        if (length > 0xFFFFFF) {
            push(data, valueheader(WorkerUndefined));
            return;
        }
        int utf16size = ALIGN(length * sizeof(uint16_t));

        reserve(data, sizeof(quint32) + utf16size);
        push(data, valueheader(WorkerRegexp, flags));
        push(data, (quint32)length);
        int offset = data.size();
        data.resize(data.size() + utf16size);
        char *buffer = data.data() + offset;

        source->Write((uint16_t*)buffer);
    } else if (v->IsObject() && !v->ToObject()->GetExternalResource()) {
        v8::Handle<v8::Object> object = v->ToObject();
        v8::Local<v8::Array> properties = engine->getOwnPropertyNames(object);
        quint32 length = properties->Length();
        if (length > 0xFFFFFF) {
            push(data, valueheader(WorkerUndefined));
            return;
        }
        push(data, valueheader(WorkerObject, length));
        v8::TryCatch tc;
        for (quint32 ii = 0; ii < length; ++ii) {
            v8::Local<v8::String> str = properties->Get(ii)->ToString();
            serialize(data, str, engine);

            v8::Local<v8::Value> val = object->Get(str);
            if (tc.HasCaught()) {
                serialize(data, v8::Undefined(), engine);
                tc.Reset();
            } else {
                serialize(data, val, engine);
            }
        }
    } else {
        push(data, valueheader(WorkerUndefined));
    }

    // XXX Need to serialize QDeclarativeListModel
    /*
        QDeclarativeListModel *lm = qobject_cast<QDeclarativeListModel *>(value.toQObject());
        if (lm) {
            QDeclarativeListModelWorkerAgent *agent = lm->agent();
            if (agent) {
                QDeclarativeListModelWorkerAgent::VariantRef v(agent);
                return QVariant::fromValue(v);
            } else {
                return QVariant();
            }
        }
    */
}

v8::Handle<v8::Value> QV8Worker::deserialize(const char *&data, QV8Engine *engine)
{
    quint32 header = popUint32(data);
    Type type = headertype(header);

    switch (type) {
    case WorkerUndefined:
        return v8::Undefined();
    case WorkerNull:
        return v8::Null();
    case WorkerTrue:
        return v8::True();
    case WorkerFalse:
        return v8::False();
    case WorkerString:
    {
        quint32 size = headersize(header);
        v8::Local<v8::String> string = v8::String::New((uint16_t*)data, size - 1);
        data += ALIGN(size * sizeof(uint16_t));
        return string;
    }
    case WorkerFunction:
        Q_ASSERT(!"Unreachable");
        break;
    case WorkerArray:
    {
        quint32 size = headersize(header);
        v8::Local<v8::Array> array = v8::Array::New(size);
        for (quint32 ii = 0; ii < size; ++ii) {
            array->Set(ii, deserialize(data, engine));
        }
        return array;
    }
    case WorkerObject:
    {
        quint32 size = headersize(header);
        v8::Local<v8::Object> o = v8::Object::New();
        for (quint32 ii = 0; ii < size; ++ii) {
            v8::Handle<v8::Value> name = deserialize(data, engine);
            v8::Handle<v8::Value> value = deserialize(data, engine);
            o->Set(name, value);
        }
        return o;
    }
    case WorkerInt32:
        return v8::Integer::New((qint32)popUint32(data));
    case WorkerUint32:
        return v8::Integer::NewFromUnsigned(popUint32(data));
    case WorkerNumber:
        return v8::Number::New(popDouble(data));
    case WorkerDate:
        return v8::Date::New(popDouble(data));
    case WorkerRegexp:
    {
        quint32 flags = headersize(header);
        quint32 length = popUint32(data);
        v8::Local<v8::String> source = v8::String::New((uint16_t*)data, length - 1);
        data += ALIGN(length * sizeof(uint16_t));
        return v8::RegExp::New(source, (v8::RegExp::Flags)flags);
    }
    }
    Q_ASSERT(!"Unreachable");
    return v8::Undefined();
}

QByteArray QV8Worker::serialize(v8::Handle<v8::Value> value, QV8Engine *engine)
{
    QByteArray rv;
    serialize(rv, value, engine);
    return rv;
}

v8::Handle<v8::Value> QV8Worker::deserialize(const QByteArray &data, QV8Engine *engine)
{
    const char *stream = data.constData();
    return deserialize(stream, engine);
}

QT_END_NAMESPACE

