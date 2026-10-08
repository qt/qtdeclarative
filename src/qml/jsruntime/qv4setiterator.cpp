// Copyright (C) 2018 Crimson AS <info@crimson.no>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include <private/qv4iterator_p.h>
#include <private/qv4estable_p.h>
#include <private/qv4setiterator_p.h>
#include <private/qv4setobject_p.h>
#include <private/qv4symbol_p.h>

using namespace QV4;

DEFINE_OBJECT_VTABLE(SetIteratorObject);

void SetIteratorPrototype::init(ExecutionEngine *e)
{
    defineDefaultProperty(QStringLiteral("next"), method_next, 0);

    Scope scope(e);
    ScopedString val(scope, e->newString(QLatin1String("Set Iterator")));
    defineReadonlyConfigurableProperty(e->symbol_toStringTag(), val);
}

ReturnedValue SetIteratorPrototype::method_next(const FunctionObject *b, const Value *that, const Value *, int)
{
    Scope scope(b);
    const SetIteratorObject *thisObject = that->as<SetIteratorObject>();
    if (!thisObject)
        return scope.engine->throwTypeError(QLatin1String("Not a Set Iterator instance"));

    Scoped<SetObject> s(scope, thisObject->d()->iteratedSet);
    uint index = thisObject->d()->setNextIndex;
    IteratorKind itemKind = thisObject->d()->iterationKind;

    if (!s) {
        QV4::Value undefined = Value::undefinedValue();
        return IteratorPrototype::createIterResultObject(scope.engine, undefined, true);
    }

    Value *arguments = scope.alloc(2);

    while (index < s->d()->esTable->size()) {
        s->d()->esTable->iterate(index, &arguments[0], &arguments[1]);
        thisObject->d()->setNextIndex = index + 1;

        if (itemKind == KeyValueIteratorKind) {
            ScopedArrayObject resultArray(scope, scope.engine->newArrayObject());
            resultArray->arrayReserve(2);
            resultArray->arrayPut(0, arguments[0]);
            resultArray->arrayPut(1, arguments[0]); // yes, the key is repeated.
            resultArray->setArrayLengthUnchecked(2);

            return IteratorPrototype::createIterResultObject(scope.engine, resultArray, false);
        }

        return IteratorPrototype::createIterResultObject(scope.engine, arguments[0], false);
    }

    thisObject->d()->iteratedSet.set(scope.engine, nullptr);
    QV4::Value undefined = Value::undefinedValue();
    return IteratorPrototype::createIterResultObject(scope.engine, undefined, true);
}

