/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgcontext2dnode_p.h"

#include <private/qsgcontext_p.h>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE


QSGContext2DNode::QSGContext2DNode(QSGCanvasItem* item)
    : QSGGeometryNode()
    , m_item(item)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
    , m_texture(0)
    , m_size(1, 1)
    , m_dirtyGeometry(false)
    , m_dirtyTexture(false)
{
    setMaterial(&m_materialO);
    setOpaqueMaterial(&m_material);
    setGeometry(&m_geometry);
    setFlag(UsePreprocess, true);
}

QSGContext2DNode::~QSGContext2DNode()
{
}

void QSGContext2DNode::setSize(const QSizeF& size)
{
    if (m_size != size) {
        m_dirtyGeometry = true;
        m_size = size;
    }
}

void QSGContext2DNode::preprocess()
{
    bool doDirty = false;
    QSGDynamicTexture *t = qobject_cast<QSGDynamicTexture *>(m_material.texture());
    if (t) {
        doDirty = t->updateTexture();
    }
    if (doDirty) {
        m_dirtyTexture = true;
        markDirty(DirtyMaterial);
    }
}
void QSGContext2DNode::setTexture(QSGContext2DTexture* texture)
{
    if (texture != m_texture) {
        m_dirtyTexture = true;
        m_texture = texture;
    }
}

void QSGContext2DNode::update()
{
    if (m_dirtyGeometry)
        updateGeometry();
    if (m_dirtyTexture)
        updateTexture();

    m_dirtyGeometry = false;
    m_dirtyTexture = false;
}

void QSGContext2DNode::updateTexture()
{
    m_material.setTexture(m_texture);
    m_materialO.setTexture(m_texture);
    markDirty(DirtyMaterial);
}

void QSGContext2DNode::updateGeometry()
{
    QRectF source = m_texture->textureSubRect();
    QSGGeometry::updateTexturedRectGeometry(&m_geometry,
                                            QRectF(0, 0, m_size.width(), m_size.height()),
                                            source);
    markDirty(DirtyGeometry);
}
QT_END_NAMESPACE
