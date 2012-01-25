/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qjsengine.h"
#include "qjsengine_p.h"
#include "qjsvalue.h"
#include "qjsvalue_p.h"
#include "qscriptisolate_p.h"
#include "qscript_impl_p.h"
#include "qv8engine_p.h"

#include <QtCore/qdatetime.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qstringlist.h>
#include <QtCore/qvariant.h>
#include <QtCore/qdatetime.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qpluginloader.h>
#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>

#undef Q_D
#undef Q_Q
#define Q_D(blah)
#define Q_Q(blah)

Q_DECLARE_METATYPE(QJSValue)
Q_DECLARE_METATYPE(QObjectList)
Q_DECLARE_METATYPE(QList<int>)

/*!
  \since 5.0
  \class QJSEngine

  \brief The QJSEngine class provides an environment for evaluating JavaScript code.

  \ingroup qtjavascript
  \mainclass

  \section1 Evaluating Scripts

  Use evaluate() to evaluate script code.

  \snippet doc/src/snippets/code/src_script_qjsengine.cpp 0

  evaluate() returns a QJSValue that holds the result of the
  evaluation. The QJSValue class provides functions for converting
  the result to various C++ types (e.g. QJSValue::toString()
  and QJSValue::toNumber()).

  The following code snippet shows how a script function can be
  defined and then invoked from C++ using QJSValue::call():

  \snippet doc/src/snippets/code/src_script_qjsengine.cpp 1

  As can be seen from the above snippets, a script is provided to the
  engine in the form of a string. One common way of loading scripts is
  by reading the contents of a file and passing it to evaluate():

  \snippet doc/src/snippets/code/src_script_qjsengine.cpp 2

  Here we pass the name of the file as the second argument to
  evaluate().  This does not affect evaluation in any way; the second
  argument is a general-purpose string that is used to identify the
  script for debugging purposes (for example, our filename will now
  show up in any uncaughtExceptionBacktrace() involving the script).

  \section1 Engine Configuration

  The globalObject() function returns the \bold {Global Object}
  associated with the script engine. Properties of the Global Object
  are accessible from any script code (i.e. they are global
  variables). Typically, before evaluating "user" scripts, you will
  want to configure a script engine by adding one or more properties
  to the Global Object:

  \snippet doc/src/snippets/code/src_script_qjsengine.cpp 3

  Adding custom properties to the scripting environment is one of the
  standard means of providing a scripting API that is specific to your
  application. Usually these custom properties are objects created by
  the newQObject() or newObject() functions.

  \section1 Script Exceptions

  evaluate() can throw a script exception (e.g. due to a syntax
  error); in that case, the return value is the value that was thrown
  (typically an \c{Error} object). You can check whether the
  evaluation caused an exception by calling hasUncaughtException(). In
  that case, you can call toString() on the error object to obtain an
  error message. The current uncaught exception is also available
  through uncaughtException().
  Calling clearExceptions() will cause any uncaught exceptions to be
  cleared.

  \snippet doc/src/snippets/code/src_script_qjsengine.cpp 4

  \section1 Script Object Creation

  Use newObject() to create a JavaScript object; this is the
  C++ equivalent of the script statement \c{new Object()}. You can use
  the object-specific functionality in QJSValue to manipulate the
  script object (e.g. QJSValue::setProperty()). Similarly, use
  newArray() to create a JavaScript array object. Use newDate() to
  create a \c{Date} object, and newRegExp() to create a \c{RegExp}
  object.

  \section1 QObject Integration

  Use newQObject() to wrap a QObject (or subclass)
  pointer. newQObject() returns a proxy script object; properties,
  children, and signals and slots of the QObject are available as
  properties of the proxy object. No binding code is needed because it
  is done dynamically using the Qt meta object system.

  \snippet doc/src/snippets/code/src_script_qjsengine.cpp 5

  \sa QJSValue, {Making Applications Scriptable}

*/

QT_BEGIN_NAMESPACE


/*!
    Constructs a QJSEngine object.

    The globalObject() is initialized to have properties as described in
    \l{ECMA-262}, Section 15.1.
*/
QJSEngine::QJSEngine()
    : d(new QV8Engine(this))
{
}

#ifdef QT_DEPRECATED

/*!
    \internal
*/
QJSEngine::QJSEngine(QJSEngine::ContextOwnership ownership)
    : d(new QV8Engine(this, ownership))
{
}

#endif // QT_DEPRECATED

/*!
    Constructs a QJSEngine object with the given \a parent.

    The globalObject() is initialized to have properties as described in
    \l{ECMA-262}, Section 15.1.
*/

QJSEngine::QJSEngine(QObject *parent)
    : QObject(parent)
    , d(new QV8Engine(this))
{
}

QJSEngine::QJSEngine(QJSEnginePrivate &dd, QObject *parent)
    : QObject(dd, parent)
    , d(new QV8Engine(this))
{
}

/*!
    Destroys this QJSEngine.
*/
QJSEngine::~QJSEngine()
{
    delete d;
}

/*!
    \fn QV8Engine *QJSEngine::handle() const
    \internal
*/

#ifdef QT_DEPRECATED

/*!
    \obsolete

    Returns true if the last script evaluation resulted in an uncaught
    exception; otherwise returns false.

    The exception state is cleared when evaluate() is called.

    \sa uncaughtException(), uncaughtExceptionLineNumber(),
      uncaughtExceptionBacktrace()
*/
bool QJSEngine::hasUncaughtException() const
{
    Q_D(const QJSEngine);
    QScriptIsolate api(d);
    return d->hasUncaughtException();
}

/*!
    \obsolete

    Returns the current uncaught exception, or an invalid QJSValue
    if there is no uncaught exception.

    The exception value is typically an \c{Error} object; in that case,
    you can call toString() on the return value to obtain an error
    message.

    \sa hasUncaughtException(), uncaughtExceptionLineNumber(),
      uncaughtExceptionBacktrace()
*/
QJSValue QJSEngine::uncaughtException() const
{
    Q_D(const QJSEngine);
    QScriptIsolate api(d);
    return d->scriptValueFromInternal(d->uncaughtException());
}

/*!
    \obsolete

    Clears any uncaught exceptions in this engine.

    \sa hasUncaughtException()
*/
void QJSEngine::clearExceptions()
{
    Q_D(QJSEngine);
    QScriptIsolate api(d);
    d->clearExceptions();
}

#endif // QT_DEPRECATED

/*!
    Runs the garbage collector.

    The garbage collector will attempt to reclaim memory by locating and disposing of objects that are
    no longer reachable in the script environment.

    Normally you don't need to call this function; the garbage collector will automatically be invoked
    when the QJSEngine decides that it's wise to do so (i.e. when a certain number of new objects
    have been created). However, you can call this function to explicitly request that garbage
    collection should be performed as soon as possible.

    \sa reportAdditionalMemoryCost()
*/
void QJSEngine::collectGarbage()
{
    Q_D(QJSEngine);
    QScriptIsolate api(d);
    d->collectGarbage();
}

/*!
    Evaluates \a program, using \a lineNumber as the base line number,
    and returns the result of the evaluation.

    The script code will be evaluated in the current context.

    The evaluation of \a program can cause an exception in the
    engine; in this case the return value will be the exception
    that was thrown (typically an \c{Error} object). You can call
    hasUncaughtException() to determine if an exception occurred in
    the last call to evaluate().

    \a lineNumber is used to specify a starting line number for \a
    program; line number information reported by the engine that pertain
    to this evaluation (e.g. uncaughtExceptionLineNumber()) will be
    based on this argument. For example, if \a program consists of two
    lines of code, and the statement on the second line causes a script
    exception, uncaughtExceptionLineNumber() would return the given \a
    lineNumber plus one. When no starting line number is specified, line
    numbers will be 1-based.

    \a fileName is used for error reporting. For example in error objects
    the file name is accessible through the "fileName" property if it's
    provided with this function.
*/
QJSValue QJSEngine::evaluate(const QString& program, const QString& fileName, int lineNumber)
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return QJSValuePrivate::get(d->evaluate(program, fileName, lineNumber));
}

#ifdef QT_DEPRECATED

/*!
  \obsolete

  Returns a QJSValue of the primitive type Null.

  \sa nullValue()
*/
QJSValue QJSEngine::nullValue()
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return QJSValuePrivate::get(new QJSValuePrivate(d, v8::Null()));
}

/*!
  \obsolete

  Returns a QJSValue of the primitive type Undefined.

  \sa nullValue()
*/
QJSValue QJSEngine::undefinedValue()
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return QJSValuePrivate::get(new QJSValuePrivate(d, v8::Undefined()));
}

#endif // QT_DEPRECATED

/*!
  Creates a JavaScript object of class Object.

  The prototype of the created object will be the Object
  prototype object.

  \sa newArray(), QJSValue::setProperty()
*/
QJSValue QJSEngine::newObject()
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return QJSValuePrivate::get(new QJSValuePrivate(d, v8::Object::New()));
}

/*!
  Creates a JavaScript object of class Array with the given \a length.

  \sa newObject()
*/
QJSValue QJSEngine::newArray(uint length)
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return QJSValuePrivate::get(d->newArray(length));
}

/*!
  Creates a JavaScript object that wraps the given QObject \a
  object, using the given \a ownership. The given \a options control
  various aspects of the interaction with the resulting script object.

  Signals and slots, properties and children of \a object are
  available as properties of the created QJSValue.

  If \a object is a null pointer, this function returns nullValue().

  If a default prototype has been registered for the \a object's class
  (or its superclass, recursively), the prototype of the new script
  object will be set to be that default prototype.

  If the given \a object is deleted outside of the engine's control, any
  attempt to access the deleted QObject's members through the JavaScript
  wrapper object (either by script code or C++) will result in a
  script exception.

  \sa QJSValue::toQObject(), reportAdditionalMemoryCost()
*/
QJSValue QJSEngine::newQObject(QObject *object)
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return d->scriptValueFromInternal(d->newQObject(object, QV8Engine::JavaScriptOwnership));
}

#ifdef QT_DEPRECATED

/*!
  \obsolete

  Creates a JavaScript object holding the given variant \a value.

  If a default prototype has been registered with the meta type id of
  \a value, then the prototype of the created object will be that
  prototype; otherwise, the prototype will be the Object prototype
  object.

  \sa setDefaultPrototype(), QJSValue::toVariant(), reportAdditionalMemoryCost()
*/
QJSValue QJSEngine::newVariant(const QVariant &value)
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return d->scriptValueFromInternal(d->newVariant(value));
}

#endif // QT_DEPRECATED

/*!
  Returns this engine's Global Object.

  By default, the Global Object contains the built-in objects that are
  part of \l{ECMA-262}, such as Math, Date and String. Additionally,
  you can set properties of the Global Object to make your own
  extensions available to all script code. Non-local variables in
  script code will be created as properties of the Global Object, as
  well as local variables in global code.
*/
QJSValue QJSEngine::globalObject() const
{
    Q_D(const QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return d->scriptValueFromInternal(d->global());
}

#ifdef QT_DEPRECATED

/*!
  \obsolete

  Converts the given \a value to an object, if such a conversion is
  possible; otherwise returns an invalid QJSValue. The conversion
  is performed according to the following table:

    \table
    \header \o Input Type \o Result
    \row    \o Undefined  \o An invalid QJSValue.
    \row    \o Null       \o An invalid QJSValue.
    \row    \o Boolean    \o A new Boolean object whose internal value is set to the value of the boolean.
    \row    \o Number     \o A new Number object whose internal value is set to the value of the number.
    \row    \o String     \o A new String object whose internal value is set to the value of the string.
    \row    \o Object     \o The result is the object itself (no conversion).
    \endtable

    \sa newObject()
*/
QJSValue QJSEngine::toObject(const QJSValue& value)
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return QJSValuePrivate::get(QJSValuePrivate::get(value)->toObject(d));
}

/*!
  \obsolete

  Creates a JavaScript object of class Date from the given \a value.

  \sa QJSValue::toDateTime()
*/
QJSValue QJSEngine::newDate(const QDateTime &dt)
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return d->scriptValueFromInternal(QJSConverter::toDateTime(dt));
}

/*!
  \obsolete

  Creates a JavaScript object of class Date with the given
  \a value (the number of milliseconds since 01 January 1970,
  UTC).
*/
QJSValue QJSEngine::newDate(double date)
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return d->scriptValueFromInternal(v8::Handle<v8::Value>(v8::Date::New(date)));
}

/*!
  \obsolete

  Creates a JavaScript object of class RegExp with the given
  \a regexp.

  \sa QJSValue::toRegExp()
*/
QJSValue QJSEngine::newRegExp(const QRegExp &regexp)
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return QJSValuePrivate::get(d->newRegExp(regexp));
}

/*!
  \obsolete

  Creates a JavaScript object of class RegExp with the given
  \a pattern and \a flags.

  The legal flags are 'g' (global), 'i' (ignore case), and 'm'
  (multiline).
*/
QJSValue QJSEngine::newRegExp(const QString &pattern, const QString &flags)
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return QJSValuePrivate::get(d->newRegExp(pattern, flags));
}

#endif // QT_DEPRECATED

/*!
 *  \internal
 * used by QJSEngine::toScriptValue
 */
QJSValue QJSEngine::create(int type, const void *ptr)
{
    Q_D(QJSEngine);
    QScriptIsolate api(d, QScriptIsolate::NotNullEngine);
    v8::HandleScope handleScope;
    return d->scriptValueFromInternal(d->metaTypeToJS(type, ptr));
}

/*!
    \internal
    \since 4.5
    convert \a value to \a type, store the result in \a ptr
*/
bool QJSEngine::convertV2(const QJSValue &value, int type, void *ptr)
{
    QJSValuePrivate *vp = QJSValuePrivate::get(value);
    QV8Engine *engine = vp->engine();
    if (engine) {
        QScriptIsolate api(engine, QScriptIsolate::NotNullEngine);
        v8::HandleScope handleScope;
        return engine->metaTypeFromJS(*vp, type, ptr);
    } else {
        switch (type) {
            case QMetaType::Bool:
                *reinterpret_cast<bool*>(ptr) = vp->toBool();
                return true;
            case QMetaType::Int:
                *reinterpret_cast<int*>(ptr) = vp->toInt32();
                return true;
            case QMetaType::UInt:
                *reinterpret_cast<uint*>(ptr) = vp->toUInt32();
                return true;
            case QMetaType::LongLong:
                *reinterpret_cast<qlonglong*>(ptr) = vp->toInteger();
                return true;
            case QMetaType::ULongLong:
                *reinterpret_cast<qulonglong*>(ptr) = vp->toInteger();
                return true;
            case QMetaType::Double:
                *reinterpret_cast<double*>(ptr) = vp->toNumber();
                return true;
            case QMetaType::QString:
                *reinterpret_cast<QString*>(ptr) = vp->toString();
                return true;
            case QMetaType::Float:
                *reinterpret_cast<float*>(ptr) = vp->toNumber();
                return true;
            case QMetaType::Short:
                *reinterpret_cast<short*>(ptr) = vp->toInt32();
                return true;
            case QMetaType::UShort:
                *reinterpret_cast<unsigned short*>(ptr) = vp->toUInt16();
                return true;
            case QMetaType::Char:
                *reinterpret_cast<char*>(ptr) = vp->toInt32();
                return true;
            case QMetaType::UChar:
                *reinterpret_cast<unsigned char*>(ptr) = vp->toUInt16();
                return true;
            case QMetaType::QChar:
                *reinterpret_cast<QChar*>(ptr) = vp->toUInt16();
                return true;
            default:
                return false;
        }
    }
}

/*! \fn QJSValue QJSEngine::toScriptValue(const T &value)

    Creates a QJSValue with the given \a value.

    \sa fromScriptValue()
*/

/*! \fn T QJSEngine::fromScriptValue(const QJSValue &value)

    Returns the given \a value converted to the template type \c{T}.

    \sa toScriptValue()
*/

QT_END_NAMESPACE

#include "moc_qjsengine.cpp"
