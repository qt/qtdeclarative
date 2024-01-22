// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgflatcolormaterial.h"
#include <private/qsgmaterialshader_p.h>

QT_BEGIN_NAMESPACE

class FlatColorMaterialRhiShader : public QSGMaterialShader
{
public:
    FlatColorMaterialRhiShader(int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

    static QSGMaterialType type;
};

QSGMaterialType FlatColorMaterialRhiShader::type;

FlatColorMaterialRhiShader::FlatColorMaterialRhiShader(int viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/flatcolor.vert.qsb"), viewCount);
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/flatcolor.frag.qsb"), viewCount);
}

bool FlatColorMaterialRhiShader::updateUniformData(RenderState &state,
                                                   QSGMaterial *newMaterial,
                                                   QSGMaterial *oldMaterial)
{
    Q_ASSERT(!oldMaterial || newMaterial->type() == oldMaterial->type());
    QSGFlatColorMaterial *oldMat = static_cast<QSGFlatColorMaterial *>(oldMaterial);
    QSGFlatColorMaterial *mat = static_cast<QSGFlatColorMaterial *>(newMaterial);
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

    const QColor &c = mat->color();
    if (!oldMat || c != oldMat->color() || state.isOpacityDirty()) {
        float r, g, b, a;
        c.getRgbF(&r, &g, &b, &a);
        const float opacity = state.opacity() * a;
        QVector4D v(r * opacity, g * opacity, b * opacity, opacity);
        Q_ASSERT(sizeof(v) == 16);
        memcpy(buf->data() + 64 * shaderMatrixCount, &v, 16);
        changed = true;
    }

    return changed;
}


/*!
    \class QSGFlatColorMaterial

    \brief The QSGFlatColorMaterial class provides a convenient way of rendering
    solid colored geometry in the scene graph.

    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    \warning This utility class is only functional when running with the default
    backend of the Qt Quick scenegraph.

    The flat color material will fill every pixel in a geometry using
    a solid color. The color can contain transparency.

    The geometry to be rendered with a flat color material requires
    vertices in attribute location 0 in the QSGGeometry object to render
    correctly. The QSGGeometry::defaultAttributes_Point2D() returns an attribute
    set compatible with this material.

    The flat color material respects both current opacity and current matrix
    when updating its rendering state.
 */


/*!
    Constructs a new flat color material.

    The default color is white.
 */

QSGFlatColorMaterial::QSGFlatColorMaterial() : m_color(QColor(255, 255, 255))
{
}

/*!
    \fn const QColor &QSGFlatColorMaterial::color() const

    Returns this flat color material's color.

    The default color is white.
 */



/*!
    Sets this flat color material's color to \a color.
 */

void QSGFlatColorMaterial::setColor(const QColor &color)
{
    m_color = color;
    setFlag(Blending, m_color.alpha() != 0xff);
}



/*!
    \internal
 */

QSGMaterialType *QSGFlatColorMaterial::type() const
{
    return &FlatColorMaterialRhiShader::type;
}



/*!
    \internal
 */

QSGMaterialShader *QSGFlatColorMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new FlatColorMaterialRhiShader(viewCount());
}


/*!
    \internal
 */

int QSGFlatColorMaterial::compare(const QSGMaterial *other) const
{
    const QSGFlatColorMaterial *flat = static_cast<const QSGFlatColorMaterial *>(other);
    return m_color.rgba() - flat->color().rgba();

}

QT_END_NAMESPACE
