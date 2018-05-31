/****************************************************************************
**
** Copyright (C) 2018 Crimson AS <info@crimson.no>
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

#include "qv4setobject_p.h" // ### temporary
#include "qv4mapobject_p.h"
#include "qv4mapiterator_p.h"
#include "qv4estable_p.h"
#include "qv4symbol_p.h"

using namespace QV4;

DEFINE_OBJECT_VTABLE(MapCtor);
DEFINE_OBJECT_VTABLE(MapObject);

void Heap::MapCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("Map"));
}

ReturnedValue MapCtor::callAsConstructor(const FunctionObject *f, const Value *argv, int argc)
{
    Scope scope(f);
    Scoped<MapObject> a(scope, scope.engine->memoryManager->allocate<MapObject>());

    if (argc > 0) {
        ScopedValue iterable(scope, argv[0]);

        // ### beware, hack alert!
        // Object iteration seems broken right now. if we allow any object to
        // iterate, it endlessly loops in the Map/prototype tests in test262...
        // disable these for now until Object iteration is fixed, just so we can
        // test this.
        Scoped<MapObject> mapObjectCheck(scope, argv[0]);
        Scoped<SetObject> setObjectCheck(scope, argv[0]);

        if (!iterable->isUndefined() && !iterable->isNull() && (mapObjectCheck || setObjectCheck)) {
            ScopedFunctionObject adder(scope, a->get(ScopedString(scope, scope.engine->newString(QString::fromLatin1("set")))));
            if (!adder)
                return scope.engine->throwTypeError();
            ScopedObject iter(scope, Runtime::method_getIterator(scope.engine, iterable, true));

            CHECK_EXCEPTION();
            if (!iter)
                return a.asReturnedValue();

            Value *nextValue = scope.alloc(1);
            ScopedValue done(scope);
            forever {
                done = Runtime::method_iteratorNext(scope.engine, iter, nextValue);
                CHECK_EXCEPTION();
                if (done->toBoolean())
                    return a.asReturnedValue();

                adder->call(a, nextValue, 1);
                if (scope.engine->hasException) {
                    ScopedValue falsey(scope, Encode(false));
                    return Runtime::method_iteratorClose(scope.engine, iter, falsey);
                }
            }
        }
    }
    return a.asReturnedValue();
}

ReturnedValue MapCtor::call(const FunctionObject *f, const Value *, const Value *, int)
{
    Scope scope(f);
    return scope.engine->throwTypeError(QString::fromLatin1("Map requires new"));
}

void MapPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedObject o(scope);
    ctor->defineReadonlyConfigurableProperty(engine->id_length(), Primitive::fromInt32(0));
    ctor->defineReadonlyProperty(engine->id_prototype(), (o = this));
    ctor->addSymbolSpecies();
    defineDefaultProperty(engine->id_constructor(), (o = ctor));

    defineDefaultProperty(QStringLiteral("clear"), method_clear, 0);
    defineDefaultProperty(QStringLiteral("delete"), method_delete, 1);
    defineDefaultProperty(QStringLiteral("forEach"), method_forEach, 1);
    defineDefaultProperty(QStringLiteral("get"), method_get, 1);
    defineDefaultProperty(QStringLiteral("has"), method_has, 1);
    defineDefaultProperty(QStringLiteral("keys"), method_keys, 0);
    defineDefaultProperty(QStringLiteral("set"), method_set, 0);
    defineAccessorProperty(QStringLiteral("size"), method_get_size, nullptr);
    defineDefaultProperty(QStringLiteral("values"), method_values, 0);

    // Per the spec, the value for entries/@@iterator is the same
    ScopedString valString(scope, scope.engine->newIdentifier(QStringLiteral("entries")));
    ScopedFunctionObject entriesFn(scope, FunctionObject::createBuiltinFunction(engine, valString, MapPrototype::method_entries, 0));
    defineDefaultProperty(QStringLiteral("entries"), entriesFn);
    defineDefaultProperty(engine->symbol_iterator(), entriesFn);

    ScopedString val(scope, engine->newString(QLatin1String("Map")));
    defineReadonlyConfigurableProperty(engine->symbol_toStringTag(), val);
}

void Heap::MapObject::init()
{
    Object::init();
    esTable = new ESTable();
}

void Heap::MapObject::destroy()
{
    delete esTable;
    esTable = 0;
}

void Heap::MapObject::markObjects(Heap::Base *that, MarkStack *markStack)
{
    MapObject *m = static_cast<MapObject *>(that);
    m->esTable->markObjects(markStack);
    Object::markObjects(that, markStack);
}

ReturnedValue MapPrototype::method_clear(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    Scope scope(b);
    Scoped<MapObject> that(scope, thisObject);
    if (!that)
        return scope.engine->throwTypeError();

    that->d()->esTable->clear();
    return Encode::undefined();
}

ReturnedValue MapPrototype::method_delete(const FunctionObject *b, const Value *thisObject, const Value *argv, int)
{
    Scope scope(b);
    Scoped<MapObject> that(scope, thisObject);
    if (!that)
        return scope.engine->throwTypeError();

    return Encode(that->d()->esTable->remove(argv[0]));
}

ReturnedValue MapPrototype::method_entries(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    Scope scope(b);
    Scoped<MapObject> that(scope, thisObject);
    if (!that)
        return scope.engine->throwTypeError();

    Scoped<MapIteratorObject> ao(scope, scope.engine->newMapIteratorObject(that));
    ao->d()->iterationKind = IteratorKind::KeyValueIteratorKind;
    return ao->asReturnedValue();
}

ReturnedValue MapPrototype::method_forEach(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    Scoped<MapObject> that(scope, thisObject);
    if (!that)
        return scope.engine->throwTypeError();

    ScopedFunctionObject callbackfn(scope, argv[0]);
    if (!callbackfn)
        return scope.engine->throwTypeError();

    ScopedValue thisArg(scope, Primitive::undefinedValue());
    if (argc > 1)
        thisArg = ScopedValue(scope, argv[1]);

    Value *arguments = scope.alloc(3);
    for (uint i = 0; i < that->d()->esTable->size(); ++i) {
        that->d()->esTable->iterate(i, &arguments[0], &arguments[1]); // fill in key (0), value (1)

        arguments[2] = that;
        callbackfn->call(thisArg, arguments, 3);
        CHECK_EXCEPTION();
    }
    return Encode::undefined();
}

ReturnedValue MapPrototype::method_get(const FunctionObject *b, const Value *thisObject, const Value *argv, int)
{
    Scope scope(b);
    Scoped<MapObject> that(scope, thisObject);
    if (!that)
        return scope.engine->throwTypeError();

    return that->d()->esTable->get(argv[0]);
}

ReturnedValue MapPrototype::method_has(const FunctionObject *b, const Value *thisObject, const Value *argv, int)
{
    Scope scope(b);
    Scoped<MapObject> that(scope, thisObject);
    if (!that)
        return scope.engine->throwTypeError();

    return Encode(that->d()->esTable->has(argv[0]));
}

ReturnedValue MapPrototype::method_keys(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    Scope scope(b);
    Scoped<MapObject> that(scope, thisObject);
    if (!that)
        return scope.engine->throwTypeError();

    Scoped<MapIteratorObject> ao(scope, scope.engine->newMapIteratorObject(that));
    ao->d()->iterationKind = IteratorKind::KeyIteratorKind;
    return ao->asReturnedValue();
}

ReturnedValue MapPrototype::method_set(const FunctionObject *b, const Value *thisObject, const Value *argv, int)
{
    Scope scope(b);
    Scoped<MapObject> that(scope, thisObject);
    if (!that)
        return scope.engine->throwTypeError();

    that->d()->esTable->set(argv[0], argv[1]);
    return that.asReturnedValue();
}

ReturnedValue MapPrototype::method_get_size(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    Scope scope(b);
    Scoped<MapObject> that(scope, thisObject);
    if (!that)
        return scope.engine->throwTypeError();

    return Encode(that->d()->esTable->size());
}

ReturnedValue MapPrototype::method_values(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    Scope scope(b);
    Scoped<MapObject> that(scope, thisObject);
    if (!that)
        return scope.engine->throwTypeError();

    Scoped<MapIteratorObject> ao(scope, scope.engine->newMapIteratorObject(that));
    ao->d()->iterationKind = IteratorKind::ValueIteratorKind;
    return ao->asReturnedValue();
}


