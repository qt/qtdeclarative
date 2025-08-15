// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qjsvalue.h"

#include <private/qjsvalue_p.h>
#include <private/qqmlbuiltins_p.h>
#include <private/qv4dateobject_p.h>
#include <private/qv4errorobject_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4jscall_p.h>
#include <private/qv4mm_p.h>
#include <private/qv4object_p.h>
#include <private/qv4qmetaobjectwrapper_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4regexpobject_p.h>
#include <private/qv4runtime_p.h>
#include <private/qv4urlobject_p.h>
#include <private/qv4value_p.h>
#include <private/qv4variantassociationobject_p.h>
#include <private/qv4variantobject_p.h>

#include <QtQml/qjsprimitivevalue.h>
#include <QtQml/qjsmanagedvalue.h>

#include <QtCore/qstring.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qdatetime.h>

/*!
  \since 5.0
  \class QJSValue

  \brief The QJSValue class acts as a container for Qt/JavaScript data types.

  \ingroup qtjavascript
  \inmodule QtQml

  QJSValue supports the types defined in the \l{ECMA-262}
  standard: The primitive types, which are Undefined, Null, Boolean,
  Number, and String; and the Object and Array types. Additionally, built-in
  support is provided for Qt/C++ types such as QVariant and QObject.

  For the object-based types (including Date and RegExp), use the
  newT() functions in QJSEngine (e.g. QJSEngine::newObject())
  to create a QJSValue of the desired type. For the primitive types,
  use one of the QJSValue constructor overloads. For other types, e.g.
  registered gadget types such as QPoint, you can use QJSEngine::toScriptValue.

  The methods named isT() (e.g. isBool(), isUndefined()) can be
  used to test if a value is of a certain type. The methods named
  toT() (e.g. toBool(), toString()) can be used to convert a
  QJSValue to another type. You can also use the generic
  qjsvalue_cast() function.

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

  \section1 Working With Arrays

  To create an array using QJSValue, use \l QJSEngine::newArray():

  \code
  // Assumes that this class was declared in QML.
  QJSValue jsArray = engine->newArray(3);
  \endcode

  To set individual elements in the array, use
  the \l {QJSValue::}{setProperty(quint32 arrayIndex, const QJSValue &value)}
  overload. For example, to fill the array above with integers:

  \code
  for (int i = 0; i < 3; ++i) {
      jsArray.setProperty(i, QRandomGenerator::global().generate());
  }
  \endcode

  To determine the length of the array, access the \c "length" property.
  To access array elements, use the
  \l {QJSValue::}{property(quint32 arrayIndex)} overload. The following code
  reads the array we created above back into a list:

  \code
  QVector<int> integers;
  const int length = jsArray.property("length").toInt();
  for (int i = 0; i < length; ++i) {
      integers.append(jsArray.property(i).toInt());
  }
  \endcode

  \section2 Converting to JSON

  It's possible to convert a QJSValue to a JSON type. For example,
  to convert to an array, use \l QJSEngine::fromScriptValue():

  \code
  const QJsonValue jsonValue = engine.fromScriptValue<QJsonValue>(jsValue);
  const QJsonArray jsonArray = jsonValue.toArray();
  \endcode
*/

/*!
    \enum QJSValue::SpecialValue

    This enum is used to specify a single-valued type.

    \value UndefinedValue An undefined value.

    \value NullValue A null value.
*/

/*!
    \typedef QJSValueList
    \relates QJSValue

    This is a typedef for a QList<QJSValue>.
*/

/*!
    \enum QJSValue::ErrorType
    \since 5.12

    Use this enum for JavaScript language-specific types of Error objects.

    They may be useful when emulating language features in C++ requires the use
    of specialized exception types. In addition, they may help to more clearly
    communicate certain typical conditions, instead of throwing a generic
    JavaScript exception. For example, code that deals with networking and
    resource locators may find it useful to propagate errors related to
    malformed locators using the URIError type.

    \omitvalue NoError
    \value GenericError A generic Error object, but not of a specific sub-type.
    \omitvalue EvalError
    \value RangeError A value did not match the expected set or range.
    \value ReferenceError A non-existing variable referenced.
    \value SyntaxError An invalid token or sequence of tokens was encountered
    that does not conform with the syntax of the language.
    \value TypeError An operand or argument is incompatible with the type
    expected.
    \value URIError A URI handling function was used incorrectly or the URI
    provided is malformed.
*/

/*!
    \enum QJSValue::ObjectConversionBehavior

    This enum is used to specify how JavaScript objects and symbols without an equivalent
    native Qt type should be treated when converting to QVariant.

    \value ConvertJSObjects A best-effort, possibly lossy, conversion is attempted.
           Symbols are converted to QString.

    \value RetainJSObjects The value is retained as QJSValue wrapped in QVariant.
*/

QT_BEGIN_NAMESPACE

using namespace QV4;

/*!
  Constructs a new QJSValue with a boolean \a value.
*/
QJSValue::QJSValue(bool value) : d(QJSValuePrivate::encode(value))
{
}

/*!
  Constructs a new QJSValue with a number \a value.
*/
QJSValue::QJSValue(int value) : d(QJSValuePrivate::encode(value))
{
}

/*!
  Constructs a new QJSValue with a number \a value.
*/
QJSValue::QJSValue(uint value) : d(QJSValuePrivate::encode(value))
{
}

/*!
  Constructs a new QJSValue with a number \a value.
*/
QJSValue::QJSValue(double value) : d(QJSValuePrivate::encode(value))
{
}

/*!
  Constructs a new QJSValue with a string \a value.
*/
QJSValue::QJSValue(const QString &value) : d(QJSValuePrivate::encode(value))
{
}

/*!
  Constructs a new QJSValue with a special \a value.
*/
QJSValue::QJSValue(SpecialValue value)
    : d(value == NullValue ? QJSValuePrivate::encodeNull() : QJSValuePrivate::encodeUndefined())
{
}

/*!
  Constructs a new QJSValue with a string \a value.
*/
QJSValue::QJSValue(const QLatin1String &value) : d(QJSValuePrivate::encode(value))
{
}

/*!
  Constructs a new QJSValue with a string \a value.
*/
#ifndef QT_NO_CAST_FROM_ASCII
QJSValue::QJSValue(const char *value) : d(QJSValuePrivate::encode(QString::fromUtf8(value)))
{
}
#endif

/*!
  Constructs a new QJSValue that is a copy of \a other.

  Note that if \a other is an object (i.e., isObject() would return
  true), then only a reference to the underlying object is copied into
  the new script value (i.e., the object itself is not copied).
*/
QJSValue::QJSValue(const QJSValue &other) : d(other.d)
{
    switch (QJSValuePrivate::tag(d)) {
    case QJSValuePrivate::Kind::Undefined:
    case QJSValuePrivate::Kind::Null:
    case QJSValuePrivate::Kind::IntValue:
    case QJSValuePrivate::Kind::BoolValue:
        return;
    case QJSValuePrivate::Kind::DoublePtr:
        d = QJSValuePrivate::encode(*QJSValuePrivate::doublePtr(d));
        return;
    case QJSValuePrivate::Kind::QV4ValuePtr:
        d = QJSValuePrivate::encode(*QJSValuePrivate::qv4ValuePtr(d));
        return;
    case QJSValuePrivate::Kind::QStringPtr:
        d = QJSValuePrivate::encode(*QJSValuePrivate::qStringPtr(d));
        break;
    }
}

/*!
    \fn QJSValue::QJSValue(QJSValue && other)

    Move constructor. Moves from \a other into this QJSValue object.
*/

/*!
    \fn QJSValue &QJSValue::operator=(QJSValue && other)

    Move-assigns \a other to this QJSValue object.
*/

/*!
    Destroys this QJSValue.
*/
QJSValue::~QJSValue()
{
    QJSValuePrivate::free(this);
}

/*!
  Returns true if this QJSValue is of the primitive type Boolean;
  otherwise returns false.

  \sa toBool()
*/
bool QJSValue::isBool() const
{
    return QJSValuePrivate::tag(d) == QJSValuePrivate::Kind::BoolValue;
}

/*!
  Returns true if this QJSValue is of the primitive type Number;
  otherwise returns false.

  \sa toNumber()
*/
bool QJSValue::isNumber() const
{
    switch (QJSValuePrivate::tag(d)) {
    case QJSValuePrivate::Kind::IntValue:
    case QJSValuePrivate::Kind::DoublePtr:
        return true;
    default:
        break;
    }

    return false;
}

/*!
  Returns true if this QJSValue is of the primitive type Null;
  otherwise returns false.
*/
bool QJSValue::isNull() const
{
    return QJSValuePrivate::tag(d) == QJSValuePrivate::Kind::Null;
}

/*!
  Returns true if this QJSValue is of the primitive type String;
  otherwise returns false.

  \sa toString()
*/
bool QJSValue::isString() const
{
    switch (QJSValuePrivate::tag(d)) {
    case QJSValuePrivate::Kind::QStringPtr:
        return true;
    case QJSValuePrivate::Kind::QV4ValuePtr: {
        return QJSValuePrivate::qv4ValuePtr(d)->isString();
    }
    default:
        break;
    }

    return false;
}

/*!
  Returns true if this QJSValue is of the primitive type Undefined or if the managed value
  has been cleared (by deleting the engine). Otherwise returns false.
*/
bool QJSValue::isUndefined() const
{
    switch (QJSValuePrivate::tag(d)) {
    case QJSValuePrivate::Kind::Undefined:
        return true;
    case QJSValuePrivate::Kind::QV4ValuePtr:
        return QJSValuePrivate::qv4ValuePtr(d)->isUndefined();
    default:
        break;
    }

    return false;
}

/*!
  Returns true if this QJSValue is an object of the Error class;
  otherwise returns false.

  \sa errorType(), {QJSEngine#Script Exceptions}{QJSEngine - Script Exceptions}
*/
bool QJSValue::isError() const
{
    return QJSValuePrivate::asManagedType<ErrorObject>(this);
}

/*!
  Returns true if this QJSValue is an object of the URL JavaScript class;
  otherwise returns false.

  \note For a QJSValue that contains a QUrl, this function returns false.
  However, \c{toVariant().value<QUrl>()} works in both cases.
*/
bool QJSValue::isUrl() const
{
    return QJSValuePrivate::asManagedType<UrlObject>(this);
}

/*!
  \since 5.12
  Returns the error type this QJSValue represents if it is an Error object.
  Otherwise, returns \c NoError."

  \sa isError(), {QJSEngine#Script Exceptions}{QJSEngine - Script Exceptions}
*/
QJSValue::ErrorType QJSValue::errorType() const
{
    const QV4::ErrorObject *error = QJSValuePrivate::asManagedType<ErrorObject>(this);
    if (!error)
        return NoError;
    switch (error->d()->errorType) {
    case QV4::Heap::ErrorObject::Error:
        return GenericError;
    case QV4::Heap::ErrorObject::EvalError:
        return EvalError;
    case QV4::Heap::ErrorObject::RangeError:
        return RangeError;
    case QV4::Heap::ErrorObject::ReferenceError:
        return ReferenceError;
    case QV4::Heap::ErrorObject::SyntaxError:
        return SyntaxError;
    case QV4::Heap::ErrorObject::TypeError:
        return TypeError;
    case QV4::Heap::ErrorObject::URIError:
        return URIError;
    }
    Q_UNREACHABLE_RETURN(NoError);
}

/*!
  Returns true if this QJSValue is an object of the Array class;
  otherwise returns false.

  \note This method is the equivalent of \e Array.isArray() in JavaScript. You
        can use it to identify JavaScript arrays, but it will return \c false
        for any array-like objects that are not JavaScript arrays. This includes
        QML \e list objects for either value types or object types, JavaScript
        typed arrays, JavaScript ArrayBuffer objects, and any custom array-like
        objects you may create yourself. All of these \e behave like JavaScript
        arrays, though: They generally expose the same methods and the
        subscript operator can be used on them. Therefore, using this method to
        determine whether an object could be used like an array is not
        advisable.

  \sa QJSEngine::newArray()
*/
bool QJSValue::isArray() const
{
    return QJSValuePrivate::asManagedType<ArrayObject>(this);
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
    return QJSValuePrivate::asManagedType<QV4::Object>(this);
}

/*!
  Returns true if this QJSValue is a function, otherwise
  returns false.

  \sa call()
*/
bool QJSValue::isCallable() const
{
    return QJSValuePrivate::asManagedType<FunctionObject>(this);
}

#if QT_DEPRECATED_SINCE(6, 9)
/*!
  \deprecated [6.9]
  Returns true if this QJSValue is a variant value;
  otherwise returns false.

  \warning This function is likely to give unexpected results.
  A variant value is only constructed by the QJSEngine in a very
  limited number of cases. This used to be different before Qt
  5.14, where \l{QJSEngine::toScriptValue} would have created
  them for more types instead of corresponding ECMAScript types.
  You can get a valid \l QVariant via \l toVariant for many values
  for which \c{isVariant} returns false.

  \sa toVariant()
*/
bool QJSValue::isVariant() const
{
    if (QJSValuePrivate::asManagedType<QV4::VariantObject>(this))
        return true;
    if (auto vt = QJSValuePrivate::asManagedType<QV4::QQmlValueTypeWrapper>(this))
        if (vt->metaObject() == &QQmlVarForeign::staticMetaObject)
            return true;
    return false;
}
#endif

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
    if (const QString *string = QJSValuePrivate::asQString(this))
        return *string;

    return QV4::Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(this)).toQStringNoThrow();
}

template<typename T>
T caughtResult(const QJSValue *v, T (QV4::Value::*convert)() const)
{
    const T result = (QV4::Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(v)).*convert)();
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(v);
    if (engine && engine->hasException) {
        engine->catchException();
        return T();
    }
    return result;
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
    if (const QString *string = QJSValuePrivate::asQString(this))
        return RuntimeHelpers::stringToNumber(*string);

    return caughtResult<double>(this, &QV4::Value::toNumber);
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
    if (const QString *string = QJSValuePrivate::asQString(this))
        return string->size() > 0;

    return caughtResult<bool>(this, &QV4::Value::toBoolean);
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
    if (const QString *string = QJSValuePrivate::asQString(this))
        return QV4::Value::toInt32(RuntimeHelpers::stringToNumber(*string));

    return caughtResult<qint32>(this, &QV4::Value::toInt32);
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
    if (const QString *string = QJSValuePrivate::asQString(this))
        return QV4::Value::toUInt32(RuntimeHelpers::stringToNumber(*string));

    return caughtResult<quint32>(this, &QV4::Value::toUInt32);
}

/*!
    \overload

    Returns toVariant(ConvertJSObjects).

    \sa isVariant()
*/
QVariant QJSValue::toVariant() const
{
    return toVariant(ConvertJSObjects);
}

/*!
    Returns the QVariant value of this QJSValue, if it can be
    converted to a QVariant; otherwise returns an invalid QVariant.
    Some JavaScript types and objects have native expressions in Qt.
    Those are converted to their native expressions. For example:

    \table
    \header \li Input Type \li Result
    \row    \li Undefined  \li An invalid QVariant.
    \row    \li Null       \li A QVariant containing a null pointer (QMetaType::Nullptr).
    \row    \li Boolean    \li A QVariant containing the value of the boolean.
    \row    \li Number     \li A QVariant containing the value of the number.
    \row    \li String     \li A QVariant containing the value of the string.
    \row    \li QVariant Object \li The result is the QVariant value of the object (no conversion).
    \row    \li QObject Object \li A QVariant containing a pointer to the QObject.
    \row    \li Date Object \li A QVariant containing the date value (toDateTime()).
    \row    \li RegularExpression Object \li A QVariant containing the regular expression value.
    \endtable

    For other types the \a behavior parameter is relevant. If
    \c ConvertJSObjects is given, a best effort but possibly lossy conversion is
    attempted. Generic JavaScript objects are converted to QVariantMap.
    JavaScript arrays are converted to QVariantList. Each property or element is
    converted to a QVariant, recursively; cyclic references are not followed.
    JavaScript function objects are dropped. If \c RetainJSObjects is given, the
    QJSValue is wrapped into a QVariant via QVariant::fromValue(). The resulting
    conversion is lossless but the internal structure of the objects is not
    immediately accessible.

    \sa isVariant()
*/
QVariant QJSValue::toVariant(QJSValue::ObjectConversionBehavior behavior) const
{
    if (const QString *string = QJSValuePrivate::asQString(this))
        return QVariant(*string);

    QV4::Value val = QV4::Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(this));
    if (val.isUndefined())
        return QVariant();
    if (val.isNull())
        return QVariant(QMetaType::fromType<std::nullptr_t>(), nullptr);
    if (val.isBoolean())
        return QVariant(val.booleanValue());
    if (val.isInt32()) // Includes doubles that can be losslessly casted to int
        return QVariant(val.integerValue());
    if (val.isNumber())
        return QVariant(val.doubleValue());

    Q_ASSERT(val.isManaged());

    if (val.isString())
        return QVariant(val.toQString());

    if (behavior == RetainJSObjects) {
        // For historical reasons we don't want to retrieve the actual container here.
        // ### Qt7: change this
        if (val.as<QV4::VariantAssociationObject>())
            return QVariant::fromValue(QJSValuePrivate::fromReturnedValue(val.asReturnedValue()));

        return QV4::ExecutionEngine::toVariant(
                val, /*typeHint*/ QMetaType{}, /*createJSValueForObjectsAndSymbols=*/ true);
    } else {
        return QV4::ExecutionEngine::toVariantLossy(val);
    }

    Q_ASSERT(false);
    return QVariant();
}

/*!
 * Converts the value to a QJSPrimitiveValue. If the value holds a type
 * supported by QJSPrimitiveValue, the value is copied. Otherwise the
 * value is converted to a string, and the string is stored in
 * QJSPrimitiveValue.
 *
 * \note Conversion of a managed value to a string can throw an exception. In
 *       particular, symbols cannot be coerced into strings, or a custom
 *       toString() method  may throw. In this case the result is the undefined
 *       value and the engine carries an error after the conversion.
 */
QJSPrimitiveValue QJSValue::toPrimitive() const
{
    if (const QString *string = QJSValuePrivate::asQString(this))
        return *string;

    const QV4::Value val = QV4::Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(this));
    return QV4::ExecutionEngine::createPrimitive(&val);
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
QJSValue QJSValue::call(const QJSValueList &args) const
{
    const FunctionObject *f = QJSValuePrivate::asManagedType<FunctionObject>(this);
    if (!f)
        return QJSValue();

    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    Q_ASSERT(engine);

    Scope scope(engine);
    JSCallArguments jsCallData(scope, args.size());
    *jsCallData.thisObject = engine->globalObject;
    for (int i = 0; i < args.size(); ++i) {
        if (!QJSValuePrivate::checkEngine(engine, args.at(i))) {
            qWarning("QJSValue::call() failed: cannot call function with argument created in a different engine");
            return QJSValue();
        }
        jsCallData.args[i] = QJSValuePrivate::convertToReturnedValue(engine, args.at(i));
    }

    ScopedValue result(scope, f->call(jsCallData));
    if (engine->hasException)
        result = engine->catchException();
    if (engine->isInterrupted.loadRelaxed())
        result = engine->newErrorObject(QStringLiteral("Interrupted"));

    return QJSValuePrivate::fromReturnedValue(result->asReturnedValue());
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
QJSValue QJSValue::callWithInstance(const QJSValue &instance, const QJSValueList &args) const
{
    const FunctionObject *f = QJSValuePrivate::asManagedType<FunctionObject>(this);
    if (!f)
        return QJSValue();

    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    Q_ASSERT(engine);
    Scope scope(engine);

    if (!QJSValuePrivate::checkEngine(engine, instance)) {
        qWarning("QJSValue::call() failed: cannot call function with thisObject created in a different engine");
        return QJSValue();
    }

    JSCallArguments jsCallData(scope, args.size());
    *jsCallData.thisObject = QJSValuePrivate::convertToReturnedValue(engine, instance);
    for (int i = 0; i < args.size(); ++i) {
        if (!QJSValuePrivate::checkEngine(engine, args.at(i))) {
            qWarning("QJSValue::call() failed: cannot call function with argument created in a different engine");
            return QJSValue();
        }
        jsCallData.args[i] = QJSValuePrivate::convertToReturnedValue(engine, args.at(i));
    }

    ScopedValue result(scope, f->call(jsCallData));
    if (engine->hasException)
        result = engine->catchException();
    if (engine->isInterrupted.loadRelaxed())
        result = engine->newErrorObject(QStringLiteral("Interrupted"));

    return QJSValuePrivate::fromReturnedValue(result->asReturnedValue());
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
QJSValue QJSValue::callAsConstructor(const QJSValueList &args) const
{
    const FunctionObject *f = QJSValuePrivate::asManagedType<FunctionObject>(this);
    if (!f)
        return QJSValue();

    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    Q_ASSERT(engine);

    Scope scope(engine);
    JSCallArguments jsCallData(scope, args.size());
    for (int i = 0; i < args.size(); ++i) {
        if (!QJSValuePrivate::checkEngine(engine, args.at(i))) {
            qWarning("QJSValue::callAsConstructor() failed: cannot construct function with argument created in a different engine");
            return QJSValue();
        }
        jsCallData.args[i] = QJSValuePrivate::convertToReturnedValue(engine, args.at(i));
    }

    ScopedValue result(scope, f->callAsConstructor(jsCallData));
    if (engine->hasException)
        result = engine->catchException();
    if (engine->isInterrupted.loadRelaxed())
        result = engine->newErrorObject(QStringLiteral("Interrupted"));

    return QJSValuePrivate::fromReturnedValue(result->asReturnedValue());
}

/*!
  If this QJSValue is an object, returns the internal prototype
  (\c{__proto__} property) of this object; otherwise returns an
  undefined QJSValue.

  \sa setPrototype(), isObject()
*/
QJSValue QJSValue::prototype() const
{
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    if (!engine)
        return QJSValue();
    QV4::Scope scope(engine);
    ScopedObject o(scope, QJSValuePrivate::asManagedType<QV4::Object>(this));
    if (!o)
        return QJSValue();
    ScopedObject p(scope, o->getPrototypeOf());
    if (!p)
        return QJSValue(NullValue);
    return QJSValuePrivate::fromReturnedValue(p.asReturnedValue());
}

/*!
  If this QJSValue is an object, sets the internal prototype
  (\c{__proto__} property) of this object to be \a prototype;
  if the QJSValue is null, it sets the prototype to null;
  otherwise does nothing.

  The internal prototype should not be confused with the public
  property with name "prototype"; the public prototype is usually
  only set on functions that act as constructors.

  \sa prototype(), isObject()
*/
void QJSValue::setPrototype(const QJSValue& prototype)
{
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    if (!engine)
        return;
    Scope scope(engine);
    ScopedObject o(scope, QJSValuePrivate::asReturnedValue(this));
    if (!o)
        return;
    QV4::Value val = QV4::Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(&prototype));
    if (val.isNull()) {
        o->setPrototypeOf(nullptr);
        return;
    }

    ScopedObject p(scope, val);
    if (!p)
        return;
    if (o->engine() != p->engine()) {
        qWarning("QJSValue::setPrototype() failed: cannot set a prototype created in a different engine");
        return;
    }
    if (!o->setPrototypeOf(p))
        qWarning("QJSValue::setPrototype() failed: cyclic prototype value");
}

/*!
  Assigns the \a other value to this QJSValue.

  Note that if \a other is an object (isObject() returns true),
  only a reference to the underlying object will be assigned;
  the object itself will not be copied.
*/
QJSValue& QJSValue::operator=(const QJSValue& other)
{
    if (d == other.d)
        return *this;

    QJSValuePrivate::free(this);
    d = 0;

    if (const QString *string = QJSValuePrivate::asQString(&other))
        QJSValuePrivate::setString(this, *string);
    else
        // fomReturnedValue is safe, as the QJSValue still has a persistent reference
        QJSValuePrivate::setValue(
            this,
            QV4::Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(&other)));

    return *this;
}

QJSValue::QJSValue(QJSPrimitiveValue &&value)
{
    switch (value.type()) {
    case QJSPrimitiveValue::Undefined:
        d = QJSValuePrivate::encodeUndefined();
        return;
    case QJSPrimitiveValue::Null:
        d = QJSValuePrivate::encodeNull();
        return;
    case QJSPrimitiveValue::Boolean:
        d = QJSValuePrivate::encode(value.asBoolean());
        return;
    case QJSPrimitiveValue::Integer:
        d = QJSValuePrivate::encode(value.asInteger());
        return;
    case QJSPrimitiveValue::Double:
        d = QJSValuePrivate::encode(value.asDouble());
        return;
    case QJSPrimitiveValue::String:
        d = QJSValuePrivate::encode(value.asString());
        return;
    }

    Q_UNREACHABLE();
}

QJSValue::QJSValue(QJSManagedValue &&value)
{
    if (!value.d) {
        d = QV4::Encode::undefined();
    } else if (value.d->isManaged()) {
        // If it's managed, we can adopt the persistent value.
        QJSValuePrivate::adoptPersistentValue(this, value.d);
        value.d = nullptr;
    } else {
        d = QJSValuePrivate::encode(*value.d);
        QV4::PersistentValueStorage::free(value.d);
        value.d = nullptr;
    }
}

static bool js_equal(const QString &string, const QV4::Value &value)
{
    if (String *s = value.stringValue())
        return string == s->toQString();
    if (value.isNumber())
        return RuntimeHelpers::stringToNumber(string) == value.asDouble();
    if (value.isBoolean())
        return RuntimeHelpers::stringToNumber(string) == double(value.booleanValue());
    if (QV4::Object *o = value.objectValue()) {
        Scope scope(o->engine());
        ScopedValue p(scope, RuntimeHelpers::toPrimitive(value, PREFERREDTYPE_HINT));
        return js_equal(string, p);
    }
    return false;
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
    // QJSValue stores heap items in persistent values, which already ensures marking
    // therefore, fromReturnedValue below is safe
    if (const QString *string = QJSValuePrivate::asQString(this)) {
        if (const QString *otherString = QJSValuePrivate::asQString(&other))
            return *string == *otherString;
        return js_equal(*string, Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(&other)));
    }

    if (const QString *otherString = QJSValuePrivate::asQString(&other))
        return js_equal(*otherString, Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(this)));

    return Runtime::CompareEqual::call(Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(this)),
                                       Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(&other)));
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
    \row    \li Boolean    \li true if values are both true or both false, false otherwise
    \row    \li Number     \li false if either value is NaN (Not-a-Number); true if values are equal, false otherwise
    \row    \li String     \li true if both values are exactly the same sequence of characters, false otherwise
    \row    \li Object     \li true if both values refer to the same object, false otherwise
    \endtable

  \sa equals()
*/
bool QJSValue::strictlyEquals(const QJSValue& other) const
{
    if (const QString *string = QJSValuePrivate::asQString(this)) {
        if (const QString *otherString = QJSValuePrivate::asQString(&other))
            return *string == *otherString;
        if (const String *s = QJSValuePrivate::asManagedType<String>(&other))
            return *string == s->toQString();
        return false;
    }

    if (const QString *otherString = QJSValuePrivate::asQString(&other)) {
        if (const String *s = QJSValuePrivate::asManagedType<String>(this))
            return *otherString == s->toQString();
        return false;
    }

    // QJSValue stores heap objects persistently, so we can be sure that they'll be marked
    // thus we can safely use fromReturnedValue
    return RuntimeHelpers::strictEqual(Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(this)),
                                       Value::fromReturnedValue(QJSValuePrivate::asReturnedValue(&other)));
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

  To access array elements, use the
  \l {QJSValue::}{setProperty(quint32 arrayIndex, const QJSValue &value)}
  overload instead.

  \sa setProperty(), hasProperty(), QJSValueIterator
*/
QJSValue QJSValue::property(const QString& name) const
{
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    if (!engine)
        return QJSValue();

    QV4::Scope scope(engine);
    ScopedObject o(scope, QJSValuePrivate::asReturnedValue(this));
    if (!o)
        return QJSValue();

    ScopedString s(scope, engine->newString(name));
    QV4::ScopedValue result(scope, o->get(s->toPropertyKey()));
    if (engine->hasException)
        result = engine->catchException();

    return QJSValuePrivate::fromReturnedValue(result->asReturnedValue());
}

/*!
  \overload

  Returns the property at the given \a arrayIndex.

  It is possible to access elements in an array in two ways. The first is to
  use the array index as the property name:

  \code
  qDebug() << jsValueArray.property(QLatin1String("4")).toString();
  \endcode

  The second is to use the overload that takes an index:

  \code
  qDebug() << jsValueArray.property(4).toString();
  \endcode

  Both of these approaches achieve the same result, except that the latter:

  \list
  \li Is easier to use (can use an integer directly)
  \li Is faster (no conversion to integer)
  \endlist

  If this QJSValue is not an Array object, this function behaves
  as if property() was called with the string representation of \a
  arrayIndex.
*/
QJSValue QJSValue::property(quint32 arrayIndex) const
{
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    if (!engine)
        return QJSValue();

    QV4::Scope scope(engine);
    ScopedObject o(scope, QJSValuePrivate::asReturnedValue(this));
    if (!o)
        return QJSValue();

    QV4::ScopedValue result(scope, arrayIndex == UINT_MAX ? o->get(engine->id_uintMax()) : o->get(arrayIndex));
    if (engine->hasException)
        engine->catchException();
    return QJSValuePrivate::fromReturnedValue(result->asReturnedValue());
}

/*!
  Sets the value of this QJSValue's property with the given \a name to
  the given \a value.

  If this QJSValue is not an object, this function does nothing.

  If this QJSValue does not already have a property with name \a name,
  a new property is created.

  To modify array elements, use the
  \l {QJSValue::}{setProperty(quint32 arrayIndex, const QJSValue &value)}
  overload instead.

  \sa property(), deleteProperty()
*/
void QJSValue::setProperty(const QString& name, const QJSValue& value)
{
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    if (!engine)
        return;
    Scope scope(engine);

    ScopedObject o(scope, QJSValuePrivate::asReturnedValue(this));
    if (!o)
        return;

    if (!QJSValuePrivate::checkEngine(engine, value)) {
        qWarning("QJSValue::setProperty(%s) failed: cannot set value created in a different engine", name.toUtf8().constData());
        return;
    }

    ScopedString s(scope, engine->newString(name));
    QV4::ScopedValue v(scope, QJSValuePrivate::convertToReturnedValue(engine, value));
    o->put(s->toPropertyKey(), v);
    if (engine->hasException)
        engine->catchException();
}

/*!
  \overload

  Sets the property at the given \a arrayIndex to the given \a value.

  It is possible to modify elements in an array in two ways. The first is to
  use the array index as the property name:

  \code
  jsValueArray.setProperty(QLatin1String("4"), value);
  \endcode

  The second is to use the overload that takes an index:

  \code
  jsValueArray.setProperty(4, value);
  \endcode

  Both of these approaches achieve the same result, except that the latter:

  \list
  \li Is easier to use (can use an integer directly)
  \li Is faster (no conversion to integer)
  \endlist

  If this QJSValue is not an Array object, this function behaves
  as if setProperty() was called with the string representation of \a
  arrayIndex.

  \sa {QJSValue::}{property(quint32 arrayIndex)}, {Working With Arrays}
*/
void QJSValue::setProperty(quint32 arrayIndex, const QJSValue& value)
{
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    if (!engine)
        return;
    Scope scope(engine);

    ScopedObject o(scope, QJSValuePrivate::asReturnedValue(this));
    if (!o)
        return;

    if (!QJSValuePrivate::checkEngine(engine, value)) {
        qWarning("QJSValue::setProperty(%d) failed: cannot set value created in a different engine", arrayIndex);
        return;
    }

    QV4::ScopedValue v(scope, QJSValuePrivate::convertToReturnedValue(engine, value));
    PropertyKey id = arrayIndex != UINT_MAX ? PropertyKey::fromArrayIndex(arrayIndex) : engine->id_uintMax()->propertyKey();
    o->put(id, v);
    if (engine->hasException)
        engine->catchException();
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
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    if (!engine)
        return false;

    Scope scope(engine);
    ScopedObject o(scope, QJSValuePrivate::asReturnedValue(this));
    if (!o)
        return false;

    ScopedString s(scope, engine->newString(name));
    return o->deleteProperty(s->toPropertyKey());
}

/*!
  Returns true if this object has a property of the given \a name,
  otherwise returns false.

  \sa property(), hasOwnProperty()
*/
bool QJSValue::hasProperty(const QString &name) const
{
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    if (!engine)
        return false;

    Scope scope(engine);
    ScopedObject o(scope, QJSValuePrivate::asReturnedValue(this));
    if (!o)
        return false;

    ScopedString s(scope, engine->newString(name));
    return o->hasProperty(s->toPropertyKey());
}

/*!
  Returns true if this object has an own (not prototype-inherited)
  property of the given \a name, otherwise returns false.

  \sa property(), hasProperty()
*/
bool QJSValue::hasOwnProperty(const QString &name) const
{
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    if (!engine)
        return false;

    Scope scope(engine);
    ScopedObject o(scope, QJSValuePrivate::asReturnedValue(this));
    if (!o)
        return false;

    ScopedString s(scope, engine->newIdentifier(name));
    return o->getOwnProperty(s->propertyKey()) != Attr_Invalid;
}

/*!
 * If this QJSValue is a QObject, returns the QObject pointer
 * that the QJSValue represents; otherwise, returns \nullptr.
 *
 * If the QObject that this QJSValue wraps has been deleted,
 * this function returns \nullptr (i.e. it is possible for toQObject()
 * to return \nullptr even when isQObject() returns true).
 *
 * \sa isQObject()
 */
QObject *QJSValue::toQObject() const
{
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    if (!engine)
        return nullptr;
    QV4::Scope scope(engine);
    QV4::Scoped<QV4::QObjectWrapper> wrapper(scope, QJSValuePrivate::asReturnedValue(this));
    if (!wrapper)
        return nullptr;

    return wrapper->object();
}

/*!
  \since 5.8

 * If this QJSValue is a QMetaObject, returns the QMetaObject pointer
 * that the QJSValue represents; otherwise, returns \nullptr.
 *
 * \sa isQMetaObject()
 */
const QMetaObject *QJSValue::toQMetaObject() const
{
    QV4::ExecutionEngine *engine = QJSValuePrivate::engine(this);
    if (!engine)
        return nullptr;
    QV4::Scope scope(engine);
    QV4::Scoped<QV4::QMetaObjectWrapper> wrapper(scope, QJSValuePrivate::asReturnedValue(this));
    if (!wrapper)
        return nullptr;

    return wrapper->metaObject();
}


/*!
  Returns a QDateTime representation of this value, in local time.
  If this QJSValue is not a date, or the value of the date is NaN
  (Not-a-Number), an invalid QDateTime is returned.

  \sa isDate()
*/
QDateTime QJSValue::toDateTime() const
{
    if (const QV4::DateObject *date = QJSValuePrivate::asManagedType<DateObject>(this))
        return date->toQDateTime();
    return QDateTime();
}

/*!
  Returns true if this QJSValue is an object of the Date class;
  otherwise returns false.
*/
bool QJSValue::isDate() const
{
    return QJSValuePrivate::asManagedType<DateObject>(this);
}

/*!
  Returns true if this QJSValue is an object of the RegExp class;
  otherwise returns false.
*/
bool QJSValue::isRegExp() const
{
    return QJSValuePrivate::asManagedType<RegExpObject>(this);
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
    return QJSValuePrivate::asManagedType<QV4::QObjectWrapper>(this);
}

/*!
  \since 5.8

  Returns true if this QJSValue is a QMetaObject; otherwise returns
  false.

  \sa toQMetaObject(), QJSEngine::newQMetaObject()
*/
bool QJSValue::isQMetaObject() const
{
    return QJSValuePrivate::asManagedType<QV4::QMetaObjectWrapper>(this);
}

#ifndef QT_NO_DATASTREAM
QDataStream &operator<<(QDataStream &stream, const QJSValue &jsv)
{
    quint32 isNullOrUndefined = 0;
    if (jsv.isNull())
        isNullOrUndefined |= 0x1;
    if (jsv.isUndefined())
        isNullOrUndefined |= 0x2;
    stream << isNullOrUndefined;
    if (!isNullOrUndefined) {
        const QVariant v = jsv.toVariant();
        switch (v.userType()) {
        case QMetaType::Bool:
        case QMetaType::Double:
        case QMetaType::Int:
        case QMetaType::QString:
            v.save(stream);
            break;
        default:
            qWarning() << "QDataStream::operator<< was to save a non-trivial QJSValue."
                       << "This is not supported anymore, please stream a QVariant instead.";
            QVariant().save(stream);
            break;
        }

    }
    return stream;
}

QDataStream &operator>>(QDataStream &stream, QJSValue &jsv)
{
    quint32 isNullOrUndefined;
    stream >> isNullOrUndefined;

    if (isNullOrUndefined & 0x1) {
        jsv = QJSValue(QJSValue::NullValue);
    } else if (isNullOrUndefined & 0x2) {
        jsv = QJSValue();
    } else {
        QVariant v;
        v.load(stream);

        switch (v.userType()) {
        case QMetaType::Bool:
            jsv = QJSValue(v.toBool());
            break;
        case QMetaType::Double:
            jsv = QJSValue(v.toDouble());
            break;
        case QMetaType::Int:
            jsv = QJSValue(v.toInt());
            break;
        case QMetaType::QString:
            jsv = QJSValue(v.toString());
            break;
        default:
            qWarning() << "QDataStream::operator>> to restore a non-trivial QJSValue."
                       << "This is not supported anymore, please stream a QVariant instead.";
            break;
        }
    }
    return stream;
}
#endif

QT_END_NAMESPACE
