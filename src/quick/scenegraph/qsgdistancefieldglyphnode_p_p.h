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

#ifndef DISTANCEFIELDTEXTMATERIAL_H
#define DISTANCEFIELDTEXTMATERIAL_H

#include <QtQuick/qsgmaterial.h>
#include "qsgdistancefieldglyphnode_p.h"
#include "qsgadaptationlayer_p.h"

QT_BEGIN_NAMESPACE

class QSGDistanceFieldTextMaterial: public QSGMaterial
{
public:
    QSGDistanceFieldTextMaterial();
    ~QSGDistanceFieldTextMaterial();

    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;
    virtual int compare(const QSGMaterial *other) const;

    void setColor(const QColor &color) { m_color = color; }
    const QColor &color() const { return m_color; }

    void setGlyphCache(QSGDistanceFieldGlyphCache *a) { m_glyph_cache = a; }
    QSGDistanceFieldGlyphCache *glyphCache() const { return m_glyph_cache; }

    void setTexture(const QSGDistanceFieldGlyphCache::Texture * tex) { m_texture = tex; }
    const QSGDistanceFieldGlyphCache::Texture * texture() const { return m_texture; }

    QSize textureSize() const { return m_size; }

    bool updateTextureSize();

protected:
    QSize m_size;
    QColor m_color;
    QSGDistanceFieldGlyphCache *m_glyph_cache;
    const QSGDistanceFieldGlyphCache::Texture *m_texture;
};

class QSGDistanceFieldStyledTextMaterial : public QSGDistanceFieldTextMaterial
{
public:
    QSGDistanceFieldStyledTextMaterial();
    ~QSGDistanceFieldStyledTextMaterial();

    virtual QSGMaterialType *type() const = 0;
    virtual QSGMaterialShader *createShader() const = 0;
    virtual int compare(const QSGMaterial *other) const;

    void setStyleColor(const QColor &color) { m_styleColor = color; }
    const QColor &styleColor() const { return m_styleColor; }

protected:
    QColor m_styleColor;
};

class QSGDistanceFieldOutlineTextMaterial : public QSGDistanceFieldStyledTextMaterial
{
public:
    QSGDistanceFieldOutlineTextMaterial();
    ~QSGDistanceFieldOutlineTextMaterial();

    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;
};

class QSGDistanceFieldShiftedStyleTextMaterial : public QSGDistanceFieldStyledTextMaterial
{
public:
    QSGDistanceFieldShiftedStyleTextMaterial();
    ~QSGDistanceFieldShiftedStyleTextMaterial();

    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;

    void setShift(const QPointF &shift) { m_shift = shift; }
    const QPointF &shift() const { return m_shift; }

protected:
    QPointF m_shift;
};

class QSGHiQSubPixelDistanceFieldTextMaterial : public QSGDistanceFieldTextMaterial
{
public:
    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;
};

class QSGLoQSubPixelDistanceFieldTextMaterial : public QSGDistanceFieldTextMaterial
{
public:
    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;
};

QT_END_NAMESPACE

#endif // DISTANCEFIELDTEXTMATERIAL_H
