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

#include "qsgd3d12material_p.h"
#include <private/qsgrenderer_p.h>

#include "vs_vertexcolor.hlslh"
#include "ps_vertexcolor.hlslh"
#include "vs_smoothcolor.hlslh"
#include "ps_smoothcolor.hlslh"

QT_BEGIN_NAMESPACE

QSGD3D12Material::RenderState QSGD3D12Material::makeRenderState(QSGRenderer *renderer, RenderState::DirtyStates dirty)
{
    RenderState rs;
    rs.m_dirty = dirty;
    rs.m_data = renderer;
    return rs;
}

float QSGD3D12Material::RenderState::opacity() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentOpacity();
}

float QSGD3D12Material::RenderState::determinant() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->determinant();
}

QMatrix4x4 QSGD3D12Material::RenderState::combinedMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentCombinedMatrix();
}

float QSGD3D12Material::RenderState::devicePixelRatio() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->devicePixelRatio();
}

QMatrix4x4 QSGD3D12Material::RenderState::modelViewMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentModelViewMatrix();
}

QMatrix4x4 QSGD3D12Material::RenderState::projectionMatrix() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->currentProjectionMatrix();
}

QRect QSGD3D12Material::RenderState::viewportRect() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->viewportRect();
}

QRect QSGD3D12Material::RenderState::deviceRect() const
{
    Q_ASSERT(m_data);
    return static_cast<const QSGRenderer *>(m_data)->deviceRect();
}

QSGMaterialShader *QSGD3D12Material::createShader() const
{
    return nullptr;
}

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

QT_END_NAMESPACE
