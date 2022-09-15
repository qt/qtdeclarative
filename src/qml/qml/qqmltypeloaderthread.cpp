// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
    // Do that after initializing all the members.
    startup();
}

#if QT_CONFIG(qml_network)
QNetworkAccessManager *QQmlTypeLoaderThread::networkAccessManager() const
{
    Q_ASSERT(isThisThread());
    if (!m_networkAccessManager) {
        m_networkAccessManager = QQmlEnginePrivate::get(m_loader->engine())->createNetworkAccessManager(nullptr);
        QObject::connect(thread(), &QThread::finished, m_networkAccessManager, &QObject::deleteLater);
        m_networkReplyProxy = new QQmlTypeLoaderNetworkReplyProxy(m_loader);
        QObject::connect(thread(), &QThread::finished, m_networkReplyProxy, &QObject::deleteLater);
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
#if !QT_CONFIG(thread)
    if (!isThisThread())
        postMethodToThread(&This::callCompletedMain, b);
#else
    postMethodToMain(&This::callCompletedMain, b);
#endif
}

void QQmlTypeLoaderThread::callDownloadProgressChanged(const QQmlDataBlob::Ptr &b, qreal p)
{
#if !QT_CONFIG(thread)
    if (!isThisThread())
        postMethodToThread(&This::callDownloadProgressChangedMain, b, p);
#else
    postMethodToMain(&This::callDownloadProgressChangedMain, b, p);
#endif
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
    Q_ASSERT(m_loader->engine()->thread() == QThread::currentThread());
    iface->initializeEngine(m_loader->engine(), uri);
}

void QQmlTypeLoaderThread::initializeEngineExtensionMain(QQmlEngineExtensionInterface *iface,
                                                const char *uri)
{
    Q_ASSERT(m_loader->engine()->thread() == QThread::currentThread());
    iface->initializeEngine(m_loader->engine(), uri);
}

QT_END_NAMESPACE
