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

#include "qv4serialize_p.h"

#include <private/qv8engine_p.h>
#include <private/qqmllistmodel_p.h>
#include <private/qqmllistmodelworkeragent_p.h>

#include <private/qv4value_p.h>
#include <private/qv4dateobject_p.h>
#include <private/qv4regexpobject_p.h>
#include <private/qv4sequenceobject_p.h>
#include <private/qv4objectproto_p.h>

QT_BEGIN_NAMESPACE

using namespace QV4;

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
    WorkerRegexp,
    WorkerListModel,
    WorkerSequence
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

static inline void push(QByteArray &data, void *ptr)
{
    data.append((const char *)&ptr, sizeof(void *));
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

static inline void *popPtr(const char *&data)
{
    void *rv = *((void **)data);
    data += sizeof(void *);
    return rv;
}

// XXX TODO: Check that worker script is exception safe in the case of 
// serialization/deserialization failures

#define ALIGN(size) (((size) + 3) & ~3)
void Serialize::serialize(QByteArray &data, const QV4::Value &v, QV8Engine *engine)
{
    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
    if (v.isEmpty()) {
    } else if (v.isUndefined()) {
        push(data, valueheader(WorkerUndefined));
    } else if (v.isNull()) {
        push(data, valueheader(WorkerNull));
    } else if (v.isBoolean()) {
        push(data, valueheader(v.booleanValue() == true ? WorkerTrue : WorkerFalse));
    } else if (QV4::String *s = v.asString()) {
        const QString &qstr = s->toQString();
        int length = qstr.length();
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

        memcpy(buffer, qstr.constData(), length*sizeof(QChar));
    } else if (v.asFunctionObject()) {
        // XXX TODO: Implement passing function objects between the main and
        // worker scripts
        push(data, valueheader(WorkerUndefined));
    } else if (QV4::ArrayObject *array = v.asArrayObject()) {
        uint32_t length = array->arrayLength();
        if (length > 0xFFFFFF) {
            push(data, valueheader(WorkerUndefined));
            return;
        }
        reserve(data, sizeof(quint32) + length * sizeof(quint32));
        push(data, valueheader(WorkerArray, length));
        for (uint32_t ii = 0; ii < length; ++ii)
            serialize(data, array->getIndexed(ii), engine);
    } else if (v.isInteger()) {
        reserve(data, 2 * sizeof(quint32));
        push(data, valueheader(WorkerInt32));
        push(data, (quint32)v.integerValue());
//    } else if (v->IsUint32()) {
//        reserve(data, 2 * sizeof(quint32));
//        push(data, valueheader(WorkerUint32));
//        push(data, v->Uint32Value());
    } else if (v.isNumber()) {
        reserve(data, sizeof(quint32) + sizeof(double));
        push(data, valueheader(WorkerNumber));
        push(data, v.asDouble());
    } else if (QV4::DateObject *d = v.asDateObject()) {
        reserve(data, sizeof(quint32) + sizeof(double));
        push(data, valueheader(WorkerDate));
        push(data, d->value.asDouble());
    } else if (QV4::RegExpObject *re = v.as<RegExpObject>()) {
        quint32 flags = re->flags();
        QString pattern = re->source();
        int length = pattern.length() + 1;
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

        memcpy(buffer, pattern.constData(), length*sizeof(QChar));
    } else if (QV4::QObjectWrapper *qobjectWrapper = v.as<QV4::QObjectWrapper>()) {
        // XXX TODO: Generalize passing objects between the main thread and worker scripts so
        // that others can trivially plug in their elements.
        QQmlListModel *lm = qobject_cast<QQmlListModel *>(qobjectWrapper->object);
        if (lm && lm->agent()) {
            QQmlListModelWorkerAgent *agent = lm->agent();
            agent->addref();
            push(data, valueheader(WorkerListModel));
            push(data, (void *)agent);
            return;
        }
        // No other QObject's are allowed to be sent
        push(data, valueheader(WorkerUndefined));
    } else if (QV4::Object *o = v.asObject()) {

        if (o->isListType()) {
            // valid sequence.  we generate a length (sequence length + 1 for the sequence type)
            uint32_t seqLength = o->get(v4->id_length).toUInt32();
            uint32_t length = seqLength + 1;
            if (length > 0xFFFFFF) {
                push(data, valueheader(WorkerUndefined));
                return;
            }
            reserve(data, sizeof(quint32) + length * sizeof(quint32));
            push(data, valueheader(WorkerSequence, length));
            serialize(data, QV4::Value::fromInt32(QV4::SequencePrototype::metaTypeForSequence(o)), engine); // sequence type
            for (uint32_t ii = 0; ii < seqLength; ++ii)
                serialize(data, o->getIndexed(ii), engine); // sequence elements

            return;
        }

        v8::Handle<v8::Object> v8object(v);
        QV8ObjectResource *r = static_cast<QV8ObjectResource *>(v8object->GetExternalResource());
        if (r) {
            // not a sequence.
            push(data, valueheader(WorkerUndefined));
            return;
        }

        // regular object
        QV4::ArrayObject *properties = QV4::ObjectPrototype::getOwnPropertyNames(v4, v);
        quint32 length = properties->arrayLength();
        if (length > 0xFFFFFF) {
            push(data, valueheader(WorkerUndefined));
            return;
        }
        push(data, valueheader(WorkerObject, length));

        QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);
        for (quint32 ii = 0; ii < length; ++ii) {
            QV4::String *s = properties->getIndexed(ii).asString();
            serialize(data, QV4::Value::fromString(s), engine);

            bool hasCaught = false;
            QV4::ExecutionContext *ctx = v4->current;
            QV4::Value val = QV4::Value::undefinedValue();
            try {
                val = o->get(s);
            } catch (QV4::Exception &e) {
                e.accept(ctx);
            }

            serialize(data, val, engine);
        }
        return;
    } else {
        push(data, valueheader(WorkerUndefined));
    }
}

QV4::Value Serialize::deserialize(const char *&data, QV8Engine *engine)
{
    quint32 header = popUint32(data);
    Type type = headertype(header);

    QV4::ExecutionEngine *v4 = QV8Engine::getV4(engine);

    switch (type) {
    case WorkerUndefined:
        return QV4::Value::undefinedValue();
    case WorkerNull:
        return QV4::Value::nullValue();
    case WorkerTrue:
        return QV4::Value::fromBoolean(true);
    case WorkerFalse:
        return QV4::Value::fromBoolean(false);
    case WorkerString:
    {
        quint32 size = headersize(header);
        QString qstr((QChar *)data, size);
        data += ALIGN(size * sizeof(uint16_t));
        return QV4::Value::fromString(v4->newString(qstr));
    }
    case WorkerFunction:
        Q_ASSERT(!"Unreachable");
        break;
    case WorkerArray:
    {
        quint32 size = headersize(header);
        QV4::ArrayObject *a = v4->newArrayObject();
        for (quint32 ii = 0; ii < size; ++ii) {
            a->putIndexed(ii, deserialize(data, engine));
        }
        return QV4::Value::fromObject(a);
    }
    case WorkerObject:
    {
        quint32 size = headersize(header);
        QV4::Object *o = v4->newObject();
        for (quint32 ii = 0; ii < size; ++ii) {
            QV4::Value name = deserialize(data, engine);
            QV4::Value value = deserialize(data, engine);
            o->put(name.asString(), value);
        }
        return QV4::Value::fromObject(o);
    }
    case WorkerInt32:
        return QV4::Value::fromInt32((qint32)popUint32(data));
    case WorkerUint32:
        return QV4::Value::fromUInt32(popUint32(data));
    case WorkerNumber:
        return QV4::Value::fromDouble(popDouble(data));
    case WorkerDate:
        return QV4::Value::fromObject(v4->newDateObject(QV4::Value::fromDouble(popDouble(data))));
    case WorkerRegexp:
    {
        quint32 flags = headersize(header);
        quint32 length = popUint32(data);
        QString pattern = QString((QChar *)data, length - 1);
        data += ALIGN(length * sizeof(uint16_t));
        return QV4::Value::fromObject(v4->newRegExpObject(pattern, flags));
    }
    case WorkerListModel:
    {
        void *ptr = popPtr(data);
        QQmlListModelWorkerAgent *agent = (QQmlListModelWorkerAgent *)ptr;
        QV4::Value rv = engine->newQObject(agent);
        // ### Find a better solution then the ugly property
        QQmlListModelWorkerAgent::VariantRef ref(agent);
        QVariant var = qVariantFromValue(ref);
        rv.asObject()->defineReadonlyProperty(v4->newString("__qml:hidden:ref"), engine->fromVariant(var));

        agent->release();
        agent->setV8Engine(engine);
        return rv;
    }
    case WorkerSequence:
    {
        bool succeeded = false;
        quint32 length = headersize(header);
        quint32 seqLength = length - 1;
        int sequenceType = deserialize(data, engine).integerValue();
        QV4::ArrayObject *array = v4->newArrayObject();
        array->arrayReserve(seqLength);
        for (quint32 ii = 0; ii < seqLength; ++ii)
            array->arrayData[ii].value = deserialize(data, engine);
        array->arrayDataLen = seqLength;
        array->setArrayLengthUnchecked(seqLength);
        QVariant seqVariant = QV4::SequencePrototype::toVariant(QV4::Value::fromObject(array), sequenceType, &succeeded);
        return QV4::SequencePrototype::fromVariant(v4, seqVariant, &succeeded);
    }
    }
    Q_ASSERT(!"Unreachable");
    return QV4::Value::undefinedValue();
}

QByteArray Serialize::serialize(const QV4::Value &value, QV8Engine *engine)
{
    QByteArray rv;
    serialize(rv, value, engine);
    return rv;
}

QV4::Value Serialize::deserialize(const QByteArray &data, QV8Engine *engine)
{
    const char *stream = data.constData();
    return deserialize(stream, engine);
}

QT_END_NAMESPACE

