/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <sharedimageprovider.h>
#include <qsharedimageloader_p.h>
#include <qquickimageprovider.h>
#include <private/qimage_p.h>
#include <QImageReader>
#include <QFileInfo>
#include <QDir>

class QuickSharedImageLoader : public QSharedImageLoader
{
    Q_OBJECT
    friend class SharedImageProvider;

public:
    QuickSharedImageLoader(QObject *parent = Q_NULLPTR)
        : QSharedImageLoader(parent)
    {
    }

protected:
    QImage loadFile(const QString &path, ImageParameters *params) override
    {
        QImageReader imgio(path);
        QSize realSize = imgio.size();
        QSize requestSize = params ? params->value(RequestedSize).toSize() : QSize();

        // Following qquickpixmapcache's readImage, from here...
        const bool force_scale = imgio.format() == "svg" || imgio.format() == "svgz";

        if (requestSize.width() > 0 || requestSize.height() > 0) {
            QSize s = realSize;
            qreal ratio = 0.0;
            if (requestSize.width() && (force_scale || requestSize.width() < s.width())) {
                ratio = qreal(requestSize.width())/s.width();
            }
            if (requestSize.height() && (force_scale || requestSize.height() < s.height())) {
                qreal hr = qreal(requestSize.height())/s.height();
                if (ratio == 0.0 || hr < ratio)
                    ratio = hr;
            }
            if (ratio > 0.0) {
                s.setHeight(qRound(s.height() * ratio));
                s.setWidth(qRound(s.width() * ratio));
                imgio.setScaledSize(s);
            }
        }
        // ... to here

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

        if (params && params->count() > OriginalSize)
            params->replace(OriginalSize, realSize);

        return image;
    }

    QString key(const QString &path, ImageParameters *params) override
    {
        QSize reqSz = params->value(RequestedSize).toSize();
        if (!reqSz.isValid())
            return path;

        QString key = path + QStringLiteral("_%1x%2").arg(reqSz.width()).arg(reqSz.height());
        qCDebug(lcSharedImage) << "KEY:" << key;
        return key;
    }
};


SharedImageProvider::SharedImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image), loader(new QuickSharedImageLoader)
{
}

QImage SharedImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QFileInfo fi(QDir::root(), id);
    QString path = fi.canonicalFilePath();
    if (path.isEmpty())
        return QImage();

    QSharedImageLoader::ImageParameters params(QSharedImageLoader::NumImageParameters);
    params[QSharedImageLoader::RequestedSize].setValue(requestedSize);

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

#include "sharedimageprovider.moc"
