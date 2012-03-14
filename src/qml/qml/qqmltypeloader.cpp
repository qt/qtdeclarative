/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qqmltypeloader_p.h"

#include <private/qqmlengine_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlthread_p.h>
#include <private/qqmlcompiler_p.h>
#include <private/qqmlcomponent_p.h>
#include <private/qqmlprofilerservice_p.h>

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmutex.h>
#include <QtCore/qthread.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qwaitcondition.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/qqmlextensioninterface.h>

#if defined (Q_OS_UNIX)
#include <sys/types.h>
#include <dirent.h>
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

QT_BEGIN_NAMESPACE

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

/*
Returns the set of QML files in path (qmldir, *.qml, *.js).  The caller
is responsible for deleting the returned data.
Returns 0 if the directory does not exist.
*/
#if defined (Q_OS_UNIX) && !defined(Q_OS_DARWIN)
static QStringHash<bool> *qmlFilesInDirectory(const QString &path)
{
    QByteArray name(QFile::encodeName(path));
    DIR *dd = opendir(name);
    if (!dd)
        return 0;

    struct dirent *result;
    union {
        struct dirent d;
        char b[offsetof (struct dirent, d_name) + NAME_MAX + 1];
    } u;

    QStringHash<bool> *files = new QStringHash<bool>;
    while (readdir_r(dd, &u.d, &result) == 0 && result != 0) {
        if (!strcmp(u.d.d_name, "qmldir")) {
            files->insert(QLatin1String("qmldir"), true);
            continue;
        }
        int len = strlen(u.d.d_name);
        if (len < 4)
            continue;
        if (!strcmp(u.d.d_name+len-4, ".qml") || !strcmp(u.d.d_name+len-3, ".js"))
            files->insert(QFile::decodeName(u.d.d_name), true);
#if defined(Q_OS_DARWIN)
        else if ((len > 6 && !strcmp(u.d.d_name+len-6, ".dylib")) || !strcmp(u.d.d_name+len-3, ".so")
                  || (len > 7 && !strcmp(u.d.d_name+len-7, ".bundle")))
            files->insert(QFile::decodeName(u.d.d_name), true);
#else  // Unix
        else if (!strcmp(u.d.d_name+len-3, ".so") || !strcmp(u.d.d_name+len-3, ".sl"))
            files->insert(QFile::decodeName(u.d.d_name), true);
#endif
    }

    closedir(dd);
    return files;
}
#else
static QStringHash<bool> *qmlFilesInDirectory(const QString &path)
{
    QDirIterator dir(path, QDir::Files);
    if (!dir.hasNext())
        return 0;
    QStringHash<bool> *files = new QStringHash<bool>;
    while (dir.hasNext()) {
        dir.next();
        QString fileName = dir.fileName();
        if (fileName == QLatin1String("qmldir")
                || fileName.endsWith(QLatin1String(".qml"))
                || fileName.endsWith(QLatin1String(".js"))
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
                || fileName.endsWith(QLatin1String(".dll"))
#elif defined(Q_OS_DARWIN)
                || fileName.endsWith(QLatin1String(".dylib"))
                || fileName.endsWith(QLatin1String(".so"))
                || fileName.endsWith(QLatin1String(".bundle"))
#else  // Unix
                || fileName.endsWith(QLatin1String(".so"))
                || fileName.endsWith(QLatin1String(".sl"))
#endif
                ) {
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE) || defined(Q_OS_DARWIN)
            fileName = fileName.toLower();
#endif
            files->insert(fileName, true);
        }
    }
    return files;
}
#endif


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
\fn void QQmlDataBlob::dataReceived(const QByteArray &data)

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

    if (blob->status() == Error) {
        dependencyError(blob);
    } else if (blob->status() == Complete) {
        dependencyComplete(blob);
    }

    m_waitingFor.removeOne(blob);
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

    m_thread->shutdown();
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

    Q_ASSERT(blob->status() == QQmlDataBlob::Null);
    Q_ASSERT(blob->m_manager == 0);

    blob->m_data.setStatus(QQmlDataBlob::Loading);
    blob->m_manager = this;

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

    Q_ASSERT(blob->status() == QQmlDataBlob::Null);
    Q_ASSERT(blob->m_manager == 0);
    
    blob->m_data.setStatus(QQmlDataBlob::Loading);
    blob->m_manager = this;

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

    if (blob->m_url.isEmpty()) {
        QQmlError error;
        error.setDescription(QLatin1String("Invalid null URL"));
        blob->setError(error);
        return;
    }

    QString lf = QQmlEnginePrivate::urlToLocalFileOrQrc(blob->m_url);

    if (!lf.isEmpty()) {
        if (!QQml_isFileCaseCorrect(lf)) {
            QQmlError error;
            error.setUrl(blob->m_url);
            error.setDescription(QLatin1String("File name case mismatch"));
            blob->setError(error);
            return;
        }
        QFile file(lf);
        if (file.open(QFile::ReadOnly)) {
            QByteArray data = file.readAll();

            blob->m_data.setProgress(0xFF);
            if (blob->m_data.isAsync())
                m_thread->callDownloadProgressChanged(blob, 1.);

            setData(blob, data);
        } else {
            blob->networkError(QNetworkReply::ContentNotFoundError);
        }

    } else {

        QNetworkReply *reply = m_thread->networkAccessManager()->get(QNetworkRequest(blob->m_url));
        QObject *nrp = m_thread->networkReplyProxy();
        QObject::connect(reply, SIGNAL(downloadProgress(qint64,qint64)), 
                         nrp, SLOT(downloadProgress(qint64,qint64)));
        QObject::connect(reply, SIGNAL(finished()), 
                         nrp, SLOT(finished()));
        m_networkReplies.insert(reply, blob);

        blob->addref();
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
    blob->m_inCallback = true;

    blob->dataReceived(data);

    if (!blob->isError() && !blob->isWaiting())
        blob->allDependenciesDone();

    if (blob->status() != QQmlDataBlob::Error) 
        blob->m_data.setStatus(QQmlDataBlob::WaitingForDependencies);

    blob->m_inCallback = false;

    blob->tryDone();
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
    clearCache();
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
QQmlTypeData *QQmlTypeLoader::get(const QUrl &url, Mode mode)
{
    Q_ASSERT(!url.isRelative() && 
            (QQmlEnginePrivate::urlToLocalFileOrQrc(url).isEmpty() || 
             !QDir::isRelativePath(QQmlEnginePrivate::urlToLocalFileOrQrc(url))));

    lock();
    
    QQmlTypeData *typeData = m_typeCache.value(url);

    if (!typeData) {
        typeData = new QQmlTypeData(url, None, this);
        m_typeCache.insert(url, typeData);
        QQmlDataLoader::load(typeData, mode);
    }

    typeData->addref();

    unlock();

    return typeData;
}

/*!
Returns a QQmlTypeData for the given \a data with the provided base \a url.  The 
QQmlTypeData will not be cached.

The specified \a options control how the loader handles type data.
*/
QQmlTypeData *QQmlTypeLoader::get(const QByteArray &data, const QUrl &url, Options options)
{
    lock();

    QQmlTypeData *typeData = new QQmlTypeData(url, options, this);
    QQmlDataLoader::loadWithStaticData(typeData, data);

    unlock();

    return typeData;
}

/*!
Return a QQmlScriptBlob for \a url.  The QQmlScriptData may be cached.
*/
QQmlScriptBlob *QQmlTypeLoader::getScript(const QUrl &url)
{
    Q_ASSERT(!url.isRelative() && 
            (QQmlEnginePrivate::urlToLocalFileOrQrc(url).isEmpty() || 
             !QDir::isRelativePath(QQmlEnginePrivate::urlToLocalFileOrQrc(url))));

    lock();

    QQmlScriptBlob *scriptBlob = m_scriptCache.value(url);

    if (!scriptBlob) {
        scriptBlob = new QQmlScriptBlob(url, this);
        m_scriptCache.insert(url, scriptBlob);
        QQmlDataLoader::load(scriptBlob);
    }

    scriptBlob->addref();

    unlock();

    return scriptBlob;
}

/*!
Returns a QQmlQmldirData for \a url.  The QQmlQmldirData may be cached.
*/
QQmlQmldirData *QQmlTypeLoader::getQmldir(const QUrl &url)
{
    Q_ASSERT(!url.isRelative() && 
            (QQmlEnginePrivate::urlToLocalFileOrQrc(url).isEmpty() || 
             !QDir::isRelativePath(QQmlEnginePrivate::urlToLocalFileOrQrc(url))));

    lock();

    QQmlQmldirData *qmldirData = m_qmldirCache.value(url);

    if (!qmldirData) {
        qmldirData = new QQmlQmldirData(url);
        m_qmldirCache.insert(url, qmldirData);
        QQmlDataLoader::load(qmldirData);
    }

    qmldirData->addref();

    unlock();

    return qmldirData;
}

/*!
Returns the absolute filename of path via a directory cache for files named
"qmldir", "*.qml", "*.js", and plugins.
Returns a empty string if the path does not exist.
*/
QString QQmlTypeLoader::absoluteFilePath(const QString &path)
{
    if (path.isEmpty())
        return QString();
    if (path.at(0) == QLatin1Char(':')) {
        // qrc resource
        QFileInfo fileInfo(path);
        return fileInfo.isFile() ? fileInfo.absoluteFilePath() : QString();
    }
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE) || defined(Q_OS_DARWIN)
    QString lowPath = path.toLower();
    int lastSlash = lowPath.lastIndexOf(QLatin1Char('/'));
    QString dirPath = lowPath.left(lastSlash);
#else
    int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    QStringRef dirPath(&path, 0, lastSlash);
#endif

    StringSet **fileSet = m_importDirCache.value(QHashedStringRef(dirPath.constData(), dirPath.length()));
    if (!fileSet) {
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE) || defined(Q_OS_DARWIN)
        QHashedString dirPathString(dirPath);
#else
        QHashedString dirPathString(dirPath.toString());
#endif
        StringSet *files = qmlFilesInDirectory(dirPathString);
        m_importDirCache.insert(dirPathString, files);
        fileSet = m_importDirCache.value(dirPathString);
    }
    if (!(*fileSet))
        return QString();

#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE) || defined(Q_OS_DARWIN)
    QString absoluteFilePath = (*fileSet)->contains(QHashedStringRef(lowPath.constData()+lastSlash+1, lowPath.length()-lastSlash-1)) ? path : QString();
#else
    QString absoluteFilePath = (*fileSet)->contains(QHashedStringRef(path.constData()+lastSlash+1, path.length()-lastSlash-1)) ? path : QString();
#endif
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
    if (path.at(0) == QLatin1Char(':')) {
        // qrc resource
        QFileInfo fileInfo(path);
        return fileInfo.exists() && fileInfo.isDir();
    }

    int length = path.length();
    if (path.endsWith(QLatin1Char('/')))
        --length;
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE) || defined(Q_OS_DARWIN)
    QString dirPath = path.left(length).toLower();
#else
    QStringRef dirPath(&path, 0, length);
#endif

    StringSet **fileSet = m_importDirCache.value(QHashedStringRef(dirPath.constData(), dirPath.length()));
    if (!fileSet) {
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE) || defined(Q_OS_DARWIN)
        QHashedString dirPathString(dirPath);
#else
        QHashedString dirPathString(dirPath.toString());
#endif
        StringSet *files = qmlFilesInDirectory(dirPathString);
        m_importDirCache.insert(dirPathString, files);
        fileSet = m_importDirCache.value(dirPathString);
    }

    return (*fileSet);
}


/*!
Return a QQmlDirParser for absoluteFilePath.  The QQmlDirParser may be cached.
*/
const QQmlDirParser *QQmlTypeLoader::qmlDirParser(const QString &absoluteFilePath)
{
    QQmlDirParser *qmldirParser;
    QQmlDirParser **val = m_importQmlDirCache.value(absoluteFilePath);
    if (!val) {
        qmldirParser = new QQmlDirParser;
        qmldirParser->setFileSource(absoluteFilePath);
        qmldirParser->setUrl(QUrl::fromLocalFile(absoluteFilePath));
        qmldirParser->parse();
        m_importQmlDirCache.insert(absoluteFilePath, qmldirParser);
    } else {
        qmldirParser = *val;
    }

    return qmldirParser;
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


QQmlTypeData::QQmlTypeData(const QUrl &url, QQmlTypeLoader::Options options, 
                                           QQmlTypeLoader *manager)
: QQmlDataBlob(url, QmlFile), m_options(options), m_imports(manager), m_typesResolved(false),
   m_compiledData(0), m_typeLoader(manager)
{
}

QQmlTypeData::~QQmlTypeData()
{
    for (int ii = 0; ii < m_scripts.count(); ++ii) 
        m_scripts.at(ii).script->release();
    for (int ii = 0; ii < m_qmldirs.count(); ++ii) 
        m_qmldirs.at(ii)->release();
    for (int ii = 0; ii < m_types.count(); ++ii) 
        if (m_types.at(ii).typeData) m_types.at(ii).typeData->release();
    if (m_compiledData)
        m_compiledData->release();
}

QQmlTypeLoader *QQmlTypeData::typeLoader() const
{
    return m_typeLoader;
}

const QQmlImports &QQmlTypeData::imports() const
{
    return m_imports;
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
    if (m_compiledData) 
        m_compiledData->addref();

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

void QQmlTypeData::dataReceived(const QByteArray &data)
{
    if (!scriptParser.parse(data, finalUrl(), finalUrlString())) {
        setError(scriptParser.errors());
        return;
    }

    m_imports.setBaseUrl(finalUrl(), finalUrlString());

    foreach (const QQmlScript::Import &import, scriptParser.imports()) {
        if (import.type == QQmlScript::Import::File && import.qualifier.isEmpty()) {
            QUrl importUrl = finalUrl().resolved(QUrl(import.uri + QLatin1String("/qmldir")));
            if (QQmlEnginePrivate::urlToLocalFileOrQrc(importUrl).isEmpty()) {
                QQmlQmldirData *data = typeLoader()->getQmldir(importUrl);
                addDependency(data);
                m_qmldirs << data;
            }
        } else if (import.type == QQmlScript::Import::Script) {
            QUrl scriptUrl = finalUrl().resolved(QUrl(import.uri));
            QQmlScriptBlob *blob = typeLoader()->getScript(scriptUrl);
            addDependency(blob);

            ScriptReference ref;
            ref.location = import.location.start;
            ref.qualifier = import.qualifier;
            ref.script = blob;
            m_scripts << ref;
        }
    }

    if (!finalUrl().scheme().isEmpty()) {
        QUrl importUrl = finalUrl().resolved(QUrl(QLatin1String("qmldir")));
        if (QQmlEnginePrivate::urlToLocalFileOrQrc(importUrl).isEmpty()) {
            QQmlQmldirData *data = typeLoader()->getQmldir(importUrl);
            addDependency(data);
            m_qmldirs << data;
        }
    }
}

void QQmlTypeData::allDependenciesDone()
{
    if (!m_typesResolved) {
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
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(m_typeLoader->engine());
    QQmlImportDatabase *importDatabase = &ep->importDatabase;

    // For local urls, add an implicit import "." as first (most overridden) lookup. 
    // This will also trigger the loading of the qmldir and the import of any native 
    // types from available plugins.
    QList<QQmlError> errors;
    if (QQmlQmldirData *qmldir = qmldirForUrl(finalUrl().resolved(QUrl(QLatin1String("./qmldir"))))) {
        m_imports.addImport(importDatabase, QLatin1String("."),
                            QString(), -1, -1, QQmlScript::Import::File, 
                            qmldir->dirComponents(), &errors);
    } else {
        m_imports.addImport(importDatabase, QLatin1String("."), 
                            QString(), -1, -1, QQmlScript::Import::File, 
                            QQmlDirComponents(), &errors);
    }

    // remove any errors which are due to the implicit import which aren't real errors.
    // for example, if the implicitly included qmldir file doesn't exist, that is not an error.
    QList<QQmlError> realErrors;
    for (int i = 0; i < errors.size(); ++i) {
        if (errors.at(i).description() != QQmlImportDatabase::tr("import \".\" has no qmldir and no namespace")
                && errors.at(i).description() != QQmlImportDatabase::tr("\".\": no such directory")) {
            realErrors.prepend(errors.at(i)); // this is a real error.
        }
    }

    // report any real errors which occurred during plugin loading or qmldir parsing.
    if (!realErrors.isEmpty()) {
        setError(realErrors);
        return;
    }

    foreach (const QQmlScript::Import &import, scriptParser.imports()) {
        QQmlDirComponents qmldircomponentsnetwork;
        if (import.type == QQmlScript::Import::Script)
            continue;

        if (import.type == QQmlScript::Import::File && import.qualifier.isEmpty()) {
            QUrl qmldirUrl = finalUrl().resolved(QUrl(import.uri + QLatin1String("/qmldir")));
            if (QQmlQmldirData *qmldir = qmldirForUrl(qmldirUrl))
                qmldircomponentsnetwork = qmldir->dirComponents();
        }

        int vmaj = -1;
        int vmin = -1;
        import.extractVersion(&vmaj, &vmin);

        QList<QQmlError> errors;
        if (m_imports.addImport(importDatabase, import.uri, import.qualifier, vmaj, vmin,
                                import.type, qmldircomponentsnetwork, &errors).isNull()) {
            QQmlError error;
            if (errors.size()) {
                error = errors.takeFirst();
            } else {
                // this should not be possible!
                // Description should come from error provided by addImport() function.
                error.setDescription(QQmlTypeLoader::tr("Unreported error adding script import to import database"));
            }
            error.setUrl(m_imports.baseUrl());
            error.setLine(import.location.start.line);
            error.setColumn(import.location.start.column);
            errors.prepend(error); // put it back on the list after filling out information.

            setError(errors);
            return;
        }
    }

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
        int majorVersion;
        int minorVersion;
        QQmlImportedNamespace *typeNamespace = 0;
        QList<QQmlError> errors;

        if (!m_imports.resolveType(parserRef->name, &ref.type, &url, &majorVersion, &minorVersion,
                                   &typeNamespace, &errors) || typeNamespace) {
            // Known to not be a type:
            //  - known to be a namespace (Namespace {})
            //  - type with unknown namespace (UnknownNamespace.SomeType {})
            QQmlError error;
            QString userTypeName = parserRef->name;
            userTypeName.replace(QLatin1Char('/'),QLatin1Char('.'));
            if (typeNamespace) {
                error.setDescription(QQmlTypeLoader::tr("Namespace %1 cannot be used as a type").arg(userTypeName));
            } else {
                if (errors.size()) {
                    error = errors.takeFirst();
                } else {
                    // this should not be possible!
                    // Description should come from error provided by addImport() function.
                    error.setDescription(QQmlTypeLoader::tr("Unreported error adding script import to import database"));
                }
                error.setUrl(m_imports.baseUrl());
                error.setDescription(QQmlTypeLoader::tr("%1 %2").arg(userTypeName).arg(error.description()));
            }

            if (!parserRef->refObjects.isEmpty()) {
                QQmlScript::Object *obj = parserRef->refObjects.first();
                error.setLine(obj->location.start.line);
                error.setColumn(obj->location.start.column);
            }

            errors.prepend(error);
            setError(errors);
            return;
        }

        if (ref.type) {
            ref.majorVersion = majorVersion;
            ref.minorVersion = minorVersion;
        } else {
            ref.typeData = typeLoader()->get(QUrl(url));
            addDependency(ref.typeData);
        }

        if (parserRef->refObjects.count())
            ref.location = parserRef->refObjects.first()->location.start;

        m_types << ref;
    }
}

QQmlQmldirData *QQmlTypeData::qmldirForUrl(const QUrl &url)
{
    for (int ii = 0; ii < m_qmldirs.count(); ++ii) {
        if (m_qmldirs.at(ii)->url() == url)
            return m_qmldirs.at(ii);
    }
    return 0;
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
: QQmlDataBlob(url, JavaScriptFile), m_pragmas(QQmlScript::Object::ScriptBlock::None),
  m_imports(loader), m_scriptData(0), m_typeLoader(loader)
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
    return m_pragmas;
}

QQmlTypeLoader *QQmlScriptBlob::typeLoader() const
{
    return m_typeLoader;
}

const QQmlImports &QQmlScriptBlob::imports() const
{
    return m_imports;
}

QQmlScriptData *QQmlScriptBlob::scriptData() const
{
    return m_scriptData;
}

void QQmlScriptBlob::dataReceived(const QByteArray &data)
{
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(m_typeLoader->engine());
    QQmlImportDatabase *importDatabase = &ep->importDatabase;

    m_source = QString::fromUtf8(data);

    QQmlScript::Parser::JavaScriptMetaData metadata =
        QQmlScript::Parser::extractMetaData(m_source);

    m_imports.setBaseUrl(finalUrl(), finalUrlString());

    m_pragmas = metadata.pragmas;

    foreach (const QQmlScript::Import &import, metadata.imports) {
        Q_ASSERT(import.type != QQmlScript::Import::File);

        if (import.type == QQmlScript::Import::Script) {
            QUrl scriptUrl = finalUrl().resolved(QUrl(import.uri));
            QQmlScriptBlob *blob = typeLoader()->getScript(scriptUrl);
            addDependency(blob);

            ScriptReference ref;
            ref.location = import.location.start;
            ref.qualifier = import.qualifier;
            ref.script = blob;
            m_scripts << ref;
        } else {
            Q_ASSERT(import.type == QQmlScript::Import::Library);
            int vmaj = -1;
            int vmin = -1;
            import.extractVersion(&vmaj, &vmin);

            QList<QQmlError> errors;
            QString importUrl = m_imports.addImport(importDatabase, import.uri, import.qualifier, vmaj, vmin,
                                                    import.type, QQmlDirComponents(), &errors);
            if (importUrl.isNull()) {
                QQmlError error = errors.takeFirst();
                // description should be set by addImport().
                error.setUrl(m_imports.baseUrl());
                error.setLine(import.location.start.line);
                error.setColumn(import.location.start.column);
                errors.prepend(error);

                setError(errors);
                return;
            }

            // Does this library contain any scripts?
            QUrl libraryUrl(importUrl);
            const QQmlDirParser *dirParser = typeLoader()->qmlDirParser(libraryUrl.path() + QLatin1String("qmldir"));
            foreach (const QQmlDirParser::Script &script, dirParser->scripts()) {
                QUrl scriptUrl = libraryUrl.resolved(QUrl(script.fileName));
                QQmlScriptBlob *blob = typeLoader()->getScript(scriptUrl);
                addDependency(blob);

                ScriptReference ref;
                ref.location = import.location.start;
                ref.qualifier = script.nameSpace;
                ref.nameSpace = import.qualifier;
                ref.script = blob;
                m_scripts << ref;
            }
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
            error.setDescription(typeLoader()->tr("Script %1 unavailable").arg(script.script->url().toString()));
            errors.prepend(error);
            setError(errors);
        }
    }

    if (isError())
        return;

    QQmlEngine *engine = typeLoader()->engine();
    m_scriptData = new QQmlScriptData();
    m_scriptData->url = finalUrl();
    m_scriptData->urlString = finalUrlString();
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

    m_imports.populateCache(m_scriptData->importCache, engine);

    m_scriptData->pragmas = m_pragmas;
    m_scriptData->m_programSource = m_source.toUtf8();
    m_source.clear();
}

QQmlQmldirData::QQmlQmldirData(const QUrl &url)
: QQmlDataBlob(url, QmldirFile)
{
}

const QQmlDirComponents &QQmlQmldirData::dirComponents() const
{
    return m_components;
}

void QQmlQmldirData::dataReceived(const QByteArray &data)
{
    QQmlDirParser parser;
    parser.setSource(QString::fromUtf8(data));
    parser.parse();
    m_components = parser.components();
}

QT_END_NAMESPACE

#include "qqmltypeloader.moc"
