// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllistwrapper_p.h"

#include <QtQml/qqmlinfo.h>

#include <private/qqmllist_p.h>

#include <private/qv4arrayiterator_p.h>
#include <private/qv4arrayobject_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4objectiterator_p.h>
#include <private/qv4objectproto_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4symbol_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcIncompatibleElement, "qt.qml.list.incompatible")

using namespace QV4;
using namespace Qt::StringLiterals;

DEFINE_OBJECT_VTABLE(QmlListWrapper);

static void setArrayData(Heap::QmlListWrapper *d)
{
    QV4::Scope scope(d->internalClass->engine);
    QV4::ScopedObject o(scope, d);
    o->arrayCreate();
}

struct ListWrapperObject
{
    QV4::Scope scope;
    QV4::ScopedObject object;

    ListWrapperObject(QQmlListProperty<QObject> *p)
        : scope(static_cast<Heap::QmlListWrapper *>(p->data)->internalClass->engine)
        , object(scope, static_cast<Heap::QmlListWrapper *>(p->data))
    {
        Q_ASSERT(object);
        Q_ASSERT(object->arrayData());
    }

    Heap::ArrayData *arrayData() const
    {
        return object->arrayData();
    }
};

static void appendWrapped(QQmlListProperty<QObject> *p, QObject *o)
{
    ListWrapperObject object(p);
    Heap::ArrayData *arrayData = object.arrayData();

    const uint length = arrayData->length();
    if (Q_UNLIKELY(length == std::numeric_limits<uint>::max())) {
        object.scope.engine->throwRangeError(QLatin1String("Too many elements."));
        return;
    }

    ArrayData::realloc(object.object, Heap::ArrayData::Simple, length + 1, false);
    QV4::Scope scope(object.scope.engine);
    QV4::ScopedValue wrappedObject(scope, QV4::QObjectWrapper::wrap(object.scope.engine, o));
    arrayData->vtable()->put(
            object.object, length, wrappedObject);
}

static qsizetype countWrapped(QQmlListProperty<QObject> *p)
{
    ListWrapperObject object(p);
    return object.arrayData()->length();
}

static QObject *atWrapped(QQmlListProperty<QObject> *p, qsizetype i)
{
    ListWrapperObject object(p);
    QV4::Scoped<QObjectWrapper> result(object.scope, object.arrayData()->get(i));
    return result ? result->object() : nullptr;
}

static void clearWrapped(QQmlListProperty<QObject> *p)
{
    ListWrapperObject object(p);
    object.arrayData()->vtable()->truncate(object.object, 0);
}

static void replaceWrapped(QQmlListProperty<QObject> *p, qsizetype i, QObject *o)
{
    ListWrapperObject object(p);
    QV4::Scope scope(object.scope.engine);
    QV4::ScopedObject wrappedObject(scope, QV4::QObjectWrapper::wrap(object.scope.engine, o));
    object.arrayData()->vtable()->put(
            object.object, i, wrappedObject);
}

static void removeLastWrapped(QQmlListProperty<QObject> *p)
{
    ListWrapperObject object(p);
    Heap::ArrayData *arrayData = object.arrayData();
    const uint length = arrayData->length();
    if (length > 0)
        arrayData->vtable()->truncate(object.object, length - 1);
}

void Heap::QmlListWrapper::init(QMetaType propertyType)
{
    Object::init();
    m_object.init();
    m_propertyType = propertyType.iface();
    setArrayData(this);
    *property() = QQmlListProperty<QObject>(
            nullptr, this,
            appendWrapped, countWrapped, atWrapped,
            clearWrapped, replaceWrapped, removeLastWrapped);
}

void Heap::QmlListWrapper::init(QObject *object, int propertyId, QMetaType propertyType)
{
    Object::init();
    m_object.init(object);
    m_propertyType = propertyType.iface();
    void *args[] = { property(), nullptr };
    QMetaObject::metacall(object, QMetaObject::ReadProperty, propertyId, args);
}

void Heap::QmlListWrapper::init(
        QObject *object, const QQmlListProperty<QObject> &list, QMetaType propertyType)
{
    Object::init();
    m_object.init(object);
    m_propertyType = propertyType.iface();
    *property() = list;
}

void Heap::QmlListWrapper::destroy()
{
    m_object.destroy();
    Object::destroy();
}

ReturnedValue QmlListWrapper::create(
        ExecutionEngine *engine, QObject *object, int propId, QMetaType propType)
{
    if (!object || propId == -1)
        return Encode::null();
    return engine->memoryManager->allocate<QmlListWrapper>(object, propId, propType)
            ->asReturnedValue();
}

ReturnedValue QmlListWrapper::create(
        ExecutionEngine *engine, const QQmlListProperty<QObject> &prop, QMetaType propType)
{
    return engine->memoryManager->allocate<QmlListWrapper>(prop.object, prop, propType)
            ->asReturnedValue();
}

ReturnedValue QmlListWrapper::create(ExecutionEngine *engine, QMetaType propType)
{
    return engine->memoryManager->allocate<QmlListWrapper>(propType)->asReturnedValue();
}

QVariant QmlListWrapper::toVariant() const
{
    if (!d()->object())
        return QVariant();

    return QVariant::fromValue(toListReference());
}

QQmlListReference QmlListWrapper::toListReference() const
{
    const Heap::QmlListWrapper *wrapper = d();
    return QQmlListReferencePrivate::init(*wrapper->property(), wrapper->propertyType());
}

ReturnedValue QmlListWrapper::virtualGet(const Managed *m, PropertyKey id, const Value *receiver, bool *hasProperty)
{
    Q_ASSERT(m->as<QmlListWrapper>());
    const QmlListWrapper *w = static_cast<const QmlListWrapper *>(m);
    QV4::ExecutionEngine *v4 = w->engine();

    if (id.isArrayIndex()) {
        const uint index = id.asArrayIndex();
        const quint32 count = w->d()->property()->count
                ? w->d()->property()->count(w->d()->property())
                : 0;
        if (index < count && w->d()->property()->at) {
            if (hasProperty)
                *hasProperty = true;
            return QV4::QObjectWrapper::wrap(v4, w->d()->property()->at(w->d()->property(), index));
        }

        if (hasProperty)
            *hasProperty = false;
        return Value::undefinedValue().asReturnedValue();
    }

    return Object::virtualGet(m, id, receiver, hasProperty);
}

qint64 QmlListWrapper::virtualGetLength(const Managed *m)
{
    Q_ASSERT(m->as<QmlListWrapper>());
    QQmlListProperty<QObject> *property = static_cast<const QmlListWrapper *>(m)->d()->property();
    Q_ASSERT(property);
    return property->count ? property->count(property) : 0;
}

bool QmlListWrapper::virtualPut(Managed *m, PropertyKey id, const Value &value, Value *receiver)
{
    Q_ASSERT(m->as<QmlListWrapper>());

    const auto *w = static_cast<const QmlListWrapper *>(m);
    QV4::ExecutionEngine *v4 = w->engine();

    QQmlListProperty<QObject> *prop = w->d()->property();

    if (id.isArrayIndex()) {
        if (!prop->count || !prop->replace)
            return false;

        const uint index = id.asArrayIndex();
        const int count = prop->count(prop);
        if (count < 0 || index >= uint(count))
            return false;

        if (value.isNull()) {
            prop->replace(prop, index, nullptr);
            return true;
        }

        QV4::Scope scope(v4);
        QV4::ScopedObject so(scope, value.toObject(scope.engine));
        if (auto *wrapper = so->as<QV4::QObjectWrapper>()) {
            QObject *object = wrapper->object();
            if (!object) {
                prop->replace(prop, index, object);
                return true;
            }

            const QMetaType elementType = w->d()->elementType();
            const QMetaObject *elementMeta = elementType.metaObject();
            if (Q_UNLIKELY(!elementMeta || !QQmlMetaObject::canConvert(object, elementMeta))) {
                qCWarning(lcIncompatibleElement)
                        << "Cannot insert" << object << "into a QML list of" << elementType.name();
                prop->replace(prop, index, nullptr);
                return true;
            }

            prop->replace(prop, index, object);
            return true;
        }

        return false;
    }

    return Object::virtualPut(m, id, value, receiver);
}

struct QmlListWrapperOwnPropertyKeyIterator : ObjectOwnPropertyKeyIterator
{
    ~QmlListWrapperOwnPropertyKeyIterator() override = default;
    PropertyKey next(const Object *o, Property *pd = nullptr, PropertyAttributes *attrs = nullptr) override;

};

PropertyKey QmlListWrapperOwnPropertyKeyIterator::next(const Object *o, Property *pd, PropertyAttributes *attrs)
{
    const QmlListWrapper *w = static_cast<const QmlListWrapper *>(o);

    quint32 count = w->d()->property()->count ? w->d()->property()->count(w->d()->property()) : 0;
    if (arrayIndex < count) {
        uint index = arrayIndex;
        ++arrayIndex;
        if (attrs)
            *attrs = QV4::Attr_Data;
        if (pd) {
            pd->value = QV4::QObjectWrapper::wrap(
                    w->engine(), w->d()->property()->at(w->d()->property(), index));
        }
        return PropertyKey::fromArrayIndex(index);
    } else if (memberIndex == 0) {
        ++memberIndex;
        return o->engine()->id_length()->propertyKey();
    }

    // You cannot add any own properties via the regular JavaScript interfaces.
    return PropertyKey::invalid();
}

OwnPropertyKeyIterator *QmlListWrapper::virtualOwnPropertyKeys(const Object *m, Value *target)
{
    *target = *m;
    return new QmlListWrapperOwnPropertyKeyIterator;
}

void PropertyListPrototype::init()
{
    defineDefaultProperty(QStringLiteral("pop"), method_pop, 0);
    defineDefaultProperty(QStringLiteral("push"), method_push, 1);
    defineDefaultProperty(QStringLiteral("shift"), method_shift, 0);
    defineDefaultProperty(QStringLiteral("splice"), method_splice, 2);
    defineDefaultProperty(QStringLiteral("unshift"), method_unshift, 1);
    defineDefaultProperty(QStringLiteral("indexOf"), method_indexOf, 1);
    defineDefaultProperty(QStringLiteral("lastIndexOf"), method_lastIndexOf, 1);
    defineDefaultProperty(QStringLiteral("sort"), method_sort, 1);
    defineAccessorProperty(QStringLiteral("length"), method_get_length, method_set_length);
}

ReturnedValue PropertyListPrototype::method_pop(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    Scope scope(b);
    ScopedObject instance(scope, thisObject->toObject(scope.engine));
    if (!instance)
        RETURN_UNDEFINED();

    QmlListWrapper *w = instance->as<QmlListWrapper>();
    if (!w)
        RETURN_UNDEFINED();

    QQmlListProperty<QObject> *property = w->d()->property();

    if (!property->count)
        return scope.engine->throwTypeError(u"List doesn't define a Count function"_s);
    const qsizetype len = property->count(property);
    if (!len)
        RETURN_UNDEFINED();

    if (!property->at)
        return scope.engine->throwTypeError(u"List doesn't define an At function"_s);
    ScopedValue result(
                scope, QV4::QObjectWrapper::wrap(scope.engine, property->at(property, len - 1)));

    if (!property->removeLast)
        return scope.engine->throwTypeError(u"List doesn't define a RemoveLast function"_s);
    property->removeLast(property);

    return result->asReturnedValue();
}

ReturnedValue PropertyListPrototype::method_push(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    ScopedObject instance(scope, thisObject->toObject(scope.engine));
    if (!instance)
        RETURN_UNDEFINED();
    QmlListWrapper *w = instance->as<QmlListWrapper>();
    if (!w)
        RETURN_UNDEFINED();

    QQmlListProperty<QObject> *property = w->d()->property();
    if (!property->append)
        return scope.engine->throwTypeError(u"List doesn't define an Append function"_s);
    if (!property->count)
        return scope.engine->throwTypeError(u"List doesn't define a Count function"_s);

    for (int i = 0; i < argc; ++i) {
        const Value &arg = argv[i];
        if (!arg.isNull() && !arg.as<QObjectWrapper>())
            THROW_TYPE_ERROR();
    }

    const qsizetype length = property->count(property);
    if (!qIsAtMostUintLimit(length, std::numeric_limits<uint>::max() - argc))
        return scope.engine->throwRangeError(QString::fromLatin1("List length out of range."));

    const QMetaType elementType = w->d()->elementType();
    const QMetaObject *elementMeta = elementType.metaObject();
    for (int i = 0; i < argc; ++i) {
        if (argv[i].isNull()) {
            property->append(property, nullptr);
            continue;
        }

        QObject *object = argv[i].as<QV4::QObjectWrapper>()->object();
        if (!object) {
            property->append(property, nullptr);
            continue;
        }

        if (Q_UNLIKELY(!elementMeta || !QQmlMetaObject::canConvert(object, elementMeta))) {
            qCWarning(lcIncompatibleElement)
                    << "Cannot append" << object << "to a QML list of" << elementType.name();
            property->append(property, nullptr);
            continue;
        }

        property->append(property, object);
    }

    const auto actualLength = property->count(property);
    if (actualLength != length + argc)
        qmlWarning(property->object) << "List didn't append all objects";

    return Encode(uint(actualLength));
}

ReturnedValue PropertyListPrototype::method_shift(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    Scope scope(b);
    ScopedObject instance(scope, thisObject->toObject(scope.engine));
    if (!instance)
        RETURN_UNDEFINED();
    QmlListWrapper *w = instance->as<QmlListWrapper>();
    if (!w)
        RETURN_UNDEFINED();

    QQmlListProperty<QObject> *property = w->d()->property();

    if (!property->count)
        return scope.engine->throwTypeError(u"List doesn't define a Count function"_s);
    const qsizetype len = property->count(property);
    if (!len)
        RETURN_UNDEFINED();

    if (!property->at)
        return scope.engine->throwTypeError(u"List doesn't define an At function"_s);
    ScopedValue result(scope, QV4::QObjectWrapper::wrap(scope.engine, property->at(property, 0)));

    if (!property->replace)
        return scope.engine->throwTypeError(u"List doesn't define a Replace function"_s);
    if (!property->removeLast)
        return scope.engine->throwTypeError(u"List doesn't define a RemoveLast function"_s);

    for (qsizetype i = 1; i < len; ++i)
        property->replace(property, i - 1, property->at(property, i));
    property->removeLast(property);

    return result->asReturnedValue();
}

ReturnedValue PropertyListPrototype::method_splice(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    ScopedObject instance(scope, thisObject->toObject(scope.engine));
    if (!instance)
        RETURN_UNDEFINED();
    QmlListWrapper *w = instance->as<QmlListWrapper>();
    if (!w)
        RETURN_UNDEFINED();

    QQmlListProperty<QObject> *property = w->d()->property();

    if (!property->count)
        return scope.engine->throwTypeError(u"List doesn't define a Count function"_s);
    const qsizetype len = property->count(property);

    const double rs = (argc ? argv[0] : Value::undefinedValue()).toInteger();
    qsizetype start;
    if (rs < 0)
        start = static_cast<qsizetype>(qMax(0., len + rs));
    else
        start = static_cast<qsizetype>(qMin(rs, static_cast<double>(len)));

    qsizetype deleteCount = 0;
    qsizetype itemCount = 0;
    if (argc == 1) {
        deleteCount = len - start;
    } else if (argc > 1){
        itemCount = argc - 2;
        double dc = argv[1].toInteger();
        deleteCount = static_cast<qsizetype>(qMin(qMax(dc, 0.), double(len - start)));
    }

    if (itemCount > deleteCount
            && len > std::numeric_limits<qsizetype>::max() - itemCount + deleteCount) {
        return scope.engine->throwTypeError();
    }

    if (!qIsAtMostUintLimit(deleteCount, std::numeric_limits<uint>::max() - 1))
        return scope.engine->throwRangeError(QString::fromLatin1("List length out of range."));

    if (!property->at)
        return scope.engine->throwTypeError(u"List doesn't define an At function"_s);

    for (qsizetype i = 0; i < itemCount; ++i) {
        const auto arg = argv[i + 2];
        if (!arg.isNull() && !arg.as<QObjectWrapper>())
            THROW_TYPE_ERROR();
    }

    ScopedArrayObject newArray(scope, scope.engine->newArrayObject());
    newArray->arrayReserve(deleteCount);
    ScopedValue v(scope);
    QV4::ScopedValue wrappedObject(scope);
    for (qsizetype i = 0; i < deleteCount; ++i) {
        wrappedObject = QObjectWrapper::wrap(scope.engine, property->at(property, start + i));
        newArray->arrayPut(
                    i, wrappedObject);
    }
    newArray->setArrayLengthUnchecked(deleteCount);

    if (!property->replace)
        return scope.engine->throwTypeError(u"List doesn't define a Replace function"_s);
    if (!property->removeLast)
        return scope.engine->throwTypeError(u"List doesn't define a RemoveLast function"_s);
    if (!property->append)
        return scope.engine->throwTypeError(u"List doesn't define an Append function"_s);

    if (itemCount < deleteCount) {
        for (qsizetype k = start; k < len - deleteCount; ++k)
            property->replace(property, k + itemCount, property->at(property, k + deleteCount));
        for (qsizetype k = len; k > len - deleteCount + itemCount; --k)
            property->removeLast(property);
    } else if (itemCount > deleteCount) {
        for (qsizetype k = 0; k < itemCount - deleteCount; ++k)
            property->append(property, nullptr);
        for (qsizetype k = len - deleteCount; k > start; --k) {
            property->replace(
                        property, k + itemCount - 1, property->at(property, k + deleteCount - 1));
        }
    }

    const QMetaType elementType = w->d()->elementType();
    const QMetaObject *elementMeta = elementType.metaObject();
    for (qsizetype i = 0; i < itemCount; ++i) {
        const auto arg = argv[i + 2];
        if (arg.isNull()) {
            property->replace(property, start + i, nullptr);
            continue;
        }

        QObject *object = arg.as<QObjectWrapper>()->object();
        if (!object) {
            property->replace(property, start + i, nullptr);
            continue;
        }

        if (Q_UNLIKELY(!elementMeta || !QQmlMetaObject::canConvert(object, elementMeta))) {
            qCWarning(lcIncompatibleElement)
                    << "Cannot splice" << object << "into a QML list of" << elementType.name();
            property->replace(property, start + i, nullptr);
            continue;
        }

        property->replace(property, start + i, object);
    }

    return newArray->asReturnedValue();
}

ReturnedValue PropertyListPrototype::method_unshift(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    ScopedObject instance(scope, thisObject->toObject(scope.engine));
    if (!instance)
        RETURN_UNDEFINED();

    QmlListWrapper *w = instance->as<QmlListWrapper>();
    if (!w)
        RETURN_UNDEFINED();

    QQmlListProperty<QObject> *property = w->d()->property();

    if (!property->count)
        return scope.engine->throwTypeError(u"List doesn't define a Count function"_s);
    const qsizetype len = property->count(property);

    if (std::numeric_limits<qsizetype>::max() - len < argc || !qIsAtMostUintLimit(len + argc))
        return scope.engine->throwRangeError(QString::fromLatin1("List length out of range."));

    if (!property->append)
        return scope.engine->throwTypeError(u"List doesn't define an Append function"_s);
    if (!property->replace)
        return scope.engine->throwTypeError(u"List doesn't define a Replace function"_s);

    for (int i = 0; i < argc; ++i) {
        const auto arg = argv[i];
        if (!arg.isNull() && !arg.as<QObjectWrapper>())
            THROW_TYPE_ERROR();
    }

    for (int i = 0; i < argc; ++i)
        property->append(property, nullptr);
    if (property->count(property) != argc + len)
        return scope.engine->throwTypeError(u"List doesn't append null objects"_s);

    for (qsizetype k = len; k > 0; --k)
        property->replace(property, k + argc - 1, property->at(property, k - 1));

    const QMetaType elementType = w->d()->elementType();
    const QMetaObject *elementMeta = elementType.metaObject();
    for (int i = 0; i < argc; ++i) {
        const auto *wrapper = argv[i].as<QObjectWrapper>();
        QObject *object = wrapper ? wrapper->object() : nullptr;
        if (!object) {
            property->replace(property, i, object);
            continue;
        }

        if (Q_UNLIKELY(!elementMeta || !QQmlMetaObject::canConvert(object, elementMeta))) {
            qCWarning(lcIncompatibleElement)
                    << "Cannot unshift" << object << "into a QML list of" << elementType.name();
            property->replace(property, i, nullptr);
            continue;
        }

        property->replace(property, i, object);
    }

    return Encode(uint(len + argc));
}

template<typename Iterate>
ReturnedValue firstOrLastIndexOf(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc, Iterate iterate)
{
    Scope scope(b);

    // Undefined cannot be encoded as QObject*. In particular it's not nullptr.
    if (argc == 0)
        THROW_TYPE_ERROR();

    QObject *searchValue;
    if (argv[0].isNull()) {
        searchValue = nullptr;
    } else {
        Scoped<QObjectWrapper> wrapper(scope, argv[0]);
        if (wrapper)
            searchValue = wrapper->object();
        else
            THROW_TYPE_ERROR();
    }

    ScopedObject instance(scope, thisObject->toObject(scope.engine));
    if (!instance)
        RETURN_UNDEFINED();

    QmlListWrapper *w = instance->as<QmlListWrapper>();
    if (!w)
        RETURN_UNDEFINED();

    QQmlListProperty<QObject> *property = w->d()->property();

    if (!property->count)
        return scope.engine->throwTypeError(u"List doesn't define a Count function"_s);
    const qsizetype len = property->count(property);
    if (!len)
        return Encode(-1);


    return iterate(scope.engine, property, len, searchValue);
}

ReturnedValue PropertyListPrototype::method_indexOf(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    return firstOrLastIndexOf(
                b, thisObject, argv, argc,
                [argc, argv](ExecutionEngine *engine, QQmlListProperty<QObject> *property,
                             qsizetype len, QObject *searchValue) -> ReturnedValue {
        qsizetype fromIndex = 0;
        if (argc >= 2) {
            double f = argv[1].toInteger();
            if (hasExceptionOrIsInterrupted(engine))
                return Encode::undefined();
            if (f >= len)
                return Encode(-1);
            if (f < 0)
                f = qMax(len + f, 0.);
            fromIndex = qsizetype(f);
        }

        for (qsizetype i = fromIndex; i < len; ++i) {
            if (property->at(property, i) == searchValue) {
                if (qIsAtMostUintLimit(i))
                    return Encode(uint(i));
                return engine->throwRangeError(QString::fromLatin1("List length out of range."));
            }
        }

        return Encode(-1);
    });
}

ReturnedValue PropertyListPrototype::method_lastIndexOf(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    return firstOrLastIndexOf(
                b, thisObject, argv, argc,
                [argc, argv](ExecutionEngine *engine, QQmlListProperty<QObject> *property,
                             qsizetype len, QObject *searchValue) -> ReturnedValue {
        qsizetype fromIndex = len - 1;
        if (argc >= 2) {
            double f = argv[1].toInteger();
            if (hasExceptionOrIsInterrupted(engine))
                return Encode::undefined();
            if (f > 0)
                f = qMin(f, (double)(len - 1));
            else if (f < 0) {
                f = len + f;
                if (f < 0)
                    return Encode(-1);
            }
            fromIndex = qsizetype(f);
        }

        for (qsizetype i = fromIndex; i >= 0; --i) {
            if (property->at(property, i) == searchValue) {
                if (qIsAtMostUintLimit(i))
                    return Encode(uint(i));
                return engine->throwRangeError(QString::fromLatin1("List length out of range."));
            }
        }

        return Encode(-1);
    });
}

ReturnedValue PropertyListPrototype::method_sort(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    Scope scope(b);
    ScopedObject instance(scope, thisObject->toObject(scope.engine));
    if (!instance)
        RETURN_UNDEFINED();

    QmlListWrapper *w = instance->as<QmlListWrapper>();
    if (!w)
        RETURN_UNDEFINED();

    QQmlListProperty<QObject> *property = w->d()->property();

    if (!property->count)
        return scope.engine->throwTypeError(u"List doesn't define a Count function"_s);
    if (property->count(property) == 0)
        return thisObject->asReturnedValue();
    if (!property->at)
        return scope.engine->throwTypeError(u"List doesn't define an At function"_s);
    if (!property->replace)
        return scope.engine->throwTypeError(u"List doesn't define a Replace function"_s);

    ScopedValue comparefn(scope, argc ? argv[0] : Value::undefinedValue());
    if (!comparefn->isUndefined() && !comparefn->isFunctionObject())
        THROW_TYPE_ERROR();

    const ArrayElementLessThan lessThan(scope.engine, comparefn);
    sortHelper(begin(*property), end(*property), [&](QObject *a, QObject *b) {
        Scoped<QObjectWrapper> o1(scope, QObjectWrapper::wrap(scope.engine, a));
        Scoped<QObjectWrapper> o2(scope, QObjectWrapper::wrap(scope.engine, b));
        return lessThan(o1, o2);
    });

    return thisObject->asReturnedValue();
}

ReturnedValue PropertyListPrototype::method_get_length(const FunctionObject *b, const Value *thisObject, const Value *, int)
{
    Scope scope(b);
    ScopedObject instance(scope, thisObject->toObject(scope.engine));
    if (!instance)
        RETURN_UNDEFINED();

    const QmlListWrapper *w = instance->as<QmlListWrapper>();
    if (!w)
        RETURN_UNDEFINED();

    QQmlListProperty<QObject> *property = w->d()->property();
    if (!property->count)
        return scope.engine->throwTypeError(u"List doesn't define a Count function"_s);

    qsizetype count = property->count(property);
    if (qIsAtMostUintLimit(count))
        return Encode(uint(count));

    return scope.engine->throwRangeError(QString::fromLatin1("List length out of range."));
}

ReturnedValue PropertyListPrototype::method_set_length(const FunctionObject *b, const Value *thisObject, const Value *argv, int argc)
{
    QV4::Scope scope(b);
    ScopedObject instance(scope, thisObject->toObject(scope.engine));
    if (!instance)
        RETURN_UNDEFINED();

    const QmlListWrapper *w = instance->as<QmlListWrapper>();
    if (!w)
        RETURN_UNDEFINED();

    QQmlListProperty<QObject> *property = w->d()->property();

    bool ok = false;
    const uint newLength = argc ? argv[0].asArrayLength(&ok) : 0;
    if (!ok)
        return scope.engine->throwRangeError(QString::fromLatin1("Invalid list length."));

    if (newLength == 0 && property->clear) {
        property->clear(property);
        return true;
    }

    if (!property->count)
        return scope.engine->throwTypeError(u"List doesn't define a Count function"_s);

    qsizetype count = property->count(property);
    if (!qIsAtMostUintLimit(count))
        return scope.engine->throwRangeError(QString::fromLatin1("List length out of range."));

    if (newLength < uint(count)) {
        if (!property->removeLast)
            return scope.engine->throwTypeError(u"List doesn't define a RemoveLast function"_s);

        for (uint i = count; i > newLength; --i)
            property->removeLast(property);

        return true;
    }

    if (!property->append)
        return scope.engine->throwTypeError(u"List doesn't define an Append function"_s);

    for (uint i = count; i < newLength; ++i)
        property->append(property, nullptr);

    count = property->count(property);
    if (!qIsAtMostUintLimit(count))
        return scope.engine->throwRangeError(QString::fromLatin1("List length out of range."));

    if (uint(count) != newLength)
        return scope.engine->throwTypeError(u"List doesn't append null objects"_s);

    return true;

}

QT_END_NAMESPACE
