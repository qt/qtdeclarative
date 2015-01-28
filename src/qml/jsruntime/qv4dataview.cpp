/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4dataview_p.h"
#include "qv4arraybuffer_p.h"

#include "qendian.h"

using namespace QV4;

DEFINE_OBJECT_VTABLE(DataViewCtor);
DEFINE_OBJECT_VTABLE(DataView);

Heap::DataViewCtor::DataViewCtor(QV4::ExecutionContext *scope)
    : Heap::FunctionObject(scope, QStringLiteral("DataView"))
{
}

ReturnedValue DataViewCtor::construct(Managed *m, CallData *callData)
{
    Scope scope(static_cast<Object *>(m)->engine());
    Scoped<ArrayBuffer> buffer(scope, callData->argument(0));
    if (!buffer)
        return scope.engine->throwTypeError();

    double bo = callData->argc > 1 ? callData->args[1].toNumber() : 0;
    uint byteOffset = (uint)bo;
    uint bufferLength = buffer->d()->data->size;
    double bl = callData->argc < 3 || callData->args[2].isUndefined() ? (bufferLength - bo) : callData->args[2].toNumber();
    uint byteLength = (uint)bl;
    if (bo != byteOffset || bl != byteLength || byteOffset + byteLength > bufferLength)
        return scope.engine->throwRangeError(QStringLiteral("DataView: constructor arguments out of range"));

    Scoped<DataView> a(scope, scope.engine->memoryManager->alloc<DataView>(scope.engine));
    a->d()->buffer = buffer->d();
    a->d()->byteLength = byteLength;
    a->d()->byteOffset = byteOffset;
    return a.asReturnedValue();

}

ReturnedValue DataViewCtor::call(Managed *that, CallData *callData)
{
    return construct(that, callData);
}


Heap::DataView::DataView(ExecutionEngine *e)
    : Heap::Object(e->emptyClass, e->dataViewPrototype.asObject()),
      buffer(0),
      byteLength(0),
      byteOffset(0)
{
}


void DataView::markObjects(Heap::Base *that, ExecutionEngine *e)
{
    DataView::Data *v = static_cast<DataView::Data *>(that);
    v->buffer->mark(e);
}

void DataViewPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedObject o(scope);
    ctor->defineReadonlyProperty(engine->id_length, Primitive::fromInt32(3));
    ctor->defineReadonlyProperty(engine->id_prototype, (o = this));
    defineDefaultProperty(engine->id_constructor, (o = ctor));
    defineAccessorProperty(QStringLiteral("buffer"), method_get_buffer, 0);
    defineAccessorProperty(QStringLiteral("byteLength"), method_get_byteLength, 0);
    defineAccessorProperty(QStringLiteral("byteOffset"), method_get_byteOffset, 0);

    defineDefaultProperty(QStringLiteral("getInt8"), method_getChar<signed char>, 0);
    defineDefaultProperty(QStringLiteral("getUInt8"), method_getChar<unsigned char>, 0);
    defineDefaultProperty(QStringLiteral("getInt16"), method_get<short>, 0);
    defineDefaultProperty(QStringLiteral("getUInt16"), method_get<unsigned short>, 0);
    defineDefaultProperty(QStringLiteral("getInt32"), method_get<int>, 0);
    defineDefaultProperty(QStringLiteral("getUInt32"), method_get<unsigned int>, 0);
    defineDefaultProperty(QStringLiteral("getFloat32"), method_getFloat<float>, 0);
    defineDefaultProperty(QStringLiteral("getFloat64"), method_getFloat<double>, 0);

    defineDefaultProperty(QStringLiteral("setInt8"), method_setChar<signed char>, 0);
    defineDefaultProperty(QStringLiteral("setUInt8"), method_setChar<unsigned char>, 0);
    defineDefaultProperty(QStringLiteral("setInt16"), method_set<short>, 0);
    defineDefaultProperty(QStringLiteral("setUInt16"), method_set<unsigned short>, 0);
    defineDefaultProperty(QStringLiteral("setInt32"), method_set<int>, 0);
    defineDefaultProperty(QStringLiteral("setUInt32"), method_set<unsigned int>, 0);
    defineDefaultProperty(QStringLiteral("setFloat32"), method_setFloat<float>, 0);
    defineDefaultProperty(QStringLiteral("setFloat64"), method_setFloat<double>, 0);
}

ReturnedValue DataViewPrototype::method_get_buffer(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<DataView> v(scope, ctx->thisObject());
    if (!v)
        return scope.engine->throwTypeError();

    return Encode(v->d()->buffer->asReturnedValue());
}

ReturnedValue DataViewPrototype::method_get_byteLength(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<DataView> v(scope, ctx->thisObject());
    if (!v)
        return scope.engine->throwTypeError();

    return Encode(v->d()->byteLength);
}

ReturnedValue DataViewPrototype::method_get_byteOffset(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<DataView> v(scope, ctx->thisObject());
    if (!v)
        return scope.engine->throwTypeError();

    return Encode(v->d()->byteOffset);
}

template <typename T>
ReturnedValue DataViewPrototype::method_getChar(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<DataView> v(scope, ctx->thisObject());
    if (!v || ctx->argc() < 1)
        return scope.engine->throwTypeError();
    double l = ctx->args()[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return scope.engine->throwTypeError();
    idx += v->d()->byteOffset;

    T t = T(v->d()->buffer->data->data()[idx]);

    return Encode((int)t);
}

template <typename T>
ReturnedValue DataViewPrototype::method_get(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<DataView> v(scope, ctx->thisObject());
    if (!v || ctx->argc() < 1)
        return scope.engine->throwTypeError();
    double l = ctx->args()[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return scope.engine->throwTypeError();
    idx += v->d()->byteOffset;

    bool littleEndian = ctx->argc() < 2 ? false : ctx->args()[1].toBoolean();

    T t = littleEndian
            ? qFromLittleEndian<T>((uchar *)v->d()->buffer->data->data() + idx)
            : qFromBigEndian<T>((uchar *)v->d()->buffer->data->data() + idx);

    return Encode(t);
}

template <typename T>
ReturnedValue DataViewPrototype::method_getFloat(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<DataView> v(scope, ctx->thisObject());
    if (!v || ctx->argc() < 1)
        return scope.engine->throwTypeError();
    double l = ctx->args()[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return scope.engine->throwTypeError();
    idx += v->d()->byteOffset;

    bool littleEndian = ctx->argc() < 2 ? false : ctx->args()[1].toBoolean();

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
ReturnedValue DataViewPrototype::method_setChar(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<DataView> v(scope, ctx->thisObject());
    if (!v || ctx->argc() < 1)
        return scope.engine->throwTypeError();
    double l = ctx->args()[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return scope.engine->throwTypeError();
    idx += v->d()->byteOffset;

    int val = ctx->argc() >= 2 ? ctx->args()[1].toInt32() : 0;
    v->d()->buffer->data->data()[idx] = (char)val;

    return Encode::undefined();
}

template <typename T>
ReturnedValue DataViewPrototype::method_set(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<DataView> v(scope, ctx->thisObject());
    if (!v || ctx->argc() < 1)
        return scope.engine->throwTypeError();
    double l = ctx->args()[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return scope.engine->throwTypeError();
    idx += v->d()->byteOffset;

    int val = ctx->argc() >= 2 ? ctx->args()[1].toInt32() : 0;

    bool littleEndian = ctx->argc() < 3 ? false : ctx->args()[2].toBoolean();

    if (littleEndian)
        qToLittleEndian<T>(val, (uchar *)v->d()->buffer->data->data() + idx);
    else
        qToBigEndian<T>(val, (uchar *)v->d()->buffer->data->data() + idx);

    return Encode::undefined();
}

template <typename T>
ReturnedValue DataViewPrototype::method_setFloat(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<DataView> v(scope, ctx->thisObject());
    if (!v || ctx->argc() < 1)
        return scope.engine->throwTypeError();
    double l = ctx->args()[0].toNumber();
    uint idx = (uint)l;
    if (l != idx || idx + sizeof(T) > v->d()->byteLength)
        return scope.engine->throwTypeError();
    idx += v->d()->byteOffset;

    double val = ctx->argc() >= 2 ? ctx->args()[1].toNumber() : qSNaN();
    bool littleEndian = ctx->argc() < 3 ? false : ctx->args()[2].toBoolean();

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
    return Encode::undefined();
}
