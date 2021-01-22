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

#ifndef QSGOPENVGPAINTERNODE_H
#define QSGOPENVGPAINTERNODE_H

#include <private/qsgadaptationlayer_p.h>
#include <QtQuick/QQuickPaintedItem>
#include "qsgopenvgrenderable.h"

QT_BEGIN_NAMESPACE

class QSGOpenVGTexture;

class QSGOpenVGPainterNode : public QSGPainterNode, public QSGOpenVGRenderable
{
public:
    QSGOpenVGPainterNode(QQuickPaintedItem *item);
    ~QSGOpenVGPainterNode();

    void setPreferredRenderTarget(QQuickPaintedItem::RenderTarget target) override;
    void setSize(const QSize &size) override;
    void setDirty(const QRect &dirtyRect) override;
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

    void render() override;
    void paint();

private:
    QQuickPaintedItem::RenderTarget m_preferredRenderTarget;

    QQuickPaintedItem *m_item;
    QSGOpenVGTexture *m_texture;
    QImage m_image;

    QSize m_size;
    bool m_dirtyContents;
    QRect m_dirtyRect;
    bool m_opaquePainting;
    bool m_linear_filtering;
    bool m_smoothPainting;
    QColor m_fillColor;
    qreal m_contentsScale;
    QSize m_textureSize;

    bool m_dirtyGeometry;
};

QT_END_NAMESPACE

#endif // QSGOPENVGPAINTERNODE_H
