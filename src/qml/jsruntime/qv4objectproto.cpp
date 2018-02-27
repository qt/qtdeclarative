/****************************************************************************
**
** Copyright (C) 2017 Crimson AS <info@crimson.no>
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


#include "qv4objectproto_p.h"
#include "qv4argumentsobject_p.h"
#include <private/qv4mm_p.h>
#include "qv4scopedvalue_p.h"
#include "qv4runtime_p.h"
#include "qv4objectiterator_p.h"
#include "qv4string_p.h"
#include "qv4jscall_p.h"

#include <QtCore/QDateTime>
#include <QtCore/QStringList>

using namespace QV4;


DEFINE_OBJECT_VTABLE(ObjectCtor);

void Heap::ObjectCtor::init(QV4::ExecutionContext *scope)
{
    Heap::FunctionObject::init(scope, QStringLiteral("Object"));
}

ReturnedValue ObjectCtor::callAsConstructor(const FunctionObject *f, const Value *argv, int argc)
{
    ExecutionEngine *v4 = f->engine();
    const ObjectCtor *ctor = static_cast<const ObjectCtor *>(f);
    if (!argc || argv[0].isUndefined() || argv[0].isNull()) {
        Scope scope(v4);
        ScopedObject obj(scope, scope.engine->newObject());
        ScopedObject proto(scope, ctor->get(scope.engine->id_prototype()));
        if (!!proto)
            obj->setPrototype(proto);
        return obj.asReturnedValue();
    } else {
        return argv[0].toObject(v4)->asReturnedValue();
    }
}

ReturnedValue ObjectCtor::call(const FunctionObject *m, const Value *, const Value *argv, int argc)
{
    ExecutionEngine *v4 = m->engine();
    if (!argc || argv[0].isUndefined() || argv[0].isNull()) {
        return v4->newObject()->asReturnedValue();
    } else {
        return argv[0].toObject(v4)->asReturnedValue();
    }
}

void ObjectPrototype::init(ExecutionEngine *v4, Object *ctor)
{
    Scope scope(v4);
    ScopedObject o(scope, this);

    ctor->defineReadonlyProperty(v4->id_prototype(), o);
    ctor->defineReadonlyConfigurableProperty(v4->id_length(), Primitive::fromInt32(1));
    ctor->defineDefaultProperty(QStringLiteral("getPrototypeOf"), method_getPrototypeOf, 1);
    ctor->defineDefaultProperty(QStringLiteral("getOwnPropertyDescriptor"), method_getOwnPropertyDescriptor, 2);
    ctor->defineDefaultProperty(QStringLiteral("getOwnPropertyNames"), method_getOwnPropertyNames, 1);
    ctor->defineDefaultProperty(QStringLiteral("assign"), method_assign, 2);
    ctor->defineDefaultProperty(QStringLiteral("create"), method_create, 2);
    ctor->defineDefaultProperty(QStringLiteral("defineProperty"), method_defineProperty, 3);
    ctor->defineDefaultProperty(QStringLiteral("defineProperties"), method_defineProperties, 2);
    ctor->defineDefaultProperty(QStringLiteral("seal"), method_seal, 1);
    ctor->defineDefaultProperty(QStringLiteral("freeze"), method_freeze, 1);
    ctor->defineDefaultProperty(QStringLiteral("preventExtensions"), method_preventExtensions, 1);
    ctor->defineDefaultProperty(QStringLiteral("isSealed"), method_isSealed, 1);
    ctor->defineDefaultProperty(QStringLiteral("isFrozen"), method_isFrozen, 1);
    ctor->defineDefaultProperty(QStringLiteral("isExtensible"), method_isExtensible, 1);
    ctor->defineDefaultProperty(QStringLiteral("keys"), method_keys, 1);

    defineDefaultProperty(QStringLiteral("constructor"), (o = ctor));
    defineDefaultProperty(v4->id_toString(), method_toString, 0);
    defineDefaultProperty(QStringLiteral("toLocaleString"), method_toLocaleString, 0);
    defineDefaultProperty(v4->id_valueOf(), method_valueOf, 0);
    defineDefaultProperty(QStringLiteral("hasOwnProperty"), method_hasOwnProperty, 1);
    defineDefaultProperty(QStringLiteral("isPrototypeOf"), method_isPrototypeOf, 1);
    defineDefaultProperty(QStringLiteral("propertyIsEnumerable"), method_propertyIsEnumerable, 1);
    defineDefaultProperty(QStringLiteral("__defineGetter__"), method_defineGetter, 2);
    defineDefaultProperty(QStringLiteral("__defineSetter__"), method_defineSetter, 2);

    ExecutionContext *global = v4->rootContext();
    ScopedProperty p(scope);
    p->value = FunctionObject::createBuiltinFunction(global, v4->id___proto__(), method_get_proto);
    p->set = FunctionObject::createBuiltinFunction(global, v4->id___proto__(), method_set_proto);
    insertMember(v4->id___proto__(), p, Attr_Accessor|Attr_NotEnumerable);
}

ReturnedValue ObjectPrototype::method_getPrototypeOf(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    if (argc < 1)
        return scope.engine->throwTypeError();

    ScopedObject o(scope, argv[0].toObject(scope.engine));
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    ScopedObject p(scope, o->prototype());
    return (!!p ? p->asReturnedValue() : Encode::null());
}

ReturnedValue ObjectPrototype::method_getOwnPropertyDescriptor(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    if (argc < 1)
        return scope.engine->throwTypeError();

    ScopedObject O(scope, argv[0].toObject(scope.engine));
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    if (ArgumentsObject::isNonStrictArgumentsObject(O))
        static_cast<ArgumentsObject *>(O.getPointer())->fullyCreate();

    ScopedValue v(scope, argc > 1 ? argv[1] : Primitive::undefinedValue());
    ScopedString name(scope, v->toString(scope.engine));
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    PropertyAttributes attrs;
    ScopedProperty desc(scope);
    O->getOwnProperty(name, &attrs, desc);
    return fromPropertyDescriptor(scope.engine, desc, attrs);
}

ReturnedValue ObjectPrototype::method_getOwnPropertyNames(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    if (argc < 1)
        return scope.engine->throwTypeError();

    ScopedObject O(scope, argv[0].toObject(scope.engine));
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    return Encode(getOwnPropertyNames(scope.engine, argv[0]));
}

// 19.1.2.1
ReturnedValue ObjectPrototype::method_assign(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    if (argc < 1)
        return scope.engine->throwTypeError();

    ScopedObject to(scope, argv[0].toObject(scope.engine));
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    if (argc == 1)
        return to.asReturnedValue();

    for (int i = 1, ei = argc; i < ei; ++i) {
        if (argv[i].isUndefined() || argv[i].isNull())
            continue;

        ScopedObject from(scope, argv[i].toObject(scope.engine));
        if (scope.engine->hasException)
        return QV4::Encode::undefined();
        QV4::ScopedArrayObject keys(scope, QV4::ObjectPrototype::getOwnPropertyNames(scope.engine, from));
        quint32 length = keys->getLength();

        ScopedString nextKey(scope);
        ScopedValue propValue(scope);
        for (quint32 i = 0; i < length; ++i) {
            nextKey = Value::fromReturnedValue(keys->getIndexed(i)).toString(scope.engine);

            PropertyAttributes attrs;
            ScopedProperty prop(scope);
            from->getOwnProperty(nextKey, &attrs, prop);

            if (attrs == PropertyFlag::Attr_Invalid)
                continue;

            if (!attrs.isEnumerable())
                continue;

            propValue = from->get(nextKey);
            to->set(nextKey, propValue, Object::DoThrowOnRejection);
            if (scope.engine->hasException)
        return QV4::Encode::undefined();
        }
    }

    return to.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_create(const FunctionObject *builtin, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(builtin);
    if (!argc || (!argv[0].isObject() && !argv[0].isNull()))
        return scope.engine->throwTypeError();

    ScopedObject O(scope, argv[0]);

    ScopedObject newObject(scope, scope.engine->newObject());
    newObject->setPrototype(O);


    if (argc > 1 && !argv[1].isUndefined()) {
        Value *arguments = scope.alloc(argc);
        arguments[0] = newObject;
        memcpy(arguments + 1, argv + 1, (argc - 1)*sizeof(Value));
        return method_defineProperties(builtin, thisObject, arguments, argc);
    }

    return newObject.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_defineProperty(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    if (!argc || !argv[0].isObject())
        return scope.engine->throwTypeError();

    ScopedObject O(scope, argv[0]);
    ScopedString name(scope, argc > 1 ? argv[1] : Primitive::undefinedValue(), ScopedString::Convert);
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    ScopedValue attributes(scope, argc > 2 ? argv[2] : Primitive::undefinedValue());
    ScopedProperty pd(scope);
    PropertyAttributes attrs;
    toPropertyDescriptor(scope.engine, attributes, pd, &attrs);
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    if (!O->__defineOwnProperty__(scope.engine, name, pd, attrs))
        THROW_TYPE_ERROR();

    return O.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_defineProperties(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    if (argc < 2 || !argv[0].isObject())
        return scope.engine->throwTypeError();

    ScopedObject O(scope, argv[0]);

    ScopedObject o(scope, argv[1].toObject(scope.engine));
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    ScopedValue val(scope);

    ObjectIterator it(scope, o, ObjectIterator::EnumerableOnly);
    ScopedString name(scope);
    ScopedProperty pd(scope);
    ScopedProperty n(scope);
    while (1) {
        uint index;
        PropertyAttributes attrs;
        it.next(name.getRef(), &index, pd, &attrs);
        if (attrs.isEmpty())
            break;
        PropertyAttributes nattrs;
        val = o->getValue(pd->value, attrs);
        toPropertyDescriptor(scope.engine, val, n, &nattrs);
        if (scope.engine->hasException)
        return QV4::Encode::undefined();
        bool ok;
        if (name)
            ok = O->__defineOwnProperty__(scope.engine, name, n, nattrs);
        else
            ok = O->__defineOwnProperty__(scope.engine, index, n, nattrs);
        if (!ok)
            THROW_TYPE_ERROR();
    }

    return O.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_seal(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    const Value a = argc ? argv[0] : Primitive::undefinedValue();
    if (!a.isObject())
        // 19.1.2.17, 1
        return a.asReturnedValue();

    Scope scope(b);
    ScopedObject o(scope, a);
    o->setInternalClass(o->internalClass()->sealed());

    if (o->arrayData()) {
        ArrayData::ensureAttributes(o);
        for (uint i = 0; i < o->d()->arrayData->values.alloc; ++i) {
            if (!o->arrayData()->isEmpty(i))
                o->d()->arrayData->attrs[i].setConfigurable(false);
        }
    }

    return o.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_freeze(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    const Value a = argc ? argv[0] : Primitive::undefinedValue();
    if (!a.isObject())
        // 19.1.2.5, 1
        return a.asReturnedValue();

    Scope scope(b);
    ScopedObject o(scope, a);

    if (ArgumentsObject::isNonStrictArgumentsObject(o))
        static_cast<ArgumentsObject *>(o.getPointer())->fullyCreate();

    o->setInternalClass(o->internalClass()->frozen());

    if (o->arrayData()) {
        ArrayData::ensureAttributes(o);
        for (uint i = 0; i < o->arrayData()->values.alloc; ++i) {
            if (!o->arrayData()->isEmpty(i))
                o->arrayData()->attrs[i].setConfigurable(false);
            if (o->arrayData()->attrs[i].isData())
                o->arrayData()->attrs[i].setWritable(false);
        }
    }
    return o.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_preventExtensions(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    if (!argc)
        return scope.engine->throwTypeError();

    ScopedObject o(scope, argv[0].toObject(scope.engine));
    if (!o)
        return argv[0].asReturnedValue();

    o->setInternalClass(o->internalClass()->nonExtensible());
    return o.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_isSealed(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    if (!argc)
        return scope.engine->throwTypeError();

    ScopedObject o(scope, argv[0].toObject(scope.engine));
    if (!o)
        return Encode(true);

    if (o->isExtensible())
        return  Encode(false);

    if (o->internalClass() != o->internalClass()->sealed())
        return Encode(false);

    if (!o->arrayData() || !o->arrayData()->length())
        return Encode(true);

    Q_ASSERT(o->arrayData() && o->arrayData()->length());
    if (!o->arrayData()->attrs)
        return Encode(false);

    for (uint i = 0; i < o->arrayData()->values.alloc; ++i) {
        if (!o->arrayData()->isEmpty(i))
            if (o->arrayData()->attributes(i).isConfigurable())
                return Encode(false);
    }

    return Encode(true);
}

ReturnedValue ObjectPrototype::method_isFrozen(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    if (!argc)
        return scope.engine->throwTypeError();

    ScopedObject o(scope, argv[0].toObject(scope.engine));
    if (!o)
        return Encode(true);

    if (o->isExtensible())
        return Encode(false);

    if (o->internalClass() != o->internalClass()->frozen())
        return Encode(false);

    if (!o->arrayData() || !o->arrayData()->length())
        return Encode(true);

    Q_ASSERT(o->arrayData() && o->arrayData()->length());
    if (!o->arrayData()->attrs)
        return Encode(false);

    for (uint i = 0; i < o->arrayData()->values.alloc; ++i) {
        if (!o->arrayData()->isEmpty(i))
            if (o->arrayData()->attributes(i).isConfigurable() || o->arrayData()->attributes(i).isWritable())
                return Encode(false);
    }

    return Encode(true);
}

ReturnedValue ObjectPrototype::method_isExtensible(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    if (!argc)
        return scope.engine->throwTypeError();

    ScopedObject o(scope, argv[0].toObject(scope.engine));
    if (!o)
        return Encode(false);

    return Encode((bool)o->isExtensible());
}

ReturnedValue ObjectPrototype::method_keys(const FunctionObject *b, const Value *, const Value *argv, int argc)
{
    Scope scope(b);
    if (!argc)
        return scope.engine->throwTypeError();

    ScopedObject o(scope, argv[0].toObject(scope.engine));
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    ScopedArrayObject a(scope, scope.engine->newArrayObject());

    ObjectIterator it(scope, o, ObjectIterator::EnumerableOnly);
    ScopedValue name(scope);
    while (1) {
        name = it.nextPropertyNameAsString();
        if (name->isNull())
            break;
        a->push_back(name);
    }

    return a.asReturnedValue();
}

ReturnedValue ObjectPrototype::method_toString(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    ExecutionEngine *v4 = b->engine();
    if (thisObject->isUndefined()) {
        return Encode(v4->newString(QStringLiteral("[object Undefined]")));
    } else if (thisObject->isNull()) {
        return Encode(v4->newString(QStringLiteral("[object Null]")));
    } else {
        Scope scope(v4);
        ScopedObject obj(scope, thisObject->toObject(scope.engine));
        QString className = obj->className();
        return Encode(v4->newString(QStringLiteral("[object %1]").arg(className)));
    }
}

ReturnedValue ObjectPrototype::method_toLocaleString(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    ScopedObject o(scope, thisObject->toObject(scope.engine));
    if (!o)
        RETURN_UNDEFINED();

    ScopedFunctionObject f(scope, o->get(scope.engine->id_toString()));
    if (!f)
        THROW_TYPE_ERROR();

    return f->call(thisObject, argv, argc);
}

ReturnedValue ObjectPrototype::method_valueOf(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    return Encode(thisObject->toObject(b->engine()));
}

ReturnedValue ObjectPrototype::method_hasOwnProperty(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    ScopedString P(scope, argc ? argv[0] : Primitive::undefinedValue(), ScopedString::Convert);
    if (scope.engine->hasException)
        return QV4::Encode::undefined();
    ScopedObject O(scope, thisObject->toObject(scope.engine));
    if (scope.engine->hasException)
        return QV4::Encode::undefined();
    bool r = O->hasOwnProperty(P);
    if (!r)
        r = !O->query(P).isEmpty();
    return Encode(r);
}

ReturnedValue ObjectPrototype::method_isPrototypeOf(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    if (!argc || !argv[0].isObject())
        return Encode(false);

    ScopedObject V(scope, argv[0]);
    ScopedObject O(scope, thisObject->toObject(scope.engine));
    if (scope.engine->hasException)
        return QV4::Encode::undefined();
    ScopedObject proto(scope, V->prototype());
    while (proto) {
        if (O->d() == proto->d())
            return Encode(true);
        proto = proto->prototype();
    }
    return Encode(false);
}

ReturnedValue ObjectPrototype::method_propertyIsEnumerable(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    ScopedString p(scope, argc ? argv[0] : Primitive::undefinedValue(), ScopedString::Convert);
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    ScopedObject o(scope, thisObject->toObject(scope.engine));
    if (scope.engine->hasException)
        return QV4::Encode::undefined();
    PropertyAttributes attrs;
    o->getOwnProperty(p, &attrs);
    return Encode(attrs.isEnumerable());
}

ReturnedValue ObjectPrototype::method_defineGetter(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    if (argc < 2)
        THROW_TYPE_ERROR();

    ScopedFunctionObject f(scope, argv[1]);
    if (!f)
        THROW_TYPE_ERROR();

    ScopedString prop(scope, argv[0], ScopedString::Convert);
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    ScopedObject o(scope, thisObject);
    if (!o) {
        if (!thisObject->isUndefined())
            RETURN_UNDEFINED();
        o = scope.engine->globalObject;
    }

    ScopedProperty pd(scope);
    pd->value = f;
    pd->set = Primitive::emptyValue();
    bool ok = o->__defineOwnProperty__(scope.engine, prop, pd, Attr_Accessor);
    if (!ok)
        THROW_TYPE_ERROR();
    RETURN_UNDEFINED();
}

ReturnedValue ObjectPrototype::method_defineSetter(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    if (argc < 2)
        THROW_TYPE_ERROR();

    ScopedFunctionObject f(scope, argv[1]);
    if (!f)
        THROW_TYPE_ERROR();

    ScopedString prop(scope, argv[0], ScopedString::Convert);
    if (scope.engine->hasException)
        return QV4::Encode::undefined();

    ScopedObject o(scope, thisObject);
    if (!o) {
        if (!thisObject->isUndefined())
            RETURN_UNDEFINED();
        o = scope.engine->globalObject;
    }

    ScopedProperty pd(scope);
    pd->value = Primitive::emptyValue();
    pd->set = f;
    bool ok = o->__defineOwnProperty__(scope.engine, prop, pd, Attr_Accessor);
    if (!ok)
        THROW_TYPE_ERROR();
    RETURN_UNDEFINED();
}

ReturnedValue ObjectPrototype::method_get_proto(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    Scope scope(b);
    ScopedObject o(scope, thisObject->as<Object>());
    if (!o)
        THROW_TYPE_ERROR();

    return Encode(o->prototype());
}

ReturnedValue ObjectPrototype::method_set_proto(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    ScopedObject o(scope, thisObject);
    if (!o || !argc)
        THROW_TYPE_ERROR();

    if (argv[0].isNull()) {
        o->setPrototype(nullptr);
        RETURN_UNDEFINED();
    }

    ScopedObject p(scope, argv[0]);
    bool ok = false;
    if (!!p) {
        if (o->prototype() == p->d()) {
            ok = true;
        } else if (o->isExtensible()) {
            ok = o->setPrototype(p);
        }
    }
    if (!ok)
        return scope.engine->throwTypeError(QStringLiteral("Cyclic __proto__ value"));
    RETURN_UNDEFINED();
}

void ObjectPrototype::toPropertyDescriptor(ExecutionEngine *engine, const Value &v, Property *desc, PropertyAttributes *attrs)
{
    Scope scope(engine);
    ScopedObject o(scope, v);
    if (!o) {
        engine->throwTypeError();
        return;
    }

    attrs->clear();
    desc->value = Primitive::emptyValue();
    desc->set = Primitive::emptyValue();
    ScopedValue tmp(scope);

    if (o->hasProperty(engine->id_enumerable()))
        attrs->setEnumerable((tmp = o->get(engine->id_enumerable()))->toBoolean());

    if (o->hasProperty(engine->id_configurable()))
        attrs->setConfigurable((tmp = o->get(engine->id_configurable()))->toBoolean());

    if (o->hasProperty(engine->id_get())) {
        ScopedValue get(scope, o->get(engine->id_get()));
        FunctionObject *f = get->as<FunctionObject>();
        if (f || get->isUndefined()) {
            desc->value = get;
        } else {
            engine->throwTypeError();
            return;
        }
        attrs->setType(PropertyAttributes::Accessor);
    }

    if (o->hasProperty(engine->id_set())) {
        ScopedValue set(scope, o->get(engine->id_set()));
        FunctionObject *f = set->as<FunctionObject>();
        if (f || set->isUndefined()) {
            desc->set = set;
        } else {
            engine->throwTypeError();
            return;
        }
        attrs->setType(PropertyAttributes::Accessor);
    }

    if (o->hasProperty(engine->id_writable())) {
        if (attrs->isAccessor()) {
            engine->throwTypeError();
            return;
        }
        attrs->setWritable((tmp = o->get(engine->id_writable()))->toBoolean());
        // writable forces it to be a data descriptor
        desc->value = Primitive::undefinedValue();
    }

    if (o->hasProperty(engine->id_value())) {
        if (attrs->isAccessor()) {
            engine->throwTypeError();
            return;
        }
        desc->value = o->get(engine->id_value());
        attrs->setType(PropertyAttributes::Data);
    }

    if (attrs->isGeneric())
        desc->value = Primitive::emptyValue();
}


ReturnedValue ObjectPrototype::fromPropertyDescriptor(ExecutionEngine *engine, const Property *desc, PropertyAttributes attrs)
{
    if (attrs.isEmpty())
        return Encode::undefined();

    Scope scope(engine);
    // Let obj be the result of creating a new object as if by the expression new Object() where Object
    // is the standard built-in constructor with that name.
    ScopedObject o(scope, engine->newObject());
    ScopedString s(scope);

    ScopedProperty pd(scope);
    if (attrs.isData()) {
        pd->value = desc->value;
        s = engine->newString(QStringLiteral("value"));
        o->__defineOwnProperty__(scope.engine, s, pd, Attr_Data);
        pd->value = Primitive::fromBoolean(attrs.isWritable());
        s = engine->newString(QStringLiteral("writable"));
        o->__defineOwnProperty__(scope.engine, s, pd, Attr_Data);
    } else {
        pd->value = desc->getter() ? desc->getter()->asReturnedValue() : Encode::undefined();
        s = engine->newString(QStringLiteral("get"));
        o->__defineOwnProperty__(scope.engine, s, pd, Attr_Data);
        pd->value = desc->setter() ? desc->setter()->asReturnedValue() : Encode::undefined();
        s = engine->newString(QStringLiteral("set"));
        o->__defineOwnProperty__(scope.engine, s, pd, Attr_Data);
    }
    pd->value = Primitive::fromBoolean(attrs.isEnumerable());
    s = engine->newString(QStringLiteral("enumerable"));
    o->__defineOwnProperty__(scope.engine, s, pd, Attr_Data);
    pd->value = Primitive::fromBoolean(attrs.isConfigurable());
    s = engine->newString(QStringLiteral("configurable"));
    o->__defineOwnProperty__(scope.engine, s, pd, Attr_Data);

    return o.asReturnedValue();
}

// es6: GetOwnPropertyKeys
Heap::ArrayObject *ObjectPrototype::getOwnPropertyNames(ExecutionEngine *v4, const Value &o)
{
    Scope scope(v4);
    ScopedArrayObject array(scope, v4->newArrayObject());
    ScopedObject O(scope, o.toObject(v4));
    if (O) {
        ObjectIterator it(scope, O, ObjectIterator::NoFlags);
        ScopedValue name(scope);
        while (1) {
            name = it.nextPropertyNameAsString();
            if (name->isNull())
                break;
            array->push_back(name);
        }
    }
    return array->d();
}
