// Copyright (C) 2018 Crimson AS <info@crimson.no>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qv4iterator_p.h>
#include <private/qv4estable_p.h>
#include <private/qv4mapiterator_p.h>
#include <private/qv4mapobject_p.h>
#include <private/qv4symbol_p.h>

using namespace QV4;

DEFINE_OBJECT_VTABLE(MapIteratorObject);

void MapIteratorPrototype::init(ExecutionEngine *e)
{
    defineDefaultProperty(QStringLiteral("next"), method_next, 0);

    Scope scope(e);
    ScopedString val(scope, e->newString(QLatin1String("Map Iterator")));
    defineReadonlyConfigurableProperty(e->symbol_toStringTag(), val);
}

ReturnedValue MapIteratorPrototype::method_next(const FunctionObject *b, const Value *that, const Value *, int)
{
    Scope scope(b);
    const MapIteratorObject *thisObject = that->as<MapIteratorObject>();
    if (!thisObject)
        return scope.engine->throwTypeError(QLatin1String("Not a Map Iterator instance"));

    Scoped<MapObject> s(scope, thisObject->d()->iteratedMap);
    uint index = thisObject->d()->mapNextIndex;
    IteratorKind itemKind = thisObject->d()->iterationKind;

    if (!s) {
        QV4::Value undefined = Value::undefinedValue();
        return IteratorPrototype::createIterResultObject(scope.engine, undefined, true);
    }

    Value *arguments = scope.alloc(2);

    while (index < s->d()->esTable->size()) {
        s->d()->esTable->iterate(index, &arguments[0], &arguments[1]);
        thisObject->d()->mapNextIndex = index + 1;

        ScopedValue result(scope);

        if (itemKind == KeyIteratorKind) {
            result = arguments[0];
        } else if (itemKind == ValueIteratorKind) {
            result = arguments[1];
        } else {
            Q_ASSERT(itemKind == KeyValueIteratorKind);

            result = scope.engine->newArrayObject();

            Scoped<ArrayObject> resultArray(scope, result);
            resultArray->arrayReserve(2);
            resultArray->arrayPut(0, arguments[0]);
            resultArray->arrayPut(1, arguments[1]);
            resultArray->setArrayLengthUnchecked(2);
        }

        return IteratorPrototype::createIterResultObject(scope.engine, result, false);
    }

    thisObject->d()->iteratedMap.set(scope.engine, nullptr);
    QV4::Value undefined = Value::undefinedValue();
    return IteratorPrototype::createIterResultObject(scope.engine, undefined, true);
}


