// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
#include "qv4arraybuffer_p.h"
#include "qv4typedarray_p.h"
#include "qv4dataview_p.h"
#include "qv4symbol_p.h"

using namespace QV4;

DEFINE_OBJECT_VTABLE(SharedArrayBufferCtor);
DEFINE_OBJECT_VTABLE(ArrayBufferCtor);
DEFINE_OBJECT_VTABLE(SharedArrayBuffer);
DEFINE_OBJECT_VTABLE(ArrayBuffer);

void Heap::SharedArrayBufferCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("SharedArrayBuffer"));
}

void Heap::ArrayBufferCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("ArrayBuffer"));
}

ReturnedValue SharedArrayBufferCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    Scope scope(f);
    if (newTarget->isUndefined())
        return scope.engine->throwTypeError();

    const double len = argc ? argv[0].toInteger() : 0;
    if (scope.hasException())
        return Encode::undefined();
    if (len < 0 || len >= std::numeric_limits<int>::max())
        return scope.engine->throwRangeError(QStringLiteral("SharedArrayBuffer: Invalid length."));

    Scoped<SharedArrayBuffer> a(
                scope, scope.engine->memoryManager->allocate<SharedArrayBuffer>(size_t(len)));
    if (scope.hasException())
        return Encode::undefined();

    return a->asReturnedValue();
}

ReturnedValue SharedArrayBufferCtor::virtualCall(const FunctionObject *f, const Value *, const Value *, int)
{
    return f->engine()->throwTypeError();
}


ReturnedValue ArrayBufferCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *newTarget)
{
    ExecutionEngine *v4 = f->engine();
    Scope scope(v4);

    ScopedValue l(scope, argc ? argv[0] : Value::undefinedValue());
    double dl = l->toInteger();
    if (v4->hasException)
        return Encode::undefined();
    uint len = (uint)qBound(0., dl, (double)UINT_MAX);
    if (len != dl)
        return v4->throwRangeError(QLatin1String("ArrayBuffer constructor: invalid length"));

    Scoped<ArrayBuffer> a(scope, v4->newArrayBuffer(len));
    if (newTarget->heapObject() != f->heapObject() && newTarget->isFunctionObject()) {
        const FunctionObject *nt = static_cast<const FunctionObject *>(newTarget);
        ScopedObject o(scope, nt->protoProperty());
        if (o)
            a->setPrototypeOf(o);
    }
    if (scope.hasException())
        return Encode::undefined();

    return a->asReturnedValue();
}

ReturnedValue ArrayBufferCtor::method_isView(const FunctionObject *, const Value *, const Value *argv, int argc)
{
    if (argc < 1)
        return Encode(false);

    if (argv[0].as<TypedArray>() ||
        argv[0].as<DataView>())
        return Encode(true);

    return Encode(false);
}


void Heap::SharedArrayBuffer::init(size_t length)
{
    Object::init();
    QPair<QTypedArrayData<char> *, char *> pair;
    if (length < UINT_MAX)
        pair =  QTypedArrayData<char>::allocate(length + 1);
    if (!pair.first) {
        new (&arrayDataPointerStorage) QArrayDataPointer<char>();
        internalClass->engine->throwRangeError(QStringLiteral("ArrayBuffer: out of memory"));
        return;
    }
    auto data = new (&arrayDataPointerStorage) QArrayDataPointer<char>{
            pair.first, pair.second, qsizetype(length) };

    // can't use appendInitialize() because we want to set the terminating '\0'
    memset(data->data(), 0, length + 1);
    isShared = true;
}

void Heap::SharedArrayBuffer::init(const QByteArray& array)
{
    Object::init();
    new (&arrayDataPointerStorage) QArrayDataPointer<char>(*const_cast<QByteArray &>(array).data_ptr());
    isShared = true;
}

void Heap::SharedArrayBuffer::destroy()
{
    arrayDataPointer().~QArrayDataPointer();
    Object::destroy();
}

QByteArray ArrayBuffer::asByteArray() const
{
    return QByteArray(constArrayData(), arrayDataLength());
}

void ArrayBuffer::detach()
{
    detachArrayData();
}


void SharedArrayBufferPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedObject o(scope);
    ctor->defineReadonlyConfigurableProperty(engine->id_length(), Value::fromInt32(1));
    ctor->defineReadonlyProperty(engine->id_prototype(), (o = this));
    ctor->addSymbolSpecies();

    defineDefaultProperty(engine->id_constructor(), (o = ctor));
    defineAccessorProperty(QStringLiteral("byteLength"), method_get_byteLength, nullptr);
    defineDefaultProperty(QStringLiteral("slice"), method_slice, 2);
    ScopedString name(scope, engine->newString(QStringLiteral("SharedArrayBuffer")));
    defineReadonlyConfigurableProperty(scope.engine->symbol_toStringTag(), name);
}

ReturnedValue SharedArrayBufferPrototype::method_get_byteLength(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    const SharedArrayBuffer *a = thisObject->as<SharedArrayBuffer>();
    if (!a || a->hasDetachedArrayData() || !a->isSharedArrayBuffer())
        return b->engine()->throwTypeError();

    return Encode(a->arrayDataLength());
}

ReturnedValue SharedArrayBufferPrototype::method_slice(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    return slice(b, thisObject, argv, argc, true);
}

ReturnedValue SharedArrayBufferPrototype::slice(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc, bool shared)
{
    Scope scope(b);
    const SharedArrayBuffer *a = thisObject->as<SharedArrayBuffer>();
    if (!a || a->hasDetachedArrayData() || (a->isSharedArrayBuffer() != shared))
        return scope.engine->throwTypeError();

    const uint aDataLength = a->arrayDataLength();

    double start = argc > 0 ? argv[0].toInteger() : 0;
    double end = (argc < 2 || argv[1].isUndefined()) ? aDataLength : argv[1].toInteger();
    if (scope.hasException())
        return QV4::Encode::undefined();

    double first = (start < 0) ? qMax(aDataLength + start, 0.) : qMin(start, double(aDataLength));
    double final = (end < 0) ? qMax(aDataLength + end, 0.) : qMin(end, double(aDataLength));

    const FunctionObject *constructor = a->speciesConstructor(scope, shared ? scope.engine->sharedArrayBufferCtor() : scope.engine->arrayBufferCtor());
    if (!constructor)
        return scope.engine->throwTypeError();

    double newLen = qMax(final - first, 0.);
    ScopedValue argument(scope, QV4::Encode(newLen));
    QV4::Scoped<SharedArrayBuffer> newBuffer(scope, constructor->callAsConstructor(argument, 1));
    if (!newBuffer || newBuffer->arrayDataLength() < newLen ||
        newBuffer->hasDetachedArrayData() || (newBuffer->isSharedArrayBuffer() != shared) ||
        newBuffer->sameValue(*a) ||
        a->hasDetachedArrayData())
        return scope.engine->throwTypeError();

    memcpy(newBuffer->arrayData(), a->constArrayData() + (uint)first, newLen);
    return newBuffer->asReturnedValue();
}


void ArrayBufferPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedObject o(scope);
    ctor->defineReadonlyConfigurableProperty(engine->id_length(), Value::fromInt32(1));
    ctor->defineReadonlyProperty(engine->id_prototype(), (o = this));
    ctor->defineDefaultProperty(QStringLiteral("isView"), ArrayBufferCtor::method_isView, 1);
    ctor->addSymbolSpecies();

    defineDefaultProperty(engine->id_constructor(), (o = ctor));
    defineAccessorProperty(QStringLiteral("byteLength"), method_get_byteLength, nullptr);
    defineDefaultProperty(QStringLiteral("slice"), method_slice, 2);
    defineDefaultProperty(QStringLiteral("toString"), method_toString, 0);
    ScopedString name(scope, engine->newString(QStringLiteral("ArrayBuffer")));
    defineReadonlyConfigurableProperty(scope.engine->symbol_toStringTag(), name);
}

ReturnedValue ArrayBufferPrototype::method_get_byteLength(const FunctionObject *f, const Value *thisObject, const Value *, int)
{
    const ArrayBuffer *a = thisObject->as<ArrayBuffer>();
    if (!a || a->isSharedArrayBuffer())
        return f->engine()->throwTypeError();

    if (a->hasDetachedArrayData())
        return Encode(0);

    return Encode(a->arrayDataLength());
}

ReturnedValue ArrayBufferPrototype::method_slice(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    return slice(b, thisObject, argv, argc, false);
}

ReturnedValue ArrayBufferPrototype::method_toString(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    const ArrayBuffer *a = thisObject->as<ArrayBuffer>();
    if (!a)
        RETURN_UNDEFINED();
    return Encode(v4->newString(QString::fromUtf8(a->asByteArray())));
}
