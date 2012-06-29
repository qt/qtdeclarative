/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
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


#ifndef DEFAULT_PIXMAPNODE_H
#define DEFAULT_PIXMAPNODE_H

#include <private/qsgadaptationlayer_p.h>
#include <QtQuick/qsgtexturematerial.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class SmoothTextureMaterial : public QSGTextureMaterial
{
public:
    SmoothTextureMaterial();

    void setTexture(QSGTexture *texture);

protected:
    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;
};

class QSGDefaultImageNode : public QSGImageNode
{
public:
    QSGDefaultImageNode();
    virtual void setTargetRect(const QRectF &rect);
    virtual void setInnerTargetRect(const QRectF &rect);
    virtual void setInnerSourceRect(const QRectF &rect);
    virtual void setSubSourceRect(const QRectF &rect);
    virtual void setTexture(QSGTexture *t);
    virtual void setAntialiasing(bool antialiasing);
    virtual void setMirror(bool mirror);
    virtual void update();

    virtual void setMipmapFiltering(QSGTexture::Filtering filtering);
    virtual void setFiltering(QSGTexture::Filtering filtering);
    virtual void setHorizontalWrapMode(QSGTexture::WrapMode wrapMode);
    virtual void setVerticalWrapMode(QSGTexture::WrapMode wrapMode);

    virtual void preprocess();

private:
    void updateGeometry();

    QRectF m_targetRect;
    QRectF m_innerTargetRect;
    QRectF m_innerSourceRect;
    QRectF m_subSourceRect;

    QSGOpaqueTextureMaterial m_material;
    QSGTextureMaterial m_materialO;
    SmoothTextureMaterial m_smoothMaterial;

    uint m_antialiasing : 1;
    uint m_mirror : 1;
    uint m_dirtyGeometry : 1;

    QSGGeometry m_geometry;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif
