/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGPAINTERNODE_P_H
#define QSGPAINTERNODE_P_H

#include "qsgnode.h"
#include "qsgtexturematerial.h"
#include "qsgtexture_p.h"
#include "qsgpainteditem.h"

#include <QtGui/qcolor.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QOpenGLFramebufferObject;
class QOpenGLPaintDevice;

class Q_DECLARATIVE_EXPORT QSGPainterTexture : public QSGPlainTexture
{
public:
    QSGPainterTexture();

    void setDirtyRect(const QRect &rect) { m_dirty_rect = rect; }

    void bind();

private:
    QRect m_dirty_rect;
};

class Q_DECLARATIVE_EXPORT QSGPainterNode : public QSGGeometryNode
{
public:
    QSGPainterNode(QSGPaintedItem *item);
    virtual ~QSGPainterNode();

    void setPreferredRenderTarget(QSGPaintedItem::RenderTarget target);

    void setSize(const QSize &size);
    QSize size() const { return m_size; }

    void setDirty(bool d, const QRect &dirtyRect = QRect());

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

    void paint();

private:
    void updateTexture();
    void updateGeometry();
    void updateRenderTarget();
    void updateFBOSize();

    QSGContext *m_context;

    QSGPaintedItem::RenderTarget m_preferredRenderTarget;
    QSGPaintedItem::RenderTarget m_actualRenderTarget;

    QSGPaintedItem *m_item;

    QOpenGLFramebufferObject *m_fbo;
    QOpenGLFramebufferObject *m_multisampledFbo;
    QImage m_image;

    QSGOpaqueTextureMaterial m_material;
    QSGTextureMaterial m_materialO;
    QSGGeometry m_geometry;
    QSGPainterTexture *m_texture;
    QOpenGLPaintDevice *m_gl_device;

    QSize m_size;
    QSize m_fboSize;
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

    bool m_dirtyGeometry;
    bool m_dirtyRenderTarget;
    bool m_dirtyTexture;
};

QT_END_HEADER

QT_END_NAMESPACE

#endif // QSGPAINTERNODE_P_H
