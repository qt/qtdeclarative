// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include <private/qqmldatablob_p.h>
#include <private/qqmlglobal_p.h>
#include <private/qqmlprofiler_p.h>
#include <private/qqmlsourcecoordinate_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmltypeloaderthread_p.h>

#include <QtQml/qqmlengine.h>

#include <qtqml_tracepoints_p.h>

DEFINE_BOOL_CONFIG_OPTION(dumpErrors, QML_DUMP_ERRORS);

QT_BEGIN_NAMESPACE

/*!
\class QQmlDataBlob
\brief The QQmlDataBlob encapsulates a data request that can be issued to a QQmlTypeLoader.
\internal

QQmlDataBlob's are loaded by a QQmlTypeLoader.  The user creates the QQmlDataBlob
and then calls QQmlTypeLoader::load() or QQmlTypeLoader::loadWithStaticData() to load it.
The QQmlTypeLoader invokes callbacks on the QQmlDataBlob as data becomes available.
*/

/*!
\enum QQmlDataBlob::Status

This enum describes the status of the data blob.

\value Null             The blob has not yet been loaded by a QQmlTypeLoader
\value Loading          The blob is loading network data.  The QQmlDataBlob::setData() callback has
                        not yet been invoked or has not yet returned.
\value WaitingForDependencies The blob is waiting for dependencies to be done before continuing.
                        This status only occurs after the QQmlDataBlob::setData() callback has been made,
                        and when the blob has outstanding dependencies.
\value Complete         The blob's data has been loaded and all dependencies are done.
\value Error            An error has been set on this blob.
*/

/*!
\enum QQmlDataBlob::Type

This enum describes the type of the data blob.

\value QmlFile          This is a QQmlTypeData
\value JavaScriptFile   This is a QQmlScriptData
\value QmldirFile       This is a QQmlQmldirData
*/

/*!
Create a new QQmlDataBlob for \a url and of the provided \a type.
*/
QQmlDataBlob::QQmlDataBlob(const QUrl &url, Type type, QQmlTypeLoader *manager)
    : m_typeLoader(manager)
    , m_type(type)
    , m_url(manager->interceptUrl(url, (QQmlAbstractUrlInterceptor::DataType)type))
    , m_finalUrl(url)
    , m_redirectCount(0)
    , m_inCallback(false)
    , m_isDone(false)
{
}

/*!  \internal */
QQmlDataBlob::~QQmlDataBlob()
{
    Q_ASSERT(m_waitingOnMe.isEmpty());

    // Deleting a QQmlDataBlob in the engine thread is conceptually dangerous
    // because it manipulates other blobs' m_waitingFor lists. We can guarantee
    // that the list is empty if the blob isCompleteOrError. Therefore, in such
    // a case, it's fine to delete it from the engine thread.
    // Furthermore, if the type loader thread is (temporarily or permanently)
    // shut down, we cannot run into concurrency here.
    //
    // Unfortunately, the typeLoader pointer itself may be dangling at this point
    // if the QQmlDataBlob is destroyed after the engine. That can happen if you
    // hold on to a QQmlComponent for longer than the engine it belongs to. You
    // shouldn't do it, but the mistake is very common and harmless if you don't
    // touch the component anymore after the engine is gone. In order to actually
    // assert on the thread safety here, we'd have to touch the typeLoader pointer.
    // Q_ASSERT(isCompleteOrError() || isTypeLoaderThread() || !isTypeLoaderThreadRunning());

    cancelAllWaitingFor();
}

/*!
  Must be called before loading can occur.
*/
void QQmlDataBlob::startLoading()
{
    // This can be called on either thread but since both status() and setStatus() are atomic
    // this is fine.
    Q_ASSERT(status() == QQmlDataBlob::Null);
    setStatus(QQmlDataBlob::Loading);
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
    return status() == WaitingForDependencies ||
            status() == ResolvingDependencies;
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

bool QQmlDataBlob::isAsync() const
{
    return m_data.isAsync();
}

/*!
Returns the data download progress from 0 to 1.
*/
qreal QQmlDataBlob::progress() const
{
    return m_data.progress();
}

/*!
Returns the physical url of the data.  Initially this is the same as
finalUrl(), but if a URL interceptor is set, it will work on this URL
and leave finalUrl() alone.

\sa finalUrl()
*/
QUrl QQmlDataBlob::url() const
{
    return m_url;
}

QString QQmlDataBlob::urlString() const
{
    // TODO: This is dangerous. It can be called from either thread.
    if (m_urlString.isEmpty())
        m_urlString = m_url.toString();

    return m_urlString;
}

/*!
Returns the logical URL to be used for resolving further URLs referred to in
the code.

This is the blob url passed to the constructor. If a URL interceptor rewrites
the URL, this one stays the same. If a network redirect happens while fetching
the data, this url is updated to reflect the new location. Therefore, if both
an interception and a redirection happen, the final url will indirectly
incorporate the result of the interception, potentially breaking further
lookups.

\sa url()
*/
QUrl QQmlDataBlob::finalUrl() const
{
    return m_finalUrl;
}

/*!
Returns the finalUrl() as a string.
*/
QString QQmlDataBlob::finalUrlString() const
{
    // TODO: This is dangerous. It can be called from either thread.

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
    Q_ASSERT(isCompleteOrError()
             || !m_typeLoader
             || !m_typeLoader->thread()
             || m_typeLoader->thread()->isThisThread());
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
    assertTypeLoaderThread();

    QList<QQmlError> l;
    l << errors;
    setError(l);
}

/*!
\overload
*/
void QQmlDataBlob::setError(const QList<QQmlError> &errors)
{
    assertTypeLoaderThread();

    Q_ASSERT(status() != Error);
    Q_ASSERT(m_errors.isEmpty());

    // m_errors must be set before the m_data fence
    m_errors.reserve(errors.size());
    for (const QQmlError &error : errors) {
        if (error.url().isEmpty()) {
            QQmlError mutableError = error;
            mutableError.setUrl(url());
            m_errors.append(mutableError);
        } else {
            m_errors.append(error);
        }
    }

    cancelAllWaitingFor();
    setStatus(Error);

    if (dumpErrors()) {
        qWarning().nospace() << "Errors for " << urlString();
        for (int ii = 0; ii < errors.size(); ++ii)
            qWarning().nospace() << "    " << qPrintable(errors.at(ii).toString());
    }

    if (!m_inCallback)
        tryDone();
}

void QQmlDataBlob::setError(const QQmlJS::DiagnosticMessage &error)
{
    assertTypeLoaderThread();
    QQmlError e;
    e.setColumn(qmlConvertSourceCoordinate<quint32, int>(error.loc.startColumn));
    e.setLine(qmlConvertSourceCoordinate<quint32, int>(error.loc.startLine));
    e.setDescription(error.message);
    e.setUrl(url());
    setError(e);
}

void QQmlDataBlob::setError(const QString &description)
{
    assertTypeLoaderThread();
    QQmlError e;
    e.setDescription(description);
    e.setUrl(url());
    setError(e);
}

/*!
Wait for \a blob to become complete or to error.  If \a blob is already
complete or in error, or this blob is already complete, this has no effect.

The setError() method may only be called from within a QQmlDataBlob callback.
*/
void QQmlDataBlob::addDependency(const QQmlDataBlob::Ptr &blob)
{
    assertTypeLoaderThread();

    Q_ASSERT(status() != Null);

    if (!blob ||
        blob->status() == Error || blob->status() == Complete ||
        status() == Error || status() == Complete || m_isDone)
        return;

    for (const auto &existingDep: std::as_const(m_waitingFor)) {
        if (existingDep.data() == blob)
            return;
    }

    m_waitingFor.append(blob);
    blob->m_waitingOnMe.append(this);

    setStatus(WaitingForDependencies);

    // Check circular dependency
    if (m_waitingOnMe.indexOf(blob.data()) >= 0) {
        qCWarning(lcCycle) << "Cyclic dependency detected between" << this->url().toString()
                           << "and" << blob->url().toString();
        cancelAllWaitingFor();
        setStatus(Error);
    }
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
    assertTypeLoaderThread();
}

#if QT_CONFIG(qml_network)
/*!
Invoked if there is a network error while fetching this blob.

The default implementation sets an appropriate QQmlError.
*/
void QQmlDataBlob::networkError(QNetworkReply::NetworkError networkError)
{
    assertTypeLoaderThread();

    Q_UNUSED(networkError);

    QQmlError error;
    error.setUrl(m_url);

    const char *errorString = nullptr;
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
#endif // qml_network

/*!
Called if \a blob, which was previously waited for, has an error.

The default implementation does nothing.
*/
void QQmlDataBlob::dependencyError(const QQmlDataBlob::Ptr &blob)
{
    assertTypeLoaderThread();
    Q_UNUSED(blob);
}

/*!
Called if \a blob, which was previously waited for, has completed.

The default implementation does nothing.
*/
void QQmlDataBlob::dependencyComplete(const QQmlDataBlob::Ptr &blob)
{
    assertTypeLoaderThread();
    Q_UNUSED(blob);
}

/*!
Called when all blobs waited for have completed.  This occurs regardless of
whether they are in error, or complete state.

The default implementation does nothing.
*/
void QQmlDataBlob::allDependenciesDone()
{
    assertTypeLoaderThread();
    setStatus(QQmlDataBlob::ResolvingDependencies);
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
    assertEngineThread();
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
    assertEngineThread();
}

void QQmlDataBlob::tryDone()
{
    assertTypeLoaderThread();

    if (status() != Loading && m_waitingFor.isEmpty() && !m_isDone) {
        m_isDone = true;
        addref();

#ifdef DATABLOB_DEBUG
        qWarning("QQmlDataBlob::done() %s", qPrintable(urlString()));
#endif
        done();

        if (status() != Error)
            setStatus(Complete);

        notifyAllWaitingOnMe();

        // Locking is not required here, as anyone expecting callbacks must
        // already be protected against the blob being completed (as set above);
#ifdef DATABLOB_DEBUG
        qWarning("QQmlDataBlob: Dispatching completed");
#endif
        m_typeLoader->thread()->callCompleted(this);

        release();
    }
}

void QQmlDataBlob::cancelAllWaitingFor()
{
    while (m_waitingFor.size()) {

        // We can assert here since we are sure that m_waitingFor is either empty whenever
        // we delete a QQmlDataBlob from outside the type loader thread, or the type loader
        // thread has been suspended before.
        assertTypeLoaderThreadIfRunning();

        QQmlRefPointer<QQmlDataBlob> blob = m_waitingFor.takeLast();

        Q_ASSERT(blob->m_waitingOnMe.contains(this));

        blob->m_waitingOnMe.removeOne(this);
    }
}

void QQmlDataBlob::notifyAllWaitingOnMe()
{
    assertTypeLoaderThread();

    while (m_waitingOnMe.size()) {
        QQmlDataBlob::Ptr blob = m_waitingOnMe.takeLast();

        Q_ASSERT(std::any_of(blob->m_waitingFor.constBegin(), blob->m_waitingFor.constEnd(),
                             [this](const QQmlRefPointer<QQmlDataBlob> &waiting) { return waiting.data() == this; }));

        blob->notifyComplete(this);
    }
}

void QQmlDataBlob::notifyComplete(const QQmlDataBlob::Ptr &blob)
{
    assertTypeLoaderThread();

    Q_ASSERT(blob->status() == Error || blob->status() == Complete);
    Q_TRACE_SCOPE(QQmlCompiling, blob->url());
    QQmlCompilingProfiler prof(typeLoader()->profiler(), blob.data());

    m_inCallback = true;

    QQmlRefPointer<QQmlDataBlob> blobRef;
    for (int i = 0; i < m_waitingFor.size(); ++i) {
        if (m_waitingFor.at(i).data() == blob) {
            blobRef = m_waitingFor.takeAt(i);
            break;
        }
    }
    Q_ASSERT(blobRef);

    if (blob->status() == Error) {
        dependencyError(blob);
    } else if (blob->status() == Complete) {
        dependencyComplete(blob);
    }

    if (!isError() && m_waitingFor.isEmpty())
        allDependenciesDone();

    m_inCallback = false;

    tryDone();
}

QString QQmlDataBlob::SourceCodeData::readAll(QString *error) const
{
    error->clear();
    if (hasInlineSourceCode)
        return inlineSourceCode;

    QFile f(fileInfo.absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
        *error = f.errorString();
        return QString();
    }

    const qint64 fileSize = fileInfo.size();

    if (uchar *mappedData = f.map(0, fileSize)) {
        QString source = QString::fromUtf8(reinterpret_cast<const char *>(mappedData), fileSize);
        f.unmap(mappedData);
        return source;
    }

    QByteArray data(fileSize, Qt::Uninitialized);
    if (f.read(data.data(), data.size()) != data.size()) {
        *error = f.errorString();
        return QString();
    }
    return QString::fromUtf8(data);
}

QDateTime QQmlDataBlob::SourceCodeData::sourceTimeStamp() const
{
    if (hasInlineSourceCode)
        return QDateTime();

    return fileInfo.lastModified();
}

bool QQmlDataBlob::SourceCodeData::exists() const
{
    if (hasInlineSourceCode)
        return true;
    return fileInfo.exists();
}

bool QQmlDataBlob::SourceCodeData::isEmpty() const
{
    if (hasInlineSourceCode)
        return inlineSourceCode.isEmpty();
    return fileInfo.size() == 0;
}

bool QQmlDataBlob::setStatus(Status status)
{
    switch (status) {
    case Loading:
        break;
    case WaitingForDependencies:
        Q_ASSERT(!m_waitingFor.isEmpty());
        break;
    case Null:
    case ResolvingDependencies:
    case Complete:
    case Error:
        Q_ASSERT(m_waitingFor.isEmpty());
        break;
    }

    return m_data.setStatus(status);
}

QT_END_NAMESPACE
