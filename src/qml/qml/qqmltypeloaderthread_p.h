// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQMLTYPELOADERTHREAD_P_H
#define QQMLTYPELOADERTHREAD_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qqmlthread_p.h>
#include <private/qv4compileddata_p.h>

#include <QtQml/qtqmlglobal.h>

#if QT_CONFIG(qml_network)
#include <private/qqmltypeloadernetworkreplyproxy_p.h>
#include <QtNetwork/qnetworkaccessmanager.h>
#endif

QT_BEGIN_NAMESPACE

class QQmlDataBlob;
class QQmlTypeLoader;
class QQmlEngineExtensionInterface;
class QQmlExtensionInterface;

class QQmlTypeLoaderThread : public QQmlThread
{
    typedef QQmlTypeLoaderThread This;

public:
    QQmlTypeLoaderThread(QQmlTypeLoader *loader);
#if QT_CONFIG(qml_network)
    QNetworkAccessManager *networkAccessManager() const;
    QQmlTypeLoaderNetworkReplyProxy *networkReplyProxy() const;
#endif // qml_network
    void load(QQmlDataBlob *b);
    void loadAsync(QQmlDataBlob *b);
    void loadWithStaticData(QQmlDataBlob *b, const QByteArray &);
    void loadWithStaticDataAsync(QQmlDataBlob *b, const QByteArray &);
    void loadWithCachedUnit(QQmlDataBlob *b, const QQmlPrivate::CachedQmlUnit *unit);
    void loadWithCachedUnitAsync(QQmlDataBlob *b, const QQmlPrivate::CachedQmlUnit *unit);
    void callCompleted(QQmlDataBlob *b);
    void callDownloadProgressChanged(QQmlDataBlob *b, qreal p);
    void initializeEngine(QQmlExtensionInterface *, const char *);
    void initializeEngine(QQmlEngineExtensionInterface *, const char *);

protected:
    void shutdownThread() override;

private:
    void loadThread(QQmlDataBlob *b);
    void loadWithStaticDataThread(QQmlDataBlob *b, const QByteArray &);
    void loadWithCachedUnitThread(QQmlDataBlob *b, const QQmlPrivate::CachedQmlUnit *unit);
    void callCompletedMain(QQmlDataBlob *b);
    void callDownloadProgressChangedMain(QQmlDataBlob *b, qreal p);
    void initializeExtensionMain(QQmlExtensionInterface *iface, const char *uri);
    void initializeEngineExtensionMain(QQmlEngineExtensionInterface *iface, const char *uri);

    QQmlTypeLoader *m_loader;
#if QT_CONFIG(qml_network)
    mutable QNetworkAccessManager *m_networkAccessManager;
    mutable QQmlTypeLoaderNetworkReplyProxy *m_networkReplyProxy;
#endif // qml_network
};

QT_END_NAMESPACE

#endif // QQMLTYPELOADERTHREAD_P_H
