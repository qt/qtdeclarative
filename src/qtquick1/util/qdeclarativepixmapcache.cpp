/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "QtQuick1/private/qdeclarativepixmapcache_p.h"
#include "QtDeclarative/qdeclarativenetworkaccessmanagerfactory.h"
#include "QtDeclarative/qdeclarativeimageprovider.h"

#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/private/qdeclarativeglobal_p.h>
#include <QtDeclarative/private/qdeclarativeengine_p.h>

#include <QCoreApplication>
#include <QImageReader>
#include <QHash>
#include <QNetworkReply>
#include <QPixmapCache>
#include <QFile>
#include <QThread>
#include <QMutex>
#include <QMutexLocker>
#include <QWaitCondition>
#include <QBuffer>
#include <QWaitCondition>
#include <QtCore/qdebug.h>
#include <private/qobject_p.h>
#include <QSslError>

#define IMAGEREQUEST_MAX_REQUEST_COUNT       8
#define IMAGEREQUEST_MAX_REDIRECT_RECURSION 16
#define CACHE_EXPIRE_TIME 30
#define CACHE_REMOVAL_FRACTION 4

QT_BEGIN_NAMESPACE



// The cache limit describes the maximum "junk" in the cache.
// These are the same defaults as QPixmapCache
#if defined(Q_WS_QWS) || defined(Q_WS_WINCE)
static int cache_limit = 2048 * 1024; // 2048 KB cache limit for embedded
#else
static int cache_limit = 10240 * 1024; // 10 MB cache limit for desktop
#endif

class QDeclarative1PixmapReader;
class QDeclarative1PixmapData;
class QDeclarative1PixmapReply : public QObject
{
    Q_OBJECT
public:
    enum ReadError { NoError, Loading, Decoding };

    QDeclarative1PixmapReply(QDeclarative1PixmapData *);
    ~QDeclarative1PixmapReply();

    QDeclarative1PixmapData *data;
    QDeclarative1PixmapReader *reader;
    QSize requestSize;

    bool loading;
    int redirectCount;

    class Event : public QEvent {
    public:
        Event(ReadError, const QString &, const QSize &, const QImage &);

        ReadError error;
        QString errorString;
        QSize implicitSize;
        QImage image;
    };
    void postReply(ReadError, const QString &, const QSize &, const QImage &);


Q_SIGNALS:
    void finished();
    void downloadProgress(qint64, qint64);

protected:
    bool event(QEvent *event);

private:
    Q_DISABLE_COPY(QDeclarative1PixmapReply)

public:
    static int finishedIndex;
    static int downloadProgressIndex;
};

class QDeclarative1PixmapReaderThreadObject : public QObject {
    Q_OBJECT
public:
    QDeclarative1PixmapReaderThreadObject(QDeclarative1PixmapReader *);
    void processJobs();
    virtual bool event(QEvent *e);
private slots:
    void networkRequestDone();
private:
    QDeclarative1PixmapReader *reader;
};

class QDeclarative1PixmapData;
class QDeclarative1PixmapReader : public QThread
{
    Q_OBJECT
public:
    QDeclarative1PixmapReader(QDeclarativeEngine *eng);
    ~QDeclarative1PixmapReader();

    QDeclarative1PixmapReply *getImage(QDeclarative1PixmapData *);
    void cancel(QDeclarative1PixmapReply *rep);

    static QDeclarative1PixmapReader *instance(QDeclarativeEngine *engine);

protected:
    void run();

private:
    friend class QDeclarative1PixmapReaderThreadObject;
    void processJobs();
    void processJob(QDeclarative1PixmapReply *, const QUrl &, const QSize &);
    void networkRequestDone(QNetworkReply *);

    QList<QDeclarative1PixmapReply*> jobs;
    QList<QDeclarative1PixmapReply*> cancelled;
    QDeclarativeEngine *engine;
    QObject *eventLoopQuitHack;

    QMutex mutex;
    QDeclarative1PixmapReaderThreadObject *threadObject;
    QWaitCondition waitCondition;

    QNetworkAccessManager *networkAccessManager();
    QNetworkAccessManager *accessManager;

    QHash<QNetworkReply*,QDeclarative1PixmapReply*> replies;

    static int replyDownloadProgress;
    static int replyFinished;
    static int downloadProgress;
    static int threadNetworkRequestDone;
    static QHash<QDeclarativeEngine *,QDeclarative1PixmapReader*> readers;
    static QMutex readerMutex;
};

class QDeclarative1PixmapData
{
public:
    QDeclarative1PixmapData(const QUrl &u, const QSize &s, const QString &e)
    : refCount(1), inCache(false), pixmapStatus(QDeclarative1Pixmap::Error), 
      url(u), errorString(e), requestSize(s), reply(0), prevUnreferenced(0),
      prevUnreferencedPtr(0), nextUnreferenced(0)
    {
    }

    QDeclarative1PixmapData(const QUrl &u, const QSize &r)
    : refCount(1), inCache(false), pixmapStatus(QDeclarative1Pixmap::Loading), 
      url(u), requestSize(r), reply(0), prevUnreferenced(0), prevUnreferencedPtr(0), 
      nextUnreferenced(0)
    {
    }

    QDeclarative1PixmapData(const QUrl &u, const QPixmap &p, const QSize &s, const QSize &r)
    : refCount(1), inCache(false), privatePixmap(false), pixmapStatus(QDeclarative1Pixmap::Ready), 
      url(u), pixmap(p), implicitSize(s), requestSize(r), reply(0), prevUnreferenced(0),
      prevUnreferencedPtr(0), nextUnreferenced(0)
    {
    }

    QDeclarative1PixmapData(const QPixmap &p)
    : refCount(1), inCache(false), privatePixmap(true), pixmapStatus(QDeclarative1Pixmap::Ready),
      pixmap(p), implicitSize(p.size()), requestSize(p.size()), reply(0), prevUnreferenced(0),
      prevUnreferencedPtr(0), nextUnreferenced(0)
    {
    }

    int cost() const;
    void addref();
    void release();
    void addToCache();
    void removeFromCache();

    uint refCount;

    bool inCache:1;
    bool privatePixmap:1;
    
    QDeclarative1Pixmap::Status pixmapStatus;
    QUrl url;
    QString errorString;
    QPixmap pixmap;
    QSize implicitSize;
    QSize requestSize;

    QDeclarative1PixmapReply *reply;

    QDeclarative1PixmapData *prevUnreferenced;
    QDeclarative1PixmapData**prevUnreferencedPtr;
    QDeclarative1PixmapData *nextUnreferenced;
};

int QDeclarative1PixmapReply::finishedIndex = -1;
int QDeclarative1PixmapReply::downloadProgressIndex = -1;

// XXX
QHash<QDeclarativeEngine *,QDeclarative1PixmapReader*> QDeclarative1PixmapReader::readers;
QMutex QDeclarative1PixmapReader::readerMutex;

int QDeclarative1PixmapReader::replyDownloadProgress = -1;
int QDeclarative1PixmapReader::replyFinished = -1;
int QDeclarative1PixmapReader::downloadProgress = -1;
int QDeclarative1PixmapReader::threadNetworkRequestDone = -1;


void QDeclarative1PixmapReply::postReply(ReadError error, const QString &errorString, 
                                        const QSize &implicitSize, const QImage &image)
{
    loading = false;
    QCoreApplication::postEvent(this, new Event(error, errorString, implicitSize, image));
}

QDeclarative1PixmapReply::Event::Event(ReadError e, const QString &s, const QSize &iSize, const QImage &i)
: QEvent(QEvent::User), error(e), errorString(s), implicitSize(iSize), image(i)
{
}

QNetworkAccessManager *QDeclarative1PixmapReader::networkAccessManager()
{
    if (!accessManager) {
        Q_ASSERT(threadObject);
        accessManager = QDeclarativeEnginePrivate::get(engine)->createNetworkAccessManager(threadObject);
    }
    return accessManager;
}

static bool readImage(const QUrl& url, QIODevice *dev, QImage *image, QString *errorString, QSize *impsize, 
                      const QSize &requestSize)
{
    QImageReader imgio(dev);

    bool force_scale = false;
    if (url.path().endsWith(QLatin1String(".svg"),Qt::CaseInsensitive)) {
        imgio.setFormat("svg"); // QSvgPlugin::capabilities bug QTBUG-9053
        force_scale = true;
    }

    bool scaled = false;
    if (requestSize.width() > 0 || requestSize.height() > 0) {
        QSize s = imgio.size();
        if (requestSize.width() && (force_scale || requestSize.width() < s.width())) {
            if (requestSize.height() <= 0)
                s.setHeight(s.height()*requestSize.width()/s.width());
            s.setWidth(requestSize.width()); scaled = true;
        }
        if (requestSize.height() && (force_scale || requestSize.height() < s.height())) {
            if (requestSize.width() <= 0)
                s.setWidth(s.width()*requestSize.height()/s.height());
            s.setHeight(requestSize.height()); scaled = true;
        }
        if (scaled) { imgio.setScaledSize(s); }
    }

    if (impsize)
        *impsize = imgio.size();

    if (imgio.read(image)) {
        if (impsize && impsize->width() < 0)
            *impsize = image->size();
        return true;
    } else {
        if (errorString)
            *errorString = QDeclarative1Pixmap::tr("Error decoding: %1: %2").arg(url.toString())
                                .arg(imgio.errorString());
        return false;
    }
}

QDeclarative1PixmapReader::QDeclarative1PixmapReader(QDeclarativeEngine *eng)
: QThread(eng), engine(eng), threadObject(0), accessManager(0)
{
    eventLoopQuitHack = new QObject;
    eventLoopQuitHack->moveToThread(this);
    connect(eventLoopQuitHack, SIGNAL(destroyed(QObject*)), SLOT(quit()), Qt::DirectConnection);
    start(QThread::IdlePriority);
}

QDeclarative1PixmapReader::~QDeclarative1PixmapReader()
{
    readerMutex.lock();
    readers.remove(engine);
    readerMutex.unlock();

    eventLoopQuitHack->deleteLater();
    wait();
}

void QDeclarative1PixmapReader::networkRequestDone(QNetworkReply *reply)
{
    QDeclarative1PixmapReply *job = replies.take(reply);

    if (job) {
        job->redirectCount++;
        if (job->redirectCount < IMAGEREQUEST_MAX_REDIRECT_RECURSION) {
            QVariant redirect = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
            if (redirect.isValid()) {
                QUrl url = reply->url().resolved(redirect.toUrl());
                QNetworkRequest req(url);
                req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);

                reply->deleteLater();
                reply = networkAccessManager()->get(req);

                QMetaObject::connect(reply, replyDownloadProgress, job, downloadProgress);
                QMetaObject::connect(reply, replyFinished, threadObject, threadNetworkRequestDone);

                replies.insert(reply, job);
                return;
            }
        }

        QImage image;
        QDeclarative1PixmapReply::ReadError error = QDeclarative1PixmapReply::NoError;
        QString errorString;
        QSize readSize;
        if (reply->error()) {
            error = QDeclarative1PixmapReply::Loading;
            errorString = reply->errorString();
        } else {
            QByteArray all = reply->readAll();
            QBuffer buff(&all);
            buff.open(QIODevice::ReadOnly);
            if (!readImage(reply->url(), &buff, &image, &errorString, &readSize, job->requestSize)) {
                error = QDeclarative1PixmapReply::Decoding;
            }
        }
        // send completion event to the QDeclarative1PixmapReply
        mutex.lock();
        if (!cancelled.contains(job)) job->postReply(error, errorString, readSize, image);
        mutex.unlock();
    }
    reply->deleteLater();

    // kick off event loop again incase we have dropped below max request count
    threadObject->processJobs();
}

QDeclarative1PixmapReaderThreadObject::QDeclarative1PixmapReaderThreadObject(QDeclarative1PixmapReader *i)
: reader(i)
{
}

void QDeclarative1PixmapReaderThreadObject::processJobs() 
{ 
    QCoreApplication::postEvent(this, new QEvent(QEvent::User)); 
}

bool QDeclarative1PixmapReaderThreadObject::event(QEvent *e) 
{
    if (e->type() == QEvent::User) { 
        reader->processJobs(); 
        return true; 
    } else { 
        return QObject::event(e);
    }
}

void QDeclarative1PixmapReaderThreadObject::networkRequestDone()
{
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    reader->networkRequestDone(reply);
}

void QDeclarative1PixmapReader::processJobs()
{
    QMutexLocker locker(&mutex);

    while (true) {
        if (cancelled.isEmpty() && (jobs.isEmpty() || replies.count() >= IMAGEREQUEST_MAX_REQUEST_COUNT)) 
            return; // Nothing else to do

        // Clean cancelled jobs
        if (cancelled.count()) {
            for (int i = 0; i < cancelled.count(); ++i) {
                QDeclarative1PixmapReply *job = cancelled.at(i);
                QNetworkReply *reply = replies.key(job, 0);
                if (reply && reply->isRunning()) {
                    // cancel any jobs already started
                    replies.remove(reply);
                    reply->close();
                }
                // deleteLater, since not owned by this thread
                job->deleteLater();
            }
            cancelled.clear();
        }

        if (!jobs.isEmpty() && replies.count() < IMAGEREQUEST_MAX_REQUEST_COUNT) {
            QDeclarative1PixmapReply *runningJob = jobs.takeLast();
            runningJob->loading = true;

            QUrl url = runningJob->data->url;
            QSize requestSize = runningJob->data->requestSize;
            locker.unlock();
            processJob(runningJob, url, requestSize);
            locker.relock();
        }
    }
}

void QDeclarative1PixmapReader::processJob(QDeclarative1PixmapReply *runningJob, const QUrl &url, 
                                          const QSize &requestSize)
{
    // fetch
    if (url.scheme() == QLatin1String("image")) {
        // Use QmlImageProvider
        QSize readSize;
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);
        QImage image = ep->getImageFromProvider(url, &readSize, requestSize);

        QDeclarative1PixmapReply::ReadError errorCode = QDeclarative1PixmapReply::NoError;
        QString errorStr;
        if (image.isNull()) {
            errorCode = QDeclarative1PixmapReply::Loading;
            errorStr = QDeclarative1Pixmap::tr("Failed to get image from provider: %1").arg(url.toString());
        }

        mutex.lock();
        if (!cancelled.contains(runningJob)) runningJob->postReply(errorCode, errorStr, readSize, image);
        mutex.unlock();
    } else {
        QString lf = QDeclarativeEnginePrivate::urlToLocalFileOrQrc(url);
        if (!lf.isEmpty()) {
            // Image is local - load/decode immediately
            QImage image;
            QDeclarative1PixmapReply::ReadError errorCode = QDeclarative1PixmapReply::NoError;
            QString errorStr;
            QFile f(lf);
            QSize readSize;
            if (f.open(QIODevice::ReadOnly)) {
                if (!readImage(url, &f, &image, &errorStr, &readSize, requestSize))
                    errorCode = QDeclarative1PixmapReply::Loading;
            } else {
                errorStr = QDeclarative1Pixmap::tr("Cannot open: %1").arg(url.toString());
                errorCode = QDeclarative1PixmapReply::Loading;
            }
            mutex.lock();
            if (!cancelled.contains(runningJob)) runningJob->postReply(errorCode, errorStr, readSize, image);
            mutex.unlock();
        } else {
            // Network resource
            QNetworkRequest req(url);
            req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
            QNetworkReply *reply = networkAccessManager()->get(req);

            QMetaObject::connect(reply, replyDownloadProgress, runningJob, downloadProgress);
            QMetaObject::connect(reply, replyFinished, threadObject, threadNetworkRequestDone);

            replies.insert(reply, runningJob);
        }
    }
}

QDeclarative1PixmapReader *QDeclarative1PixmapReader::instance(QDeclarativeEngine *engine)
{
    readerMutex.lock();
    QDeclarative1PixmapReader *reader = readers.value(engine);
    if (!reader) {
        reader = new QDeclarative1PixmapReader(engine);
        readers.insert(engine, reader);
    }
    readerMutex.unlock();

    return reader;
}

QDeclarative1PixmapReply *QDeclarative1PixmapReader::getImage(QDeclarative1PixmapData *data)
{
    mutex.lock();
    QDeclarative1PixmapReply *reply = new QDeclarative1PixmapReply(data);
    reply->reader = this;
    jobs.append(reply);
    // XXX 
    if (threadObject) threadObject->processJobs();
    mutex.unlock();
    return reply;
}

void QDeclarative1PixmapReader::cancel(QDeclarative1PixmapReply *reply)
{
    mutex.lock();
    if (reply->loading) {
        cancelled.append(reply);
        reply->data = 0;
        // XXX 
        if (threadObject) threadObject->processJobs();
    } else {
        jobs.removeAll(reply);
        delete reply;
    }
    mutex.unlock();
}

void QDeclarative1PixmapReader::run()
{
    if (replyDownloadProgress == -1) {
        const QMetaObject *nr = &QNetworkReply::staticMetaObject;
        const QMetaObject *pr = &QDeclarative1PixmapReply::staticMetaObject;
        const QMetaObject *ir = &QDeclarative1PixmapReaderThreadObject::staticMetaObject;
        replyDownloadProgress = nr->indexOfSignal("downloadProgress(qint64,qint64)");
        replyFinished = nr->indexOfSignal("finished()");
        downloadProgress = pr->indexOfSignal("downloadProgress(qint64,qint64)");
        threadNetworkRequestDone = ir->indexOfSlot("networkRequestDone()");
    }

    mutex.lock();
    threadObject = new QDeclarative1PixmapReaderThreadObject(this);
    mutex.unlock();

    processJobs();
    exec();

    delete threadObject;
    threadObject = 0;
}

class QDeclarative1PixmapKey
{
public:
    const QUrl *url;
    const QSize *size;
};

inline bool operator==(const QDeclarative1PixmapKey &lhs, const QDeclarative1PixmapKey &rhs)
{
    return *lhs.size == *rhs.size && *lhs.url == *rhs.url;
}

inline uint qHash(const QDeclarative1PixmapKey &key)
{
    return qHash(*key.url) ^ key.size->width() ^ key.size->height();
}

class QDeclarative1PixmapStore : public QObject
{
    Q_OBJECT
public:
    QDeclarative1PixmapStore();

    void unreferencePixmap(QDeclarative1PixmapData *);
    void referencePixmap(QDeclarative1PixmapData *);

protected:
    virtual void timerEvent(QTimerEvent *);

public:
    QHash<QDeclarative1PixmapKey, QDeclarative1PixmapData *> m_cache;

private:
    void shrinkCache(int remove);

    QDeclarative1PixmapData *m_unreferencedPixmaps;
    QDeclarative1PixmapData *m_lastUnreferencedPixmap;

    int m_unreferencedCost;
    int m_timerId;
};

Q_GLOBAL_STATIC(QDeclarative1PixmapStore, pixmapStore)

QDeclarative1PixmapStore::QDeclarative1PixmapStore()
: m_unreferencedPixmaps(0), m_lastUnreferencedPixmap(0), m_unreferencedCost(0), m_timerId(-1)
{
}

void QDeclarative1PixmapStore::unreferencePixmap(QDeclarative1PixmapData *data)
{
    Q_ASSERT(data->prevUnreferenced == 0);
    Q_ASSERT(data->prevUnreferencedPtr == 0);
    Q_ASSERT(data->nextUnreferenced == 0);

    data->nextUnreferenced = m_unreferencedPixmaps;
    data->prevUnreferencedPtr = &m_unreferencedPixmaps;

    m_unreferencedPixmaps = data;
    if (m_unreferencedPixmaps->nextUnreferenced) {
        m_unreferencedPixmaps->nextUnreferenced->prevUnreferenced = m_unreferencedPixmaps;
        m_unreferencedPixmaps->nextUnreferenced->prevUnreferencedPtr = &m_unreferencedPixmaps->nextUnreferenced;
    }

    if (!m_lastUnreferencedPixmap)
        m_lastUnreferencedPixmap = data;

    m_unreferencedCost += data->cost();

    shrinkCache(-1); // Shrink the cache incase it has become larger than cache_limit

    if (m_timerId == -1 && m_unreferencedPixmaps) 
        m_timerId = startTimer(CACHE_EXPIRE_TIME * 1000);
}

void QDeclarative1PixmapStore::referencePixmap(QDeclarative1PixmapData *data)
{
    Q_ASSERT(data->prevUnreferencedPtr);

    *data->prevUnreferencedPtr = data->nextUnreferenced;
    if (data->nextUnreferenced) { 
        data->nextUnreferenced->prevUnreferencedPtr = data->prevUnreferencedPtr;
        data->nextUnreferenced->prevUnreferenced = data->prevUnreferenced;
    }
    if (m_lastUnreferencedPixmap == data)
        m_lastUnreferencedPixmap = data->prevUnreferenced;

    data->nextUnreferenced = 0;
    data->prevUnreferencedPtr = 0;
    data->prevUnreferenced = 0;

    m_unreferencedCost -= data->cost();
}

void QDeclarative1PixmapStore::shrinkCache(int remove)
{
    while ((remove > 0 || m_unreferencedCost > cache_limit) && m_lastUnreferencedPixmap) {
        QDeclarative1PixmapData *data = m_lastUnreferencedPixmap;
        Q_ASSERT(data->nextUnreferenced == 0);

        *data->prevUnreferencedPtr = 0;
        m_lastUnreferencedPixmap = data->prevUnreferenced;
        data->prevUnreferencedPtr = 0;
        data->prevUnreferenced = 0;

        remove -= data->cost();
        m_unreferencedCost -= data->cost();
        data->removeFromCache();
        delete data;
    }
}

void QDeclarative1PixmapStore::timerEvent(QTimerEvent *)
{
    int removalCost = m_unreferencedCost / CACHE_REMOVAL_FRACTION;

    shrinkCache(removalCost);

    if (m_unreferencedPixmaps == 0) {
        killTimer(m_timerId);
        m_timerId = -1;
    }
}

QDeclarative1PixmapReply::QDeclarative1PixmapReply(QDeclarative1PixmapData *d)
: data(d), reader(0), requestSize(d->requestSize), loading(false), redirectCount(0)
{
    if (finishedIndex == -1) {
        finishedIndex = QDeclarative1PixmapReply::staticMetaObject.indexOfSignal("finished()");
        downloadProgressIndex = QDeclarative1PixmapReply::staticMetaObject.indexOfSignal("downloadProgress(qint64,qint64)");
    }
}

QDeclarative1PixmapReply::~QDeclarative1PixmapReply()
{
}

bool QDeclarative1PixmapReply::event(QEvent *event)
{
    if (event->type() == QEvent::User) {

        if (data) {
            Event *de = static_cast<Event *>(event);
            data->pixmapStatus = (de->error == NoError) ? QDeclarative1Pixmap::Ready : QDeclarative1Pixmap::Error;
            
            if (data->pixmapStatus == QDeclarative1Pixmap::Ready) {
                data->pixmap = QPixmap::fromImage(de->image);
                data->implicitSize = de->implicitSize;
            } else {
                data->errorString = de->errorString;
                data->removeFromCache(); // We don't continue to cache error'd pixmaps
            }

            data->reply = 0;
            emit finished();
        }

        delete this;
        return true;
    } else {
        return QObject::event(event);
    }
}

int QDeclarative1PixmapData::cost() const
{
    return (pixmap.width() * pixmap.height() * pixmap.depth()) / 8;
}

void QDeclarative1PixmapData::addref()
{
    ++refCount;
    if (prevUnreferencedPtr) 
        pixmapStore()->referencePixmap(this);
}

void QDeclarative1PixmapData::release()
{
    Q_ASSERT(refCount > 0);
    --refCount;

    if (refCount == 0) {
        if (reply) {
            reply->reader->cancel(reply);
            reply = 0;
        }

        if (pixmapStatus == QDeclarative1Pixmap::Ready) {
            pixmapStore()->unreferencePixmap(this);
        } else {
            removeFromCache();
            delete this;
        }
    }
}

void QDeclarative1PixmapData::addToCache()
{
    if (!inCache) {
        QDeclarative1PixmapKey key = { &url, &requestSize };
        pixmapStore()->m_cache.insert(key, this);
        inCache = true;
    }
}

void QDeclarative1PixmapData::removeFromCache()
{
    if (inCache) {
        QDeclarative1PixmapKey key = { &url, &requestSize };
        pixmapStore()->m_cache.remove(key);
        inCache = false;
    }
}

static QDeclarative1PixmapData* createPixmapDataSync(QDeclarativeEngine *engine, const QUrl &url, const QSize &requestSize, bool *ok)
{
    if (url.scheme() == QLatin1String("image")) {
        QSize readSize;
        QDeclarativeEnginePrivate *ep = QDeclarativeEnginePrivate::get(engine);
        QDeclarativeImageProvider::ImageType imageType = ep->getImageProviderType(url);

        switch (imageType) {
            case QDeclarativeImageProvider::Image:
            {
                QImage image = ep->getImageFromProvider(url, &readSize, requestSize);
                if (!image.isNull()) {
                    *ok = true;
                    return new QDeclarative1PixmapData(url, QPixmap::fromImage(image), readSize, requestSize);
                }
            }
            break;
            case QDeclarativeImageProvider::Pixmap:
            {
                QPixmap pixmap = ep->getPixmapFromProvider(url, &readSize, requestSize);
                if (!pixmap.isNull()) {
                    *ok = true;
                    return new QDeclarative1PixmapData(url, pixmap, readSize, requestSize);
                }
            }
            break;
        case QDeclarativeImageProvider::Texture:
        case QDeclarativeImageProvider::Invalid:
            break;
        }

        // no matching provider, or provider has bad image type, or provider returned null image
        return new QDeclarative1PixmapData(url, requestSize,
            QDeclarative1Pixmap::tr("Failed to get image from provider: %1").arg(url.toString()));
    }

    QString localFile = QDeclarativeEnginePrivate::urlToLocalFileOrQrc(url);
    if (localFile.isEmpty()) 
        return 0;

    QFile f(localFile);
    QSize readSize;
    QString errorString;

    if (f.open(QIODevice::ReadOnly)) {
        QImage image;
        if (readImage(url, &f, &image, &errorString, &readSize, requestSize)) {
            *ok = true;
            return new QDeclarative1PixmapData(url, QPixmap::fromImage(image), readSize, requestSize);
        }
    } else {
        errorString = QDeclarative1Pixmap::tr("Cannot open: %1").arg(url.toString());
    }
    return new QDeclarative1PixmapData(url, requestSize, errorString);
}


struct QDeclarative1PixmapNull {
    QUrl url;
    QPixmap pixmap;
    QSize size;
};
Q_GLOBAL_STATIC(QDeclarative1PixmapNull, nullPixmap);

QDeclarative1Pixmap::QDeclarative1Pixmap()
: d(0)
{
}

QDeclarative1Pixmap::QDeclarative1Pixmap(QDeclarativeEngine *engine, const QUrl &url)
: d(0)
{
    load(engine, url);
}

QDeclarative1Pixmap::QDeclarative1Pixmap(QDeclarativeEngine *engine, const QUrl &url, const QSize &size)
: d(0)
{
    load(engine, url, size);
}

QDeclarative1Pixmap::~QDeclarative1Pixmap()
{
    if (d) {
        d->release();
        d = 0;
    }
}

bool QDeclarative1Pixmap::isNull() const
{
    return d == 0;
}

bool QDeclarative1Pixmap::isReady() const
{
    return status() == Ready;
}

bool QDeclarative1Pixmap::isError() const
{
    return status() == Error;
}

bool QDeclarative1Pixmap::isLoading() const
{
    return status() == Loading;
}

QString QDeclarative1Pixmap::error() const
{
    if (d)
        return d->errorString;
    else
        return QString();
}

QDeclarative1Pixmap::Status QDeclarative1Pixmap::status() const
{
    if (d)
        return d->pixmapStatus;
    else
        return Null;
}

const QUrl &QDeclarative1Pixmap::url() const
{
    if (d)
        return d->url;
    else
        return nullPixmap()->url;
}

const QSize &QDeclarative1Pixmap::implicitSize() const
{
    if (d) 
        return d->implicitSize;
    else
        return nullPixmap()->size;
}

const QSize &QDeclarative1Pixmap::requestSize() const
{
    if (d)
        return d->requestSize;
    else
        return nullPixmap()->size;
}

const QPixmap &QDeclarative1Pixmap::pixmap() const
{
    if (d) 
        return d->pixmap;
    else
        return nullPixmap()->pixmap;
}

void QDeclarative1Pixmap::setPixmap(const QPixmap &p) 
{
    clear();

    if (!p.isNull())
        d = new QDeclarative1PixmapData(p);
}

int QDeclarative1Pixmap::width() const
{
    if (d) 
        return d->pixmap.width();
    else
        return 0;
}

int QDeclarative1Pixmap::height() const
{
    if (d) 
        return d->pixmap.height();
    else
        return 0;
}

QRect QDeclarative1Pixmap::rect() const
{
    if (d)
        return d->pixmap.rect();
    else
        return QRect();
}

void QDeclarative1Pixmap::load(QDeclarativeEngine *engine, const QUrl &url)
{
    load(engine, url, QSize(), QDeclarative1Pixmap::Cache);
}

void QDeclarative1Pixmap::load(QDeclarativeEngine *engine, const QUrl &url, QDeclarative1Pixmap::Options options)
{
    load(engine, url, QSize(), options);
}

void QDeclarative1Pixmap::load(QDeclarativeEngine *engine, const QUrl &url, const QSize &size)
{
    load(engine, url, size, QDeclarative1Pixmap::Cache);
}

void QDeclarative1Pixmap::load(QDeclarativeEngine *engine, const QUrl &url, const QSize &requestSize, QDeclarative1Pixmap::Options options)
{
    if (d) { d->release(); d = 0; }

    QDeclarative1PixmapKey key = { &url, &requestSize };
    QDeclarative1PixmapStore *store = pixmapStore();

    QHash<QDeclarative1PixmapKey, QDeclarative1PixmapData *>::Iterator iter = store->m_cache.find(key);

    if (iter == store->m_cache.end()) {
        if (options & QDeclarative1Pixmap::Asynchronous) {
            // pixmaps can only be loaded synchronously
            if (url.scheme() == QLatin1String("image") 
                    && QDeclarativeEnginePrivate::get(engine)->getImageProviderType(url) == QDeclarativeImageProvider::Pixmap) {
                options &= ~QDeclarative1Pixmap::Asynchronous;
            }
        }

        if (!(options & QDeclarative1Pixmap::Asynchronous)) {
            bool ok = false;
            d = createPixmapDataSync(engine, url, requestSize, &ok);
            if (ok) {
                if (options & QDeclarative1Pixmap::Cache)
                    d->addToCache();
                return;
            }
            if (d)  // loadable, but encountered error while loading
                return;
        } 

        if (!engine)
            return;

        QDeclarative1PixmapReader *reader = QDeclarative1PixmapReader::instance(engine);

        d = new QDeclarative1PixmapData(url, requestSize);
        if (options & QDeclarative1Pixmap::Cache)
            d->addToCache();

        d->reply = reader->getImage(d);
    } else {
        d = *iter;
        d->addref();
    }
}

void QDeclarative1Pixmap::clear()
{
    if (d) {
        d->release();
        d = 0;
    }
}

void QDeclarative1Pixmap::clear(QObject *obj)
{
    if (d) {
        if (d->reply) 
            QObject::disconnect(d->reply, 0, obj, 0);
        d->release();
        d = 0;
    }
}

bool QDeclarative1Pixmap::connectFinished(QObject *object, const char *method)
{
    if (!d || !d->reply) {
        qWarning("QDeclarative1Pixmap: connectFinished() called when not loading.");
        return false;
    }

    return QObject::connect(d->reply, SIGNAL(finished()), object, method);
}

bool QDeclarative1Pixmap::connectFinished(QObject *object, int method)
{
    if (!d || !d->reply) {
        qWarning("QDeclarative1Pixmap: connectFinished() called when not loading.");
        return false;
    }

    return QMetaObject::connect(d->reply, QDeclarative1PixmapReply::finishedIndex, object, method);
}

bool QDeclarative1Pixmap::connectDownloadProgress(QObject *object, const char *method)
{
    if (!d || !d->reply) {
        qWarning("QDeclarative1Pixmap: connectDownloadProgress() called when not loading.");
        return false;
    }

    return QObject::connect(d->reply, SIGNAL(downloadProgress(qint64,qint64)), object, method);
}

bool QDeclarative1Pixmap::connectDownloadProgress(QObject *object, int method)
{
    if (!d || !d->reply) {
        qWarning("QDeclarative1Pixmap: connectDownloadProgress() called when not loading.");
        return false;
    }

    return QMetaObject::connect(d->reply, QDeclarative1PixmapReply::downloadProgressIndex, object, method);
}



QT_END_NAMESPACE

#include <qdeclarativepixmapcache.moc>
