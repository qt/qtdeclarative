/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtScript module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL-ONLY$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qjsvalueiterator.h"
#include "qjsvalueiterator_p.h"

#include "qscriptisolate_p.h"
#include "qjsvalue_p.h"
#include "qv8engine_p.h"
#include "qscript_impl_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QJSValueIterator

    \brief The QJSValueIterator class provides a Java-style iterator for QJSValue.

    \ingroup script


    The QJSValueIterator constructor takes a QJSValue as
    argument.  After construction, the iterator is located at the very
    beginning of the sequence of properties. Here's how to iterate over
    all the properties of a QJSValue:

    \snippet doc/src/snippets/code/src_script_QJSValueIterator.cpp 0

    The next() advances the iterator. The name(), value() and flags()
    functions return the name, value and flags of the last item that was
    jumped over.

    If you want to remove properties as you iterate over the
    QJSValue, use remove(). If you want to modify the value of a
    property, use setValue().

    Note that QJSValueIterator only iterates over the QJSValue's
    own properties; i.e. it does not follow the prototype chain. You can
    use a loop like this to follow the prototype chain:

    \snippet doc/src/snippets/code/src_script_QJSValueIterator.cpp 1

    Note that QJSValueIterator will not automatically skip over
    properties that have the QJSValue::SkipInEnumeration flag set;
    that flag only affects iteration in script code.  If you want, you
    can skip over such properties with code like the following:

    \snippet doc/src/snippets/code/src_script_QJSValueIterator.cpp 2

    \sa QJSValue::property()
*/

/*!
    Constructs an iterator for traversing \a object. The iterator is
    set to be at the front of the sequence of properties (before the
    first property).
*/
QJSValueIterator::QJSValueIterator(const QJSValue& object)
    : d_ptr(new QJSValueIteratorPrivate(QJSValuePrivate::get(object)))
{}

/*!
    Destroys the iterator.
*/
QJSValueIterator::~QJSValueIterator()
{}

/*!
    Returns true if there is at least one item ahead of the iterator
    (i.e. the iterator is \e not at the back of the property sequence);
    otherwise returns false.

    \sa next(), hasPrevious()
*/
bool QJSValueIterator::hasNext() const
{
    Q_D(const QJSValueIterator);
    QScriptIsolate api(d->engine());
    return d->hasNext();
}

/*!
    Advances the iterator by one position.

    Calling this function on an iterator located at the back of the
    container leads to undefined results.

    \sa hasNext(), previous(), name()
*/
bool QJSValueIterator::next()
{
    Q_D(QJSValueIterator);
    QScriptIsolate api(d->engine());
    return d->next();
}

/*!
    Returns the name of the last property that was jumped over using
    next() or previous().

    \sa value(), flags()
*/
QString QJSValueIterator::name() const
{
    Q_D(const QJSValueIterator);
    QScriptIsolate api(d->engine());
    return d_ptr->name();
}


/*!
    Returns the value of the last property that was jumped over using
    next() or previous().

    \sa setValue(), name()
*/
QJSValue QJSValueIterator::value() const
{
    Q_D(const QJSValueIterator);
    QScriptIsolate api(d->engine());
    return QJSValuePrivate::get(d->value());
}


/*!
    Makes the iterator operate on \a object. The iterator is set to be
    at the front of the sequence of properties (before the first
    property).
*/
QJSValueIterator& QJSValueIterator::operator=(QJSValue& object)
{
    Q_D(QJSValueIterator);
    QScriptIsolate api(d->engine());
    d_ptr.reset(new QJSValueIteratorPrivate(QJSValuePrivate::get(object)));
    return *this;
}

QT_END_NAMESPACE
