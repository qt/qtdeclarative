// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgtexturematerial_p.h"
#include <private/qsgtexture_p.h>
#include <rhi/qrhi.h>

QT_BEGIN_NAMESPACE

inline static bool isPowerOfTwo(int x)
{
    // Assumption: x >= 1
    return x == (x & -x);
}

QSGOpaqueTextureMaterialRhiShader::QSGOpaqueTextureMaterialRhiShader(int viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/opaquetexture.vert.qsb"), viewCount);
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/opaquetexture.frag.qsb"), viewCount);
}

bool QSGOpaqueTextureMaterialRhiShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();
    const int matrixCount = qMin(state.projectionMatrixCount(), newMaterial->viewCount());

    for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
        if (state.isMatrixDirty()) {
            const QMatrix4x4 m = state.combinedMatrix(viewIndex);
            memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            changed = true;
        }
    }

    return changed;
}

void QSGOpaqueTextureMaterialRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    if (binding != 1)
        return;

#ifdef QT_NO_DEBUG
    Q_UNUSED(oldMaterial);
#endif
    Q_ASSERT(oldMaterial == nullptr || newMaterial->type() == oldMaterial->type());
    QSGOpaqueTextureMaterial *tx = static_cast<QSGOpaqueTextureMaterial *>(newMaterial);
    QSGTexture *t = tx->texture();
    if (!t) {
        *texture = nullptr;
        return;
    }

    t->setFiltering(tx->filtering());
    t->setMipmapFiltering(tx->mipmapFiltering());
    t->setAnisotropyLevel(tx->anisotropyLevel());

    t->setHorizontalWrapMode(tx->horizontalWrapMode());
    t->setVerticalWrapMode(tx->verticalWrapMode());
    if (!state.rhi()->isFeatureSupported(QRhi::NPOTTextureRepeat)) {
        QSize size = t->textureSize();
        const bool isNpot = !isPowerOfTwo(size.width()) || !isPowerOfTwo(size.height());
        if (isNpot) {
            t->setHorizontalWrapMode(QSGTexture::ClampToEdge);
            t->setVerticalWrapMode(QSGTexture::ClampToEdge);
            t->setMipmapFiltering(QSGTexture::None);
        }
    }

    t->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
    *texture = t;
}


/*!
    \class QSGOpaqueTextureMaterial
    \brief The QSGOpaqueTextureMaterial class provides a convenient way of
    rendering textured geometry in the scene graph.
    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    \warning This utility class is only functional when running with the
    default backend of the Qt Quick scenegraph.

    The opaque textured material will fill every pixel in a geometry with
    the supplied texture. The material does not respect the opacity of the
    QSGMaterialShader::RenderState, so opacity nodes in the parent chain
    of nodes using this material, have no effect.

    The geometry to be rendered with an opaque texture material requires
    vertices in attribute location 0 and texture coordinates in attribute
    location 1. The texture coordinate is a 2-dimensional floating-point
    tuple. The QSGGeometry::defaultAttributes_TexturedPoint2D returns an
    attribute set compatible with this material.

    The texture to be rendered can be set using setTexture(). How the
    texture should be rendered can be specified using setMipmapFiltering(),
    setFiltering(), setHorizontalWrapMode() and setVerticalWrapMode().
    The rendering state is set on the texture instance just before it
    is bound.

    The opaque textured material respects the current matrix and the alpha
    channel of the texture. It will disregard the accumulated opacity in
    the scenegraph.

    A texture material must have a texture set before it is used as
    a material in the scene graph.
 */



/*!
    Creates a new QSGOpaqueTextureMaterial.

    The default mipmap filtering and filtering mode is set to
    QSGTexture::Nearest. The default wrap modes is set to
    \c QSGTexture::ClampToEdge.

 */
QSGOpaqueTextureMaterial::QSGOpaqueTextureMaterial()
    : m_texture(nullptr)
    , m_filtering(QSGTexture::Nearest)
    , m_mipmap_filtering(QSGTexture::None)
    , m_horizontal_wrap(QSGTexture::ClampToEdge)
    , m_vertical_wrap(QSGTexture::ClampToEdge)
    , m_anisotropy_level(QSGTexture::AnisotropyNone)
{
}


/*!
    \internal
 */
QSGMaterialType *QSGOpaqueTextureMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

/*!
    \internal
 */
QSGMaterialShader *QSGOpaqueTextureMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new QSGOpaqueTextureMaterialRhiShader(viewCount());
}


/*!
    \fn QSGTexture *QSGOpaqueTextureMaterial::texture() const

    Returns this texture material's texture.
 */



/*!
    Sets the texture of this material to \a texture.

    The material does not take ownership of the texture.
 */

void QSGOpaqueTextureMaterial::setTexture(QSGTexture *texture)
{
    m_texture = texture;
    setFlag(Blending, m_texture ? m_texture->hasAlphaChannel() : false);
}



/*!
    \fn void QSGOpaqueTextureMaterial::setMipmapFiltering(QSGTexture::Filtering filtering)

    Sets the mipmap mode to \a filtering.

    The mipmap filtering mode is set on the texture instance just before the
    texture is bound for rendering.

    If the texture does not have mipmapping support, enabling mipmapping has no
    effect.
 */



/*!
    \fn QSGTexture::Filtering QSGOpaqueTextureMaterial::mipmapFiltering() const

    Returns this material's mipmap filtering mode.

    The default mipmap mode is \c QSGTexture::Nearest.
 */



/*!
    \fn void QSGOpaqueTextureMaterial::setFiltering(QSGTexture::Filtering filtering)

    Sets the filtering to \a filtering.

    The filtering mode is set on the texture instance just before the texture
    is bound for rendering.
 */



/*!
    \fn QSGTexture::Filtering QSGOpaqueTextureMaterial::filtering() const

    Returns this material's filtering mode.

    The default filtering is \c QSGTexture::Nearest.
 */



/*!
    \fn void QSGOpaqueTextureMaterial::setHorizontalWrapMode(QSGTexture::WrapMode mode)

    Sets the horizontal wrap mode to \a mode.

    The horizontal wrap mode is set on the texture instance just before the texture
    is bound for rendering.
 */



 /*!
     \fn QSGTexture::WrapMode QSGOpaqueTextureMaterial::horizontalWrapMode() const

     Returns this material's horizontal wrap mode.

     The default horizontal wrap mode is \c QSGTexture::ClampToEdge.
  */



/*!
    \fn void QSGOpaqueTextureMaterial::setVerticalWrapMode(QSGTexture::WrapMode mode)

    Sets the vertical wrap mode to \a mode.

    The vertical wrap mode is set on the texture instance just before the texture
    is bound for rendering.
 */



 /*!
     \fn QSGTexture::WrapMode QSGOpaqueTextureMaterial::verticalWrapMode() const

     Returns this material's vertical wrap mode.

     The default vertical wrap mode is \c QSGTexture::ClampToEdge.
  */

/*!
  \fn void QSGOpaqueTextureMaterial::setAnisotropyLevel(QSGTexture::AnisotropyLevel level)

  Sets this material's anistropy level to \a level.
*/

/*!
  \fn QSGTexture::AnisotropyLevel QSGOpaqueTextureMaterial::anisotropyLevel() const

  Returns this material's anistropy level.
*/

/*!
    \internal
 */

int QSGOpaqueTextureMaterial::compare(const QSGMaterial *o) const
{
    Q_ASSERT(o && type() == o->type());
    const QSGOpaqueTextureMaterial *other = static_cast<const QSGOpaqueTextureMaterial *>(o);
    Q_ASSERT(m_texture);
    Q_ASSERT(other->texture());
    const qint64 diff = m_texture->comparisonKey() - other->texture()->comparisonKey();
    if (diff != 0)
        return diff < 0 ? -1 : 1;
    return int(m_filtering) - int(other->m_filtering);
}



/*!
    \class QSGTextureMaterial
    \brief The QSGTextureMaterial class provides a convenient way of
    rendering textured geometry in the scene graph.
    \inmodule QtQuick
    \ingroup qtquick-scenegraph-materials

    \warning This utility class is only functional when running with the
    default backend of the Qt Quick scenegraph.

    The textured material will fill every pixel in a geometry with
    the supplied texture.

    The geometry to be rendered with a texture material requires
    vertices in attribute location 0 and texture coordinates in attribute
    location 1. The texture coordinate is a 2-dimensional floating-point
    tuple. The QSGGeometry::defaultAttributes_TexturedPoint2D returns an
    attribute set compatible with this material.

    The texture to be rendered can be set using setTexture(). How the
    texture should be rendered can be specified using setMipmapFiltering(),
    setFiltering(), setHorizontalWrapMode() and setVerticalWrapMode().
    The rendering state is set on the texture instance just before it
    is bound.

    The textured material respects the current matrix and the alpha
    channel of the texture. It will also respect the accumulated opacity
    in the scenegraph.

    A texture material must have a texture set before it is used as
    a material in the scene graph.
 */

/*!
    \internal
 */

QSGMaterialType *QSGTextureMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

/*!
    \internal
 */

QSGMaterialShader *QSGTextureMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new QSGTextureMaterialRhiShader(viewCount());
}


QSGTextureMaterialRhiShader::QSGTextureMaterialRhiShader(int viewCount)
    : QSGOpaqueTextureMaterialRhiShader(viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/texture.vert.qsb"), viewCount);
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/texture.frag.qsb"), viewCount);
}

bool QSGTextureMaterialRhiShader::updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = false;
    QByteArray *buf = state.uniformData();
    const int shaderMatrixCount = newMaterial->viewCount();

    if (state.isOpacityDirty()) {
        const float opacity = state.opacity();
        memcpy(buf->data() + 64 * shaderMatrixCount, &opacity, 4);
        changed = true;
    }

    changed |= QSGOpaqueTextureMaterialRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    return changed;
}

QT_END_NAMESPACE
