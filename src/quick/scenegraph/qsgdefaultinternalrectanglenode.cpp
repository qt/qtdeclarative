// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgdefaultinternalrectanglenode_p.h"

#include <QtQuick/qsgvertexcolormaterial.h>
#include <QtQuick/qsgtexturematerial.h>

#include <QtQuick/private/qsgcontext_p.h>

#include <QtCore/qmath.h>
#include <QtCore/qvarlengtharray.h>

QT_BEGIN_NAMESPACE

class SmoothColorMaterialRhiShader : public QSGMaterialShader
{
public:
    SmoothColorMaterialRhiShader(int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

SmoothColorMaterialRhiShader::SmoothColorMaterialRhiShader(int viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/smoothcolor.vert.qsb"), viewCount);
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/smoothcolor.frag.qsb"), viewCount);
}

bool SmoothColorMaterialRhiShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();
    const int shaderMatrixCount = newMaterial->viewCount();
    const int matrixCount = qMin(state.projectionMatrixCount(), shaderMatrixCount);

    for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
        if (state.isMatrixDirty()) {
            const QMatrix4x4 m = state.combinedMatrix(viewIndex);
            memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            changed = true;
        }
    }

    if (oldMaterial == nullptr) {
        // The viewport is constant, so set the pixel size uniform only once.
        const QRect r = state.viewportRect();
        const QVector2D v(2.0f / r.width(), 2.0f / r.height());
        Q_ASSERT(sizeof(v) == 8);
        memcpy(buf->data() + 64 * shaderMatrixCount, &v, 8);
        changed = true;
    }

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64 * shaderMatrixCount + 8, &opacity, 4);
        changed = true;
    }

    return changed;
}


QSGSmoothColorMaterial::QSGSmoothColorMaterial()
{
    setFlag(RequiresFullMatrixExceptTranslate, true);
    setFlag(Blending, true);
}

int QSGSmoothColorMaterial::compare(const QSGMaterial *) const
{
    // all state in vertex attributes -> all smoothcolor materials are equal
    return 0;
}

QSGMaterialType *QSGSmoothColorMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGSmoothColorMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new SmoothColorMaterialRhiShader(viewCount());
}

QSGDefaultInternalRectangleNode::QSGDefaultInternalRectangleNode()
{
    setMaterial(&m_material);
}

void QSGDefaultInternalRectangleNode::updateMaterialAntialiasing()
{
    if (m_antialiasing)
        setMaterial(&m_smoothMaterial);
    else
        setMaterial(&m_material);
}

void QSGDefaultInternalRectangleNode::updateMaterialBlending(QSGNode::DirtyState *state)
{
    // smoothed material is always blended, so no change in material state
    if (material() == &m_material) {
        bool wasBlending = (m_material.flags() & QSGMaterial::Blending);
        bool isBlending = (m_gradient_stops.size() > 0 && !m_gradient_is_opaque)
                           || (m_color.alpha() < 255 && m_color.alpha() != 0)
                           || (m_pen_width > 0 && m_border_color.alpha() < 255);
        if (wasBlending != isBlending) {
            m_material.setFlag(QSGMaterial::Blending, isBlending);
            *state |= QSGNode::DirtyMaterial;
        }
    }
}

QT_END_NAMESPACE
