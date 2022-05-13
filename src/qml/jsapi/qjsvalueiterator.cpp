// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qjsvalueiterator.h"
#include "qjsvalueiterator_p.h"
#include "qjsvalue_p.h"
#include "private/qv4string_p.h"
#include "private/qv4object_p.h"
#include "private/qv4context_p.h"

QT_BEGIN_NAMESPACE

QJSValueIteratorPrivate::QJSValueIteratorPrivate(const QJSValue &v)
{
    init(v);
}

void QJSValueIteratorPrivate::init(const QJSValue &v)
{
    engine = nullptr;

    QV4::ExecutionEngine *e = QJSValuePrivate::engine(&v);
    if (!e)
        return;
    const QV4::Object *o = QJSValuePrivate::asManagedType<QV4::Object>(&v);
    if (!o)
        return;

    engine = e;
    object.set(e, o->asReturnedValue());
    iterator.reset(o->ownPropertyKeys(object.valueRef()));
    next();
}

void QJSValueIteratorPrivate::next()
{
    QV4::Object *o = object.as<QV4::Object>();
    if (!o || !iterator)
        return;

    QV4::PropertyKey key;
    while (1) {
        key = iterator->next(o);
        if (!key.isSymbol())
            break;
    }
    currentKey = nextKey;
    nextKey.set(engine, key.id());
}

bool QJSValueIteratorPrivate::isValid() const
{
    if (!engine || !iterator)
        return false;
    QV4::Value *val = object.valueRef();
    return (val && val->isObject());
}

/*!
    \class QJSValueIterator

    \brief The QJSValueIterator class provides a Java-style iterator for QJSValue.

    \ingroup qtjavascript
    \inmodule QtQml


    The QJSValueIterator constructor takes a QJSValue as
    argument.  After construction, the iterator is located at the very
    beginning of the sequence of properties. Here's how to iterate over
    all the properties of a QJSValue:

    \snippet code/src_script_qjsvalueiterator.cpp 0

    The next() advances the iterator. The name() and value()
    functions return the name and value of the last item that was
    jumped over.

    Note that QJSValueIterator only iterates over the QJSValue's
    own properties; i.e. it does not follow the prototype chain. You can
    use a loop like this to follow the prototype chain:

    \snippet code/src_script_qjsvalueiterator.cpp 1

    \sa QJSValue::property()
*/

/*!
    Constructs an iterator for traversing \a object. The iterator is
    set to be at the front of the sequence of properties (before the
    first property).
*/
QJSValueIterator::QJSValueIterator(const QJSValue& object)
    : d_ptr(new QJSValueIteratorPrivate(object))
{
}

/*!
    Destroys the iterator.
*/
QJSValueIterator::~QJSValueIterator()
{
}

/*!
    Returns true if there is at least one item ahead of the iterator
    (i.e. the iterator is \e not at the back of the property sequence);
    otherwise returns false.

    \sa next()
*/
bool QJSValueIterator::hasNext() const
{
    if (!d_ptr->isValid())
        return false;
    return QV4::PropertyKey::fromId(d_ptr->nextKey.value()).isValid();
}

/*!
    Advances the iterator by one position.
    Returns true if there was at least one item ahead of the iterator
    (i.e. the iterator was \e not already at the back of the property sequence);
    otherwise returns false.

    \sa hasNext(), name()
*/
bool QJSValueIterator::next()
{
    if (!d_ptr->isValid())
        return false;
    d_ptr->next();
    return QV4::PropertyKey::fromId(d_ptr->currentKey.value()).isValid();
}

/*!
    Returns the name of the last property that was jumped over using
    next().

    \sa value()
*/
QString QJSValueIterator::name() const
{
    if (!d_ptr->isValid())
        return QString();
    QV4::Scope scope(d_ptr->engine);
    QV4::ScopedPropertyKey key(scope, QV4::PropertyKey::fromId(d_ptr->currentKey.value()));
    if (!key->isValid())
        return QString();
    Q_ASSERT(!key->isSymbol());
    return key->toStringOrSymbol(d_ptr->engine)->toQString();
}


/*!
    Returns the value of the last property that was jumped over using
    next().

    \sa name()
*/
QJSValue QJSValueIterator::value() const
{
    if (!d_ptr->isValid())
        return QJSValue();
    QV4::Scope scope(d_ptr->engine);
    QV4::ScopedPropertyKey key(scope, QV4::PropertyKey::fromId(d_ptr->currentKey.value()));
    if (!key->isValid())
        return QJSValue();

    QV4::ScopedObject obj(scope, d_ptr->object.asManaged());
    QV4::ScopedValue val(scope, obj->get(key));

    if (scope.hasException()) {
        scope.engine->catchException();
        return QJSValue();
    }
    return QJSValuePrivate::fromReturnedValue(val->asReturnedValue());
}


/*!
    Makes the iterator operate on \a object. The iterator is set to be
    at the front of the sequence of properties (before the first
    property).
*/
QJSValueIterator& QJSValueIterator::operator=(QJSValue& object)
{
    d_ptr->init(object);
    return *this;
}

QT_END_NAMESPACE
