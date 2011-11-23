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

#ifndef ADAPTATIONINTERFACES_H
#define ADAPTATIONINTERFACES_H

#include <QtQuick/qsgnode.h>
#include <QtQuick/qsgtexture.h>
#include <QtCore/qobject.h>
#include <QtCore/qrect.h>
#include <QtGui/qbrush.h>
#include <QtGui/qcolor.h>
#include <QtCore/qsharedpointer.h>
#include <QtGui/qglyphrun.h>
#include <QtCore/qurl.h>
#include <private/qfontengine_p.h>
#include <QtGui/private/qdatabuffer_p.h>
#include <private/qopenglcontext_p.h>

// ### remove
#include <QtQuick/private/qquicktext_p.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QSGNode;
class QImage;
class TextureReference;
class QSGDistanceFieldGlyphCacheManager;
class QSGDistanceFieldGlyphNode;

// TODO: Rename from XInterface to AbstractX.
class Q_QUICK_EXPORT QSGRectangleNode : public QSGGeometryNode
{
public:
    virtual void setRect(const QRectF &rect) = 0;
    virtual void setColor(const QColor &color) = 0;
    virtual void setPenColor(const QColor &color) = 0;
    virtual void setPenWidth(qreal width) = 0;
    virtual void setGradientStops(const QGradientStops &stops) = 0;
    virtual void setRadius(qreal radius) = 0;
    virtual void setAligned(bool aligned) = 0;

    virtual void update() = 0;
};


class Q_QUICK_EXPORT QSGImageNode : public QSGGeometryNode
{
public:
    virtual void setTargetRect(const QRectF &rect) = 0;
    virtual void setSourceRect(const QRectF &rect) = 0;
    virtual void setTexture(QSGTexture *texture) = 0;

    virtual void setMipmapFiltering(QSGTexture::Filtering filtering) = 0;
    virtual void setFiltering(QSGTexture::Filtering filtering) = 0;
    virtual void setHorizontalWrapMode(QSGTexture::WrapMode wrapMode) = 0;
    virtual void setVerticalWrapMode(QSGTexture::WrapMode wrapMode) = 0;

    virtual void update() = 0;
};


class Q_QUICK_EXPORT QSGGlyphNode : public QSGGeometryNode
{
public:
    enum AntialiasingMode
    {
        GrayAntialiasing,
        LowQualitySubPixelAntialiasing,
        HighQualitySubPixelAntialiasing
    };

    virtual void setGlyphs(const QPointF &position, const QGlyphRun &glyphs) = 0;
    virtual void setColor(const QColor &color) = 0;
    virtual void setStyle(QQuickText::TextStyle style) = 0;
    virtual void setStyleColor(const QColor &color) = 0;
    virtual QPointF baseLine() const = 0;

    virtual QRectF boundingRect() const { return m_bounding_rect; }
    virtual void setBoundingRect(const QRectF &bounds) { m_bounding_rect = bounds; }

    virtual void setPreferredAntialiasingMode(AntialiasingMode) = 0;

    virtual void update() = 0;

protected:
    QRectF m_bounding_rect;
};

class Q_QUICK_EXPORT QSGDistanceFieldGlyphCache
{
public:
    QSGDistanceFieldGlyphCache(QSGDistanceFieldGlyphCacheManager *man, QOpenGLContext *c, const QRawFont &font);
    virtual ~QSGDistanceFieldGlyphCache();

    struct Metrics {
        qreal width;
        qreal height;
        qreal baselineX;
        qreal baselineY;

        bool isNull() const { return width == 0 || height == 0; }
    };

    struct TexCoord {
        qreal x;
        qreal y;
        qreal width;
        qreal height;
        qreal xMargin;
        qreal yMargin;

        TexCoord() : x(0), y(0), width(-1), height(-1), xMargin(0), yMargin(0) { }

        bool isNull() const { return width <= 0 || height <= 0; }
        bool isValid() const { return width >= 0 && height >= 0; }
    };

    struct Texture {
        GLuint textureId;
        QSize size;

        Texture() : textureId(0), size(QSize()) { }
        bool operator == (const Texture &other) const { return textureId == other.textureId; }
    };

    const QSGDistanceFieldGlyphCacheManager *manager() const { return m_manager; }

    const QRawFont &font() const { return m_font; }

    qreal fontScale() const;
    int distanceFieldRadius() const;
    int glyphCount() const { return m_glyphCount; }
    bool doubleGlyphResolution() const { return m_cacheData->doubleGlyphResolution; }

    Metrics glyphMetrics(glyph_t glyph);
    TexCoord glyphTexCoord(glyph_t glyph) const;
    const Texture *glyphTexture(glyph_t glyph) const;

    void populate(const QVector<glyph_t> &glyphs);
    void release(const QVector<glyph_t> &glyphs);

    void update();

    void registerGlyphNode(QSGDistanceFieldGlyphNode *node);
    void unregisterGlyphNode(QSGDistanceFieldGlyphNode *node);

protected:
    struct GlyphPosition {
        glyph_t glyph;
        QPointF position;
    };

    virtual void requestGlyphs(const QVector<glyph_t> &glyphs) = 0;
    virtual void storeGlyphs(const QHash<glyph_t, QImage> &glyphs) = 0;
    virtual void releaseGlyphs(const QVector<glyph_t> &glyphs) = 0;

    void addGlyphPositions(const QList<GlyphPosition> &glyphs);
    void addGlyphTextures(const QVector<glyph_t> &glyphs, const Texture &tex);
    void markGlyphsToRender(const QVector<glyph_t> &glyphs);
    void removeGlyph(glyph_t glyph);

    void updateTexture(GLuint oldTex, GLuint newTex, const QSize &newTexSize);

    bool containsGlyph(glyph_t glyph) const;

    QOpenGLContext *ctx;

private:
    struct GlyphCacheData : public QOpenGLSharedResource {
        QList<Texture> textures;
        QHash<glyph_t, Texture*> glyphTextures;
        QHash<glyph_t, TexCoord> texCoords;
        QDataBuffer<glyph_t> pendingGlyphs;
        QHash<glyph_t, QPainterPath> glyphPaths;
        bool doubleGlyphResolution;
        QLinkedList<QSGDistanceFieldGlyphNode*> m_registeredNodes;

        GlyphCacheData(QOpenGLContext *ctx)
            : QOpenGLSharedResource(ctx->shareGroup())
            , pendingGlyphs(64)
            , doubleGlyphResolution(false)
        {}

        void invalidateResource()
        {
            textures.clear();
            glyphTextures.clear();
            texCoords.clear();
        }

        void freeResource(QOpenGLContext *)
        {
        }
    };

    QSGDistanceFieldGlyphCacheManager *m_manager;

    QRawFont m_font;
    QRawFont m_referenceFont;

    int m_glyphCount;
    QHash<glyph_t, Metrics> m_metrics;

    GlyphCacheData *cacheData();
    GlyphCacheData *m_cacheData;
    static QHash<QString, QOpenGLMultiGroupSharedResource> m_caches_data;
};


QT_END_NAMESPACE

QT_END_HEADER

#endif
