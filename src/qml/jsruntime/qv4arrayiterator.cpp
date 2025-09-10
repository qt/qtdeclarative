// Copyright (C) 2017 Crimson AS <info@crimson.no>
// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qv4iterator_p.h>
#include <private/qv4arrayiterator_p.h>
#include <private/qv4typedarray_p.h>
#include <private/qv4symbol_p.h>

using namespace QV4;

DEFINE_OBJECT_VTABLE(ArrayIteratorObject);

void ArrayIteratorPrototype::init(ExecutionEngine *e)
{
    defineDefaultProperty(QStringLiteral("next"), method_next, 0);

    Scope scope(e);
    ScopedString val(scope, e->newString(QLatin1String("Array Iterator")));
    defineReadonlyConfigurableProperty(e->symbol_toStringTag(), val);
}

ReturnedValue ArrayIteratorPrototype::method_next(const FunctionObject *b, const Value *that, const Value *, int)
{
    Scope scope(b);
    const ArrayIteratorObject *thisObject = that->as<ArrayIteratorObject>();
    if (!thisObject)
        return scope.engine->throwTypeError(QLatin1String("Not an Array Iterator instance"));

    ScopedObject a(scope, thisObject->d()->iteratedObject);
    if (!a) {
        QV4::Value undefined = Value::undefinedValue();
        return IteratorPrototype::createIterResultObject(scope.engine, undefined, true);
    }

    quint32 index = thisObject->d()->nextIndex;
    IteratorKind itemKind = thisObject->d()->iterationKind;

    quint32 len = a->getLength();

    if (index >= len) {
        thisObject->d()->iteratedObject.set(scope.engine, nullptr);
        QV4::Value undefined = Value::undefinedValue();
        return IteratorPrototype::createIterResultObject(scope.engine, undefined, true);
    }

    thisObject->d()->nextIndex = index + 1;
    if (itemKind == KeyIteratorKind) {
        return IteratorPrototype::createIterResultObject(scope.engine, Value::fromInt32(index), false);
    }

    QV4::ScopedValue elementValue(scope, a->get(index));
    CHECK_EXCEPTION();

    if (itemKind == ValueIteratorKind) {
        return IteratorPrototype::createIterResultObject(scope.engine, elementValue, false);
    } else {
        Q_ASSERT(itemKind == KeyValueIteratorKind);

        ScopedArrayObject resultArray(scope, scope.engine->newArrayObject());
        resultArray->arrayReserve(2);
        resultArray->arrayPut(0, Value::fromInt32(index));
        resultArray->arrayPut(1, elementValue);
        resultArray->setArrayLengthUnchecked(2);

        return IteratorPrototype::createIterResultObject(scope.engine, resultArray, false);
    }
}

