/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgd3d12builtinmaterials_p.h"

#include "vs_vertexcolor.hlslh"
#include "ps_vertexcolor.hlslh"
#include "vs_smoothcolor.hlslh"
#include "ps_smoothcolor.hlslh"
#include "vs_texture.hlslh"
#include "ps_texture.hlslh"
#include "vs_smoothtexture.hlslh"
#include "ps_smoothtexture.hlslh"

QT_BEGIN_NAMESPACE

QSGMaterialType QSGD3D12VertexColorMaterial::mtype;

QSGMaterialType *QSGD3D12VertexColorMaterial::type() const
{
    return &QSGD3D12VertexColorMaterial::mtype;
}

int QSGD3D12VertexColorMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    // As the vertex color material has all its state in the vertex attributes
    // defined by the geometry, all such materials will be equal.
    return 0;
}

static const int VERTEX_COLOR_CB_SIZE_0 = 16 * sizeof(float); // float4x4
static const int VERTEX_COLOR_CB_SIZE_1 = sizeof(float); // float
static const int VERTEX_COLOR_CB_SIZE = VERTEX_COLOR_CB_SIZE_0 + VERTEX_COLOR_CB_SIZE_1;

int QSGD3D12VertexColorMaterial::constantBufferSize() const
{
    return QSGD3D12Engine::alignedConstantBufferSize(VERTEX_COLOR_CB_SIZE);
}

void QSGD3D12VertexColorMaterial::preparePipeline(QSGD3D12ShaderState *shaders)
{
    shaders->vs = g_VS_VertexColor;
    shaders->vsSize = sizeof(g_VS_VertexColor);
    shaders->ps = g_PS_VertexColor;
    shaders->psSize = sizeof(g_PS_VertexColor);
}

QSGD3D12Material::UpdateResults QSGD3D12VertexColorMaterial::updatePipeline(const RenderState &state,
                                                                            QSGD3D12ShaderState *,
                                                                            quint8 *constantBuffer)
{
    QSGD3D12Material::UpdateResults r = 0;
    quint8 *p = constantBuffer;

    if (state.isMatrixDirty()) {
        memcpy(p, state.combinedMatrix().constData(), VERTEX_COLOR_CB_SIZE_0);
        r |= UpdatedConstantBuffer;
    }
    p += VERTEX_COLOR_CB_SIZE_0;

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(p, &opacity, VERTEX_COLOR_CB_SIZE_1);
        r |= UpdatedConstantBuffer;
    }

    return r;
}

QSGD3D12SmoothColorMaterial::QSGD3D12SmoothColorMaterial()
{
    setFlag(RequiresFullMatrixExceptTranslate, true);
    setFlag(Blending, true);
}

QSGMaterialType QSGD3D12SmoothColorMaterial::mtype;

QSGMaterialType *QSGD3D12SmoothColorMaterial::type() const
{
    return &QSGD3D12SmoothColorMaterial::mtype;
}

int QSGD3D12SmoothColorMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    return 0;
}

static const int SMOOTH_COLOR_CB_SIZE_0 = 16 * sizeof(float); // float4x4
static const int SMOOTH_COLOR_CB_SIZE_1 = sizeof(float); // float
static const int SMOOTH_COLOR_CB_SIZE_2 = 2 * sizeof(float); // float2
static const int SMOOTH_COLOR_CB_SIZE = SMOOTH_COLOR_CB_SIZE_0 + SMOOTH_COLOR_CB_SIZE_1 + SMOOTH_COLOR_CB_SIZE_2;

int QSGD3D12SmoothColorMaterial::constantBufferSize() const
{
    return QSGD3D12Engine::alignedConstantBufferSize(SMOOTH_COLOR_CB_SIZE);
}

void QSGD3D12SmoothColorMaterial::preparePipeline(QSGD3D12ShaderState *shaders)
{
    shaders->vs = g_VS_SmoothColor;
    shaders->vsSize = sizeof(g_VS_SmoothColor);
    shaders->ps = g_PS_SmoothColor;
    shaders->psSize = sizeof(g_PS_SmoothColor);
}

QSGD3D12Material::UpdateResults QSGD3D12SmoothColorMaterial::updatePipeline(const RenderState &state,
                                                                            QSGD3D12ShaderState *,
                                                                            quint8 *constantBuffer)
{
    QSGD3D12Material::UpdateResults r = 0;
    quint8 *p = constantBuffer;

    if (state.isMatrixDirty()) {
        memcpy(p, state.combinedMatrix().constData(), SMOOTH_COLOR_CB_SIZE_0);
        r |= UpdatedConstantBuffer;
    }
    p += SMOOTH_COLOR_CB_SIZE_0;

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(p, &opacity, SMOOTH_COLOR_CB_SIZE_1);
        r |= UpdatedConstantBuffer;
    }
    p += SMOOTH_COLOR_CB_SIZE_1;

    if (state.isMatrixDirty()) {
        const QRect viewport = state.viewportRect();
        const float v[] = { 2.0f / viewport.width(), 2.0f / viewport.height() };
        memcpy(p, v, SMOOTH_COLOR_CB_SIZE_2);
        r |= UpdatedConstantBuffer;
    }

    return r;
}

QSGMaterialType QSGD3D12TextureMaterial::mtype;

QSGMaterialType *QSGD3D12TextureMaterial::type() const
{
    return &QSGD3D12TextureMaterial::mtype;
}

int QSGD3D12TextureMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    const QSGD3D12TextureMaterial *o = static_cast<const QSGD3D12TextureMaterial *>(other);
    if (int diff = m_texture->textureId() - o->texture()->textureId())
        return diff;
    return int(m_filtering) - int(o->m_filtering);
}

static const int TEXTURE_CB_SIZE_0 = 16 * sizeof(float); // float4x4
static const int TEXTURE_CB_SIZE_1 = sizeof(float); // float
static const int TEXTURE_CB_SIZE = TEXTURE_CB_SIZE_0 + TEXTURE_CB_SIZE_1;

int QSGD3D12TextureMaterial::constantBufferSize() const
{
    return QSGD3D12Engine::alignedConstantBufferSize(TEXTURE_CB_SIZE);
}

void QSGD3D12TextureMaterial::preparePipeline(QSGD3D12ShaderState *shaders)
{
    shaders->vs = g_VS_Texture;
    shaders->vsSize = sizeof(g_VS_Texture);
    shaders->ps = g_PS_Texture;
    shaders->psSize = sizeof(g_PS_Texture);

    shaders->rootSig.textureViews.resize(1);
}

QSGD3D12Material::UpdateResults QSGD3D12TextureMaterial::updatePipeline(const RenderState &state,
                                                                        QSGD3D12ShaderState *shaders,
                                                                        quint8 *constantBuffer)
{
    QSGD3D12Material::UpdateResults r = 0;
    quint8 *p = constantBuffer;

    if (state.isMatrixDirty()) {
        memcpy(p, state.combinedMatrix().constData(), TEXTURE_CB_SIZE_0);
        r |= UpdatedConstantBuffer;
    }
    p += TEXTURE_CB_SIZE_0;

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(p, &opacity, TEXTURE_CB_SIZE_1);
        r |= UpdatedConstantBuffer;
    }

    Q_ASSERT(m_texture);
    m_texture->setFiltering(m_filtering);
    m_texture->setMipmapFiltering(m_mipmap_filtering);
    m_texture->setHorizontalWrapMode(m_horizontal_wrap);
    m_texture->setVerticalWrapMode(m_vertical_wrap);

    QSGD3D12TextureView &tv(shaders->rootSig.textureViews[0]);
    if (m_filtering == QSGTexture::Linear)
        tv.filter = m_mipmap_filtering == QSGTexture::Linear
                ? QSGD3D12TextureView::FilterLinear : QSGD3D12TextureView::FilterMinMagLinearMipNearest;
    else
        tv.filter = m_mipmap_filtering == QSGTexture::Linear
                ? QSGD3D12TextureView::FilterMinMagNearestMipLinear : QSGD3D12TextureView::FilterNearest;
    tv.addressModeHoriz = m_horizontal_wrap == QSGTexture::ClampToEdge ? QSGD3D12TextureView::AddressClamp : QSGD3D12TextureView::AddressWrap;
    tv.addressModeVert = m_vertical_wrap == QSGTexture::ClampToEdge ? QSGD3D12TextureView::AddressClamp : QSGD3D12TextureView::AddressWrap;

    m_texture->bind();

    return r;
}

QSGD3D12SmoothTextureMaterial::QSGD3D12SmoothTextureMaterial()
{
    setFlag(RequiresFullMatrixExceptTranslate, true);
    setFlag(Blending, true);
}

QSGMaterialType QSGD3D12SmoothTextureMaterial::mtype;

QSGMaterialType *QSGD3D12SmoothTextureMaterial::type() const
{
    return &QSGD3D12SmoothTextureMaterial::mtype;
}

int QSGD3D12SmoothTextureMaterial::compare(const QSGMaterial *other) const
{
    Q_ASSERT(other && type() == other->type());
    const QSGD3D12SmoothTextureMaterial *o = static_cast<const QSGD3D12SmoothTextureMaterial *>(other);
    if (int diff = m_texture->textureId() - o->texture()->textureId())
        return diff;
    return int(m_filtering) - int(o->m_filtering);
}

static const int SMOOTH_TEXTURE_CB_SIZE_0 = 16 * sizeof(float); // float4x4
static const int SMOOTH_TEXTURE_CB_SIZE_1 = sizeof(float); // float
static const int SMOOTH_TEXTURE_CB_SIZE_2 = 2 * sizeof(float); // float2
static const int SMOOTH_TEXTURE_CB_SIZE = SMOOTH_TEXTURE_CB_SIZE_0 + SMOOTH_TEXTURE_CB_SIZE_1 + SMOOTH_TEXTURE_CB_SIZE_2;

int QSGD3D12SmoothTextureMaterial::constantBufferSize() const
{
    return QSGD3D12Engine::alignedConstantBufferSize(SMOOTH_TEXTURE_CB_SIZE);
}

void QSGD3D12SmoothTextureMaterial::preparePipeline(QSGD3D12ShaderState *shaders)
{
    shaders->vs = g_VS_SmoothTexture;
    shaders->vsSize = sizeof(g_VS_SmoothTexture);
    shaders->ps = g_PS_SmoothTexture;
    shaders->psSize = sizeof(g_PS_SmoothTexture);

    shaders->rootSig.textureViews.resize(1);
}

QSGD3D12Material::UpdateResults QSGD3D12SmoothTextureMaterial::updatePipeline(const RenderState &state,
                                                                              QSGD3D12ShaderState *shaders,
                                                                              quint8 *constantBuffer)
{
    QSGD3D12Material::UpdateResults r = 0;
    quint8 *p = constantBuffer;

    if (state.isMatrixDirty()) {
        memcpy(p, state.combinedMatrix().constData(), SMOOTH_TEXTURE_CB_SIZE_0);
        r |= UpdatedConstantBuffer;
    }
    p += SMOOTH_TEXTURE_CB_SIZE_0;

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(p, &opacity, SMOOTH_TEXTURE_CB_SIZE_1);
        r |= UpdatedConstantBuffer;
    }
    p += SMOOTH_TEXTURE_CB_SIZE_1;

    if (state.isMatrixDirty()) {
        const QRect viewport = state.viewportRect();
        const float v[] = { 2.0f / viewport.width(), 2.0f / viewport.height() };
        memcpy(p, v, SMOOTH_TEXTURE_CB_SIZE_2);
        r |= UpdatedConstantBuffer;
    }

    Q_ASSERT(m_texture);
    m_texture->setFiltering(m_filtering);
    m_texture->setMipmapFiltering(m_mipmap_filtering);
    m_texture->setHorizontalWrapMode(m_horizontal_wrap);
    m_texture->setVerticalWrapMode(m_vertical_wrap);

    QSGD3D12TextureView &tv(shaders->rootSig.textureViews[0]);
    if (m_filtering == QSGTexture::Linear)
        tv.filter = m_mipmap_filtering == QSGTexture::Linear
                ? QSGD3D12TextureView::FilterLinear : QSGD3D12TextureView::FilterMinMagLinearMipNearest;
    else
        tv.filter = m_mipmap_filtering == QSGTexture::Linear
                ? QSGD3D12TextureView::FilterMinMagNearestMipLinear : QSGD3D12TextureView::FilterNearest;
    tv.addressModeHoriz = m_horizontal_wrap == QSGTexture::ClampToEdge ? QSGD3D12TextureView::AddressClamp : QSGD3D12TextureView::AddressWrap;
    tv.addressModeVert = m_vertical_wrap == QSGTexture::ClampToEdge ? QSGD3D12TextureView::AddressClamp : QSGD3D12TextureView::AddressWrap;

    m_texture->bind();

    return r;
}

QT_END_NAMESPACE
