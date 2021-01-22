/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
    void loadWithCachedUnit(QQmlDataBlob *b, const QV4::CompiledData::Unit *unit);
    void loadWithCachedUnitAsync(QQmlDataBlob *b, const QV4::CompiledData::Unit *unit);
    void callCompleted(QQmlDataBlob *b);
    void callDownloadProgressChanged(QQmlDataBlob *b, qreal p);
    void initializeEngine(QQmlExtensionInterface *, const char *);
    void initializeEngine(QQmlEngineExtensionInterface *, const char *);

protected:
    void shutdownThread() override;

private:
    void loadThread(QQmlDataBlob *b);
    void loadWithStaticDataThread(QQmlDataBlob *b, const QByteArray &);
    void loadWithCachedUnitThread(QQmlDataBlob *b, const QV4::CompiledData::Unit *unit);
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
