// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include <qv4symbol_p.h>
#include <qv4functionobject_p.h>
#include <qv4identifiertable_p.h>

using namespace QV4;

DEFINE_OBJECT_VTABLE(SymbolCtor);
DEFINE_MANAGED_VTABLE(Symbol);
DEFINE_OBJECT_VTABLE(SymbolObject);

void Heap::Symbol::init(const QString &s)
{
    Q_ASSERT(s.at(0) == QLatin1Char('@'));
    QString desc(s);
    StringOrSymbol::init(desc.data_ptr());
    identifier = PropertyKey::fromStringOrSymbol(internalClass->engine, this);
}

void Heap::SymbolCtor::init(QV4::ExecutionEngine *engine)
{
    Heap::FunctionObject::init(engine, QStringLiteral("Symbol"));
}

void Heap::SymbolObject::init(const QV4::Symbol *s)
{
    Object::init();
    symbol.set(internalClass->engine, s->d());
}

ReturnedValue QV4::SymbolCtor::virtualCall(const QV4::FunctionObject *f, const QV4::Value *, const QV4::Value *argv, int argc)
{
    Scope scope(f);
    QString desc = QChar::fromLatin1('@');
    if (argc && !argv[0].isUndefined()) {
        ScopedString s(scope, argv[0].toString(scope.engine));
        if (scope.hasException())
            return Encode::undefined();
        desc += s->toQString();
    }
    return Symbol::create(scope.engine, desc)->asReturnedValue();
}

ReturnedValue SymbolCtor::virtualCallAsConstructor(const FunctionObject *f, const Value *, int, const Value *)
{
    return f->engine()->throwTypeError(QStringLiteral("Symbol can't be used together with |new|."));
}

ReturnedValue SymbolCtor::method_for(const FunctionObject *f, const Value *, const Value *argv, int argc)
{
    Scope scope(f);
    ScopedValue k(scope, argc ? argv[0] : Value::undefinedValue());
    ScopedString key(scope, k->toString(scope.engine));
    if (scope.hasException())
        return Encode::undefined();
    QString desc = QLatin1Char('@') + key->toQString();
    return scope.engine->identifierTable->insertSymbol(desc)->asReturnedValue();
}

ReturnedValue SymbolCtor::method_keyFor(const FunctionObject *f, const Value *, const Value *argv, int argc)
{
    ExecutionEngine *e = f->engine();
    if (!argc || !argv[0].isSymbol())
        return e->throwTypeError(QLatin1String("Symbol.keyFor: Argument is not a symbol."));
    const Symbol &arg = static_cast<const Symbol &>(argv[0]);
    Heap::Symbol *s = e->identifierTable->symbolForId(arg.propertyKey());
    Q_ASSERT(!s || s == arg.d());
    if (s)
        return e->newString(arg.toQString().mid((1)))->asReturnedValue();
    return Encode::undefined();
}

void SymbolPrototype::init(ExecutionEngine *engine, Object *ctor)
{
    Scope scope(engine);
    ScopedValue v(scope);
    ctor->defineReadonlyProperty(engine->id_prototype(), (v = this));
    ctor->defineReadonlyConfigurableProperty(engine->id_length(), Value::fromInt32(0));

    ctor->defineDefaultProperty(QStringLiteral("for"), SymbolCtor::method_for, 1);
    ctor->defineDefaultProperty(QStringLiteral("keyFor"), SymbolCtor::method_keyFor, 1);
    ctor->defineReadonlyProperty(QStringLiteral("hasInstance"), *engine->symbol_hasInstance());
    ctor->defineReadonlyProperty(QStringLiteral("isConcatSpreadable"), *engine->symbol_isConcatSpreadable());
    ctor->defineReadonlyProperty(QStringLiteral("iterator"), *engine->symbol_iterator());
    ctor->defineReadonlyProperty(QStringLiteral("match"), *engine->symbol_match());
    ctor->defineReadonlyProperty(QStringLiteral("replace"), *engine->symbol_replace());
    ctor->defineReadonlyProperty(QStringLiteral("search"), *engine->symbol_search());
    ctor->defineReadonlyProperty(QStringLiteral("species"), *engine->symbol_species());
    ctor->defineReadonlyProperty(QStringLiteral("split"), *engine->symbol_split());
    ctor->defineReadonlyProperty(QStringLiteral("toPrimitive"), *engine->symbol_toPrimitive());
    ctor->defineReadonlyProperty(QStringLiteral("toStringTag"), *engine->symbol_toStringTag());
    ctor->defineReadonlyProperty(QStringLiteral("unscopables"), *engine->symbol_unscopables());

    defineDefaultProperty(QStringLiteral("constructor"), (v = ctor));
    defineDefaultProperty(QStringLiteral("toString"), method_toString);
    defineDefaultProperty(QStringLiteral("valueOf"), method_valueOf);
    defineDefaultProperty(engine->symbol_toPrimitive(), method_symbolToPrimitive, 1, Attr_ReadOnly_ButConfigurable);

    v = engine->newString(QStringLiteral("Symbol"));
    defineReadonlyConfigurableProperty(engine->symbol_toStringTag(), v);

}

ReturnedValue SymbolPrototype::method_toString(const FunctionObject *f, const Value *thisObject, const Value *, int)
{
    Scope scope(f);
    Scoped<Symbol> s(scope, thisObject->as<Symbol>());
    if (!s) {
        if (const SymbolObject *o = thisObject->as<SymbolObject>())
            s = o->d()->symbol;
        else
            return scope.engine->throwTypeError();
    }
    return scope.engine->newString(s->descriptiveString())->asReturnedValue();
}

ReturnedValue SymbolPrototype::method_valueOf(const FunctionObject *f, const Value *thisObject, const Value *, int)
{
    Scope scope(f);
    Scoped<Symbol> s(scope, thisObject->as<Symbol>());
    if (!s) {
        if (const SymbolObject *o = thisObject->as<SymbolObject>())
            s = o->d()->symbol;
        else
            return scope.engine->throwTypeError();
    }
    return s->asReturnedValue();
}

ReturnedValue SymbolPrototype::method_symbolToPrimitive(const FunctionObject *f, const Value *thisObject, const Value *, int)
{
    if (thisObject->isSymbol())
        return thisObject->asReturnedValue();
    if (const SymbolObject *o = thisObject->as<SymbolObject>())
        return o->d()->symbol->asReturnedValue();
    return f->engine()->throwTypeError();
}

Heap::Symbol *Symbol::create(ExecutionEngine *e, const QString &s)
{
    Q_ASSERT(s.at(0) == QLatin1Char('@'));
    return e->memoryManager->alloc<Symbol>(s);
}

QString Symbol::descriptiveString() const
{
    return QLatin1String("Symbol(") + QStringView{toQString()}.mid(1) + QLatin1String(")");
}
