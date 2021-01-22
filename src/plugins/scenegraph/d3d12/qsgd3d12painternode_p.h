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

#ifndef QSGD3D12PAINTERNODE_P_H
#define QSGD3D12PAINTERNODE_P_H

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
#include "qsgd3d12texture_p.h"
#include "qsgd3d12builtinmaterials_p.h"

QT_BEGIN_NAMESPACE

class QSGD3D12Engine;

class QSGD3D12PainterTexture : public QSGD3D12Texture
{
public:
    QSGD3D12PainterTexture(QSGD3D12Engine *engine);

    void bind() override;
    bool hasAlphaChannel() const override { return true; }

    QImage *image() { return &m_image; }

    QRect dirty;

private:
    QSize lastSize;
};

class QSGD3D12PainterNode : public QSGPainterNode
{
public:
    QSGD3D12PainterNode(QQuickPaintedItem *item);
    ~QSGD3D12PainterNode();

    void setPreferredRenderTarget(QQuickPaintedItem::RenderTarget target) override;
    void setSize(const QSize &size) override;
    void setDirty(const QRect &dirtyRect = QRect()) override;
    void setOpaquePainting(bool opaque) override;
    void setLinearFiltering(bool linearFiltering) override;
    void setMipmapping(bool mipmapping) override;
    void setSmoothPainting(bool s) override;
    void setFillColor(const QColor &c) override;
    void setContentsScale(qreal s) override;
    void setFastFBOResizing(bool dynamic) override;
    void setTextureSize(const QSize &size) override;

    QImage toImage() const override;
    void update() override;
    QSGTexture *texture() const override;

private:
    QQuickPaintedItem *m_item;
    QSGD3D12Engine *m_engine;
    QSGD3D12PainterTexture *m_texture;
    QSize m_size;
    QSize m_textureSize;
    float m_contentsScale = 1;
    bool m_smoothPainting = false;
    QColor m_fillColor = Qt::transparent;
    QRect m_dirtyRect;

    QSGGeometry m_geometry;
    QSGD3D12TextureMaterial m_material;

    uint m_dirtyGeometry : 1;
    uint m_dirtyContents : 1;
};

QT_END_NAMESPACE

#endif // QSGD3D12PAINTERNODE_P_H
