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

#ifndef QSGDEFAULTGLYPHNODE_P_P_H
#define QSGDEFAULTGLYPHNODE_P_P_H

#include <qcolor.h>
#include <QtGui/private/qopengltextureglyphcache_p.h>
#include <QtQuick/qsgmaterial.h>
#include <QtQuick/qsgtexture.h>
#include <QtQuick/qsggeometry.h>
#include <qshareddata.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <qrawfont.h>
#include <qmargins.h>

QT_BEGIN_NAMESPACE

class QFontEngine;
class Geometry;
class QSGTextMaskMaterial: public QSGMaterial
{
public:
    QSGTextMaskMaterial(const QRawFont &font,
                        QFontEngineGlyphCache::Type cacheType = QFontEngineGlyphCache::Raster_RGBMask);
    virtual ~QSGTextMaskMaterial();

    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;
    virtual int compare(const QSGMaterial *other) const;

    void setColor(const QColor &color) { m_color = color; }
    const QColor &color() const { return m_color; }

    QSGTexture *texture() const { return m_texture; }

    int cacheTextureWidth() const;
    int cacheTextureHeight() const;

    bool ensureUpToDate();

    QOpenGLTextureGlyphCache *glyphCache() const;
    void populate(const QPointF &position,
                  const QVector<quint32> &glyphIndexes, const QVector<QPointF> &glyphPositions,
                  QSGGeometry *geometry, QRectF *boundingRect, QPointF *baseLine,
                  const QMargins &margins = QMargins(0, 0, 0, 0));

private:
    void init();

    QSGPlainTexture *m_texture;
    QFontEngineGlyphCache::Type m_cacheType;
    QExplicitlySharedDataPointer<QFontEngineGlyphCache> m_glyphCache;
    QRawFont m_font;
    QColor m_color;
    QSize m_size;
};

class QSGStyledTextMaterial : public QSGTextMaskMaterial
{
public:
    QSGStyledTextMaterial(const QRawFont &font);
    virtual ~QSGStyledTextMaterial() { }

    void setStyleShift(const QPointF &shift) { m_styleShift = shift; }
    const QPointF &styleShift() const { return m_styleShift; }

    void setStyleColor(const QColor &color) { m_styleColor = color; }
    const QColor &styleColor() const { return m_styleColor; }

    virtual QSGMaterialType *type() const;
    virtual QSGMaterialShader *createShader() const;

    int compare(const QSGMaterial *other) const;

private:
    QPointF m_styleShift;
    QColor m_styleColor;
};

class QSGOutlinedTextMaterial : public QSGStyledTextMaterial
{
public:
    QSGOutlinedTextMaterial(const QRawFont &font);
    ~QSGOutlinedTextMaterial() { }

    QSGMaterialType *type() const;
    QSGMaterialShader *createShader() const;
};

QT_END_NAMESPACE

#endif
