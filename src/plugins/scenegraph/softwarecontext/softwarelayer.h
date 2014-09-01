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
#ifndef SOFTWARELAYER_H
#define SOFTWARELAYER_H

#include <private/qsgadaptationlayer_p.h>
#include <private/qsgcontext_p.h>

namespace SoftwareContext {
class PixmapRenderer;
}

class SoftwareLayer : public QSGLayer
{
    Q_OBJECT
public:
    SoftwareLayer(QSGRenderContext *renderContext);
    ~SoftwareLayer();

    const QPixmap &pixmap() const { return m_pixmap; }

    // QSGTexture interface
public:
    virtual int textureId() const;
    virtual QSize textureSize() const;
    virtual bool hasAlphaChannel() const;
    virtual bool hasMipmaps() const;
    virtual void bind();

    // QSGDynamicTexture interface
public:
    virtual bool updateTexture();

    // QSGLayer interface
public:
    virtual void setItem(QSGNode *item);
    virtual void setShaderSourceNode(QSGNode *node);
    virtual void setRect(const QRectF &rect);
    virtual void setSize(const QSize &size);
    virtual void scheduleUpdate();
    virtual QImage toImage() const;
    virtual void setLive(bool live);
    virtual void setRecursive(bool recursive);
    virtual void setFormat(GLenum);
    virtual void setHasMipmaps(bool);
    virtual void setDevicePixelRatio(qreal ratio);

public slots:
    virtual void markDirtyTexture();
    virtual void invalidated();

private:
    void grab();

    QSGNode *m_item;
    QSGNode *m_shaderSourceNode;
    QSGRenderContext *m_context;
    SoftwareContext::PixmapRenderer *m_renderer;
    QRectF m_rect;
    QSize m_size;
    QPixmap m_pixmap;
    qreal m_device_pixel_ratio;
    bool m_live;
    bool m_grab;
    bool m_recursive;
    bool m_dirtyTexture;
};

#endif // SOFTWARELAYER_H
