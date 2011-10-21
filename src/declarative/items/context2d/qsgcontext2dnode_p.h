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

#ifndef QSGCONTEXT2DNODE_P_H
#define QSGCONTEXT2DNODE_P_H

#include <qsgnode.h>
#include <qsgtexturematerial.h>

#include "qsgcanvasitem_p.h"
#include "qsgcontext2dtexture_p.h"
#include "qsgcontext2d_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGContext2DNode : public QSGGeometryNode
{
public:
    QSGContext2DNode(QSGCanvasItem* item);
    virtual ~QSGContext2DNode();
    void setTexture(QSGContext2DTexture* texture);
    void update();
    void preprocess();
    void setSize(const QSizeF& size);
private:
    void updateTexture();
    void updateGeometry();

    QSGCanvasItem* m_item;
    QSGOpaqueTextureMaterial m_material;
    QSGTextureMaterial m_materialO;
    QSGGeometry m_geometry;
    QSGContext2DTexture* m_texture;
    QSizeF m_size;

    bool m_dirtyGeometry;
    bool m_dirtyTexture;
};

QT_END_HEADER

QT_END_NAMESPACE

#endif // QSGCONTEXT2DNODE_P_H
