/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "private/qdeclarativeworkerscript_p.h"
#include "private/qdeclarativelistmodel_p.h"
#include "private/qdeclarativelistmodelworkeragent_p.h"
#include "private/qdeclarativeengine_p.h"
#include "private/qdeclarativeexpression_p.h"

#include <QtCore/qcoreevent.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qdebug.h>
#include <QtScript/qscriptengine.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <QtScript/qscriptvalueiterator.h>
#include <QtCore/qfile.h>
#include <QtCore/qdatetime.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#include <QtDeclarative/qdeclarativeinfo.h>
#include "qdeclarativenetworkaccessmanagerfactory.h"

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

    WorkerErrorEvent(const QDeclarativeError &error);

    QDeclarativeError error() const;

private:
    QDeclarativeError m_error;
};

class QDeclarativeWorkerScriptEnginePrivate : public QObject
{
    Q_OBJECT
public:
    enum WorkerEventTypes {
        WorkerDestroyEvent = QEvent::User + 100
    };

    QDeclarativeWorkerScriptEnginePrivate(QDeclarativeEngine *eng);

    class WorkerEngine : public QV8Engine
    {
    public:
        WorkerEngine(QDeclarativeWorkerScriptEnginePrivate *parent);
        ~WorkerEngine();

        void init();
        virtual QNetworkAccessManager *networkAccessManager();

        QDeclarativeWorkerScriptEnginePrivate *p;

        v8::Local<v8::Function> sendFunction(int id);
        void callOnMessage(v8::Handle<v8::Object> object, v8::Handle<v8::Value> arg);
    private:
        v8::Persistent<v8::Function> onmessage;
        v8::Persistent<v8::Function> createsend;
        QNetworkAccessManager *accessManager;
    };

    WorkerEngine *workerEngine;
    static QDeclarativeWorkerScriptEnginePrivate *get(QV8Engine *e) {
        return static_cast<WorkerEngine *>(e)->p;
    }

    QDeclarativeEngine *qmlengine;

    QMutex m_lock;
    QWaitCondition m_wait;

    struct WorkerScript {
        WorkerScript();
        ~WorkerScript();

        int id;
        QUrl source;
        bool initialized;
        QDeclarativeWorkerScript *owner;
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
    void reportScriptException(WorkerScript *, const QDeclarativeError &error);
};

QDeclarativeWorkerScriptEnginePrivate::WorkerEngine::WorkerEngine(QDeclarativeWorkerScriptEnginePrivate *parent) 
: p(parent), accessManager(0) 
{
}

QDeclarativeWorkerScriptEnginePrivate::WorkerEngine::~WorkerEngine() 
{ 
    createsend.Dispose(); createsend.Clear();
    onmessage.Dispose(); onmessage.Clear();
    delete accessManager; 
}

void QDeclarativeWorkerScriptEnginePrivate::WorkerEngine::init()
{
    QV8Engine::init(0);

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
    onmessage = v8::Persistent<v8::Function>::New(v8::Handle<v8::Function>::Cast(onmessagescript->Run()));
    }
    {
    v8::Local<v8::Script> createsendscript = v8::Script::New(v8::String::New(SEND_MESSAGE_CREATE_SCRIPT));
    v8::Local<v8::Function> createsendconstructor = v8::Local<v8::Function>::Cast(createsendscript->Run());

    v8::Handle<v8::Value> args[] = { 
        V8FUNCTION(QDeclarativeWorkerScriptEnginePrivate::sendMessage, this)
    };
    v8::Local<v8::Value> createsendvalue = createsendconstructor->Call(global(), 1, args);
    
    createsend = v8::Persistent<v8::Function>::New(v8::Handle<v8::Function>::Cast(createsendvalue));
    }
}

// Requires handle and context scope
v8::Local<v8::Function> QDeclarativeWorkerScriptEnginePrivate::WorkerEngine::sendFunction(int id)
{
    v8::Handle<v8::Value> args[] = { v8::Integer::New(id) };
    return v8::Local<v8::Function>::Cast(createsend->Call(global(), 1, args));
}

// Requires handle and context scope
void QDeclarativeWorkerScriptEnginePrivate::WorkerEngine::callOnMessage(v8::Handle<v8::Object> object, 
                                                                        v8::Handle<v8::Value> arg)
{
    v8::Handle<v8::Value> args[] = { object, arg };
    onmessage->Call(global(), 2, args);
}

QNetworkAccessManager *QDeclarativeWorkerScriptEnginePrivate::WorkerEngine::networkAccessManager() 
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

QDeclarativeWorkerScriptEnginePrivate::QDeclarativeWorkerScriptEnginePrivate(QDeclarativeEngine *engine)
: workerEngine(0), qmlengine(engine), m_nextId(0)
{
}

v8::Handle<v8::Value> QDeclarativeWorkerScriptEnginePrivate::sendMessage(const v8::Arguments &args)
{
    WorkerEngine *engine = (WorkerEngine*)V8ENGINE();

    int id = args[1]->Int32Value();

    QByteArray data = QV8Worker::serialize(args[2], engine);

    QMutexLocker(&engine->p->m_lock);
    WorkerScript *script = engine->p->workers.value(id);
    if (!script)
        return v8::Undefined();

    if (script->owner)
        QCoreApplication::postEvent(script->owner, new WorkerDataEvent(0, data));

    return v8::Undefined();
}

// Requires handle scope and context scope
v8::Handle<v8::Object> QDeclarativeWorkerScriptEnginePrivate::getWorker(WorkerScript *script)
{
    if (!script->initialized) {
        script->initialized = true;

        script->object = v8::Persistent<v8::Object>::New(workerEngine->contextWrapper()->urlScope(script->source));

        workerEngine->contextWrapper()->setReadOnly(script->object, false);

        v8::Local<v8::Object> api = v8::Object::New();
        api->Set(v8::String::New("sendMessage"), workerEngine->sendFunction(script->id));

        script->object->Set(v8::String::New("WorkerScript"), api);

        workerEngine->contextWrapper()->setReadOnly(script->object, true);
    }

    return script->object;
}

bool QDeclarativeWorkerScriptEnginePrivate::event(QEvent *event)
{
    // XXX must handle remove request
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
    } else {
        return QObject::event(event);
    }
}

void QDeclarativeWorkerScriptEnginePrivate::processMessage(int id, const QByteArray &data)
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
        QDeclarativeError error;
        QDeclarativeExpressionPrivate::exceptionToError(tc.Message(), error);
        reportScriptException(script, error);
    }
}

void QDeclarativeWorkerScriptEnginePrivate::processLoad(int id, const QUrl &url)
{
    if (url.isRelative())
        return;

    QString fileName = QDeclarativeEnginePrivate::urlToLocalFileOrQrc(url);

    QFile f(fileName);
    if (f.open(QIODevice::ReadOnly)) {
        QByteArray data = f.readAll();
        QString sourceCode = QString::fromUtf8(data);
        QDeclarativeScriptParser::extractPragmas(sourceCode);

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
            QDeclarativeError error;
            QDeclarativeExpressionPrivate::exceptionToError(tc.Message(), error);
            reportScriptException(script, error);
        }
    } else {
        qWarning().nospace() << "WorkerScript: Cannot find source file " << url.toString();
    }
}

void QDeclarativeWorkerScriptEnginePrivate::reportScriptException(WorkerScript *script, 
                                                                  const QDeclarativeError &error)
{
    QDeclarativeWorkerScriptEnginePrivate *p = QDeclarativeWorkerScriptEnginePrivate::get(workerEngine);

    QMutexLocker(&p->m_lock);
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

WorkerErrorEvent::WorkerErrorEvent(const QDeclarativeError &error)
: QEvent((QEvent::Type)WorkerError), m_error(error)
{
}

QDeclarativeError WorkerErrorEvent::error() const
{
    return m_error;
}

QDeclarativeWorkerScriptEngine::QDeclarativeWorkerScriptEngine(QDeclarativeEngine *parent)
: QThread(parent), d(new QDeclarativeWorkerScriptEnginePrivate(parent))
{
    d->m_lock.lock();
    connect(d, SIGNAL(stopThread()), this, SLOT(quit()), Qt::DirectConnection);
    start(QThread::IdlePriority);
    d->m_wait.wait(&d->m_lock);
    d->moveToThread(this);
    d->m_lock.unlock();
}

QDeclarativeWorkerScriptEngine::~QDeclarativeWorkerScriptEngine()
{
    d->m_lock.lock();
    QCoreApplication::postEvent(d, new QEvent((QEvent::Type)QDeclarativeWorkerScriptEnginePrivate::WorkerDestroyEvent));
    d->m_lock.unlock();

    wait();
    d->deleteLater();
}

QDeclarativeWorkerScriptEnginePrivate::WorkerScript::WorkerScript()
: id(-1), initialized(false), owner(0)
{
}

QDeclarativeWorkerScriptEnginePrivate::WorkerScript::~WorkerScript()
{
    object.Dispose(); object.Clear();
}

int QDeclarativeWorkerScriptEngine::registerWorkerScript(QDeclarativeWorkerScript *owner)
{
    typedef QDeclarativeWorkerScriptEnginePrivate::WorkerScript WorkerScript;
    WorkerScript *script = new WorkerScript;

    script->id = d->m_nextId++;
    script->owner = owner;

    d->m_lock.lock();
    d->workers.insert(script->id, script);
    d->m_lock.unlock();

    return script->id;
}

void QDeclarativeWorkerScriptEngine::removeWorkerScript(int id)
{
    QCoreApplication::postEvent(d, new WorkerRemoveEvent(id));
}

void QDeclarativeWorkerScriptEngine::executeUrl(int id, const QUrl &url)
{
    QCoreApplication::postEvent(d, new WorkerLoadEvent(id, url));
}

void QDeclarativeWorkerScriptEngine::sendMessage(int id, const QByteArray &data)
{
    QCoreApplication::postEvent(d, new WorkerDataEvent(id, data));
}

void QDeclarativeWorkerScriptEngine::run()
{
    d->m_lock.lock();

    v8::Isolate *isolate = v8::Isolate::New(); 
    isolate->Enter();

    d->workerEngine = new QDeclarativeWorkerScriptEnginePrivate::WorkerEngine(d);
    d->workerEngine->init();

    d->m_wait.wakeAll();

    d->m_lock.unlock();

    exec();

    qDeleteAll(d->workers);
    d->workers.clear();

    delete d->workerEngine; d->workerEngine = 0;

    isolate->Exit();
    isolate->Dispose();
}


/*!
    \qmlclass WorkerScript QDeclarativeWorkerScript
    \ingroup qml-utility-elements
    \brief The WorkerScript element enables the use of threads in QML.

    Use WorkerScript to run operations in a new thread.
    This is useful for running operations in the background so
    that the main GUI thread is not blocked.

    Messages can be passed between the new thread and the parent thread
    using \l sendMessage() and the \l {WorkerScript::onMessage}{onMessage()} handler.

    An example:

    \snippet doc/src/snippets/declarative/workerscript.qml 0

    The above worker script specifies a JavaScript file, "script.js", that handles
    the operations to be performed in the new thread. Here is \c script.js:

    \quotefile doc/src/snippets/declarative/script.js

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
    through QDeclarativeContext.

    Additionally, there are restrictions on the types of values that can be passed to and
    from the worker script. See the sendMessage() documentation for details.

    \sa {declarative/threading/workerscript}{WorkerScript example},
        {declarative/threading/threadedlistmodel}{Threaded ListModel example}
*/
QDeclarativeWorkerScript::QDeclarativeWorkerScript(QObject *parent)
: QObject(parent), m_engine(0), m_scriptId(-1), m_componentComplete(true)
{
}

QDeclarativeWorkerScript::~QDeclarativeWorkerScript()
{
    if (m_scriptId != -1) m_engine->removeWorkerScript(m_scriptId);
}

/*!
    \qmlproperty url WorkerScript::source

    This holds the url of the JavaScript file that implements the
    \tt WorkerScript.onMessage() handler for threaded operations.
*/
QUrl QDeclarativeWorkerScript::source() const
{
    return m_source;
}

void QDeclarativeWorkerScript::setSource(const QUrl &source)
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
    \o boolean, number, string
    \o JavaScript objects and arrays
    \o ListModel objects (any other type of QObject* is not allowed)
    \endlist

    All objects and arrays are copied to the \c message. With the exception
    of ListModel objects, any modifications by the other thread to an object
    passed in \c message will not be reflected in the original object.
*/
void QDeclarativeWorkerScript::sendMessage(QDeclarativeV8Function *args)
{
    if (!engine()) {
        qWarning("QDeclarativeWorkerScript: Attempt to send message before WorkerScript establishment");
        return;
    }

    v8::Handle<v8::Value> argument = v8::Undefined();
    if (args->Length() != 0) 
        argument = (*args)[0];

    m_engine->sendMessage(m_scriptId, QV8Worker::serialize(argument, args->engine()));
}

void QDeclarativeWorkerScript::classBegin()
{
    m_componentComplete = false;
}

QDeclarativeWorkerScriptEngine *QDeclarativeWorkerScript::engine()
{
    if (m_engine) return m_engine;
    if (m_componentComplete) {
        QDeclarativeEngine *engine = qmlEngine(this);
        if (!engine) {
            qWarning("QDeclarativeWorkerScript: engine() called without qmlEngine() set");
            return 0;
        }

        m_engine = QDeclarativeEnginePrivate::get(engine)->getWorkerScriptEngine();
        m_scriptId = m_engine->registerWorkerScript(this);

        if (m_source.isValid())
            m_engine->executeUrl(m_scriptId, m_source);

        return m_engine;
    }
    return 0;
}

void QDeclarativeWorkerScript::componentComplete()
{
    m_componentComplete = true;
    engine(); // Get it started now.
}

/*!
    \qmlsignal WorkerScript::onMessage(jsobject msg)

    This handler is called when a message \a msg is received from a worker
    script in another thread through a call to sendMessage().
*/

bool QDeclarativeWorkerScript::event(QEvent *event)
{
    if (event->type() == (QEvent::Type)WorkerDataEvent::WorkerData) {
        QDeclarativeEngine *engine = qmlEngine(this);
        if (engine) {
            WorkerDataEvent *workerEvent = static_cast<WorkerDataEvent *>(event);
            QV8Engine *v8engine = &QDeclarativeEnginePrivate::get(engine)->v8engine;
            v8::HandleScope handle_scope;
            v8::Context::Scope scope(v8engine->context());
            v8::Handle<v8::Value> value = QV8Worker::deserialize(workerEvent->data(), v8engine);
            emit message(QDeclarativeV8Handle::fromHandle(value));
        }
        return true;
    } else if (event->type() == (QEvent::Type)WorkerErrorEvent::WorkerError) {
        WorkerErrorEvent *workerEvent = static_cast<WorkerErrorEvent *>(event);
        QDeclarativeEnginePrivate::warning(qmlEngine(this), workerEvent->error());
        return true;
    } else {
        return QObject::event(event);
    }
}

QT_END_NAMESPACE

#include <qdeclarativeworkerscript.moc>

