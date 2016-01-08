/******************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick 2d Renderer module.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

#include "pixmaptexture.h"

PixmapTexture::PixmapTexture(const QImage &image)
    // Prevent pixmap format conversion to reduce memory consumption
    // and surprises in calling code. (See QTBUG-47328)
    : m_pixmap(QPixmap::fromImage(image, Qt::NoFormatConversion))
{
}

PixmapTexture::PixmapTexture(const QPixmap &pixmap)
    : m_pixmap(pixmap)
{
}


int PixmapTexture::textureId() const
{
    return 0;
}

QSize PixmapTexture::textureSize() const
{
    return m_pixmap.size();
}

bool PixmapTexture::hasAlphaChannel() const
{
    return m_pixmap.hasAlphaChannel();
}

bool PixmapTexture::hasMipmaps() const
{
    return false;
}

void PixmapTexture::bind()
{
    Q_UNREACHABLE();
}
