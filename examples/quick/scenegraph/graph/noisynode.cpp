/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "noisynode.h"

#include <QtCore/QRandomGenerator>

#include <QtQuick/QSGTexture>
#include <QtQuick/QQuickWindow>
#include <QtQuick/QSGMaterial>

#define NOISE_SIZE 64

class NoisyShader : public QSGMaterialShader
{
public:
    NoisyShader() {
        setShaderFileName(VertexStage, QLatin1String(":/scenegraph/graph/shaders/noisy.vert.qsb"));
        setShaderFileName(FragmentStage, QLatin1String(":/scenegraph/graph/shaders/noisy.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

class NoisyMaterial : public QSGMaterial
{
public:
    NoisyMaterial()
    {
        setFlag(Blending);
    }

    ~NoisyMaterial()
    {
        delete state.texture;
    }

    QSGMaterialType *type() const override
    {
        static QSGMaterialType type;
        return &type;
    }

    QSGMaterialShader *createShader(QSGRendererInterface::RenderMode) const override
    {
        return new NoisyShader;
    }

    int compare(const QSGMaterial *m) const override
    {
        const NoisyMaterial *other = static_cast<const NoisyMaterial *>(m);

        if (int diff = int(state.color.rgb()) - int(other->state.color.rgb()))
            return diff;

        if (!state.texture || !other->state.texture)
            return state.texture ? 1 : -1;

        const qint64 diff = state.texture->comparisonKey() - other->state.texture->comparisonKey();
        return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
    }

    struct {
        QColor color;
        QSGTexture *texture;
    } state;
};

bool NoisyShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *)
{
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 92);

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 88, &opacity, 4);
    }

    NoisyMaterial *mat = static_cast<NoisyMaterial *>(newMaterial);
    float c[4] = { float(mat->state.color.redF()),
                   float(mat->state.color.greenF()),
                   float(mat->state.color.blueF()),
                   float(mat->state.color.alphaF()) };
    memcpy(buf->data() + 64, c, 16);

    const QSize s = mat->state.texture->textureSize();
    float textureSize[2] = { 1.0f / s.width(), 1.0f / s.height() };
    memcpy(buf->data() + 80, textureSize, 8);

    return true;
}

void NoisyShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                     QSGMaterial *newMaterial, QSGMaterial *)
{
    Q_UNUSED(state);
    Q_UNUSED(binding);

    NoisyMaterial *mat = static_cast<NoisyMaterial *>(newMaterial);
    *texture = mat->state.texture;
}

NoisyNode::NoisyNode(QQuickWindow *window)
{
    // Make some noise...
    QImage image(NOISE_SIZE, NOISE_SIZE, QImage::Format_RGB32);
    uint *data = (uint *) image.bits();
    for (int i=0; i<NOISE_SIZE * NOISE_SIZE; ++i) {
        uint g = QRandomGenerator::global()->bounded(0xff);
        data[i] = 0xff000000 | (g << 16) | (g << 8) | g;
    }

    QSGTexture *t = window->createTextureFromImage(image);
    t->setFiltering(QSGTexture::Nearest);
    t->setHorizontalWrapMode(QSGTexture::Repeat);
    t->setVerticalWrapMode(QSGTexture::Repeat);

    NoisyMaterial *m = new NoisyMaterial;
    m->state.texture = t;
    m->state.color = QColor::fromRgbF(0.95, 0.95, 0.97);

    setMaterial(m);
    setFlag(OwnsMaterial, true);

    QSGGeometry *g = new QSGGeometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4);
    QSGGeometry::updateTexturedRectGeometry(g, QRect(), QRect());
    setGeometry(g);
    setFlag(OwnsGeometry, true);
}

void NoisyNode::setRect(const QRectF &bounds)
{
    QSGGeometry::updateTexturedRectGeometry(geometry(), bounds, QRectF(0, 0, 1, 1));
    markDirty(QSGNode::DirtyGeometry);
}
