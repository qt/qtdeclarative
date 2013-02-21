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

#include "qscriptisolate_p.h"
#include "qjsengine.h"
#include "qv8engine_p.h"
#include "qjsvalue.h"
#include "qjsvalue_p.h"
#include "qscript_impl_p.h"
#include "qscriptshareddata_p.h"
#include <QtCore/qstring.h>

/*!
  \since 5.0
  \class QJSValue

  \brief The QJSValue class acts as a container for Qt/JavaScript data types.

  \ingroup qtjavascript
  \inmodule QtQml
  \mainclass

  QJSValue supports the types defined in the \l{ECMA-262}
  standard: The primitive types, which are Undefined, Null, Boolean,
  Number, and String; and the Object type. Additionally, built-in
  support is provided for Qt/C++ types such as QVariant and QObject.

  For the object-based types (including Date and RegExp), use the
  newT() functions in QJSEngine (e.g. QJSEngine::newObject())
  to create a QJSValue of the desired type. For the primitive types,
  use one of the QJSValue constructor overloads.

  The methods named isT() (e.g. isBool(), isUndefined()) can be
  used to test if a value is of a certain type. The methods named
  toT() (e.g. toBool(), toString()) can be used to convert a
  QJSValue to another type. You can also use the generic
  QJSValue_cast() function.

  Object values have zero or more properties which are themselves
  QJSValues. Use setProperty() to set a property of an object, and
  call property() to retrieve the value of a property.

  \snippet code/src_script_qjsvalue.cpp 0

  If you want to iterate over the properties of a script object, use
  the QJSValueIterator class.

  Object values have an internal \c{prototype} property, which can be
  accessed with prototype() and setPrototype().

  Function objects (objects for which isCallable()) returns true) can
  be invoked by calling call(). Constructor functions can be used to
  construct new objects by calling callAsConstructor().

  Use equals() or strictlyEquals() to compare a QJSValue to another.

  Note that a QJSValue for which isObject() is true only carries a
  reference to an actual object; copying the QJSValue will only
  copy the object reference, not the object itself. If you want to
  clone an object (i.e. copy an object's properties to another
  object), you can do so with the help of a \c{for-in} statement in
  script code, or QJSValueIterator in C++.

  \sa QJSEngine, QJSValueIterator
*/

/*!
    \enum QJSValue::SpecialValue

    This enum is used to specify a single-valued type.

    \value UndefinedValue An undefined value.

    \value NullValue A null value.
*/

QT_BEGIN_NAMESPACE

/*!
  Constructs a new QJSValue with a boolean \a value.
*/
QJSValue::QJSValue(bool value)
    : d_ptr(new QJSValuePrivate(value))
{
}

/*!
  Constructs a new QJSValue with a number \a value.
*/
QJSValue::QJSValue(int value)
    : d_ptr(new QJSValuePrivate(value))
{
}

/*!
  Constructs a new QJSValue with a number \a value.
*/
QJSValue::QJSValue(uint value)
    : d_ptr(new QJSValuePrivate(value))
{
}

/*!
  Constructs a new QJSValue with a number \a value.
*/
QJSValue::QJSValue(double value)
    : d_ptr(new QJSValuePrivate(value))
{
}

/*!
  Constructs a new QJSValue with a string \a value.
*/
QJSValue::QJSValue(const QString& value)
    : d_ptr(new QJSValuePrivate(value))
{
}

/*!
  Constructs a new QJSValue with a special \a value.
*/
QJSValue::QJSValue(SpecialValue value)
    : d_ptr(new QJSValuePrivate(value))
{
}

/*!
  Constructs a new QJSValue with a string \a value.
*/
QJSValue::QJSValue(const QLatin1String &value)
    : d_ptr(new QJSValuePrivate(value))
{
}

/*!
  Constructs a new QJSValue with a string \a value.
*/
#ifndef QT_NO_CAST_FROM_ASCII
QJSValue::QJSValue(const char *value)
    : d_ptr(new QJSValuePrivate(QString::fromLatin1(value)))
{
}
#endif

/*!
    Constructs a new QJSValue from private
    \internal
*/
QJSValue::QJSValue(QJSValuePrivate* d)
    : d_ptr(d)
{
}

/*!
    Constructs a new QJSValue from private
    \internal
*/
QJSValue::QJSValue(QScriptPassPointer<QJSValuePrivate> d)
    : d_ptr(d.give())
{
}

/*!
  Constructs a new QJSValue that is a copy of \a other.

  Note that if \a other is an object (i.e., isObject() would return
  true), then only a reference to the underlying object is copied into
  the new script value (i.e., the object itself is not copied).
*/
QJSValue::QJSValue(const QJSValue& other)
    : d_ptr(other.d_ptr)
{
}

/*!
    Destroys this QJSValue.
*/
QJSValue::~QJSValue()
{
}

/*!
  Returns true if this QJSValue is of the primitive type Boolean;
  otherwise returns false.

  \sa toBool()
*/
bool QJSValue::isBool() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isBool();
}

/*!
  Returns true if this QJSValue is of the primitive type Number;
  otherwise returns false.

  \sa toNumber()
*/
bool QJSValue::isNumber() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isNumber();
}

/*!
  Returns true if this QJSValue is of the primitive type Null;
  otherwise returns false.
*/
bool QJSValue::isNull() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isNull();
}

/*!
  Returns true if this QJSValue is of the primitive type String;
  otherwise returns false.

  \sa toString()
*/
bool QJSValue::isString() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isString();
}

/*!
  Returns true if this QJSValue is of the primitive type Undefined;
  otherwise returns false.
*/
bool QJSValue::isUndefined() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isUndefined();
}

/*!
  Returns true if this QJSValue is an object of the Error class;
  otherwise returns false.
*/
bool QJSValue::isError() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isError();
}

/*!
  Returns true if this QJSValue is an object of the Array class;
  otherwise returns false.

  \sa QJSEngine::newArray()
*/
bool QJSValue::isArray() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isArray();
 }

/*!
  Returns true if this QJSValue is of the Object type; otherwise
  returns false.

  Note that function values, variant values, and QObject values are
  objects, so this function returns true for such values.

  \sa QJSEngine::newObject()
*/
bool QJSValue::isObject() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isObject();
}

/*!
  Returns true if this QJSValue can be called a function, otherwise
  returns false.

  \sa call()
*/
bool QJSValue::isCallable() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isCallable();
}

/*!
  Returns true if this QJSValue is a variant value;
  otherwise returns false.

  \sa toVariant()
*/
bool QJSValue::isVariant() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isVariant();
}

/*!
  Returns the string value of this QJSValue, as defined in
  \l{ECMA-262} section 9.8, "ToString".

  Note that if this QJSValue is an object, calling this function
  has side effects on the script engine, since the engine will call
  the object's toString() function (and possibly valueOf()) in an
  attempt to convert the object to a primitive value (possibly
  resulting in an uncaught script exception).

  \sa isString()
*/
QString QJSValue::toString() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toString();
}

/*!
  Returns the number value of this QJSValue, as defined in
  \l{ECMA-262} section 9.3, "ToNumber".

  Note that if this QJSValue is an object, calling this function
  has side effects on the script engine, since the engine will call
  the object's valueOf() function (and possibly toString()) in an
  attempt to convert the object to a primitive value (possibly
  resulting in an uncaught script exception).

  \sa isNumber(), toInt(), toUInt()
*/
double QJSValue::toNumber() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toNumber();
}

/*!
  Returns the boolean value of this QJSValue, using the conversion
  rules described in \l{ECMA-262} section 9.2, "ToBoolean".

  Note that if this QJSValue is an object, calling this function
  has side effects on the script engine, since the engine will call
  the object's valueOf() function (and possibly toString()) in an
  attempt to convert the object to a primitive value (possibly
  resulting in an uncaught script exception).

  \sa isBool()
*/
bool QJSValue::toBool() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toBool();
}

/*!
  Returns the signed 32-bit integer value of this QJSValue, using
  the conversion rules described in \l{ECMA-262} section 9.5, "ToInt32".

  Note that if this QJSValue is an object, calling this function
  has side effects on the script engine, since the engine will call
  the object's valueOf() function (and possibly toString()) in an
  attempt to convert the object to a primitive value (possibly
  resulting in an uncaught script exception).

  \sa toNumber(), toUInt()
*/
qint32 QJSValue::toInt() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toInt32();
}

/*!
  Returns the unsigned 32-bit integer value of this QJSValue, using
  the conversion rules described in \l{ECMA-262} section 9.6, "ToUint32".

  Note that if this QJSValue is an object, calling this function
  has side effects on the script engine, since the engine will call
  the object's valueOf() function (and possibly toString()) in an
  attempt to convert the object to a primitive value (possibly
  resulting in an uncaught script exception).

  \sa toNumber(), toInt()
*/
quint32 QJSValue::toUInt() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toUInt32();
}

/*!
  Returns the QVariant value of this QJSValue, if it can be
  converted to a QVariant; otherwise returns an invalid QVariant.
  The conversion is performed according to the following table:

    \table
    \header \li Input Type \li Result
    \row    \li Undefined  \li An invalid QVariant.
    \row    \li Null       \li A QVariant containing a null pointer (QMetaType::VoidStar).
    \row    \li Boolean    \li A QVariant containing the value of the boolean.
    \row    \li Number     \li A QVariant containing the value of the number.
    \row    \li String     \li A QVariant containing the value of the string.
    \row    \li QVariant Object \li The result is the QVariant value of the object (no conversion).
    \row    \li QObject Object \li A QVariant containing a pointer to the QObject.
    \row    \li Date Object \li A QVariant containing the date value (toDateTime()).
    \row    \li RegExp Object \li A QVariant containing the regular expression value.
    \row    \li Array Object \li The array is converted to a QVariantList. Each element is converted to a QVariant, recursively; cyclic references are not followed.
    \row    \li Object     \li The object is converted to a QVariantMap. Each property is converted to a QVariant, recursively; cyclic references are not followed.
    \endtable

  \sa isVariant()
*/
QVariant QJSValue::toVariant() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toVariant();
}

/*!
  Calls this QJSValue as a function, passing \a args as arguments
  to the function, and using the globalObject() as the "this"-object.
  Returns the value returned from the function.

  If this QJSValue is not callable, call() does nothing and
  returns an undefined QJSValue.

  Calling call() can cause an exception to occur in the script engine;
  in that case, call() returns the value that was thrown (typically an
  \c{Error} object). You can call isError() on the return value to
  determine whether an exception occurred.

  \sa isCallable(), callWithInstance(), callAsConstructor()
*/
QJSValue QJSValue::call(const QJSValueList &args)
{
    Q_D(QJSValue);
    QScriptIsolate api(d->engine());
    return d->call(/*thisObject=*/0, args);
}

/*!
  Calls this QJSValue as a function, using \a instance as
  the `this' object in the function call, and passing \a args
  as arguments to the function. Returns the value returned from
  the function.

  If this QJSValue is not a function, call() does nothing
  and returns an undefined QJSValue.

  Note that if \a instance is not an object, the global object
  (see \l{QJSEngine::globalObject()}) will be used as the
  `this' object.

  Calling call() can cause an exception to occur in the script engine;
  in that case, call() returns the value that was thrown (typically an
  \c{Error} object). You can call isError() on the return value to
  determine whether an exception occurred.

  \sa call()
*/
QJSValue QJSValue::callWithInstance(const QJSValue &instance, const QJSValueList &args)
{
    Q_D(QJSValue);
    QScriptIsolate api(d->engine());
    return d->call(QJSValuePrivate::get(instance), args);
}

/*!
  Creates a new \c{Object} and calls this QJSValue as a
  constructor, using the created object as the `this' object and
  passing \a args as arguments. If the return value from the
  constructor call is an object, then that object is returned;
  otherwise the default constructed object is returned.

  If this QJSValue is not a function, callAsConstructor() does
  nothing and returns an undefined QJSValue.

  Calling this function can cause an exception to occur in the
  script engine; in that case, the value that was thrown
  (typically an \c{Error} object) is returned. You can call
  isError() on the return value to determine whether an
  exception occurred.

  \sa call(), QJSEngine::newObject()
*/
QJSValue QJSValue::callAsConstructor(const QJSValueList &args)
{
    Q_D(QJSValue);
    QScriptIsolate api(d->engine());
    return QJSValuePrivate::get(d->callAsConstructor(args));
}

#ifdef QT_DEPRECATED

/*!
  \obsolete

  Returns the QJSEngine that created this QJSValue,
  or 0 if this QJSValue is invalid or the value is not
  associated with a particular engine.
*/
QJSEngine* QJSValue::engine() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    QV8Engine* engine = d->engine();
    if (engine)
        return QV8Engine::get(engine);
    return 0;
}

#endif // QT_DEPRECATED

/*!
  If this QJSValue is an object, returns the internal prototype
  (\c{__proto__} property) of this object; otherwise returns an
  undefined QJSValue.

  \sa setPrototype(), isObject()
*/
QJSValue QJSValue::prototype() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return QJSValuePrivate::get(d->prototype());
}

/*!
  If this QJSValue is an object, sets the internal prototype
  (\c{__proto__} property) of this object to be \a prototype;
  otherwise does nothing.

  The internal prototype should not be confused with the public
  property with name "prototype"; the public prototype is usually
  only set on functions that act as constructors.

  \sa prototype(), isObject()
*/
void QJSValue::setPrototype(const QJSValue& prototype)
{
    Q_D(QJSValue);
    QScriptIsolate api(d->engine());
    d->setPrototype(QJSValuePrivate::get(prototype));
}

/*!
  Assigns the \a other value to this QJSValue.

  Note that if \a other is an object (isObject() returns true),
  only a reference to the underlying object will be assigned;
  the object itself will not be copied.
*/
QJSValue& QJSValue::operator=(const QJSValue& other)
{
    d_ptr = other.d_ptr;
    return *this;
}

/*!
  Returns true if this QJSValue is equal to \a other, otherwise
  returns false. The comparison follows the behavior described in
  \l{ECMA-262} section 11.9.3, "The Abstract Equality Comparison
  Algorithm".

  This function can return true even if the type of this QJSValue
  is different from the type of the \a other value; i.e. the
  comparison is not strict.  For example, comparing the number 9 to
  the string "9" returns true; comparing an undefined value to a null
  value returns true; comparing a \c{Number} object whose primitive
  value is 6 to a \c{String} object whose primitive value is "6"
  returns true; and comparing the number 1 to the boolean value
  \c{true} returns true. If you want to perform a comparison
  without such implicit value conversion, use strictlyEquals().

  Note that if this QJSValue or the \a other value are objects,
  calling this function has side effects on the script engine, since
  the engine will call the object's valueOf() function (and possibly
  toString()) in an attempt to convert the object to a primitive value
  (possibly resulting in an uncaught script exception).

  \sa strictlyEquals()
*/
bool QJSValue::equals(const QJSValue& other) const
{
    Q_D(const QJSValue);
    QJSValuePrivate* otherValue = QJSValuePrivate::get(other);
    QScriptIsolate api(d->engine() ? d->engine() : otherValue->engine());
    return d_ptr->equals(otherValue);
}

/*!
  Returns true if this QJSValue is equal to \a other using strict
  comparison (no conversion), otherwise returns false. The comparison
  follows the behavior described in \l{ECMA-262} section 11.9.6, "The
  Strict Equality Comparison Algorithm".

  If the type of this QJSValue is different from the type of the
  \a other value, this function returns false. If the types are equal,
  the result depends on the type, as shown in the following table:

    \table
    \header \li Type \li Result
    \row    \li Undefined  \li true
    \row    \li Null       \li true
    \row    \li Boolean    \li true if both values are true, false otherwise
    \row    \li Number     \li false if either value is NaN (Not-a-Number); true if values are equal, false otherwise
    \row    \li String     \li true if both values are exactly the same sequence of characters, false otherwise
    \row    \li Object     \li true if both values refer to the same object, false otherwise
    \endtable

  \sa equals()
*/
bool QJSValue::strictlyEquals(const QJSValue& other) const
{
    Q_D(const QJSValue);
    QJSValuePrivate* o = QJSValuePrivate::get(other);
    QScriptIsolate api(d->engine() ? d->engine() : o->engine());
    return d_ptr->strictlyEquals(o);
}

/*!
  Returns the value of this QJSValue's property with the given \a name.
  If no such property exists, an undefined QJSValue is returned.

  If the property is implemented using a getter function (i.e. has the
  PropertyGetter flag set), calling property() has side-effects on the
  script engine, since the getter function will be called (possibly
  resulting in an uncaught script exception). If an exception
  occurred, property() returns the value that was thrown (typically
  an \c{Error} object).

  \sa setProperty(), hasProperty(), QJSValueIterator
*/
QJSValue QJSValue::property(const QString& name) const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return QJSValuePrivate::get(d->property(name));
}

/*!
  \overload

  Returns the property at the given \a arrayIndex.

  This function is provided for convenience and performance when
  working with array objects.

  If this QJSValue is not an Array object, this function behaves
  as if property() was called with the string representation of \a
  arrayIndex.
*/
QJSValue QJSValue::property(quint32 arrayIndex) const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return QJSValuePrivate::get(d->property(arrayIndex));
}

/*!
  Sets the value of this QJSValue's property with the given \a name to
  the given \a value.

  If this QJSValue is not an object, this function does nothing.

  If this QJSValue does not already have a property with name \a name,
  a new property is created.

  \sa property(), deleteProperty()
*/
void QJSValue::setProperty(const QString& name, const QJSValue& value)
{
    Q_D(QJSValue);
    QScriptIsolate api(d->engine());
    d->setProperty(name, QJSValuePrivate::get(value));
}

/*!
  \overload

  Sets the property at the given \a arrayIndex to the given \a value.

  This function is provided for convenience and performance when
  working with array objects.

  If this QJSValue is not an Array object, this function behaves
  as if setProperty() was called with the string representation of \a
  arrayIndex.
*/
void QJSValue::setProperty(quint32 arrayIndex, const QJSValue& value)
{
    Q_D(QJSValue);
    QScriptIsolate api(d->engine());
    d->setProperty(arrayIndex, QJSValuePrivate::get(value));
}

/*!
  Attempts to delete this object's property of the given \a name.
  Returns true if the property was deleted, otherwise returns false.

  The behavior of this function is consistent with the JavaScript
  delete operator. In particular:

  \list
  \li Non-configurable properties cannot be deleted.
  \li This function will return true even if this object doesn't
     have a property of the given \a name (i.e., non-existent
     properties are "trivially deletable").
  \li If this object doesn't have an own property of the given
     \a name, but an object in the prototype() chain does, the
     prototype object's property is not deleted, and this function
     returns true.
  \endlist

  \sa setProperty(), hasOwnProperty()
*/
bool QJSValue::deleteProperty(const QString &name)
{
    Q_D(QJSValue);
    QScriptIsolate api(d->engine());
    return d->deleteProperty(name);
}

/*!
  Returns true if this object has a property of the given \a name,
  otherwise returns false.

  \sa property(), hasOwnProperty()
*/
bool QJSValue::hasProperty(const QString &name) const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->hasProperty(name);
}

/*!
  Returns true if this object has an own (not prototype-inherited)
  property of the given \a name, otherwise returns false.

  \sa property(), hasProperty()
*/
bool QJSValue::hasOwnProperty(const QString &name) const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->hasOwnProperty(name);
}

/*!
 * If this QJSValue is a QObject, returns the QObject pointer
 * that the QJSValue represents; otherwise, returns 0.
 *
 * If the QObject that this QJSValue wraps has been deleted,
 * this function returns 0 (i.e. it is possible for toQObject()
 * to return 0 even when isQObject() returns true).
 *
 * \sa isQObject()
 */
QObject *QJSValue::toQObject() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toQObject();
}

/*!
  Returns a QDateTime representation of this value, in local time.
  If this QJSValue is not a date, or the value of the date is NaN
  (Not-a-Number), an invalid QDateTime is returned.

  \sa isDate()
*/
QDateTime QJSValue::toDateTime() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toDataTime();
}

/*!
  Returns true if this QJSValue is an object of the Date class;
  otherwise returns false.

  \sa QJSEngine::newDate()
*/
bool QJSValue::isDate() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isDate();
}

/*!
  Returns true if this QJSValue is an object of the RegExp class;
  otherwise returns false.
*/
bool QJSValue::isRegExp() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isRegExp();
}

/*!
  Returns true if this QJSValue is a QObject; otherwise returns
  false.

  Note: This function returns true even if the QObject that this
  QJSValue wraps has been deleted.

  \sa toQObject(), QJSEngine::newQObject()
*/
bool QJSValue::isQObject() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isQObject();
}

/*!
    \internal

    Returns this value's internal V8 value, or an empty handle if
    the QJSValue isn't bound to a QJSEngine.

    The V8 value enables direct use of the V8 API.
    The caller is responsible for ensuring that a handle scope is in place.
    Example:

    \code
    QJSValue value = ...;
    v8::HandleScope handleScope;
    v8::Local<v8::Value> v8value = qt_QJSValueV8Value(value);

    // Do something with the V8 value (e.g., call v8::Value::IsInt32()) ...
    \endcode

    \sa qt_QJSEngineV8Context()
*/
Q_QML_EXPORT v8::Local<v8::Value> qt_QJSValueV8Value(const QJSValue &value)
{
    QJSValuePrivate *d = QJSValuePrivate::get(value);
    return v8::Local<v8::Value>::New(d->handle());
}

QT_END_NAMESPACE
