// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant

#include <private/qqmlengine_p.h>
#include <private/qqmlextensionplugin_p.h>
#include <private/qqmltypeloaderthread_p.h>

#if QT_CONFIG(qml_network)
#include <private/qqmltypeloadernetworkreplyproxy_p.h>
#endif

QT_BEGIN_NAMESPACE

QQmlTypeLoaderThread::QQmlTypeLoaderThread(QQmlTypeLoader *loader)
    : m_loader(loader)
{
}

QQmlTypeLoaderThread::~QQmlTypeLoaderThread()
{
    // The thread has to be shutdown() first.
    Q_ASSERT(!thread()->isRunning());

    // Discard all remaining messages.
    // We don't need the lock anymore because the thread is dead.
    discardMessages();
}

#if QT_CONFIG(qml_network)
QNetworkAccessManager *QQmlTypeLoaderThread::networkAccessManager() const
{
    Q_ASSERT(isThisThread());
    if (!m_networkAccessManager) {
        m_networkAccessManager = m_loader->createNetworkAccessManager(nullptr);
        QObject::connect(thread(), &QThread::finished, m_networkAccessManager,
                         [this]() { std::exchange(m_networkAccessManager, nullptr)->deleteLater(); });
        m_networkReplyProxy = new QQmlTypeLoaderNetworkReplyProxy(m_loader, threadObject());
        QObject::connect(thread(), &QThread::finished, m_networkReplyProxy,
                         [this]() { std::exchange(m_networkReplyProxy, nullptr)->deleteLater(); });
    }

    return m_networkAccessManager;
}

QQmlTypeLoaderNetworkReplyProxy *QQmlTypeLoaderThread::networkReplyProxy() const
{
    Q_ASSERT(isThisThread());
    Q_ASSERT(m_networkReplyProxy); // Must call networkAccessManager() first
    return m_networkReplyProxy;
}
#endif // qml_network

void QQmlTypeLoaderThread::load(const QQmlDataBlob::Ptr &b)
{
    callMethodInThread(&This::loadThread, b);
}

void QQmlTypeLoaderThread::loadAsync(const QQmlDataBlob::Ptr &b)
{
    postMethodToThread(&This::loadThread, b);
}

void QQmlTypeLoaderThread::loadWithStaticData(const QQmlDataBlob::Ptr &b, const QByteArray &d)
{
    callMethodInThread(&This::loadWithStaticDataThread, b, d);
}

void QQmlTypeLoaderThread::loadWithStaticDataAsync(const QQmlDataBlob::Ptr &b, const QByteArray &d)
{
    postMethodToThread(&This::loadWithStaticDataThread, b, d);
}

void QQmlTypeLoaderThread::loadWithCachedUnit(const QQmlDataBlob::Ptr &b, const QQmlPrivate::CachedQmlUnit *unit)
{
    callMethodInThread(&This::loadWithCachedUnitThread, b, unit);
}

void QQmlTypeLoaderThread::loadWithCachedUnitAsync(const QQmlDataBlob::Ptr &b, const QQmlPrivate::CachedQmlUnit *unit)
{
    postMethodToThread(&This::loadWithCachedUnitThread, b, unit);
}

void QQmlTypeLoaderThread::callCompleted(const QQmlDataBlob::Ptr &b)
{
    postMethodToMain(&This::callCompletedMain, b);
}

void QQmlTypeLoaderThread::callDownloadProgressChanged(const QQmlDataBlob::Ptr &b, qreal p)
{
    postMethodToMain(&This::callDownloadProgressChangedMain, b, p);
}

void QQmlTypeLoaderThread::initializeEngine(QQmlExtensionInterface *iface,
                                            const char *uri)
{
    callMethodInMain(&This::initializeExtensionMain, iface, uri);
}

void QQmlTypeLoaderThread::initializeEngine(QQmlEngineExtensionInterface *iface,
                                            const char *uri)
{
    callMethodInMain(&This::initializeEngineExtensionMain, iface, uri);
}

void QQmlTypeLoaderThread::drop(const QQmlDataBlob::Ptr &b)
{
    postMethodToThread(&This::dropThread, b);
}

void QQmlTypeLoaderThread::loadThread(const QQmlDataBlob::Ptr &b)
{
    m_loader->loadThread(b);
}

void QQmlTypeLoaderThread::loadWithStaticDataThread(const QQmlDataBlob::Ptr &b, const QByteArray &d)
{
    m_loader->loadWithStaticDataThread(b, d);
}

void QQmlTypeLoaderThread::loadWithCachedUnitThread(const QQmlDataBlob::Ptr &b, const QQmlPrivate::CachedQmlUnit *unit)
{
    m_loader->loadWithCachedUnitThread(b, unit);
}

void QQmlTypeLoaderThread::callCompletedMain(const QQmlDataBlob::Ptr &b)
{
#ifdef DATABLOB_DEBUG
    qWarning("QQmlTypeLoaderThread: %s completed() callback", qPrintable(b->urlString()));
#endif
    b->completed();
}

void QQmlTypeLoaderThread::callDownloadProgressChangedMain(const QQmlDataBlob::Ptr &b, qreal p)
{
#ifdef DATABLOB_DEBUG
    qWarning("QQmlTypeLoaderThread: %s downloadProgressChanged(%f) callback",
             qPrintable(b->urlString()), p);
#endif
    b->downloadProgressChanged(p);
}

void QQmlTypeLoaderThread::initializeExtensionMain(QQmlExtensionInterface *iface,
                                                const char *uri)
{
    // We can use m_engine because we're on the engine thread.
    QQmlEngine *engine = m_loader->engine()->qmlEngine();
    Q_ASSERT(engine && engine->thread() == QThread::currentThread());
    iface->initializeEngine(engine, uri);
}

void QQmlTypeLoaderThread::initializeEngineExtensionMain(QQmlEngineExtensionInterface *iface,
                                                const char *uri)
{
    // We can use m_engine because we're on the engine thread.
    QQmlEngine *engine = m_loader->engine()->qmlEngine();
    Q_ASSERT(engine && engine->thread() == QThread::currentThread());
    iface->initializeEngine(engine, uri);
}

void QQmlTypeLoaderThread::dropThread(const QQmlDataBlob::Ptr &b)
{
    // Simply drop the reference to b
    Q_UNUSED(b);
}

QT_END_NAMESPACE
