/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#include "qsgsoftwarepixmaptexture_p.h"
#include <private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

QSGSoftwarePixmapTexture::QSGSoftwarePixmapTexture(const QImage &image, uint flags)
    : QSGTexture(*(new QSGSoftwarePixmapTexturePrivate))
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
    : QSGTexture(*(new QSGSoftwarePixmapTexturePrivate)),
      m_pixmap(pixmap)
{
}

int QSGSoftwarePixmapTexture::textureId() const
{
    return 0;
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

void QSGSoftwarePixmapTexture::bind()
{
    Q_UNREACHABLE();
}

int QSGSoftwarePixmapTexturePrivate::comparisonKey() const
{
    return 0;
}

QT_END_NAMESPACE

#include "moc_qsgsoftwarepixmaptexture_p.cpp"
