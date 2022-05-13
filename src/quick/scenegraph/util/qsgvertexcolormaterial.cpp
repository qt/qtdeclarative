// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgvertexcolormaterial.h"

QT_BEGIN_NAMESPACE

class QSGVertexColorMaterialRhiShader : public QSGMaterialShader
{
public:
    QSGVertexColorMaterialRhiShader();

    bool updateUniformData(RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
};

QSGVertexColorMaterialRhiShader::QSGVertexColorMaterialRhiShader()
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/vertexcolor.vert.qsb"));
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/vertexcolor.frag.qsb"));
}

bool QSGVertexColorMaterialRhiShader::updateUniformData(RenderState &state,
                                                        QSGMaterial * /*newEffect*/,
                                                        QSGMaterial * /*oldEffect*/)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();

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

    return changed;
}


/*!
    \class QSGVertexColorMaterial
    \brief The QSGVertexColorMaterial class provides a convenient way of rendering per-vertex
    colored geometry in the scene graph.

    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    \warning This utility class is only functional when running with the
    default backend of the Qt Quick scenegraph.

    The vertex color material will give each vertex in a geometry a color. Pixels between
    vertices will be linearly interpolated. The colors can contain transparency.

    The geometry to be rendered with vertex color must have the following layout. Attribute
    position 0 must contain vertices. Attribute position 1 must contain colors, a tuple of
    4 values with RGBA layout. Both floats in the range of 0 to 1 and unsigned bytes in
    the range 0 to 255 are valid for the color values.

    \note The rendering pipeline expects pixels with premultiplied alpha.

    QSGGeometry::defaultAttributes_ColoredPoint2D() can be used to construct an attribute
    set that is compatible with this material.

    The vertex color material respects both current opacity and current matrix when
    updating it's rendering state.
 */


/*!
    Creates a new vertex color material.
 */

QSGVertexColorMaterial::QSGVertexColorMaterial()
{
    setFlag(Blending, true);
}


/*!
    int QSGVertexColorMaterial::compare() const

    As the vertex color material has all its state in the vertex attributes,
    all materials will be equal.

    \internal
 */

int QSGVertexColorMaterial::compare(const QSGMaterial * /* other */) const
{
    return 0;
}

/*!
    \internal
 */

QSGMaterialType *QSGVertexColorMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}



/*!
    \internal
 */

QSGMaterialShader *QSGVertexColorMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new QSGVertexColorMaterialRhiShader;
}

QT_END_NAMESPACE
