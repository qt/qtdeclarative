// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSGOPENVGLAYER_H
#define QSGOPENVGLAYER_H

#include <private/qsgadaptationlayer_p.h>
#include <private/qsgcontext_p.h>
#include <private/qsgtexture_p.h>

#include "qopenvgcontext_p.h"
#include "qopenvgoffscreensurface.h"

QT_BEGIN_NAMESPACE

class QSGOpenVGRenderer;
class QSGOpenVGRenderContext;
class QSGOpenVGLayerPrivate;

class QSGOpenVGLayer : public QSGLayer
{
    Q_DECLARE_PRIVATE(QSGOpenVGLayer)
public:
    QSGOpenVGLayer(QSGRenderContext *renderContext);
    ~QSGOpenVGLayer();

    // QSGTexture interface
public:
    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;
    qint64 comparisonKey() const override;

    // QSGDynamicTexture interface
public:
    bool updateTexture() override;

    // QSGLayer interface
public:
    void setItem(QSGNode *item) override;
    void setRect(const QRectF &rect) override;
    void setSize(const QSize &size) override;
    void scheduleUpdate() override;
    QImage toImage() const override;
    void setLive(bool live) override;
    void setRecursive(bool recursive) override;
    void setFormat(uint format) override;
    void setHasMipmaps(bool mipmap) override;
    void setDevicePixelRatio(qreal ratio) override;
    void setMirrorHorizontal(bool mirror) override;
    void setMirrorVertical(bool mirror) override;
    void setSamples(int) override { }

public Q_SLOTS:
    void markDirtyTexture() override;
    void invalidated() override;

private:
    void grab();

    QSGNode *m_item;
    QSGOpenVGRenderContext *m_context;
    QSGOpenVGRenderer *m_renderer;
    QRectF m_rect;
    QSize m_size;
    qreal m_device_pixel_ratio;
    bool m_mirrorHorizontal;
    bool m_mirrorVertical;
    bool m_live;
    bool m_grab;
    bool m_recursive;
    bool m_dirtyTexture;

    QOpenVGOffscreenSurface *m_offscreenSurface;
    QOpenVGOffscreenSurface *m_secondaryOffscreenSurface;
};

QT_END_NAMESPACE

#endif // QSGOPENVGLAYER_H
