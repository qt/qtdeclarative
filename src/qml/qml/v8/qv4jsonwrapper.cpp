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

#include "qv4jsonwrapper_p.h"
#include "private/qv4engine_p.h"
#include "private/qv4object_p.h"
#include "private/qv4objectiterator_p.h"

QT_BEGIN_NAMESPACE

using namespace QV4;

QV4JsonWrapper::QV4JsonWrapper()
: m_engine(0)
{
}

QV4JsonWrapper::~QV4JsonWrapper()
{
}

void QV4JsonWrapper::init(QV4::ExecutionEngine *engine)
{
    m_engine = engine;
}

void QV4JsonWrapper::destroy()
{
}

QV4::Value QV4JsonWrapper::fromJsonValue(const QJsonValue &value)
{
    if (value.isString())
        return Value::fromString(m_engine->current, value.toString());
    else if (value.isDouble())
        return Value::fromDouble(value.toDouble());
    else if (value.isBool())
        return Value::fromBoolean(value.toBool());
    else if (value.isArray())
        return fromJsonArray(value.toArray());
    else if (value.isObject())
        return fromJsonObject(value.toObject());
    else if (value.isNull())
        return Value::nullValue();
    else
        return Value::undefinedValue();
}

QJsonValue QV4JsonWrapper::toJsonValue(const QV4::Value &value,
                                       V4ObjectSet &visitedObjects)
{
    if (String *s = value.asString())
        return QJsonValue(s->toQString());
    else if (value.isNumber())
        return QJsonValue(value.toNumber());
    else if (value.isBoolean())
        return QJsonValue((bool)value.booleanValue());
    else if (ArrayObject *a = value.asArrayObject())
        return toJsonArray(a, visitedObjects);
    else if (Object *o = value.asObject())
        return toJsonObject(o, visitedObjects);
    else if (value.isNull())
        return QJsonValue(QJsonValue::Null);
    else
        return QJsonValue(QJsonValue::Undefined);
}

QV4::Value QV4JsonWrapper::fromJsonObject(const QJsonObject &object)
{
    Object *o = m_engine->newObject();
    for (QJsonObject::const_iterator it = object.begin(); it != object.end(); ++it)
        o->put(m_engine->current, m_engine->newString(it.key()), fromJsonValue(it.value()));
    return Value::fromObject(o);
}

QJsonObject QV4JsonWrapper::toJsonObject(QV4::Object *o, V4ObjectSet &visitedObjects)
{
    QJsonObject result;
    if (!o || o->asFunctionObject())
        return result;

    if (visitedObjects.contains(o)) {
        // Avoid recursion.
        // For compatibility with QVariant{List,Map} conversion, we return an
        // empty object (and no error is thrown).
        return result;
    }

    visitedObjects.insert(o);

    ObjectIterator it(o, ObjectIterator::EnumberableOnly);
    while (1) {
        PropertyAttributes attributes;
        String *name;
        uint idx;
        Property *p = it.next(&name, &idx, &attributes);
        if (!p)
            break;

        Value v = o->getValue(m_engine->current, p, attributes);
        QString key = name ? name->toQString() : QString::number(idx);
        result.insert(key, toJsonValue(v, visitedObjects));
    }

    visitedObjects.remove(o);

    return result;
}

QV4::Value QV4JsonWrapper::fromJsonArray(const QJsonArray &array)
{
    int size = array.size();
    ArrayObject *a = m_engine->newArrayObject();
    a->arrayReserve(size);
    for (int i = 0; i < size; i++)
        a->arrayData[i].value = fromJsonValue(array.at(i));
    a->setArrayLengthUnchecked(size);
    return Value::fromObject(a);
}

QJsonArray QV4JsonWrapper::toJsonArray(ArrayObject *a, V4ObjectSet &visitedObjects)
{
    QJsonArray result;
    if (!a)
        return result;

    if (visitedObjects.contains(a)) {
        // Avoid recursion.
        // For compatibility with QVariant{List,Map} conversion, we return an
        // empty array (and no error is thrown).
        return result;
    }

    visitedObjects.insert(a);

    quint32 length = a->arrayLength();
    for (quint32 i = 0; i < length; ++i) {
        Value v = a->getIndexed(m_engine->current, i);
        result.append(toJsonValue(v, visitedObjects));
    }

    visitedObjects.remove(a);

    return result;
}

QT_END_NAMESPACE
