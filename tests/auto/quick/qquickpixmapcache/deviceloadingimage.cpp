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
    d->pix.loadImageFromDevice(qmlEngine(this), device.get(), context->resolvedUrl(d->url),
                               d->sourceClipRect.toRect(), d->sourcesize * d->devicePixelRatio,
                               QQuickImageProviderOptions(), d->currentFrame, d->frameCount);

    qCDebug(lcTests) << "loading page" << d->currentFrame << "of" << d->frameCount << "status" << d->pix.status();

    switch (d->pix.status()) {
    case QQuickPixmap::Ready:
        pixmapChange();
        break;
    case QQuickPixmap::Loading:
        d->pix.connectFinished(this, thisRequestFinished);
        if (d->status != Loading) {
            d->status = Loading;
            emit statusChanged(d->status);
        }
        break;
    default:
        qCWarning(lcTests) << "unexpected status" << d->pix.status();
        break;
    }
}
