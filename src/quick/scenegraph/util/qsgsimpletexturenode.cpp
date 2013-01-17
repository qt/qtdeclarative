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


#include "qsgsimpletexturenode.h"

QT_BEGIN_NAMESPACE

static void qsgsimpletexturenode_update(QSGGeometry *g,
                                        QSGTexture *texture,
                                        const QRectF &rect)
{
    if (!texture)
        return;

    QSize ts = texture->textureSize();
    QRectF sourceRect(0, 0, ts.width(), ts.height());
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
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
{
    setGeometry(&m_geometry);
    setMaterial(&m_material);
    setOpaqueMaterial(&m_opaque_material);
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
    qsgsimpletexturenode_update(&m_geometry, texture(), m_rect);
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
    if (m_material.texture() == texture)
        return;
    m_material.setTexture(texture);
    m_opaque_material.setTexture(texture);
    qsgsimpletexturenode_update(&m_geometry, texture, m_rect);
    markDirty(DirtyMaterial);
}



/*!
    Returns the texture for this texture node
 */
QSGTexture *QSGSimpleTextureNode::texture() const
{
    return m_material.texture();
}

QT_END_NAMESPACE
