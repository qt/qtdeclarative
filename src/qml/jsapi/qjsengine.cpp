// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qjsengine.h"
#include "qjsengine_p.h"
#include "qjsvalue.h"
#include "qjsvalue_p.h"

#include "private/qv4engine_p.h"
#include "private/qv4mm_p.h"
#include "private/qv4errorobject_p.h"
#include "private/qv4globalobject_p.h"
#include "private/qv4script_p.h"
#include "private/qv4runtime_p.h"
#include <private/qv4dateobject_p.h>
#include <private/qqmlbuiltinfunctions_p.h>
#include <private/qqmldebugconnector_p.h>
#include <private/qv4qobjectwrapper_p.h>
#include <private/qv4stackframe_p.h>
#include <private/qv4module_p.h>
#include <private/qv4symbol_p.h>

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
#include <private/qqmlglobal_p.h>
#include <qqmlengine.h>

Q_DECLARE_METATYPE(QList<int>)

/*!
  \since 5.0
  \class QJSEngine
  \reentrant

  \brief The QJSEngine class provides an environment for evaluating JavaScript code.

  \ingroup qtjavascript
  \inmodule QtQml

  \section1 Evaluating Scripts

  Use evaluate() to evaluate script code.

  \snippet code/src_script_qjsengine.cpp 0

  evaluate() returns a QJSValue that holds the result of the
  evaluation. The QJSValue class provides functions for converting
  the result to various C++ types (e.g. QJSValue::toString()
  and QJSValue::toNumber()).

  The following code snippet shows how a script function can be
  defined and then invoked from C++ using QJSValue::call():

  \snippet code/src_script_qjsengine.cpp 1

  As can be seen from the above snippets, a script is provided to the
  engine in the form of a string. One common way of loading scripts is
  by reading the contents of a file and passing it to evaluate():

  \snippet code/src_script_qjsengine.cpp 2

  Here we pass the name of the file as the second argument to
  evaluate().  This does not affect evaluation in any way; the second
  argument is a general-purpose string that is stored in the \c Error
  object for debugging purposes.

  For larger pieces of functionality, you may want to encapsulate
  your code and data into modules. A module is a file that contains
  script code, variables, etc., and uses export statements to describe
  its interface towards the rest of the application. With the help of
  import statements, a module can refer to functionality from other modules.
  This allows building a scripted application from smaller connected building blocks
  in a safe way. In contrast, the approach of using evaluate() carries the risk
  that internal variables or functions from one evaluate() call accidentally pollute the
  global object and affect subsequent evaluations.

  The following example provides a module that can add numbers:

  \code
  export function sum(left, right)
  {
      return left + right
  }
  \endcode

  This module can be loaded with QJSEngine::import() if it is saved under
  the name \c{math.mjs}:

  \code
  QJSvalue module = myEngine.importModule("./math.mjs");
  QJSValue sumFunction = module.property("sum");
  QJSValue result = sumFunction.call(args);
  \endcode

  Modules can also use functionality from other modules using import
  statements:

  \code
  import { sum } from "./math.mjs";
  export function addTwice(left, right)
  {
      return sum(left, right) * 2;
  }
  \endcode

  Modules don't have to be files. They can be values registered with
  QJSEngine::registerModule():

  \code
  import version from "version";

  export function getVersion()
  {
      return version;
  }
  \endcode

  \code
  QJSValue version(610);
  myEngine.registerModule("version", version);
  QJSValue module = myEngine.importModule("./myprint.mjs");
  QJSValue getVersion = module.property("getVersion");
  QJSValue result = getVersion.call();
  \endcode

  Named exports are supported, but because they are treated as members of an
  object, the default export must be an ECMAScript object. Most of the newXYZ
  functions in QJSValue will return an object.

  \code
  QJSValue name("Qt6");
  QJSValue obj = myEngine.newObject();
  obj.setProperty("name", name);
  myEngine.registerModule("info", obj);
  \endcode

  \code
  import { name } from "info";

  export function getName()
  {
      return name;
  }
  \endcode

  \section1 Engine Configuration

  The globalObject() function returns the \b {Global Object}
  associated with the script engine. Properties of the Global Object
  are accessible from any script code (i.e. they are global
  variables). Typically, before evaluating "user" scripts, you will
  want to configure a script engine by adding one or more properties
  to the Global Object:

  \snippet code/src_script_qjsengine.cpp 3

  Adding custom properties to the scripting environment is one of the
  standard means of providing a scripting API that is specific to your
  application. Usually these custom properties are objects created by
  the newQObject() or newObject() functions.

  \section1 Script Exceptions

  evaluate() can throw a script exception (e.g. due to a syntax
  error). If it does, then evaluate() returns the value that was thrown
  (typically an \c{Error} object). Use \l QJSValue::isError() to check
  for exceptions.

  For detailed information about the error, use \l QJSValue::toString() to
  obtain an error message, and use \l QJSValue::property() to query the
  properties of the \c Error object. The following properties are available:

  \list
  \li \c name
  \li \c message
  \li \c fileName
  \li \c lineNumber
  \li \c stack
  \endlist

  \snippet code/src_script_qjsengine.cpp 4

  \section1 Script Object Creation

  Use newObject() to create a JavaScript object; this is the
  C++ equivalent of the script statement \c{new Object()}. You can use
  the object-specific functionality in QJSValue to manipulate the
  script object (e.g. QJSValue::setProperty()). Similarly, use
  newArray() to create a JavaScript array object.

  \section1 QObject Integration

  Use newQObject() to wrap a QObject (or subclass)
  pointer. newQObject() returns a proxy script object; properties,
  children, and signals and slots of the QObject are available as
  properties of the proxy object. No binding code is needed because it
  is done dynamically using the Qt meta object system.

  \snippet code/src_script_qjsengine.cpp 5

  Use newQMetaObject() to wrap a QMetaObject; this gives you a
  "script representation" of a QObject-based class. newQMetaObject()
  returns a proxy script object; enum values of the class are available
  as properties of the proxy object.

  Constructors exposed to the meta-object system (using Q_INVOKABLE) can be
  called from the script to create a new QObject instance with
  JavaScriptOwnership. For example, given the following class definition:

  \snippet code/src_script_qjsengine.cpp 7

  The \c staticMetaObject for the class can be exposed to JavaScript like so:

  \snippet code/src_script_qjsengine.cpp 8

  Instances of the class can then be created in JavaScript:

  \snippet code/src_script_qjsengine.cpp 9

  \note Currently only classes using the Q_OBJECT macro are supported; it is
  not possible to expose the \c staticMetaObject of a Q_GADGET class to
  JavaScript.

  \section2 Dynamic QObject Properties

  Dynamic QObject properties are not supported. For example, the following code
  will not work:

  \snippet code/src_script_qjsengine.cpp 6

  \section1 Extensions

  QJSEngine provides a compliant ECMAScript implementation. By default,
  familiar utilities like logging are not available, but they can be
  installed via the \l installExtensions() function.

  \sa QJSValue, {Making Applications Scriptable},
      {List of JavaScript Objects and Functions}

*/

/*!
    \enum QJSEngine::Extension

    This enum is used to specify extensions to be installed via
    \l installExtensions().

    \value TranslationExtension Indicates that translation functions (\c qsTr(),
        for example) should be installed. This also installs the Qt.uiLanguage property.

    \value ConsoleExtension Indicates that console functions (\c console.log(),
        for example) should be installed.

    \value GarbageCollectionExtension Indicates that garbage collection
        functions (\c gc(), for example) should be installed.

    \value AllExtensions Indicates that all extension should be installed.

    \b TranslationExtension

    The relation between script translation functions and C++ translation
    functions is described in the following table:

    \table
    \header \li Script Function \li Corresponding C++ Function
    \row    \li qsTr()       \li QObject::tr()
    \row    \li QT_TR_NOOP() \li QT_TR_NOOP()
    \row    \li qsTranslate() \li QCoreApplication::translate()
    \row    \li QT_TRANSLATE_NOOP() \li QT_TRANSLATE_NOOP()
    \row    \li qsTrId() \li qtTrId()
    \row    \li QT_TRID_NOOP() \li QT_TRID_NOOP()
    \endtable

    This flag also adds an \c arg() function to the string prototype.

    For more information, see the \l {Internationalization with Qt}
    documentation.

    \b ConsoleExtension

    The \l {Console API}{console} object implements a subset of the
    \l {https://developer.mozilla.org/en-US/docs/Web/API/Console}{Console API},
    which provides familiar logging functions, such as \c console.log().

    The list of functions added is as follows:

    \list
    \li \c console.assert()
    \li \c console.debug()
    \li \c console.exception()
    \li \c console.info()
    \li \c console.log() (equivalent to \c console.debug())
    \li \c console.error()
    \li \c console.time()
    \li \c console.timeEnd()
    \li \c console.trace()
    \li \c console.count()
    \li \c console.warn()
    \li \c {print()} (equivalent to \c console.debug())
    \endlist

    For more information, see the \l {Console API} documentation.

    \b GarbageCollectionExtension

    The \c gc() function is equivalent to calling \l collectGarbage().
*/

QT_BEGIN_NAMESPACE

static void checkForApplicationInstance()
{
    if (!QCoreApplication::instance())
        qFatal("QJSEngine: Must construct a QCoreApplication before a QJSEngine");
}

/*!
    Constructs a QJSEngine object.

    The globalObject() is initialized to have properties as described in
    \l{ECMA-262}, Section 15.1.
*/
QJSEngine::QJSEngine()
    : QJSEngine(nullptr)
{
}

/*!
    Constructs a QJSEngine object with the given \a parent.

    The globalObject() is initialized to have properties as described in
    \l{ECMA-262}, Section 15.1.
*/

QJSEngine::QJSEngine(QObject *parent)
    : QObject(*new QJSEnginePrivate, parent)
    , m_v4Engine(new QV4::ExecutionEngine(this))
{
    checkForApplicationInstance();

    QJSEnginePrivate::addToDebugServer(this);
}

/*!
    \internal
*/
QJSEngine::QJSEngine(QJSEnginePrivate &dd, QObject *parent)
    : QObject(dd, parent)
    , m_v4Engine(new QV4::ExecutionEngine(this))
{
    checkForApplicationInstance();
}

/*!
    Destroys this QJSEngine.

    Garbage is not collected from the persistent JS heap during QJSEngine
    destruction. If you need all memory freed, call collectGarbage() manually
    right before destroying the QJSEngine.
*/
QJSEngine::~QJSEngine()
{
    QJSEnginePrivate::removeFromDebugServer(this);
    delete m_v4Engine;
}

/*!
    \fn QV4::ExecutionEngine *QJSEngine::handle() const
    \internal
*/

/*!
    Runs the garbage collector.

    The garbage collector will attempt to reclaim memory by locating and disposing of objects that are
    no longer reachable in the script environment.

    Normally you don't need to call this function; the garbage collector will automatically be invoked
    when the QJSEngine decides that it's wise to do so (i.e. when a certain number of new objects
    have been created). However, you can call this function to explicitly request that garbage
    collection should be performed as soon as possible.
*/
void QJSEngine::collectGarbage()
{
    m_v4Engine->memoryManager->runGC();
}

/*!
    \since 5.6

    Installs JavaScript \a extensions to add functionality that is not
    available in a standard ECMAScript implementation.

    The extensions are installed on the given \a object, or on the
    \l {globalObject()}{Global Object} if no object is specified.

    Several extensions can be installed at once by \c {OR}-ing the enum values:

    \code
    installExtensions(QJSEngine::TranslationExtension | QJSEngine::ConsoleExtension);
    \endcode

    \sa Extension
*/
void QJSEngine::installExtensions(QJSEngine::Extensions extensions, const QJSValue &object)
{
    QV4::ExecutionEngine *otherEngine = QJSValuePrivate::engine(&object);
    if (otherEngine && otherEngine != m_v4Engine) {
        qWarning("QJSEngine: Trying to install extensions from a different engine");
        return;
    }

    QV4::Scope scope(m_v4Engine);
    QV4::ScopedObject obj(scope, QJSValuePrivate::asReturnedValue(&object));
    if (!obj)
        obj = scope.engine->globalObject;

    QV4::GlobalExtensions::init(obj, extensions);
}

/*!
  \since 5.14
  Interrupts or re-enables JavaScript execution.

  If \a interrupted is \c true, any JavaScript executed by this engine
  immediately aborts and returns an error object until this function is
  called again with a value of \c false for \a interrupted.

  This function is thread safe. You may call it from a different thread
  in order to interrupt, for example, an infinite loop in JavaScript.
*/
void QJSEngine::setInterrupted(bool interrupted)
{
    m_v4Engine->isInterrupted.storeRelaxed(interrupted);
}

/*!
  \since 5.14
  Returns whether JavaScript execution is currently interrupted.

  \sa setInterrupted()
*/
bool QJSEngine::isInterrupted() const
{
    return m_v4Engine->isInterrupted.loadRelaxed();
}

static QUrl urlForFileName(const QString &fileName)
{
    if (!fileName.startsWith(QLatin1Char(':')))
        return QUrl::fromLocalFile(fileName);

    QUrl url;
    url.setPath(fileName.mid(1));
    url.setScheme(QLatin1String("qrc"));
    return url;
}

/*!
    Evaluates \a program, using \a lineNumber as the base line number,
    and returns the result of the evaluation.

    The script code will be evaluated in the context of the global object.

    \note If you need to evaluate inside a QML context, use \l QQmlExpression
    instead.

    The evaluation of \a program can cause an \l{Script Exceptions}{exception} in the
    engine; in this case the return value will be the exception
    that was thrown (typically an \c{Error} object; see
    QJSValue::isError()).

    \a lineNumber is used to specify a starting line number for \a
    program; line number information reported by the engine that pertains
    to this evaluation will be based on this argument. For example, if
    \a program consists of two lines of code, and the statement on the
    second line causes a script exception, the exception line number
    would be \a lineNumber plus one. When no starting line number is
    specified, line numbers will be 1-based.

    \a fileName is used for error reporting. For example, in error objects
    the file name is accessible through the "fileName" property if it is
    provided with this function.

    \a exceptionStackTrace is used to report whether an uncaught exception was
    thrown. If you pass a non-null pointer to a QStringList to it, it will set
    it to list of "stackframe messages" if the script threw an unhandled
    exception, or an empty list otherwise. A stackframe message has the format
    function name:line number:column:file name
    \note In some cases, e.g. for native functions, function name and file name
    can be empty and line number and column can be -1.

    \note If an exception was thrown and the exception value is not an
    Error instance (i.e., QJSValue::isError() returns \c false), the
    exception value will still be returned. Use \c exceptionStackTrace->isEmpty()
    to distinguish whether the value was a normal or an exceptional return
    value.

    \sa QQmlExpression::evaluate
*/
QJSValue QJSEngine::evaluate(const QString& program, const QString& fileName, int lineNumber, QStringList *exceptionStackTrace)
{
    QV4::ExecutionEngine *v4 = m_v4Engine;
    QV4::Scope scope(v4);
    QV4::ScopedValue result(scope);

    QV4::Script script(v4->rootContext(), QV4::Compiler::ContextType::Global, program, urlForFileName(fileName).toString(), lineNumber);
    script.strictMode = false;
    if (v4->currentStackFrame)
        script.strictMode = v4->currentStackFrame->v4Function->isStrict();
    else if (v4->globalCode)
        script.strictMode = v4->globalCode->isStrict();
    script.inheritContext = true;
    script.parse();
    if (!scope.hasException())
        result = script.run();
    if (exceptionStackTrace)
        exceptionStackTrace->clear();
    if (scope.hasException()) {
        QV4::StackTrace trace;
        result = v4->catchException(&trace);
        if (exceptionStackTrace) {
            for (auto &&frame: trace)
                exceptionStackTrace->push_back(QString::fromLatin1("%1:%2:%3:%4").arg(
                                          frame.function,
                                          QString::number(qAbs(frame.line)),
                                          QString::number(frame.column),
                                          frame.source)
                                      );
        }
    }
    if (v4->isInterrupted.loadRelaxed())
        result = v4->newErrorObject(QStringLiteral("Interrupted"));

    return QJSValuePrivate::fromReturnedValue(result->asReturnedValue());
}

/*!
    Imports the module located at \a fileName and returns a module namespace object that
    contains all exported variables, constants and functions as properties.

    If this is the first time the module is imported in the engine, the file is loaded
    from the specified location in either the local file system or the Qt resource system
    and evaluated as an ECMAScript module. The file is expected to be encoded in UTF-8 text.

    Subsequent imports of the same module will return the previously imported instance. Modules
    are singletons and remain around until the engine is destroyed.

    The specified \a fileName will internally be normalized using \l QFileInfo::canonicalFilePath().
    That means that multiple imports of the same file on disk using different relative paths will
    load the file only once.

    \note If an exception is thrown during the loading of the module, the return value
    will be the exception (typically an \c{Error} object; see QJSValue::isError()).

    \sa registerModule()

    \since 5.12
 */
QJSValue QJSEngine::importModule(const QString &fileName)
{
    const QUrl url = urlForFileName(QFileInfo(fileName).canonicalFilePath());
    const auto module = m_v4Engine->loadModule(url);
    if (m_v4Engine->hasException)
        return QJSValuePrivate::fromReturnedValue(m_v4Engine->catchException());

    QV4::Scope scope(m_v4Engine);
    if (const auto compiled = module.compiled) {
        QV4::Scoped<QV4::Module> moduleNamespace(scope, compiled->instantiate(m_v4Engine));
        if (m_v4Engine->hasException)
            return QJSValuePrivate::fromReturnedValue(m_v4Engine->catchException());
        compiled->evaluate();
        if (!m_v4Engine->isInterrupted.loadRelaxed())
            return QJSValuePrivate::fromReturnedValue(moduleNamespace->asReturnedValue());
        return QJSValuePrivate::fromReturnedValue(
                    m_v4Engine->newErrorObject(QStringLiteral("Interrupted"))->asReturnedValue());
    }

    // If there is neither a native nor a compiled module, we should have seen an exception
    Q_ASSERT(module.native);

    return QJSValuePrivate::fromReturnedValue(module.native->asReturnedValue());
}

/*!
    Registers a QJSValue to serve as a module. After this function is called,
    all modules that import \a moduleName will import the value of \a value
    instead of loading \a moduleName from the filesystem.

    Any valid QJSValue can be registered, but named exports (i.e.
    \c {import { name } from "info"} are treated as members of an object, so
    the default export must be created with one of the newXYZ methods of
    QJSEngine.

    Because this allows modules that do not exist on the filesystem to be imported,
    scripting applications can use this to provide built-in modules, similar to
    Node.js.

    Returns \c true on success, \c false otherwise.

    \note The QJSValue \a value is not called or read until it is used by another module.
    This means that there is no code to evaluate, so no errors will be seen until
    another module throws an exception while trying to load this module.

    \warning Attempting to access a named export from a QJSValue that is not an
    object will trigger a \l{Script Exceptions}{exception}.

    \sa importModule()
 */
bool QJSEngine::registerModule(const QString &moduleName, const QJSValue &value)
{
    QV4::Scope scope(m_v4Engine);
    QV4::ScopedValue v4Value(scope, QJSValuePrivate::asReturnedValue(&value));
    m_v4Engine->registerNativeModule(QUrl(moduleName), v4Value);
    if (m_v4Engine->hasException)
        return false;
    return true;
}

/*!
  Creates a JavaScript object of class Object.

  The prototype of the created object will be the Object
  prototype object.

  \sa newArray(), QJSValue::setProperty()
*/
QJSValue QJSEngine::newObject()
{
    QV4::Scope scope(m_v4Engine);
    QV4::ScopedValue v(scope, m_v4Engine->newObject());
    return QJSValuePrivate::fromReturnedValue(v->asReturnedValue());
}

/*!
  \since 6.2

  Creates a JavaScript object of class Symbol, with value \a name.

  The prototype of the created object will be the Symbol prototype object.

  \sa newObject()
*/
QJSValue QJSEngine::newSymbol(const QString &name)
{
    QV4::Scope scope(m_v4Engine);
    QV4::ScopedValue v(scope, QV4::Symbol::create(m_v4Engine, u'@' + name));
    return QJSValuePrivate::fromReturnedValue(v->asReturnedValue());
}

/*!
  \since 5.12

  Creates a JavaScript object of class Error, with \a message as the error
  message.

  The prototype of the created object will be \a errorType.

  \sa newObject(), throwError(), QJSValue::isError()
*/
QJSValue QJSEngine::newErrorObject(QJSValue::ErrorType errorType, const QString &message)
{
    QV4::Scope scope(m_v4Engine);
    QV4::ScopedObject error(scope);
    switch (errorType) {
    case QJSValue::RangeError:
        error = m_v4Engine->newRangeErrorObject(message);
        break;
    case QJSValue::SyntaxError:
        error = m_v4Engine->newSyntaxErrorObject(message);
        break;
    case QJSValue::TypeError:
        error = m_v4Engine->newTypeErrorObject(message);
        break;
    case QJSValue::URIError:
        error = m_v4Engine->newURIErrorObject(message);
        break;
    case QJSValue::ReferenceError:
        error = m_v4Engine->newReferenceErrorObject(message);
        break;
    case QJSValue::EvalError:
        error = m_v4Engine->newEvalErrorObject(message);
        break;
    case QJSValue::GenericError:
        error = m_v4Engine->newErrorObject(message);
        break;
    case QJSValue::NoError:
        return QJSValue::UndefinedValue;
    }
    return QJSValuePrivate::fromReturnedValue(error->asReturnedValue());
}

/*!
  Creates a JavaScript object of class Array with the given \a length.

  \sa newObject()
*/
QJSValue QJSEngine::newArray(uint length)
{
    QV4::Scope scope(m_v4Engine);
    QV4::ScopedArrayObject array(scope, m_v4Engine->newArrayObject());
    if (length < 0x1000)
        array->arrayReserve(length);
    array->setArrayLengthUnchecked(length);
    return QJSValuePrivate::fromReturnedValue(array.asReturnedValue());
}

/*!
  Creates a JavaScript object that wraps the given QObject \a
  object, using JavaScriptOwnership.

  Signals and slots, properties and children of \a object are
  available as properties of the created QJSValue.

  If \a object is a null pointer, this function returns a null value.

  If a default prototype has been registered for the \a object's class
  (or its superclass, recursively), the prototype of the new script
  object will be set to be that default prototype.

  If the given \a object is deleted outside of the engine's control, any
  attempt to access the deleted QObject's members through the JavaScript
  wrapper object (either by script code or C++) will result in a
  \l{Script Exceptions}{script exception}.

  \sa QJSValue::toQObject()
*/
QJSValue QJSEngine::newQObject(QObject *object)
{
    QV4::ExecutionEngine *v4 = m_v4Engine;
    QV4::Scope scope(v4);
    if (object) {
        QQmlData *ddata = QQmlData::get(object, true);
        if (!ddata || !ddata->explicitIndestructibleSet)
            QQmlEngine::setObjectOwnership(object, QQmlEngine::JavaScriptOwnership);
    }
    QV4::ScopedValue v(scope, QV4::QObjectWrapper::wrap(v4, object));
    return QJSValuePrivate::fromReturnedValue(v->asReturnedValue());
}

/*!
  \since 5.8

  Creates a JavaScript object that wraps the given QMetaObject
  The \a metaObject must outlive the script engine. It is recommended to only
  use this method with static metaobjects.


  When called as a constructor, a new instance of the class will be created.
  Only constructors exposed by Q_INVOKABLE will be visible from the script engine.

  \sa newQObject(), {QObject Integration}
*/

QJSValue QJSEngine::newQMetaObject(const QMetaObject* metaObject) {
    QV4::ExecutionEngine *v4 = m_v4Engine;
    QV4::Scope scope(v4);
    QV4::ScopedValue v(scope, QV4::QMetaObjectWrapper::create(v4, metaObject));
    return QJSValuePrivate::fromReturnedValue(v->asReturnedValue());
}

/*! \fn template <typename T> QJSValue QJSEngine::newQMetaObject()

  \since 5.8
  Creates a JavaScript object that wraps the static QMetaObject associated
  with class \c{T}.

  \sa newQObject(), {QObject Integration}
*/


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
    QV4::Scope scope(m_v4Engine);
    QV4::ScopedValue v(scope, m_v4Engine->globalObject);
    return QJSValuePrivate::fromReturnedValue(v->asReturnedValue());
}

QJSPrimitiveValue QJSEngine::createPrimitive(QMetaType type, const void *ptr)
{
    QV4::Scope scope(m_v4Engine);
    QV4::ScopedValue v(scope, m_v4Engine->metaTypeToJS(type, ptr));
    return QV4::ExecutionEngine::createPrimitive(v);
}

QJSManagedValue QJSEngine::createManaged(QMetaType type, const void *ptr)
{
    QJSManagedValue result(m_v4Engine);
    *result.d = m_v4Engine->metaTypeToJS(type, ptr);
    return result;
}

/*!
 *  \internal
 * used by QJSEngine::toScriptValue
 */
QJSValue QJSEngine::create(QMetaType type, const void *ptr)
{
    QV4::Scope scope(m_v4Engine);
    QV4::ScopedValue v(scope, scope.engine->metaTypeToJS(type, ptr));
    return QJSValuePrivate::fromReturnedValue(v->asReturnedValue());
}

bool QJSEngine::convertPrimitive(const QJSPrimitiveValue &value, QMetaType type, void *ptr)
{
    switch (value.type()) {
    case QJSPrimitiveValue::Undefined:
        return QV4::ExecutionEngine::metaTypeFromJS(QV4::Value::undefinedValue(), type, ptr);
    case QJSPrimitiveValue::Null:
        return QV4::ExecutionEngine::metaTypeFromJS(QV4::Value::nullValue(), type, ptr);
    case QJSPrimitiveValue::Boolean:
        return QV4::ExecutionEngine::metaTypeFromJS(QV4::Value::fromBoolean(value.toBoolean()), type, ptr);
    case QJSPrimitiveValue::Integer:
        return QV4::ExecutionEngine::metaTypeFromJS(QV4::Value::fromInt32(value.toInteger()), type, ptr);
    case QJSPrimitiveValue::Double:
        return QV4::ExecutionEngine::metaTypeFromJS(QV4::Value::fromDouble(value.toDouble()), type, ptr);
    case QJSPrimitiveValue::String:
        return convertString(value.toString(), type, ptr);
    }

    Q_UNREACHABLE_RETURN(false);
}

bool QJSEngine::convertManaged(const QJSManagedValue &value, int type, void *ptr)
{
    return convertManaged(value, QMetaType(type), ptr);
}

bool QJSEngine::convertManaged(const QJSManagedValue &value, QMetaType type, void *ptr)
{
    return QV4::ExecutionEngine::metaTypeFromJS(*value.d, type, ptr);
}

bool QJSEngine::convertString(const QString &string, QMetaType metaType, void *ptr)
{
    // have a string based value without engine. Do conversion manually
    if (metaType == QMetaType::fromType<bool>()) {
        *reinterpret_cast<bool*>(ptr) = string.size() != 0;
        return true;
    }
    if (metaType == QMetaType::fromType<QString>()) {
        *reinterpret_cast<QString*>(ptr) = string;
        return true;
    }
    if (metaType == QMetaType::fromType<QUrl>()) {
        *reinterpret_cast<QUrl *>(ptr) = QUrl(string);
        return true;
    }

    double d = QV4::RuntimeHelpers::stringToNumber(string);
    switch (metaType.id()) {
    case QMetaType::Int:
        *reinterpret_cast<int*>(ptr) = QV4::Value::toInt32(d);
        return true;
    case QMetaType::UInt:
        *reinterpret_cast<uint*>(ptr) = QV4::Value::toUInt32(d);
        return true;
    case QMetaType::Long:
        *reinterpret_cast<long*>(ptr) = QV4::Value::toInteger(d);
        return true;
    case QMetaType::ULong:
        *reinterpret_cast<ulong*>(ptr) = QV4::Value::toInteger(d);
        return true;
    case QMetaType::LongLong:
        *reinterpret_cast<qlonglong*>(ptr) = QV4::Value::toInteger(d);
        return true;
    case QMetaType::ULongLong:
        *reinterpret_cast<qulonglong*>(ptr) = QV4::Value::toInteger(d);
        return true;
    case QMetaType::Double:
        *reinterpret_cast<double*>(ptr) = d;
        return true;
    case QMetaType::Float:
        *reinterpret_cast<float*>(ptr) = d;
        return true;
    case QMetaType::Short:
        *reinterpret_cast<short*>(ptr) = QV4::Value::toInt32(d);
        return true;
    case QMetaType::UShort:
        *reinterpret_cast<unsigned short*>(ptr) = QV4::Value::toUInt32(d);
        return true;
    case QMetaType::Char:
        *reinterpret_cast<char*>(ptr) = QV4::Value::toInt32(d);
        return true;
    case QMetaType::UChar:
        *reinterpret_cast<unsigned char*>(ptr) = QV4::Value::toUInt32(d);
        return true;
    case QMetaType::QChar:
        *reinterpret_cast<QChar*>(ptr) = QChar(QV4::Value::toUInt32(d));
        return true;
    case QMetaType::Char16:
        *reinterpret_cast<char16_t *>(ptr) = QV4::Value::toUInt32(d);
        return true;
    default:
        return false;
    }
}

/*!
    \internal
    convert \a value to \a type, store the result in \a ptr
*/
bool QJSEngine::convertV2(const QJSValue &value, QMetaType metaType, void *ptr)
{
    if (const QString *string = QJSValuePrivate::asQString(&value))
        return convertString(*string, metaType, ptr);

    // Does not need scoping since QJSValue still holds on to the value.
    return QV4::ExecutionEngine::metaTypeFromJS(QJSValuePrivate::asReturnedValue(&value), metaType, ptr);
}

bool QJSEngine::convertVariant(const QVariant &value, QMetaType metaType, void *ptr)
{
    // TODO: We could probably avoid creating a QV4::Value in many cases, but we'd have to
    //       duplicate much of metaTypeFromJS and some methods of QV4::Value itself here.
    QV4::Scope scope(handle());
    QV4::ScopedValue scoped(scope, scope.engine->fromVariant(value));
    return QV4::ExecutionEngine::metaTypeFromJS(scoped, metaType, ptr);
}

bool QJSEngine::convertMetaType(QMetaType fromType, const void *from, QMetaType toType, void *to)
{
    // TODO: We could probably avoid creating a QV4::Value in many cases, but we'd have to
    //       duplicate much of metaTypeFromJS and some methods of QV4::Value itself here.
    QV4::Scope scope(handle());
    QV4::ScopedValue scoped(scope, scope.engine->fromData(fromType, from));
    return QV4::ExecutionEngine::metaTypeFromJS(scoped, toType, to);
}

QString QJSEngine::convertQObjectToString(QObject *object)
{
    return QV4::QObjectWrapper::objectToString(
                handle(), object ? object->metaObject() : nullptr, object);
}

QString QJSEngine::convertDateTimeToString(const QDateTime &dateTime)
{
    return QV4::DateObject::dateTimeToString(dateTime, handle());
}

QDate QJSEngine::convertDateTimeToDate(const QDateTime &dateTime)
{
    return QV4::DateObject::dateTimeToDate(dateTime);
}

/*! \fn template <typename T> QJSValue QJSEngine::toScriptValue(const T &value)

    Creates a QJSValue with the given \a value.

    \sa fromScriptValue(), coerceValue()
*/

/*! \fn template <typename T> QJSManagedValue QJSEngine::toManagedValue(const T &value)

    Creates a QJSManagedValue with the given \a value.

    \sa fromManagedValue(), coerceValue()
*/

/*! \fn template <typename T> QJSPrimitiveValue QJSEngine::toPrimitiveValue(const T &value)

    Creates a QJSPrimitiveValue with the given \a value.

    Since QJSPrimitiveValue can only hold int, bool, double, QString, and the
    equivalents of JavaScript \c null and \c undefined, the value will be
    coerced aggressively if you pass any other type.

    \sa fromPrimitiveValue(), coerceValue()
*/

/*! \fn template <typename T> T QJSEngine::fromScriptValue(const QJSValue &value)

    Returns the given \a value converted to the template type \c{T}.

    \sa toScriptValue(), coerceValue()
*/

/*! \fn template <typename T> T QJSEngine::fromManagedValue(const QJSManagedValue &value)

    Returns the given \a value converted to the template type \c{T}.

    \sa toManagedValue(), coerceValue()
*/

/*! \fn template <typename T> T QJSEngine::fromPrimitiveValue(const QJSPrimitiveValue &value)

    Returns the given \a value converted to the template type \c{T}.

    Since QJSPrimitiveValue can only hold int, bool, double, QString, and the
    equivalents of JavaScript \c null and \c undefined, the value will be
    coerced aggressively if you request any other type.

    \sa toPrimitiveValue(), coerceValue()
*/

/*! \fn template <typename T> T QJSEngine::fromVariant(const QVariant &value)

    Returns the given \a value converted to the template type \c{T}.
    The conversion is done in JavaScript semantics. Those differ from
    qvariant_cast's semantics. There are a number of implicit
    conversions between JavaScript-equivalent types that are not
    performed by qvariant_cast by default.

    \sa coerceValue(), fromScriptValue(), qvariant_cast()
*/

/*! \fn template <typename From, typename To> T QJSEngine::coerceValue(const From &from)

    Returns the given \a from converted to the template type \c{To}.
    The conversion is done in JavaScript semantics. Those differ from
    qvariant_cast's semantics. There are a number of implicit
    conversions between JavaScript-equivalent types that are not
    performed by qvariant_cast by default. This method is a generalization of
    all the other conversion methods in this class.

    \sa fromVariant(), qvariant_cast(), fromScriptValue(), toScriptValue()
*/

/*!
    Throws a run-time error (exception) with the given \a message.

    This method is the C++ counterpart of a \c throw() expression in
    JavaScript. It enables C++ code to report run-time errors to QJSEngine.
    Therefore it should only be called from C++ code that was invoked by a
    JavaScript function through QJSEngine.

    When returning from C++, the engine will interrupt the normal flow of
    execution and call the next pre-registered exception handler with
    an error object that contains the given \a message. The error object
    will point to the location of the top-most context on the JavaScript
    caller stack; specifically, it will have properties \c lineNumber,
    \c fileName and \c stack. These properties are described in
    \l{Script Exceptions}.

    In the following example a C++ method in \e FileAccess.cpp throws an error
    in \e qmlFile.qml at the position where \c readFileAsText() is called:

    \code
    // qmlFile.qml
    function someFunction() {
      ...
      var text = FileAccess.readFileAsText("/path/to/file.txt");
    }
    \endcode

    \code
    // FileAccess.cpp
    // Assuming that FileAccess is a QObject-derived class that has been
    // registered as a singleton type and provides an invokable method
    // readFileAsText()

    QJSValue FileAccess::readFileAsText(const QString & filePath) {
      QFile file(filePath);

      if (!file.open(QIODevice::ReadOnly)) {
        jsEngine->throwError(file.errorString());
        return QString();
      }

      ...
      return content;
    }
    \endcode

    It is also possible to catch the thrown error in JavaScript:
    \code
    // qmlFile.qml
    function someFunction() {
      ...
      var text;
      try {
        text = FileAccess.readFileAsText("/path/to/file.txt");
      } catch (error) {
        console.warn("In " + error.fileName + ":" + "error.lineNumber" +
                     ": " + error.message);
      }
    }
    \endcode

    If you need a more specific run-time error to describe an exception, you can use the
    \l {QJSEngine::}{throwError(QJSValue::ErrorType errorType, const QString &message)}
    overload.

    \since Qt 5.12
    \sa {Script Exceptions}
*/
void QJSEngine::throwError(const QString &message)
{
    m_v4Engine->throwError(message);
}

/*!
    \overload throwError()

    Throws a run-time error (exception) with the given \a errorType and
    \a message.

    \code
    // Assuming that DataEntry is a QObject-derived class that has been
    // registered as a singleton type and provides an invokable method
    // setAge().

    void DataEntry::setAge(int age) {
      if (age < 0 || age > 200) {
        jsEngine->throwError(QJSValue::RangeError,
                             "Age must be between 0 and 200");
      }
      ...
    }
    \endcode

    \since Qt 5.12
    \sa {Script Exceptions}, newErrorObject()
*/
void QJSEngine::throwError(QJSValue::ErrorType errorType, const QString &message)
{
    QV4::Scope scope(m_v4Engine);
    QJSValue error = newErrorObject(errorType, message);
    QV4::ScopedObject e(scope, QJSValuePrivate::asReturnedValue(&error));
    if (!e)
        return;
    m_v4Engine->throwError(e);
}

/*!
    \overload throwError()

    Throws a pre-constructed run-time \a error (exception). This way you can
    use \l newErrorObject() to create the error and customize it as necessary.

    \since 6.1
    \sa {Script Exceptions}, newErrorObject()
*/
void QJSEngine::throwError(const QJSValue &error)
{
    m_v4Engine->throwError(QJSValuePrivate::asReturnedValue(&error));
}

/*!
 * Returns \c true if the last JavaScript execution resulted in an exception or
 * if throwError() was called. Otherwise returns \c false. Mind that evaluate()
 * catches any exceptions thrown in the evaluated code.
 *
 * \since Qt 6.1
 */
bool QJSEngine::hasError() const
{
    return m_v4Engine->hasException;
}

/*!
 * If an exception is currently pending, catches it and returns it as a
 * QJSValue. Otherwise returns undefined as QJSValue. After calling this method
 * hasError() returns \c false.
 *
 * \since Qt 6.1
 */
QJSValue QJSEngine::catchError()
{
    if (m_v4Engine->hasException)
        return QJSValuePrivate::fromReturnedValue(m_v4Engine->catchException());
    else
        return QJSValue();
}

/*!
  \property QJSEngine::uiLanguage
  \brief the language to be used for translating user interface strings
  \since 5.15

  This property holds the name of the language to be used for user interface
  string translations. It is exposed for reading and writing as \c{Qt.uiLanguage} when
  the QJSEngine::TranslationExtension is installed on the engine. It is always exposed
  in instances of QQmlEngine.

  You can set the value freely and use it in bindings. It is recommended to set it
  after installing translators in your application. By convention, an empty string
  means no translation from the language used in the source code is intended to occur.
*/
void QJSEngine::setUiLanguage(const QString &language) {
    Q_D(QJSEngine);
    d->uiLanguage = language; // property takes care of signal emission if necessary
}

QString QJSEngine::uiLanguage() const
{
    Q_D(const QJSEngine);
    return d->uiLanguage;
}

QJSEnginePrivate *QJSEnginePrivate::get(QV4::ExecutionEngine *e)
{
    return e->jsEngine()->d_func();
}

QJSEnginePrivate::~QJSEnginePrivate()
{
    QQmlMetaType::freeUnusedTypesAndCaches();
}

void QJSEnginePrivate::addToDebugServer(QJSEngine *q)
{
    if (QCoreApplication::instance()->thread() != q->thread())
        return;

    QQmlDebugConnector *server = QQmlDebugConnector::instance();
    if (!server || server->hasEngine(q))
        return;

    server->open();
    server->addEngine(q);
}

void QJSEnginePrivate::removeFromDebugServer(QJSEngine *q)
{
    QQmlDebugConnector *server = QQmlDebugConnector::instance();
    if (server && server->hasEngine(q))
        server->removeEngine(q);
}

/*!
   \since 5.5
   \relates QJSEngine

   Returns the QJSEngine associated with \a object, if any.

   This function is useful if you have exposed a QObject to the JavaScript environment
   and later in your program would like to regain access. It does not require you to
   keep the wrapper around that was returned from QJSEngine::newQObject().
 */
QJSEngine *qjsEngine(const QObject *object)
{
    QQmlData *data = QQmlData::get(object);
    if (!data || data->jsWrapper.isNullOrUndefined())
        return nullptr;
    return data->jsWrapper.engine()->jsEngine();
}


/*!
  \enum QJSEngine::ObjectOwnership

  ObjectOwnership controls whether or not the JavaScript memory manager automatically destroys the
  QObject when the corresponding JavaScript object is garbage collected by the
  engine. The two ownership options are:

  \value CppOwnership The object is owned by C++ code and the JavaScript memory manager will never
  delete it. The JavaScript destroy() method cannot be used on these objects. This
  option is similar to QScriptEngine::QtOwnership.

  \value JavaScriptOwnership The object is owned by JavaScript. When the object
  is returned to the JavaScript memory manager as the return value of a method call, the JavaScript
  memory manager will track it and delete it if there are no remaining JavaScript references to it
  and it has no QObject::parent(). An object tracked by one QJSEngine will be deleted during that
  QJSEngine's destructor. Thus, JavaScript references between objects with JavaScriptOwnership from
  two different engines will not be valid if one of these engines is deleted. This option is similar
  to QScriptEngine::ScriptOwnership.

  Generally an application doesn't need to set an object's ownership explicitly. The JavaScript
  memory manager uses a heuristic to set the default ownership. By default, an object that is
  created by the JavaScript memory manager has JavaScriptOwnership. The exception to this are the
  root objects created by calling QQmlComponent::create() or QQmlComponent::beginCreate(), which
  have CppOwnership by default. The ownership of these root-level objects is considered to have been
  transferred to the C++ caller.

  Objects not-created by the JavaScript memory manager have CppOwnership by default. The exception
  to this are objects returned from C++ method calls; their ownership will be set to
  JavaScriptOwnership. This applies only to explicit invocations of Q_INVOKABLE methods or slots,
  but not to property getter invocations.

  Calling setObjectOwnership() overrides the default ownership.

  \sa {Data Ownership}
*/

/*!
  Sets the \a ownership of \a object.

  An object with \c JavaScriptOwnership is not garbage collected as long
  as it still has a parent, even if there are no references to it.

  \sa QJSEngine::ObjectOwnership
*/
void QJSEngine::setObjectOwnership(QObject *object, ObjectOwnership ownership)
{
    if (!object)
        return;

    QQmlData *ddata = QQmlData::get(object, true);
    if (!ddata)
        return;

    ddata->indestructible = (ownership == CppOwnership)?true:false;
    ddata->explicitIndestructibleSet = true;
}

/*!
  Returns the ownership of \a object.

  \sa QJSEngine::ObjectOwnership
*/
QJSEngine::ObjectOwnership QJSEngine::objectOwnership(QObject *object)
{
    if (!object)
        return CppOwnership;

    QQmlData *ddata = QQmlData::get(object, false);
    if (!ddata)
        return CppOwnership;
    else
        return ddata->indestructible?CppOwnership:JavaScriptOwnership;
}

QT_END_NAMESPACE

#include "moc_qjsengine.cpp"
