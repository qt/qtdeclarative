// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlassetdownloader_p.h"
#include <QtQml/qqmlcontext.h>

QT_BEGIN_NAMESPACE

using Assets::Downloader::AssetDownloader;
using Assets::Downloader::AssetDownloaderHelper;
using Assets::Downloader::QQmlAssetDownloader;

AssetDownloaderHelper::AssetDownloaderHelper(QObject *parent)
    : AssetDownloader(parent)
{}

QUrl AssetDownloaderHelper::resolvedUrl(const QUrl &url) const
{
    const QQmlContext *context = qmlContext(this);
    return context ? context->resolvedUrl(url) : url;
}

AssetDownloaderHelper *QQmlAssetDownloader::create(QQmlEngine *, QJSEngine *)
{
    return new AssetDownloaderHelper();
}

QT_END_NAMESPACE
