// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include "qtqmlworkerscriptglobal_p.h"
#include "qquickworkerscript_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlexpression_p.h>
#include <private/qjsvalue_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qqmlscriptdata_p.h>

#include <QtCore/qcoreevent.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtQml/qjsengine.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qfile.h>
#include <QtCore/qdatetime.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlfile.h>
#if QT_CONFIG(qml_network)
#include <QtNetwork/qnetworkaccessmanager.h>
#include "qqmlnetworkaccessmanagerfactory.h"
#endif

#include <private/qv4serialize_p.h>

#include <private/qv4value_p.h>
#include <private/qv4functionobject_p.h>
#include <private/qv4script_p.h>
#include <private/qv4scopedvalue_p.h>
#include <private/qv4jscall_p.h>

QT_BEGIN_NAMESPACE

enum class WorkerEventType
{
    Data = QEvent::User,
    Load,
    Remove,
    Error,
    Ready,
    Destroy = QEvent::User + 100,
};

class WorkerIdEvent : public QEvent
{
public:
    WorkerIdEvent(int workerId, WorkerEventType type)
        : QEvent(QEvent::Type(type)), m_workerId(workerId)
    {}

    int workerId() const { return m_workerId; }

private:
    int m_workerId = -1;
};

class WorkerDataEvent : public WorkerIdEvent
{
public:
    WorkerDataEvent(int workerId, const QByteArray &data)
        : WorkerIdEvent(workerId, WorkerEventType::Data), m_data(data)
    {}

    QByteArray data() const { return m_data; }

private:
    QByteArray m_data;
};

class WorkerLoadEvent : public WorkerIdEvent
{
public:
    WorkerLoadEvent(int workerId, const QUrl &url)
        : WorkerIdEvent(workerId, WorkerEventType::Load), m_url(url)
    {}

    QUrl url() const { return m_url; }

private:
    QUrl m_url;
};

class WorkerRemoveEvent : public WorkerIdEvent
{
public:
    WorkerRemoveEvent(int workerId) : WorkerIdEvent(workerId, WorkerEventType::Remove) {}
};

class WorkerErrorEvent : public QEvent
{
public:
    WorkerErrorEvent(const QQmlError &error)
        : QEvent(QEvent::Type(WorkerEventType::Error)), m_error(error)
    {}

    QQmlError error() const { return m_error; }

private:
    QQmlError m_error;
};

class WorkerReadyEvent : public QEvent
{
public:
    WorkerReadyEvent() : QEvent(QEvent::Type(WorkerEventType::Ready)) {}
};

class WorkerDestroyEvent : public QEvent
{
public:
    WorkerDestroyEvent() : QEvent(QEvent::Type(WorkerEventType::Destroy)) {}
};

struct WorkerScript
    : public QV4::ExecutionEngine::Deletable
    , public QQmlNotifyingBlob::Callback
#if QT_CONFIG(qml_network)
    , public QQmlNetworkAccessManagerFactory
#endif
{
    WorkerScript(QV4::ExecutionEngine *engine);
    ~WorkerScript() = default;

    QV4::ExecutionEngine *engine = nullptr;
    QQuickWorkerScriptEnginePrivate *p = nullptr;
    QQuickWorkerScript *owner = nullptr;

#if QT_CONFIG(qml_network)
    QNetworkAccessManager *create(QObject *parent) final;
#endif

    void ready(QQmlNotifyingBlob *blob) final;
};

V4_DEFINE_EXTENSION(WorkerScript, workerScriptExtension);

class QQuickWorkerScriptEnginePrivate : public QObject
{
    Q_OBJECT
public:
    QQuickWorkerScriptEnginePrivate(QQmlTypeLoader *typeLoader)
        : m_typeLoader(typeLoader), m_nextId(0)
    {
    }

    QQmlTypeLoader *m_typeLoader = nullptr;

    QMutex m_lock;
    QWaitCondition m_wait;

    // ExecutionEngines are owned by the worker script and created and deleted
    // in the worker thread. QQuickWorkerScript instances, however, belong to
    // the main thread. They are only inserted as place holders when creating
    // the worker script.
    QHash<int, QBiPointer<QV4::ExecutionEngine, QQuickWorkerScript>> workers;

    int m_nextId = 0;

    static QV4::ReturnedValue method_sendMessage(const QV4::FunctionObject *, const QV4::Value *thisObject, const QV4::Value *argv, int argc);
    QV4::ExecutionEngine *workerEngine(int id);

    void reportScriptReady(WorkerScript *);
    void reportScriptException(WorkerScript *, const QQmlError &error);

signals:
    void stopThread();

protected:
    bool event(QEvent *) override;

private:
    void processMessage(int id, const QByteArray &data);
    void processLoad(int id, const QUrl &url);
};

QV4::ReturnedValue QQuickWorkerScriptEnginePrivate::method_sendMessage(const QV4::FunctionObject *b,
                                                                       const QV4::Value *, const QV4::Value *argv, int argc)
{
    QV4::Scope scope(b);
    const WorkerScript *script = workerScriptExtension(scope.engine);
    Q_ASSERT(script);

    QV4::ScopedValue v(scope, argc > 0 ? argv[0] : QV4::Value::undefinedValue());
    QByteArray data = QV4::Serialize::serialize(v, scope.engine);

    QMutexLocker locker(&script->p->m_lock);
    if (script->owner)
        QCoreApplication::postEvent(script->owner, new WorkerDataEvent(0, data));

    return QV4::Encode::undefined();
}

bool QQuickWorkerScriptEnginePrivate::event(QEvent *event)
{
    switch (WorkerEventType(event->type())) {
    case WorkerEventType::Data: {
        WorkerDataEvent *workerEvent = static_cast<WorkerDataEvent *>(event);
        processMessage(workerEvent->workerId(), workerEvent->data());
        return true;
    }
    case WorkerEventType::Load: {
        WorkerLoadEvent *workerEvent = static_cast<WorkerLoadEvent *>(event);
        processLoad(workerEvent->workerId(), workerEvent->url());
        return true;
    }
    case WorkerEventType::Remove: {
        QMutexLocker locker(&m_lock);
        WorkerRemoveEvent *workerEvent = static_cast<WorkerRemoveEvent *>(event);
        auto itr = workers.constFind(workerEvent->workerId());
        if (itr != workers.cend()) {
            if (itr->isT1())
                delete itr->asT1();
            workers.erase(itr);
        }
        return true;
    }
    case WorkerEventType::Destroy:
        emit stopThread();
        return true;
    default:
        break;
    }

    return QObject::event(event);
}

QV4::ExecutionEngine *QQuickWorkerScriptEnginePrivate::workerEngine(int id)
{
    QMutexLocker locker(&m_lock);

    const auto it = workers.find(id);
    if (it == workers.end())
        return nullptr;
    if (it->isT1())
        return it->asT1();

    QQuickWorkerScript *owner = it->asT2();
    auto *engine = new QV4::ExecutionEngine;
    WorkerScript *script = workerScriptExtension(engine);
    script->owner = owner;
    script->p = this;
    *it = engine;
    return engine;
}

void QQuickWorkerScriptEnginePrivate::processMessage(int id, const QByteArray &data)
{
    QV4::ExecutionEngine *engine = workerEngine(id);
    if (!engine)
        return;

    QV4::Scope scope(engine);
    QV4::ScopedString v(scope, engine->newString(QStringLiteral("WorkerScript")));
    QV4::ScopedObject worker(scope, engine->globalObject->get(v));
    QV4::ScopedFunctionObject onmessage(scope);
    if (worker)
        onmessage = worker->get((v = engine->newString(QStringLiteral("onMessage"))));

    if (!onmessage)
        return;

    QV4::ScopedValue value(scope, QV4::Serialize::deserialize(data, engine));

    QV4::JSCallArguments jsCallData(scope, 1);
    *jsCallData.thisObject = engine->global();
    jsCallData.args[0] = value;
    onmessage->call(jsCallData);
    if (scope.hasException()) {
        QQmlError error = scope.engine->catchExceptionAsQmlError();
        WorkerScript *script = workerScriptExtension(engine);
        reportScriptException(script, error);
    }
}

void QQuickWorkerScriptEnginePrivate::processLoad(int id, const QUrl &url)
{
    if (url.isRelative())
        return;

    QV4::ExecutionEngine *engine = workerEngine(id);
    if (!engine)
        return;

    WorkerScript *script = workerScriptExtension(engine);
    QQmlRefPointer<QQmlScriptBlob> scriptBlob = engine->typeLoader()->getScript(url);

    if (scriptBlob->isCompleteOrError())
        script->ready(scriptBlob.data());
    else
        scriptBlob->registerCallback(script);
}

void QQuickWorkerScriptEnginePrivate::reportScriptReady(WorkerScript *script)
{
    QMutexLocker locker(&script->p->m_lock);
    if (script->owner)
        QCoreApplication::postEvent(script->owner, new WorkerReadyEvent);
}

void QQuickWorkerScriptEnginePrivate::reportScriptException(WorkerScript *script,
                                                                  const QQmlError &error)
{
    QMutexLocker locker(&script->p->m_lock);
    if (script->owner)
        QCoreApplication::postEvent(script->owner, new WorkerErrorEvent(error));
}

QQuickWorkerScriptEngine::QQuickWorkerScriptEngine(QQmlEngine *parent)
    : QThread(parent)
    , d(new QQuickWorkerScriptEnginePrivate(QQmlTypeLoader::get(parent)))
{
    connect(d, SIGNAL(stopThread()), this, SLOT(quit()), Qt::DirectConnection);
    QMutexLocker locker(&d->m_lock);
    start(QThread::LowestPriority);
    d->m_wait.wait(&d->m_lock);
    d->moveToThread(this);
}

QQuickWorkerScriptEngine::~QQuickWorkerScriptEngine()
{
    QCoreApplication::postEvent(d, new WorkerDestroyEvent);

    //We have to force to cleanup the main thread's event queue here
    //to make sure the main GUI release all pending locks/wait conditions which
    //some worker script/agent are waiting for (QQmlListModelWorkerAgent::sync() for example).
    while (!isFinished()) {
        // We can't simply wait here, because the worker thread will not terminate
        // until the main thread processes the last data event it generates
        QCoreApplication::processEvents();
        yieldCurrentThread();
    }

    delete d;
}


WorkerScript::WorkerScript(QV4::ExecutionEngine *engine) : engine(engine)
{
    engine->initQmlGlobalObject();

    QV4::Scope scope(engine);
    QV4::ScopedObject api(scope, engine->newObject());
    QV4::ScopedString sendMessageName(scope, engine->newString(QStringLiteral("sendMessage")));
    QV4::ScopedFunctionObject sendMessage(
                scope, QV4::FunctionObject::createBuiltinFunction(
                    engine, sendMessageName,
                    QQuickWorkerScriptEnginePrivate::method_sendMessage, 1));
    api->put(sendMessageName, sendMessage);
    QV4::ScopedString workerScriptName(scope, engine->newString(QStringLiteral("WorkerScript")));
    engine->globalObject->put(workerScriptName, api);

#if QT_CONFIG(qml_network)
    engine->typeLoader()->setNetworkAccessManagerFactory(this);
#endif // qml_network
}

void WorkerScript::ready(QQmlNotifyingBlob *scriptBlob)
{
    if (scriptBlob->isComplete()) {
        const auto cu = engine->executableCompilationUnit(
                static_cast<QQmlScriptBlob *>(scriptBlob)->scriptData()->compilationUnit());
        if (cu->isESModule()) {
            if (cu->instantiate())
                cu->evaluate();
        } else {
            QV4::Function *vmFunction = cu->rootFunction();
            QScopedValueRollback<QV4::Function *> savedGlobal(engine->globalCode, vmFunction);
            vmFunction->call(engine->globalObject, nullptr, 0, engine->rootContext());
        }

        if (engine->hasException)
            p->reportScriptException(this, engine->catchExceptionAsQmlError());

    } else {
        Q_ASSERT(scriptBlob->isError());

        const QList<QQmlError> errors = scriptBlob->errors();
        for (const QQmlError &error : errors) {
            // Funnel this through SyntaxError to get the right output format.
            engine->throwSyntaxError(
                    error.description(), error.url().toString(), error.line(), error.column());
            p->reportScriptException(this, engine->catchExceptionAsQmlError());
        }
    }

    p->reportScriptReady(this);
}

#if QT_CONFIG(qml_network)
QNetworkAccessManager *WorkerScript::create(QObject *parent)
{
    return p->m_typeLoader->createNetworkAccessManager(parent);
}
#endif

int QQuickWorkerScriptEngine::registerWorkerScript(QQuickWorkerScript *owner)
{
    const int id = d->m_nextId++;

    QMutexLocker locker(&d->m_lock);
    d->workers.insert(id, owner);

    return id;
}

void QQuickWorkerScriptEngine::removeWorkerScript(int id)
{
    {
        QMutexLocker locker(&d->m_lock);
        const auto it = d->workers.find(id);
        if (it == d->workers.end())
            return;

        if (it->isT1())
            workerScriptExtension(it->asT1())->owner = nullptr;
        else
            *it = static_cast<QQuickWorkerScript *>(nullptr);
    }

    QCoreApplication::postEvent(d, new WorkerRemoveEvent(id));
}

void QQuickWorkerScriptEngine::executeUrl(int id, const QUrl &url)
{
    QCoreApplication::postEvent(d, new WorkerLoadEvent(id, url));
}

void QQuickWorkerScriptEngine::sendMessage(int id, const QByteArray &data)
{
    QCoreApplication::postEvent(d, new WorkerDataEvent(id, data));
}

void QQuickWorkerScriptEngine::run()
{
    {
        QMutexLocker locker(&d->m_lock);
        d->m_wait.wakeAll();
    }

    exec();

    QMutexLocker locker(&d->m_lock);
    for (auto it = d->workers.begin(), end = d->workers.end(); it != end; ++it) {
        if (it->isT1())
            delete it->asT1();
    }

    d->workers.clear();
}


/*!
    \qmltype WorkerScript
    \nativetype QQuickWorkerScript
    \ingroup qtquick-threading
    \inqmlmodule QtQml.WorkerScript
    \brief Enables the use of threads in a Qt Quick application.

    Use WorkerScript to run operations in a new thread.
    This is useful for running operations in the background so
    that the main GUI thread is not blocked.

    Messages can be passed between the new thread and the parent thread
    using \l sendMessage() and the \c onMessage() handler.

    An example:

    \snippet qml/workerscript/workerscript.qml 0

    The above worker script specifies a JavaScript file, "script.mjs", that handles
    the operations to be performed in the new thread. Here is \c script.mjs:

    \quotefile qml/workerscript/script.mjs

    When the user clicks anywhere within the rectangle, \c sendMessage() is
    called, triggering the \tt WorkerScript.onMessage() handler in
    \tt script.mjs. This in turn sends a reply message that is then received
    by the \tt onMessage() handler of \tt myWorker.

    The example uses a script that is an ECMAScript module, because it has the ".mjs" extension.
    It can use import statements to access functionality from other modules and it is run in JavaScript
    strict mode.

    If a worker script has the extension ".js" instead, then it is considered to contain plain JavaScript
    statements and it is run in non-strict mode.

    \note Each WorkerScript element will instantiate a separate JavaScript engine to ensure perfect
    isolation and thread-safety. If the impact of that results in a memory consumption that is too
    high for your environment, then consider sharing a WorkerScript element.

    \section3 Restrictions

    Since the \c WorkerScript.onMessage() function is run in a separate thread, the
    JavaScript file is evaluated in a context separate from the main QML engine. This means
    that unlike an ordinary JavaScript file that is imported into QML, the \c script.mjs
    in the above example cannot access the properties, methods or other attributes
    of the QML item, nor can it access any context properties set on the QML object
    through QQmlContext.

    Additionally, there are restrictions on the types of values that can be passed to and
    from the worker script. See the sendMessage() documentation for details.

    Worker scripts that are plain JavaScript sources can not use \l {qtqml-javascript-imports.html}{.import} syntax.
    Scripts that are ECMAScript modules can freely use import and export statements.
*/
QQuickWorkerScript::QQuickWorkerScript(QObject *parent)
: QObject(parent)
{
}

QQuickWorkerScript::~QQuickWorkerScript()
{
    if (m_scriptId != -1) m_engine->removeWorkerScript(m_scriptId);
}

/*!
    \qmlproperty url WorkerScript::source

    This holds the url of the JavaScript file that implements the
    \tt WorkerScript.onMessage() handler for threaded operations.

    If the file name component of the url ends with ".mjs", then the script
    is parsed as an ECMAScript module and run in strict mode. Otherwise it is considered to be
    plain script.
*/
QUrl QQuickWorkerScript::source() const
{
    return m_source;
}

void QQuickWorkerScript::setSource(const QUrl &source)
{
    if (m_source == source)
        return;

    m_source = source;

    if (engine()) {
        const QQmlContext *context = qmlContext(this);
        m_engine->executeUrl(m_scriptId, context ? context->resolvedUrl(m_source) : m_source);
        if (m_ready) {
            // While the new script is loading, we can't accept any events.
            m_ready = false;
            emit readyChanged();
        }
    }

    emit sourceChanged();
}

/*!
    \qmlproperty bool WorkerScript::ready

    This holds whether the WorkerScript has been initialized and is ready
    for receiving messages via \tt WorkerScript.sendMessage().
*/
bool QQuickWorkerScript::ready() const
{
    return m_ready;
}

/*!
    \qmlmethod void WorkerScript::sendMessage(jsobject message)

    Sends the given \a message to a worker script handler in another
    thread. The other worker script handler can receive this message
    through the onMessage() handler.

    The \c message object may only contain values of the following
    types:

    \list
    \li boolean, number, string
    \li JavaScript objects and arrays
    \li ListModel objects (any other type of QObject* is not allowed)
    \endlist

    All objects and arrays are copied to the \c message. With the exception
    of ListModel objects, any modifications by the other thread to an object
    passed in \c message will not be reflected in the original object.
*/
void QQuickWorkerScript::sendMessage(QQmlV4FunctionPtr args)
{
    if (!engine()) {
        qWarning("QQuickWorkerScript: Attempt to send message before WorkerScript establishment");
        return;
    }

    QV4::Scope scope(args->v4engine());
    QV4::ScopedValue argument(scope, QV4::Value::undefinedValue());
    if (args->length() != 0)
        argument = (*args)[0];

    m_engine->sendMessage(m_scriptId, QV4::Serialize::serialize(argument, scope.engine));
}

void QQuickWorkerScript::classBegin()
{
    m_componentComplete = false;
}

QQuickWorkerScriptEngine *QQuickWorkerScript::engine()
{
    if (m_engine) return m_engine;
    if (m_componentComplete) {
        const QQmlContext *context = qmlContext(this);
        QQmlEngine *engine = context ? context->engine() : nullptr;
        if (!engine) {
            qWarning("QQuickWorkerScript: engine() called without qmlEngine() set");
            return nullptr;
        }

        QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);
        if (enginePrivate->workerScriptEngine == nullptr)
            enginePrivate->workerScriptEngine = new QQuickWorkerScriptEngine(engine);
        m_engine = qobject_cast<QQuickWorkerScriptEngine *>(enginePrivate->workerScriptEngine);
        Q_ASSERT(m_engine);
        m_scriptId = m_engine->registerWorkerScript(this);

        if (m_source.isValid())
            m_engine->executeUrl(m_scriptId, context->resolvedUrl(m_source));

        return m_engine;
    }
    return nullptr;
}

void QQuickWorkerScript::componentComplete()
{
    m_componentComplete = true;
    engine(); // Get it started now.
}

/*!
    \qmlsignal WorkerScript::message(jsobject msg)

    This signal is emitted when a message \a msg is received from a worker
    script in another thread through a call to sendMessage().
*/

bool QQuickWorkerScript::event(QEvent *event)
{
    switch (WorkerEventType(event->type())) {
    case WorkerEventType::Data:
        if (QQmlEngine *engine = qmlEngine(this)) {
            QV4::ExecutionEngine *v4 = engine->handle();
            WorkerDataEvent *workerEvent = static_cast<WorkerDataEvent *>(event);
            emit message(QJSValuePrivate::fromReturnedValue(
                    QV4::Serialize::deserialize(workerEvent->data(), v4)));
        }
        return true;
    case WorkerEventType::Error: {
        WorkerErrorEvent *workerEvent = static_cast<WorkerErrorEvent *>(event);
        QQmlEnginePrivate::warning(qmlEngine(this), workerEvent->error());
        return true;
    }
    case WorkerEventType::Ready:
        Q_ASSERT(!m_ready);
        m_ready = true;
        emit readyChanged();
        return true;
    default:
        break;
    }

    return QObject::event(event);
}

QT_END_NAMESPACE

#include <qquickworkerscript.moc>

#include "moc_qquickworkerscript_p.cpp"
