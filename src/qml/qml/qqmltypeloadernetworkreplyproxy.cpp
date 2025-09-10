// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <private/qqmltypeloadernetworkreplyproxy_p.h>
#include <private/qqmltypeloader_p.h>

QT_BEGIN_NAMESPACE

QQmlTypeLoaderNetworkReplyProxy::QQmlTypeLoaderNetworkReplyProxy(QQmlTypeLoader *l)
    : l(l)
{
}

void QQmlTypeLoaderNetworkReplyProxy::finished()
{
    Q_ASSERT(sender());
    Q_ASSERT(qobject_cast<QNetworkReply *>(sender()));
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    l->networkReplyFinished(reply);
}

void QQmlTypeLoaderNetworkReplyProxy::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
    Q_ASSERT(sender());
    Q_ASSERT(qobject_cast<QNetworkReply *>(sender()));
    QNetworkReply *reply = static_cast<QNetworkReply *>(sender());
    l->networkReplyProgress(reply, bytesReceived, bytesTotal);
}

// This function is for when you want to shortcut the signals and call directly
void QQmlTypeLoaderNetworkReplyProxy::manualFinished(QNetworkReply *reply)
{
    qint64 replySize = reply->size();
    l->networkReplyProgress(reply, replySize, replySize);
    l->networkReplyFinished(reply);
}

QT_END_NAMESPACE

#include "moc_qqmltypeloadernetworkreplyproxy_p.cpp"
