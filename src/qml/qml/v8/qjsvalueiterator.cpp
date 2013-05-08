/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qjsvalueiterator.h"
#include "qjsvalueiterator_p.h"
#include "qjsvalue_p.h"
#include "private/qv4string_p.h"
#include "private/qv4object_p.h"

QT_BEGIN_NAMESPACE

QJSValueIteratorPrivate::QJSValueIteratorPrivate(const QJSValue &v)
    : value(v)
    , iterator(QJSValuePrivate::get(v)->value.asObject(), QV4::ObjectIterator::NoFlags)
    , currentValue(0)
    , currentName(0)
    , currentIndex(UINT_MAX)
    , nextValue(0)
    , nextName(0)
    , nextIndex(UINT_MAX)
{
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
    d_ptr->nextValue = d_ptr->iterator.next(&d_ptr->nextName, &d_ptr->nextIndex, &d_ptr->nextAttributes);
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
    if (!QJSValuePrivate::get(d_ptr->value)->value.isObject())
        return false;
    return d_ptr->nextValue != 0;
}

/*!
    Advances the iterator by one position.
    Returns true if there is at least one item ahead of the iterator
    (i.e. the iterator is \e not at the back of the property sequence);
    otherwise returns false.

    Calling this function on an iterator located at the back of the
    container leads to undefined results.

    \sa hasNext(), name()
*/
bool QJSValueIterator::next()
{
    if (!QJSValuePrivate::get(d_ptr->value)->value.isObject())
        return false;
    d_ptr->currentValue = d_ptr->nextValue;
    d_ptr->currentName = d_ptr->nextName;
    d_ptr->currentIndex = d_ptr->nextIndex;
    d_ptr->currentAttributes = d_ptr->nextAttributes;

    d_ptr->nextValue = d_ptr->iterator.next(&d_ptr->nextName, &d_ptr->nextIndex, &d_ptr->nextAttributes);
    return d_ptr->nextValue != 0;
}

/*!
    Returns the name of the last property that was jumped over using
    next().

    \sa value()
*/
QString QJSValueIterator::name() const
{
    if (!QJSValuePrivate::get(d_ptr->value)->value.isObject())
        return false;
    if (d_ptr->currentName)
        return d_ptr->currentName->toQString();
    if (d_ptr->currentIndex < UINT_MAX)
        return QString::number(d_ptr->currentIndex);
    return QString();
}


/*!
    Returns the value of the last property that was jumped over using
    next().

    \sa name()
*/
QJSValue QJSValueIterator::value() const
{
    if (!QJSValuePrivate::get(d_ptr->value)->value.isObject())
        return QJSValue();
    if (!d_ptr->currentValue)
        return QJSValue();

    QV4::Object *o = d_ptr->iterator.object;
    QV4::ExecutionContext *ctx = o->internalClass->engine->current;
    try {
        QV4::Value v = o->getValue(ctx, d_ptr->currentValue, d_ptr->currentAttributes);
        return new QJSValuePrivate(v);
    } catch (QV4::Exception &e) {
        e.accept(ctx);
        return QJSValue();
    }
}


/*!
    Makes the iterator operate on \a object. The iterator is set to be
    at the front of the sequence of properties (before the first
    property).
*/
QJSValueIterator& QJSValueIterator::operator=(QJSValue& object)
{
    d_ptr->iterator = QV4::ObjectIterator(QJSValuePrivate::get(object)->value.asObject(), QV4::ObjectIterator::NoFlags);
    d_ptr->nextValue = d_ptr->iterator.next(&d_ptr->nextName, &d_ptr->nextIndex, &d_ptr->nextAttributes);
    return *this;
}

QT_END_NAMESPACE
