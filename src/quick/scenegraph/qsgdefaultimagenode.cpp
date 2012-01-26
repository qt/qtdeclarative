/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "qsgdefaultimagenode_p.h"

#include <QtQuick/qsgtextureprovider.h>

#include <QtCore/qvarlengtharray.h>
#include <QtCore/qmath.h>
#include <QtGui/qopenglfunctions.h>

QT_BEGIN_NAMESPACE

QSGDefaultImageNode::QSGDefaultImageNode()
    : m_sourceRect(0, 0, 1, 1)
    , m_dirtyGeometry(false)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
{
    setMaterial(&m_materialO);
    setOpaqueMaterial(&m_material);
    setGeometry(&m_geometry);

#ifdef QML_RUNTIME_TESTING
    description = QLatin1String("image");
#endif
}

void QSGDefaultImageNode::setTargetRect(const QRectF &rect)
{
    if (rect == m_targetRect)
        return;
    m_targetRect = rect;
    m_dirtyGeometry = true;
}

void QSGDefaultImageNode::setSourceRect(const QRectF &rect)
{
    if (rect == m_sourceRect)
        return;
    m_sourceRect = rect;
    m_dirtyGeometry = true;
}


void QSGDefaultImageNode::setFiltering(QSGTexture::Filtering filtering)
{
    if (m_material.filtering() == filtering)
        return;

    m_material.setFiltering(filtering);
    m_materialO.setFiltering(filtering);
    markDirty(DirtyMaterial);
}


void QSGDefaultImageNode::setMipmapFiltering(QSGTexture::Filtering filtering)
{
    if (m_material.mipmapFiltering() == filtering)
        return;

    m_material.setMipmapFiltering(filtering);
    m_materialO.setMipmapFiltering(filtering);
    markDirty(DirtyMaterial);
}

void QSGDefaultImageNode::setVerticalWrapMode(QSGTexture::WrapMode wrapMode)
{
    if (m_material.verticalWrapMode() == wrapMode)
        return;

    m_material.setVerticalWrapMode(wrapMode);
    m_materialO.setVerticalWrapMode(wrapMode);
    markDirty(DirtyMaterial);
}

void QSGDefaultImageNode::setHorizontalWrapMode(QSGTexture::WrapMode wrapMode)
{
    if (m_material.horizontalWrapMode() == wrapMode)
        return;

    m_material.setHorizontalWrapMode(wrapMode);
    m_materialO.setHorizontalWrapMode(wrapMode);
    markDirty(DirtyMaterial);
}


void QSGDefaultImageNode::setTexture(QSGTexture *texture)
{
    if (texture == m_material.texture())
        return;

    m_material.setTexture(texture);
    m_materialO.setTexture(texture);
    // Texture cleanup
//    if (!texture.isNull())
//        m_material.setBlending(texture->hasAlphaChannel());
    markDirty(DirtyMaterial);

    // Because the texture can be a different part of the atlas, we need to update it...
    m_dirtyGeometry = true;
}

void QSGDefaultImageNode::update()
{
    if (m_dirtyGeometry)
        updateGeometry();
}

void QSGDefaultImageNode::preprocess()
{
    bool doDirty = false;
    QSGDynamicTexture *t = qobject_cast<QSGDynamicTexture *>(m_material.texture());
    if (t) {
        doDirty = t->updateTexture();
        updateGeometry();
    }
// ### texture cleanup
//    bool alpha = m_material.blending();
//    if (!m_material->texture().isNull() && alpha != m_material.texture()->hasAlphaChannel()) {
//        m_material.setBlending(!alpha);
//        doDirty = true;
//    }

    if (doDirty)
        markDirty(DirtyMaterial);
}

inline static bool isPowerOfTwo(int x)
{
    // Assumption: x >= 1
    return x == (x & -x);
}

namespace {
    struct X { float x, tx; };
    struct Y { float y, ty; };
}

void QSGDefaultImageNode::updateGeometry()
{
    const QSGTexture *t = m_material.texture();
    if (!t) {
        m_geometry.allocate(4);
        m_geometry.setDrawingMode(GL_TRIANGLE_STRIP);
        QSGGeometry::updateTexturedRectGeometry(&m_geometry, QRectF(), QRectF());
    } else {
        QRectF textureRect = t->normalizedTextureSubRect();

        bool isSubRect = textureRect != QRectF(0, 0, 1, 1);
        const int ceilRight = qCeil(m_sourceRect.right());
        const int floorLeft = qFloor(m_sourceRect.left());
        const int ceilBottom = qCeil(m_sourceRect.bottom());
        const int floorTop = qFloor(m_sourceRect.top());
        const int hCells = ceilRight - floorLeft;
        const int vCells = ceilBottom - floorTop;
        bool isRepeating = hCells > 1 || vCells > 1;

#ifdef QT_OPENGL_ES_2
        QOpenGLContext *ctx = QOpenGLContext::currentContext();
        bool npotSupported = ctx->functions()->hasOpenGLFeature(QOpenGLFunctions::NPOTTextures);

        QSize size = t->textureSize();
        bool isNpot = !isPowerOfTwo(size.width()) || !isPowerOfTwo(size.height());

        if (isRepeating && (isSubRect || (isNpot && !npotSupported))) {
#else
        if (isRepeating && isSubRect) {
#endif
            m_geometry.allocate(hCells * vCells * 4, hCells * vCells * 6);
            m_geometry.setDrawingMode(GL_TRIANGLES);
            QVarLengthArray<X, 32> xData(2 * hCells);
            QVarLengthArray<Y, 32> yData(2 * vCells);
            X *xs = xData.data();
            Y *ys = yData.data();
            
            xs->x = m_targetRect.left();
            xs->tx = textureRect.x() + (m_sourceRect.left() - floorLeft) * textureRect.width();
            ++xs;
            ys->y = m_targetRect.top();
            ys->ty = textureRect.y() + (m_sourceRect.top() - floorTop) * textureRect.height();
            ++ys;

            float a, b;
            b = m_targetRect.width() / m_sourceRect.width();
            a = m_targetRect.x() - m_sourceRect.x() * b;

            float tex_x1 = textureRect.x();
            float tex_x2 = textureRect.right();
            float tex_y1 = textureRect.y();
            float tex_y2 = textureRect.bottom();
            for (int i = floorLeft + 1; i <= ceilRight - 1; ++i) {
                xs[0].x = xs[1].x = a + b * i;
                xs[0].tx = tex_x2;
                xs[1].tx = tex_x1;
                xs += 2;
            }
            b = m_targetRect.height() / m_sourceRect.height();
            a = m_targetRect.y() - m_sourceRect.y() * b;
            for (int i = floorTop + 1; i <= ceilBottom - 1; ++i) {
                ys[0].y = ys[1].y = a + b * i;
                ys[0].ty = tex_y2;
                ys[1].ty = tex_y1;
                ys += 2;
            }

            xs->x = m_targetRect.right();
            xs->tx = textureRect.x() + (m_sourceRect.right() - ceilRight + 1) * textureRect.width();

            ys->y = m_targetRect.bottom();
            ys->ty = textureRect.y() + (m_sourceRect.bottom() - ceilBottom + 1) * textureRect.height();

            QSGGeometry::TexturedPoint2D *vertices = m_geometry.vertexDataAsTexturedPoint2D();
            ys = yData.data();
            for (int j = 0; j < vCells; ++j, ys += 2) {
                xs = xData.data();
                for (int i = 0; i < hCells; ++i, xs += 2) {
                    vertices[0].x = vertices[2].x = xs[0].x;
                    vertices[0].tx = vertices[2].tx = xs[0].tx;
                    vertices[1].x = vertices[3].x = xs[1].x;
                    vertices[1].tx = vertices[3].tx = xs[1].tx;

                    vertices[0].y = vertices[1].y = ys[0].y;
                    vertices[0].ty = vertices[1].ty = ys[0].ty;
                    vertices[2].y = vertices[3].y = ys[1].y;
                    vertices[2].ty = vertices[3].ty = ys[1].ty;

                    vertices += 4;
                }
            }

            quint16 *indices = m_geometry.indexDataAsUShort();
            for (int i = 0; i < 4 * vCells * hCells; i += 4) {
                *indices++ = i;
                *indices++ = i + 2;
                *indices++ = i + 3;
                *indices++ = i + 3;
                *indices++ = i + 1;
                *indices++ = i;
            }
        } else {
            QRectF sr(textureRect.x() + m_sourceRect.x() * textureRect.width(),
                      textureRect.y() + m_sourceRect.y() * textureRect.height(),
                      m_sourceRect.width() * textureRect.width(),
                      m_sourceRect.height() * textureRect.height());

            m_geometry.allocate(4);
            m_geometry.setDrawingMode(GL_TRIANGLE_STRIP);
            QSGGeometry::updateTexturedRectGeometry(&m_geometry, m_targetRect, sr);
        }
    }
    markDirty(DirtyGeometry);
    m_dirtyGeometry = false;
}

QT_END_NAMESPACE
