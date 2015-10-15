/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of Qt Quick 2d Renderer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PAINTERNODE_H
#define PAINTERNODE_H

#include <private/qsgadaptationlayer_p.h>
#include <QtQuick/qquickpainteditem.h>

#include <QtGui/QPixmap>

class PainterNode : public QSGPainterNode
{
public:
    PainterNode(QQuickPaintedItem *item);
    ~PainterNode();

    void setPreferredRenderTarget(QQuickPaintedItem::RenderTarget target);

    void setSize(const QSize &size);
    QSize size() const { return m_size; }

    void setDirty(const QRect &dirtyRect = QRect());

    void setOpaquePainting(bool opaque);
    bool opaquePainting() const { return m_opaquePainting; }

    void setLinearFiltering(bool linearFiltering);
    bool linearFiltering() const { return m_linear_filtering; }

    void setMipmapping(bool mipmapping);
    bool mipmapping() const { return m_mipmapping; }

    void setSmoothPainting(bool s);
    bool smoothPainting() const { return m_smoothPainting; }

    void setFillColor(const QColor &c);
    QColor fillColor() const { return m_fillColor; }

    void setContentsScale(qreal s);
    qreal contentsScale() const { return m_contentsScale; }

    void setFastFBOResizing(bool dynamic);
    bool fastFBOResizing() const { return m_fastFBOResizing; }

    QImage toImage() const;
    void update();
    QSGTexture *texture() const { return m_texture; }

    void paint(QPainter *painter);

    void paint();

    void setTextureSize(const QSize &size);
    QSize textureSize() const { return m_textureSize; }

private:

    QQuickPaintedItem::RenderTarget m_preferredRenderTarget;
    QQuickPaintedItem::RenderTarget m_actualRenderTarget;

    QQuickPaintedItem *m_item;

    QPixmap m_pixmap;
    QSGTexture *m_texture;

    QSize m_size;
    bool m_dirtyContents;
    QRect m_dirtyRect;
    bool m_opaquePainting;
    bool m_linear_filtering;
    bool m_mipmapping;
    bool m_smoothPainting;
    bool m_extensionsChecked;
    bool m_multisamplingSupported;
    bool m_fastFBOResizing;
    QColor m_fillColor;
    qreal m_contentsScale;
    QSize m_textureSize;

    bool m_dirtyGeometry;
};

#endif // PAINTERNODE_H
