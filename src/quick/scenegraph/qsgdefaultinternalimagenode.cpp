// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgdefaultinternalimagenode_p.h"
#include <private/qsgdefaultrendercontext_p.h>
#include <private/qsgmaterialshader_p.h>
#include <private/qsgtexturematerial_p.h>
#include <qopenglfunctions.h>
#include <QtCore/qmath.h>
#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

class SmoothTextureMaterialRhiShader : public QSGTextureMaterialRhiShader
{
public:
    SmoothTextureMaterialRhiShader(int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};


QSGSmoothTextureMaterial::QSGSmoothTextureMaterial()
{
    setFlag(RequiresFullMatrixExceptTranslate, true);
    setFlag(Blending, true);
}

void QSGSmoothTextureMaterial::setTexture(QSGTexture *texture)
{
    m_texture = texture;
}

QSGMaterialType *QSGSmoothTextureMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGSmoothTextureMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new SmoothTextureMaterialRhiShader(viewCount());
}

SmoothTextureMaterialRhiShader::SmoothTextureMaterialRhiShader(int viewCount)
    : QSGTextureMaterialRhiShader(viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/smoothtexture.vert.qsb"), viewCount);
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/smoothtexture.frag.qsb"), viewCount);
}

bool SmoothTextureMaterialRhiShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();
    const int shaderMatrixCount = newMaterial->viewCount();

    if (!oldMaterial) {
        // The viewport is constant, so set the pixel size uniform only once (per batches with the same material).
        const QRect r = state.viewportRect();
        const QVector2D v(2.0f / r.width(), 2.0f / r.height());
        // mat4 matrix, float opacity, vec2 pixelSize, and the vec2 must be 2*4 aligned
        memcpy(buf->data() + 64 * shaderMatrixCount + 8, &v, 8);
        changed = true;
    }

    changed |= QSGTextureMaterialRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    return changed;
}


QSGDefaultInternalImageNode::QSGDefaultInternalImageNode(QSGDefaultRenderContext *rc)
    : m_rc(rc)
{
    setMaterial(&m_materialO);
    setOpaqueMaterial(&m_material);
}

void QSGDefaultInternalImageNode::setFiltering(QSGTexture::Filtering filtering)
{
    if (m_material.filtering() == filtering)
        return;

    m_material.setFiltering(filtering);
    m_materialO.setFiltering(filtering);
    m_smoothMaterial.setFiltering(filtering);
    markDirty(DirtyMaterial);
}

void QSGDefaultInternalImageNode::setMipmapFiltering(QSGTexture::Filtering filtering)
{
    if (m_material.mipmapFiltering() == filtering)
        return;

    m_material.setMipmapFiltering(filtering);
    m_materialO.setMipmapFiltering(filtering);
    m_smoothMaterial.setMipmapFiltering(filtering);
    markDirty(DirtyMaterial);
}

void QSGDefaultInternalImageNode::setVerticalWrapMode(QSGTexture::WrapMode wrapMode)
{
    if (m_material.verticalWrapMode() == wrapMode)
        return;

    m_material.setVerticalWrapMode(wrapMode);
    m_materialO.setVerticalWrapMode(wrapMode);
    m_smoothMaterial.setVerticalWrapMode(wrapMode);
    markDirty(DirtyMaterial);
}

void QSGDefaultInternalImageNode::setHorizontalWrapMode(QSGTexture::WrapMode wrapMode)
{
    if (m_material.horizontalWrapMode() == wrapMode)
        return;

    m_material.setHorizontalWrapMode(wrapMode);
    m_materialO.setHorizontalWrapMode(wrapMode);
    m_smoothMaterial.setHorizontalWrapMode(wrapMode);
    markDirty(DirtyMaterial);
}

void QSGDefaultInternalImageNode::updateMaterialAntialiasing()
{
    if (m_antialiasing) {
        setMaterial(&m_smoothMaterial);
        setOpaqueMaterial(nullptr);
    } else {
        setMaterial(&m_materialO);
        setOpaqueMaterial(&m_material);
    }
}

void QSGDefaultInternalImageNode::setMaterialTexture(QSGTexture *texture)
{
    m_material.setTexture(texture);
    m_materialO.setTexture(texture);
    m_smoothMaterial.setTexture(texture);
}

QSGTexture *QSGDefaultInternalImageNode::materialTexture() const
{
    return m_material.texture();
}

bool QSGDefaultInternalImageNode::updateMaterialBlending()
{
    const bool alpha = m_material.flags() & QSGMaterial::Blending;
    if (materialTexture() && alpha != materialTexture()->hasAlphaChannel()) {
        m_material.setFlag(QSGMaterial::Blending, !alpha);
        return true;
    }
    return false;
}

inline static bool isPowerOfTwo(int x)
{
    // Assumption: x >= 1
    return x == (x & -x);
}

bool QSGDefaultInternalImageNode::supportsWrap(const QSize &size) const
{
    bool npotSupported = m_rc->rhi() && m_rc->rhi()->isFeatureSupported(QRhi::NPOTTextureRepeat);
    return npotSupported || (isPowerOfTwo(size.width()) && isPowerOfTwo(size.height()));
}

QT_END_NAMESPACE
