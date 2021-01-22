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

#ifndef QSGSOFTWARELAYER_H
#define QSGSOFTWARELAYER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qsgadaptationlayer_p.h>
#include <private/qsgcontext_p.h>
#include <private/qsgtexture_p.h>

QT_BEGIN_NAMESPACE

class QSGSoftwarePixmapRenderer;
class QSGSoftwareLayerPrivate;

class QSGSoftwareLayer : public QSGLayer
{
    Q_DECLARE_PRIVATE(QSGSoftwareLayer)
    Q_OBJECT
public:
    QSGSoftwareLayer(QSGRenderContext *renderContext);
    ~QSGSoftwareLayer();

    const QPixmap &pixmap() const { return m_pixmap; }

    // QSGTexture interface
public:
    int textureId() const override;
    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;
    void bind() override;

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
    void setFormat(uint) override;
    void setHasMipmaps(bool) override;
    void setDevicePixelRatio(qreal ratio) override;
    void setMirrorHorizontal(bool mirror) override;
    void setMirrorVertical(bool mirror) override;
    void setSamples(int) override { }

public slots:
    void markDirtyTexture() override;
    void invalidated() override;

private:
    void grab();

    QSGNode *m_item;
    QSGRenderContext *m_context;
    QSGSoftwarePixmapRenderer *m_renderer;
    QRectF m_rect;
    QSize m_size;
    QPixmap m_pixmap;
    qreal m_device_pixel_ratio;
    bool m_mirrorHorizontal;
    bool m_mirrorVertical;
    bool m_live;
    bool m_grab;
    bool m_recursive;
    bool m_dirtyTexture;
};

class QSGSoftwareLayerPrivate : public QSGTexturePrivate
{
    Q_DECLARE_PUBLIC(QSGSoftwareLayer)
public:
    int comparisonKey() const override;
};

QT_END_NAMESPACE

#endif // QSGSOFTWARELAYER_H
