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

#include "qquickworkerscript_p.h"
#include "qqmllistmodel_p.h"
#include "qqmllistmodelworkeragent_p.h"
#include <private/qqmlengine_p.h>
#include <private/qqmlexpression_p.h>

#include <QtCore/qcoreevent.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtQml/qjsengine.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qfile.h>
#include <QtCore/qdatetime.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtQml/qqmlinfo.h>
#include <QtQml/qqmlfile.h>
#include "qqmlnetworkaccessmanagerfactory.h"

#include <private/qv8engine_p.h>
#include <private/qv8worker_p.h>

QT_BEGIN_NAMESPACE

class WorkerDataEvent : public QEvent
{
public:
    enum Type { WorkerData = QEvent::User };

    WorkerDataEvent(int workerId, const QByteArray &data);
    virtual ~WorkerDataEvent();

    int workerId() const;
    QByteArray data() const;

private:
    int m_id;
    QByteArray m_data;
};

class WorkerLoadEvent : public QEvent
{
public:
    enum Type { WorkerLoad = WorkerDataEvent::WorkerData + 1 };

    WorkerLoadEvent(int workerId, const QUrl &url);

    int workerId() const;
    QUrl url() const;

private:
    int m_id;
    QUrl m_url;
};

class WorkerRemoveEvent : public QEvent
{
public:
    enum Type { WorkerRemove = WorkerLoadEvent::WorkerLoad + 1 };

    WorkerRemoveEvent(int workerId);

    int workerId() const;

private:
    int m_id;
};

class WorkerErrorEvent : public QEvent
{
public:
    enum Type { WorkerError = WorkerRemoveEvent::WorkerRemove + 1 };

    WorkerErrorEvent(const QQmlError &error);

    QQmlError error() const;

private:
    QQmlError m_error;
};

class QQuickWorkerScriptEnginePrivate : public QObject
{
    Q_OBJECT
public:
    enum WorkerEventTypes {
        WorkerDestroyEvent = QEvent::User + 100
    };

    QQuickWorkerScriptEnginePrivate(QQmlEngine *eng);

    class WorkerEngine : public QV8Engine
    {
    public:
        WorkerEngine(QQuickWorkerScriptEnginePrivate *parent);
        ~WorkerEngine();

        void init();
        virtual QNetworkAccessManager *networkAccessManager();

        QQuickWorkerScriptEnginePrivate *p;

        v8::Local<v8::Function> sendFunction(int id);
        void callOnMessage(v8::Handle<v8::Object> object, v8::Handle<v8::Value> arg);
    private:
        v8::Persistent<v8::Function> onmessage;
        v8::Persistent<v8::Function> createsend;
        QNetworkAccessManager *accessManager;
    };

    WorkerEngine *workerEngine;
    static QQuickWorkerScriptEnginePrivate *get(QV8Engine *e) {
        return static_cast<WorkerEngine *>(e)->p;
    }

    QQmlEngine *qmlengine;

    QMutex m_lock;
    QWaitCondition m_wait;

    struct WorkerScript {
        WorkerScript();
        ~WorkerScript();

        int id;
        QUrl source;
        bool initialized;
        QQuickWorkerScript *owner;
        v8::Persistent<v8::Object> object;
    };

    QHash<int, WorkerScript *> workers;
    v8::Handle<v8::Object> getWorker(WorkerScript *);

    int m_nextId;

    static v8::Handle<v8::Value> sendMessage(const v8::Arguments &args);

signals:
    void stopThread();

protected:
    virtual bool event(QEvent *);

private:
    void processMessage(int, const QByteArray &);
    void processLoad(int, const QUrl &);
    void reportScriptException(WorkerScript *, const QQmlError &error);
};

QQuickWorkerScriptEnginePrivate::WorkerEngine::WorkerEngine(QQuickWorkerScriptEnginePrivate *parent) 
: QV8Engine(0), p(parent), accessManager(0)
{
}

QQuickWorkerScriptEnginePrivate::WorkerEngine::~WorkerEngine() 
{ 
    qPersistentDispose(createsend);
    qPersistentDispose(onmessage);
    delete accessManager; 
}

void QQuickWorkerScriptEnginePrivate::WorkerEngine::init()
{
    initQmlGlobalObject();
#define CALL_ONMESSAGE_SCRIPT \
    "(function(object, message) { "\
        "var isfunction = false; "\
        "try { "\
            "isfunction = object.WorkerScript.onMessage instanceof Function; "\
        "} catch (e) {}" \
        "if (isfunction) "\
            "object.WorkerScript.onMessage(message); "\
    "})"

#define SEND_MESSAGE_CREATE_SCRIPT \
    "(function(method, engine) { "\
        "return (function(id) { "\
            "return (function(message) { "\
                "if (arguments.length) method(engine, id, message); "\
            "}); "\
        "}); "\
    "})"

    v8::HandleScope handle_scope;
    v8::Context::Scope scope(context());

    {
    v8::Local<v8::Script> onmessagescript = v8::Script::New(v8::String::New(CALL_ONMESSAGE_SCRIPT));
    onmessage = qPersistentNew<v8::Function>(v8::Handle<v8::Function>::Cast(onmessagescript->Run()));
    }
    {
    v8::Local<v8::Script> createsendscript = v8::Script::New(v8::String::New(SEND_MESSAGE_CREATE_SCRIPT));
    v8::Local<v8::Function> createsendconstructor = v8::Local<v8::Function>::Cast(createsendscript->Run());

    v8::Handle<v8::Value> args[] = { 
        V8FUNCTION(QQuickWorkerScriptEnginePrivate::sendMessage, this)
    };
    v8::Local<v8::Value> createsendvalue = createsendconstructor->Call(global(), 1, args);
    
    createsend = qPersistentNew<v8::Function>(v8::Handle<v8::Function>::Cast(createsendvalue));
    }
}

// Requires handle and context scope
v8::Local<v8::Function> QQuickWorkerScriptEnginePrivate::WorkerEngine::sendFunction(int id)
{
    v8::Handle<v8::Value> args[] = { v8::Integer::New(id) };
    return v8::Local<v8::Function>::Cast(createsend->Call(global(), 1, args));
}

// Requires handle and context scope
void QQuickWorkerScriptEnginePrivate::WorkerEngine::callOnMessage(v8::Handle<v8::Object> object, 
                                                                        v8::Handle<v8::Value> arg)
{
    v8::Handle<v8::Value> args[] = { object, arg };
    onmessage->Call(global(), 2, args);
}

QNetworkAccessManager *QQuickWorkerScriptEnginePrivate::WorkerEngine::networkAccessManager() 
{
    if (!accessManager) {
        if (p->qmlengine && p->qmlengine->networkAccessManagerFactory()) {
            accessManager = p->qmlengine->networkAccessManagerFactory()->create(p);
        } else {
            accessManager = new QNetworkAccessManager(p);
        }
    }
    return accessManager;
}

QQuickWorkerScriptEnginePrivate::QQuickWorkerScriptEnginePrivate(QQmlEngine *engine)
: workerEngine(0), qmlengine(engine), m_nextId(0)
{
}

v8::Handle<v8::Value> QQuickWorkerScriptEnginePrivate::sendMessage(const v8::Arguments &args)
{
    WorkerEngine *engine = (WorkerEngine*)V8ENGINE();

    int id = args[1]->Int32Value();

    QByteArray data = QV8Worker::serialize(args[2], engine);

    QMutexLocker locker(&engine->p->m_lock);
    WorkerScript *script = engine->p->workers.value(id);
    if (!script)
        return v8::Undefined();

    if (script->owner)
        QCoreApplication::postEvent(script->owner, new WorkerDataEvent(0, data));

    return v8::Undefined();
}

// Requires handle scope and context scope
v8::Handle<v8::Object> QQuickWorkerScriptEnginePrivate::getWorker(WorkerScript *script)
{
    if (!script->initialized) {
        script->initialized = true;

        script->object = qPersistentNew<v8::Object>(workerEngine->contextWrapper()->urlScope(script->source));

        workerEngine->contextWrapper()->setReadOnly(script->object, false);

        v8::Local<v8::Object> api = v8::Object::New();
        api->Set(v8::String::New("sendMessage"), workerEngine->sendFunction(script->id));

        script->object->Set(v8::String::New("WorkerScript"), api);

        workerEngine->contextWrapper()->setReadOnly(script->object, true);
    }

    return script->object;
}

bool QQuickWorkerScriptEnginePrivate::event(QEvent *event)
{
    if (event->type() == (QEvent::Type)WorkerDataEvent::WorkerData) {
        WorkerDataEvent *workerEvent = static_cast<WorkerDataEvent *>(event);
        processMessage(workerEvent->workerId(), workerEvent->data());
        return true;
    } else if (event->type() == (QEvent::Type)WorkerLoadEvent::WorkerLoad) {
        WorkerLoadEvent *workerEvent = static_cast<WorkerLoadEvent *>(event);
        processLoad(workerEvent->workerId(), workerEvent->url());
        return true;
    } else if (event->type() == (QEvent::Type)WorkerDestroyEvent) {
        emit stopThread();
        return true;
    } else if (event->type() == (QEvent::Type)WorkerRemoveEvent::WorkerRemove) {
        WorkerRemoveEvent *workerEvent = static_cast<WorkerRemoveEvent *>(event);
        workers.remove(workerEvent->workerId());
        return true;
    } else {
        return QObject::event(event);
    }
}

void QQuickWorkerScriptEnginePrivate::processMessage(int id, const QByteArray &data)
{
    WorkerScript *script = workers.value(id);
    if (!script)
        return;

    v8::HandleScope handle_scope;
    v8::Context::Scope scope(workerEngine->context());

    v8::Handle<v8::Value> value = QV8Worker::deserialize(data, workerEngine);

    v8::TryCatch tc;
    workerEngine->callOnMessage(script->object, value);

    if (tc.HasCaught()) {
        QQmlError error;
        QQmlExpressionPrivate::exceptionToError(tc.Message(), error);
        reportScriptException(script, error);
    }
}

void QQuickWorkerScriptEnginePrivate::processLoad(int id, const QUrl &url)
{
    if (url.isRelative())
        return;

    QString fileName = QQmlFile::urlToLocalFileOrQrc(url);

    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray data = f.readAll();
        QString sourceCode = QString::fromUtf8(data);
        QQmlScript::Parser::extractPragmas(sourceCode);

        v8::HandleScope handle_scope;
        v8::Context::Scope scope(workerEngine->context());

        WorkerScript *script = workers.value(id);
        if (!script)
            return;
        script->source = url;
        v8::Handle<v8::Object> activation = getWorker(script);
        if (activation.IsEmpty())
            return;

        // XXX ???
        // workerEngine->baseUrl = url;

        v8::TryCatch tc;
        v8::Local<v8::Script> program = workerEngine->qmlModeCompile(sourceCode, url.toString());

        if (!tc.HasCaught()) 
            program->Run(activation);
        
        if (tc.HasCaught()) {
            QQmlError error;
            QQmlExpressionPrivate::exceptionToError(tc.Message(), error);
            reportScriptException(script, error);
        }
    } else {
        qWarning().nospace() << "WorkerScript: Cannot find source file " << url.toString();
    }
}

void QQuickWorkerScriptEnginePrivate::reportScriptException(WorkerScript *script, 
                                                                  const QQmlError &error)
{
    QQuickWorkerScriptEnginePrivate *p = QQuickWorkerScriptEnginePrivate::get(workerEngine);

    QMutexLocker locker(&p->m_lock);
    if (script->owner)
        QCoreApplication::postEvent(script->owner, new WorkerErrorEvent(error));
}

WorkerDataEvent::WorkerDataEvent(int workerId, const QByteArray &data)
: QEvent((QEvent::Type)WorkerData), m_id(workerId), m_data(data)
{
}

WorkerDataEvent::~WorkerDataEvent()
{
}

int WorkerDataEvent::workerId() const
{
    return m_id;
}

QByteArray WorkerDataEvent::data() const
{
    return m_data;
}

WorkerLoadEvent::WorkerLoadEvent(int workerId, const QUrl &url)
: QEvent((QEvent::Type)WorkerLoad), m_id(workerId), m_url(url)
{
}

int WorkerLoadEvent::workerId() const
{
    return m_id;
}

QUrl WorkerLoadEvent::url() const
{
    return m_url;
}

WorkerRemoveEvent::WorkerRemoveEvent(int workerId)
: QEvent((QEvent::Type)WorkerRemove), m_id(workerId)
{
}

int WorkerRemoveEvent::workerId() const
{
    return m_id;
}

WorkerErrorEvent::WorkerErrorEvent(const QQmlError &error)
: QEvent((QEvent::Type)WorkerError), m_error(error)
{
}

QQmlError WorkerErrorEvent::error() const
{
    return m_error;
}

QQuickWorkerScriptEngine::QQuickWorkerScriptEngine(QQmlEngine *parent)
: QThread(parent), d(new QQuickWorkerScriptEnginePrivate(parent))
{
    d->m_lock.lock();
    connect(d, SIGNAL(stopThread()), this, SLOT(quit()), Qt::DirectConnection);
    start(QThread::LowestPriority);
    d->m_wait.wait(&d->m_lock);
    d->moveToThread(this);
    d->m_lock.unlock();
}

QQuickWorkerScriptEngine::~QQuickWorkerScriptEngine()
{
    d->m_lock.lock();
    QCoreApplication::postEvent(d, new QEvent((QEvent::Type)QQuickWorkerScriptEnginePrivate::WorkerDestroyEvent));
    d->m_lock.unlock();

    //We have to force to cleanup the main thread's event queue here
    //to make sure the main GUI release all pending locks/wait conditions which
    //some worker script/agent are waiting for (QQmlListModelWorkerAgent::sync() for example).
    while (!isFinished()) {
        // We can't simply wait here, because the worker thread will not terminate
        // until the main thread processes the last data event it generates
        QCoreApplication::processEvents();
        yieldCurrentThread();
    }

    d->deleteLater();
}

QQuickWorkerScriptEnginePrivate::WorkerScript::WorkerScript()
: id(-1), initialized(false), owner(0)
{
}

QQuickWorkerScriptEnginePrivate::WorkerScript::~WorkerScript()
{
    qPersistentDispose(object);
}

int QQuickWorkerScriptEngine::registerWorkerScript(QQuickWorkerScript *owner)
{
    typedef QQuickWorkerScriptEnginePrivate::WorkerScript WorkerScript;
    WorkerScript *script = new WorkerScript;

    script->id = d->m_nextId++;
    script->owner = owner;

    d->m_lock.lock();
    d->workers.insert(script->id, script);
    d->m_lock.unlock();

    return script->id;
}

void QQuickWorkerScriptEngine::removeWorkerScript(int id)
{
    QQuickWorkerScriptEnginePrivate::WorkerScript* script = d->workers.value(id);
    if (script) {
        script->owner = 0;
        QCoreApplication::postEvent(d, new WorkerRemoveEvent(id));
    }
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
    d->m_lock.lock();

    d->workerEngine = new QQuickWorkerScriptEnginePrivate::WorkerEngine(d);
    d->workerEngine->init();

    d->m_wait.wakeAll();

    d->m_lock.unlock();

    exec();

    qDeleteAll(d->workers);
    d->workers.clear();

    delete d->workerEngine; d->workerEngine = 0;
}


/*!
    \qmltype WorkerScript
    \instantiates QQuickWorkerScript
    \ingroup qtquick-threading
    \inqmlmodule QtQuick 2
    \brief Enables the use of threads in a Qt Quick application

    Use WorkerScript to run operations in a new thread.
    This is useful for running operations in the background so
    that the main GUI thread is not blocked.

    Messages can be passed between the new thread and the parent thread
    using \l sendMessage() and the \l {WorkerScript::onMessage}{onMessage()} handler.

    An example:

    \snippet qml/workerscript/workerscript.qml 0

    The above worker script specifies a JavaScript file, "script.js", that handles
    the operations to be performed in the new thread. Here is \c script.js:

    \quotefile qml/workerscript/script.js

    When the user clicks anywhere within the rectangle, \c sendMessage() is
    called, triggering the \tt WorkerScript.onMessage() handler in
    \tt script.js. This in turn sends a reply message that is then received
    by the \tt onMessage() handler of \tt myWorker.


    \section3 Restrictions

    Since the \c WorkerScript.onMessage() function is run in a separate thread, the
    JavaScript file is evaluated in a context separate from the main QML engine. This means
    that unlike an ordinary JavaScript file that is imported into QML, the \c script.js
    in the above example cannot access the properties, methods or other attributes
    of the QML item, nor can it access any context properties set on the QML object
    through QQmlContext.

    Additionally, there are restrictions on the types of values that can be passed to and
    from the worker script. See the sendMessage() documentation for details.

    Worker script can not use \l {qtqml-javascript-imports.html}{.import} syntax.

    \sa {declarative/threading/workerscript}{WorkerScript example},
        {declarative/threading/threadedlistmodel}{Threaded ListModel example}
*/
QQuickWorkerScript::QQuickWorkerScript(QObject *parent)
: QObject(parent), m_engine(0), m_scriptId(-1), m_componentComplete(true)
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

    if (engine())
        m_engine->executeUrl(m_scriptId, m_source);

    emit sourceChanged();
}

/*!
    \qmlmethod WorkerScript::sendMessage(jsobject message)

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
void QQuickWorkerScript::sendMessage(QQmlV8Function *args)
{
    if (!engine()) {
        qWarning("QQuickWorkerScript: Attempt to send message before WorkerScript establishment");
        return;
    }

    v8::Handle<v8::Value> argument = v8::Undefined();
    if (args->Length() != 0) 
        argument = (*args)[0];

    m_engine->sendMessage(m_scriptId, QV8Worker::serialize(argument, args->engine()));
}

void QQuickWorkerScript::classBegin()
{
    m_componentComplete = false;
}

QQuickWorkerScriptEngine *QQuickWorkerScript::engine()
{
    if (m_engine) return m_engine;
    if (m_componentComplete) {
        QQmlEngine *engine = qmlEngine(this);
        if (!engine) {
            qWarning("QQuickWorkerScript: engine() called without qmlEngine() set");
            return 0;
        }

        m_engine = QQmlEnginePrivate::get(engine)->getWorkerScriptEngine();
        m_scriptId = m_engine->registerWorkerScript(this);

        if (m_source.isValid())
            m_engine->executeUrl(m_scriptId, m_source);

        return m_engine;
    }
    return 0;
}

void QQuickWorkerScript::componentComplete()
{
    m_componentComplete = true;
    engine(); // Get it started now.
}

/*!
    \qmlsignal WorkerScript::onMessage(jsobject msg)

    This handler is called when a message \a msg is received from a worker
    script in another thread through a call to sendMessage().
*/

bool QQuickWorkerScript::event(QEvent *event)
{
    if (event->type() == (QEvent::Type)WorkerDataEvent::WorkerData) {
        QQmlEngine *engine = qmlEngine(this);
        if (engine) {
            WorkerDataEvent *workerEvent = static_cast<WorkerDataEvent *>(event);
            QV8Engine *v8engine = QQmlEnginePrivate::get(engine)->v8engine();
            v8::HandleScope handle_scope;
            v8::Context::Scope scope(v8engine->context());
            v8::Handle<v8::Value> value = QV8Worker::deserialize(workerEvent->data(), v8engine);
            emit message(QQmlV8Handle::fromHandle(value));
        }
        return true;
    } else if (event->type() == (QEvent::Type)WorkerErrorEvent::WorkerError) {
        WorkerErrorEvent *workerEvent = static_cast<WorkerErrorEvent *>(event);
        QQmlEnginePrivate::warning(qmlEngine(this), workerEvent->error());
        return true;
    } else {
        return QObject::event(event);
    }
}

QT_END_NAMESPACE

#include <qquickworkerscript.moc>

