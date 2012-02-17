/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQUICKCONTEXT2DNODE_P_H
#define QQUICKCONTEXT2DNODE_P_H

#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgtexturematerial.h>

#include "qquickcanvasitem_p.h"
#include "qquickcontext2dtexture_p.h"
#include "qquickcontext2d_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE


class QSGDynamicTexture;

class QQuickCanvasItemNode : public QSGGeometryNode
{
public:
    QQuickCanvasItemNode();
    ~QQuickCanvasItemNode();

    void setTexture(QSGDynamicTexture *texture);
    void update();
    void setSize(const QSizeF& size);

private:
    void updateTexture();
    void updateGeometry();

    QSGOpaqueTextureMaterial m_material;
    QSGTextureMaterial m_materialO;
    QSGGeometry m_geometry;
    QSGTexture* m_texture;
    QSizeF m_size;

    bool m_dirtyGeometry;
    bool m_dirtyTexture;
};

QT_END_HEADER

QT_END_NAMESPACE

#endif // QQUICKCONTEXT2DNODE_P_H
