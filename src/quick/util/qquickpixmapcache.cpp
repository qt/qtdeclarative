// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuick/private/qquickpixmapcache_p.h>
#include <QtQuick/private/qquickimageprovider_p.h>
#include <QtQuick/private/qquickprofiler_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <QtQuick/private/qsgrenderer_p.h>
#include <QtQuick/private/qsgtexturereader_p.h>
#include <QtQuick/qquickwindow.h>

#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qimage_p.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/qimagereader.h>
#include <QtGui/qpixmapcache.h>

#include <QtQml/private/qqmlglobal_p.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/qqmlfile.h>

#include <QtCore/private/qobject_p.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qhash.h>
#include <QtCore/qfile.h>
#include <QtCore/qthread.h>
#include <QtCore/qmutex.h>
#include <QtCore/qbuffer.h>
#include <QtCore/qdebug.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qscopeguard.h>

#if QT_CONFIG(qml_network)
#include <QtQml/qqmlnetworkaccessmanagerfactory.h>
#include <QtNetwork/qnetworkreply.h>
#include <QtNetwork/qsslerror.h>
#endif

#include <private/qdebug_p.h>

#define IMAGEREQUEST_MAX_NETWORK_REQUEST_COUNT 8

// After QQuickPixmapCache::unreferencePixmap() it may get deleted via a timer in 30 seconds
#define CACHE_EXPIRE_TIME 30

// How many (1/4) of the unreferenced pixmaps to delete in QQuickPixmapCache::timerEvent()
#define CACHE_REMOVAL_FRACTION 4

#define PIXMAP_PROFILE(Code) Q_QUICK_PROFILE(QQuickProfiler::ProfilePixmapCache, Code)

#if QT_CONFIG(thread) && !defined(Q_OS_WASM)
#  define USE_THREADED_DOWNLOAD 1
#else
#  define USE_THREADED_DOWNLOAD 0
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::Literals::StringLiterals;

Q_DECLARE_LOGGING_CATEGORY(lcQsgLeak)

#if defined(QT_DEBUG) && QT_CONFIG(thread)
class ThreadAffinityMarker
{
public:
    ThreadAffinityMarker() { attachToCurrentThread(); }

    void assertOnAssignedThread()
    {
        QMutexLocker locker(&m_mutex);
        if (!m_assignedThread)
            attachToCurrentThread();
        Q_ASSERT_X(m_assignedThread == QThread::currentThreadId(), Q_FUNC_INFO,
                   "Running on a wrong thread!");
    }

    void detachFromCurrentThread()
    {
        QMutexLocker locker(&m_mutex);
        m_assignedThread = nullptr;
    }

    void attachToCurrentThread() { m_assignedThread = QThread::currentThreadId(); }

private:
    Qt::HANDLE m_assignedThread;
    QMutex m_mutex;
};
#  define Q_THREAD_AFFINITY_MARKER(x) ThreadAffinityMarker x
#  define Q_ASSERT_CALLED_ON_VALID_THREAD(x) x.assertOnAssignedThread()
#  define Q_DETACH_THREAD_AFFINITY_MARKER(x) x.detachFromCurrentThread()
#else
#  define Q_THREAD_AFFINITY_MARKER(x)
#  define Q_ASSERT_CALLED_ON_VALID_THREAD(x)
#  define Q_DETACH_THREAD_AFFINITY_MARKER(x)
#endif

const QLatin1String QQuickPixmap::itemGrabberScheme = QLatin1String("itemgrabber");

Q_LOGGING_CATEGORY(lcImg, "qt.quick.image")

/*! \internal
    The maximum currently-unused image data that can be stored for potential
    later reuse, in bytes. See QQuickPixmapCache::shrinkCache()
*/
static int cache_limit = 2048 * 1024;

static inline QString imageProviderId(const QUrl &url)
{
    return url.host();
}

static inline QString imageId(const QUrl &url)
{
    return url.toString(QUrl::RemoveScheme | QUrl::RemoveAuthority).mid(1);
}

QQuickDefaultTextureFactory::QQuickDefaultTextureFactory(const QImage &image)
{
    if (image.format() == QImage::Format_ARGB32_Premultiplied
            || image.format() == QImage::Format_RGB32) {
        im = image;
    } else {
        im = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    }
    size = im.size();
}


QSGTexture *QQuickDefaultTextureFactory::createTexture(QQuickWindow *window) const
{
    QSGTexture *t = window->createTextureFromImage(im, QQuickWindow::TextureCanUseAtlas);
    static bool transient = qEnvironmentVariableIsSet("QSG_TRANSIENT_IMAGES");
    if (transient)
        const_cast<QQuickDefaultTextureFactory *>(this)->im = QImage();
    return t;
}

class QQuickPixmapReader;
class QQuickPixmapData;
class QQuickPixmapReply : public QObject
{
    Q_OBJECT
public:
    enum ReadError { NoError, Loading, Decoding };

    QQuickPixmapReply(QQuickPixmapData *);
    ~QQuickPixmapReply();

    QQuickPixmapData *data;
    QQmlEngine *engineForReader; // always access reader inside readerMutex
    QRect requestRegion;
    QSize requestSize;
    QUrl url;

    bool loading;
    QQuickImageProviderOptions providerOptions;

    class Event : public QEvent {
        Q_EVENT_DISABLE_COPY(Event);
    public:
        Event(ReadError, const QString &, const QSize &, QQuickTextureFactory *factory);
        ~Event();

        ReadError error;
        QString errorString;
        QSize implicitSize;
        QQuickTextureFactory *textureFactory;
    };
    void postReply(ReadError, const QString &, const QSize &, QQuickTextureFactory *factory);


Q_SIGNALS:
    void finished();
    void downloadProgress(qint64, qint64);

protected:
    bool event(QEvent *event) override;

private:
    Q_DISABLE_COPY(QQuickPixmapReply)

public:
    static int finishedMethodIndex;
    static int downloadProgressMethodIndex;
};

/*! \internal
    Serves as an endpoint for notifications on the connected reader's thread, thus enforcing
    execution of their continuation on the thread. */
class ReaderThreadExecutionEnforcer : public QObject
{
    Q_OBJECT
public:
    enum Event {
        ProcessJobs = QEvent::User,
    };

    ReaderThreadExecutionEnforcer(QQuickPixmapReader *reader);

    /*! \internal
        Forces the execution of processJobs() on the original reader on the thread it's running on.
    */
    void processJobsOnReaderThreadLater();

public slots:
    void asyncResponseFinished(QQuickImageResponse *response);
    void asyncResponseFinished();
private slots:
    void networkRequestDone();
private:
    Q_DISABLE_COPY(ReaderThreadExecutionEnforcer)
    bool event(QEvent *e) override;

    QQuickPixmapReader *reader;
};

class QQuickPixmapData;
class QQuickPixmapReader : public QThread
{
    Q_OBJECT
public:
    QQuickPixmapReader(QQmlEngine *eng);
    ~QQuickPixmapReader();

    QQuickPixmapReply *getImage(QQuickPixmapData *);
    void cancel(QQuickPixmapReply *rep);

    static QQuickPixmapReader *instance(QQmlEngine *engine);
    static QQuickPixmapReader *existingInstance(QQmlEngine *engine);
    void startJob(QQuickPixmapReply *job);

protected:
    void run() override;

private:
    Q_DISABLE_COPY(QQuickPixmapReader)
    friend class ReaderThreadExecutionEnforcer;
    void processJobs();
    void processJob(QQuickPixmapReply *, const QUrl &, const QString &, QQuickImageProvider::ImageType, const QSharedPointer<QQuickImageProvider> &);
#if QT_CONFIG(qml_network)
    void networkRequestDone(QNetworkReply *);
#endif
    void asyncResponseFinished(QQuickImageResponse *);

    QList<QQuickPixmapReply*> jobs;
    QList<QQuickPixmapReply *> cancelledJobs;
    QQmlEngine *engine;

#if QT_CONFIG(quick_pixmap_cache_threaded_download)
    /*! \internal
        Returns a pointer to the thread object owned by the run loop in QQuickPixmapReader::run.
     */
    ReaderThreadExecutionEnforcer *readerThreadExecutionEnforcer()
    {
        return runLoopReaderThreadExecutionEnforcer;
    }
    QObject *eventLoopQuitHack;
    QMutex mutex;
    ReaderThreadExecutionEnforcer *runLoopReaderThreadExecutionEnforcer = nullptr;
#else
    /*! \internal
        Returns a pointer to the thread object owned by this instance.
     */
    ReaderThreadExecutionEnforcer *readerThreadExecutionEnforcer()
    {
        return ownedReaderThreadExecutionEnforcer.get();
    }
    std::unique_ptr<ReaderThreadExecutionEnforcer> ownedReaderThreadExecutionEnforcer;
#endif

#if QT_CONFIG(qml_network)
    QNetworkAccessManager *networkAccessManager();
    QNetworkAccessManager *accessManager;
    QHash<QNetworkReply*,QQuickPixmapReply*> networkJobs;
#endif
    QHash<QQuickImageResponse*,QQuickPixmapReply*> asyncResponses;

    Q_THREAD_AFFINITY_MARKER(m_creatorThreadAffinityMarker);
    Q_THREAD_AFFINITY_MARKER(m_readerThreadAffinityMarker);

    static int replyDownloadProgressMethodIndex;
    static int replyFinishedMethodIndex;
    static int downloadProgressMethodIndex;
    static int threadNetworkRequestDoneMethodIndex;
    static QHash<QQmlEngine *,QQuickPixmapReader*> readers;
public:
    static QMutex readerMutex;
};

#if QT_CONFIG(quick_pixmap_cache_threaded_download)
#  define PIXMAP_READER_LOCK() QMutexLocker locker(&mutex)
#else
#  define PIXMAP_READER_LOCK()
#endif

class QQuickPixmapCache;

/*! \internal
    The private storage for QQuickPixmap.
*/
class QQuickPixmapData
{
public:
    QQuickPixmapData(const QUrl &u, const QRect &r, const QSize &rs,
                     const QQuickImageProviderOptions &po, const QString &e)
    : refCount(1), frameCount(1), frame(0), inCache(false), fromSpecialDevice(false), pixmapStatus(QQuickPixmap::Error),
      url(u), errorString(e), requestRegion(r), requestSize(rs),
      providerOptions(po), appliedTransform(QQuickImageProviderOptions::UsePluginDefaultTransform),
      textureFactory(nullptr), reply(nullptr), prevUnreferenced(nullptr),
      prevUnreferencedPtr(nullptr), nextUnreferenced(nullptr)
#ifdef Q_OS_WEBOS
    , storeToCache(true)
#endif
    {
    }

    QQuickPixmapData(const QUrl &u, const QRect &r, const QSize &s, const QQuickImageProviderOptions &po,
                     QQuickImageProviderOptions::AutoTransform aTransform, int frame=0, int frameCount=1)
    : refCount(1), frameCount(frameCount), frame(frame), inCache(false), fromSpecialDevice(false), pixmapStatus(QQuickPixmap::Loading),
      url(u), requestRegion(r), requestSize(s),
      providerOptions(po), appliedTransform(aTransform),
      textureFactory(nullptr), reply(nullptr), prevUnreferenced(nullptr), prevUnreferencedPtr(nullptr),
      nextUnreferenced(nullptr)
#ifdef Q_OS_WEBOS
    , storeToCache(true)
#endif
    {
    }

    QQuickPixmapData(const QUrl &u, QQuickTextureFactory *texture,
                     const QSize &s, const QRect &r, const QSize &rs, const QQuickImageProviderOptions &po,
                     QQuickImageProviderOptions::AutoTransform aTransform, int frame=0, int frameCount=1)
    : refCount(1), frameCount(frameCount), frame(frame), inCache(false), fromSpecialDevice(false), pixmapStatus(QQuickPixmap::Ready),
      url(u), implicitSize(s), requestRegion(r), requestSize(rs),
      providerOptions(po), appliedTransform(aTransform),
      textureFactory(texture), reply(nullptr), prevUnreferenced(nullptr),
      prevUnreferencedPtr(nullptr), nextUnreferenced(nullptr)
#ifdef Q_OS_WEBOS
    , storeToCache(true)
#endif
    {
    }

    QQuickPixmapData(QQuickTextureFactory *texture)
    : refCount(1), frameCount(1), frame(0), inCache(false), fromSpecialDevice(false), pixmapStatus(QQuickPixmap::Ready),
      appliedTransform(QQuickImageProviderOptions::UsePluginDefaultTransform),
      textureFactory(texture), reply(nullptr), prevUnreferenced(nullptr),
      prevUnreferencedPtr(nullptr), nextUnreferenced(nullptr)
#ifdef Q_OS_WEBOS
    , storeToCache(true)
#endif
    {
        if (texture)
            requestSize = implicitSize = texture->textureSize();
    }

    ~QQuickPixmapData()
    {
        delete textureFactory;
    }

    int cost() const;
    void addref();
    void release(QQuickPixmapCache *store = nullptr);
    void addToCache();
    void removeFromCache(QQuickPixmapCache *store = nullptr);

    uint refCount;
    int frameCount;
    int frame;

    bool inCache:1;
    bool fromSpecialDevice:1;

    QQuickPixmap::Status pixmapStatus;
    QUrl url;
    QString errorString;
    QSize implicitSize;
    QRect requestRegion;
    QSize requestSize;
    QQuickImageProviderOptions providerOptions;
    QQuickImageProviderOptions::AutoTransform appliedTransform;
    QColorSpace targetColorSpace;

    QPointer<QIODevice> specialDevice;

    // actual image data, after loading
    QQuickTextureFactory *textureFactory;

    QQuickPixmapReply *reply;

    // prev/next pointers to form a linked list for dereferencing pixmaps that are currently unused
    // (those get lazily deleted in QQuickPixmapCache::shrinkCache())
    QQuickPixmapData *prevUnreferenced;
    QQuickPixmapData**prevUnreferencedPtr;
    QQuickPixmapData *nextUnreferenced;

#ifdef Q_OS_WEBOS
    bool storeToCache;
#endif

private:
    Q_DISABLE_COPY(QQuickPixmapData)
};

int QQuickPixmapReply::finishedMethodIndex = -1;
int QQuickPixmapReply::downloadProgressMethodIndex = -1;

// XXX
QHash<QQmlEngine *,QQuickPixmapReader*> QQuickPixmapReader::readers;
QMutex QQuickPixmapReader::readerMutex;

int QQuickPixmapReader::replyDownloadProgressMethodIndex = -1;
int QQuickPixmapReader::replyFinishedMethodIndex = -1;
int QQuickPixmapReader::downloadProgressMethodIndex = -1;
int QQuickPixmapReader::threadNetworkRequestDoneMethodIndex = -1;

void QQuickPixmapReply::postReply(ReadError error, const QString &errorString,
                                        const QSize &implicitSize, QQuickTextureFactory *factory)
{
    loading = false;
    QCoreApplication::postEvent(this, new Event(error, errorString, implicitSize, factory));
}

QQuickPixmapReply::Event::Event(ReadError e, const QString &s, const QSize &iSize, QQuickTextureFactory *factory)
    : QEvent(QEvent::User), error(e), errorString(s), implicitSize(iSize), textureFactory(factory)
{
}

QQuickPixmapReply::Event::~Event()
{
    delete textureFactory;
}

#if QT_CONFIG(qml_network)
QNetworkAccessManager *QQuickPixmapReader::networkAccessManager()
{
    if (!accessManager) {
        Q_ASSERT(readerThreadExecutionEnforcer());
        accessManager = QQmlEnginePrivate::get(engine)->createNetworkAccessManager(
                readerThreadExecutionEnforcer());
    }
    return accessManager;
}
#endif

static void maybeRemoveAlpha(QImage *image)
{
    // If the image
    if (image->hasAlphaChannel() && image->data_ptr()
            && !image->data_ptr()->checkForAlphaPixels()) {
        switch (image->format()) {
        case QImage::Format_RGBA8888:
        case QImage::Format_RGBA8888_Premultiplied:
            if (image->data_ptr()->convertInPlace(QImage::Format_RGBX8888, Qt::AutoColor))
                break;

            *image = image->convertToFormat(QImage::Format_RGBX8888);
            break;
        case QImage::Format_A2BGR30_Premultiplied:
            if (image->data_ptr()->convertInPlace(QImage::Format_BGR30, Qt::AutoColor))
                break;

            *image = image->convertToFormat(QImage::Format_BGR30);
            break;
        case QImage::Format_A2RGB30_Premultiplied:
            if (image->data_ptr()->convertInPlace(QImage::Format_RGB30, Qt::AutoColor))
                break;

            *image = image->convertToFormat(QImage::Format_RGB30);
            break;
        default:
            if (image->data_ptr()->convertInPlace(QImage::Format_RGB32, Qt::AutoColor))
                break;

            *image = image->convertToFormat(QImage::Format_RGB32);
            break;
        }
    }
}

static bool readImage(const QUrl& url, QIODevice *dev, QImage *image, QString *errorString, QSize *impsize, int *frameCount,
                      const QRect &requestRegion, const QSize &requestSize, const QQuickImageProviderOptions &providerOptions,
                      QQuickImageProviderOptions::AutoTransform *appliedTransform = nullptr, int frame = 0,
                      qreal devicePixelRatio = 1.0)
{
    QImageReader imgio(dev);
    if (providerOptions.autoTransform() != QQuickImageProviderOptions::UsePluginDefaultTransform)
        imgio.setAutoTransform(providerOptions.autoTransform() == QQuickImageProviderOptions::ApplyTransform);
    else if (appliedTransform)
        *appliedTransform = imgio.autoTransform() ? QQuickImageProviderOptions::ApplyTransform : QQuickImageProviderOptions::DoNotApplyTransform;

    if (frame < imgio.imageCount())
        imgio.jumpToImage(frame);

    if (frameCount)
        *frameCount = imgio.imageCount();

    QSize scSize = QQuickImageProviderWithOptions::loadSize(imgio.size(), requestSize, imgio.format(), providerOptions, devicePixelRatio);
    if (scSize.isValid())
        imgio.setScaledSize(scSize);
    if (!requestRegion.isNull())
        imgio.setScaledClipRect(requestRegion);
    const QSize originalSize = imgio.size();
    qCDebug(lcImg) << url << "frame" << frame << "of" << imgio.imageCount()
                   << "requestRegion" << requestRegion << "QImageReader size" << originalSize << "-> scSize" << scSize;

    if (impsize)
        *impsize = originalSize;

    if (imgio.read(image)) {
        maybeRemoveAlpha(image);
        if (impsize && impsize->width() < 0)
            *impsize = image->size();
        if (providerOptions.targetColorSpace().isValid()) {
            if (image->colorSpace().isValid())
                image->convertToColorSpace(providerOptions.targetColorSpace());
            else
                image->setColorSpace(providerOptions.targetColorSpace());
        }
        return true;
    } else {
        if (errorString)
            *errorString = QQuickPixmap::tr("Error decoding: %1: %2").arg(url.toString())
                                .arg(imgio.errorString());
        return false;
    }
}

static QStringList fromLatin1List(const QList<QByteArray> &list)
{
    QStringList res;
    res.reserve(list.size());
    for (const QByteArray &item : list)
        res.append(QString::fromLatin1(item));
    return res;
}

class BackendSupport
{
public:
    BackendSupport()
    {
        delete QSGContext::createTextureFactoryFromImage(QImage());  // Force init of backend data
        hasOpenGL = QQuickWindow::sceneGraphBackend().isEmpty();     // i.e. default
        QList<QByteArray> list;
        if (hasOpenGL)
            list.append(QSGTextureReader::supportedFileFormats());
        list.append(QImageReader::supportedImageFormats());
        fileSuffixes = fromLatin1List(list);
    }
    bool hasOpenGL;
    QStringList fileSuffixes;
private:
    Q_DISABLE_COPY(BackendSupport)
};
Q_GLOBAL_STATIC(BackendSupport, backendSupport);

static QString existingImageFileForPath(const QString &localFile)
{
    // Do nothing if given filepath exists or already has a suffix
    QFileInfo fi(localFile);
    if (!fi.suffix().isEmpty() || fi.exists())
        return localFile;

    QString tryFile = localFile + QStringLiteral(".xxxx");
    const int suffixIdx = localFile.size() + 1;
    for (const QString &suffix : backendSupport()->fileSuffixes) {
        tryFile.replace(suffixIdx, 10, suffix);
        if (QFileInfo::exists(tryFile))
            return tryFile;
    }
    return localFile;
}

QQuickPixmapReader::QQuickPixmapReader(QQmlEngine *eng)
: QThread(eng), engine(eng)
#if QT_CONFIG(qml_network)
, accessManager(nullptr)
#endif
{
    Q_DETACH_THREAD_AFFINITY_MARKER(m_readerThreadAffinityMarker);
#if QT_CONFIG(quick_pixmap_cache_threaded_download)
    eventLoopQuitHack = new QObject;
    eventLoopQuitHack->moveToThread(this);
    QObject::connect(eventLoopQuitHack, &QObject::destroyed, this, &QThread::quit, Qt::DirectConnection);
    start(QThread::LowestPriority);
#else
    run(); // Call nonblocking run for ourselves.
#endif
}

QQuickPixmapReader::~QQuickPixmapReader()
{
    Q_ASSERT_CALLED_ON_VALID_THREAD(m_creatorThreadAffinityMarker);

    readerMutex.lock();
    readers.remove(engine);
    readerMutex.unlock();

    {
        PIXMAP_READER_LOCK();
        // manually cancel all outstanding jobs.
        for (QQuickPixmapReply *reply : std::as_const(jobs)) {
            if (reply->data && reply->data->reply == reply)
                reply->data->reply = nullptr;
            delete reply;
        }
        jobs.clear();
        const auto cancelJob = [this](QQuickPixmapReply *reply) {
            if (reply->loading) {
                cancelledJobs.append(reply);
                reply->data = nullptr;
            }
        };
#if QT_CONFIG(qml_network)
        for (auto *reply : std::as_const(networkJobs))
            cancelJob(reply);
#endif
        for (auto *reply : std::as_const(asyncResponses))
            cancelJob(reply);
#if !QT_CONFIG(quick_pixmap_cache_threaded_download)
    // In this case we won't be waiting, but we are on the correct thread already, so we can
    // perform housekeeping synchronously now.
    processJobs();
#else  // QT_CONFIG(quick_pixmap_cache_threaded_download) is true
    // Perform housekeeping on all the jobs cancelled above soon...
    if (readerThreadExecutionEnforcer())
        readerThreadExecutionEnforcer()->processJobsOnReaderThreadLater();
#endif
    }

#if QT_CONFIG(quick_pixmap_cache_threaded_download)
    // ... schedule stopping of this thread via the eventLoopQuitHack (processJobs scheduled above
    // will run first) ...
    eventLoopQuitHack->deleteLater();
    // ... and wait() for it all to finish, as the thread will only quit after eventLoopQuitHack
    // has been deleted.
    wait();
#endif

    // While we've been waiting, the other thread may have added
    // more replies. No one will care about them anymore.

    auto deleteReply = [](QQuickPixmapReply *reply) {
        if (reply->data && reply->data->reply == reply)
            reply->data->reply = nullptr;
        delete reply;
    };
#if QT_CONFIG(qml_network)
    for (QQuickPixmapReply *reply : std::as_const(networkJobs))
        deleteReply(reply);
#endif
    for (QQuickPixmapReply *reply : std::as_const(asyncResponses))
        deleteReply(reply);

#if QT_CONFIG(qml_network)
    networkJobs.clear();
#endif
    asyncResponses.clear();
}

#if QT_CONFIG(qml_network)
void QQuickPixmapReader::networkRequestDone(QNetworkReply *reply)
{
    Q_ASSERT_CALLED_ON_VALID_THREAD(m_readerThreadAffinityMarker);

    QQuickPixmapReply *job = networkJobs.take(reply);

    if (job) {
        QImage image;
        QQuickPixmapReply::ReadError error = QQuickPixmapReply::NoError;
        QString errorString;
        QSize readSize;
        QQuickTextureFactory *factory = nullptr;
        if (reply->error()) {
            error = QQuickPixmapReply::Loading;
            errorString = reply->errorString();
        } else {
            QByteArray all = reply->readAll();
            QBuffer buff(&all);
            buff.open(QIODevice::ReadOnly);
            QSGTextureReader texReader(&buff, reply->url().fileName());
            if (backendSupport()->hasOpenGL && texReader.isTexture()) {
                factory = texReader.read();
                if (factory) {
                    readSize = factory->textureSize();
                } else {
                    error = QQuickPixmapReply::Decoding;
                    errorString = QQuickPixmap::tr("Error decoding: %1").arg(reply->url().toString());
                }
            } else {
                int frameCount;
                int const frame = job->data ? job->data->frame : 0;
                if (!readImage(reply->url(), &buff, &image, &errorString, &readSize, &frameCount,
                               job->requestRegion, job->requestSize, job->providerOptions, nullptr, frame))
                    error = QQuickPixmapReply::Decoding;
                else if (job->data)
                    job->data->frameCount = frameCount;
            }
        }
        // send completion event to the QQuickPixmapReply
        if (!factory)
            factory = QQuickTextureFactory::textureFactoryForImage(image);

        PIXMAP_READER_LOCK();
        if (!cancelledJobs.contains(job))
            job->postReply(error, errorString, readSize, factory);
    }
    reply->deleteLater();

    // kick off event loop again in case we have dropped below max request count
    readerThreadExecutionEnforcer()->processJobsOnReaderThreadLater();
}
#endif // qml_network

void QQuickPixmapReader::asyncResponseFinished(QQuickImageResponse *response)
{
    Q_ASSERT_CALLED_ON_VALID_THREAD(m_readerThreadAffinityMarker);

    QQuickPixmapReply *job = asyncResponses.take(response);

    if (job) {
        QQuickTextureFactory *t = nullptr;
        QQuickPixmapReply::ReadError error = QQuickPixmapReply::NoError;
        QString errorString;
        if (!response->errorString().isEmpty()) {
            error = QQuickPixmapReply::Loading;
            errorString = response->errorString();
        } else {
            t = response->textureFactory();
        }

        PIXMAP_READER_LOCK();
        if (!cancelledJobs.contains(job))
            job->postReply(error, errorString, t ? t->textureSize() : QSize(), t);
        else
            delete t;
    }
    response->deleteLater();

    // kick off event loop again in case we have dropped below max request count
    readerThreadExecutionEnforcer()->processJobsOnReaderThreadLater();
}

ReaderThreadExecutionEnforcer::ReaderThreadExecutionEnforcer(QQuickPixmapReader *i) : reader(i) { }

void ReaderThreadExecutionEnforcer::processJobsOnReaderThreadLater()
{
    QCoreApplication::postEvent(
            this, new QEvent(QEvent::Type(ReaderThreadExecutionEnforcer::ProcessJobs)));
}

bool ReaderThreadExecutionEnforcer::event(QEvent *e)
{
    switch (e->type()) {
    case QEvent::Type(ReaderThreadExecutionEnforcer::ProcessJobs):
        reader->processJobs();
        return true;
    default:
        return QObject::event(e);
    }
}

void ReaderThreadExecutionEnforcer::networkRequestDone()
{
#if QT_CONFIG(qml_network)
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reader->networkRequestDone(reply);
#endif
}

void ReaderThreadExecutionEnforcer::asyncResponseFinished(QQuickImageResponse *response)
{
    reader->asyncResponseFinished(response);
}

void ReaderThreadExecutionEnforcer::asyncResponseFinished()
{
    QQuickImageResponse *response = static_cast<QQuickImageResponse *>(sender());
    asyncResponseFinished(response);
}

void QQuickPixmapReader::processJobs()
{
    Q_ASSERT_CALLED_ON_VALID_THREAD(m_readerThreadAffinityMarker);

    PIXMAP_READER_LOCK();
    while (true) {
        if (cancelledJobs.isEmpty() && jobs.isEmpty())
            return; // Nothing else to do

        // Clean cancelled jobs
        if (!cancelledJobs.isEmpty()) {
            for (int i = 0; i < cancelledJobs.size(); ++i) {
                QQuickPixmapReply *job = cancelledJobs.at(i);
#if QT_CONFIG(qml_network)
                QNetworkReply *reply = networkJobs.key(job, 0);
                if (reply) {
                    networkJobs.remove(reply);
                    if (reply->isRunning()) {
                        // cancel any jobs already started
                        reply->close();
                    }
                } else
#endif
                {
                    QQuickImageResponse *asyncResponse = asyncResponses.key(job);
                    if (asyncResponse) {
                        asyncResponses.remove(asyncResponse);
                        asyncResponse->cancel();
                    }
                }
                PIXMAP_PROFILE(pixmapStateChanged<QQuickProfiler::PixmapLoadingError>(job->url));
                // deleteLater, since not owned by this thread
                job->deleteLater();
            }
            cancelledJobs.clear();
        }

        if (!jobs.isEmpty()) {
            // Find a job we can use
            bool usableJob = false;
            for (int i = jobs.size() - 1; !usableJob && i >= 0; i--) {
                QQuickPixmapReply *job = jobs.at(i);
                const QUrl url = job->url;
                QString localFile;
                QQuickImageProvider::ImageType imageType = QQuickImageProvider::Invalid;
                QSharedPointer<QQuickImageProvider> provider;

                if (url.scheme() == QLatin1String("image")) {
                    QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);
                    provider = enginePrivate->imageProvider(imageProviderId(url)).staticCast<QQuickImageProvider>();
                    if (provider)
                        imageType = provider->imageType();

                    usableJob = true;
                } else {
                    localFile = QQmlFile::urlToLocalFileOrQrc(url);
                    usableJob = !localFile.isEmpty()
#if QT_CONFIG(qml_network)
                            || networkJobs.size() < IMAGEREQUEST_MAX_NETWORK_REQUEST_COUNT
#endif
                            ;
                }

                if (usableJob) {
                    jobs.removeAt(i);

                    job->loading = true;

                    PIXMAP_PROFILE(pixmapStateChanged<QQuickProfiler::PixmapLoadingStarted>(url));

#if QT_CONFIG(quick_pixmap_cache_threaded_download)
                    locker.unlock();
                    auto relockMutexGuard = qScopeGuard(([&locker]() {
                        locker.relock();
                    }));
#endif
                    processJob(job, url, localFile, imageType, provider);
                }
            }

            if (!usableJob)
                return;
        }
    }
}

void QQuickPixmapReader::processJob(QQuickPixmapReply *runningJob, const QUrl &url, const QString &localFile,
                                    QQuickImageProvider::ImageType imageType, const QSharedPointer<QQuickImageProvider> &provider)
{
    Q_ASSERT_CALLED_ON_VALID_THREAD(m_readerThreadAffinityMarker);

    // fetch
    if (url.scheme() == QLatin1String("image")) {
        // Use QQuickImageProvider
        QSize readSize;

        if (imageType == QQuickImageProvider::Invalid) {
            QString errorStr = QQuickPixmap::tr("Invalid image provider: %1").arg(url.toString());
            PIXMAP_READER_LOCK();
            if (!cancelledJobs.contains(runningJob))
                runningJob->postReply(QQuickPixmapReply::Loading, errorStr, readSize, nullptr);
            return;
        }

        // This is safe because we ensure that provider does outlive providerV2 and it does not escape the function
        QQuickImageProviderWithOptions *providerV2 = QQuickImageProviderWithOptions::checkedCast(provider.get());

        switch (imageType) {
            case QQuickImageProvider::Invalid:
            {
                // Already handled
                break;
            }

            case QQuickImageProvider::Image:
            {
                QImage image;
                if (providerV2) {
                    image = providerV2->requestImage(imageId(url), &readSize, runningJob->requestSize, runningJob->providerOptions);
                } else {
                    image = provider->requestImage(imageId(url), &readSize, runningJob->requestSize);
                }
                QQuickPixmapReply::ReadError errorCode = QQuickPixmapReply::NoError;
                QString errorStr;
                if (image.isNull()) {
                    errorCode = QQuickPixmapReply::Loading;
                    errorStr = QQuickPixmap::tr("Failed to get image from provider: %1").arg(url.toString());
                }
                PIXMAP_READER_LOCK();
                if (!cancelledJobs.contains(runningJob)) {
                    runningJob->postReply(errorCode, errorStr, readSize,
                                          QQuickTextureFactory::textureFactoryForImage(image));
                }
                break;
            }

            case QQuickImageProvider::Pixmap:
            {
                QPixmap pixmap;
                if (providerV2) {
                    pixmap = providerV2->requestPixmap(imageId(url), &readSize, runningJob->requestSize, runningJob->providerOptions);
                } else {
                    pixmap = provider->requestPixmap(imageId(url), &readSize, runningJob->requestSize);
                }
                QQuickPixmapReply::ReadError errorCode = QQuickPixmapReply::NoError;
                QString errorStr;
                if (pixmap.isNull()) {
                    errorCode = QQuickPixmapReply::Loading;
                    errorStr = QQuickPixmap::tr("Failed to get image from provider: %1").arg(url.toString());
                }

                PIXMAP_READER_LOCK();
                if (!cancelledJobs.contains(runningJob)) {
                    runningJob->postReply(
                            errorCode, errorStr, readSize,
                            QQuickTextureFactory::textureFactoryForImage(pixmap.toImage()));
                }
                break;
            }

            case QQuickImageProvider::Texture:
            {
                QQuickTextureFactory *t;
                if (providerV2) {
                    t = providerV2->requestTexture(imageId(url), &readSize, runningJob->requestSize, runningJob->providerOptions);
                } else {
                    t = provider->requestTexture(imageId(url), &readSize, runningJob->requestSize);
                }
                QQuickPixmapReply::ReadError errorCode = QQuickPixmapReply::NoError;
                QString errorStr;
                if (!t) {
                    errorCode = QQuickPixmapReply::Loading;
                    errorStr = QQuickPixmap::tr("Failed to get texture from provider: %1").arg(url.toString());
                }
                PIXMAP_READER_LOCK();
                if (!cancelledJobs.contains(runningJob))
                    runningJob->postReply(errorCode, errorStr, readSize, t);
                else
                    delete t;
                break;
            }

            case QQuickImageProvider::ImageResponse:
            {
                QQuickImageResponse *response;
                if (providerV2) {
                    response = providerV2->requestImageResponse(imageId(url), runningJob->requestSize, runningJob->providerOptions);
                } else {
                    QQuickAsyncImageProvider *asyncProvider = static_cast<QQuickAsyncImageProvider*>(provider.get());
                    response = asyncProvider->requestImageResponse(imageId(url), runningJob->requestSize);
                }

                {
                    QObject::connect(response, &QQuickImageResponse::finished, readerThreadExecutionEnforcer(),
                                     qOverload<>(&ReaderThreadExecutionEnforcer::asyncResponseFinished));
                    // as the response object can outlive the provider QSharedPointer, we have to extend the pointee's lifetime by that of the response
                    // we do this by capturing a copy of the QSharedPointer in a lambda, and dropping it once the lambda has been called
                    auto provider_copy = provider; // capturing provider would capture it as a const reference, and copy capture with initializer is only available in C++14
                    QObject::connect(response, &QQuickImageResponse::destroyed, response, [provider_copy]() {
                        // provider_copy will be deleted when the connection gets deleted
                    });
                }
                // Might be that the async provider was so quick it emitted the signal before we
                // could connect to it.
                //
                // loadAcquire() synchronizes-with storeRelease() in QQuickImageResponsePrivate::_q_finished():
                if (static_cast<QQuickImageResponsePrivate*>(QObjectPrivate::get(response))->finished.loadAcquire()) {
                    QMetaObject::invokeMethod(readerThreadExecutionEnforcer(), "asyncResponseFinished",
                                              Qt::QueuedConnection,
                                              Q_ARG(QQuickImageResponse *, response));
                }

                asyncResponses.insert(response, runningJob);
                break;
            }
        }

    } else {
        if (!localFile.isEmpty()) {
            // Image is local - load/decode immediately
            QImage image;
            QQuickPixmapReply::ReadError errorCode = QQuickPixmapReply::NoError;
            QString errorStr;
            QSize readSize;

            if (runningJob->data && runningJob->data->fromSpecialDevice) {
                auto specialDevice = runningJob->data->specialDevice;
                if (specialDevice.isNull() || QObjectPrivate::get(specialDevice.data())->deleteLaterCalled) {
                    qCDebug(lcImg) << "readImage job aborted" << url;
                    return;
                }
                int frameCount;
                // Ensure that specialDevice's thread affinity is _this_ thread, to avoid deleteLater()
                // deleting prematurely, before readImage() is done. But this is only possible if it has already
                // relinquished its initial thread affinity.
                if (!specialDevice->thread()) {
                    qCDebug(lcQsgLeak) << specialDevice.data() << ": changing thread affinity so that"
                                       << QThread::currentThread() << "will handle any deleteLater() calls";
                    specialDevice->moveToThread(QThread::currentThread());
                }
                if (!readImage(url, specialDevice.data(), &image, &errorStr, &readSize, &frameCount,
                               runningJob->requestRegion, runningJob->requestSize,
                               runningJob->providerOptions, nullptr, runningJob->data->frame)) {
                    errorCode = QQuickPixmapReply::Loading;
                } else if (runningJob->data) {
                    runningJob->data->frameCount = frameCount;
                }
            } else {
                QFile f(existingImageFileForPath(localFile));
                if (f.open(QIODevice::ReadOnly)) {
                    QSGTextureReader texReader(&f, localFile);
                    if (backendSupport()->hasOpenGL && texReader.isTexture()) {
                        QQuickTextureFactory *factory = texReader.read();
                        if (factory) {
                            readSize = factory->textureSize();
                        } else {
                            errorStr = QQuickPixmap::tr("Error decoding: %1").arg(url.toString());
                            if (f.fileName() != localFile)
                                errorStr += QString::fromLatin1(" (%1)").arg(f.fileName());
                            errorCode = QQuickPixmapReply::Decoding;
                        }
                        PIXMAP_READER_LOCK();
                        if (!cancelledJobs.contains(runningJob))
                            runningJob->postReply(errorCode, errorStr, readSize, factory);
                        return;
                    } else {
                        int frameCount;
                        int const frame = runningJob->data ? runningJob->data->frame : 0;
                        if (!readImage(url, &f, &image, &errorStr, &readSize, &frameCount,
                                       runningJob->requestRegion, runningJob->requestSize,
                                       runningJob->providerOptions, nullptr, frame)) {
                            errorCode = QQuickPixmapReply::Loading;
                            if (f.fileName() != localFile)
                                errorStr += QString::fromLatin1(" (%1)").arg(f.fileName());
                        } else if (runningJob->data) {
                            runningJob->data->frameCount = frameCount;
                        }
                    }
                } else {
                    errorStr = QQuickPixmap::tr("Cannot open: %1").arg(url.toString());
                    errorCode = QQuickPixmapReply::Loading;
                }
            }
            PIXMAP_READER_LOCK();
            if (!cancelledJobs.contains(runningJob)) {
                runningJob->postReply(errorCode, errorStr, readSize,
                                      QQuickTextureFactory::textureFactoryForImage(image));
            }
        } else {
#if QT_CONFIG(qml_network)
            // Network resource
            QNetworkRequest req(url);
            req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
            QNetworkReply *reply = networkAccessManager()->get(req);

            QMetaObject::connect(reply, replyDownloadProgressMethodIndex, runningJob,
                                 downloadProgressMethodIndex);
            QMetaObject::connect(reply, replyFinishedMethodIndex, readerThreadExecutionEnforcer(),
                                 threadNetworkRequestDoneMethodIndex);

            networkJobs.insert(reply, runningJob);
#else
// Silently fail if compiled with no_network
#endif
        }
    }
}

QQuickPixmapReader *QQuickPixmapReader::instance(QQmlEngine *engine)
{
    // XXX NOTE: must be called within readerMutex locking.
    QQuickPixmapReader *reader = readers.value(engine);
    if (!reader) {
        reader = new QQuickPixmapReader(engine);
        readers.insert(engine, reader);
    }

    return reader;
}

QQuickPixmapReader *QQuickPixmapReader::existingInstance(QQmlEngine *engine)
{
    // XXX NOTE: must be called within readerMutex locking.
    return readers.value(engine, 0);
}

QQuickPixmapReply *QQuickPixmapReader::getImage(QQuickPixmapData *data)
{
    QQuickPixmapReply *reply = new QQuickPixmapReply(data);
    reply->engineForReader = engine;
    return reply;
}

void QQuickPixmapReader::startJob(QQuickPixmapReply *job)
{
    PIXMAP_READER_LOCK();
    jobs.append(job);
    if (readerThreadExecutionEnforcer())
        readerThreadExecutionEnforcer()->processJobsOnReaderThreadLater();
}

void QQuickPixmapReader::cancel(QQuickPixmapReply *reply)
{
    PIXMAP_READER_LOCK();
    if (reply->loading) {
        cancelledJobs.append(reply);
        reply->data = nullptr;
        // XXX
        if (readerThreadExecutionEnforcer())
            readerThreadExecutionEnforcer()->processJobsOnReaderThreadLater();
    } else {
        // If loading was started (reply removed from jobs) but the reply was never processed
        // (otherwise it would have deleted itself) we need to profile an error.
        if (jobs.removeAll(reply) == 0) {
            PIXMAP_PROFILE(pixmapStateChanged<QQuickProfiler::PixmapLoadingError>(reply->url));
        }
        delete reply;
    }
}

void QQuickPixmapReader::run()
{
    Q_ASSERT_CALLED_ON_VALID_THREAD(m_readerThreadAffinityMarker);

    if (replyDownloadProgressMethodIndex == -1) {
#if QT_CONFIG(qml_network)
        replyDownloadProgressMethodIndex =
                QMetaMethod::fromSignal(&QNetworkReply::downloadProgress).methodIndex();
        replyFinishedMethodIndex = QMetaMethod::fromSignal(&QNetworkReply::finished).methodIndex();
        const QMetaObject *ir = &ReaderThreadExecutionEnforcer::staticMetaObject;
        threadNetworkRequestDoneMethodIndex = ir->indexOfSlot("networkRequestDone()");
#endif
        downloadProgressMethodIndex =
                QMetaMethod::fromSignal(&QQuickPixmapReply::downloadProgress).methodIndex();
    }

#if QT_CONFIG(quick_pixmap_cache_threaded_download)
    const auto guard = qScopeGuard([this]() {
        // We need to delete the runLoopReaderThreadExecutionEnforcer from the same thread.
        PIXMAP_READER_LOCK();
        delete runLoopReaderThreadExecutionEnforcer;
        runLoopReaderThreadExecutionEnforcer = nullptr;
    });

    {
        PIXMAP_READER_LOCK();
        Q_ASSERT(!runLoopReaderThreadExecutionEnforcer);
        runLoopReaderThreadExecutionEnforcer = new ReaderThreadExecutionEnforcer(this);
    }

    processJobs();
    exec();
#else
    ownedReaderThreadExecutionEnforcer = std::make_unique<ReaderThreadExecutionEnforcer>(this);
    processJobs();
#endif
}

inline bool operator==(const QQuickPixmapKey &lhs, const QQuickPixmapKey &rhs)
{
    return *lhs.url == *rhs.url &&
           *lhs.region == *rhs.region &&
           *lhs.size == *rhs.size &&
            lhs.frame == rhs.frame &&
            lhs.options == rhs.options;
}

inline size_t qHash(const QQuickPixmapKey &key, size_t seed) noexcept
{
    return qHashMulti(seed, *key.url, *key.region, *key.size, key.frame, key.options.autoTransform());
}

#ifndef QT_NO_DEBUG_STREAM
inline QDebug operator<<(QDebug debug, const QQuickPixmapKey &key)
{
    QDebugStateSaver saver(debug);
    debug.nospace();
    if (!key.url) {
        debug << "QQuickPixmapKey(0)";
        return debug;
    }

    debug << "QQuickPixmapKey(" << key.url->toString() << " frame=" << key.frame;
    if (!key.region->isEmpty()) {
        debug << " region=";
        QtDebugUtils::formatQRect(debug, *key.region);
    }
    if (!key.size->isEmpty()) {
        debug << " size=";
        QtDebugUtils::formatQSize(debug, *key.size);
    }
    debug << ')';
    return debug;
}
#endif

QQuickPixmapCache *QQuickPixmapCache::instance()
{
    static QQuickPixmapCache self;
    return &self;
}

QQuickPixmapCache::~QQuickPixmapCache()
{
    destroyCache();
}

/*! \internal
    Empty the cache completely, to prevent leaks. Returns the number of
    leaked pixmaps (should always be \c 0).

    This is work the destructor needs to do, but we put it into a function
    only to make it testable in autotests, because the static instance()
    cannot be destroyed before shutdown.
*/
int QQuickPixmapCache::destroyCache()
{
    if (m_destroying)
        return -1;

    m_destroying = true;

    // Prevent unreferencePixmap() from assuming it needs to kick
    // off the cache expiry timer, as we're shrinking the cache
    // manually below after releasing all the pixmaps.
    m_timerId = -2;

    // unreference all (leaked) pixmaps
    int leakedPixmaps = 0;
    const auto cache = m_cache; // NOTE: intentional copy (QTBUG-65077); releasing items from the cache modifies m_cache.
    for (auto *pixmap : cache) {
        auto currRefCount = pixmap->refCount;
        if (currRefCount) {
            leakedPixmaps++;
            qCDebug(lcQsgLeak) << "leaked pixmap: refCount" << pixmap->refCount << pixmap->url << "frame" << pixmap->frame
                               << "size" << pixmap->requestSize << "region" << pixmap->requestRegion;
            while (currRefCount > 0) {
                pixmap->release(this);
                currRefCount--;
            }
        }
    }

    // free all unreferenced pixmaps
    while (m_lastUnreferencedPixmap)
        shrinkCache(20);

    qCDebug(lcQsgLeak, "Number of leaked pixmaps: %i", leakedPixmaps);
    return leakedPixmaps;
}

qsizetype QQuickPixmapCache::referencedCost() const
{
    qsizetype ret = 0;
    QMutexLocker locker(&m_cacheMutex);
    for (const auto *pixmap : std::as_const(m_cache)) {
        if (pixmap->refCount)
            ret += pixmap->cost();
    }
    return ret;
}

/*! \internal
    Declare that \a data is currently unused so that shrinkCache() can lazily
    delete it later.
*/
void QQuickPixmapCache::unreferencePixmap(QQuickPixmapData *data)
{
    Q_ASSERT(data->prevUnreferenced == nullptr);
    Q_ASSERT(data->prevUnreferencedPtr == nullptr);
    Q_ASSERT(data->nextUnreferenced == nullptr);

    data->nextUnreferenced = m_unreferencedPixmaps;
    data->prevUnreferencedPtr = &m_unreferencedPixmaps;
    if (!m_destroying) { // the texture factories may have been cleaned up already.
        m_unreferencedCost += data->cost();
        qCDebug(lcImg) << data->url << "had cost" << data->cost() << "of total unreferenced" << m_unreferencedCost;
    }

    m_unreferencedPixmaps = data;
    if (m_unreferencedPixmaps->nextUnreferenced) {
        m_unreferencedPixmaps->nextUnreferenced->prevUnreferenced = m_unreferencedPixmaps;
        m_unreferencedPixmaps->nextUnreferenced->prevUnreferencedPtr = &m_unreferencedPixmaps->nextUnreferenced;
    }

    if (!m_lastUnreferencedPixmap)
        m_lastUnreferencedPixmap = data;

    shrinkCache(-1); // Shrink the cache in case it has become larger than cache_limit

    if (m_timerId == -1 && m_unreferencedPixmaps
            && !m_destroying && !QCoreApplication::closingDown()) {
        m_timerId = startTimer(CACHE_EXPIRE_TIME * 1000);
    }
}

/*! \internal
    Declare that \a data is being used (by a QQuickPixmap) so that
    shrinkCache() won't delete it. (This is not reference counting though.)
*/
void QQuickPixmapCache::referencePixmap(QQuickPixmapData *data)
{
    Q_ASSERT(data->prevUnreferencedPtr);

    *data->prevUnreferencedPtr = data->nextUnreferenced;
    if (data->nextUnreferenced) {
        data->nextUnreferenced->prevUnreferencedPtr = data->prevUnreferencedPtr;
        data->nextUnreferenced->prevUnreferenced = data->prevUnreferenced;
    }
    if (m_lastUnreferencedPixmap == data)
        m_lastUnreferencedPixmap = data->prevUnreferenced;

    data->nextUnreferenced = nullptr;
    data->prevUnreferencedPtr = nullptr;
    data->prevUnreferenced = nullptr;

    m_unreferencedCost -= data->cost();
    qCDebug(lcImg) << data->url << "subtracts cost" << data->cost() << "of total" << m_unreferencedCost;
}

/*! \internal
    Delete the least-recently-released QQuickPixmapData instances
    until the remaining bytes are less than cache_limit.
*/
void QQuickPixmapCache::shrinkCache(int remove)
{
    qCDebug(lcImg) << "reduce unreferenced cost" << m_unreferencedCost << "to less than limit" << cache_limit;
    while ((remove > 0 || m_unreferencedCost > cache_limit) && m_lastUnreferencedPixmap) {
        QQuickPixmapData *data = m_lastUnreferencedPixmap;
        Q_ASSERT(data->nextUnreferenced == nullptr);

        *data->prevUnreferencedPtr = nullptr;
        m_lastUnreferencedPixmap = data->prevUnreferenced;
        data->prevUnreferencedPtr = nullptr;
        data->prevUnreferenced = nullptr;

        if (!m_destroying) {
            remove -= data->cost();
            m_unreferencedCost -= data->cost();
        }
        data->removeFromCache(this);
        delete data;
    }
}

void QQuickPixmapCache::timerEvent(QTimerEvent *)
{
    int removalCost = m_unreferencedCost / CACHE_REMOVAL_FRACTION;

    shrinkCache(removalCost);

    if (m_unreferencedPixmaps == nullptr) {
        killTimer(m_timerId);
        m_timerId = -1;
    }
}

void QQuickPixmapCache::purgeCache()
{
    shrinkCache(m_unreferencedCost);
}

void QQuickPixmap::purgeCache()
{
    QQuickPixmapCache::instance()->purgeCache();
}

QQuickPixmapReply::QQuickPixmapReply(QQuickPixmapData *d)
  : data(d), engineForReader(nullptr), requestRegion(d->requestRegion), requestSize(d->requestSize),
    url(d->url), loading(false), providerOptions(d->providerOptions)
{
    if (finishedMethodIndex == -1) {
        finishedMethodIndex = QMetaMethod::fromSignal(&QQuickPixmapReply::finished).methodIndex();
        downloadProgressMethodIndex =
                QMetaMethod::fromSignal(&QQuickPixmapReply::downloadProgress).methodIndex();
    }
}

QQuickPixmapReply::~QQuickPixmapReply()
{
    // note: this->data->reply must be set to zero if this->data->reply == this
    // but it must be done within mutex locking, to be guaranteed to be safe.
}

bool QQuickPixmapReply::event(QEvent *event)
{
    if (event->type() == QEvent::User) {

        if (data) {
            Event *de = static_cast<Event *>(event);
            data->pixmapStatus = (de->error == NoError) ? QQuickPixmap::Ready : QQuickPixmap::Error;
            if (data->pixmapStatus == QQuickPixmap::Ready) {
                data->textureFactory = de->textureFactory;
                de->textureFactory = nullptr;
                data->implicitSize = de->implicitSize;
                PIXMAP_PROFILE(pixmapLoadingFinished(data->url,
                        data->textureFactory != nullptr && data->textureFactory->textureSize().isValid() ?
                        data->textureFactory->textureSize() :
                        (data->requestSize.isValid() ? data->requestSize : data->implicitSize)));
            } else {
                PIXMAP_PROFILE(pixmapStateChanged<QQuickProfiler::PixmapLoadingError>(data->url));
                data->errorString = de->errorString;
                data->removeFromCache(); // We don't continue to cache error'd pixmaps
            }

            data->reply = nullptr;
            emit finished();
        } else {
            PIXMAP_PROFILE(pixmapStateChanged<QQuickProfiler::PixmapLoadingError>(url));
        }

        delete this;
        return true;
    } else {
        return QObject::event(event);
    }
}

int QQuickPixmapData::cost() const
{
    if (textureFactory)
        return textureFactory->textureByteCount();
    return 0;
}

void QQuickPixmapData::addref()
{
    ++refCount;
    PIXMAP_PROFILE(pixmapCountChanged<QQuickProfiler::PixmapReferenceCountChanged>(url, refCount));
    if (prevUnreferencedPtr)
        QQuickPixmapCache::instance()->referencePixmap(this);
}

void QQuickPixmapData::release(QQuickPixmapCache *store)
{
    Q_ASSERT(refCount > 0);
    --refCount;
    PIXMAP_PROFILE(pixmapCountChanged<QQuickProfiler::PixmapReferenceCountChanged>(url, refCount));
    if (refCount == 0) {
        if (reply) {
            QQuickPixmapReply *cancelReply = reply;
            reply->data = nullptr;
            reply = nullptr;
            QQuickPixmapReader::readerMutex.lock();
            QQuickPixmapReader *reader = QQuickPixmapReader::existingInstance(cancelReply->engineForReader);
            if (reader)
                reader->cancel(cancelReply);
            QQuickPixmapReader::readerMutex.unlock();
        }

        store = store ? store : QQuickPixmapCache::instance();
        if (pixmapStatus == QQuickPixmap::Ready
#ifdef Q_OS_WEBOS
                && storeToCache
#endif
                ) {
            if (inCache)
                store->unreferencePixmap(this);
            else
                delete this;
        } else {
            removeFromCache(store);
            delete this;
        }
    }
}

/*! \internal
    Add this to the QQuickPixmapCache singleton.

    \note The actual image will end up in QQuickPixmapData::textureFactory.
    At the time addToCache() is called, it's generally not yet loaded; so the
    qCDebug() below cannot say how much data we're committing to storing.
    (On the other hand, removeFromCache() can tell.) QQuickTextureFactory is an
    abstraction for image data. See QQuickDefaultTextureFactory for example:
    it stores a QImage directly. Other QQuickTextureFactory subclasses store data
    in other ways.
*/
void QQuickPixmapData::addToCache()
{
    if (!inCache) {
        QQuickPixmapKey key = { &url, &requestRegion, &requestSize, frame, providerOptions };
        QMutexLocker locker(&QQuickPixmapCache::instance()->m_cacheMutex);
        if (lcImg().isDebugEnabled()) {
            qCDebug(lcImg) << "adding" << key << "to total" << QQuickPixmapCache::instance()->m_cache.size();
            for (auto it = QQuickPixmapCache::instance()->m_cache.keyBegin(); it != QQuickPixmapCache::instance()->m_cache.keyEnd(); ++it) {
                if (*(it->url) == url && it->frame == frame)
                    qCDebug(lcImg) << "    similar pre-existing:" << *it;
            }
        }
        QQuickPixmapCache::instance()->m_cache.insert(key, this);
        inCache = true;
        PIXMAP_PROFILE(pixmapCountChanged<QQuickProfiler::PixmapCacheCountChanged>(
                url, QQuickPixmapCache::instance()->m_cache.size()));
    }
}

void QQuickPixmapData::removeFromCache(QQuickPixmapCache *store)
{
    if (inCache) {
        if (!store)
            store = QQuickPixmapCache::instance();
        QQuickPixmapKey key = { &url, &requestRegion, &requestSize, frame, providerOptions };
        QMutexLocker locker(&QQuickPixmapCache::instance()->m_cacheMutex);
        store->m_cache.remove(key);
        qCDebug(lcImg) << "removed" << key << implicitSize << "; total remaining" << QQuickPixmapCache::instance()->m_cache.size();
        inCache = false;
        PIXMAP_PROFILE(pixmapCountChanged<QQuickProfiler::PixmapCacheCountChanged>(
                url, store->m_cache.size()));
    }
}

static QQuickPixmapData* createPixmapDataSync(QQmlEngine *engine, const QUrl &url,
                                              const QRect &requestRegion, const QSize &requestSize,
                                              const QQuickImageProviderOptions &providerOptions, int frame, bool *ok,
                                              qreal devicePixelRatio)
{
    if (url.scheme() == QLatin1String("image")) {
        QSize readSize;

        QQuickImageProvider::ImageType imageType = QQuickImageProvider::Invalid;
        QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);
        QSharedPointer<QQuickImageProvider> provider = enginePrivate->imageProvider(imageProviderId(url)).objectCast<QQuickImageProvider>();
        // it is safe to use get() as providerV2 does not escape and is outlived by provider
        QQuickImageProviderWithOptions *providerV2 = QQuickImageProviderWithOptions::checkedCast(provider.get());
        if (provider)
            imageType = provider->imageType();

        switch (imageType) {
            case QQuickImageProvider::Invalid:
                return new QQuickPixmapData(url, requestRegion, requestSize, providerOptions,
                    QQuickPixmap::tr("Invalid image provider: %1").arg(url.toString()));
            case QQuickImageProvider::Texture:
            {
                QQuickTextureFactory *texture = providerV2 ? providerV2->requestTexture(imageId(url), &readSize, requestSize, providerOptions)
                                                           : provider->requestTexture(imageId(url), &readSize, requestSize);
                if (texture) {
                    *ok = true;
                    return new QQuickPixmapData(url, texture, readSize, requestRegion, requestSize,
                                                providerOptions, QQuickImageProviderOptions::UsePluginDefaultTransform, frame);
                }
                break;
            }

            case QQuickImageProvider::Image:
            {
                QImage image = providerV2 ? providerV2->requestImage(imageId(url), &readSize, requestSize, providerOptions)
                                          : provider->requestImage(imageId(url), &readSize, requestSize);
                if (!image.isNull()) {
                    *ok = true;
                    return new QQuickPixmapData(url, QQuickTextureFactory::textureFactoryForImage(image),
                                                readSize, requestRegion, requestSize, providerOptions,
                                                QQuickImageProviderOptions::UsePluginDefaultTransform, frame);
                }
                break;
            }
            case QQuickImageProvider::Pixmap:
            {
                QPixmap pixmap = providerV2 ? providerV2->requestPixmap(imageId(url), &readSize, requestSize, providerOptions)
                                            : provider->requestPixmap(imageId(url), &readSize, requestSize);
                if (!pixmap.isNull()) {
                    *ok = true;
                    return new QQuickPixmapData(url, QQuickTextureFactory::textureFactoryForImage(pixmap.toImage()),
                                                readSize, requestRegion, requestSize, providerOptions,
                                                QQuickImageProviderOptions::UsePluginDefaultTransform, frame);
                }
                break;
            }
            case QQuickImageProvider::ImageResponse:
            {
                // Fall through, ImageResponse providers never get here
                Q_ASSERT(imageType != QQuickImageProvider::ImageResponse && "Sync call to ImageResponse provider");
            }
        }

        // provider has bad image type, or provider returned null image
        return new QQuickPixmapData(url, requestRegion, requestSize, providerOptions,
            QQuickPixmap::tr("Failed to get image from provider: %1").arg(url.toString()));
    }

    QString localFile = QQmlFile::urlToLocalFileOrQrc(url);
    if (localFile.isEmpty())
        return nullptr;

    QFile f(existingImageFileForPath(localFile));
    QSize readSize;
    QString errorString;

    if (f.open(QIODevice::ReadOnly)) {
        QSGTextureReader texReader(&f, localFile);
        if (backendSupport()->hasOpenGL && texReader.isTexture()) {
            QQuickTextureFactory *factory = texReader.read();
            if (factory) {
                *ok = true;
                return new QQuickPixmapData(url, factory, factory->textureSize(), requestRegion, requestSize,
                                            providerOptions, QQuickImageProviderOptions::UsePluginDefaultTransform, frame);
            } else {
                errorString = QQuickPixmap::tr("Error decoding: %1").arg(url.toString());
                if (f.fileName() != localFile)
                    errorString += QString::fromLatin1(" (%1)").arg(f.fileName());
            }
        } else {
            QImage image;
            QQuickImageProviderOptions::AutoTransform appliedTransform = providerOptions.autoTransform();
            int frameCount;
            if (readImage(url, &f, &image, &errorString, &readSize, &frameCount, requestRegion, requestSize,
                          providerOptions, &appliedTransform, frame, devicePixelRatio)) {
                *ok = true;
                return new QQuickPixmapData(url, QQuickTextureFactory::textureFactoryForImage(image), readSize, requestRegion, requestSize,
                                            providerOptions, appliedTransform, frame, frameCount);
            } else if (f.fileName() != localFile) {
                errorString += QString::fromLatin1(" (%1)").arg(f.fileName());
            }
        }
    } else {
        errorString = QQuickPixmap::tr("Cannot open: %1").arg(url.toString());
    }
    return new QQuickPixmapData(url, requestRegion, requestSize, providerOptions, errorString);
}


struct QQuickPixmapNull {
    QUrl url;
    QRect region;
    QSize size;
};
Q_GLOBAL_STATIC(QQuickPixmapNull, nullPixmap);

QQuickPixmap::QQuickPixmap()
: d(nullptr)
{
}

QQuickPixmap::QQuickPixmap(QQmlEngine *engine, const QUrl &url)
: d(nullptr)
{
    load(engine, url);
}

QQuickPixmap::QQuickPixmap(QQmlEngine *engine, const QUrl &url, Options options)
: d(nullptr)
{
    load(engine, url, options);
}

QQuickPixmap::QQuickPixmap(QQmlEngine *engine, const QUrl &url, const QRect &region, const QSize &size)
: d(nullptr)
{
    load(engine, url, region, size);
}

QQuickPixmap::QQuickPixmap(const QUrl &url, const QImage &image)
{
    d = new QQuickPixmapData(url, new QQuickDefaultTextureFactory(image), image.size(), QRect(), QSize(),
                             QQuickImageProviderOptions(), QQuickImageProviderOptions::UsePluginDefaultTransform);
    d->addToCache();
}

QQuickPixmap::~QQuickPixmap()
{
    if (d) {
        d->release();
        d = nullptr;
    }
}

bool QQuickPixmap::isNull() const
{
    return d == nullptr;
}

bool QQuickPixmap::isReady() const
{
    return status() == Ready;
}

bool QQuickPixmap::isError() const
{
    return status() == Error;
}

bool QQuickPixmap::isLoading() const
{
    return status() == Loading;
}

QString QQuickPixmap::error() const
{
    if (d)
        return d->errorString;
    else
        return QString();
}

QQuickPixmap::Status QQuickPixmap::status() const
{
    if (d)
        return d->pixmapStatus;
    else
        return Null;
}

const QUrl &QQuickPixmap::url() const
{
    if (d)
        return d->url;
    else
        return nullPixmap()->url;
}

const QSize &QQuickPixmap::implicitSize() const
{
    if (d)
        return d->implicitSize;
    else
        return nullPixmap()->size;
}

const QSize &QQuickPixmap::requestSize() const
{
    if (d)
        return d->requestSize;
    else
        return nullPixmap()->size;
}

const QRect &QQuickPixmap::requestRegion() const
{
    if (d)
        return d->requestRegion;
    else
        return nullPixmap()->region;
}

QQuickImageProviderOptions::AutoTransform QQuickPixmap::autoTransform() const
{
    if (d)
        return d->appliedTransform;
    else
        return QQuickImageProviderOptions::UsePluginDefaultTransform;
}

int QQuickPixmap::frameCount() const
{
    if (d)
        return d->frameCount;
    return 0;
}

QQuickTextureFactory *QQuickPixmap::textureFactory() const
{
    if (d)
        return d->textureFactory;

    return nullptr;
}

QImage QQuickPixmap::image() const
{
    if (d && d->textureFactory)
        return d->textureFactory->image();
    return QImage();
}

void QQuickPixmap::setImage(const QImage &p)
{
    clear();

    if (!p.isNull()) {
        if (d)
            d->release();
        d = new QQuickPixmapData(QQuickTextureFactory::textureFactoryForImage(p));
    }
}

void QQuickPixmap::setPixmap(const QQuickPixmap &other)
{
    if (d == other.d)
        return;
    clear();

    if (other.d) {
        if (d)
            d->release();
        d = other.d;
        d->addref();
    }
}

int QQuickPixmap::width() const
{
    if (d && d->textureFactory)
        return d->textureFactory->textureSize().width();
    else
        return 0;
}

int QQuickPixmap::height() const
{
    if (d && d->textureFactory)
        return d->textureFactory->textureSize().height();
    else
        return 0;
}

QRect QQuickPixmap::rect() const
{
    if (d && d->textureFactory)
        return QRect(QPoint(), d->textureFactory->textureSize());
    else
        return QRect();
}

void QQuickPixmap::load(QQmlEngine *engine, const QUrl &url)
{
    load(engine, url, QRect(), QSize(), QQuickPixmap::Cache);
}

void QQuickPixmap::load(QQmlEngine *engine, const QUrl &url, QQuickPixmap::Options options)
{
    load(engine, url, QRect(), QSize(), options);
}

void QQuickPixmap::load(QQmlEngine *engine, const QUrl &url, const QRect &requestRegion, const QSize &requestSize)
{
    load(engine, url, requestRegion, requestSize, QQuickPixmap::Cache);
}

void QQuickPixmap::load(QQmlEngine *engine, const QUrl &url, const QRect &requestRegion, const QSize &requestSize, QQuickPixmap::Options options)
{
    load(engine, url, requestRegion, requestSize, options, QQuickImageProviderOptions());
}

void QQuickPixmap::load(QQmlEngine *engine, const QUrl &url, const QRect &requestRegion, const QSize &requestSize,
                        QQuickPixmap::Options options, const QQuickImageProviderOptions &providerOptions, int frame, int frameCount,
                        qreal devicePixelRatio)
{
    if (d) {
        d->release();
        d = nullptr;
    }

    QQuickPixmapKey key = { &url, &requestRegion, &requestSize, frame, providerOptions };
    QQuickPixmapCache *store = QQuickPixmapCache::instance();

    QMutexLocker locker(&QQuickPixmapCache::instance()->m_cacheMutex);
    QHash<QQuickPixmapKey, QQuickPixmapData *>::Iterator iter = store->m_cache.end();

#ifdef Q_OS_WEBOS
    QQuickPixmap::Options orgOptions = options;
    // In webOS, we suppose that cache is always enabled to share image instances along its source.
    // So, original option(orgOptions) for cache only decides whether to store the instances when it's unreferenced.
    options |= QQuickPixmap::Cache;
#endif

    // If Cache is disabled, the pixmap will always be loaded, even if there is an existing
    // cached version. Unless it's an itemgrabber url, since the cache is used to pass
    // the result between QQuickItemGrabResult and QQuickImage.
    if (url.scheme() == itemGrabberScheme) {
        QRect dummyRegion;
        QSize dummySize;
        if (requestSize != dummySize)
            qWarning() << "Ignoring sourceSize request for image url that came from grabToImage. Use the targetSize parameter of the grabToImage() function instead.";
        const QQuickPixmapKey grabberKey = { &url, &dummyRegion, &dummySize, 0, QQuickImageProviderOptions() };
        iter = store->m_cache.find(grabberKey);
    } else if (options & QQuickPixmap::Cache)
        iter = store->m_cache.find(key);

    if (iter == store->m_cache.end()) {
        if (!engine)
            return;

        locker.unlock();

        if (url.scheme() == QLatin1String("image")) {
            QQmlEnginePrivate *enginePrivate = QQmlEnginePrivate::get(engine);
            if (auto provider = enginePrivate->imageProvider(imageProviderId(url)).staticCast<QQuickImageProvider>()) {
                const bool threadedPixmaps = QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ThreadedPixmaps);
                if (!threadedPixmaps && provider->imageType() == QQuickImageProvider::Pixmap) {
                    // pixmaps can only be loaded synchronously
                    options &= ~QQuickPixmap::Asynchronous;
                } else if (provider->flags() & QQuickImageProvider::ForceAsynchronousImageLoading) {
                    options |= QQuickPixmap::Asynchronous;
                }
            }
        }

        if (!(options & QQuickPixmap::Asynchronous)) {
            bool ok = false;
            PIXMAP_PROFILE(pixmapStateChanged<QQuickProfiler::PixmapLoadingStarted>(url));
            d = createPixmapDataSync(engine, url, requestRegion, requestSize, providerOptions, frame, &ok, devicePixelRatio);
            if (ok) {
                PIXMAP_PROFILE(pixmapLoadingFinished(url, QSize(width(), height())));
                if (options & QQuickPixmap::Cache)
                    d->addToCache();
#ifdef Q_OS_WEBOS
                d->storeToCache = orgOptions & QQuickPixmap::Cache;
#endif
                return;
            }
            if (d) { // loadable, but encountered error while loading
                PIXMAP_PROFILE(pixmapStateChanged<QQuickProfiler::PixmapLoadingError>(url));
                return;
            }
        }

        d = new QQuickPixmapData(url, requestRegion, requestSize, providerOptions,
                                 QQuickImageProviderOptions::UsePluginDefaultTransform, frame, frameCount);
        if (options & QQuickPixmap::Cache)
            d->addToCache();
#ifdef Q_OS_WEBOS
        d->storeToCache = orgOptions & QQuickPixmap::Cache;
#endif

        QQuickPixmapReader::readerMutex.lock();
        QQuickPixmapReader *reader = QQuickPixmapReader::instance(engine);
        d->reply = reader->getImage(d);
        reader->startJob(d->reply);
        QQuickPixmapReader::readerMutex.unlock();
    } else {
        d = *iter;
        d->addref();
        qCDebug(lcImg) << "loaded from cache" << url << "frame" << frame;
    }
}

/*! \internal
    Attempts to load an image from the given \a url via the given \a device.
    This is for special cases when the QImageIOHandler can benefit from reusing
    the I/O device, or from something extra that a subclass of QIODevice
    carries with it. So far, this code doesn't support loading anything other
    than a QImage, for example compressed textures. It can be added if needed.
*/
void QQuickPixmap::loadImageFromDevice(QQmlEngine *engine, QIODevice *device, const QUrl &url,
                                       const QRect &requestRegion, const QSize &requestSize,
                                       const QQuickImageProviderOptions &providerOptions, int frame, int frameCount)
{
    auto oldD = d;
    QQuickPixmapKey key = { &url, &requestRegion, &requestSize, frame, providerOptions };
    QQuickPixmapCache *store = QQuickPixmapCache::instance();
    QHash<QQuickPixmapKey, QQuickPixmapData *>::Iterator iter = store->m_cache.end();
    QMutexLocker locker(&store->m_cacheMutex);
    iter = store->m_cache.find(key);
    if (iter == store->m_cache.end()) {
        if (!engine)
            return;

        locker.unlock();
        d = new QQuickPixmapData(url, requestRegion, requestSize, providerOptions,
                                 QQuickImageProviderOptions::UsePluginDefaultTransform, frame, frameCount);
        d->specialDevice = device;
        d->fromSpecialDevice = true;
        d->addToCache();

        QQuickPixmapReader::readerMutex.lock();
        QQuickPixmapReader *reader = QQuickPixmapReader::instance(engine);
        d->reply = reader->getImage(d);
        if (oldD) {
            QObject::connect(d->reply, &QQuickPixmapReply::destroyed, store, [oldD]() {
                oldD->release();
            }, Qt::QueuedConnection);
        }
        reader->startJob(d->reply);
        QQuickPixmapReader::readerMutex.unlock();
    } else {
        d = *iter;
        d->addref();
        qCDebug(lcImg) << "loaded from cache" << url << "frame" << frame << "refCount" << d->refCount;
        locker.unlock();
        if (oldD)
            oldD->release();
    }
}

void QQuickPixmap::clear()
{
    if (d) {
        d->release();
        d = nullptr;
    }
}

void QQuickPixmap::clear(QObject *obj)
{
    if (d) {
        if (d->reply)
            QObject::disconnect(d->reply, nullptr, obj, nullptr);
        d->release();
        d = nullptr;
    }
}

bool QQuickPixmap::isCached(const QUrl &url, const QRect &requestRegion, const QSize &requestSize,
                            const int frame, const QQuickImageProviderOptions &options)
{
    QQuickPixmapKey key = { &url, &requestRegion, &requestSize, frame, options };
    QQuickPixmapCache *store = QQuickPixmapCache::instance();

    return store->m_cache.contains(key);
}

bool QQuickPixmap::isScalableImageFormat(const QUrl &url)
{
    if (url.scheme() == "image"_L1)
        return true;

    const QString stringUrl = url.path(QUrl::PrettyDecoded);
    return stringUrl.endsWith("svg"_L1)
            || stringUrl.endsWith("svgz"_L1)
            || stringUrl.endsWith("pdf"_L1);
}

bool QQuickPixmap::connectFinished(QObject *object, const char *method)
{
    if (!d || !d->reply) {
        qWarning("QQuickPixmap: connectFinished() called when not loading.");
        return false;
    }

    return QObject::connect(d->reply, SIGNAL(finished()), object, method);
}

bool QQuickPixmap::connectFinished(QObject *object, int method)
{
    if (!d || !d->reply) {
        qWarning("QQuickPixmap: connectFinished() called when not loading.");
        return false;
    }

    return QMetaObject::connect(d->reply, QQuickPixmapReply::finishedMethodIndex, object, method);
}

bool QQuickPixmap::connectDownloadProgress(QObject *object, const char *method)
{
    if (!d || !d->reply) {
        qWarning("QQuickPixmap: connectDownloadProgress() called when not loading.");
        return false;
    }

    return QObject::connect(d->reply, SIGNAL(downloadProgress(qint64,qint64)), object,
                            method);
}

bool QQuickPixmap::connectDownloadProgress(QObject *object, int method)
{
    if (!d || !d->reply) {
        qWarning("QQuickPixmap: connectDownloadProgress() called when not loading.");
        return false;
    }

    return QMetaObject::connect(d->reply, QQuickPixmapReply::downloadProgressMethodIndex, object,
                                method);
}

QColorSpace QQuickPixmap::colorSpace() const
{
    if (!d || !d->textureFactory)
        return QColorSpace();
    return d->textureFactory->image().colorSpace();
}

QT_END_NAMESPACE

#include <qquickpixmapcache.moc>

#include "moc_qquickpixmapcache_p.cpp"
