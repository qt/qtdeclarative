/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick module of the Qt Toolkit.
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


#include "qsgsimpletexturenode.h"
#include <private/qsgnode_p.h>

QT_BEGIN_NAMESPACE

class QSGSimpleTextureNodePrivate : public QSGGeometryNodePrivate
{
public:
    QSGSimpleTextureNodePrivate()
        : QSGGeometryNodePrivate()
        , m_texCoordMode(QSGSimpleTextureNode::NoTransform)
        , isAtlasTexture(false)
    {}

    QSGSimpleTextureNode::TextureCoordinatesTransformMode m_texCoordMode;
    uint isAtlasTexture : 1;
};

static void qsgsimpletexturenode_update(QSGGeometry *g,
                                        QSGTexture *texture,
                                        const QRectF &rect,
                                        QSGSimpleTextureNode::TextureCoordinatesTransformMode texCoordMode)
{
    if (!texture)
        return;

    QSize ts = texture->textureSize();
    QRectF sourceRect(0, 0, ts.width(), ts.height());

    // Maybe transform the texture coordinates
    if (texCoordMode.testFlag(QSGSimpleTextureNode::MirrorHorizontally)) {
        float tmp = sourceRect.left();
        sourceRect.setLeft(sourceRect.right());
        sourceRect.setRight(tmp);
    }
    if (texCoordMode.testFlag(QSGSimpleTextureNode::MirrorVertically)) {
        float tmp = sourceRect.top();
        sourceRect.setTop(sourceRect.bottom());
        sourceRect.setBottom(tmp);
    }

    QSGGeometry::updateTexturedRectGeometry(g, rect, texture->convertToNormalizedSourceRect(sourceRect));
}

/*!
  \class QSGSimpleTextureNode
  \brief The QSGSimpleTextureNode class is provided for convenience to easily draw
  textured content using the QML scene graph.

  \inmodule QtQuick

  \warning The simple texture node class must have a texture before being
  added to the scene graph to be rendered.
*/

/*!
    Constructs a new simple texture node
 */
QSGSimpleTextureNode::QSGSimpleTextureNode()
    : QSGGeometryNode(*new QSGSimpleTextureNodePrivate)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
{
    setGeometry(&m_geometry);
    setMaterial(&m_material);
    setOpaqueMaterial(&m_opaque_material);
    m_material.setMipmapFiltering(QSGTexture::None);
    m_opaque_material.setMipmapFiltering(QSGTexture::None);
#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(this, QLatin1String("simpletexture"));
#endif
}

/*!
    Sets the filtering to be used for this texture node to \a filtering.

    For smooth scaling, use QSGTexture::Linear; for normal scaling, use
    QSGTexture::Nearest.
 */
void QSGSimpleTextureNode::setFiltering(QSGTexture::Filtering filtering)
{
    if (m_material.filtering() == filtering)
        return;

    m_material.setFiltering(filtering);
    m_opaque_material.setFiltering(filtering);
    markDirty(DirtyMaterial);
}


/*!
    Returns the filtering currently set on this texture node
 */
QSGTexture::Filtering QSGSimpleTextureNode::filtering() const
{
    return m_material.filtering();
}


/*!
    Sets the target rect of this texture node to \a r.
 */
void QSGSimpleTextureNode::setRect(const QRectF &r)
{
    if (m_rect == r)
        return;
    m_rect = r;
    Q_D(QSGSimpleTextureNode);
    qsgsimpletexturenode_update(&m_geometry, texture(), m_rect, d->m_texCoordMode);
    markDirty(DirtyGeometry);
}

/*!
    \fn void QSGSimpleTextureNode::setRect(qreal x, qreal y, qreal w, qreal h)
    \overload

    Sets the rectangle of this texture node to begin at (\a x, \a y) and have
    width \a w and height \a h.
 */

/*!
    Returns the target rect of this texture node.
 */
QRectF QSGSimpleTextureNode::rect() const
{
    return m_rect;
}

/*!
    Sets the texture of this texture node to \a texture.

    \warning A texture node must have a texture before being added
    to the scenegraph to be rendered.
 */
void QSGSimpleTextureNode::setTexture(QSGTexture *texture)
{
    Q_ASSERT(texture);
    m_material.setTexture(texture);
    m_opaque_material.setTexture(texture);
    Q_D(QSGSimpleTextureNode);
    qsgsimpletexturenode_update(&m_geometry, texture, m_rect, d->m_texCoordMode);

    DirtyState dirty = DirtyMaterial;
    // It would be tempting to skip the extra bit here and instead use
    // m_material.texture to get the old state, but that texture could
    // have been deleted in the mean time.
    bool wasAtlas = d->isAtlasTexture;
    d->isAtlasTexture = texture->isAtlasTexture();
    if (wasAtlas || d->isAtlasTexture)
        dirty |= DirtyGeometry;
    markDirty(dirty);
}



/*!
    Returns the texture for this texture node
 */
QSGTexture *QSGSimpleTextureNode::texture() const
{
    return m_material.texture();
}

/*!
    \enum QSGSimpleTextureNode::TextureCoordinatesTransformFlag

    The TextureCoordinatesTransformFlag enum is used to specify the
    mode used to generate texture coordinates for a textured quad.

    \value NoTransform          Texture coordinates are oriented with window coordinates
                                i.e. with origin at top-left.

    \value MirrorHorizontally   Texture coordinates are inverted in the horizontal axis with
                                respect to window coordinates

    \value MirrorVertically     Texture coordinates are inverted in the vertical axis with
                                respect to window coordinates
 */

/*!
    Sets the method used to generate texture coordinates to \a mode. This can be used to obtain
    correct orientation of the texture. This is commonly needed when using a third party OpenGL
    library to render to texture as OpenGL has an inverted y-axis relative to Qt Quick.

    \sa textureCoordinatesTransform()
 */
void QSGSimpleTextureNode::setTextureCoordinatesTransform(QSGSimpleTextureNode::TextureCoordinatesTransformMode mode)
{
    Q_D(QSGSimpleTextureNode);
    if (d->m_texCoordMode == mode)
        return;
    d->m_texCoordMode = mode;
    qsgsimpletexturenode_update(&m_geometry, texture(), m_rect, d->m_texCoordMode);
    markDirty(DirtyMaterial);
}

/*!
    Returns the mode used to generate texture coordinates for this node.

    \sa setTextureCoordinatesTransform()
 */
QSGSimpleTextureNode::TextureCoordinatesTransformMode QSGSimpleTextureNode::textureCoordinatesTransform() const
{
    Q_D(const QSGSimpleTextureNode);
    return d->m_texCoordMode;
}

QT_END_NAMESPACE
