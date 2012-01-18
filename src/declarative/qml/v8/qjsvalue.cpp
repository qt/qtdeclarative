/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#include "qscriptisolate_p.h"
#include "qjsengine.h"
#include "qv8engine_p.h"
#include "qjsvalue.h"
#include "qjsvalue_p.h"
#include "qscript_impl_p.h"
#include "qscriptshareddata_p.h"
#include <QtCore/qregexp.h>
#include <QtCore/qstring.h>

/*!
  \since 5.0
  \class QJSValue

  \brief The QJSValue class acts as a container for Qt/JavaScript data types.

  \ingroup qtjavascript
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

  \snippet doc/src/snippets/code/src_script_qjsvalue.cpp 0

  The attributes of a property can be queried by calling the
  propertyFlags() function.

  If you want to iterate over the properties of a script object, use
  the QJSValueIterator class.

  Object values have an internal \c{prototype} property, which can be
  accessed with prototype() and setPrototype().

  Function objects (objects for which isCallable()) returns true) can
  be invoked by calling call(). Constructor functions can be used to
  construct new objects by calling construct().

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

/*!
    \enum QJSValue::PropertyFlag

    This enum describes the attributes of a property.

    \value ReadOnly The property is read-only. Attempts by Qt Script code to write to the property will be ignored.

    \value Undeletable Attempts by Qt Script code to \c{delete} the property will be ignored.

    \value SkipInEnumeration The property is not to be enumerated by a \c{for-in} enumeration.
*/

QT_BEGIN_NAMESPACE

/*!
    Constructs an invalid value.
*/
QJSValue::QJSValue()
    : d_ptr(InvalidValue())
{
}

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
    : d_ptr(new QJSValuePrivate(QString::fromAscii(value)))
{
}
#endif

/*!
    Block automatic convertion to bool
    \internal
*/
QJSValue::QJSValue(void* d)
{
    Q_UNUSED(d);
    Q_ASSERT(false);
}

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
  \obsolete

  Constructs a new QJSValue with the boolean \a value and
  registers it with the script \a engine.
*/
QJSValue::QJSValue(QJSEngine* engine, bool value)
{
    if (engine) {
        QScriptIsolate api(QV8Engine::get(engine), QScriptIsolate::NotNullEngine);
        d_ptr = new QJSValuePrivate(QV8Engine::get(engine), value);
    } else {
        d_ptr = new QJSValuePrivate(value);
    }
}

/*!
  \obsolete

  Constructs a new QJSValue with the integer \a value and
  registers it with the script \a engine.
*/
QJSValue::QJSValue(QJSEngine* engine, int value)
{
    if (engine) {
        QScriptIsolate api(QV8Engine::get(engine), QScriptIsolate::NotNullEngine);
        d_ptr = new QJSValuePrivate(QV8Engine::get(engine), value);
    } else {
        d_ptr = new QJSValuePrivate(value);
    }
}

/*!
  \obsolete

  Constructs a new QJSValue with the unsigned integer \a value and
  registers it with the script \a engine.
 */
QJSValue::QJSValue(QJSEngine* engine, uint value)
{
    if (engine) {
        QScriptIsolate api(QV8Engine::get(engine), QScriptIsolate::NotNullEngine);
        d_ptr = new QJSValuePrivate(QV8Engine::get(engine), value);
    } else {
        d_ptr = new QJSValuePrivate(value);
    }
}

/*!
  \obsolete

  Constructs a new QJSValue with the double \a value and
  registers it with the script \a engine.
*/
QJSValue::QJSValue(QJSEngine* engine, double value)
{
    if (engine) {
        QScriptIsolate api(QV8Engine::get(engine), QScriptIsolate::NotNullEngine);
        d_ptr = new QJSValuePrivate(QV8Engine::get(engine), value);
    } else {
        d_ptr = new QJSValuePrivate(value);
    }
}

/*!
  \obsolete

  Constructs a new QJSValue with the string \a value and
  registers it with the script \a engine.
*/
QJSValue::QJSValue(QJSEngine* engine, const QString& value)
{
    if (engine) {
        QScriptIsolate api(QV8Engine::get(engine), QScriptIsolate::NotNullEngine);
        d_ptr = new QJSValuePrivate(QV8Engine::get(engine), value);
    } else {
        d_ptr = new QJSValuePrivate(value);
    }
}

/*!
  \obsolete

  Constructs a new QJSValue with the string \a value and
  registers it with the script \a engine.
*/
QJSValue::QJSValue(QJSEngine* engine, const char* value)
{
    if (engine) {
        QScriptIsolate api(QV8Engine::get(engine), QScriptIsolate::NotNullEngine);
        d_ptr = new QJSValuePrivate(QV8Engine::get(engine), QString::fromUtf8(value));
    } else {
        d_ptr = new QJSValuePrivate(QString::fromUtf8(value));
    }
}

/*!
  \obsolete

  Constructs a new QJSValue with the special \a value and
  registers it with the script \a engine.
*/
QJSValue::QJSValue(QJSEngine* engine, SpecialValue value)
{
    if (engine) {
        QScriptIsolate api(QV8Engine::get(engine), QScriptIsolate::NotNullEngine);
        d_ptr = new QJSValuePrivate(QV8Engine::get(engine), value);
    } else {
        d_ptr = new QJSValuePrivate(value);
    }
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
  Returns true if this QJSValue is valid; otherwise returns
  false.
*/
bool QJSValue::isValid() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isValid();
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

  \sa QJSEngine::nullValue()
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

  \sa QJSEngine::undefinedValue()
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

  \sa toObject(), QJSEngine::newObject()
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
  \obsolete

  Use isCallable() instead.
*/
bool QJSValue::isFunction() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->isCallable();
}

/*!
  Returns true if this QJSValue is a variant value;
  otherwise returns false.

  \sa toVariant(), QJSEngine::newVariant()
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

  \sa isNumber(), toInteger(), toInt(), toUInt(), toUInt16()
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
  Returns the integer value of this QJSValue, using the conversion
  rules described in \l{ECMA-262} section 9.4, "ToInteger".

  Note that if this QJSValue is an object, calling this function
  has side effects on the script engine, since the engine will call
  the object's valueOf() function (and possibly toString()) in an
  attempt to convert the object to a primitive value (possibly
  resulting in an uncaught script exception).

  \sa toNumber()
*/
double QJSValue::toInteger() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toInteger();
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
  \obsolete

  Use toInt() instead.
*/
qint32 QJSValue::toInt32() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toInt32();
}

/*!
  \obsolete

  Use toUInt() instead.
*/
quint32 QJSValue::toUInt32() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toUInt32();
}

/*!
  Returns the unsigned 16-bit integer value of this QJSValue, using
  the conversion rules described in \l{ECMA-262} section 9.7, "ToUint16".

  Note that if this QJSValue is an object, calling this function
  has side effects on the script engine, since the engine will call
  the object's valueOf() function (and possibly toString()) in an
  attempt to convert the object to a primitive value (possibly
  resulting in an uncaught script exception).

  \sa toNumber()
*/
quint16 QJSValue::toUInt16() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toUInt16();
}

/*!
  \obsolete

  This function is obsolete; use QJSEngine::toObject() instead.
*/
QJSValue QJSValue::toObject() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return QJSValuePrivate::get(d->toObject());
}

/*!
  Returns the QVariant value of this QJSValue, if it can be
  converted to a QVariant; otherwise returns an invalid QVariant.
  The conversion is performed according to the following table:

    \table
    \header \o Input Type \o Result
    \row    \o Undefined  \o An invalid QVariant.
    \row    \o Null       \o An invalid QVariant.
    \row    \o Boolean    \o A QVariant containing the value of the boolean.
    \row    \o Number     \o A QVariant containing the value of the number.
    \row    \o String     \o A QVariant containing the value of the string.
    \row    \o QVariant Object \o The result is the QVariant value of the object (no conversion).
    \row    \o QObject Object \o A QVariant containing a pointer to the QObject.
    \row    \o Date Object \o A QVariant containing the date value (toDateTime()).
    \row    \o RegExp Object \o A QVariant containing the regular expression value (toRegExp()).
    \row    \o Array Object \o The array is converted to a QVariantList. Each element is converted to a QVariant, recursively; cyclic references are not followed.
    \row    \o Object     \o The object is converted to a QVariantMap. Each property is converted to a QVariant, recursively; cyclic references are not followed.
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
  \c{Error} object). You can call
  QJSEngine::hasUncaughtException() to determine if an exception
  occurred.

  \sa isCallable(), callWithInstance()
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
  and returns an invalid QJSValue.

  Note that if \a instance is not an object, the global object
  (see \l{QJSEngine::globalObject()}) will be used as the
  `this' object.

  Calling call() can cause an exception to occur in the script engine;
  in that case, call() returns the value that was thrown (typically an
  \c{Error} object). You can call
  QJSEngine::hasUncaughtException() to determine if an exception
  occurred.

  \snippet doc/src/snippets/code/src_script_qjsvalue.cpp 1

  \sa call()
*/
QJSValue QJSValue::callWithInstance(const QJSValue &instance, const QJSValueList &args)
{
    Q_D(QJSValue);
    QScriptIsolate api(d->engine());
    return d->call(QJSValuePrivate::get(instance), args);
}

/*!
  \obsolete

  Use callWithInstance() instead.
*/
QJSValue QJSValue::call(const QJSValue& thisObject, const QJSValueList& args)
{
    Q_D(QJSValue);
    QScriptIsolate api(d->engine());
    return d->call(QJSValuePrivate::get(thisObject), args);
}

/*!
  Creates a new \c{Object} and calls this QJSValue as a
  constructor, using the created object as the `this' object and
  passing \a args as arguments. If the return value from the
  constructor call is an object, then that object is returned;
  otherwise the default constructed object is returned.

  If this QJSValue is not a function, construct() does nothing
  and returns an invalid QJSValue.

  Calling construct() can cause an exception to occur in the script
  engine; in that case, construct() returns the value that was thrown
  (typically an \c{Error} object). You can call
  QJSEngine::hasUncaughtException() to determine if an exception
  occurred.

  \sa call(), QJSEngine::newObject()
*/
QJSValue QJSValue::construct(const QJSValueList &args)
{
    Q_D(QJSValue);
    QScriptIsolate api(d->engine());
    return QJSValuePrivate::get(d->construct(args));
}

/*!
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


/*!
  If this QJSValue is an object, returns the internal prototype
  (\c{__proto__} property) of this object; otherwise returns an
  invalid QJSValue.

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
    \header \o Type \o Result
    \row    \o Undefined  \o true
    \row    \o Null       \o true
    \row    \o Boolean    \o true if both values are true, false otherwise
    \row    \o Number     \o false if either value is NaN (Not-a-Number); true if values are equal, false otherwise
    \row    \o String     \o true if both values are exactly the same sequence of characters, false otherwise
    \row    \o Object     \o true if both values refer to the same object, false otherwise
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
    Returns true if this QJSValue is an instance of
    \a other; otherwise returns false.

    This QJSValue is considered to be an instance of \a other if
    \a other is a function and the value of the \c{prototype}
    property of \a other is in the prototype chain of this
    QJSValue.
*/
bool QJSValue::instanceOf(const QJSValue &other) const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->instanceOf(QJSValuePrivate::get(other));
}

/*!
  Returns the value of this QJSValue's property with the given \a name.
  If no such property exists, an invalid QJSValue is returned.

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

  If \a value is invalid, the property is removed.

  If the property is implemented using a setter function (i.e. has the
  PropertySetter flag set), calling setProperty() has side-effects on
  the script engine, since the setter function will be called with the
  given \a value as argument (possibly resulting in an uncaught script
  exception).

  Note that you cannot specify custom getter or setter functions for
  built-in properties, such as the \c{length} property of Array objects
  or meta properties of QObject objects.

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
  \o Non-configurable properties cannot be deleted.
  \o This function will return true even if this object doesn't
     have a property of the given \a name (i.e., non-existent
     properties are "trivially deletable").
  \o If this object doesn't have an own property of the given
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
  Returns the flags of the property with the given \a name.

  \sa property()
*/
QJSValue::PropertyFlags QJSValue::propertyFlags(const QString& name) const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->propertyFlags(name);
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
  Returns the QRegExp representation of this value.
  If this QJSValue is not a regular expression, an empty
  QRegExp is returned.

  \sa isRegExp()
*/
QRegExp QJSValue::toRegExp() const
{
    Q_D(const QJSValue);
    QScriptIsolate api(d->engine());
    return d->toRegExp();
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

  \sa QJSEngine::newRegExp()
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

QT_END_NAMESPACE
