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

#include "qqmltypeloader_p.h"
#include "qqmlabstracturlinterceptor_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlthread_p.h>
#include <private/qqmlcompiler_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlprofilerservice_p.h>
#include <private/qqmlmemoryprofiler_p.h>

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtQml/qqmlfile.h>
#include <QtCore/qdiriterator.h>
#include <QtQml/qqmlcomponent.h>
#include <QtCore/qwaitcondition.h>
#include <QtQml/qqmlextensioninterface.h>

#if defined (Q_OS_UNIX)
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#if defined (QT_LINUXBASE)
// LSB doesn't declare NAME_MAX. Use SYMLINK_MAX instead, which seems to
// always be identical to NAME_MAX
#ifndef NAME_MAX
#  define NAME_MAX _POSIX_SYMLINK_MAX
#endif

// LSB has a broken version of offsetof that can't be used at compile time
// https://lsbbugs.linuxfoundation.org/show_bug.cgi?id=3462
#undef offsetof
#define offsetof(TYPE, MEMBER) __builtin_offsetof (TYPE, MEMBER)
#endif

// #define DATABLOB_DEBUG

#ifdef DATABLOB_DEBUG

#define ASSERT_MAINTHREAD() do { if(m_thread->isThisThread()) qFatal("QQmlDataLoader: Caller not in main thread"); } while(false)
#define ASSERT_LOADTHREAD() do { if(!m_thread->isThisThread()) qFatal("QQmlDataLoader: Caller not in load thread"); } while(false)
#define ASSERT_CALLBACK() do { if(!m_manager || !m_manager->m_thread->isThisThread()) qFatal("QQmlDataBlob: An API call was made outside a callback"); } while(false)

#else

#define ASSERT_MAINTHREAD() 
#define ASSERT_LOADTHREAD()
#define ASSERT_CALLBACK()

#endif

DEFINE_BOOL_CONFIG_OPTION(dumpErrors, QML_DUMP_ERRORS);

QT_BEGIN_NAMESPACE

namespace {

    template<typename LockType>
    struct LockHolder
    {
        LockType& lock;
        LockHolder(LockType *l) : lock(*l) { lock.lock(); }
        ~LockHolder() { lock.unlock(); }
    };
}

// This is a lame object that we need to ensure that slots connected to
// QNetworkReply get called in the correct thread (the loader thread).  
// As QQmlDataLoader lives in the main thread, and we can't use
// Qt::DirectConnection connections from a QNetworkReply (because then 
// sender() wont work), we need to insert this object in the middle.
class QQmlDataLoaderNetworkReplyProxy : public QObject
{
    Q_OBJECT
public:
    QQmlDataLoaderNetworkReplyProxy(QQmlDataLoader *l);

public slots:
    void finished();
    void downloadProgress(qint64, qint64);
    void manualFinished(QNetworkReply*);

private:
    QQmlDataLoader *l;
};

class QQmlDataLoaderThread : public QQmlThread
{
    typedef QQmlDataLoaderThread This;

public:
    QQmlDataLoaderThread(QQmlDataLoader *loader);
    QNetworkAccessManager *networkAccessManager() const;
    QQmlDataLoaderNetworkReplyProxy *networkReplyProxy() const;

    void load(QQmlDataBlob *b);
    void loadAsync(QQmlDataBlob *b);
    void loadWithStaticData(QQmlDataBlob *b, const QByteArray &);
    void loadWithStaticDataAsync(QQmlDataBlob *b, const QByteArray &);
    void callCompleted(QQmlDataBlob *b);
    void callDownloadProgressChanged(QQmlDataBlob *b, qreal p);
    void initializeEngine(QQmlExtensionInterface *, const char *);

protected:
    virtual void shutdownThread();

private:
    void loadThread(QQmlDataBlob *b);
    void loadWithStaticDataThread(QQmlDataBlob *b, const QByteArray &);
    void callCompletedMain(QQmlDataBlob *b);
    void callDownloadProgressChangedMain(QQmlDataBlob *b, qreal p);
    void initializeEngineMain(QQmlExtensionInterface *iface, const char *uri);

    QQmlDataLoader *m_loader;
    mutable QNetworkAccessManager *m_networkAccessManager;
    mutable QQmlDataLoaderNetworkReplyProxy *m_networkReplyProxy;
};


QQmlDataLoaderNetworkReplyProxy::QQmlDataLoaderNetworkReplyProxy(QQmlDataLoader *l) 
: l(l) 
{
}

void QQmlDataLoaderNetworkReplyProxy::finished()
{
    Q_ASSERT(sender());
    Q_ASSERT(qobject_cast<QNetworkReply *>(sender()));
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    l->networkReplyFinished(reply);
}

void QQmlDataLoaderNetworkReplyProxy::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_ASSERT(sender());
    Q_ASSERT(qobject_cast<QNetworkReply *>(sender()));
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    l->networkReplyProgress(reply, bytesReceived, bytesTotal);
}

// This function is for when you want to shortcut the signals and call directly
void QQmlDataLoaderNetworkReplyProxy::manualFinished(QNetworkReply *reply)
{
    qint64 replySize = reply->size();
    l->networkReplyProgress(reply, replySize, replySize);
    l->networkReplyFinished(reply);
}


/*!
\class QQmlDataBlob
\brief The QQmlDataBlob encapsulates a data request that can be issued to a QQmlDataLoader.
\internal

QQmlDataBlob's are loaded by a QQmlDataLoader.  The user creates the QQmlDataBlob
and then calls QQmlDataLoader::load() or QQmlDataLoader::loadWithStaticData() to load it.
The QQmlDataLoader invokes callbacks on the QQmlDataBlob as data becomes available.
*/

/*!
\enum QQmlDataBlob::Status

This enum describes the status of the data blob.

\list
\li Null The blob has not yet been loaded by a QQmlDataLoader
\li Loading The blob is loading network data.  The QQmlDataBlob::setData() callback has not yet been
invoked or has not yet returned.
\li WaitingForDependencies The blob is waiting for dependencies to be done before continueing.  This status
only occurs after the QQmlDataBlob::setData() callback has been made, and when the blob has outstanding
dependencies.
\li Complete The blob's data has been loaded and all dependencies are done.
\li Error An error has been set on this blob.
\endlist
*/

/*!
\enum QQmlDataBlob::Type

This enum describes the type of the data blob.

\list
\li QmlFile This is a QQmlTypeData
\li JavaScriptFile This is a QQmlScriptData
\li QmldirFile This is a QQmlQmldirData
\endlist
*/

/*!
Create a new QQmlDataBlob for \a url and of the provided \a type.
*/
QQmlDataBlob::QQmlDataBlob(const QUrl &url, Type type)
: m_type(type), m_url(url), m_finalUrl(url), m_manager(0), m_redirectCount(0), 
  m_inCallback(false), m_isDone(false)
{
}

/*!  \internal */
QQmlDataBlob::~QQmlDataBlob()
{
    Q_ASSERT(m_waitingOnMe.isEmpty());

    cancelAllWaitingFor();
}

/*!
  Sets the manager, and does stuff like selection which needs access to the manager.
  Must be called before loading can occur.
*/
void QQmlDataBlob::startLoading(QQmlDataLoader *manager)
{
    Q_ASSERT(status() == QQmlDataBlob::Null);
    Q_ASSERT(m_manager == 0);
    m_data.setStatus(QQmlDataBlob::Loading);
    m_manager = manager;

    //Set here because we need to get the engine from the manager
    if (manager && manager->engine() && manager->engine()->urlInterceptor())
        m_url = manager->engine()->urlInterceptor()->intercept(m_url,
                    (QQmlAbstractUrlInterceptor::DataType)m_type);
}

/*!
Returns the type provided to the constructor.
*/
QQmlDataBlob::Type QQmlDataBlob::type() const
{
    return m_type;
}

/*!
Returns the blob's status.
*/
QQmlDataBlob::Status QQmlDataBlob::status() const
{
    return m_data.status();
}

/*!
Returns true if the status is Null.
*/
bool QQmlDataBlob::isNull() const
{
    return status() == Null;
}

/*!
Returns true if the status is Loading.
*/
bool QQmlDataBlob::isLoading() const
{
    return status() == Loading;
}

/*!
Returns true if the status is WaitingForDependencies.
*/
bool QQmlDataBlob::isWaiting() const
{
    return status() == WaitingForDependencies;
}

/*!
Returns true if the status is Complete.
*/
bool QQmlDataBlob::isComplete() const
{
    return status() == Complete;
}

/*!
Returns true if the status is Error.
*/
bool QQmlDataBlob::isError() const
{
    return status() == Error;
}

/*!
Returns true if the status is Complete or Error.
*/
bool QQmlDataBlob::isCompleteOrError() const
{
    Status s = status();
    return s == Error || s == Complete;
}

/*!
Returns the data download progress from 0 to 1.
*/
qreal QQmlDataBlob::progress() const
{
    quint8 p = m_data.progress();
    if (p == 0xFF) return 1.;
    else return qreal(p) / qreal(0xFF);
}

/*!
Returns the blob url passed to the constructor.  If a network redirect
happens while fetching the data, this url remains the same.

\sa finalUrl()
*/
QUrl QQmlDataBlob::url() const
{
    return m_url;
}

/*!
Returns the final url of the data.  Initially this is the same as
url(), but if a network redirect happens while fetching the data, this url
is updated to reflect the new location.

May only be called from the load thread, or after the blob isCompleteOrError().
*/
QUrl QQmlDataBlob::finalUrl() const
{
    Q_ASSERT(isCompleteOrError() || (m_manager && m_manager->m_thread->isThisThread()));
    return m_finalUrl;
}

/*!
Returns the finalUrl() as a string.
*/
QString QQmlDataBlob::finalUrlString() const
{
    Q_ASSERT(isCompleteOrError() || (m_manager && m_manager->m_thread->isThisThread()));
    if (m_finalUrlString.isEmpty())
        m_finalUrlString = m_finalUrl.toString();

    return m_finalUrlString;
}

/*!
Return the errors on this blob.

May only be called from the load thread, or after the blob isCompleteOrError().
*/
QList<QQmlError> QQmlDataBlob::errors() const
{
    Q_ASSERT(isCompleteOrError() || (m_manager && m_manager->m_thread->isThisThread()));
    return m_errors;
}

/*!
Mark this blob as having \a errors.

All outstanding dependencies will be cancelled.  Requests to add new dependencies 
will be ignored.  Entry into the Error state is irreversable.

The setError() method may only be called from within a QQmlDataBlob callback.
*/
void QQmlDataBlob::setError(const QQmlError &errors)
{
    ASSERT_CALLBACK();

    QList<QQmlError> l;
    l << errors;
    setError(l);
}

/*!
\overload
*/
void QQmlDataBlob::setError(const QList<QQmlError> &errors)
{
    ASSERT_CALLBACK();

    Q_ASSERT(status() != Error);
    Q_ASSERT(m_errors.isEmpty());

    m_errors = errors; // Must be set before the m_data fence
    m_data.setStatus(Error);

    if (dumpErrors()) {
        qWarning().nospace() << "Errors for " << m_finalUrl.toString();
        for (int ii = 0; ii < errors.count(); ++ii)
            qWarning().nospace() << "    " << qPrintable(errors.at(ii).toString());
    }
    cancelAllWaitingFor();

    if (!m_inCallback)
        tryDone();
}

/*! 
Wait for \a blob to become complete or to error.  If \a blob is already 
complete or in error, or this blob is already complete, this has no effect.

The setError() method may only be called from within a QQmlDataBlob callback.
*/
void QQmlDataBlob::addDependency(QQmlDataBlob *blob)
{
    ASSERT_CALLBACK();

    Q_ASSERT(status() != Null);

    if (!blob ||
        blob->status() == Error || blob->status() == Complete ||
        status() == Error || status() == Complete || m_isDone || 
        m_waitingFor.contains(blob))
        return;

    blob->addref();

    m_data.setStatus(WaitingForDependencies);

    m_waitingFor.append(blob);
    blob->m_waitingOnMe.append(this);
}

/*!
\fn void QQmlDataBlob::dataReceived(const Data &data)

Invoked when data for the blob is received.  Implementors should use this callback
to determine a blob's dependencies.  Within this callback you may call setError()
or addDependency().
*/

/*!
Invoked once data has either been received or a network error occurred, and all 
dependencies are complete.

You can set an error in this method, but you cannot add new dependencies.  Implementors
should use this callback to finalize processing of data.

The default implementation does nothing.

XXX Rename processData() or some such to avoid confusion between done() (processing thread)
and completed() (main thread)
*/
void QQmlDataBlob::done()
{
}

/*!
Invoked if there is a network error while fetching this blob.

The default implementation sets an appropriate QQmlError.
*/
void QQmlDataBlob::networkError(QNetworkReply::NetworkError networkError)
{
    Q_UNUSED(networkError);

    QQmlError error;
    error.setUrl(m_finalUrl);

    const char *errorString = 0;
    switch (networkError) {
        default:
            errorString = "Network error";
            break;
        case QNetworkReply::ConnectionRefusedError:
            errorString = "Connection refused";
            break;
        case QNetworkReply::RemoteHostClosedError:
            errorString = "Remote host closed the connection";
            break;
        case QNetworkReply::HostNotFoundError:
            errorString = "Host not found";
            break;
        case QNetworkReply::TimeoutError:
            errorString = "Timeout";
            break;
        case QNetworkReply::ProxyConnectionRefusedError:
        case QNetworkReply::ProxyConnectionClosedError:
        case QNetworkReply::ProxyNotFoundError:
        case QNetworkReply::ProxyTimeoutError:
        case QNetworkReply::ProxyAuthenticationRequiredError:
        case QNetworkReply::UnknownProxyError:
            errorString = "Proxy error";
            break;
        case QNetworkReply::ContentAccessDenied:
            errorString = "Access denied";
            break;
        case QNetworkReply::ContentNotFoundError:
            errorString = "File not found";
            break;
        case QNetworkReply::AuthenticationRequiredError:
            errorString = "Authentication required";
            break;
    };

    error.setDescription(QLatin1String(errorString));

    setError(error);
}

/*! 
Called if \a blob, which was previously waited for, has an error.

The default implementation does nothing.
*/
void QQmlDataBlob::dependencyError(QQmlDataBlob *blob)
{
    Q_UNUSED(blob);
}

/*!
Called if \a blob, which was previously waited for, has completed.

The default implementation does nothing.
*/
void QQmlDataBlob::dependencyComplete(QQmlDataBlob *blob)
{
    Q_UNUSED(blob);
}

/*! 
Called when all blobs waited for have completed.  This occurs regardless of 
whether they are in error, or complete state.  

The default implementation does nothing.
*/
void QQmlDataBlob::allDependenciesDone()
{
}

/*!
Called when the download progress of this blob changes.  \a progress goes
from 0 to 1.

This callback is only invoked if an asynchronous load for this blob is 
made.  An asynchronous load is one in which the Asynchronous mode is
specified explicitly, or one that is implicitly delayed due to a network 
operation.

The default implementation does nothing.
*/
void QQmlDataBlob::downloadProgressChanged(qreal progress)
{
    Q_UNUSED(progress);
}

/*!
Invoked on the main thread sometime after done() was called on the load thread.

You cannot modify the blobs state at all in this callback and cannot depend on the
order or timeliness of these callbacks.  Implementors should use this callback to notify 
dependencies on the main thread that the blob is done and not a lot else.

This callback is only invoked if an asynchronous load for this blob is 
made.  An asynchronous load is one in which the Asynchronous mode is
specified explicitly, or one that is implicitly delayed due to a network 
operation.

The default implementation does nothing.
*/
void QQmlDataBlob::completed()
{
}


void QQmlDataBlob::tryDone()
{
    if (status() != Loading && m_waitingFor.isEmpty() && !m_isDone) {
        m_isDone = true;
        addref();

#ifdef DATABLOB_DEBUG
        qWarning("QQmlDataBlob::done() %s", qPrintable(url().toString()));
#endif
        done();

        if (status() != Error)
            m_data.setStatus(Complete);

        notifyAllWaitingOnMe();

        // Locking is not required here, as anyone expecting callbacks must
        // already be protected against the blob being completed (as set above);
        if (m_data.isAsync()) {
#ifdef DATABLOB_DEBUG
            qWarning("QQmlDataBlob: Dispatching completed");
#endif
            m_manager->m_thread->callCompleted(this);
        }

        release();
    }
}

void QQmlDataBlob::cancelAllWaitingFor()
{
    while (m_waitingFor.count()) {
        QQmlDataBlob *blob = m_waitingFor.takeLast();

        Q_ASSERT(blob->m_waitingOnMe.contains(this));

        blob->m_waitingOnMe.removeOne(this);

        blob->release();
    }
}

void QQmlDataBlob::notifyAllWaitingOnMe()
{
    while (m_waitingOnMe.count()) {
        QQmlDataBlob *blob = m_waitingOnMe.takeLast();

        Q_ASSERT(blob->m_waitingFor.contains(this));

        blob->notifyComplete(this);
    }
}

void QQmlDataBlob::notifyComplete(QQmlDataBlob *blob)
{
    Q_ASSERT(m_waitingFor.contains(blob));
    Q_ASSERT(blob->status() == Error || blob->status() == Complete);

    m_inCallback = true;

    m_waitingFor.removeOne(blob);

    if (blob->status() == Error) {
        dependencyError(blob);
    } else if (blob->status() == Complete) {
        dependencyComplete(blob);
    }

    blob->release();

    if (!isError() && m_waitingFor.isEmpty()) 
        allDependenciesDone();

    m_inCallback = false;

    tryDone();
}

#define TD_STATUS_MASK 0x0000FFFF
#define TD_STATUS_SHIFT 0
#define TD_PROGRESS_MASK 0x00FF0000
#define TD_PROGRESS_SHIFT 16
#define TD_ASYNC_MASK 0x80000000

QQmlDataBlob::ThreadData::ThreadData()
: _p(0)
{
}

QQmlDataBlob::Status QQmlDataBlob::ThreadData::status() const
{
    return QQmlDataBlob::Status((_p.load() & TD_STATUS_MASK) >> TD_STATUS_SHIFT);
}

void QQmlDataBlob::ThreadData::setStatus(QQmlDataBlob::Status status)
{
    while (true) {
        int d = _p.load();
        int nd = (d & ~TD_STATUS_MASK) | ((status << TD_STATUS_SHIFT) & TD_STATUS_MASK);
        if (d == nd || _p.testAndSetOrdered(d, nd)) return;
    }
}

bool QQmlDataBlob::ThreadData::isAsync() const
{
    return _p.load() & TD_ASYNC_MASK;
}

void QQmlDataBlob::ThreadData::setIsAsync(bool v)
{
    while (true) {
        int d = _p.load();
        int nd = (d & ~TD_ASYNC_MASK) | (v?TD_ASYNC_MASK:0);
        if (d == nd || _p.testAndSetOrdered(d, nd)) return;
    }
}

quint8 QQmlDataBlob::ThreadData::progress() const
{
    return quint8((_p.load() & TD_PROGRESS_MASK) >> TD_PROGRESS_SHIFT);
}

void QQmlDataBlob::ThreadData::setProgress(quint8 v)
{
    while (true) {
        int d = _p.load();
        int nd = (d & ~TD_PROGRESS_MASK) | ((v << TD_PROGRESS_SHIFT) & TD_PROGRESS_MASK);
        if (d == nd || _p.testAndSetOrdered(d, nd)) return;
    }
}

QQmlDataLoaderThread::QQmlDataLoaderThread(QQmlDataLoader *loader)
: m_loader(loader), m_networkAccessManager(0), m_networkReplyProxy(0)
{
}

QNetworkAccessManager *QQmlDataLoaderThread::networkAccessManager() const
{
    Q_ASSERT(isThisThread());
    if (!m_networkAccessManager) {
        m_networkAccessManager = QQmlEnginePrivate::get(m_loader->engine())->createNetworkAccessManager(0);
        m_networkReplyProxy = new QQmlDataLoaderNetworkReplyProxy(m_loader);
    }

    return m_networkAccessManager;
}

QQmlDataLoaderNetworkReplyProxy *QQmlDataLoaderThread::networkReplyProxy() const
{
    Q_ASSERT(isThisThread());
    Q_ASSERT(m_networkReplyProxy); // Must call networkAccessManager() first
    return m_networkReplyProxy;
}

void QQmlDataLoaderThread::load(QQmlDataBlob *b) 
{ 
    b->addref();
    callMethodInThread(&This::loadThread, b); 
}

void QQmlDataLoaderThread::loadAsync(QQmlDataBlob *b)
{
    b->addref();
    postMethodToThread(&This::loadThread, b);
}

void QQmlDataLoaderThread::loadWithStaticData(QQmlDataBlob *b, const QByteArray &d)
{
    b->addref();
    callMethodInThread(&This::loadWithStaticDataThread, b, d);
}

void QQmlDataLoaderThread::loadWithStaticDataAsync(QQmlDataBlob *b, const QByteArray &d)
{
    b->addref();
    postMethodToThread(&This::loadWithStaticDataThread, b, d);
}

void QQmlDataLoaderThread::callCompleted(QQmlDataBlob *b) 
{ 
    b->addref(); 
    postMethodToMain(&This::callCompletedMain, b); 
}

void QQmlDataLoaderThread::callDownloadProgressChanged(QQmlDataBlob *b, qreal p) 
{ 
    b->addref(); 
    postMethodToMain(&This::callDownloadProgressChangedMain, b, p); 
}

void QQmlDataLoaderThread::initializeEngine(QQmlExtensionInterface *iface, 
                                                    const char *uri)
{
    callMethodInMain(&This::initializeEngineMain, iface, uri);
}

void QQmlDataLoaderThread::shutdownThread()
{
    delete m_networkAccessManager;
    m_networkAccessManager = 0;
    delete m_networkReplyProxy;
    m_networkReplyProxy = 0;
}

void QQmlDataLoaderThread::loadThread(QQmlDataBlob *b) 
{ 
    m_loader->loadThread(b); 
    b->release();
}

void QQmlDataLoaderThread::loadWithStaticDataThread(QQmlDataBlob *b, const QByteArray &d)
{
    m_loader->loadWithStaticDataThread(b, d);
    b->release();
}

void QQmlDataLoaderThread::callCompletedMain(QQmlDataBlob *b) 
{ 
    QML_MEMORY_SCOPE_URL(b->url());
#ifdef DATABLOB_DEBUG
    qWarning("QQmlDataLoaderThread: %s completed() callback", qPrintable(b->url().toString())); 
#endif
    b->completed(); 
    b->release(); 
}

void QQmlDataLoaderThread::callDownloadProgressChangedMain(QQmlDataBlob *b, qreal p) 
{ 
#ifdef DATABLOB_DEBUG
    qWarning("QQmlDataLoaderThread: %s downloadProgressChanged(%f) callback", 
             qPrintable(b->url().toString()), p); 
#endif
    b->downloadProgressChanged(p); 
    b->release(); 
}

void QQmlDataLoaderThread::initializeEngineMain(QQmlExtensionInterface *iface, 
                                                        const char *uri)
{
    Q_ASSERT(m_loader->engine()->thread() == QThread::currentThread());
    iface->initializeEngine(m_loader->engine(), uri);
}

/*!
\class QQmlDataLoader
\brief The QQmlDataLoader class abstracts loading files and their dependencies over the network.
\internal

The QQmlDataLoader class is provided for the exclusive use of the QQmlTypeLoader class.

Clients create QQmlDataBlob instances and submit them to the QQmlDataLoader class
through the QQmlDataLoader::load() or QQmlDataLoader::loadWithStaticData() methods.
The loader then fetches the data over the network or from the local file system in an efficient way.
QQmlDataBlob is an abstract class, so should always be specialized.

Once data is received, the QQmlDataBlob::dataReceived() method is invoked on the blob.  The
derived class should use this callback to process the received data.  Processing of the data can 
result in an error being set (QQmlDataBlob::setError()), or one or more dependencies being
created (QQmlDataBlob::addDependency()).  Dependencies are other QQmlDataBlob's that
are required before processing can fully complete.

To complete processing, the QQmlDataBlob::done() callback is invoked.  done() is called when
one of these three preconditions are met.

\list 1
\li The QQmlDataBlob has no dependencies.
\li The QQmlDataBlob has an error set.
\li All the QQmlDataBlob's dependencies are themselves "done()".
\endlist

Thus QQmlDataBlob::done() will always eventually be called, even if the blob has an error set.
*/

/*!
Create a new QQmlDataLoader for \a engine.
*/
QQmlDataLoader::QQmlDataLoader(QQmlEngine *engine)
: m_engine(engine), m_thread(new QQmlDataLoaderThread(this))
{
}

/*! \internal */
QQmlDataLoader::~QQmlDataLoader()
{
    for (NetworkReplies::Iterator iter = m_networkReplies.begin(); iter != m_networkReplies.end(); ++iter) 
        (*iter)->release();

    shutdownThread();
    delete m_thread;
}

void QQmlDataLoader::lock()
{
    m_thread->lock();
}

void QQmlDataLoader::unlock()
{
    m_thread->unlock();
}

/*!
Load the provided \a blob from the network or filesystem.

The loader must be locked.
*/
void QQmlDataLoader::load(QQmlDataBlob *blob, Mode mode)
{
#ifdef DATABLOB_DEBUG
    qWarning("QQmlDataLoader::load(%s): %s thread", qPrintable(blob->m_url.toString()), 
             m_thread->isThisThread()?"Compile":"Engine");
#endif
    blob->startLoading(this);

    if (m_thread->isThisThread()) {
        unlock();
        loadThread(blob);
        lock();
    } else if (mode == PreferSynchronous) {
        unlock();
        m_thread->load(blob);
        lock();
        if (!blob->isCompleteOrError())
            blob->m_data.setIsAsync(true);
    } else {
        Q_ASSERT(mode == Asynchronous);
        blob->m_data.setIsAsync(true);
        unlock();
        m_thread->loadAsync(blob);
        lock();
    }
}

/*!
Load the provided \a blob with \a data.  The blob's URL is not used by the data loader in this case.

The loader must be locked.
*/
void QQmlDataLoader::loadWithStaticData(QQmlDataBlob *blob, const QByteArray &data, Mode mode)
{
#ifdef DATABLOB_DEBUG
    qWarning("QQmlDataLoader::loadWithStaticData(%s, data): %s thread", qPrintable(blob->m_url.toString()), 
             m_thread->isThisThread()?"Compile":"Engine");
#endif

    blob->startLoading(this);

    if (m_thread->isThisThread()) {
        unlock();
        loadWithStaticDataThread(blob, data);
        lock();
    } else if (mode == PreferSynchronous) {
        unlock();
        m_thread->loadWithStaticData(blob, data);
        lock();
        if (!blob->isCompleteOrError())
            blob->m_data.setIsAsync(true);
    } else {
        Q_ASSERT(mode == Asynchronous);
        blob->m_data.setIsAsync(true);
        unlock();
        m_thread->loadWithStaticDataAsync(blob, data);
        lock();
    }
}

void QQmlDataLoader::loadWithStaticDataThread(QQmlDataBlob *blob, const QByteArray &data)
{
    ASSERT_LOADTHREAD();

    setData(blob, data);
}

void QQmlDataLoader::loadThread(QQmlDataBlob *blob)
{
    ASSERT_LOADTHREAD();

    // Don't continue loading if we've been shutdown
    if (m_thread->isShutdown()) {
        QQmlError error;
        error.setDescription(QLatin1String("Interrupted by shutdown"));
        blob->setError(error);
        return;
    }

    if (blob->m_url.isEmpty()) {
        QQmlError error;
        error.setDescription(QLatin1String("Invalid null URL"));
        blob->setError(error);
        return;
    }

    QML_MEMORY_SCOPE_URL(blob->m_url);
    QQmlEnginePrivate *engine_d = QQmlEnginePrivate::get(m_engine);
    QHash<QUrl, QByteArray> debugCache = engine_d->debugChangesCache();

    if (!debugCache.isEmpty()) {
        foreach (const QUrl &url, debugCache.keys()) {
            if (blob->m_url == blob->m_url.resolved(url)) {
                blob->m_data.setProgress(0xFF);
                if (blob->m_data.isAsync())
                    m_thread->callDownloadProgressChanged(blob, 1.);
                setData(blob, debugCache.value(url, QByteArray()));
                return;
            }
        }
    }

    if (QQmlFile::isSynchronous(blob->m_url)) {
        QQmlFile file(m_engine, blob->m_url);

        if (file.isError()) {
            QQmlError error;
            error.setUrl(blob->m_url);
            error.setDescription(file.error());
            blob->setError(error);
            return;
        }

        blob->m_data.setProgress(0xFF);
        if (blob->m_data.isAsync())
            m_thread->callDownloadProgressChanged(blob, 1.);

        setData(blob, &file);

    } else {

        QNetworkReply *reply = m_thread->networkAccessManager()->get(QNetworkRequest(blob->m_url));
        QQmlDataLoaderNetworkReplyProxy *nrp = m_thread->networkReplyProxy();
        blob->addref();
        m_networkReplies.insert(reply, blob);

        if (reply->isFinished()) {
            nrp->manualFinished(reply);
        } else {
            QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
                             nrp, SLOT(downloadProgress(qint64,qint64)));
            QObject::connect(reply, SIGNAL(finished()),
                             nrp, SLOT(finished()));
        }

#ifdef DATABLOB_DEBUG
        qWarning("QQmlDataBlob: requested %s", qPrintable(blob->url().toString()));
#endif

    }
}

#define DATALOADER_MAXIMUM_REDIRECT_RECURSION 16

void QQmlDataLoader::networkReplyFinished(QNetworkReply *reply)
{
    Q_ASSERT(m_thread->isThisThread());

    reply->deleteLater();

    QQmlDataBlob *blob = m_networkReplies.take(reply);

    Q_ASSERT(blob);

    blob->m_redirectCount++;

    if (blob->m_redirectCount < DATALOADER_MAXIMUM_REDIRECT_RECURSION) {
        QVariant redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            QUrl url = reply->url().resolved(redirect.toUrl());
            blob->m_finalUrl = url;

            QNetworkReply *reply = m_thread->networkAccessManager()->get(QNetworkRequest(url));
            QObject *nrp = m_thread->networkReplyProxy();
            QObject::connect(reply, SIGNAL(finished()), nrp, SLOT(finished()));
            m_networkReplies.insert(reply, blob);
#ifdef DATABLOB_DEBUG
            qWarning("QQmlDataBlob: redirected to %s", qPrintable(blob->m_finalUrl.toString()));
#endif
            return;
        }
    }

    if (reply->error()) {
        blob->networkError(reply->error());
    } else {
        QByteArray data = reply->readAll();
        setData(blob, data);
    }

    blob->release();
}

void QQmlDataLoader::networkReplyProgress(QNetworkReply *reply,
                                                  qint64 bytesReceived, qint64 bytesTotal)
{
    Q_ASSERT(m_thread->isThisThread());

    QQmlDataBlob *blob = m_networkReplies.value(reply);

    Q_ASSERT(blob);

    if (bytesTotal != 0) {
        quint8 progress = 0xFF * (qreal(bytesReceived) / qreal(bytesTotal));
        blob->m_data.setProgress(progress);
        if (blob->m_data.isAsync())
            m_thread->callDownloadProgressChanged(blob, blob->m_data.progress());
    }
}

/*!
Return the QQmlEngine associated with this loader
*/
QQmlEngine *QQmlDataLoader::engine() const
{
    return m_engine;
}

/*!
Call the initializeEngine() method on \a iface.  Used by QQmlImportDatabase to ensure it
gets called in the correct thread.
*/
void QQmlDataLoader::initializeEngine(QQmlExtensionInterface *iface, 
                                              const char *uri)
{
    Q_ASSERT(m_thread->isThisThread() || engine()->thread() == QThread::currentThread());

    if (m_thread->isThisThread()) {
        m_thread->initializeEngine(iface, uri);
    } else {
        Q_ASSERT(engine()->thread() == QThread::currentThread());
        iface->initializeEngine(engine(), uri);
    }
}


void QQmlDataLoader::setData(QQmlDataBlob *blob, const QByteArray &data)
{
    QML_MEMORY_SCOPE_URL(blob->url());
    QQmlDataBlob::Data d;
    d.d = &data;
    setData(blob, d);
}

void QQmlDataLoader::setData(QQmlDataBlob *blob, QQmlFile *file)
{
    QML_MEMORY_SCOPE_URL(blob->url());
    QQmlDataBlob::Data d;
    d.d = file;
    setData(blob, d);
}

void QQmlDataLoader::setData(QQmlDataBlob *blob, const QQmlDataBlob::Data &d)
{
    QML_MEMORY_SCOPE_URL(blob->url());
    blob->m_inCallback = true;

    blob->dataReceived(d);

    if (!blob->isError() && !blob->isWaiting())
        blob->allDependenciesDone();

    if (blob->status() != QQmlDataBlob::Error) 
        blob->m_data.setStatus(QQmlDataBlob::WaitingForDependencies);

    blob->m_inCallback = false;

    blob->tryDone();
}

void QQmlDataLoader::shutdownThread()
{
    if (!m_thread->isShutdown())
        m_thread->shutdown();
}

QQmlTypeLoader::Blob::Blob(const QUrl &url, QQmlDataBlob::Type type, QQmlTypeLoader *loader)
: QQmlDataBlob(url, type), m_typeLoader(loader), m_imports(loader)
{
}

QQmlTypeLoader::Blob::~Blob()
{
    for (int ii = 0; ii < m_qmldirs.count(); ++ii)
        m_qmldirs.at(ii)->release();
}

bool QQmlTypeLoader::Blob::fetchQmldir(const QUrl &url, const QQmlScript::Import *import, int priority, QList<QQmlError> *errors)
{
    QQmlQmldirData *data = typeLoader()->getQmldir(url);

    data->setImport(import);
    data->setPriority(priority);

    if (data->status() == Error) {
        // This qmldir must not exist - which is not an error
        data->release();
        return true;
    } else if (data->status() == Complete) {
        // This data is already available
        return qmldirDataAvailable(data, errors);
    }

    // Wait for this data to become available
    addDependency(data);
    return true;
}

bool QQmlTypeLoader::Blob::updateQmldir(QQmlQmldirData *data, const QQmlScript::Import *import, QList<QQmlError> *errors)
{
    QString qmldirIdentifier = data->url().toString();
    QString qmldirUrl = qmldirIdentifier.left(qmldirIdentifier.lastIndexOf(QLatin1Char('/')) + 1);

    typeLoader()->setQmldirContent(qmldirIdentifier, data->content());

    if (!m_imports.updateQmldirContent(typeLoader()->importDatabase(), import->uri, import->qualifier, qmldirIdentifier, qmldirUrl, errors))
        return false;

    QHash<const QQmlScript::Import *, int>::iterator it = m_unresolvedImports.find(import);
    if (it != m_unresolvedImports.end()) {
        *it = data->priority();
    }

    // Release this reference at destruction
    m_qmldirs << data;

    if (!import->qualifier.isEmpty()) {
        // Does this library contain any qualified scripts?
        QUrl libraryUrl(qmldirUrl);
        const QmldirContent *qmldir = typeLoader()->qmldirContent(qmldirIdentifier, qmldirUrl);
        foreach (const QQmlDirParser::Script &script, qmldir->scripts()) {
            QUrl scriptUrl = libraryUrl.resolved(QUrl(script.fileName));
            QQmlScriptBlob *blob = typeLoader()->getScript(scriptUrl);
            addDependency(blob);

            scriptImported(blob, import->location.start, script.nameSpace, import->qualifier);
        }
    }

    return true;
}

bool QQmlTypeLoader::Blob::addImport(const QQmlScript::Import &import, QList<QQmlError> *errors)
{
    Q_ASSERT(errors);

    QQmlImportDatabase *importDatabase = typeLoader()->importDatabase();

    if (import.type == QQmlScript::Import::Script) {
        QUrl scriptUrl = finalUrl().resolved(QUrl(import.uri));
        QQmlScriptBlob *blob = typeLoader()->getScript(scriptUrl);
        addDependency(blob);

        scriptImported(blob, import.location.start, import.qualifier, QString());
    } else if (import.type == QQmlScript::Import::Library) {
        QString qmldirFilePath;
        QString qmldirUrl;

        if (m_imports.locateQmldir(importDatabase, import.uri, import.majorVersion, import.minorVersion,
                                 &qmldirFilePath, &qmldirUrl)) {
            // This is a local library import
            if (!m_imports.addLibraryImport(importDatabase, import.uri, import.qualifier, import.majorVersion,
                                          import.minorVersion, qmldirFilePath, qmldirUrl, false, errors))
                return false;

            if (!import.qualifier.isEmpty()) {
                // Does this library contain any qualified scripts?
                QUrl libraryUrl(qmldirUrl);
                const QmldirContent *qmldir = typeLoader()->qmldirContent(qmldirFilePath, qmldirUrl);
                foreach (const QQmlDirParser::Script &script, qmldir->scripts()) {
                    QUrl scriptUrl = libraryUrl.resolved(QUrl(script.fileName));
                    QQmlScriptBlob *blob = typeLoader()->getScript(scriptUrl);
                    addDependency(blob);

                    scriptImported(blob, import.location.start, script.nameSpace, import.qualifier);
                }
            }
        } else {
            // Is this a module?
            if (QQmlMetaType::isAnyModule(import.uri)) {
                if (!m_imports.addLibraryImport(importDatabase, import.uri, import.qualifier, import.majorVersion,
                                              import.minorVersion, QString(), QString(), false, errors))
                    return false;
            } else {
                // We haven't yet resolved this import
                m_unresolvedImports.insert(&import, 0);

                // Query any network import paths for this library
                QStringList remotePathList = importDatabase->importPathList(QQmlImportDatabase::Remote);
                if (!remotePathList.isEmpty()) {
                    // Add this library and request the possible locations for it
                    if (!m_imports.addLibraryImport(importDatabase, import.uri, import.qualifier, import.majorVersion,
                                                  import.minorVersion, QString(), QString(), true, errors))
                        return false;

                    // Probe for all possible locations
                    int priority = 0;
                    for (int version = QQmlImports::FullyVersioned; version <= QQmlImports::Unversioned; ++version) {
                        foreach (const QString &path, remotePathList) {
                            QString qmldirUrl = QQmlImports::completeQmldirPath(import.uri, path, import.majorVersion, import.minorVersion,
                                                                                static_cast<QQmlImports::ImportVersion>(version));
                            if (!fetchQmldir(QUrl(qmldirUrl), &import, ++priority, errors))
                                return false;
                        }
                    }
                }
            }
        }
    } else {
        Q_ASSERT(import.type == QQmlScript::Import::File);

        bool incomplete = false;

        QUrl qmldirUrl;
        if (import.qualifier.isEmpty()) {
            qmldirUrl = finalUrl().resolved(QUrl(import.uri + QLatin1String("/qmldir")));
            if (!QQmlImports::isLocal(qmldirUrl)) {
                // This is a remote file; the import is currently incomplete
                incomplete = true;
            }
        }

        if (!m_imports.addFileImport(importDatabase, import.uri, import.qualifier, import.majorVersion,
                                   import.minorVersion, incomplete, errors))
            return false;

        if (incomplete) {
            if (!fetchQmldir(qmldirUrl, &import, 1, errors))
                return false;
        }
    }

    return true;
}

void QQmlTypeLoader::Blob::dependencyError(QQmlDataBlob *blob)
{
    if (blob->type() == QQmlDataBlob::QmldirFile) {
        QQmlQmldirData *data = static_cast<QQmlQmldirData *>(blob);
        data->release();
    }
}

void QQmlTypeLoader::Blob::dependencyComplete(QQmlDataBlob *blob)
{
    if (blob->type() == QQmlDataBlob::QmldirFile) {
        QQmlQmldirData *data = static_cast<QQmlQmldirData *>(blob);

        const QQmlScript::Import *import = data->import();

        QList<QQmlError> errors;
        if (!qmldirDataAvailable(data, &errors)) {
            Q_ASSERT(errors.size());
            QQmlError error(errors.takeFirst());
            error.setUrl(m_imports.baseUrl());
            error.setLine(import->location.start.line);
            error.setColumn(import->location.start.column);
            errors.prepend(error); // put it back on the list after filling out information.
            setError(errors);
        }
    }
}

bool QQmlTypeLoader::Blob::qmldirDataAvailable(QQmlQmldirData *data, QList<QQmlError> *errors)
{
    bool resolve = true;

    const QQmlScript::Import *import = data->import();
    data->setImport(0);

    int priority = data->priority();
    data->setPriority(0);

    if (import) {
        // Do we need to resolve this import?
        QHash<const QQmlScript::Import *, int>::iterator it = m_unresolvedImports.find(import);
        if (it != m_unresolvedImports.end()) {
            resolve = (*it == 0) || (*it > priority);
        }

        if (resolve) {
            // This is the (current) best resolution for this import
            if (!updateQmldir(data, import, errors)) {
                data->release();
                return false;
            }

            *it = priority;
            return true;
        }
    }

    data->release();
    return true;
}


QQmlTypeLoader::QmldirContent::QmldirContent()
{
}

bool QQmlTypeLoader::QmldirContent::hasError() const
{
    return m_parser.hasError();
}

QList<QQmlError> QQmlTypeLoader::QmldirContent::errors(const QString &uri) const
{
    return m_parser.errors(uri);
}

QString QQmlTypeLoader::QmldirContent::typeNamespace() const
{
    return m_parser.typeNamespace();
}

void QQmlTypeLoader::QmldirContent::setContent(const QString &location, const QString &content)
{
    m_location = location;
    m_parser.parse(content);
}

void QQmlTypeLoader::QmldirContent::setError(const QQmlError &error)
{
    m_parser.setError(error);
}

QQmlDirComponents QQmlTypeLoader::QmldirContent::components() const
{
    return m_parser.components();
}

QQmlDirScripts QQmlTypeLoader::QmldirContent::scripts() const
{
    return m_parser.scripts();
}

QQmlDirPlugins QQmlTypeLoader::QmldirContent::plugins() const
{
    return m_parser.plugins();
}

QString QQmlTypeLoader::QmldirContent::pluginLocation() const
{
    return m_location;
}


/*!
Constructs a new type loader that uses the given \a engine.
*/
QQmlTypeLoader::QQmlTypeLoader(QQmlEngine *engine)
: QQmlDataLoader(engine)
{
}

/*!
Destroys the type loader, first clearing the cache of any information about
loaded files.
*/
QQmlTypeLoader::~QQmlTypeLoader()
{
    // Stop the loader thread before releasing resources
    shutdownThread();

    clearCache();
}

QQmlImportDatabase *QQmlTypeLoader::importDatabase()
{
    return &QQmlEnginePrivate::get(engine())->importDatabase;
}

/*!
\enum QQmlTypeLoader::Option

This enum defines the options that control the way type data is handled.

\value None             The default value, indicating that no other options
                        are enabled.
\value PreserveParser   The parser used to handle the type data is preserved
                        after the data has been parsed.
*/

/*!
Returns a QQmlTypeData for the specified \a url.  The QQmlTypeData may be cached.
*/
QQmlTypeData *QQmlTypeLoader::getType(const QUrl &url, Mode mode)
{
    Q_ASSERT(!url.isRelative() && 
            (QQmlFile::urlToLocalFileOrQrc(url).isEmpty() ||
             !QDir::isRelativePath(QQmlFile::urlToLocalFileOrQrc(url))));

    LockHolder<QQmlTypeLoader> holder(this);
    
    QQmlTypeData *typeData = m_typeCache.value(url);

    if (!typeData) {
        typeData = new QQmlTypeData(url, None, this);
        // TODO: if (compiledData == 0), is it safe to omit this insertion?
        m_typeCache.insert(url, typeData);
        QQmlDataLoader::load(typeData, mode);
    }

    typeData->addref();

    return typeData;
}

/*!
Returns a QQmlTypeData for the given \a data with the provided base \a url.  The 
QQmlTypeData will not be cached.

The specified \a options control how the loader handles type data.
*/
QQmlTypeData *QQmlTypeLoader::getType(const QByteArray &data, const QUrl &url, Options options)
{
    LockHolder<QQmlTypeLoader> holder(this);

    QQmlTypeData *typeData = new QQmlTypeData(url, options, this);
    QQmlDataLoader::loadWithStaticData(typeData, data);

    return typeData;
}

/*!
Return a QQmlScriptBlob for \a url.  The QQmlScriptData may be cached.
*/
QQmlScriptBlob *QQmlTypeLoader::getScript(const QUrl &url)
{
    Q_ASSERT(!url.isRelative() && 
            (QQmlFile::urlToLocalFileOrQrc(url).isEmpty() ||
             !QDir::isRelativePath(QQmlFile::urlToLocalFileOrQrc(url))));

    LockHolder<QQmlTypeLoader> holder(this);

    QQmlScriptBlob *scriptBlob = m_scriptCache.value(url);

    if (!scriptBlob) {
        scriptBlob = new QQmlScriptBlob(url, this);
        m_scriptCache.insert(url, scriptBlob);
        QQmlDataLoader::load(scriptBlob);
    }

    scriptBlob->addref();

    return scriptBlob;
}

/*!
Returns a QQmlQmldirData for \a url.  The QQmlQmldirData may be cached.
*/
QQmlQmldirData *QQmlTypeLoader::getQmldir(const QUrl &url)
{
    Q_ASSERT(!url.isRelative() && 
            (QQmlFile::urlToLocalFileOrQrc(url).isEmpty() ||
             !QDir::isRelativePath(QQmlFile::urlToLocalFileOrQrc(url))));

    LockHolder<QQmlTypeLoader> holder(this);

    QQmlQmldirData *qmldirData = m_qmldirCache.value(url);

    if (!qmldirData) {
        qmldirData = new QQmlQmldirData(url, this);
        m_qmldirCache.insert(url, qmldirData);
        QQmlDataLoader::load(qmldirData);
    }

    qmldirData->addref();

    return qmldirData;
}

/*!
Returns a QQmlBundleData for \a identifier.
*/
QQmlBundleData *QQmlTypeLoader::getBundle(const QString &identifier)
{
    return getBundle(QHashedStringRef(identifier));
}

QQmlBundleData *QQmlTypeLoader::getBundle(const QHashedStringRef &identifier)
{
    lock();

    QQmlBundleData *rv = 0;
    QQmlBundleData **bundle = m_bundleCache.value(identifier);
    if (bundle) {
        rv = *bundle;
        rv->addref();
    }

    unlock();

    return rv;
}

QQmlBundleData::QQmlBundleData(const QString &file)
: QQmlBundle(file), fileName(file)
{
}

// XXX check for errors etc.
void QQmlTypeLoader::addBundle(const QString &identifier, const QString &fileName)
{
    lock();
    addBundleNoLock(identifier, fileName);
    unlock();
}

void QQmlTypeLoader::addBundleNoLock(const QString &identifier, const QString &fileName)
{
    QQmlBundleData *data = new QQmlBundleData(fileName);
    if (data->open()) {

        m_bundleCache.insert(identifier, data);

    } else {
        data->release();
    }
}

QString QQmlTypeLoader::bundleIdForQmldir(const QString &name, const QString &uriHint)
{
    lock();
    QString *bundleId = m_qmldirBundleIdCache.value(name);
    if (!bundleId) {
        QString newBundleId = QLatin1String("qml.") + uriHint.toLower() /* XXX toLower()? */;
        if (m_qmldirBundleIdCache.contains(newBundleId))
            newBundleId += QString::number(m_qmldirBundleIdCache.count());
        m_qmldirBundleIdCache.insert(name, newBundleId);
        addBundleNoLock(newBundleId, name);
        unlock();
        return newBundleId;
    } else {
        unlock();
        return *bundleId;
    }
}

bool QQmlEngine::addNamedBundle(const QString &name, const QString &fileName)
{
    Q_D(QQmlEngine);

    if (name.startsWith(QLatin1String("qml."))) // reserved
        return false;

    d->typeLoader.addBundle(name, fileName);
    return true;
}

/*!
Returns the absolute filename of path via a directory cache.
Returns a empty string if the path does not exist.

Why a directory cache?  QML checks for files in many paths with
invalid directories.  By caching whether a directory exists
we avoid many stats.  We also cache the files' existence in the
directory, for the same reason.
*/
QString QQmlTypeLoader::absoluteFilePath(const QString &path)
{
    if (path.isEmpty())
        return QString();
    if (path.at(0) == QLatin1Char(':')) {
        // qrc resource
        QFileInfo fileInfo(path);
        return fileInfo.isFile() ? fileInfo.absoluteFilePath() : QString();
    } else if (path.count() > 3 && path.at(3) == QLatin1Char(':') &&
               path.startsWith(QLatin1String("qrc"), Qt::CaseInsensitive)) {
        // qrc resource url
        QFileInfo fileInfo(QQmlFile::urlToLocalFileOrQrc(path));
        return fileInfo.isFile() ? fileInfo.absoluteFilePath() : QString();
    }
#if defined(Q_OS_ANDROID)
    else if (path.count() > 7 && path.at(6) == QLatin1Char(':') && path.at(7) == QLatin1Char('/') &&
           path.startsWith(QLatin1String("assets"), Qt::CaseInsensitive)) {
        // assets resource url
        QFileInfo fileInfo(QQmlFile::urlToLocalFileOrQrc(path));
        return fileInfo.isFile() ? fileInfo.absoluteFilePath() : QString();
    }
#endif

    int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    QStringRef dirPath(&path, 0, lastSlash);

    StringSet **fileSet = m_importDirCache.value(QHashedStringRef(dirPath.constData(), dirPath.length()));
    if (!fileSet) {
        QHashedString dirPathString(dirPath.toString());
        bool exists = false;
#ifdef Q_OS_UNIX
        struct stat statBuf;
        if (::stat(QFile::encodeName(dirPathString).constData(), &statBuf) == 0)
            exists = S_ISDIR(statBuf.st_mode);
#else
        exists = QDir(dirPathString).exists();
#endif
        QStringHash<bool> *files = exists ? new QStringHash<bool> : 0;
        m_importDirCache.insert(dirPathString, files);
        fileSet = m_importDirCache.value(dirPathString);
    }
    if (!(*fileSet))
        return QString();

    QString absoluteFilePath;
    QHashedStringRef fileName(path.constData()+lastSlash+1, path.length()-lastSlash-1);

    bool *value = (*fileSet)->value(fileName);
    if (value) {
        if (*value)
            absoluteFilePath = path;
    } else {
        bool exists = false;
#ifdef Q_OS_UNIX
        struct stat statBuf;
        // XXX Avoid encoding entire path. Should store encoded dirpath in cache
        if (::stat(QFile::encodeName(path).constData(), &statBuf) == 0)
            exists = S_ISREG(statBuf.st_mode);
#else
        exists = QFile::exists(path);
#endif
        (*fileSet)->insert(fileName.toString(), exists);
        if (exists)
            absoluteFilePath = path;
    }

    if (absoluteFilePath.length() > 2 && absoluteFilePath.at(0) != QLatin1Char('/') && absoluteFilePath.at(1) != QLatin1Char(':'))
        absoluteFilePath = QFileInfo(absoluteFilePath).absoluteFilePath();

    return absoluteFilePath;
}


/*!
Returns true if the path is a directory via a directory cache.  Cache is
shared with absoluteFilePath().
*/
bool QQmlTypeLoader::directoryExists(const QString &path)
{
    if (path.isEmpty())
        return false;

    bool isResource = path.at(0) == QLatin1Char(':');
#if defined(Q_OS_ANDROID)
    isResource = isResource || path.startsWith(QLatin1String("assets:/"));
#endif

    if (isResource) {
        // qrc resource
        QFileInfo fileInfo(path);
        return fileInfo.exists() && fileInfo.isDir();
    }

    int length = path.length();
    if (path.endsWith(QLatin1Char('/')))
        --length;
    QStringRef dirPath(&path, 0, length);

    StringSet **fileSet = m_importDirCache.value(QHashedStringRef(dirPath.constData(), dirPath.length()));
    if (!fileSet) {
        QHashedString dirPathString(dirPath.toString());
        bool exists = false;
#ifdef Q_OS_UNIX
        struct stat statBuf;
        if (::stat(QFile::encodeName(dirPathString).constData(), &statBuf) == 0)
            exists = S_ISDIR(statBuf.st_mode);
#else
        exists = QDir(dirPathString).exists();
#endif
        QStringHash<bool> *files = exists ? new QStringHash<bool> : 0;
        m_importDirCache.insert(dirPathString, files);
        fileSet = m_importDirCache.value(dirPathString);
    }

    return (*fileSet);
}


/*!
Return a QmldirContent for absoluteFilePath.  The QmldirContent may be cached.

\a filePath is either a bundle URL, or a local file path.
*/
const QQmlTypeLoader::QmldirContent *QQmlTypeLoader::qmldirContent(const QString &filePath, const QString &uriHint)
{
    QmldirContent *qmldir;
    QmldirContent **val = m_importQmlDirCache.value(filePath);
    if (!val) {
        qmldir = new QmldirContent;

#define ERROR(description) { QQmlError e; e.setDescription(description); qmldir->setError(e); }
#define NOT_READABLE_ERROR QString(QLatin1String("module \"$$URI$$\" definition \"%1\" not readable"))
#define CASE_MISMATCH_ERROR QString(QLatin1String("cannot load module \"$$URI$$\": File name case mismatch for \"%1\""))

        if (QQmlFile::isBundle(filePath)) {

            QUrl url(filePath);

            QQmlFile file(engine(), url);
            if (file.isError()) {
                ERROR(NOT_READABLE_ERROR.arg(filePath));
            } else {
                QString content(QString::fromUtf8(file.data(), file.size()));
                qmldir->setContent(filePath, content);
            }

        } else {

            QFile file(filePath);
            if (!QQml_isFileCaseCorrect(filePath)) {
                ERROR(CASE_MISMATCH_ERROR.arg(filePath));
            } else if (file.open(QFile::ReadOnly)) {
                QByteArray data = file.read(QQmlBundle::bundleHeaderLength());

                if (QQmlBundle::isBundleHeader(data.constData(), data.length())) {
                    QString id = bundleIdForQmldir(filePath, uriHint);

                    QString bundleUrl = QLatin1String("bundle://") + id + QLatin1Char('/');

                    QUrl url(bundleUrl + QLatin1String("qmldir"));

                    QQmlFile file(engine(), url);
                    if (file.isError()) {
                        ERROR(NOT_READABLE_ERROR.arg(filePath));
                    } else {
                        QString content(QString::fromUtf8(file.data(), file.size()));
                        qmldir->setContent(QQmlFile::bundleFileName(bundleUrl, engine()), content);
                    }
                } else {
                    data += file.readAll();
                    qmldir->setContent(filePath, QString::fromUtf8(data));
                }
            } else {
                ERROR(NOT_READABLE_ERROR.arg(filePath));
            }

        }

#undef ERROR
#undef NOT_READABLE_ERROR
#undef CASE_MISMATCH_ERROR

        m_importQmlDirCache.insert(filePath, qmldir);
    } else {
        qmldir = *val;
    }

    return qmldir;
}

void QQmlTypeLoader::setQmldirContent(const QString &url, const QString &content)
{
    QmldirContent *qmldir;
    QmldirContent **val = m_importQmlDirCache.value(url);
    if (val) {
        qmldir = *val;
    } else {
        qmldir = new QmldirContent;
        m_importQmlDirCache.insert(url, qmldir);
    }

    qmldir->setContent(url, content);
}

/*!
Clears cached information about loaded files, including any type data, scripts
and qmldir information.
*/
void QQmlTypeLoader::clearCache()
{
    for (TypeCache::Iterator iter = m_typeCache.begin(); iter != m_typeCache.end(); ++iter)
        (*iter)->release();
    for (ScriptCache::Iterator iter = m_scriptCache.begin(); iter != m_scriptCache.end(); ++iter) 
        (*iter)->release();
    for (QmldirCache::Iterator iter = m_qmldirCache.begin(); iter != m_qmldirCache.end(); ++iter)
        (*iter)->release();
    qDeleteAll(m_importDirCache);
    qDeleteAll(m_importQmlDirCache);

    m_typeCache.clear();
    m_scriptCache.clear();
    m_qmldirCache.clear();
    m_importDirCache.clear();
    m_importQmlDirCache.clear();
}

void QQmlTypeLoader::trimCache()
{
    while (true) {
        QList<TypeCache::Iterator> unneededTypes;
        for (TypeCache::Iterator iter = m_typeCache.begin(); iter != m_typeCache.end(); ++iter)  {
            QQmlTypeData *typeData = iter.value();
            if (typeData->m_compiledData && typeData->m_compiledData->count() == 1) {
                // There are no live objects of this type
                unneededTypes.append(iter);
            }
        }

        if (unneededTypes.isEmpty())
            break;

        while (!unneededTypes.isEmpty()) {
            TypeCache::Iterator iter = unneededTypes.last();
            unneededTypes.removeLast();

            iter.value()->release();
            m_typeCache.erase(iter);
        }
    }

    // TODO: release any scripts which are no longer referenced by any types
}

bool QQmlTypeLoader::isTypeLoaded(const QUrl &url) const
{
    LockHolder<QQmlTypeLoader> holder(const_cast<QQmlTypeLoader *>(this));
    return m_typeCache.contains(url);
}

bool QQmlTypeLoader::isScriptLoaded(const QUrl &url) const
{
    LockHolder<QQmlTypeLoader> holder(const_cast<QQmlTypeLoader *>(this));
    return m_scriptCache.contains(url);
}

QQmlTypeData::TypeDataCallback::~TypeDataCallback()
{
}

QQmlTypeData::QQmlTypeData(const QUrl &url, QQmlTypeLoader::Options options, 
                                           QQmlTypeLoader *manager)
: QQmlTypeLoader::Blob(url, QmlFile, manager), m_options(options),
   m_typesResolved(false), m_compiledData(0), m_implicitImport(0), m_implicitImportLoaded(false)
{
}

QQmlTypeData::~QQmlTypeData()
{
    for (int ii = 0; ii < m_scripts.count(); ++ii) 
        m_scripts.at(ii).script->release();
    for (int ii = 0; ii < m_types.count(); ++ii) 
        if (m_types.at(ii).typeData) m_types.at(ii).typeData->release();
    if (m_compiledData)
        m_compiledData->release();
    delete m_implicitImport;
}

const QQmlScript::Parser &QQmlTypeData::parser() const
{
    return scriptParser;
}

const QList<QQmlTypeData::TypeReference> &QQmlTypeData::resolvedTypes() const
{
    return m_types;
}

const QList<QQmlTypeData::ScriptReference> &QQmlTypeData::resolvedScripts() const
{
    return m_scripts;
}

const QSet<QString> &QQmlTypeData::namespaces() const
{
    return m_namespaces;
}

QQmlCompiledData *QQmlTypeData::compiledData() const
{
    return m_compiledData;
}

void QQmlTypeData::registerCallback(TypeDataCallback *callback)
{
    Q_ASSERT(!m_callbacks.contains(callback));
    m_callbacks.append(callback);
}

void QQmlTypeData::unregisterCallback(TypeDataCallback *callback)
{
    Q_ASSERT(m_callbacks.contains(callback));
    m_callbacks.removeOne(callback);
    Q_ASSERT(!m_callbacks.contains(callback));
}

void QQmlTypeData::done()
{
    // Check all script dependencies for errors
    for (int ii = 0; !isError() && ii < m_scripts.count(); ++ii) {
        const ScriptReference &script = m_scripts.at(ii);
        Q_ASSERT(script.script->isCompleteOrError());
        if (script.script->isError()) {
            QList<QQmlError> errors = script.script->errors();
            QQmlError error;
            error.setUrl(finalUrl());
            error.setLine(script.location.line);
            error.setColumn(script.location.column);
            error.setDescription(QQmlTypeLoader::tr("Script %1 unavailable").arg(script.script->url().toString()));
            errors.prepend(error);
            setError(errors);
        }
    }

    // Check all type dependencies for errors
    for (int ii = 0; !isError() && ii < m_types.count(); ++ii) {
        const TypeReference &type = m_types.at(ii);
        Q_ASSERT(!type.typeData || type.typeData->isCompleteOrError());
        if (type.typeData && type.typeData->isError()) {
            QString typeName = scriptParser.referencedTypes().at(ii)->name;

            QList<QQmlError> errors = type.typeData->errors();
            QQmlError error;
            error.setUrl(finalUrl());
            error.setLine(type.location.line);
            error.setColumn(type.location.column);
            error.setDescription(QQmlTypeLoader::tr("Type %1 unavailable").arg(typeName));
            errors.prepend(error);
            setError(errors);
        }
    }

    // Compile component
    if (!isError()) 
        compile();

    if (!(m_options & QQmlTypeLoader::PreserveParser))
        scriptParser.clear();
}

void QQmlTypeData::completed()
{
    // Notify callbacks
    while (!m_callbacks.isEmpty()) {
        TypeDataCallback *callback = m_callbacks.takeFirst();
        callback->typeDataReady(this);
    }
}

bool QQmlTypeData::loadImplicitImport()
{
    m_implicitImportLoaded = true; // Even if we hit an error, count as loaded (we'd just keep hitting the error)

    m_imports.setBaseUrl(finalUrl(), finalUrlString());

    QQmlImportDatabase *importDatabase = typeLoader()->importDatabase();
    // For local urls, add an implicit import "." as most overridden lookup.
    // This will also trigger the loading of the qmldir and the import of any native
    // types from available plugins.
    QList<QQmlError> implicitImportErrors;
    m_imports.addImplicitImport(importDatabase, &implicitImportErrors);

    if (!implicitImportErrors.isEmpty()) {
        setError(implicitImportErrors);
        return false;
    }

    return true;
}

void QQmlTypeData::dataReceived(const Data &data)
{
    QString code = QString::fromUtf8(data.data(), data.size());
    QByteArray preparseData;

    if (data.isFile()) preparseData = data.asFile()->metaData(QLatin1String("qml:preparse"));

    if (!scriptParser.parse(code, preparseData, finalUrl(), finalUrlString())) {
        setError(scriptParser.errors());
        return;
    }

    m_imports.setBaseUrl(finalUrl(), finalUrlString());

    // For remote URLs, we don't delay the loading of the implicit import
    // because the loading probably requires an asynchronous fetch of the
    // qmldir (so we can't load it just in time).
    if (!finalUrl().scheme().isEmpty()) {
        QUrl qmldirUrl = finalUrl().resolved(QUrl(QLatin1String("qmldir")));
        if (!QQmlImports::isLocal(qmldirUrl)) {
            if (!loadImplicitImport())
                return;
            // This qmldir is for the implicit import
            m_implicitImport = new QQmlScript::Import;
            m_implicitImport->uri = QLatin1String(".");
            m_implicitImport->qualifier = QString();
            m_implicitImport->majorVersion = -1;
            m_implicitImport->minorVersion = -1;
            QList<QQmlError> errors;

            if (!fetchQmldir(qmldirUrl, m_implicitImport, 1, &errors)) {
                setError(errors);
                return;
            }
        }
    }

    QList<QQmlError> errors;

    foreach (const QQmlScript::Import &import, scriptParser.imports()) {
        if (!addImport(import, &errors)) {
            Q_ASSERT(errors.size());
            QQmlError error(errors.takeFirst());
            error.setUrl(m_imports.baseUrl());
            error.setLine(import.location.start.line);
            error.setColumn(import.location.start.column);
            errors.prepend(error); // put it back on the list after filling out information.
            setError(errors);
            return;
        }
    }
}

void QQmlTypeData::allDependenciesDone()
{
    if (!m_typesResolved) {
        // Check that all imports were resolved
        QList<QQmlError> errors;
        QHash<const QQmlScript::Import *, int>::const_iterator it = m_unresolvedImports.constBegin(), end = m_unresolvedImports.constEnd();
        for ( ; it != end; ++it) {
            if (*it == 0) {
                // This import was not resolved
                foreach (const QQmlScript::Import *import, m_unresolvedImports.keys()) {
                    QQmlError error;
                    error.setDescription(QQmlTypeLoader::tr("module \"%1\" is not installed").arg(import->uri));
                    error.setUrl(m_imports.baseUrl());
                    error.setLine(import->location.start.line);
                    error.setColumn(import->location.start.column);
                    errors.prepend(error);
                }
            }
        }
        if (errors.size()) {
            setError(errors);
            return;
        }

        resolveTypes();
        m_typesResolved = true;
    }
}

void QQmlTypeData::downloadProgressChanged(qreal p)
{
    for (int ii = 0; ii < m_callbacks.count(); ++ii) {
        TypeDataCallback *callback = m_callbacks.at(ii);
        callback->typeDataProgress(this, p);
    }
}

void QQmlTypeData::compile()
{
    Q_ASSERT(m_compiledData == 0);

    m_compiledData = new QQmlCompiledData(typeLoader()->engine());
    m_compiledData->url = finalUrl();
    m_compiledData->name = finalUrlString();

    QQmlCompilingProfiler prof(m_compiledData->name);

    QQmlCompiler compiler(&scriptParser._pool);
    if (!compiler.compile(typeLoader()->engine(), this, m_compiledData)) {
        setError(compiler.errors());
        m_compiledData->release();
        m_compiledData = 0;
    }
}

void QQmlTypeData::resolveTypes()
{
    // Add any imported scripts to our resolved set
    foreach (const QQmlImports::ScriptReference &script, m_imports.resolvedScripts())
    {
        QQmlScriptBlob *blob = typeLoader()->getScript(script.location);
        addDependency(blob);

        ScriptReference ref;
        //ref.location = ...
        ref.qualifier = script.nameSpace;
        if (!script.qualifier.isEmpty())
        {
            ref.qualifier.prepend(script.qualifier + QLatin1Char('.'));

            // Add a reference to the enclosing namespace
            m_namespaces.insert(script.qualifier);
        }

        ref.script = blob;
        m_scripts << ref;
    }

    foreach (QQmlScript::TypeReference *parserRef, scriptParser.referencedTypes()) {
        TypeReference ref;

        QString url;
        int majorVersion = -1;
        int minorVersion = -1;
        QQmlImportNamespace *typeNamespace = 0;
        QList<QQmlError> errors;

        bool typeFound = m_imports.resolveType(parserRef->name, &ref.type,
                &majorVersion, &minorVersion, &typeNamespace, &errors);
        if (!typeNamespace && !typeFound && !m_implicitImportLoaded) {
            // Lazy loading of implicit import
            if (loadImplicitImport()) {
                // Try again to find the type
                errors.clear();
                typeFound = m_imports.resolveType(parserRef->name, &ref.type,
                    &majorVersion, &minorVersion, &typeNamespace, &errors);
            } else {
                return; //loadImplicitImport() hit an error, and called setError already
            }
        }

        if (!typeFound || typeNamespace) {
            // Known to not be a type:
            //  - known to be a namespace (Namespace {})
            //  - type with unknown namespace (UnknownNamespace.SomeType {})
            QQmlError error;
            if (typeNamespace) {
                error.setDescription(QQmlTypeLoader::tr("Namespace %1 cannot be used as a type").arg(parserRef->name));
            } else {
                if (errors.size()) {
                    error = errors.takeFirst();
                } else {
                    // this should not be possible!
                    // Description should come from error provided by addImport() function.
                    error.setDescription(QQmlTypeLoader::tr("Unreported error adding script import to import database"));
                }
                error.setUrl(m_imports.baseUrl());
                error.setDescription(QQmlTypeLoader::tr("%1 %2").arg(parserRef->name).arg(error.description()));
            }

            Q_ASSERT(parserRef->firstUse);
            error.setLine(parserRef->firstUse->location.start.line);
            error.setColumn(parserRef->firstUse->location.start.column);

            errors.prepend(error);
            setError(errors);
            return;
        }

        if (ref.type->isComposite()) {
            ref.typeData = typeLoader()->getType(ref.type->sourceUrl());
            addDependency(ref.typeData);
        }
        ref.majorVersion = majorVersion;
        ref.minorVersion = minorVersion;

        Q_ASSERT(parserRef->firstUse);
        ref.location = parserRef->firstUse->location.start;

        m_types << ref;
    }
}

void QQmlTypeData::scriptImported(QQmlScriptBlob *blob, const QQmlScript::Location &location, const QString &qualifier, const QString &/*nameSpace*/)
{
    ScriptReference ref;
    ref.script = blob;
    ref.location = location;
    ref.qualifier = qualifier;

    m_scripts << ref;
}

QQmlScriptData::QQmlScriptData()
: importCache(0), pragmas(QQmlScript::Object::ScriptBlock::None), m_loaded(false) 
{
}

QQmlScriptData::~QQmlScriptData()
{
}

void QQmlScriptData::clear()
{
    if (importCache) {
        importCache->release();
        importCache = 0;
    }

    for (int ii = 0; ii < scripts.count(); ++ii)
        scripts.at(ii)->release();
    scripts.clear();

    qPersistentDispose(m_program);
    qPersistentDispose(m_value);

    // An addref() was made when the QQmlCleanup was added to the engine.
    release();
}

QQmlScriptBlob::QQmlScriptBlob(const QUrl &url, QQmlTypeLoader *loader)
: QQmlTypeLoader::Blob(url, JavaScriptFile, loader), m_scriptData(0)
{
}

QQmlScriptBlob::~QQmlScriptBlob()
{
    if (m_scriptData) {
        m_scriptData->release();
        m_scriptData = 0;
    }
}

QQmlScript::Object::ScriptBlock::Pragmas QQmlScriptBlob::pragmas() const
{
    return m_metadata.pragmas;
}

QQmlScriptData *QQmlScriptBlob::scriptData() const
{
    return m_scriptData;
}

void QQmlScriptBlob::dataReceived(const Data &data)
{
    m_source = QString::fromUtf8(data.data(), data.size());

    m_scriptData = new QQmlScriptData();
    m_scriptData->url = finalUrl();
    m_scriptData->urlString = finalUrlString();

    QQmlError metaDataError;
    m_metadata = QQmlScript::Parser::extractMetaData(m_source, &metaDataError);
    if (metaDataError.isValid()) {
        metaDataError.setUrl(finalUrl());
        m_scriptData->setError(metaDataError);
    }

    m_imports.setBaseUrl(finalUrl(), finalUrlString());

    QList<QQmlError> errors;

    foreach (const QQmlScript::Import &import, m_metadata.imports) {
        if (!addImport(import, &errors)) {
            Q_ASSERT(errors.size());
            QQmlError error(errors.takeFirst());
            error.setUrl(m_imports.baseUrl());
            error.setLine(import.location.start.line);
            error.setColumn(import.location.start.column);
            errors.prepend(error); // put it back on the list after filling out information.
            setError(errors);
            return;
        }
    }
}

void QQmlScriptBlob::done()
{
    // Check all script dependencies for errors
    for (int ii = 0; !isError() && ii < m_scripts.count(); ++ii) {
        const ScriptReference &script = m_scripts.at(ii);
        Q_ASSERT(script.script->isCompleteOrError());
        if (script.script->isError()) {
            QList<QQmlError> errors = script.script->errors();
            QQmlError error;
            error.setUrl(finalUrl());
            error.setLine(script.location.line);
            error.setColumn(script.location.column);
            error.setDescription(QQmlTypeLoader::tr("Script %1 unavailable").arg(script.script->url().toString()));
            errors.prepend(error);
            setError(errors);
        }
    }

    if (isError())
        return;

    m_scriptData->importCache = new QQmlTypeNameCache();

    QSet<QString> ns;

    for (int scriptIndex = 0; !isError() && scriptIndex < m_scripts.count(); ++scriptIndex) {
        const ScriptReference &script = m_scripts.at(scriptIndex);

        m_scriptData->scripts.append(script.script);

        if (!script.nameSpace.isNull()) {
            if (!ns.contains(script.nameSpace)) {
                ns.insert(script.nameSpace);
                m_scriptData->importCache->add(script.nameSpace);
            }
        }
        m_scriptData->importCache->add(script.qualifier, scriptIndex, script.nameSpace);
    }

    m_imports.populateCache(m_scriptData->importCache);

    m_scriptData->pragmas = m_metadata.pragmas;
    m_scriptData->m_programSource = m_source.toUtf8();
    m_source.clear();
}

void QQmlScriptBlob::scriptImported(QQmlScriptBlob *blob, const QQmlScript::Location &location, const QString &qualifier, const QString &nameSpace)
{
    ScriptReference ref;
    ref.script = blob;
    ref.location = location;
    ref.qualifier = qualifier;
    ref.nameSpace = nameSpace;

    m_scripts << ref;
}

QQmlQmldirData::QQmlQmldirData(const QUrl &url, QQmlTypeLoader *loader)
: QQmlTypeLoader::Blob(url, QmldirFile, loader), m_import(0), m_priority(0)
{
}

const QString &QQmlQmldirData::content() const
{
    return m_content;
}

const QQmlScript::Import *QQmlQmldirData::import() const
{
    return m_import;
}

void QQmlQmldirData::setImport(const QQmlScript::Import *import)
{
    m_import = import;
}

int QQmlQmldirData::priority() const
{
    return m_priority;
}

void QQmlQmldirData::setPriority(int priority)
{
    m_priority = priority;
}

void QQmlQmldirData::dataReceived(const Data &data)
{
    m_content = QString::fromUtf8(data.data(), data.size());
}

QT_END_NAMESPACE

#include "qqmltypeloader.moc"
