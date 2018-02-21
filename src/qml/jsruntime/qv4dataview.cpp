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

#include "qv4dataview_p.h"
#include "qv4arraybuffer_p.h"
#include "qv4string_p.h"

#include <QtCore/private/qnumeric_p.h>
#include "qendian.h"

using namespace QV4;

DEFINE_OBJECT_VTABLE(DataViewCtor);
DEFINE_OBJECT_VTABLE(DataView);

void Heap::DataViewCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("DataView"));
}

ReturnedValue DataViewCtor::callAsConstructor(const FunctionObject *f, const Value *argv, int argc)
{
    Scope scope(f->engine());
    Scoped<ArrayBuffer> buffer(scope, argc ? argv[0] : Primitive::undefinedValue());
    if (!buffer)
        return scope.engine->throwTypeError();

    double bo = argc > 1 ? argv[1].toNumber() : 0;
    uint byteOffset = (uint)bo;
    uint bufferLength = buffer->d()->data->size;
    double bl = argc < 3 || argv[2].isUndefined() ? (bufferLength - bo) : argv[2].toNumber();
    uint byteLength = (uint)bl;
    if (bo != byteOffset || bl != byteLength || byteOffset + byteLength > bufferLength)
        return scope.engine->throwRangeError(QStringLiteral("DataView: constructor arguments out of range"));

    Scoped<DataView> a(scope, scope.engine->memoryManager->allocObject<DataView>());
    a->d()->buffer.set(scope.engine, buffer->d());
    a->d()->byteLength = byteLength;
    a->d()->byteOffset = byteOffset;
    return a.asReturnedValue();
}

ReturnedValue DataViewCtor::call(const FunctionObject *f, const Value *, const Value *argv, int argc)
{
    return callAsConstructor(f, argv, argc);
}

void DataViewPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedObject o(scope);
    ctor->defineReadonlyProperty(engine->id_length(), Primitive::fromInt32(3));
    ctor->defineReadonlyProperty(engine->id_prototype(), (o = this));
    defineDefaultProperty(engine->id_constructor(), (o = ctor));
    defineAccessorProperty(QStringLiteral("buffer"), method_get_buffer, nullptr);
    defineAccessorProperty(QStringLiteral("byteLength"), method_get_byteLength, nullptr);
    defineAccessorProperty(QStringLiteral("byteOffset"), method_get_byteOffset, nullptr);

    defineDefaultProperty(QStringLiteral("getInt8"), method_getChar<signed char>, 0);
    defineDefaultProperty(QStringLiteral("getUint8"), method_getChar<unsigned char>, 0);
    defineDefaultProperty(QStringLiteral("getInt16"), method_get<short>, 0);
    defineDefaultProperty(QStringLiteral("getUint16"), method_get<unsigned short>, 0);
    defineDefaultProperty(QStringLiteral("getInt32"), method_get<int>, 0);
    defineDefaultProperty(QStringLiteral("getUint32"), method_get<unsigned int>, 0);
    defineDefaultProperty(QStringLiteral("getFloat32"), method_getFloat<float>, 0);
    defineDefaultProperty(QStringLiteral("getFloat64"), method_getFloat<double>, 0);

    defineDefaultProperty(QStringLiteral("setInt8"), method_setChar<signed char>, 0);
    defineDefaultProperty(QStringLiteral("setUint8"), method_setChar<unsigned char>, 0);
    defineDefaultProperty(QStringLiteral("setInt16"), method_set<short>, 0);
    defineDefaultProperty(QStringLiteral("setUint16"), method_set<unsigned short>, 0);
    defineDefaultProperty(QStringLiteral("setInt32"), method_set<int>, 0);
    defineDefaultProperty(QStringLiteral("setUint32"), method_set<unsigned int>, 0);
    defineDefaultProperty(QStringLiteral("setFloat32"), method_setFloat<float>, 0);
    defineDefaultProperty(QStringLiteral("setFloat64"), method_setFloat<double>, 0);

    // For backword compatibility
    defineDefaultProperty(QStringLiteral("getUInt8"), method_getChar<unsigned char>, 0);
    defineDefaultProperty(QStringLiteral("getUInt16"), method_get<unsigned short>, 0);
    defineDefaultProperty(QStringLiteral("getUInt32"), method_get<unsigned int>, 0);
    defineDefaultProperty(QStringLiteral("setUInt8"), method_setChar<unsigned char>, 0);
    defineDefaultProperty(QStringLiteral("setUInt16"), method_set<unsigned short>, 0);
    defineDefaultProperty(QStringLiteral("setUInt32"), method_set<unsigned int>, 0);
}

ReturnedValue DataViewPrototype::method_get_buffer(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    const DataView *v = thisObject->as<DataView>();
    if (!v)
        return b->engine()->throwTypeError();

    return v->d()->buffer->asReturnedValue();
}

ReturnedValue DataViewPrototype::method_get_byteLength(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    const DataView *v = thisObject->as<DataView>();
    if (!v)
        return b->engine()->throwTypeError();

    return Encode(v->d()->byteLength);
}

ReturnedValue DataViewPrototype::method_get_byteOffset(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    const DataView *v = thisObject->as<DataView>();
    if (!v)
        return b->engine()->throwTypeError();

    return Encode(v->d()->byteOffset);
}

template <typename T>
ReturnedValue DataViewPrototype::method_getChar(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    const DataView *v = thisObject->as<DataView>();
    if (!v || argc < 1)
        return b->engine()->throwTypeError();
    double l = argv[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return b->engine()->throwTypeError();
    idx += v->d()->byteOffset;

    T t = T(v->d()->buffer->data->data()[idx]);

    return Encode((int)t);
}

template <typename T>
ReturnedValue DataViewPrototype::method_get(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    const DataView *v = thisObject->as<DataView>();
    if (!v || argc < 1)
        return b->engine()->throwTypeError();
    double l = argv[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return b->engine()->throwTypeError();
    idx += v->d()->byteOffset;

    bool littleEndian = argc < 2 ? false : argv[1].toBoolean();

    T t = littleEndian
            ? qFromLittleEndian<T>((uchar *)v->d()->buffer->data->data() + idx)
            : qFromBigEndian<T>((uchar *)v->d()->buffer->data->data() + idx);

    return Encode(t);
}

template <typename T>
ReturnedValue DataViewPrototype::method_getFloat(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    const DataView *v = thisObject->as<DataView>();
    if (!v || argc < 1)
        return b->engine()->throwTypeError();
    double l = argv[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return b->engine()->throwTypeError();
    idx += v->d()->byteOffset;

    bool littleEndian = argc < 2 ? false : argv[1].toBoolean();

    if (sizeof(T) == 4) {
        // float
        union {
            uint i;
            float f;
        } u;
        u.i = littleEndian
                ? qFromLittleEndian<uint>((uchar *)v->d()->buffer->data->data() + idx)
                : qFromBigEndian<uint>((uchar *)v->d()->buffer->data->data() + idx);
        return Encode(u.f);
    } else {
        Q_ASSERT(sizeof(T) == 8);
        union {
            quint64 i;
            double d;
        } u;
        u.i = littleEndian
                ? qFromLittleEndian<quint64>((uchar *)v->d()->buffer->data->data() + idx)
                : qFromBigEndian<quint64>((uchar *)v->d()->buffer->data->data() + idx);
        return Encode(u.d);
    }
}

template <typename T>
ReturnedValue DataViewPrototype::method_setChar(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    const DataView *v = thisObject->as<DataView>();
    if (!v || argc < 1)
        return b->engine()->throwTypeError();
    double l = argv[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return b->engine()->throwTypeError();
    idx += v->d()->byteOffset;

    int val = argc >= 2 ? argv[1].toInt32() : 0;
    v->d()->buffer->data->data()[idx] = (char)val;

    RETURN_UNDEFINED();
}

template <typename T>
ReturnedValue DataViewPrototype::method_set(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    const DataView *v = thisObject->as<DataView>();
    if (!v || argc < 1)
        return b->engine()->throwTypeError();
    double l = argv[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return b->engine()->throwTypeError();
    idx += v->d()->byteOffset;

    int val = argc >= 2 ? argv[1].toInt32() : 0;

    bool littleEndian = argc < 3 ? false : argv[2].toBoolean();

    if (littleEndian)
        qToLittleEndian<T>(val, (uchar *)v->d()->buffer->data->data() + idx);
    else
        qToBigEndian<T>(val, (uchar *)v->d()->buffer->data->data() + idx);

    RETURN_UNDEFINED();
}

template <typename T>
ReturnedValue DataViewPrototype::method_setFloat(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    const DataView *v = thisObject->as<DataView>();
    if (!v || argc < 1)
        return b->engine()->throwTypeError();
    double l = argv[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return b->engine()->throwTypeError();
    idx += v->d()->byteOffset;

    double val = argc >= 2 ? argv[1].toNumber() : qt_qnan();
    bool littleEndian = argc < 3 ? false : argv[2].toBoolean();

    if (sizeof(T) == 4) {
        // float
        union {
            uint i;
            float f;
        } u;
        u.f = val;
        if (littleEndian)
            qToLittleEndian(u.i, (uchar *)v->d()->buffer->data->data() + idx);
        else
            qToBigEndian(u.i, (uchar *)v->d()->buffer->data->data() + idx);
    } else {
        Q_ASSERT(sizeof(T) == 8);
        union {
            quint64 i;
            double d;
        } u;
        u.d = val;
        if (littleEndian)
            qToLittleEndian(u.i, (uchar *)v->d()->buffer->data->data() + idx);
        else
            qToBigEndian(u.i, (uchar *)v->d()->buffer->data->data() + idx);
    }
    RETURN_UNDEFINED();
}
