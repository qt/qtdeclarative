/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#include "qsgvertexcolormaterial.h"

#include <qopenglshaderprogram.h>

QT_BEGIN_NAMESPACE

class QSGVertexColorMaterialShader : public QSGMaterialShader
{
public:
    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect);
    virtual char const *const *attributeNames() const;

    static QSGMaterialType type;

private:
    virtual void initialize();
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;

    int m_matrix_id;
    int m_opacity_id;
};

QSGMaterialType QSGVertexColorMaterialShader::type;

void QSGVertexColorMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *)
{
    if (!(newEffect->flags() & QSGMaterial::Blending) || state.isOpacityDirty())
        program()->setUniformValue(m_opacity_id, state.opacity());

    if (state.isMatrixDirty())
        program()->setUniformValue(m_matrix_id, state.combinedMatrix());
}

char const *const *QSGVertexColorMaterialShader::attributeNames() const
{
    static const char *const attr[] = { "vertexCoord", "vertexColor", 0 };
    return attr;
}

void QSGVertexColorMaterialShader::initialize()
{
    m_matrix_id = program()->uniformLocation("matrix");
    m_opacity_id = program()->uniformLocation("opacity");
}

const char *QSGVertexColorMaterialShader::vertexShader() const {
    return
        "attribute highp vec4 vertexCoord;              \n"
        "attribute highp vec4 vertexColor;              \n"
        "uniform highp mat4 matrix;                     \n"
        "uniform highp float opacity;                   \n"
        "varying lowp vec4 color;                       \n"
        "void main() {                                  \n"
        "    gl_Position = matrix * vertexCoord;        \n"
        "    color = vertexColor * opacity;             \n"
        "}";
}

const char *QSGVertexColorMaterialShader::fragmentShader() const {
    return
        "varying lowp vec4 color;                       \n"
        "void main() {                                  \n"
        "    gl_FragColor = color;                      \n"
        "}";
}



/*!
    \class QSGVertexColorMaterial
    \brief The QSGVertexColorMaterial class provides a convenient way of rendering per-vertex
    colored geometry in the scene graph.

    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    The vertex color material will give each vertex in a geometry a color. Pixels between
    vertices will be linearly interpolated. The colors can contain transparency.

    The geometry to be rendered with vertex color must have the following layout. Attribute
    position 0 must contain vertices. Attribute position 1 must contain colors, a tuple of
    4 values with RGBA layout. Both floats in the range of 0 to 1 and unsigned bytes in
    the range 0 to 255 are valid for the color values. The
    QSGGeometry::defaultAttributes_ColoredPoint2D() constructs an attribute set
    compatible with this material.

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
    return &QSGVertexColorMaterialShader::type;
}



/*!
    \internal
 */

QSGMaterialShader *QSGVertexColorMaterial::createShader() const
{
    return new QSGVertexColorMaterialShader;
}

QT_END_NAMESPACE
