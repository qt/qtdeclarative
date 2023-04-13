// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmllist.h"
#include "qqmllist_p.h"
#include <QtQml/private/qqmlproperty_p.h>

QT_BEGIN_NAMESPACE

static bool isObjectCompatible(QObject *object, QQmlListReferencePrivate *d)
{
    if (object) {
        const QQmlMetaObject elementType = d->elementType();
        if (elementType.isNull() || !QQmlMetaObject::canConvert(object, elementType))
            return false;
    }
    return true;
}

QQmlListReferencePrivate::QQmlListReferencePrivate()
: refCount(1)
{
}

QQmlListReference QQmlListReferencePrivate::init(
        const QQmlListProperty<QObject> &prop, QMetaType propType)
{
    QQmlListReference rv;

    if (!prop.object) return rv;

    rv.d = new QQmlListReferencePrivate;
    rv.d->object = prop.object;
    rv.d->property = prop;
    rv.d->propertyType = propType;

    return rv;
}

void QQmlListReferencePrivate::addref()
{
    Q_ASSERT(refCount > 0);
    ++refCount;
}

void QQmlListReferencePrivate::release()
{
    Q_ASSERT(refCount > 0);
    --refCount;
    if (!refCount)
        delete this;
}

/*!
\class QQmlListReference
\since 5.0
\inmodule QtQml
\brief The QQmlListReference class allows the manipulation of QQmlListProperty properties.

QQmlListReference allows C++ programs to read from, and assign values to a QML list property in a
simple and type-safe way.  A QQmlListReference can be created by passing an object and property
name or through a QQmlProperty instance.  These two are equivalent:

\code
QQmlListReference ref1(object, "children");

QQmlProperty ref2(object, "children");
QQmlListReference ref2 = qvariant_cast<QQmlListReference>(ref2.read());
\endcode

Not all QML list properties support all operations.  A set of methods, canAppend(), canAt(), canClear() and
canCount() allow programs to query whether an operation is supported on a given property.

QML list properties are type-safe.  Only QObject's that derive from the correct base class can be assigned to
the list.  The listElementType() method can be used to query the QMetaObject of the QObject type supported.
Attempting to add objects of the incorrect type to a list property will fail.

Like with normal lists, when accessing a list element by index, it is the callers responsibility to ensure
that it does not request an out of range element using the count() method before calling at().
*/

/*!
Constructs an invalid instance.
*/
QQmlListReference::QQmlListReference()
: d(nullptr)
{
}

#if QT_DEPRECATED_SINCE(6, 4)
/*!
\since 6.1
\obsolete [6.4] Use the constructors without QQmlEngine argument instead.

Constructs a QQmlListReference from a QVariant \a variant containing a QQmlListProperty. If
\a variant does not contain a list property, an invalid QQmlListReference is created. If the object
owning the list property is destroyed after the reference is constructed, it will automatically
become invalid.  That is, it is safe to hold QQmlListReference instances even after the object is
deleted.

The \a engine is unused.
*/
QQmlListReference::QQmlListReference(const QVariant &variant, [[maybe_unused]] QQmlEngine *engine)
    : QQmlListReference(variant)
{}

/*!
\obsolete [6.4] Use the constructors without QQmlEngine argument instead.

Constructs a QQmlListReference for \a object's \a property.  If \a property is not a list
property, an invalid QQmlListReference is created.  If \a object is destroyed after
the reference is constructed, it will automatically become invalid.  That is, it is safe to hold
QQmlListReference instances even after \a object is deleted.

The \a engine is unused.
*/
QQmlListReference::QQmlListReference(QObject *object, const char *property,
                                     [[maybe_unused]] QQmlEngine *engine)
    : QQmlListReference(object, property)
{}
#endif

/*!
\since 6.1

Constructs a QQmlListReference from a QVariant \a variant containing a QQmlListProperty. If
\a variant does not contain a list property, an invalid QQmlListReference is created. If the object
owning the list property is destroyed after the reference is constructed, it will automatically
become invalid.  That is, it is safe to hold QQmlListReference instances even after the object is
deleted.
*/
QQmlListReference::QQmlListReference(const QVariant &variant)
    : d(nullptr)
{
    const QMetaType t = variant.metaType();
    if (!(t.flags() & QMetaType::IsQmlList))
        return;

    d = new QQmlListReferencePrivate;
    d->propertyType = t;

    d->property.~QQmlListProperty();
    t.construct(&d->property, variant.constData());

    d->object = d->property.object;
}

/*!
Constructs a QQmlListReference for \a object's \a property.  If \a property is not a list
property, an invalid QQmlListReference is created.  If \a object is destroyed after
the reference is constructed, it will automatically become invalid.  That is, it is safe to hold
QQmlListReference instances even after \a object is deleted.
*/
QQmlListReference::QQmlListReference(QObject *object, const char *property)
    : d(nullptr)
{
    if (!object || !property) return;

    QQmlPropertyData local;
    const QQmlPropertyData *data =
        QQmlPropertyCache::property(object, QLatin1String(property), nullptr, &local);

    if (!data || !data->isQList()) return;

    d = new QQmlListReferencePrivate;
    d->object = object;
    d->propertyType = data->propType();

    void *args[] = { &d->property, nullptr };
    QMetaObject::metacall(object, QMetaObject::ReadProperty, data->coreIndex(), args);
}

/*! \internal */
QQmlListReference::QQmlListReference(const QQmlListReference &o)
: d(o.d)
{
    if (d) d->addref();
}

/*! \internal */
QQmlListReference &QQmlListReference::operator=(const QQmlListReference &o)
{
    if (o.d) o.d->addref();
    if (d) d->release();
    d = o.d;
    return *this;
}

/*! \internal */
QQmlListReference::~QQmlListReference()
{
    if (d) d->release();
}

/*!
Returns true if the instance refers to a valid list property, otherwise false.
*/
bool QQmlListReference::isValid() const
{
    return d && d->object;
}

/*!
Returns the list property's object. Returns \nullptr if the reference is invalid.
*/
QObject *QQmlListReference::object() const
{
    if (isValid()) return d->object;
    else return nullptr;
}

/*!
Returns the QMetaObject for the elements stored in the list property,
or \nullptr if the reference is invalid.

The QMetaObject can be used ahead of time to determine whether a given instance can be added
to a list. If you didn't pass an engine on construction this may return nullptr.
*/
const QMetaObject *QQmlListReference::listElementType() const
{
    return isValid() ? d->elementType() : nullptr;
}

/*!
Returns true if the list property can be appended to, otherwise false.  Returns false if the
reference is invalid.

\sa append()
*/
bool QQmlListReference::canAppend() const
{
    return (isValid() && d->property.append);
}

/*!
Returns true if the list property can queried by index, otherwise false.  Returns false if the
reference is invalid.

\sa at()
*/
bool QQmlListReference::canAt() const
{
    return (isValid() && d->property.at);
}

/*!
Returns true if the list property can be cleared, otherwise false.  Returns false if the
reference is invalid.

\sa clear()
*/
bool QQmlListReference::canClear() const
{
    return (isValid() && d->property.clear);
}

/*!
Returns true if the list property can be queried for its element count, otherwise false.
Returns false if the reference is invalid.

\sa count()
*/
bool QQmlListReference::canCount() const
{
    return (isValid() && d->property.count);
}

/*!
Returns true if items in the list property can be replaced, otherwise false.
Returns false if the reference is invalid.

\sa replace()
*/
bool QQmlListReference::canReplace() const
{
    return (isValid() && d->property.replace);
}

/*!
Returns true if the last item can be removed from the list property, otherwise false.
Returns false if the reference is invalid.

\sa removeLast()
*/
bool QQmlListReference::canRemoveLast() const
{
    return (isValid() && d->property.removeLast);
}

/*!
    Return true if at(), count(), append(), and either clear() or removeLast()
    are implemented, so you can manipulate the list.

    Mind that replace() and removeLast() can be emulated by stashing all
    items and rebuilding the list using clear() and append(). Therefore,
    they are not required for the list to be manipulable. Furthermore,
    clear() can be emulated using removeLast().

\sa isReadable(), at(), count(), append(), clear(), replace(), removeLast()
*/
bool QQmlListReference::isManipulable() const
{
    return (isValid()
            && d->property.append
            && d->property.count
            && d->property.at
            && d->property.clear);
}


/*!
    Return true if at() and count() are implemented, so you can access the elements.

\sa isManipulable(), at(), count()
*/
bool QQmlListReference::isReadable() const
{
    return (isValid() && d->property.count && d->property.at);
}

/*!
Appends \a object to the list.  Returns true if the operation succeeded, otherwise false.

\sa canAppend()
*/
bool QQmlListReference::append(QObject *object) const
{
    if (!canAppend()) return false;

    if (!isObjectCompatible(object, d))
        return false;

    d->property.append(&d->property, object);

    return true;
}

/*!
Returns the list element at \a index, or 0 if the operation failed.

\sa canAt()
*/
QObject *QQmlListReference::at(qsizetype index) const
{
    if (!canAt()) return nullptr;

    return d->property.at(&d->property, index);
}

/*!
Clears the list.  Returns true if the operation succeeded, otherwise false.

\sa canClear()
*/
bool QQmlListReference::clear() const
{
    if (!canClear()) return false;

    d->property.clear(&d->property);

    return true;
}

/*!
Returns the number of objects in the list, or 0 if the operation failed.
*/
qsizetype QQmlListReference::count() const
{
    if (!canCount()) return 0;

    return d->property.count(&d->property);
}

/*!
\fn qsizetype QQmlListReference::size() const
\since 6.2
Returns the number of objects in the list, or 0 if the operation failed.
*/

/*!
Replaces the item at \a index in the list with \a object.
Returns true if the operation succeeded, otherwise false.

\sa canReplace()
*/
bool QQmlListReference::replace(qsizetype index, QObject *object) const
{
    if (!canReplace())
        return false;

    if (!isObjectCompatible(object, d))
        return false;

    d->property.replace(&d->property, index, object);
    return true;
}

/*!
Removes the last item in the list.
Returns true if the operation succeeded, otherwise false.

\sa canRemoveLast()
*/
bool QQmlListReference::removeLast() const
{
    if (!canRemoveLast())
        return false;

    d->property.removeLast(&d->property);
    return true;
}

/*!
\class QQmlListProperty
\since 5.0
\inmodule QtQml
\brief The QQmlListProperty class allows applications to expose list-like
properties of QObject-derived classes to QML.

QML has many list properties, where more than one object value can be assigned.
The use of a list property from QML looks like this:

\code
FruitBasket {
    fruit: [
        Apple {},
        Orange{},
        Banana{}
    ]
}
\endcode

The QQmlListProperty encapsulates a group of function pointers that represent the
set of actions QML can perform on the list - adding items, retrieving items and
clearing the list.  In the future, additional operations may be supported.  All
list properties must implement the append operation, but the rest are optional.

To provide a list property, a C++ class must implement the operation callbacks,
and then return an appropriate QQmlListProperty value from the property getter.
List properties should have no setter.  In the example above, the Q_PROPERTY()
declarative will look like this:

\code
Q_PROPERTY(QQmlListProperty<Fruit> fruit READ fruit)
\endcode

QML list properties are type-safe - in this case \c {Fruit} is a QObject type that
\c {Apple}, \c {Orange} and \c {Banana} all derive from.

\sa {Chapter 5: Using List Property Types}
*/

/*!
    \macro QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_APPEND
    \relates QQmlListProperty

    This macro defines the behavior of the list properties of this class to Append.
    When assigning the property in a derived type, the values are appended
    to those of the base class. This is the default behavior.

    \snippet code/src_qml_qqmllist.cpp 0

    \sa QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE_IF_NOT_DEFAULT
    \sa QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE
    \sa {Defining Object Types through QML Documents}
*/

/*!
    \macro QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE_IF_NOT_DEFAULT
    \relates QQmlListProperty

    This macro defines the behavior of the list properties of this class to
    ReplaceIfNotDefault.
    When assigning the property in a derived type, the values replace those of
    the base class unless it's the default property.
    In the case of the default property, values are appended to those of the base class.

    \snippet code/src_qml_qqmllist.cpp 1

    \sa QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_APPEND
    \sa QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE
    \sa {Defining Object Types through QML Documents}
*/

/*!
    \macro QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE
    \relates QQmlListProperty

    This macro defines the behavior of the list properties of this class to Replace.
    When assigning the property in a derived type, the values replace those
    of the base class.

    \snippet code/src_qml_qqmllist.cpp 2

    \sa QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_APPEND
    \sa QML_LIST_PROPERTY_ASSIGN_BEHAVIOR_REPLACE_IF_NOT_DEFAULT
    \sa {Defining Object Types through QML Documents}
*/

/*!
\fn template<typename T> QQmlListProperty<T>::QQmlListProperty()
\internal
*/

/*!
\fn template<typename T> QQmlListProperty<T>::QQmlListProperty(QObject *object, QList<T *> &list)
\deprecated

Convenience constructor for making a QQmlListProperty value from an existing
QList \a list.  The \a list reference must remain valid for as long as \a object
exists.  \a object must be provided.

This constructor synthesizes the removeLast() and replace() methods
introduced in Qt 5.15, using count(), at(), clear(), and append(). This is slow.
If you intend to manipulate the list beyond clearing it, you should explicitly
provide these methods.
*/

/*!
\fn template<typename T> QQmlListProperty<T>::QQmlListProperty(QObject *object, QList<T *> *list)
\since 5.15

Convenience constructor for making a QQmlListProperty value from an existing
QList \a list. The \a list reference must remain valid for as long as \a object
exists. \a object must be provided.
*/

/*!
\fn template<typename T> QQmlListProperty<T>::QQmlListProperty(QObject *object, void *data,
                                    CountFunction count, AtFunction at)

Construct a readonly QQmlListProperty from a set of operation functions
\a count and \a at. An opaque \a data handle may be passed which can be
accessed from within the operation functions.  The list property
remains valid while \a object exists.
*/

/*!
\fn template<typename T> QQmlListProperty<T>::QQmlListProperty(QObject *object, void *data, AppendFunction append,
                                     CountFunction count, AtFunction at,
                                     ClearFunction clear)

Construct a QQmlListProperty from a set of operation functions \a append,
\a count, \a at, and \a clear.  An opaque \a data handle may be passed which
can be accessed from within the operation functions.  The list property
remains valid while \a object exists.

Null pointers can be passed for any function. If any null pointers are passed in, the list
will be neither designable nor alterable by the debugger. It is recommended to provide valid
pointers for all functions.

\note The resulting QQmlListProperty will synthesize the removeLast() and
replace() methods using \a count, \a at, \a clear, and \a append if all of those
are given. This is slow. If you intend to manipulate the list beyond clearing it,
you should explicitly provide these methods.
*/

/*!
\fn template<typename T> QQmlListProperty<T>::QQmlListProperty(
        QObject *object, void *data, AppendFunction append, CountFunction count,
        AtFunction at, ClearFunction clear, ReplaceFunction replace,
        RemoveLastFunction removeLast)

Construct a QQmlListProperty from a set of operation functions \a append,
\a count, \a at, \a clear, \a replace, and \removeLast. An opaque \a data handle
may be passed which can be accessed from within the operation functions. The
list property remains valid while \a object exists.

Null pointers can be passed for any function, causing the respective function to
be synthesized using the others, if possible. QQmlListProperty can synthesize
\list
    \li \a clear using \a count and \a removeLast
    \li \a replace using \a count, \a at, \a clear, and \a append
    \li \a replace using \a count, \a at, \a removeLast, and \a append
    \li \a removeLast using \a count, \a at, \a clear, and \a append
\endlist
if those are given. This is slow, but if your list does not natively provide
faster options for these primitives, you may want to use the synthesized ones.

Furthermore, if either of \a count, \a at, \a append, and \a clear are neither
given explicitly nor synthesized, the list will be neither designable nor
alterable by the debugger. It is recommended to provide enough valid pointers
to avoid this situation.
*/

/*!
\typedef QQmlListProperty::AppendFunction

Synonym for \c {void (*)(QQmlListProperty<T> *property, T *value)}.

Append the \a value to the list \a property.
*/

/*!
\typedef QQmlListProperty::CountFunction

Synonym for \c {qsizetype (*)(QQmlListProperty<T> *property)}.

Return the number of elements in the list \a property.
*/

/*!
\fn template<typename T> bool QQmlListProperty<T>::operator==(const QQmlListProperty &other) const

Returns true if this QQmlListProperty is equal to \a other, otherwise false.
*/

/*!
\typedef QQmlListProperty::AtFunction

Synonym for \c {T *(*)(QQmlListProperty<T> *property, qsizetype index)}.

Return the element at position \a index in the list \a property.
*/

/*!
\typedef QQmlListProperty::ClearFunction

Synonym for \c {void (*)(QQmlListProperty<T> *property)}.

Clear the list \a property.
*/

/*!
\typedef QQmlListProperty::ReplaceFunction

Synonym for \c {void (*)(QQmlListProperty<T> *property, qsizetype index, T *value)}.

Replace the element at position \a index in the list \a property with \a value.
*/

/*!
\typedef QQmlListProperty::RemoveLastFunction

Synonym for \c {void (*)(QQmlListProperty<T> *property)}.

Remove the last element from the list \a property.
*/

/*!
\fn bool QQmlListReference::operator==(const QQmlListReference &other) const

Compares this QQmlListReference to \a other, and returns \c true if they are
equal. The two are only considered equal if one was created from the other
via copy assignment or copy construction.

\note Independently created references to the same object are not considered
to be equal.
*/

QT_END_NAMESPACE
