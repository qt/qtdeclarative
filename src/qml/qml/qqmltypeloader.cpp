/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qqmltypeloader_p.h>

#include <private/qqmldirdata_p.h>
#include <private/qqmlprofiler_p.h>
#include <private/qqmlscriptblob_p.h>
#include <private/qqmltypedata_p.h>
#include <private/qqmltypeloaderqmldircontent_p.h>
#include <private/qqmltypeloaderthread_p.h>
#include <private/qqmlsourcecoordinate_p.h>

#include <QtQml/qqmlabstracturlinterceptor.h>
#include <QtQml/qqmlengine.h>
#include <QtQml/qqmlextensioninterface.h>
#include <QtQml/qqmlfile.h>

#include <qtqml_tracepoints_p.h>

#include <QtCore/qdir.h>
#include <QtCore/qdiriterator.h>
#include <QtCore/qfile.h>
#include <QtCore/qthread.h>

#include <functional>

// #define DATABLOB_DEBUG
#ifdef DATABLOB_DEBUG
#define ASSERT_LOADTHREAD() do { if (!m_thread->isThisThread()) qFatal("QQmlTypeLoader: Caller not in load thread"); } while (false)
#else
#define ASSERT_LOADTHREAD()
#endif

DEFINE_BOOL_CONFIG_OPTION(disableDiskCache, QML_DISABLE_DISK_CACHE);
DEFINE_BOOL_CONFIG_OPTION(forceDiskCache, QML_FORCE_DISK_CACHE);

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
    if (m_thread) {
        shutdownThread();
        delete m_thread;
        m_thread = nullptr;
    }

#if QT_CONFIG(qml_network)
    // Need to delete the network replies after
    // the loader thread is shutdown as it could be
    // getting new replies while we clear them
    for (NetworkReplies::Iterator iter = m_networkReplies.begin(); iter != m_networkReplies.end(); ++iter)
        (*iter)->release();
    m_networkReplies.clear();
#endif // qml_network
}

#if QT_CONFIG(qml_debug)
void QQmlTypeLoader::setProfiler(QQmlProfiler *profiler)
{
    Q_ASSERT(!m_profiler);
    m_profiler.reset(profiler);
}
#endif

struct PlainLoader {
    void loadThread(QQmlTypeLoader *loader, QQmlDataBlob *blob) const
    {
        loader->loadThread(blob);
    }
    void load(QQmlTypeLoader *loader, QQmlDataBlob *blob) const
    {
        loader->m_thread->load(blob);
    }
    void loadAsync(QQmlTypeLoader *loader, QQmlDataBlob *blob) const
    {
        loader->m_thread->loadAsync(blob);
    }
};

struct StaticLoader {
    const QByteArray &data;
    StaticLoader(const QByteArray &data) : data(data) {}

    void loadThread(QQmlTypeLoader *loader, QQmlDataBlob *blob) const
    {
        loader->loadWithStaticDataThread(blob, data);
    }
    void load(QQmlTypeLoader *loader, QQmlDataBlob *blob) const
    {
        loader->m_thread->loadWithStaticData(blob, data);
    }
    void loadAsync(QQmlTypeLoader *loader, QQmlDataBlob *blob) const
    {
        loader->m_thread->loadWithStaticDataAsync(blob, data);
    }
};

struct CachedLoader {
    const QV4::CompiledData::Unit *unit;
    CachedLoader(const QV4::CompiledData::Unit *unit) :  unit(unit) {}

    void loadThread(QQmlTypeLoader *loader, QQmlDataBlob *blob) const
    {
        loader->loadWithCachedUnitThread(blob, unit);
    }
    void load(QQmlTypeLoader *loader, QQmlDataBlob *blob) const
    {
        loader->m_thread->loadWithCachedUnit(blob, unit);
    }
    void loadAsync(QQmlTypeLoader *loader, QQmlDataBlob *blob) const
    {
        loader->m_thread->loadWithCachedUnitAsync(blob, unit);
    }
};

template<typename Loader>
void QQmlTypeLoader::doLoad(const Loader &loader, QQmlDataBlob *blob, Mode mode)
{
#ifdef DATABLOB_DEBUG
    qWarning("QQmlTypeLoader::doLoad(%s): %s thread", qPrintable(blob->urlString()),
             m_thread->isThisThread()?"Compile":"Engine");
#endif
    blob->startLoading();

    if (m_thread->isThisThread()) {
        unlock();
        loader.loadThread(this, blob);
        lock();
    } else if (mode == Asynchronous) {
        blob->m_data.setIsAsync(true);
        unlock();
        loader.loadAsync(this, blob);
        lock();
    } else {
        unlock();
        loader.load(this, blob);
        lock();
        if (mode == PreferSynchronous) {
            if (!blob->isCompleteOrError())
                blob->m_data.setIsAsync(true);
        } else {
            Q_ASSERT(mode == Synchronous);
            while (!blob->isCompleteOrError()) {
                unlock();
                m_thread->waitForNextMessage();
                lock();
            }
        }
    }
}

/*!
Load the provided \a blob from the network or filesystem.

The loader must be locked.
*/
void QQmlTypeLoader::load(QQmlDataBlob *blob, Mode mode)
{
    doLoad(PlainLoader(), blob, mode);
}

/*!
Load the provided \a blob with \a data.  The blob's URL is not used by the data loader in this case.

The loader must be locked.
*/
void QQmlTypeLoader::loadWithStaticData(QQmlDataBlob *blob, const QByteArray &data, Mode mode)
{
    doLoad(StaticLoader(data), blob, mode);
}

void QQmlTypeLoader::loadWithCachedUnit(QQmlDataBlob *blob, const QV4::CompiledData::Unit *unit, Mode mode)
{
    doLoad(CachedLoader(unit), blob, mode);
}

void QQmlTypeLoader::loadWithStaticDataThread(QQmlDataBlob *blob, const QByteArray &data)
{
    ASSERT_LOADTHREAD();

    setData(blob, data);
}

void QQmlTypeLoader::loadWithCachedUnitThread(QQmlDataBlob *blob, const QV4::CompiledData::Unit *unit)
{
    ASSERT_LOADTHREAD();

    setCachedUnit(blob, unit);
}

void QQmlTypeLoader::loadThread(QQmlDataBlob *blob)
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

    if (QQmlFile::isSynchronous(blob->m_url)) {
        const QString fileName = QQmlFile::urlToLocalFileOrQrc(blob->m_url);
        if (!QQml_isFileCaseCorrect(fileName)) {
            blob->setError(QLatin1String("File name case mismatch"));
            return;
        }

        blob->m_data.setProgress(0xFF);
        if (blob->m_data.isAsync())
            m_thread->callDownloadProgressChanged(blob, 1.);

        setData(blob, fileName);

    } else {
#if QT_CONFIG(qml_network)
        QNetworkReply *reply = m_thread->networkAccessManager()->get(QNetworkRequest(blob->m_url));
        QQmlTypeLoaderNetworkReplyProxy *nrp = m_thread->networkReplyProxy();
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
        qWarning("QQmlDataBlob: requested %s", qPrintable(blob->urlString()));
#endif // DATABLOB_DEBUG
#endif // qml_network
    }
}

#define DATALOADER_MAXIMUM_REDIRECT_RECURSION 16

#ifndef TYPELOADER_MINIMUM_TRIM_THRESHOLD
#define TYPELOADER_MINIMUM_TRIM_THRESHOLD 64
#endif

#if QT_CONFIG(qml_network)
void QQmlTypeLoader::networkReplyFinished(QNetworkReply *reply)
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
            blob->m_finalUrlString.clear();

            QNetworkReply *reply = m_thread->networkAccessManager()->get(QNetworkRequest(url));
            QObject *nrp = m_thread->networkReplyProxy();
            QObject::connect(reply, SIGNAL(finished()), nrp, SLOT(finished()));
            m_networkReplies.insert(reply, blob);
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
        setData(blob, data);
    }

    blob->release();
}

void QQmlTypeLoader::networkReplyProgress(QNetworkReply *reply,
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
#endif // qml_network

/*!
Return the QQmlEngine associated with this loader
*/
QQmlEngine *QQmlTypeLoader::engine() const
{
    return m_engine;
}

/*! \internal
Call the initializeEngine() method on \a iface.  Used by QQmlImportDatabase to ensure it
gets called in the correct thread.
*/
template<class Interface>
void doInitializeEngine(Interface *iface, QQmlTypeLoaderThread *thread, QQmlEngine *engine,
                      const char *uri)
{
    Q_ASSERT(thread->isThisThread() || engine->thread() == QThread::currentThread());

    if (thread->isThisThread()) {
        thread->initializeEngine(iface, uri);
    } else {
        Q_ASSERT(engine->thread() == QThread::currentThread());
        iface->initializeEngine(engine, uri);
    }
}

void QQmlTypeLoader::initializeEngine(QQmlEngineExtensionInterface *iface, const char *uri)
{
    doInitializeEngine(iface, m_thread, engine(), uri);
}

void QQmlTypeLoader::initializeEngine(QQmlExtensionInterface *iface, const char *uri)
{
    doInitializeEngine(iface, m_thread, engine(), uri);
}

void QQmlTypeLoader::setData(QQmlDataBlob *blob, const QByteArray &data)
{
    QQmlDataBlob::SourceCodeData d;
    d.inlineSourceCode = QString::fromUtf8(data);
    d.hasInlineSourceCode = true;
    setData(blob, d);
}

void QQmlTypeLoader::setData(QQmlDataBlob *blob, const QString &fileName)
{
    QQmlDataBlob::SourceCodeData d;
    d.fileInfo = QFileInfo(fileName);
    setData(blob, d);
}

void QQmlTypeLoader::setData(QQmlDataBlob *blob, const QQmlDataBlob::SourceCodeData &d)
{
    Q_TRACE_SCOPE(QQmlCompiling, blob->url());
    QQmlCompilingProfiler prof(profiler(), blob);

    blob->m_inCallback = true;

    blob->dataReceived(d);

    if (!blob->isError() && !blob->isWaiting())
        blob->allDependenciesDone();

    if (blob->status() != QQmlDataBlob::Error)
        blob->m_data.setStatus(QQmlDataBlob::WaitingForDependencies);

    blob->m_inCallback = false;

    blob->tryDone();
}

void QQmlTypeLoader::setCachedUnit(QQmlDataBlob *blob, const QV4::CompiledData::Unit *unit)
{
    Q_TRACE_SCOPE(QQmlCompiling, blob->url());
    QQmlCompilingProfiler prof(profiler(), blob);

    blob->m_inCallback = true;

    blob->initializeFromCachedUnit(unit);

    if (!blob->isError() && !blob->isWaiting())
        blob->allDependenciesDone();

    if (blob->status() != QQmlDataBlob::Error)
        blob->m_data.setStatus(QQmlDataBlob::WaitingForDependencies);

    blob->m_inCallback = false;

    blob->tryDone();
}

void QQmlTypeLoader::shutdownThread()
{
    if (m_thread && !m_thread->isShutdown())
        m_thread->shutdown();
}

QQmlTypeLoader::Blob::PendingImport::PendingImport(QQmlTypeLoader::Blob *blob, const QV4::CompiledData::Import *import)
{
    type = static_cast<QV4::CompiledData::Import::ImportType>(quint32(import->type));
    uri = blob->stringAt(import->uriIndex);
    qualifier = blob->stringAt(import->qualifierIndex);
    majorVersion = import->majorVersion;
    minorVersion = import->minorVersion;
    location = import->location;
}

QQmlTypeLoader::Blob::Blob(const QUrl &url, QQmlDataBlob::Type type, QQmlTypeLoader *loader)
  : QQmlDataBlob(url, type, loader), m_importCache(loader)
{
}

QQmlTypeLoader::Blob::~Blob()
{
}

bool QQmlTypeLoader::Blob::fetchQmldir(const QUrl &url, PendingImportPtr import, int priority, QList<QQmlError> *errors)
{
    QQmlRefPointer<QQmlQmldirData> data = typeLoader()->getQmldir(url);

    data->setImport(this, std::move(import));
    data->setPriority(this, priority);

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

bool QQmlTypeLoader::Blob::updateQmldir(const QQmlRefPointer<QQmlQmldirData> &data, PendingImportPtr import, QList<QQmlError> *errors)
{
    QString qmldirIdentifier = data->urlString();
    QString qmldirUrl = qmldirIdentifier.left(qmldirIdentifier.lastIndexOf(QLatin1Char('/')) + 1);

    typeLoader()->setQmldirContent(qmldirIdentifier, data->content());

    if (!m_importCache.updateQmldirContent(typeLoader()->importDatabase(), import->uri, import->qualifier, qmldirIdentifier, qmldirUrl, errors))
        return false;

    if (!loadImportDependencies(import, qmldirIdentifier, errors))
        return false;

    import->priority = data->priority(this);

    // Release this reference at destruction
    m_qmldirs << data;

    if (!import->qualifier.isEmpty()) {
        // Does this library contain any qualified scripts?
        QUrl libraryUrl(qmldirUrl);
        const QQmlTypeLoaderQmldirContent qmldir = typeLoader()->qmldirContent(qmldirIdentifier);
        const auto qmldirScripts = qmldir.scripts();
        for (const QQmlDirParser::Script &script : qmldirScripts) {
            QUrl scriptUrl = libraryUrl.resolved(QUrl(script.fileName));
            QQmlRefPointer<QQmlScriptBlob> blob = typeLoader()->getScript(scriptUrl);
            addDependency(blob.data());

            scriptImported(blob, import->location, script.nameSpace, import->qualifier);
        }
    }

    return true;
}

bool QQmlTypeLoader::Blob::addImport(const QV4::CompiledData::Import *import, QList<QQmlError> *errors)
{
    return addImport(std::make_shared<PendingImport>(this, import), errors);
}

bool QQmlTypeLoader::Blob::addImport(QQmlTypeLoader::Blob::PendingImportPtr import, QList<QQmlError> *errors)
{
    Q_ASSERT(errors);

    QQmlImportDatabase *importDatabase = typeLoader()->importDatabase();

    if (import->type == QV4::CompiledData::Import::ImportScript) {
        QUrl scriptUrl = finalUrl().resolved(QUrl(import->uri));
        QQmlRefPointer<QQmlScriptBlob> blob = typeLoader()->getScript(scriptUrl);
        addDependency(blob.data());

        scriptImported(blob, import->location, import->qualifier, QString());
    } else if (import->type == QV4::CompiledData::Import::ImportLibrary) {
        QString qmldirFilePath;
        QString qmldirUrl;

        const QQmlImports::LocalQmldirResult qmldirResult = m_importCache.locateLocalQmldir(
                    importDatabase, import->uri, import->majorVersion, import->minorVersion,
                    &qmldirFilePath, &qmldirUrl);
        if (qmldirResult == QQmlImports::QmldirFound) {
            // This is a local library import
            if (!m_importCache.addLibraryImport(importDatabase, import->uri, import->qualifier, import->majorVersion,
                                          import->minorVersion, qmldirFilePath, qmldirUrl, false, errors))
                return false;

            if (!loadImportDependencies(import, qmldirFilePath, errors))
                return false;

            if (!import->qualifier.isEmpty()) {
                // Does this library contain any qualified scripts?
                QUrl libraryUrl(qmldirUrl);
                const QQmlTypeLoaderQmldirContent qmldir = typeLoader()->qmldirContent(qmldirFilePath);
                const auto qmldirScripts = qmldir.scripts();
                for (const QQmlDirParser::Script &script : qmldirScripts) {
                    QUrl scriptUrl = libraryUrl.resolved(QUrl(script.fileName));
                    QQmlRefPointer<QQmlScriptBlob> blob = typeLoader()->getScript(scriptUrl);
                    addDependency(blob.data());

                    scriptImported(blob, import->location, script.nameSpace, import->qualifier);
                }
            }
        } else if (
                // Major version of module already registered:
                // We believe that the registration is complete.
                QQmlMetaType::typeModule(import->uri, import->majorVersion)

                // Otherwise, try to register further module types.
                || (qmldirResult != QQmlImports::QmldirInterceptedToRemote
                    && QQmlMetaType::qmlRegisterModuleTypes(import->uri, import->majorVersion))

                // Otherwise, there is no way to register any further types.
                // Try with any module of that name.
                || QQmlMetaType::isAnyModule(import->uri)) {

            if (!m_importCache.addLibraryImport(
                        importDatabase, import->uri, import->qualifier, import->majorVersion,
                        import->minorVersion, QString(), QString(), false, errors)) {
                return false;
            }
        } else {
            // We haven't yet resolved this import
            m_unresolvedImports << import;

            QQmlAbstractUrlInterceptor *interceptor = typeLoader()->engine()->urlInterceptor();

            // Query any network import paths for this library.
            // Interceptor might redirect local paths.
            QStringList remotePathList = importDatabase->importPathList(
                        interceptor ? QQmlImportDatabase::LocalOrRemote
                                    : QQmlImportDatabase::Remote);
            if (!remotePathList.isEmpty()) {
                // Add this library and request the possible locations for it
                if (!m_importCache.addLibraryImport(importDatabase, import->uri, import->qualifier, import->majorVersion,
                                              import->minorVersion, QString(), QString(), true, errors))
                    return false;

                // Probe for all possible locations
                int priority = 0;
                const QStringList qmlDirPaths = QQmlImports::completeQmldirPaths(import->uri, remotePathList, import->majorVersion, import->minorVersion);
                for (const QString &qmldirPath : qmlDirPaths) {
                    if (interceptor) {
                        QUrl url = interceptor->intercept(
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
        }
    } else {
        Q_ASSERT(import->type == QV4::CompiledData::Import::ImportFile);

        bool incomplete = false;

        QUrl importUrl(import->uri);
        QString path = importUrl.path();
        path.append(QLatin1String(path.endsWith(QLatin1Char('/')) ? "qmldir" : "/qmldir"));
        importUrl.setPath(path);
        QUrl qmldirUrl = finalUrl().resolved(importUrl);
        if (!QQmlImports::isLocal(qmldirUrl)) {
            // This is a remote file; the import is currently incomplete
            incomplete = true;
        }

        if (!m_importCache.addFileImport(importDatabase, import->uri, import->qualifier, import->majorVersion,
                                   import->minorVersion, incomplete, errors))
            return false;

        if (incomplete) {
            if (!fetchQmldir(qmldirUrl, import, 1, errors))
                return false;
        }
    }

    return true;
}

void QQmlTypeLoader::Blob::dependencyComplete(QQmlDataBlob *blob)
{
    if (blob->type() == QQmlDataBlob::QmldirFile) {
        QQmlQmldirData *data = static_cast<QQmlQmldirData *>(blob);

        PendingImportPtr import = data->import(this);

        QList<QQmlError> errors;
        if (!qmldirDataAvailable(data, &errors)) {
            Q_ASSERT(errors.size());
            QQmlError error(errors.takeFirst());
            error.setUrl(m_importCache.baseUrl());
            error.setLine(qmlConvertSourceCoordinate<quint32, int>(import->location.line));
            error.setColumn(qmlConvertSourceCoordinate<quint32, int>(import->location.column));
            errors.prepend(error); // put it back on the list after filling out information.
            setError(errors);
        }
    }
}

bool QQmlTypeLoader::Blob::loadImportDependencies(PendingImportPtr currentImport, const QString &qmldirUri, QList<QQmlError> *errors)
{
    const QQmlTypeLoaderQmldirContent qmldir = typeLoader()->qmldirContent(qmldirUri);
    for (const QString &implicitImports: qmldir.imports()) {
        auto dependencyImport = std::make_shared<PendingImport>();
        dependencyImport->uri = implicitImports;
        dependencyImport->qualifier = currentImport->qualifier;
        dependencyImport->majorVersion = currentImport->majorVersion;
        dependencyImport->minorVersion = currentImport->minorVersion;
        if (!addImport(dependencyImport, errors))
            return false;
    }
    return true;
}

bool QQmlTypeLoader::Blob::isDebugging() const
{
    return typeLoader()->engine()->handle()->debugger() != nullptr;
}

bool QQmlTypeLoader::Blob::diskCacheEnabled() const
{
    return (!disableDiskCache() && !isDebugging()) || forceDiskCache();
}

bool QQmlTypeLoader::Blob::qmldirDataAvailable(const QQmlRefPointer<QQmlQmldirData> &data, QList<QQmlError> *errors)
{
    PendingImportPtr import = data->import(this);
    data->setImport(this, nullptr);

    int priority = data->priority(this);
    data->setPriority(this, 0);

    if (import) {
        // Do we need to resolve this import?
        const bool resolve = (import->priority == 0) || (import->priority > priority);

        if (resolve) {
            // This is the (current) best resolution for this import
            if (!updateQmldir(data, import, errors)) {
                return false;
            }

            import->priority = priority;
            return true;
        }
    }

    return true;
}

/*!
Constructs a new type loader that uses the given \a engine.
*/
QQmlTypeLoader::QQmlTypeLoader(QQmlEngine *engine)
    : m_engine(engine)
    , m_thread(new QQmlTypeLoaderThread(this))
    , m_mutex(m_thread->mutex())
    , m_typeCacheTrimThreshold(TYPELOADER_MINIMUM_TRIM_THRESHOLD)
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

    invalidate();
}

QQmlImportDatabase *QQmlTypeLoader::importDatabase() const
{
    return &QQmlEnginePrivate::get(engine())->importDatabase;
}

QUrl QQmlTypeLoader::normalize(const QUrl &unNormalizedUrl)
{
    QUrl normalized(unNormalizedUrl);
    if (normalized.scheme() == QLatin1String("qrc"))
        normalized.setHost(QString()); // map qrc:///a.qml to qrc:/a.qml
    return normalized;
}

/*!
Returns a QQmlTypeData for the specified \a url.  The QQmlTypeData may be cached.
*/
QQmlRefPointer<QQmlTypeData> QQmlTypeLoader::getType(const QUrl &unNormalizedUrl, Mode mode)
{
    Q_ASSERT(!unNormalizedUrl.isRelative() &&
            (QQmlFile::urlToLocalFileOrQrc(unNormalizedUrl).isEmpty() ||
             !QDir::isRelativePath(QQmlFile::urlToLocalFileOrQrc(unNormalizedUrl))));

    const QUrl url = normalize(unNormalizedUrl);

    LockHolder<QQmlTypeLoader> holder(this);

    QQmlTypeData *typeData = m_typeCache.value(url);

    if (!typeData) {
        // Trim before adding the new type, so that we don't immediately trim it away
        if (m_typeCache.size() >= m_typeCacheTrimThreshold)
            trimCache();

        typeData = new QQmlTypeData(url, this);
        // TODO: if (compiledData == 0), is it safe to omit this insertion?
        m_typeCache.insert(url, typeData);
        QQmlMetaType::CachedUnitLookupError error = QQmlMetaType::CachedUnitLookupError::NoError;
        if (const QV4::CompiledData::Unit *cachedUnit = QQmlMetaType::findCachedCompilationUnit(typeData->url(), &error)) {
            QQmlTypeLoader::loadWithCachedUnit(typeData, cachedUnit, mode);
        } else {
            typeData->setCachedUnitStatus(error);
            QQmlTypeLoader::load(typeData, mode);
        }
    } else if ((mode == PreferSynchronous || mode == Synchronous) && QQmlFile::isSynchronous(url)) {
        // this was started Asynchronous, but we need to force Synchronous
        // completion now (if at all possible with this type of URL).

        if (!m_thread->isThisThread()) {
            // this only works when called directly from the UI thread, but not
            // when recursively called on the QML thread via resolveTypes()

            while (!typeData->isCompleteOrError()) {
                unlock();
                m_thread->waitForNextMessage();
                lock();
            }
        }
    }

    return typeData;
}

/*!
Returns a QQmlTypeData for the given \a data with the provided base \a url.  The
QQmlTypeData will not be cached.
*/
QQmlRefPointer<QQmlTypeData> QQmlTypeLoader::getType(const QByteArray &data, const QUrl &url, Mode mode)
{
    LockHolder<QQmlTypeLoader> holder(this);

    QQmlTypeData *typeData = new QQmlTypeData(url, this);
    QQmlTypeLoader::loadWithStaticData(typeData, data, mode);

    return QQmlRefPointer<QQmlTypeData>(typeData, QQmlRefPointer<QQmlTypeData>::Adopt);
}

/*!
Return a QQmlScriptBlob for \a url.  The QQmlScriptData may be cached.
*/
QQmlRefPointer<QQmlScriptBlob> QQmlTypeLoader::getScript(const QUrl &unNormalizedUrl)
{
    Q_ASSERT(!unNormalizedUrl.isRelative() &&
            (QQmlFile::urlToLocalFileOrQrc(unNormalizedUrl).isEmpty() ||
             !QDir::isRelativePath(QQmlFile::urlToLocalFileOrQrc(unNormalizedUrl))));

    const QUrl url = normalize(unNormalizedUrl);

    LockHolder<QQmlTypeLoader> holder(this);

    QQmlScriptBlob *scriptBlob = m_scriptCache.value(url);

    if (!scriptBlob) {
        scriptBlob = new QQmlScriptBlob(url, this);
        m_scriptCache.insert(url, scriptBlob);

        QQmlMetaType::CachedUnitLookupError error;
        if (const QV4::CompiledData::Unit *cachedUnit = QQmlMetaType::findCachedCompilationUnit(scriptBlob->url(), &error)) {
            QQmlTypeLoader::loadWithCachedUnit(scriptBlob, cachedUnit);
        } else {
            scriptBlob->setCachedUnitStatus(error);
            QQmlTypeLoader::load(scriptBlob);
        }
    }

    return scriptBlob;
}

/*!
Returns a QQmlQmldirData for \a url.  The QQmlQmldirData may be cached.
*/
QQmlRefPointer<QQmlQmldirData> QQmlTypeLoader::getQmldir(const QUrl &url)
{
    Q_ASSERT(!url.isRelative() &&
            (QQmlFile::urlToLocalFileOrQrc(url).isEmpty() ||
             !QDir::isRelativePath(QQmlFile::urlToLocalFileOrQrc(url))));
    LockHolder<QQmlTypeLoader> holder(this);

    QQmlQmldirData *qmldirData = m_qmldirCache.value(url);

    if (!qmldirData) {
        qmldirData = new QQmlQmldirData(url, this);
        m_qmldirCache.insert(url, qmldirData);
        QQmlTypeLoader::load(qmldirData);
    }

    return qmldirData;
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
    } else if (path.count() > 8 && path.at(7) == QLatin1Char(':') && path.at(8) == QLatin1Char('/') &&
           path.startsWith(QLatin1String("content"), Qt::CaseInsensitive)) {
        // content url
        QFileInfo fileInfo(QQmlFile::urlToLocalFileOrQrc(path));
        return fileInfo.isFile() ? fileInfo.absoluteFilePath() : QString();
    }
#endif

    int lastSlash = path.lastIndexOf(QLatin1Char('/'));
    QString dirPath(path.left(lastSlash));

    LockHolder<QQmlTypeLoader> holder(this);
    if (!m_importDirCache.contains(dirPath)) {
        bool exists = QDir(dirPath).exists();
        QCache<QString, bool> *entry = exists ? new QCache<QString, bool> : nullptr;
        m_importDirCache.insert(dirPath, entry);
    }
    QCache<QString, bool> *fileSet = m_importDirCache.object(dirPath);
    if (!fileSet)
        return QString();

    QString absoluteFilePath;
    QString fileName(path.mid(lastSlash+1, path.length()-lastSlash-1));

    bool *value = fileSet->object(fileName);
    if (value) {
        if (*value)
            absoluteFilePath = path;
    } else {
        bool exists = QFile::exists(path);
        fileSet->insert(fileName, new bool(exists));
        if (exists)
            absoluteFilePath = path;
    }

    if (absoluteFilePath.length() > 2 && absoluteFilePath.at(0) != QLatin1Char('/') && absoluteFilePath.at(1) != QLatin1Char(':'))
        absoluteFilePath = QFileInfo(absoluteFilePath).absoluteFilePath();

    return absoluteFilePath;
}

bool QQmlTypeLoader::fileExists(const QString &path, const QString &file)
{
    const QChar nullChar(QChar::Null);
    if (path.isEmpty() || path.contains(nullChar) || file.isEmpty() || file.contains(nullChar))
        return false;

    Q_ASSERT(path.endsWith(QLatin1Char('/')));

    LockHolder<QQmlTypeLoader> holder(this);
    QCache<QString, bool> *fileSet = m_importDirCache.object(path);
    if (fileSet) {
        if (bool *value = fileSet->object(file))
            return *value;
    } else if (m_importDirCache.contains(path)) {
        // explicit nullptr in cache
        return false;
    }

    auto addToCache = [&](const QFileInfo &fileInfo) {
        if (!fileSet) {
            fileSet = fileInfo.dir().exists() ? new QCache<QString, bool> : nullptr;
            m_importDirCache.insert(path, fileSet);
            if (!fileSet)
                return false;
        }

        const bool exists = fileInfo.exists();
        fileSet->insert(file, new bool(exists));
        return exists;
    };

    if (path.at(0) == QLatin1Char(':')) {
        // qrc resource
        return addToCache(QFileInfo(path + file));
    }

    if (path.count() > 3 && path.at(3) == QLatin1Char(':')
            && path.startsWith(QLatin1String("qrc"), Qt::CaseInsensitive)) {
        // qrc resource url
        return addToCache(QFileInfo(QQmlFile::urlToLocalFileOrQrc(path + file)));
    }

#if defined(Q_OS_ANDROID)
    if (path.count() > 7 && path.at(6) == QLatin1Char(':') && path.at(7) == QLatin1Char('/')
            && path.startsWith(QLatin1String("assets"), Qt::CaseInsensitive)) {
        // assets resource url
        return addToCache(QFileInfo(QQmlFile::urlToLocalFileOrQrc(path + file)));
    }

    if (path.count() > 8 && path.at(7) == QLatin1Char(':') && path.at(8) == QLatin1Char('/')
            && path.startsWith(QLatin1String("content"), Qt::CaseInsensitive)) {
        // content url
        return addToCache(QFileInfo(QQmlFile::urlToLocalFileOrQrc(path + file)));
    }
#endif

    return addToCache(QFileInfo(path + file));
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
    isResource = isResource || path.startsWith(QLatin1String("assets:/")) || path.startsWith(QLatin1String("content:/"));
#endif

    if (isResource) {
        // qrc resource
        QFileInfo fileInfo(path);
        return fileInfo.exists() && fileInfo.isDir();
    }

    int length = path.length();
    if (path.endsWith(QLatin1Char('/')))
        --length;
    QString dirPath(path.left(length));

    LockHolder<QQmlTypeLoader> holder(this);
    if (!m_importDirCache.contains(dirPath)) {
        bool exists = QDir(dirPath).exists();
        QCache<QString, bool> *files = exists ? new QCache<QString, bool> : nullptr;
        m_importDirCache.insert(dirPath, files);
    }

    QCache<QString, bool> *fileSet = m_importDirCache.object(dirPath);
    return fileSet != nullptr;
}


/*!
Return a QQmlTypeLoaderQmldirContent for absoluteFilePath.  The QQmlTypeLoaderQmldirContent may be cached.

\a filePath is a local file path.

It can also be a remote path for a remote directory import, but it will have been cached by now in this case.
*/
const QQmlTypeLoaderQmldirContent QQmlTypeLoader::qmldirContent(const QString &filePathIn)
{
    LockHolder<QQmlTypeLoader> holder(this);

    QString filePath;

    // Try to guess if filePathIn is already a URL. This is necessarily fragile, because
    // - paths can contain ':', which might make them appear as URLs with schemes.
    // - windows drive letters appear as schemes (thus "< 2" below).
    // - a "file:" URL is equivalent to the respective file, but will be treated differently.
    // Yet, this heuristic is the best we can do until we pass more structured information here,
    // for example a QUrl also for local files.
    QUrl url(filePathIn);
    if (url.scheme().length() < 2) {
        filePath = filePathIn;
    } else {
        filePath = QQmlFile::urlToLocalFileOrQrc(url);
        if (filePath.isEmpty()) { // Can't load the remote here, but should be cached
            if (auto entry = m_importQmlDirCache.value(filePathIn))
                return **entry;
            else
                return QQmlTypeLoaderQmldirContent();
        }
    }

    QQmlTypeLoaderQmldirContent **val = m_importQmlDirCache.value(filePath);
    if (val)
        return **val;
    QQmlTypeLoaderQmldirContent *qmldir = new QQmlTypeLoaderQmldirContent;

#define ERROR(description) { QQmlError e; e.setDescription(description); qmldir->setError(e); }
#define NOT_READABLE_ERROR QString(QLatin1String("module \"$$URI$$\" definition \"%1\" not readable"))
#define CASE_MISMATCH_ERROR QString(QLatin1String("cannot load module \"$$URI$$\": File name case mismatch for \"%1\""))

    QFile file(filePath);
    if (!QQml_isFileCaseCorrect(filePath)) {
        ERROR(CASE_MISMATCH_ERROR.arg(filePath));
    } else if (file.open(QFile::ReadOnly)) {
        QByteArray data = file.readAll();
        qmldir->setContent(filePath, QString::fromUtf8(data));
    } else {
        ERROR(NOT_READABLE_ERROR.arg(filePath));
    }

#undef ERROR
#undef NOT_READABLE_ERROR
#undef CASE_MISMATCH_ERROR

    m_importQmlDirCache.insert(filePath, qmldir);
    return *qmldir;
}

void QQmlTypeLoader::setQmldirContent(const QString &url, const QString &content)
{
    QQmlTypeLoaderQmldirContent *qmldir;
    QQmlTypeLoaderQmldirContent **val = m_importQmlDirCache.value(url);
    if (val) {
        qmldir = *val;
    } else {
        qmldir = new QQmlTypeLoaderQmldirContent;
        m_importQmlDirCache.insert(url, qmldir);
    }

    if (!qmldir->hasContent())
        qmldir->setContent(url, content);
}

/*!
Clears cached information about loaded files, including any type data, scripts
and qmldir information.
*/
void QQmlTypeLoader::clearCache()
{
    for (TypeCache::Iterator iter = m_typeCache.begin(), end = m_typeCache.end(); iter != end; ++iter)
        (*iter)->release();
    for (ScriptCache::Iterator iter = m_scriptCache.begin(), end = m_scriptCache.end(); iter != end; ++iter)
        (*iter)->release();
    for (QmldirCache::Iterator iter = m_qmldirCache.begin(), end = m_qmldirCache.end(); iter != end; ++iter)
        (*iter)->release();

    qDeleteAll(m_importQmlDirCache);

    m_typeCache.clear();
    m_typeCacheTrimThreshold = TYPELOADER_MINIMUM_TRIM_THRESHOLD;
    m_scriptCache.clear();
    m_qmldirCache.clear();
    m_importDirCache.clear();
    m_importQmlDirCache.clear();
    QQmlMetaType::freeUnusedTypesAndCaches();
}

void QQmlTypeLoader::updateTypeCacheTrimThreshold()
{
    int size = m_typeCache.size();
    if (size > m_typeCacheTrimThreshold)
        m_typeCacheTrimThreshold = size * 2;
    if (size < m_typeCacheTrimThreshold / 2)
        m_typeCacheTrimThreshold = qMax(size * 2, TYPELOADER_MINIMUM_TRIM_THRESHOLD);
}

void QQmlTypeLoader::trimCache()
{
    while (true) {
        QList<TypeCache::Iterator> unneededTypes;
        for (TypeCache::Iterator iter = m_typeCache.begin(), end = m_typeCache.end(); iter != end; ++iter)  {
            QQmlTypeData *typeData = iter.value();

            // typeData->m_compiledData may be set early on in the proccess of loading a file, so
            // it's important to check the general loading status of the typeData before making any
            // other decisions.
            if (typeData->count() == 1 && (typeData->isError() || typeData->isComplete())
                    && (!typeData->m_compiledData || typeData->m_compiledData->count() == 1)) {
                // There are no live objects of this type
                unneededTypes.append(iter);
            }
        }

        if (unneededTypes.isEmpty())
            break;

        while (!unneededTypes.isEmpty()) {
            TypeCache::Iterator iter = unneededTypes.takeLast();

            iter.value()->release();
            m_typeCache.erase(iter);
        }
    }

    updateTypeCacheTrimThreshold();

    QQmlMetaType::freeUnusedTypesAndCaches();

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

QT_END_NAMESPACE
