// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "deviceloadingimage.h"

#include <QtQuick/private/qquickimage_p_p.h>

Q_DECLARE_LOGGING_CATEGORY(lcTests)

void DeviceLoadingImage::load()
{
    auto *d = static_cast<QQuickImagePrivate *>(QQuickImagePrivate::get(this));
    static int thisRequestFinished = -1;
    if (thisRequestFinished == -1) {
        thisRequestFinished =
                QQuickImageBase::staticMetaObject.indexOfSlot("requestFinished()");
    }
    const QQmlContext *context = qmlContext(this);
    Q_ASSERT(context);
    QUrl resolved = context->resolvedUrl(d->url);
    device = std::make_unique<QFile>(resolved.toLocalFile());
    const bool statusChange = (d->status != Loading);
    if (statusChange)
        d->status = Loading;
    d->pendingPix->loadImageFromDevice(qmlEngine(this), device.get(), context->resolvedUrl(d->url),
                               d->sourceClipRect.toRect(), d->sourcesize * d->devicePixelRatio,
                               QQuickImageProviderOptions(), d->currentFrame, d->frameCount);
    connectSuccess &= d->pendingPix->connectFinished(this, thisRequestFinished);
    connectSuccess &= d->pendingPix->connectFinished(this, SLOT(onRequestFinished()));
    qCDebug(lcTests) << "loading page" << d->currentFrame << "of" << d->frameCount
                     << "statuses" << d->currentPix->status() << d->pendingPix->status()
                     << "waiting?" << connectSuccess;
    if (statusChange)
        emit statusChanged(d->status);
}

void DeviceLoadingImage::onRequestFinished()
{
    auto *d = static_cast<QQuickImagePrivate *>(QQuickImagePrivate::get(this));
    qCDebug(lcTests) << "statuses" << d->currentPix->status() << d->pendingPix->status();
    ++requestsFinished;
}
