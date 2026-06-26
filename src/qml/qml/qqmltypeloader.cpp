// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include <private/qqmltypeloader_p.h>

#include <private/qqmldirdata_p.h>
#include <private/qqmlprofiler_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qqmlscriptdata_p.h>
#include <private/qqmlsourcecoordinate_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmltypeloaderqmldircontent_p.h>
#include <private/qqmltypeloaderthread_p.h>
#include <private/qv4compiler_p.h>
#include <private/qv4compilercontext_p.h>
#include <private/qv4runtimecodegen_p.h>

#include <QtQml/qqmlabstracturlinterceptor.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlextensioninterface.h>
#include <QtQml/qqmlfile.h>
#include <QtQml/qqmlnetworkaccessmanagerfactory.h>

#include <qtqml_tracepoints_p.h>

#include <QtCore/qdir.h>
#include <QtCore/qdirlisting.h>
#include <QtCore/qfile.h>
#include <QtCore/qlibraryinfo.h>
#include <QtCore/qthread.h>

#ifdef Q_OS_MACOS
#include "private/qcore_mac_p.h"
#endif

#include <functional>

#define ASSERT_LOADTHREAD() \
    Q_ASSERT(thread() && thread()->isThisThread())
#define ASSERT_ENGINETHREAD() \
    Q_ASSERT(!engine()->jsEngine() || engine()->jsEngine()->thread()->isCurrentThread())

QT_BEGIN_NAMESPACE

Q_TRACE_POINT(qtqml, QQmlCompiling_entry, const QUrl &url)
Q_TRACE_POINT(qtqml, QQmlCompiling_exit)

/*!
\class QQmlTypeLoader
\brief The QQmlTypeLoader class abstracts loading files and their dependencies over the network.
\internal

The QQmlTypeLoader class is provided for the exclusive use of the QQmlTypeLoader class.

Clients create QQmlDataBlob instances and submit them to the QQmlTypeLoader class
through the QQmlTypeLoader::load() or QQmlTypeLoader::loadWithStaticData() methods.
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

void QQmlTypeLoader::invalidate()
{
    ASSERT_ENGINETHREAD();

    shutdownThread();

#if QT_CONFIG(qml_network)
    // Need to delete the network replies after
    // the loader thread is shutdown as it could be
    // getting new replies while we clear them
    QQmlTypeLoaderThreadDataPtr data(&m_data);
    data->networkReplies.clear();
#endif // qml_network
}

void QQmlTypeLoader::addUrlInterceptor(QQmlAbstractUrlInterceptor *urlInterceptor)
{
    ASSERT_ENGINETHREAD();
    QQmlTypeLoaderConfiguredDataPtr data(&m_data);
    data->urlInterceptors.append(urlInterceptor);
}

void QQmlTypeLoader::removeUrlInterceptor(QQmlAbstractUrlInterceptor *urlInterceptor)
{
    ASSERT_ENGINETHREAD();
    QQmlTypeLoaderConfiguredDataPtr data(&m_data);
    data->urlInterceptors.removeOne(urlInterceptor);
}

QList<QQmlAbstractUrlInterceptor *> QQmlTypeLoader::urlInterceptors() const
{
    ASSERT_ENGINETHREAD();
    QQmlTypeLoaderConfiguredDataConstPtr data(&m_data);
    return data->urlInterceptors;
}

static QUrl doInterceptUrl(
        const QUrl &url, QQmlAbstractUrlInterceptor::DataType type,
        const QList<QQmlAbstractUrlInterceptor *> &urlInterceptors)
{
    QUrl result = url;
    for (QQmlAbstractUrlInterceptor *interceptor : urlInterceptors)
        result = interceptor->intercept(result, type);
    return result;
}

QUrl QQmlTypeLoader::interceptUrl(const QUrl &url, QQmlAbstractUrlInterceptor::DataType type) const
{
    // Can be called from either thread, but only after interceptor setup is done.

    QQmlTypeLoaderConfiguredDataConstPtr data(&m_data);
    return doInterceptUrl(url, type, data->urlInterceptors);
}

bool QQmlTypeLoader::hasUrlInterceptors() const
{
    // Can be called from either thread, but only after interceptor setup is done.
    QQmlTypeLoaderConfiguredDataConstPtr data(&m_data);
    return !data->urlInterceptors.isEmpty();
}

#if QT_CONFIG(qml_debug)
void QQmlTypeLoader::setProfiler(QQmlProfiler *profiler)
{
    ASSERT_ENGINETHREAD();

    QQmlTypeLoaderConfiguredDataPtr data(&m_data);
    Q_ASSERT(!data->profiler);
    data->profiler.reset(profiler);
}
#endif

struct PlainLoader {
    void loadThread(QQmlTypeLoader *loader, const QQmlDataBlob::Ptr &blob) const
    {
        loader->loadThread(blob);
    }
    void load(QQmlTypeLoader *loader, const QQmlDataBlob::Ptr &blob) const
    {
        loader->ensureThread()->load(blob);
    }
    void loadAsync(QQmlTypeLoader *loader, const QQmlDataBlob::Ptr &blob) const
    {
        loader->ensureThread()->loadAsync(blob);
    }
};

struct StaticLoader {
    const QByteArray &data;
    StaticLoader(const QByteArray &data) : data(data) {}

    void loadThread(QQmlTypeLoader *loader, const QQmlDataBlob::Ptr &blob) const
    {
        loader->loadWithStaticDataThread(blob, data);
    }
    void load(QQmlTypeLoader *loader, const QQmlDataBlob::Ptr &blob) const
    {
        loader->ensureThread()->loadWithStaticData(blob, data);
    }
    void loadAsync(QQmlTypeLoader *loader, const QQmlDataBlob::Ptr &blob) const
    {
        loader->ensureThread()->loadWithStaticDataAsync(blob, data);
    }
};

struct CachedLoader {
    const QQmlPrivate::CachedQmlUnit *unit;
    CachedLoader(const QQmlPrivate::CachedQmlUnit *unit) :  unit(unit) {}

    void loadThread(QQmlTypeLoader *loader, const QQmlDataBlob::Ptr &blob) const
    {
        loader->loadWithCachedUnitThread(blob, unit);
    }
    void load(QQmlTypeLoader *loader, const QQmlDataBlob::Ptr &blob) const
    {
        loader->ensureThread()->loadWithCachedUnit(blob, unit);
    }
    void loadAsync(QQmlTypeLoader *loader, const QQmlDataBlob::Ptr &blob) const
    {
        loader->ensureThread()->loadWithCachedUnitAsync(blob, unit);
    }
};

template<typename Loader>
void QQmlTypeLoader::doLoad(const Loader &loader, const QQmlDataBlob::Ptr &blob, Mode mode)
{
    // Can be called from either thread.
#ifdef DATABLOB_DEBUG
    qWarning("QQmlTypeLoader::doLoad(%s): %s thread", qPrintable(blob->urlString()),
             (m_thread && m_thread->isThisThread()) ? "Compile" : "Engine");
#endif
    blob->startLoading();

    if (QQmlTypeLoaderThread *t = thread(); t && t->isThisThread()) {
        loader.loadThread(this, blob);
        return;
    }

    if (mode == Asynchronous) {
        blob->setIsAsync(true);
        loader.loadAsync(this, blob);
        return;
    }

    loader.load(this, blob);
    if (blob->isCompleteOrError())
        return;

    if (mode == PreferSynchronous) {
        blob->setIsAsync(true);
        return;
    }

    Q_ASSERT(mode == Synchronous);
    Q_ASSERT(thread());

    QQmlTypeLoaderSharedDataConstPtr lock(&m_data);
    do {
        m_data.thread()->waitForNextMessage();
    } while (!blob->isCompleteOrError());
}

/*!
Load the provided \a blob from the network or filesystem.

The loader must be locked.
*/
void QQmlTypeLoader::load(const QQmlDataBlob::Ptr &blob, Mode mode)
{
    // Can be called from either thread.
    doLoad(PlainLoader(), blob, mode);
}

/*!
Load the provided \a blob with \a data.  The blob's URL is not used by the data loader in this case.

The loader must be locked.
*/
void QQmlTypeLoader::loadWithStaticData(
        const QQmlDataBlob::Ptr &blob, const QByteArray &data, Mode mode)
{
    // Can be called from either thread.
    doLoad(StaticLoader(data), blob, mode);
}

void QQmlTypeLoader::loadWithCachedUnit(
        const QQmlDataBlob::Ptr &blob, const QQmlPrivate::CachedQmlUnit *unit, Mode mode)
{
    // Can be called from either thread.
    doLoad(CachedLoader(unit), blob, mode);
}

void QQmlTypeLoader::drop(const QQmlDataBlob::Ptr &blob)
{
    ASSERT_ENGINETHREAD();

    // We must not destroy a QQmlDataBlob from the main thread
    // since it will shuffle its dependencies around.
    // Therefore, if we're not on the type loader thread,
    // we defer the destruction to the type loader thread.
    if (QQmlTypeLoaderThread *t = thread(); t && !t->isThisThread())
        t->drop(blob);
}

void QQmlTypeLoader::loadWithStaticDataThread(const QQmlDataBlob::Ptr &blob, const QByteArray &data)
{
    ASSERT_LOADTHREAD();

    setData(blob, data, DataOrigin::Static);
}

void QQmlTypeLoader::loadWithCachedUnitThread(const QQmlDataBlob::Ptr &blob, const QQmlPrivate::CachedQmlUnit *unit)
{
    ASSERT_LOADTHREAD();

    setCachedUnit(blob, unit);
}

void QQmlTypeLoader::loadThread(const QQmlDataBlob::Ptr &blob)
{
    ASSERT_LOADTHREAD();

    if (blob->m_url.isEmpty()) {
        QQmlError error;
        error.setDescription(QLatin1String("Invalid null URL"));
        blob->setError(error);
        return;
    }

    if (QQmlFile::isSynchronous(blob->m_url)) {
        const QString fileName = QQmlFile::urlToLocalFileOrQrc(blob->m_url);
        if (!fileExists(fileName) && QFileInfo::exists(fileName)) {
            // If the file doesn't exist at all, that's fine. It may be cached. If it's a case
            // mismatch, though, we have to error out.
            blob->setError(QLatin1String("File name case mismatch"));
            return;
        }

        if (blob->setProgress(1.f) && blob->isAsync())
            thread()->callDownloadProgressChanged(blob, 1.);

        setData(blob, fileName);

    } else {
#if QT_CONFIG(qml_network)
        QNetworkReply *reply = thread()->networkAccessManager()->get(QNetworkRequest(blob->m_url));
        QQmlTypeLoaderNetworkReplyProxy *nrp = thread()->networkReplyProxy();

        QQmlTypeLoaderThreadDataPtr data(&m_data);
        data->networkReplies.insert(reply, blob);

        if (reply->isFinished()) {
            nrp->manualFinished(reply);
        } else {
            QObject::connect(reply, &QNetworkReply::downloadProgress,
                             nrp, &QQmlTypeLoaderNetworkReplyProxy::downloadProgress);
            QObject::connect(reply, &QNetworkReply::finished,
                             nrp, &QQmlTypeLoaderNetworkReplyProxy::finished);
        }

#ifdef DATABLOB_DEBUG
        qWarning("QQmlDataBlob: requested %s", qPrintable(blob->urlString()));
#endif // DATABLOB_DEBUG
#endif // qml_network
    }
}

#define DATALOADER_MAXIMUM_REDIRECT_RECURSION 16

#if QT_CONFIG(qml_network)
void QQmlTypeLoader::networkReplyFinished(QNetworkReply *reply)
{
    ASSERT_LOADTHREAD();

    reply->deleteLater();

    QQmlTypeLoaderThreadDataPtr data(&m_data);
    QQmlRefPointer<QQmlDataBlob> blob = data->networkReplies.take(reply);

    Q_ASSERT(blob);

    blob->m_redirectCount++;

    if (blob->m_redirectCount < DATALOADER_MAXIMUM_REDIRECT_RECURSION) {
        QVariant redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            QUrl url = reply->url().resolved(redirect.toUrl());
            blob->m_finalUrl = url;
            blob->m_finalUrlString.clear();

            QNetworkReply *reply = thread()->networkAccessManager()->get(QNetworkRequest(url));
            QObject *nrp = thread()->networkReplyProxy();
            QObject::connect(reply, SIGNAL(finished()), nrp, SLOT(finished()));
            data->networkReplies.insert(reply, std::move(blob));
#ifdef DATABLOB_DEBUG
            qWarning("QQmlDataBlob: redirected to %s", qPrintable(blob->finalUrlString()));
#endif
            return;
        }
    }

    if (reply->error()) {
        blob->networkError(reply->error());
    } else {
        QByteArray data = reply->readAll();
        setData(blob, data, DataOrigin::Device);
    }
}

void QQmlTypeLoader::networkReplyProgress(QNetworkReply *reply,
                                                  qint64 bytesReceived, qint64 bytesTotal)
{
    ASSERT_LOADTHREAD();

    QQmlTypeLoaderThreadDataConstPtr data(&m_data);
    const QQmlRefPointer<QQmlDataBlob> blob = data->networkReplies.value(reply);

    Q_ASSERT(blob);

    if (bytesTotal != 0) {
        const qreal progress = (qreal(bytesReceived) / qreal(bytesTotal));
        if (blob->setProgress(progress) && blob->isAsync())
            thread()->callDownloadProgressChanged(blob, blob->progress());
    }
}
#endif // qml_network

/*! \internal
Call the initializeEngine() method on \a iface.  Used by QQmlTypeLoader to ensure it
gets called in the correct thread.
*/
template<class Interface>
void doInitializeEngine(
        Interface *iface, QQmlTypeLoaderThread *thread, QQmlTypeLoaderLockedData *data,
        const char *uri)
{
    // Can be called from either thread
    // Must not touch engine if called from type loader thread

    if (thread && thread->isThisThread())
        thread->initializeEngine(iface, uri);
    else
        iface->initializeEngine(data->engine()->qmlEngine(), uri);
}

void QQmlTypeLoader::initializeEngine(QQmlEngineExtensionInterface *iface, const char *uri)
{
    // Can be called from either thread
    doInitializeEngine(iface, thread(), &m_data, uri);
}

void QQmlTypeLoader::initializeEngine(QQmlExtensionInterface *iface, const char *uri)
{
    // Can be called from either thread
    doInitializeEngine(iface, thread(), &m_data, uri);
}

/*!
 * \internal
 * Use the given \a data as source code for the given \a blob. \a origin states where the \a data
 * came from and what we can do with it. DataOrigin::Static means that it's arbitrary static data
 * passed by the user. We shall not produce a .qmlc file for it and we shall disregard any existing
 * compilation units. DataOrigin::Device means that it was loaded from a file or other device and
 * can be assumed to remain the same. We can use any caching mechanism to load a compilation unit
 * for it and we can produce a .qmlc file for it.
 */
void QQmlTypeLoader::setData(const QQmlDataBlob::Ptr &blob, const QByteArray &data, DataOrigin origin)
{
    ASSERT_LOADTHREAD();

    QQmlDataBlob::SourceCodeData d;
    d.inlineSourceCode = QString::fromUtf8(data);
    d.hasInlineSourceCode = true;
    d.hasStaticData = (origin == DataOrigin::Static);
    setData(blob, d);
}

void QQmlTypeLoader::setData(const QQmlDataBlob::Ptr &blob, const QString &fileName)
{
    ASSERT_LOADTHREAD();

    QQmlDataBlob::SourceCodeData d;
    d.fileInfo = QFileInfo(fileName);
    setData(blob, d);
}

void QQmlTypeLoader::setData(const QQmlDataBlob::Ptr &blob, const QQmlDataBlob::SourceCodeData &d)
{
    ASSERT_LOADTHREAD();

    Q_TRACE_SCOPE(QQmlCompiling, blob->url());
    QQmlCompilingProfiler prof(profiler(), blob.data());

    blob->m_inCallback = true;

    blob->dataReceived(d);

    if (!blob->isError() && !blob->isWaiting())
        blob->allDependenciesDone();

    blob->m_inCallback = false;

    blob->tryDone();
}

void QQmlTypeLoader::setCachedUnit(const QQmlDataBlob::Ptr &blob, const QQmlPrivate::CachedQmlUnit *unit)
{
    ASSERT_LOADTHREAD();

    Q_TRACE_SCOPE(QQmlCompiling, blob->url());
    QQmlCompilingProfiler prof(profiler(), blob.data());

    blob->m_inCallback = true;

    blob->initializeFromCachedUnit(unit);

    if (!blob->isError() && !blob->isWaiting())
        blob->allDependenciesDone();

    blob->m_inCallback = false;

    blob->tryDone();
}

static bool isPathAbsolute(const QString &path)
{
#if defined(Q_OS_UNIX)
    return (path.at(0) == QLatin1Char('/'));
#else
    QFileInfo fi(path);
    return fi.isAbsolute();
#endif
}

static bool isPathQrcOrAbsolute(const QString &path)
{
    return path.startsWith(u':') || isPathAbsolute(path);
}

// importPathList() and the qmldir candidate paths derived from it are a mixture of absolute
// file system paths, qrc paths (":/...") and URLs (e.g. "qrc:/...", "file:///...", remote URLs,
// or whatever a URL interceptor produced). absoluteFilePath() and fileExists() only understand
// paths, not URLs. Turn any local URL into its path here. Returns an empty string for empty
// input or for anything that does not resolve to a local path (in particular remote URLs).
static QString localPathForUrlOrPath(const QString &urlOrPath)
{
    if (urlOrPath.isEmpty() || isPathQrcOrAbsolute(urlOrPath))
        return urlOrPath;
    return QQmlFile::urlToLocalFileOrQrc(urlOrPath);
}

/*!
    \internal
*/
QStringList QQmlTypeLoader::importPathList(PathType type) const
{
    QQmlTypeLoaderConfiguredDataConstPtr data(&m_data);
    if (type == LocalOrRemote)
        return data->importPaths;

    QStringList list;
    for (const QString &path : data->importPaths) {
        // The import paths are a mixture of absolute file system paths and URLs. None of them
        // starts with a bare ':' because addImportPath() rewrites qrc paths (":/foo") to
        // "qrc:/foo" URLs before storing them. So an absolute-path check together with
        // QQmlFile::isLocalFile() is enough to tell local entries (resolvable on the file
        // system or in qrc) from remote ones.
        if ((isPathAbsolute(path) || QQmlFile::isLocalFile(path)) == (type == Local))
            list.append(path);
    }

    return list;
}

/*!
    \internal
*/
void QQmlTypeLoader::addImportPath(const QString &path, AddPathMode mode)
{
    qCDebug(lcQmlImport) << "addImportPath:" << path;

    if (path.isEmpty())
        return;

    QUrl url = QUrl(path);
    QString cPath;

    if (url.scheme() == QLatin1String("file")) {
        cPath = QQmlFile::urlToLocalFileOrQrc(url);
    } else if (path.startsWith(QLatin1Char(':'))) {
        // qrc directory, e.g. :/foo
        // need to convert to a qrc url, e.g. qrc:/foo
        cPath = QLatin1String("qrc") + path;
        cPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
    } else if (url.isRelative() ||
               (url.scheme().size() == 1 && QFile::exists(path)) ) {  // windows path
        QDir dir = QDir(path);
        cPath = dir.canonicalPath();
    } else {
        cPath = path;
        cPath.replace(QLatin1Char('\\'), QLatin1Char('/'));
    }

    QQmlTypeLoaderConfiguredDataPtr data(&m_data);
    if (!cPath.isEmpty()) {
        if (mode == PrependPath) {
            // Prepending an existing path moves it to the front, to reflect
            // its new, higher priority.
            if (data->importPaths.contains(cPath))
                data->importPaths.move(data->importPaths.indexOf(cPath), 0);
            else
                data->importPaths.prepend(cPath);
        } else if (!data->importPaths.contains(cPath)) {
            // Appending leaves an already known path untouched, so that we
            // don't lower the priority of a path that was explicitly prepended.
            data->importPaths.append(cPath);
        }
    }
}

/*!
    \internal
*/
void QQmlTypeLoader::setImportPathList(const QStringList &paths)
{
    qCDebug(lcQmlImport) << "setImportPathList:" << paths;

    QQmlTypeLoaderConfiguredDataPtr data(&m_data);
    data->importPaths.clear();
    for (const QString &path : paths)
        addImportPath(path, AppendPath);

    // Our existing cached paths may have been invalidated
    clearQmldirInfo();
}


/*!
    \internal
*/
void QQmlTypeLoader::setPluginPathList(const QStringList &paths)
{
    qCDebug(lcQmlImport) << "setPluginPathList:" << paths;
    QQmlTypeLoaderConfiguredDataPtr data(&m_data);
    data->pluginPaths = paths;
}

/*!
    \internal
*/
void QQmlTypeLoader::addPluginPath(const QString& path, AddPathMode mode)
{
    qCDebug(lcQmlImport) << "addPluginPath:" << path;

    QQmlTypeLoaderConfiguredDataPtr data(&m_data);

    QUrl url = QUrl(path);
    QString canonicalPath = path;
    if (url.isRelative() || url.scheme() == QLatin1String("file")
        || (url.scheme().size() == 1 && QFile::exists(path)) ) {  // windows path
        QDir dir = QDir(path);
        canonicalPath = dir.canonicalPath();
    }

    if (mode == PrependPath)
        data->pluginPaths.prepend(canonicalPath);
    else
        data->pluginPaths.append(canonicalPath);
}

#if QT_CONFIG(qml_network)
QQmlNetworkAccessManagerFactoryPtrConst QQmlTypeLoader::networkAccessManagerFactory() const
{
    ASSERT_ENGINETHREAD();
    return QQmlNetworkAccessManagerFactoryPtrConst(&m_data);
}

void QQmlTypeLoader::setNetworkAccessManagerFactory(QQmlNetworkAccessManagerFactory *factory)
{
    ASSERT_ENGINETHREAD();
    QQmlNetworkAccessManagerFactoryPtr(&m_data).reset(factory);
}

QNetworkAccessManager *QQmlTypeLoader::createNetworkAccessManager(QObject *parent) const
{
    // Can be called from both threads, or even from a WorkerScript

    // TODO: Calling the user's create() method under the lock is quite rude.
    //       However, we've been doing so for a long time and stopping the
    //       practice would expose thread safety issues in user code.
    // ### Qt7: Maybe change the factory interface to provide a different method
    //          that can be called without the lock.
    if (const auto factory = QQmlNetworkAccessManagerFactoryPtrConst(&m_data))
        return factory->create(parent);

    return new QNetworkAccessManager(parent);
}
#endif // QT_CONFIG(qml_network)

void QQmlTypeLoader::clearQmldirInfo()
{
    QQmlTypeLoaderThreadDataPtr data(&m_data);

    auto itr = data->qmldirInfo.constBegin();
    while (itr != data->qmldirInfo.constEnd()) {
        const QQmlTypeLoaderThreadData::QmldirInfo *cache = *itr;
        do {
            const QQmlTypeLoaderThreadData::QmldirInfo *nextCache = cache->next;
            delete cache;
            cache = nextCache;
        } while (cache);

        ++itr;
    }
    data->qmldirInfo.clear();
}

static void initializeConfiguredData(
        const QQmlTypeLoaderConfiguredDataPtr &data, QV4::ExecutionEngine *engine)
{
    data->diskCacheOptions = engine->diskCacheOptions();
    data->isDebugging = engine->debugger() != nullptr;
    data->initialized = true;
}

void QQmlTypeLoader::startThread()
{
    ASSERT_ENGINETHREAD();

    if (!m_data.thread()) {
        // Re-read the relevant configuration values at the last possible moment before we start
        // the thread. After the thread has been started, changing the configuration would result
        // in UB. Therefore we can disregard this case. We need to re-read it because a preview
        // or a debugger may have been connected in between.
        QQmlTypeLoaderConfiguredDataPtr data(&m_data);
        initializeConfiguredData(data, m_data.engine());
        m_data.createThread(this);
    }
}

void QQmlTypeLoader::shutdownThread()
{
    ASSERT_ENGINETHREAD();

    if (m_data.thread())
        m_data.deleteThread();
}

QQmlTypeLoader::Blob::PendingImport::PendingImport(
        const QQmlRefPointer<Blob> &blob, const QV4::CompiledData::Import *import,
        QQmlImports::ImportFlags flags)
    : uri(blob->stringAt(import->uriIndex))
    , qualifier(blob->stringAt(import->qualifierIndex))
    , type(static_cast<QV4::CompiledData::Import::ImportType>(quint32(import->type)))
    , location(import->location)
    , flags(flags)
    , version(import->version)
{
}

QQmlTypeLoader::Blob::Blob(const QUrl &url, QQmlDataBlob::Type type, QQmlTypeLoader *loader)
  : QQmlDataBlob(url, type, loader)
  , m_importCache(new QQmlImports(), QQmlRefPointer<QQmlImports>::Adopt)
{
}

QQmlTypeLoader::Blob::~Blob()
{
}

bool QQmlTypeLoader::Blob::fetchQmldir(
        const QUrl &url, const QQmlTypeLoader::Blob::PendingImportPtr &import, int priority,
        QList<QQmlError> *errors)
{
    assertTypeLoaderThread();

    QQmlRefPointer<QQmlQmldirData> data = typeLoader()->getQmldir(url);

    data->setPriority(this, import, priority);

    if (data->status() == Error) {
        // This qmldir must not exist - which is not an error
        return true;
    } else if (data->status() == Complete) {
        // This data is already available
        return qmldirDataAvailable(data, errors);
    }

    // Wait for this data to become available
    addDependency(data.data());
    return true;
}

/*!
 * \internal
 * Import any qualified scripts of for \a import as listed in \a qmldir.
 * Precondition is that \a import is actually qualified.
 */
void QQmlTypeLoader::Blob::importQmldirScripts(
        const QQmlTypeLoader::Blob::PendingImportPtr &import,
        const QQmlTypeLoaderQmldirContent &qmldir, const QUrl &qmldirUrl)
{
    assertTypeLoaderThread();

    // A "prefer" directive in the qmldir redirects file lookups to another
    // location, typically the compiled-in resources. Honor it here so that
    // qualified scripts resolve from the same location as components and
    // unqualified scripts, which inherit the redirect via the import URL set
    // up by QQmlImports::addLibraryImport(). Otherwise a deployed module whose
    // files only exist in the resources would try to load the script from a
    // nonexistent file next to the qmldir on disk. See QTBUG-143877.
    QQmlTypeLoaderQmldirContent redirectedQmldir = qmldir;
    const QUrl scriptBaseUrl = redirectedQmldir.hasRedirection()
        ? QUrl{QQmlImports::redirectQmldirContent(typeLoader(), &redirectedQmldir)}
        : qmldirUrl;
    const auto qmldirScripts = redirectedQmldir.scripts();
    for (const QQmlDirParser::Script &script : qmldirScripts) {
        const QUrl plainUrl = QUrl(script.fileName);
        const QUrl scriptUrl = scriptBaseUrl.resolved(plainUrl);
        QQmlRefPointer<QQmlScriptBlob> blob = typeLoader()->getScript(scriptUrl, plainUrl);

        // Self-import via qmldir is OK-ish. We ignore it.
        if (blob.data() == this)
            continue;

        addDependency(blob.data());
        scriptImported(blob, import->location, script.nameSpace, import->qualifier);
    }
}

template<typename URL>
void postProcessQmldir(
        QQmlTypeLoader::Blob *self,
        const QQmlTypeLoader::Blob::PendingImportPtr &import, const QString &qmldirFilePath,
        const URL &qmldirUrl)
{
    self->assertTypeLoaderThread();

    const QQmlTypeLoaderQmldirContent qmldir = self->typeLoader()->qmldirContent(qmldirFilePath);
    if (!import->qualifier.isEmpty())
        self->importQmldirScripts(import, qmldir, QUrl(qmldirUrl));

    if (qmldir.plugins().isEmpty()) {
        // If the qmldir does not register a plugin, we might still have declaratively
        // registered types (if we are dealing with an application instead of a library)
        // We should use module name given in the qmldir rather than the one given by the
        // import since the import may be a directory import.
        auto module = QQmlMetaType::typeModule(qmldir.typeNamespace(), import->version);
        if (!module)
            QQmlMetaType::qmlRegisterModuleTypes(qmldir.typeNamespace());
        // else: If the module already exists, the types must have been already registered
    }
}

static void addDependencyImportError(
        const QQmlTypeLoader::Blob::PendingImportPtr &import, QList<QQmlError> *errors)
{
    QQmlError error;
    QString reason = errors->front().description();
    if (reason.size() > 512)
        reason = reason.first(252) + QLatin1String("... ...") + reason.last(252);
    if (import->version.hasMajorVersion()) {
        error.setDescription(
                QQmlImports::tr("module \"%1\" version %2.%3 cannot be imported because:\n%4")
                        .arg(import->uri, QString::number(import->version.majorVersion()),
                             import->version.hasMinorVersion()
                                     ? QString::number(import->version.minorVersion())
                                     : QLatin1String("x"),
                             reason));
    } else {
        error.setDescription(QQmlImports::tr("module \"%1\" cannot be imported because:\n%2")
                                     .arg(import->uri, reason));
    }
    errors->prepend(error);
}

bool QQmlTypeLoader::Blob::handleLocalQmldirForImport(
        const PendingImportPtr &import, const QString &qmldirFilePath,
        const QString &qmldirUrl, QList<QQmlError> *errors)
{
    // This is a local library import
    const QTypeRevision actualVersion = m_importCache->addLibraryImport(
            typeLoader(), import->uri, import->qualifier, import->version, qmldirFilePath,
            qmldirUrl, import->flags, import->precedence, errors);
    if (!actualVersion.isValid())
        return false;

           // Use more specific version for dependencies if possible
    if (actualVersion.hasMajorVersion())
        import->version = actualVersion;

    if (!loadImportDependencies(import, qmldirFilePath, import->flags, errors)) {
        addDependencyImportError(import, errors);
        return false;
    }

    postProcessQmldir(this, import, qmldirFilePath, qmldirUrl);
    return true;
}

bool QQmlTypeLoader::Blob::updateQmldir(const QQmlRefPointer<QQmlQmldirData> &data, const QQmlTypeLoader::Blob::PendingImportPtr &import, QList<QQmlError> *errors)
{
    // TODO: Shouldn't this lock?

    assertTypeLoaderThread();

    QString qmldirIdentifier = data->urlString();
    QString qmldirUrl = qmldirIdentifier.left(qmldirIdentifier.lastIndexOf(QLatin1Char('/')) + 1);

    typeLoader()->setQmldirContent(qmldirIdentifier, data->content());

    const QTypeRevision version = m_importCache->updateQmldirContent(
            typeLoader(), import->uri, import->version, import->qualifier, qmldirIdentifier,
            qmldirUrl, errors);
    if (!version.isValid())
        return false;

    // Use more specific version for dependencies if possible
    if (version.hasMajorVersion())
        import->version = version;

    if (!loadImportDependencies(import, qmldirIdentifier, import->flags, errors))
        return false;

    import->priority = 0;

    // Release this reference at destruction
    m_qmldirs << data;

    postProcessQmldir(this, import, qmldirIdentifier, qmldirUrl);
    return true;
}

bool QQmlTypeLoader::Blob::addScriptImport(const QQmlTypeLoader::Blob::PendingImportPtr &import)
{
    assertTypeLoaderThread();
    const QUrl url(import->uri);
    QQmlTypeLoader *loader = typeLoader();
    QQmlRefPointer<QQmlScriptBlob> blob = loader->getScript(finalUrl().resolved(url), url);
    addDependency(blob.data());
    scriptImported(blob, import->location, import->qualifier, QString());
    return true;
}

bool QQmlTypeLoader::Blob::addFileImport(const QQmlTypeLoader::Blob::PendingImportPtr &import, QList<QQmlError> *errors)
{
    assertTypeLoaderThread();
    QQmlImports::ImportFlags flags;

    QUrl importUrl(import->uri);
    QString path = importUrl.path();
    path.append(QLatin1String(path.endsWith(QLatin1Char('/')) ? "qmldir" : "/qmldir"));
    importUrl.setPath(path);

    // Can't resolve a relative URL if we don't know where we are
    if (!finalUrl().isValid() && importUrl.isRelative()) {
        QQmlError error;
        error.setDescription(
                QString::fromLatin1("Can't resolve relative qmldir URL %1 on invalid base URL")
                .arg(importUrl.toString()));
        errors->append(error);
        return false;
    }

    QUrl qmldirUrl = finalUrl().resolved(importUrl);
    if (!QQmlImports::isLocal(qmldirUrl)) {
        // This is a remote file; the import is currently incomplete
        flags = QQmlImports::ImportIncomplete;
    }

    const QTypeRevision version = m_importCache->addFileImport(
            typeLoader(), import->uri, import->qualifier, import->version, flags,
            import->precedence, nullptr, errors);
    if (!version.isValid())
        return false;

    // Use more specific version for the qmldir if possible
    if (version.hasMajorVersion())
        import->version = version;

    if (flags & QQmlImports::ImportIncomplete) {
        if (!fetchQmldir(qmldirUrl, import, 1, errors))
            return false;
    } else {
        const QString qmldirFilePath = QQmlFile::urlToLocalFileOrQrc(qmldirUrl);
        if (!loadImportDependencies(import, qmldirFilePath, import->flags, errors))
            return false;

        postProcessQmldir(this, import, qmldirFilePath, qmldirUrl);
    }

    return true;
}

bool QQmlTypeLoader::Blob::addLibraryImport(const QQmlTypeLoader::Blob::PendingImportPtr &import, QList<QQmlError> *errors)
{
    assertTypeLoaderThread();

    const LocalQmldirResult qmldirResult = typeLoader()->locateLocalQmldir(this, import, errors);
    switch (qmldirResult) {
    case QmldirFound:
        return true;
    case QmldirNotFound: {
        if (!loadImportDependencies(import, QString(), import->flags, errors)) {
            addDependencyImportError(import, errors);
            return false;
        }
        break;
    }
    case QmldirInterceptedToRemote:
        break;
    case QmldirRejected:
        return false;
    }

    // If there is a qmldir we cannot see, yet, then we have to wait.
    // The qmldir might contain import directives.
    // TODO: This should trigger on any potentially remote URLs, not only intercepted ones.
    //       However, fixing this would open the door for follow-up problems while providing
    //       rather limited benefits.
    if (qmldirResult != QmldirInterceptedToRemote && registerPendingTypes(import)) {
        if (m_importCache->addLibraryImport(
                typeLoader(), import->uri, import->qualifier, import->version, QString(),
                QString(), import->flags, import->precedence, errors).isValid()) {
            return true;
        }

        return false;
    }

    // We haven't yet resolved this import
    m_unresolvedImports << import;

    // Add this library and request the possible locations for it
    const QTypeRevision version = m_importCache->addLibraryImport(
            typeLoader(), import->uri, import->qualifier, import->version, QString(),
            QString(), import->flags | QQmlImports::ImportIncomplete, import->precedence,
            errors);

    if (!version.isValid())
        return false;

    // Use more specific version for finding the qmldir if possible
    if (version.hasMajorVersion())
        import->version = version;

    const bool hasInterceptors = m_typeLoader->hasUrlInterceptors();

    // Query any network import paths for this library.
    // Interceptor might redirect local paths.
    QStringList remotePathList = typeLoader()->importPathList(
            hasInterceptors ? LocalOrRemote : Remote);
    if (!remotePathList.isEmpty()) {
        // Probe for all possible locations
        int priority = 0;
        const QStringList qmlDirPaths = QQmlImports::completeQmldirPaths(
                    import->uri, remotePathList, import->version);
        for (const QString &qmldirPath : qmlDirPaths) {
            if (hasInterceptors) {
                QUrl url = m_typeLoader->interceptUrl(
                            QQmlImports::urlFromLocalFileOrQrcOrUrl(qmldirPath),
                            QQmlAbstractUrlInterceptor::QmldirFile);
                if (!QQmlFile::isLocalFile(url)
                        && !fetchQmldir(url, import, ++priority, errors)) {
                    return false;
                }
            } else if (!fetchQmldir(QUrl(qmldirPath), import, ++priority, errors)) {
                return false;
            }
        }
    }

    return true;
}

bool QQmlTypeLoader::Blob::registerPendingTypes(const PendingImportPtr &import)
{
    assertTypeLoaderThread();

    return
            // Major version of module already registered:
            // We believe that the registration is complete.
            QQmlMetaType::typeModule(import->uri, import->version)

            // Otherwise, try to register further module types.
            || QQmlMetaType::qmlRegisterModuleTypes(import->uri)

            // Otherwise, there is no way to register any further types.
            // Try with any module of that name.
            || QQmlMetaType::latestModuleVersion(import->uri).isValid();
}

bool QQmlTypeLoader::Blob::addImport(const QV4::CompiledData::Import *import,
                                     QQmlImports::ImportFlags flags, QList<QQmlError> *errors)
{
    assertTypeLoaderThread();
    return addImport(std::make_shared<PendingImport>(this, import, flags), errors);
}

bool QQmlTypeLoader::Blob::addImport(
        const QQmlTypeLoader::Blob::PendingImportPtr &import, QList<QQmlError> *errors)
{
    assertTypeLoaderThread();

    Q_ASSERT(errors);

    switch (import->type)
    {
    case QV4::CompiledData::Import::ImportLibrary:
        return addLibraryImport(import, errors);
    case QV4::CompiledData::Import::ImportFile:
        return addFileImport(import ,errors);
    case QV4::CompiledData::Import::ImportScript:
        return addScriptImport(import);
    case QV4::CompiledData::Import::ImportInlineComponent:
        Q_UNREACHABLE_RETURN(false); // addImport is never called with an inline component import
    }

    Q_UNREACHABLE_RETURN(false);
}

void QQmlTypeLoader::Blob::dependencyComplete(const QQmlDataBlob::Ptr &blob)
{
    assertTypeLoaderThread();

    if (blob->type() == QQmlDataBlob::QmldirFile) {
        QQmlQmldirData *data = static_cast<QQmlQmldirData *>(blob.data());
        QList<QQmlError> errors;
        if (!qmldirDataAvailable(data, &errors)) {
            Q_ASSERT(errors.size());
            QQmlError error(errors.takeFirst());
            error.setUrl(m_importCache->baseUrl());
            const QV4::CompiledData::Location importLocation = data->importLocation(this);
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(importLocation.line()));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(importLocation.column()));
            errors.prepend(error); // put it back on the list after filling out information.
            setError(errors);
        }
    }
}

bool QQmlTypeLoader::Blob::loadDependentImports(
        const QList<QQmlDirParser::Import> &imports, const QString &qualifier,
        QTypeRevision version, quint8 precedence, QQmlImports::ImportFlags flags,
        QList<QQmlError> *errors)
{
    assertTypeLoaderThread();

    for (const auto &import : imports) {
        if (import.flags & QQmlDirParser::Import::Optional)
            continue;
        auto dependencyImport = std::make_shared<PendingImport>();
        dependencyImport->uri = import.module;
        dependencyImport->qualifier = qualifier;
        dependencyImport->version = (import.flags & QQmlDirParser::Import::Auto)
                ? version : import.version;
        dependencyImport->flags = flags;
        dependencyImport->precedence = precedence;

        qCDebug(lcQmlImport)
                << "loading dependent import" << dependencyImport->uri << "version"
                << dependencyImport->version << "as" << dependencyImport->qualifier;

        if (!addImport(dependencyImport, errors)) {
            QQmlError error;
            error.setDescription(
                        QString::fromLatin1(
                            "Failed to load dependent import \"%1\" version %2.%3")
                        .arg(dependencyImport->uri)
                        .arg(dependencyImport->version.majorVersion())
                        .arg(dependencyImport->version.minorVersion()));
            errors->append(error);
            return false;
        }
    }

    return true;
}

bool QQmlTypeLoader::Blob::loadImportDependencies(
        const QQmlTypeLoader::Blob::PendingImportPtr &currentImport, const QString &qmldirUri,
        QQmlImports::ImportFlags flags, QList<QQmlError> *errors)
{
    assertTypeLoaderThread();

    QList<QQmlDirParser::Import> implicitImports
            = QQmlMetaType::moduleImports(currentImport->uri, currentImport->version);
    if (!qmldirUri.isEmpty())
        implicitImports += typeLoader()->qmldirContent(qmldirUri).imports();

    // Prevent overflow from one category of import into the other.
    switch (currentImport->precedence) {
    case QQmlImportInstance::Implicit - 1:
    case QQmlImportInstance::Lowest: {
        QQmlError error;
        error.setDescription(
                    QString::fromLatin1("Too many dependent imports for %1 %2.%3")
                    .arg(currentImport->uri)
                    .arg(currentImport->version.majorVersion())
                    .arg(currentImport->version.minorVersion()));
        errors->append(error);
        return false;
    }
    default:
        break;
    }

    if (!loadDependentImports(
                implicitImports, currentImport->qualifier, currentImport->version,
                currentImport->precedence + 1, flags, errors)) {
        QQmlError error;
        error.setDescription(
                    QString::fromLatin1(
                        "Failed to load dependencies for module \"%1\" version %2.%3")
                    .arg(currentImport->uri)
                    .arg(currentImport->version.majorVersion())
                    .arg(currentImport->version.minorVersion()));
        errors->append(error);
        return false;
    }

    return true;
}

static QQmlTypeLoaderConfiguredDataConstPtr configuredData(QQmlTypeLoaderLockedData *m_data)
{
    if (!QQmlTypeLoaderConfiguredDataConstPtr(m_data)->initialized)
        initializeConfiguredData(QQmlTypeLoaderConfiguredDataPtr(m_data), m_data->engine());

    return QQmlTypeLoaderConfiguredDataConstPtr(m_data);
}

bool QQmlTypeLoader::isDebugging()
{
    return configuredData(&m_data)->isDebugging;
}

bool QQmlTypeLoader::readCacheFile()
{
    return configuredData(&m_data)->diskCacheOptions & QV4::ExecutionEngine::DiskCache::QmlcRead;
}

bool QQmlTypeLoader::writeCacheFile()
{
    return configuredData(&m_data)->diskCacheOptions & QV4::ExecutionEngine::DiskCache::QmlcWrite;
}

QQmlMetaType::CacheMode QQmlTypeLoader::aotCacheMode()
{
    const QV4::ExecutionEngine::DiskCacheOptions options
            = configuredData(&m_data)->diskCacheOptions;
    if (!(options & QV4::ExecutionEngine::DiskCache::Aot))
        return QQmlMetaType::RejectAll;
    if (options & QV4::ExecutionEngine::DiskCache::AotByteCode)
        return QQmlMetaType::AcceptUntyped;
    return QQmlMetaType::RequireFullyTyped;
}

bool QQmlTypeLoader::Blob::qmldirDataAvailable(const QQmlRefPointer<QQmlQmldirData> &data, QList<QQmlError> *errors)
{
    assertTypeLoaderThread();
    return data->processImports(this, [&](const PendingImportPtr &import) {
        return updateQmldir(data, import, errors);
    });
}

static QStringList parseEnvPath(const QString &envImportPath)
{
    if (QDir::listSeparator() == u':') {
        // Double colons are interpreted as separator + resource path.
        QStringList paths = envImportPath.split(u':');
        bool wasEmpty = false;
        for (auto it = paths.begin(); it != paths.end();) {
            if (it->isEmpty()) {
                wasEmpty = true;
                it = paths.erase(it);
            } else {
                if (wasEmpty) {
                    it->prepend(u':');
                    wasEmpty = false;
                }
                ++it;
            }
        }
        return paths;
    } else {
        return envImportPath.split(QDir::listSeparator(), Qt::SkipEmptyParts);
    }
}

/*!
Constructs a new type loader that uses the given \a engine.
*/
QQmlTypeLoader::QQmlTypeLoader(QV4::ExecutionEngine *engine)
    : m_data(engine)
{
    // Add default import and plugin paths. Paths are added in decreasting
    // priority, by appending to the path lists.

    const bool isPluginApplication = QCoreApplication::testAttribute(Qt::AA_PluginApplication);

    auto addEnvPath = [this, isPluginApplication](const char *var, auto addPath) {
        if (Q_UNLIKELY(!isPluginApplication && !qEnvironmentVariableIsEmpty(var))) {
            const QStringList paths = parseEnvPath(qEnvironmentVariable(var));
            for (const QString &path : paths)
                (this->*addPath)(path, AppendPath);
        }
    };

    // Import paths, used for looking up QML modules, e.g. `MyModules/qmldir`

#if defined(Q_OS_ANDROID)
    addImportPath(QStringLiteral("qrc:/android_rcc_bundle/qml"), AppendPath);
#elif defined(Q_OS_MACOS)
    // Add the main bundle's Resources/qml directory as an import path, so that QML modules are
    // found successfully when running the app from its build dir.
    // This is where macdeployqt and our CMake deployment logic puts Qt and user qmldir files.
    if (!isPluginApplication) {
        if (CFBundleRef bundleRef = CFBundleGetMainBundle()) {
            if (QCFType<CFURLRef> urlRef = CFBundleCopyResourceURL(
                        bundleRef, QCFString(QLatin1String("qml")), 0, 0)) {
                if (QCFType<CFURLRef> absoluteUrlRef = CFURLCopyAbsoluteURL(urlRef)) {
                    if (QCFString path = CFURLCopyFileSystemPath(
                                absoluteUrlRef, kCFURLPOSIXPathStyle)) {
                        if (QFile::exists(path)) {
                            addImportPath(QDir(path).canonicalPath(), AppendPath);
                        }
                    }
                }
            }
        }
    }
#endif // Q_OS_DARWIN

    if (!isPluginApplication)
        addImportPath(QCoreApplication::applicationDirPath(), AppendPath);

    addImportPath(QStringLiteral("qrc:/qt-project.org/imports"), AppendPath);
    addImportPath(QStringLiteral("qrc:/qt/qml"), AppendPath);

    addEnvPath("QML2_IMPORT_PATH", &QQmlTypeLoader::addImportPath);
    addEnvPath("QML_IMPORT_PATH", &QQmlTypeLoader::addImportPath);

    const auto qmlImportPaths = QLibraryInfo::paths(QLibraryInfo::QmlImportsPath);
    for (const auto &qmlImportPath : qmlImportPaths)
        addImportPath(qmlImportPath, AppendPath);

    // Plugin paths, used for looking up the backing library of a QML module

#if defined(Q_OS_ANDROID)
    addEnvPath("QT_BUNDLED_LIBS_PATH", &QQmlTypeLoader::addPluginPath);
#endif
    addEnvPath("QML_PLUGIN_PATH", &QQmlTypeLoader::addPluginPath);

    const auto pluginPaths = QLibraryInfo::paths(QLibraryInfo::PluginsPath);
    for (const auto &pluginPath : pluginPaths)
        addPluginPath(pluginPath, AppendPath);

    // Explicitly add "." to the plugin paths, to represent the path relative
    // to where the qmldir was found. We can't use addPluginPath() here, as
    // it will resolve the path to an absolute path based on the working dir.
    QQmlTypeLoaderConfiguredDataPtr data(&m_data);
    data->pluginPaths << QLatin1String(".");
}

/*!
Destroys the type loader, first clearing the cache of any information about
loaded files.
*/
QQmlTypeLoader::~QQmlTypeLoader()
{
    ASSERT_ENGINETHREAD();

    shutdownThread();

    // Delete the thread before clearing the cache. Otherwise it will be started up again.
    invalidate();

    clearCache();

    clearQmldirInfo();
}

template<typename Blob>
QQmlRefPointer<Blob> handleExisting(
        const QQmlTypeLoaderSharedDataPtr &data, QQmlRefPointer<Blob> &&blob,
        QQmlTypeLoader::Mode mode)
{
    if ((mode == QQmlTypeLoader::PreferSynchronous && QQmlFile::isSynchronous(blob->finalUrl()))
            || mode == QQmlTypeLoader::Synchronous) {
        // this was started Asynchronous, but we need to force Synchronous
        // completion now.

        // This only works when called directly from e.g. the UI thread, but not
        // when recursively called on the QML thread via resolveTypes()

        // NB: We do not want to know whether the thread is the main thread, but specifically
        //     that the thread is _not_ the thread we're waiting for.
        //     If !QT_CONFIG(qml_type_loader_thread) the QML thread is the main thread.

        QQmlTypeLoaderThread *thread = data.thread();
        if (thread && !thread->isThisThread()) {
            while (!blob->isCompleteOrError())
                thread->waitForNextMessage(); // Requires lock to be held, via data above
        }
    }
    return blob;
}

/*!
Returns a QQmlTypeData for the specified \a url.  The QQmlTypeData may be cached.
*/
QQmlRefPointer<QQmlTypeData> QQmlTypeLoader::getType(const QUrl &unNormalizedUrl, Mode mode)
{
    // This can be called from either thread.

    Q_ASSERT(!unNormalizedUrl.isRelative() &&
            (QQmlFile::urlToLocalFileOrQrc(unNormalizedUrl).isEmpty() ||
             !QDir::isRelativePath(QQmlFile::urlToLocalFileOrQrc(unNormalizedUrl))));

    QQmlRefPointer<QQmlTypeData> typeData;
    {
        const QUrl url = QQmlMetaType::normalizedUrl(unNormalizedUrl);
        QQmlTypeLoaderSharedDataPtr data(&m_data);

        typeData = data->typeCache.value(url);
        if (typeData)
            return handleExisting(data, std::move(typeData), mode);

        // Trim before adding the new type, so that we don't immediately trim it away
        if (data->typeCache.size() >= data->typeCacheTrimThreshold)
            trimCache(data);

        typeData = QQml::makeRefPointer<QQmlTypeData>(url, this);

        // TODO: if (compiledData == 0), is it safe to omit this insertion?
        data->typeCache.insert(url, typeData);
    }

    return finalizeBlob(std::move(typeData), mode);
}

/*!
Returns a QQmlTypeData for the given \a data with the provided base \a url.  The
QQmlTypeData will not be cached.
*/
QQmlRefPointer<QQmlTypeData> QQmlTypeLoader::getType(
        const QByteArray &data, const QUrl &url, Mode mode)
{
    // Can be called from either thread.

    QQmlRefPointer<QQmlTypeData> typeData = QQml::makeRefPointer<QQmlTypeData>(url, this);
    QQmlTypeLoader::loadWithStaticData(QQmlDataBlob::Ptr(typeData.data()), data, mode);
    return typeData;
}

static bool isModuleUrl(const QUrl &url)
{
    return url.fragment() == QLatin1String("module") || url.path().endsWith(QLatin1String(".mjs"));
}

QQmlRefPointer<QQmlScriptBlob> QQmlTypeLoader::getScript(const QUrl &unNormalizedUrl, Mode mode)
{
    // This can be called from either thread.

    Q_ASSERT(!unNormalizedUrl.isRelative() &&
             (QQmlFile::urlToLocalFileOrQrc(unNormalizedUrl).isEmpty() ||
              !QDir::isRelativePath(QQmlFile::urlToLocalFileOrQrc(unNormalizedUrl))));

    QQmlRefPointer<QQmlScriptBlob> scriptBlob;
    {
        const QUrl url = QQmlMetaType::normalizedUrl(unNormalizedUrl);
        QQmlTypeLoaderSharedDataPtr data(&m_data);

        scriptBlob = data->scriptCache.value(url);
        if (scriptBlob)
            return handleExisting(data, std::move(scriptBlob), mode);

        scriptBlob = QQml::makeRefPointer<QQmlScriptBlob>(url, this, isModuleUrl(url)
                ? QQmlScriptBlob::IsESModule::Yes
                : QQmlScriptBlob::IsESModule::No);
        data->scriptCache.insert(url, scriptBlob);
    }

    return finalizeBlob(std::move(scriptBlob), mode);
}

QQmlRefPointer<QV4::CompiledData::CompilationUnit> QQmlTypeLoader::injectModule(
        const QUrl &relativeUrl, const QV4::CompiledData::Unit *unit)
{
    ASSERT_ENGINETHREAD();

    QQmlRefPointer<QQmlScriptBlob> blob = QQml::makeRefPointer<QQmlScriptBlob>(
            relativeUrl, this, QQmlScriptBlob::IsESModule::Yes);
    QQmlPrivate::CachedQmlUnit cached { unit, nullptr, nullptr};

    {
        QQmlTypeLoaderSharedDataPtr data(&m_data);
        data->scriptCache.insert(relativeUrl, blob);
    }

    loadWithCachedUnit(blob.data(), &cached, Synchronous);
    Q_ASSERT(blob->isComplete());
    return blob->scriptData()->compilationUnit();
}

/*!
Return a QQmlScriptBlob for \a unNormalizedUrl or \a relativeUrl.
This assumes PreferSynchronous, and therefore the result may not be ready yet.
*/
QQmlRefPointer<QQmlScriptBlob> QQmlTypeLoader::getScript(
        const QUrl &unNormalizedUrl, const QUrl &relativeUrl)
{
    // Can be called from either thread

    Q_ASSERT(!unNormalizedUrl.isRelative() &&
            (QQmlFile::urlToLocalFileOrQrc(unNormalizedUrl).isEmpty() ||
             !QDir::isRelativePath(QQmlFile::urlToLocalFileOrQrc(unNormalizedUrl))));

    const QUrl url = QQmlMetaType::normalizedUrl(unNormalizedUrl);

    QQmlRefPointer<QQmlScriptBlob> scriptBlob;
    {
        QQmlTypeLoaderSharedDataPtr data(&m_data);
        scriptBlob = data->scriptCache.value(url);

        // Also try the relative URL since manually registering native modules doesn't require
        // passing an absolute URL and we don't have a reference URL for native modules.
        if (!scriptBlob && unNormalizedUrl != relativeUrl)
            scriptBlob = data->scriptCache.value(relativeUrl);

        // Do not try to finish the loading via handleExisting() here.
        if (scriptBlob)
            return scriptBlob;

        scriptBlob = QQml::makeRefPointer<QQmlScriptBlob>(url, this, isModuleUrl(url)
                ? QQmlScriptBlob::IsESModule::Yes
                : QQmlScriptBlob::IsESModule::No);
        data->scriptCache.insert(url, scriptBlob);
    }

    return finalizeBlob(std::move(scriptBlob), PreferSynchronous);
}

/*!
Returns a QQmlQmldirData for \a url.  The QQmlQmldirData may be cached.
*/
QQmlRefPointer<QQmlQmldirData> QQmlTypeLoader::getQmldir(const QUrl &url)
{
    // Can be called from either thread.

    Q_ASSERT(!url.isRelative() &&
            (QQmlFile::urlToLocalFileOrQrc(url).isEmpty() ||
             !QDir::isRelativePath(QQmlFile::urlToLocalFileOrQrc(url))));

    QQmlRefPointer<QQmlQmldirData> qmldirData;
    {
        QQmlTypeLoaderSharedDataPtr data(&m_data);
        qmldirData = data->qmldirCache.value(url);
        if (qmldirData)
            return qmldirData;

        qmldirData = QQml::makeRefPointer<QQmlQmldirData>(url, this);
        data->qmldirCache.insert(url, qmldirData);
    }

    QQmlTypeLoader::load(QQmlDataBlob::Ptr(qmldirData.data()));
    return qmldirData;
}

static bool isResource(const QString &path)
{
    const bool startsWithColon = path.at(0) == QLatin1Char(':');
#if defined(Q_OS_ANDROID)
    return startsWithColon || path.startsWith(QLatin1String("assets:/", Qt::CaseInsensitive))
            || path.startsWith(QLatin1String("content:/"), Qt::CaseInsensitive);
#else
    return startsWithColon;
#endif
}

/*!
Returns the absolute filename of path via a directory cache.
Returns a empty string if the path does not exist.

Why a directory cache?  QML checks for files in many paths with
invalid directories.  By caching whether a directory exists
we avoid many stats.  We also cache the files' existence in the
directory, for the same reason.
*/
QString QQmlTypeLoader::absoluteFilePath(const QString &path) const
{
    // Can be called from either thread.

    if (path.isEmpty())
        return QString();

    if (isResource(path)) {
        // qrc resource
        QFileInfo fileInfo(path);
        return fileInfo.isFile() ? fileInfo.absoluteFilePath() : QString();
    }

    return fileExists(path)
            ? QFileInfo(path).absoluteFilePath()
            : QString();
}

static QString stripTrailingSlashes(const QString &path)
{
    for (qsizetype length = path.size(); length > 0; --length) {
        if (path[length - 1] != QLatin1Char('/'))
            return path.left(length);
    }

    return QString();
}

bool QQmlTypeLoader::fileExists(const QString &dirPath, const QString &file) const
{
    // Can be called from either thread.

    // We want to use QDirListing here because that gives us case-sensitive results even on
    // case-insensitive file systems. That is, for a file date.qml it only lists date.qml, not
    // Date.qml, dAte.qml, DATE.qml etc. QFileInfo::exists(), on the other hand, will happily
    // claim that Date.qml exists in such a situation on a case-insensitive file sysem. Such a
    // thing then shadows the JavaScript Date object and disaster ensues.

    const QChar nullChar(QChar::Null);
    if (dirPath.isEmpty() || dirPath.contains(nullChar) || file.isEmpty()
        || file.contains(nullChar)) {
        return false;
    }

    // NB: We really shouldn't see URLs (qrc:/ or the like) here. This is explicitly about paths.
    //     We don't handle file:/ either.

    if (isResource(dirPath)) {
        // The resource file system is case-sensitive. So we don't have to do the below gymnastics.
        // However, it changes spontanously as resources are loaded. Therefore we can't cache it.
        const QFileInfo info(dirPath + u'/' + file);
        return info.exists();
    }

    const QString path = stripTrailingSlashes(dirPath);

    QQmlTypeLoaderSharedDataConstPtr data(&m_data);
    return data->importDirCache.fileExists(path, file);
}


/*!
Returns true if the path is a directory via a directory cache.  Cache is
shared with absoluteFilePath().
*/
bool QQmlTypeLoader::directoryExists(const QString &path) const
{
    // Can be called from either thread.

    if (path.isEmpty())
        return false;

    if (isResource(path)) {
        // qrc resource
        QFileInfo fileInfo(path);
        return fileInfo.exists() && fileInfo.isDir();
    }

    const QString dirPath = stripTrailingSlashes(path);

    QQmlTypeLoaderSharedDataConstPtr data(&m_data);
    return data->importDirCache.directoryExists(path);

}


/*!
Return a QQmlTypeLoaderQmldirContent for absoluteFilePath.  The QQmlTypeLoaderQmldirContent may be cached.

\a filePath is a local file path.

It can also be a remote path for a remote directory import, but it will have been cached by now in this case.
*/
const QQmlTypeLoaderQmldirContent QQmlTypeLoader::qmldirContent(const QString &filePathIn)
{
    ASSERT_LOADTHREAD();

    QString filePath;

    // Try to guess if filePathIn is already a URL. This is necessarily fragile, because
    // - paths can contain ':', which might make them appear as URLs with schemes.
    // - windows drive letters appear as schemes (thus "< 2" below).
    // - a "file:" URL is equivalent to the respective file, but will be treated differently.
    // Yet, this heuristic is the best we can do until we pass more structured information here,
    // for example a QUrl also for local files.
    QUrl url(filePathIn);

    QQmlTypeLoaderThreadDataPtr data(&m_data);

    if (url.scheme().size() < 2) {
        filePath = filePathIn;
    } else {
        filePath = QQmlFile::urlToLocalFileOrQrc(url);
        if (filePath.isEmpty()) { // Can't load the remote here, but should be cached
            if (auto entry = data->importQmlDirCache.value(filePathIn))
                return **entry;
            else
                return QQmlTypeLoaderQmldirContent();
        }
    }

    QQmlTypeLoaderQmldirContent **val = data->importQmlDirCache.value(filePath);
    if (val)
        return **val;
    QQmlTypeLoaderQmldirContent *qmldir = new QQmlTypeLoaderQmldirContent;

#define ERROR(description) { QQmlError e; e.setDescription(description); qmldir->setError(e); }
#define NOT_READABLE_ERROR QString(QLatin1String("module \"$$URI$$\" definition \"%1\" not readable"))
#define NOT_FOUND_ERROR QString(QLatin1String("cannot load module \"$$URI$$\": File \"%1\" not found"))

    QFile file(filePath);
    if (!fileExists(filePath)) {
        ERROR(NOT_FOUND_ERROR.arg(filePath));
    } else if (file.open(QFile::ReadOnly)) {
        QByteArray data = file.readAll();
        qmldir->setContent(filePath, QString::fromUtf8(data));
    } else {
        ERROR(NOT_READABLE_ERROR.arg(filePath));
    }

#undef ERROR
#undef NOT_READABLE_ERROR
#undef NOT_FOUND_ERROR

    data->importQmlDirCache.insert(filePath, qmldir);
    return *qmldir;
}

void QQmlTypeLoader::setQmldirContent(const QString &url, const QString &content)
{
    ASSERT_LOADTHREAD();

    QQmlTypeLoaderThreadDataPtr data(&m_data);
    QQmlTypeLoaderQmldirContent *qmldir;
    QQmlTypeLoaderQmldirContent **val = data->importQmlDirCache.value(url);
    if (val) {
        qmldir = *val;
    } else {
        qmldir = new QQmlTypeLoaderQmldirContent;
        data->importQmlDirCache.insert(url, qmldir);
    }

    if (!qmldir->hasContent())
        qmldir->setContent(url, content);
}

template<typename Blob>
void clearBlobs(QHash<QUrl, QQmlRefPointer<Blob>> *blobs)
{
    std::for_each(blobs->cbegin(), blobs->cend(), [](const QQmlRefPointer<Blob> &blob) {
        blob->resetTypeLoader();
    });
    blobs->clear();
}

/*!
Clears cached information about loaded files, including any type data, scripts
and qmldir information.
*/
void QQmlTypeLoader::clearCache()
{
    // This looks dangerous because we're dropping live blobs on the engine thread.
    // However, it's safe because we shut down the type loader thread before we do so.

    ASSERT_ENGINETHREAD();

    // Temporarily shut the thread down and discard all messages, making it safe to
    // hack into the various data structures below.
    shutdownThread();

    QQmlTypeLoaderThreadDataPtr threadData(&m_data);
    qDeleteAll(threadData->importQmlDirCache);
    threadData->checksumCache.clear();
    threadData->importQmlDirCache.clear();

    QQmlTypeLoaderSharedDataPtr data(&m_data);
    clearBlobs(&data->typeCache);
    clearBlobs(&data->scriptCache);
    clearBlobs(&data->qmldirCache);
    data->typeCacheTrimThreshold = QQmlTypeLoaderSharedData::MinimumTypeCacheTrimThreshold;
    data->importDirCache.clear();

    // The thread will auto-restart next time we need it.
}

/*!
    \internal
    Remove any cached artifacts for \a url.

    Returns \c true if anything was removed, or \c false otherwise.
*/
bool QQmlTypeLoader::removeFromCache(const QUrl &url)
{
    const QUrl normalized = QQmlMetaType::normalizedUrl(url);
    const QQmlTypeLoaderSharedDataPtr data(&m_data);
    return data->typeCache.remove(normalized) || data->scriptCache.remove(normalized)
            || data->qmldirCache.remove(normalized);
}

void QQmlTypeLoader::trimCache()
{
    const QQmlTypeLoaderSharedDataPtr data(&m_data);
    trimCache(data);
}

void QQmlTypeLoader::updateTypeCacheTrimThreshold(const QQmlTypeLoaderSharedDataPtr &data)
{
    // This can be called from either thread and is called from a method that locks.

    int size = data->typeCache.size();
    if (size > data->typeCacheTrimThreshold)
        data->typeCacheTrimThreshold = size * 2;
    if (size < data->typeCacheTrimThreshold / 2) {
        data->typeCacheTrimThreshold
                = qMax(size * 2, int(QQmlTypeLoaderSharedData::MinimumTypeCacheTrimThreshold));
    }
}

void QQmlTypeLoader::trimCache(const QQmlTypeLoaderSharedDataPtr &data)
{
    // This can be called from either thread. It has to be called while the type loader mutex
    // is locked. It drops potentially live blobs, but only ones which are isCompleteOrError and
    // are not depended on by other blobs.

    while (true) {
        bool deletedOneType = false;
        for (auto iter = data->typeCache.begin(), end = data->typeCache.end(); iter != end;)  {
            const QQmlRefPointer<QQmlTypeData> &typeData = iter.value();

            // typeData->m_compiledData may be set early on in the proccess of loading a file, so
            // it's important to check the general loading status of the typeData before making any
            // other decisions.
            if (typeData->count() != 1 || !typeData->isCompleteOrError()) {
                ++iter;
                continue;
            }

            // isCompleteOrError means the waitingFor list of this typeData is empty.
            // Therefore, it cannot interfere with other blobs on destruction anymore.
            // Therefore, we can drop it on either the engine thread or the type loader thread.

            const QQmlRefPointer<QV4::CompiledData::CompilationUnit> &compilationUnit
                = typeData->m_compiledData;
            if (compilationUnit) {
                if (compilationUnit->count()
                        > QQmlMetaType::countInternalCompositeTypeSelfReferences(
                              compilationUnit) + 1) {
                    ++iter;
                    continue;
                }

                QQmlMetaType::unregisterInternalCompositeType(compilationUnit);
                Q_ASSERT(compilationUnit->count() == 1);
            }

            // There are no live objects of this type
            iter = data->typeCache.erase(iter);
            deletedOneType = true;
        }

        if (!deletedOneType)
            break;
    }

    // TODO: release any scripts which are no longer referenced by any types

    updateTypeCacheTrimThreshold(data);

    QQmlMetaType::freeUnusedTypesAndCaches();
}

bool QQmlTypeLoader::isTypeLoaded(const QUrl &url) const
{
    const QQmlTypeLoaderSharedDataConstPtr data(&m_data);
    return data->typeCache.contains(url);
}

bool QQmlTypeLoader::isScriptLoaded(const QUrl &url) const
{
    const QQmlTypeLoaderSharedDataConstPtr data(&m_data);
    return data->scriptCache.contains(url);
}

static QString pathToUrl(const QString &path)
{
    const auto dir = path.left(path.lastIndexOf(u"/") + 1);
    if (dir.at(0) == u':')
        return QStringLiteral("qrc") + dir;
    else
        return QUrl::fromLocalFile(dir).toString();
};

QStringList QQmlTypeLoader::urlsForModule(const QString &module) const
{
    const auto &importPaths = importPathList();
    const auto completedImportPaths = QQmlImports::completeQmldirPaths(module, importPaths, {});
    QStringList urls;
    for (const auto &importPath : completedImportPaths) {
        const auto absolutePath = absoluteFilePath(localPathForUrlOrPath(importPath));
        if (absolutePath.isEmpty())
            continue;
        if (const std::optional<QString> url = pathToUrl(absolutePath))
            urls.append(url.value());
    }

    return urls;
};

/*!
\internal

Locates the qmldir files for \a import. For each one, calls
handleLocalQmldirForImport() on \a blob. If that returns \c true, returns
\c QmldirFound.

If at least one callback invocation returned \c false and there are no qmldir
files left to check, returns \c QmldirRejected.

Otherwise, if interception redirects a previously local qmldir URL to a remote
one, returns \c QmldirInterceptedToRemote. Otherwise, returns \c QmldirNotFound.
*/
QQmlTypeLoader::LocalQmldirResult QQmlTypeLoader::locateLocalQmldir(
        QQmlTypeLoader::Blob *blob, const QQmlTypeLoader::Blob::PendingImportPtr &import,
        QList<QQmlError> *errors)
{
    // Check cache first

    LocalQmldirResult result = QmldirNotFound;
    QQmlTypeLoaderThreadData::QmldirInfo *cacheTail = nullptr;

    QQmlTypeLoaderThreadDataPtr threadData(&m_data);
    QQmlTypeLoaderThreadData::QmldirInfo **cachePtr = threadData->qmldirInfo.value(import->uri);
    QQmlTypeLoaderThreadData::QmldirInfo *cacheHead = cachePtr ? *cachePtr : nullptr;
    if (cacheHead) {
        cacheTail = cacheHead;
        do {
            if (cacheTail->version == import->version) {
                if (cacheTail->qmldirFilePath.isEmpty()) {
                    return cacheTail->qmldirPathUrl.isEmpty()
                    ? QmldirNotFound
                    : QmldirInterceptedToRemote;
                }
                if (blob->handleLocalQmldirForImport(
                            import, cacheTail->qmldirFilePath, cacheTail->qmldirPathUrl, errors)) {
                    return QmldirFound;
                }
                result = QmldirRejected;
            }
        } while (cacheTail->next && (cacheTail = cacheTail->next));
    }


    // Do not try to construct the cache if it already had any entries for the URI.
    // Otherwise we might duplicate cache entries.
    if (result != QmldirNotFound
            || QQmlMetaType::isStronglyLockedModule(import->uri, import->version)) {
        return result;
    }

    QQmlTypeLoaderConfiguredDataConstPtr configuredData(&m_data);
    const bool hasInterceptors = !configuredData->urlInterceptors.isEmpty();

    // Interceptor might redirect remote files to local ones.
    QStringList localImportPaths = importPathList(hasInterceptors ? LocalOrRemote : Local);

    // Search local import paths for a matching version
    const QStringList qmlDirPaths = QQmlImports::completeQmldirPaths(
            import->uri, localImportPaths, import->version);

    QString qmldirAbsoluteFilePath;
    for (QString qmldirPath : qmlDirPaths) {
        if (hasInterceptors) {
            // TODO:
            // 1. This is inexact. It triggers only on the existence of interceptors, not on
            //    actual interception. If the URL was remote to begin with but no interceptor
            //    actually changes it, we still clear the qmldirPath and consider it
            //    QmldirInterceptedToRemote.
            // 2. This misdiagnosis makes addLibraryImport do the right thing and postpone
            //    the loading of pre-registered types for any QML engine that has interceptors
            //    (even if they don't do anything in this case).
            // Fixing this would open the door to follow-up problems but wouldn't result in any
            // significant benefit.
            const QUrl intercepted = doInterceptUrl(
                    QQmlImports::urlFromLocalFileOrQrcOrUrl(qmldirPath),
                    QQmlAbstractUrlInterceptor::QmldirFile,
                    configuredData->urlInterceptors);
            qmldirPath = QQmlFile::urlToLocalFileOrQrc(intercepted);
            if (result != QmldirInterceptedToRemote
                && qmldirPath.isEmpty()
                && !QQmlFile::isLocalFile(intercepted)) {
                result = QmldirInterceptedToRemote;
            }
        }

        // qmldirPath can be a path or a URL at this point because we've inherited that property
        // from importPathList(). absoluteFilePath() however, wants only paths, not URLs. It may
        // also be empty here if an interceptor redirected it to a non-local URL above.
        qmldirAbsoluteFilePath = absoluteFilePath(localPathForUrlOrPath(qmldirPath));

        if (!qmldirAbsoluteFilePath.isEmpty()) {
            QString url = pathToUrl(qmldirAbsoluteFilePath);
            if (url.startsWith(QStringLiteral("file:")))
                sanitizeUNCPath(&qmldirAbsoluteFilePath);

            QQmlTypeLoaderThreadData::QmldirInfo *cache = new QQmlTypeLoaderThreadData::QmldirInfo;
            cache->version = import->version;
            cache->qmldirFilePath = qmldirAbsoluteFilePath;
            cache->qmldirPathUrl = url;
            cache->next = nullptr;
            if (cacheTail)
                cacheTail->next = cache;
            else
                threadData->qmldirInfo.insert(import->uri, cache);
            cacheTail = cache;

            if (result != QmldirFound) {
                result = blob->handleLocalQmldirForImport(
                                 import, qmldirAbsoluteFilePath, url, errors)
                        ? QmldirFound
                        : QmldirRejected;
            }

            // Do not return here. Rather, construct the complete cache for this URI.
        }
    }

    // Nothing found? Add an empty cache entry to signal that for further requests.
    if (result == QmldirNotFound || result == QmldirInterceptedToRemote) {
        QQmlTypeLoaderThreadData::QmldirInfo *cache = new QQmlTypeLoaderThreadData::QmldirInfo;
        cache->version = import->version;
        cache->next = cacheHead;
        if (result == QmldirInterceptedToRemote) {
            // The actual value doesn't matter as long as it's not empty.
            // We only use it to discern QmldirInterceptedToRemote from QmldirNotFound above.
            cache->qmldirPathUrl = QStringLiteral("intercepted");
        }
        threadData->qmldirInfo.insert(import->uri, cache);

        if (result == QmldirNotFound) {
            qCDebug(lcQmlImport)
                    << "locateLocalQmldir:" << qPrintable(import->uri)
                    << "module's qmldir file not found";
        }
    } else {
        qCDebug(lcQmlImport)
                << "locateLocalQmldir:" << qPrintable(import->uri) << "module's qmldir found at"
                << qmldirAbsoluteFilePath;
    }

    return result;
}

QT_END_NAMESPACE
