/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSGPAINTERNODE_P_H
#define QSGPAINTERNODE_P_H

#include <QtQuick/qsgnode.h>
#include "qsgtexturematerial.h"
#include "qsgtexture_p.h"

#include <QtQuick/qquickpainteditem.h>

#include <QtGui/qcolor.h>

QT_BEGIN_NAMESPACE

class QOpenGLFramebufferObject;
class QOpenGLPaintDevice;

class Q_QUICK_PRIVATE_EXPORT QSGPainterTexture : public QSGPlainTexture
{
public:
    QSGPainterTexture();

    void setDirtyRect(const QRect &rect) { m_dirty_rect = rect; }

    void bind();

private:
    QRect m_dirty_rect;
};

class Q_QUICK_PRIVATE_EXPORT QSGPainterNode : public QSGGeometryNode
{
public:
    QSGPainterNode(QQuickPaintedItem *item);
    virtual ~QSGPainterNode();

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

    void paint();

private:
    void updateTexture();
    void updateGeometry();
    void updateRenderTarget();
    void updateFBOSize();

    QSGRenderContext *m_context;

    QQuickPaintedItem::RenderTarget m_preferredRenderTarget;
    QQuickPaintedItem::RenderTarget m_actualRenderTarget;

    QQuickPaintedItem *m_item;

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

QT_END_NAMESPACE

#endif // QSGPAINTERNODE_P_H
