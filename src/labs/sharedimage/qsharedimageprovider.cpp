// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qsharedimageprovider_p.h>
#include <private/qquickpixmapcache_p.h>
#include <private/qimage_p.h>
#include <QImageReader>
#include <QFileInfo>
#include <QDir>

Q_DECLARE_METATYPE(QQuickImageProviderOptions)

QT_BEGIN_NAMESPACE

QuickSharedImageLoader::QuickSharedImageLoader(QObject *parent) : QSharedImageLoader(parent) {}

QImage QuickSharedImageLoader::loadFile(const QString &path, ImageParameters *params)
{
    QImageReader imgio(path);
    QSize realSize = imgio.size();
    QSize requestSize;
    QQuickImageProviderOptions options;
    if (params) {
        requestSize = params->value(RequestedSize).toSize();
        options = params->value(ProviderOptions).value<QQuickImageProviderOptions>();
    }

    QSize scSize = QQuickImageProviderWithOptions::loadSize(imgio.size(), requestSize, imgio.format(), options);

    if (scSize.isValid())
        imgio.setScaledSize(scSize);

    QImage image;
    if (imgio.read(&image)) {
        if (realSize.isEmpty())
            realSize = image.size();
        // Make sure we have acceptable format for texture uploader, or it will convert & lose sharing
        // This mimics the testing & conversion normally done by the quick pixmapcache & texturefactory
        if (image.format() != QImage::Format_RGB32 && image.format() != QImage::Format_ARGB32_Premultiplied) {
            QImage::Format newFmt = QImage::Format_RGB32;
            if (image.hasAlphaChannel() && image.data_ptr()->checkForAlphaPixels())
                newFmt = QImage::Format_ARGB32_Premultiplied;
            qCDebug(lcSharedImage) << "Convert on load from format" << image.format() << "to" << newFmt;
            image = image.convertToFormat(newFmt);
        }
    }

    if (params && params->size() > OriginalSize)
        params->replace(OriginalSize, realSize);

    return image;
}

QString QuickSharedImageLoader::key(const QString &path, ImageParameters *params)
{
    QSize reqSz;
    QQuickImageProviderOptions opts;
    if (params) {
        reqSz = params->value(RequestedSize).toSize();
        opts = params->value(ProviderOptions).value<QQuickImageProviderOptions>();
    }
    if (!reqSz.isValid())
        return path;
    int aspect = opts.preserveAspectRatioCrop() || opts.preserveAspectRatioFit() ? 1 : 0;

    QString key = path + QStringLiteral("_%1x%2_%3").arg(reqSz.width()).arg(reqSz.height()).arg(aspect);
    qCDebug(lcSharedImage) << "KEY:" << key;
    return key;
}

SharedImageProvider::SharedImageProvider()
    : QQuickImageProviderWithOptions(QQuickImageProvider::Image), loader(new QuickSharedImageLoader)
{
}

QImage SharedImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize, const QQuickImageProviderOptions &options)
{
    QFileInfo fi(QDir::root(), id);
    QString path = fi.canonicalFilePath();
    if (path.isEmpty())
        return QImage();

    QSharedImageLoader::ImageParameters params(QuickSharedImageLoader::NumImageParameters);
    params[QuickSharedImageLoader::RequestedSize].setValue(requestedSize);
    params[QuickSharedImageLoader::ProviderOptions].setValue(options);

    QImage img = loader->load(path, &params);
    if (img.isNull()) {
        // May be sharing problem, fall back to normal local load
        img = loader->loadFile(path, &params);
        if (!img.isNull())
            qCWarning(lcSharedImage) << "Sharing problem; loading" << id << "unshared";
    }

    //... QSize realSize = params.value(QSharedImageLoader::OriginalSize).toSize();
    // quickpixmapcache's readImage() reports back the original size, prior to requestedSize scaling, in the *size
    // parameter. That value is currently ignored by quick however, which only cares about the present size of the
    // returned image. So handling and sharing of info on pre-scaled size is currently not implemented.
    if (size) {
        *size = img.size();
    }

    return img;
}

QT_END_NAMESPACE

#include "moc_qsharedimageprovider_p.cpp"
