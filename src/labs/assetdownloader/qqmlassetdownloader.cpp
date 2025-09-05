// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqmlassetdownloader_p.h"
#include <QtQml/qqmlcontext.h>

QT_BEGIN_NAMESPACE

using Assets::Downloader::AssetDownloader;
using Assets::Downloader::AssetDownloaderHelper;
using Assets::Downloader::QQmlAssetDownloader;

/*!
   \qmlmodule Qt.labs.assetdownloader
   \title Qt Labs Asset Downloader QML Types
   \brief Provides the AssetDownloader type to download external assets for Qt Examples.

    \note This module is not part of the public Qt API and may change without notice.
    It is intended for internal use or tightly controlled environments.
*/

/*!
    \qmltype AssetDownloader
//! \nativetype QQmlAssetDownloader
    \inqmlmodule Qt.labs.assetdownloader
    \brief Downloads assets asynchronously for use in QML Examples.
    \since 6.8

    The AssetDownloader type provides a convenient way to download external assets
    such as images, models, or data files from remote URLs and make them available to
    QML applications.

    \note This type is not part of the public Qt API and may change without notice.
    It is intended for internal use or tightly controlled environments.

    \section1 QML Usage

    To use this type in QML, import the module and instantiate the downloader:

    \qml
    import Qt.labs.assetdownloader

    AssetDownloader {
        downloadBase: "https://example.com/assets/"
        preferredLocalDownloadDir: "file:///home/user/assets/"
        jsonFileName: "data.json"
        zipFileName: "archive.zip"
        onFinished: (success) => {
            if (success)
                console.log("Download completed successfully");
            else
                console.log("Download failed");
        }
    }
    \endqml
*/

/*!
    \qmlproperty url AssetDownloader::downloadBase

    The base URL from which assets will be downloaded.
*/

/*!
    \qmlproperty url AssetDownloader::preferredLocalDownloadDir

    The preferred local directory where downloaded assets should be stored.
*/

/*!
    \qmlproperty url AssetDownloader::offlineAssetsFilePath

    The file path to offline assets, used when network access is unavailable.
*/

/*!
    \qmlproperty string AssetDownloader::jsonFileName

    The name of the asset JSON file to be downloaded. This file should contain a list
    of assets to be downloaded.

    Example format:

    \code
    {
        "url": "<base URL for asset downloads>",
        "assets": [
            "<relative path to asset file>",
            ...
        ]
    }
    \endcode
*/

/*!
    \qmlproperty string AssetDownloader::zipFileName

    The name of the ZIP file to be downloaded.
*/

/*!
    \qmlproperty url AssetDownloader::localDownloadDir

    The actual local directory where assets are stored after download.
*/

/*!
    \qmlsignal AssetDownloader::started()

    Emitted when the download process starts.
*/

/*!
    \qmlsignal AssetDownloader::finished(bool success)

    Emitted when the download process finishes. The \a success parameter indicates
    whether the download was successful.

    \sa AssetDownloader::networkErrors, AssetDownloader::sslErrors
*/

/*!
    \qmlsignal AssetDownloader::progressChanged(int progressValue, int progressMaximum, string progressText)

    Emitted to indicate progress during the download. \a progressValue is the current
    progress, \a progressMaximum is the total expected progress, and \a progressText
    provides a textual description.
*/

/*!
    \qmlmethod void AssetDownloader::start()

    Starts the download process.
*/

/*!
    \qmlmethod stringlist AssetDownloader::networkErrors()

    Returns a list of network-related errors encountered during the download.
*/

/*!
    \qmlmethod stringlist AssetDownloader::sslErrors()

    Returns a list of SSL-related errors encountered during the download.
*/

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
