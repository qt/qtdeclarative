/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "customitem.h"

#include <QtCore/QPointer>

#include <QtQuick/QSGMaterial>
#include <QtQuick/QSGTexture>
#include <QtQuick/QSGGeometryNode>
#include <QtQuick/QSGTextureProvider>

//! [2]
class CustomShader : public QSGMaterialShader
{
public:
    CustomShader()
    {
        setShaderFileName(VertexStage, QLatin1String(":/scenegraph/custommaterial/shaders/mandelbrot.vert.qsb"));
        setShaderFileName(FragmentStage, QLatin1String(":/scenegraph/custommaterial/shaders/mandelbrot.frag.qsb"));
    }
    bool updateUniformData(RenderState &state,
                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};
//! [2]

//! [1]
class CustomMaterial : public QSGMaterial
{
public:
    CustomMaterial();
    QSGMaterialType *type() const override;
    int compare(const QSGMaterial *other) const override;

    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override
    {
        return new CustomShader;
    }

    struct {
        float center[2];
        float zoom;
        int limit;
        bool dirty;
    } uniforms;
};
//! [1]

CustomMaterial::CustomMaterial()
{
}

QSGMaterialType *CustomMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

int CustomMaterial::compare(const QSGMaterial *o) const
{
    Q_ASSERT(o && type() == o->type());
    const auto *other = static_cast<const CustomMaterial *>(o);
    return other == this ? 0 : 1; // ### TODO: compare state???
}

//! [3]
bool CustomShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 84);

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64, &opacity, 4);
        changed = true;
    }

    auto *customMaterial = static_cast<CustomMaterial *>(newMaterial);
    if (oldMaterial != newMaterial || customMaterial->uniforms.dirty) {
        memcpy(buf->data() + 68, &customMaterial->uniforms.zoom, 4);
        memcpy(buf->data() + 72, &customMaterial->uniforms.center, 8);
        memcpy(buf->data() + 80, &customMaterial->uniforms.limit, 4);
        customMaterial->uniforms.dirty = false;
        changed = true;
    }
    return changed;
}
//! [3]

//! [4]
class CustomNode : public QSGGeometryNode
{
public:
    CustomNode()
    {
        auto *m = new CustomMaterial;
        setMaterial(m);
        setFlag(OwnsMaterial, true);

        QSGGeometry *g = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
        QSGGeometry::updateTexturedRectGeometry(g, QRect(), QRect());
        setGeometry(g);
        setFlag(OwnsGeometry, true);
    }

    void setRect(const QRectF &bounds)
    {
        QSGGeometry::updateTexturedRectGeometry(geometry(), bounds, QRectF(0, 0, 1, 1));
        markDirty(QSGNode::DirtyGeometry);
    }

    void setZoom(qreal zoom)
    {
        auto *m = static_cast<CustomMaterial *>(material());
        m->uniforms.zoom = zoom;
        m->uniforms.dirty = true;
        markDirty(DirtyMaterial);
    }

    void setLimit(int limit)
    {
        auto *m = static_cast<CustomMaterial *>(material());
        m->uniforms.limit = limit;
        m->uniforms.dirty = true;
        markDirty(DirtyMaterial);
    }

    void setCenter(const QPointF &center)
    {
        auto *m = static_cast<CustomMaterial *>(material());
        m->uniforms.center[0] = center.x();
        m->uniforms.center[1] = center.y();
        m->uniforms.dirty = true;
        markDirty(DirtyMaterial);
    }
};
//! [4]

CustomItem::CustomItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
}

//! [5]
void CustomItem::setZoom(qreal zoom)
{
    if (qFuzzyCompare(m_zoom, zoom))
        return;

    m_zoom = zoom;
    m_zoomChanged = true;
    emit zoomChanged(m_zoom);
    update();
}

void CustomItem::setIterationLimit(int limit)
{
    if (m_limit == limit)
        return;

    m_limit = limit;
    m_limitChanged = true;
    emit iterationLimitChanged(m_limit);
    update();
}

void CustomItem::setCenter(QPointF center)
{
    if (m_center == center)
        return;

    m_center = center;
    m_centerChanged = true;
    emit centerChanged(m_center);
    update();
}
//! [5]

void CustomItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    m_geometryChanged = true;
    update();
    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

//! [6]
QSGNode *CustomItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *)
{
    auto *node = static_cast<CustomNode *>(old);

    if (!node)
        node = new CustomNode;

    if (m_geometryChanged)
        node->setRect(boundingRect());
    m_geometryChanged = false;

    if (m_zoomChanged)
        node->setZoom(m_zoom);
    m_zoomChanged = false;

    if (m_limitChanged)
        node->setLimit(m_limit);
    m_limitChanged = false;

    if (m_centerChanged)
        node->setCenter(m_center);
    m_centerChanged = false;

    return node;
}
//! [6]
