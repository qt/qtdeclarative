// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgsoftwarepixmaptexture_p.h"
#include <private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

QSGSoftwarePixmapTexture::QSGSoftwarePixmapTexture(const QImage &image, uint flags)
{
    // Prevent pixmap format conversion to reduce memory consumption
    // and surprises in calling code. (See QTBUG-47328)
    if (flags & QSGRenderContext::CreateTexture_Alpha) {
        //If texture should have an alpha
        m_pixmap = QPixmap::fromImage(image, Qt::NoFormatConversion);
    } else {
        //Force opaque texture
        m_pixmap = QPixmap::fromImage(image.convertToFormat(QImage::Format_RGB32), Qt::NoFormatConversion);
    }
}

QSGSoftwarePixmapTexture::QSGSoftwarePixmapTexture(const QPixmap &pixmap)
    : m_pixmap(pixmap)
{
}

QSize QSGSoftwarePixmapTexture::textureSize() const
{
    return m_pixmap.size();
}

bool QSGSoftwarePixmapTexture::hasAlphaChannel() const
{
    return m_pixmap.hasAlphaChannel();
}

bool QSGSoftwarePixmapTexture::hasMipmaps() const
{
    return false;
}

qint64 QSGSoftwarePixmapTexture::comparisonKey() const
{
    return 0;
}

QT_END_NAMESPACE

#include "moc_qsgsoftwarepixmaptexture_p.cpp"
