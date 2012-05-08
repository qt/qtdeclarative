/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
** us via http://www.qt-project.org/.
**
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

    \ingroup qtjavascript


    The QJSValueIterator constructor takes a QJSValue as
    argument.  After construction, the iterator is located at the very
    beginning of the sequence of properties. Here's how to iterate over
    all the properties of a QJSValue:

    \snippet doc/snippets/code/src_script_qjsvalueiterator.cpp 0

    The next() advances the iterator. The name() and value()
    functions return the name and value of the last item that was
    jumped over.

    Note that QJSValueIterator only iterates over the QJSValue's
    own properties; i.e. it does not follow the prototype chain. You can
    use a loop like this to follow the prototype chain:

    \snippet doc/snippets/code/src_script_qjsvalueiterator.cpp 1

    Note that QJSValueIterator will not automatically skip over
    properties that have the QJSValue::SkipInEnumeration flag set;
    that flag only affects iteration in script code.  If you want, you
    can skip over such properties with code like the following:

    \snippet doc/snippets/code/src_script_qjsvalueiterator.cpp 2

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

    \sa next()
*/
bool QJSValueIterator::hasNext() const
{
    Q_D(const QJSValueIterator);
    QScriptIsolate api(d->engine());
    return d->hasNext();
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
    Q_D(QJSValueIterator);
    QScriptIsolate api(d->engine());
    return d->next();
}

/*!
    Returns the name of the last property that was jumped over using
    next().

    \sa value()
*/
QString QJSValueIterator::name() const
{
    Q_D(const QJSValueIterator);
    QScriptIsolate api(d->engine());
    return d_ptr->name();
}


/*!
    Returns the value of the last property that was jumped over using
    next().

    \sa name()
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
