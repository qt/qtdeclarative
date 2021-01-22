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

#ifndef QSGD3D12LAYER_P_H
#define QSGD3D12LAYER_P_H

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
#include <private/qsgtexture_p.h>

QT_BEGIN_NAMESPACE

class QSGD3D12RenderContext;
class QSGD3D12LayerPrivate;

class QSGD3D12Layer : public QSGLayer
{
    Q_DECLARE_PRIVATE(QSGD3D12Layer)
    Q_OBJECT

public:
    QSGD3D12Layer(QSGD3D12RenderContext *rc);
    ~QSGD3D12Layer();

    int textureId() const override;
    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;
    QRectF normalizedTextureSubRect() const override;
    void bind() override;

    bool updateTexture() override;

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
    void cleanup();
    void resetRenderTarget();
    void updateContent();

    QSGD3D12RenderContext *m_rc;
    uint m_rt = 0;
    uint m_secondaryRT = 0;
    QSize m_rtSize;
    QSize m_size;
    QRectF m_rect;
    QSGNode *m_item = nullptr;
    QSGRenderer *m_renderer = nullptr;
    float m_dpr = 1;
    bool m_mirrorHorizontal = false;
    bool m_mirrorVertical = true;
    bool m_live = true;
    bool m_recursive = false;
    bool m_dirtyTexture = true;
    bool m_updateContentPending = false;
};

class QSGD3D12LayerPrivate : public QSGTexturePrivate
{
    Q_DECLARE_PUBLIC(QSGD3D12Layer)
public:
    int comparisonKey() const override;
};

QT_END_NAMESPACE

#endif // QSGD3D12LAYER_P_H
