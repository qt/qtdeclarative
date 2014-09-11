/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
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
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
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

ArrayBufferCtor::Data::Data(ExecutionContext *scope)
    : FunctionObject::Data(scope, QStringLiteral("ArrayBuffer"))
{
    setVTable(staticVTable());
}

ReturnedValue ArrayBufferCtor::construct(Managed *m, CallData *callData)
{
    ExecutionEngine *v4 = m->engine();

    Scope scope(v4);
    ScopedValue l(scope, callData->argument(0));
    double dl = l->toInteger();
    if (v4->hasException)
        return Encode::undefined();
    uint len = (uint)qBound(0., dl, (double)UINT_MAX);
    if (len != dl)
        return v4->currentContext()->throwRangeError(QLatin1String("ArrayBuffer constructor: invalid length"));

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
    return Encode(true);
}


ArrayBuffer::Data::Data(ExecutionEngine *e, int length)
    : Object::Data(e->arrayBufferClass)
{
    data = QTypedArrayData<char>::allocate(length + 1);
    if (!data) {
        data = 0;
        e->currentContext()->throwRangeError(QStringLiteral("ArrayBuffer: out of memory"));
        return;
    }
    data->size = length;
    memset(data->data(), 0, length + 1);
}

QByteArray ArrayBuffer::asByteArray() const
{
    QByteArrayDataPtr ba = { d()->data };
    ba.ptr->ref.ref();
    return QByteArray(ba);
}

void ArrayBuffer::destroy(Managed *m)
{
    ArrayBuffer *b = static_cast<ArrayBuffer *>(m);
    if (!b->d()->data->ref.deref())
        QTypedArrayData<char>::deallocate(b->d()->data);
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
    Scoped<ArrayBuffer> v(scope, ctx->d()->callData->thisObject);
    if (!v)
        return ctx->throwTypeError();

    return Encode(v->d()->data->size);
}

ReturnedValue ArrayBufferPrototype::method_slice(CallContext *ctx)
{
    Scope scope(ctx);
    Scoped<ArrayBuffer> a(scope, ctx->d()->callData->thisObject);
    if (!a)
        return ctx->throwTypeError();

    double start = ctx->d()->callData->argc > 0 ? ctx->d()->callData->args[0].toInteger() : 0;
    double end = (ctx->d()->callData->argc < 2 || ctx->d()->callData->args[1].isUndefined()) ?
                a->d()->data->size : ctx->d()->callData->args[1].toInteger();
    if (scope.engine->hasException)
        return Encode::undefined();

    double first = (start < 0) ? qMax(a->d()->data->size + start, 0.) : qMin(start, (double)a->d()->data->size);
    double final = (end < 0) ? qMax(a->d()->data->size + end, 0.) : qMin(end, (double)a->d()->data->size);

    Scoped<FunctionObject> constructor(scope, a->get(scope.engine->id_constructor));
    if (!constructor)
        return ctx->throwTypeError();

    ScopedCallData callData(scope, 1);
    double newLen = qMax(final - first, 0.);
    callData->args[0] = QV4::Encode(newLen);
    QV4::Scoped<ArrayBuffer> newBuffer(scope, constructor->construct(callData));
    if (!newBuffer || newBuffer->d()->data->size < (int)newLen)
        return scope.engine->currentContext()->throwTypeError();

    memcpy(newBuffer->d()->data->data(), a->d()->data->data() + (uint)first, newLen);

    return newBuffer.asReturnedValue();
}
