/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt SceneGraph Raster Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/
#ifndef PIXMAPTEXTURE_H
#define PIXMAPTEXTURE_H

#include <private/qsgtexture_p.h>

class PixmapTexture : public QSGTexture
{
    Q_OBJECT
public:
    PixmapTexture(const QImage &image);

    virtual int textureId() const;
    virtual QSize textureSize() const;
    virtual bool hasAlphaChannel() const;
    virtual bool hasMipmaps() const;
    virtual void bind();

    const QPixmap &pixmap() const { return m_pixmap; }

private:
    QPixmap m_pixmap;
};

#endif // PIXMAPTEXTURE_H
