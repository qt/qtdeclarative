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
#include "qv4arraybuffer_p.h"
#include "qv4typedarray_p.h"
#include "qv4dataview_p.h"

using namespace QV4;

DEFINE_OBJECT_VTABLE(ArrayBufferCtor);
DEFINE_OBJECT_VTABLE(ArrayBuffer);

Heap::ArrayBufferCtor::ArrayBufferCtor(QV4::ExecutionContext *scope)
    : Heap::FunctionObject(scope, QStringLiteral("ArrayBuffer"))
{
}

ReturnedValue ArrayBufferCtor::construct(Managed *m, CallData *callData)
{
    ExecutionEngine *v4 = static_cast<Object *>(m)->engine();

    Scope scope(v4);
    ScopedValue l(scope, callData->argument(0));
    double dl = l->toInteger();
    if (v4->hasException)
        return Encode::undefined();
    uint len = (uint)qBound(0., dl, (double)UINT_MAX);
    if (len != dl)
        return v4->throwRangeError(QLatin1String("ArrayBuffer constructor: invalid length"));

    Scoped<ArrayBuffer> a(scope, v4->memoryManager->alloc<ArrayBuffer>(v4, len));
    if (scope.engine->hasException)
        return Encode::undefined();
    return a.asReturnedValue();
}


ReturnedValue ArrayBufferCtor::call(Managed *that, CallData *callData)
{
    return construct(that, callData);
}

ReturnedValue ArrayBufferCtor::method_isView(CallContext *ctx)
{
    QV4::Scope scope(ctx);
    QV4::Scoped<TypedArray> a(scope, ctx->argument(0));
    if (!!a)
        return Encode(true);
    QV4::Scoped<DataView> v(scope, ctx->argument(0));
    if (!!v)
        return Encode(true);
    return Encode(false);
}


Heap::ArrayBuffer::ArrayBuffer(ExecutionEngine *e, size_t length)
    : Heap::Object(e->emptyClass, e->arrayBufferPrototype.asObject())
{
    data = QTypedArrayData<char>::allocate(length + 1);
    if (!data) {
        data = 0;
        e->throwRangeError(QStringLiteral("ArrayBuffer: out of memory"));
        return;
    }
    data->size = int(length);
    memset(data->data(), 0, length + 1);
}

Heap::ArrayBuffer::ArrayBuffer(ExecutionEngine *e, const QByteArray& array)
    : Heap::Object(e->emptyClass, e->arrayBufferPrototype.asObject())
    , data(const_cast<QByteArray&>(array).data_ptr())
{
    data->ref.ref();
}

Heap::ArrayBuffer::~ArrayBuffer()
{
    if (!data->ref.deref())
        QTypedArrayData<char>::deallocate(data);
}

QByteArray ArrayBuffer::asByteArray() const
{
    QByteArrayDataPtr ba = { d()->data };
    ba.ptr->ref.ref();
    return QByteArray(ba);
}

void ArrayBuffer::detach() {
    if (!d()->data->ref.isShared())
        return;

    QTypedArrayData<char> *oldData = d()->data;

    d()->data = QTypedArrayData<char>::allocate(oldData->size + 1);
    if (!d()->data) {
        engine()->throwRangeError(QStringLiteral("ArrayBuffer: out of memory"));
        return;
    }

    memcpy(d()->data->data(), oldData->data(), oldData->size + 1);

    if (!oldData->ref.deref())
        QTypedArrayData<char>::deallocate(oldData);
}


void ArrayBufferPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedObject o(scope);
    ctor->defineReadonlyProperty(engine->id_length, Primitive::fromInt32(1));
    ctor->defineReadonlyProperty(engine->id_prototype, (o = this));
    ctor->defineDefaultProperty(QStringLiteral("isView"), ArrayBufferCtor::method_isView, 1);
    defineDefaultProperty(engine->id_constructor, (o = ctor));
    defineAccessorProperty(QStringLiteral("byteLength"), method_get_byteLength, 0);
    defineDefaultProperty(QStringLiteral("slice"), method_slice, 2);
}

ReturnedValue ArrayBufferPrototype::method_get_byteLength(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<ArrayBuffer> v(scope, ctx->thisObject());
    if (!v)
        return scope.engine->throwTypeError();

    return Encode(v->d()->data->size);
}

ReturnedValue ArrayBufferPrototype::method_slice(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<ArrayBuffer> a(scope, ctx->thisObject());
    if (!a)
        return scope.engine->throwTypeError();

    double start = ctx->argc() > 0 ? ctx->args()[0].toInteger() : 0;
    double end = (ctx->argc() < 2 || ctx->args()[1].isUndefined()) ?
                a->d()->data->size : ctx->args()[1].toInteger();
    if (scope.engine->hasException)
        return Encode::undefined();

    double first = (start < 0) ? qMax(a->d()->data->size + start, 0.) : qMin(start, (double)a->d()->data->size);
    double final = (end < 0) ? qMax(a->d()->data->size + end, 0.) : qMin(end, (double)a->d()->data->size);

    ScopedFunctionObject constructor(scope, a->get(scope.engine->id_constructor));
    if (!constructor)
        return scope.engine->throwTypeError();

    ScopedCallData callData(scope, 1);
    double newLen = qMax(final - first, 0.);
    callData->args[0] = QV4::Encode(newLen);
    QV4::Scoped<ArrayBuffer> newBuffer(scope, constructor->construct(callData));
    if (!newBuffer || newBuffer->d()->data->size < (int)newLen)
        return scope.engine->throwTypeError();

    memcpy(newBuffer->d()->data->data(), a->d()->data->data() + (uint)first, newLen);

    return newBuffer.asReturnedValue();
}
