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

#ifndef DISTANCEFIELDGLYPHCACHE_H
#define DISTANCEFIELDGLYPHCACHE_H

#include <qgl.h>
#include <qrawfont.h>
#include <private/qgl_p.h>
#include <private/qfont_p.h>
#include <private/qfontengine_p.h>
#include <QtGui/private/qdatabuffer_p.h>

QT_BEGIN_NAMESPACE

class QGLShaderProgram;

typedef float (*ThresholdFunc)(float glyphScale);
typedef float (*AntialiasingSpreadFunc)(float glyphScale);

class Q_DECLARATIVE_EXPORT QSGDistanceFieldGlyphCache : public QObject
{
    Q_OBJECT
public:
    ~QSGDistanceFieldGlyphCache();

    static QSGDistanceFieldGlyphCache *get(const QGLContext *ctx, const QRawFont &font);

    struct Metrics {
        qreal width;
        qreal height;
        qreal baselineX;
        qreal baselineY;

        bool isNull() const { return width == 0 || height == 0; }
    };
    Metrics glyphMetrics(glyph_t glyph);

    struct TexCoord {
        qreal x;
        qreal y;
        qreal width;
        qreal height;
        qreal xMargin;
        qreal yMargin;

        TexCoord() : x(0), y(0), width(0), height(0), xMargin(0), yMargin(0) { }

        bool isNull() const { return width == 0 || height == 0; }
    };
    TexCoord glyphTexCoord(glyph_t glyph);

    GLuint texture();
    QSize textureSize() const;
    int maxTextureSize() const;
    qreal fontScale() const;
    int distanceFieldRadius() const;
    QImage renderDistanceFieldGlyph(glyph_t glyph) const;

    int glyphCount() const;

    void populate(int count, const glyph_t *glyphs);
    void derefGlyphs(int count, const glyph_t *glyphs);
    void updateCache();

    bool cacheIsFull() const { return m_textureData->currY >= maxTextureSize(); }

    bool useWorkaroundBrokenFBOReadback() const;

    static bool distanceFieldEnabled();

    ThresholdFunc thresholdFunc() const { return m_threshold_func; }
    void setThresholdFunc(ThresholdFunc func) { m_threshold_func = func; }

    AntialiasingSpreadFunc antialiasingSpreadFunc() const { return m_antialiasingSpread_func; }
    void setAntialiasingSpreadFunc(AntialiasingSpreadFunc func) { m_antialiasingSpread_func = func; }

private Q_SLOTS:
    void onContextDestroyed(const QGLContext *context);

private:
    QSGDistanceFieldGlyphCache(const QGLContext *c, const QRawFont &font);

    void createTexture(int width, int height);
    void resizeTexture(int width, int height);

    static QHash<QPair<const QGLContext *, QFontEngine *>, QSGDistanceFieldGlyphCache *> m_caches;

    QRawFont m_font;
    QRawFont m_referenceFont;

    int m_glyphCount;
    QHash<glyph_t, Metrics> m_metrics;
    mutable int m_maxTextureSize;

    struct DistanceFieldTextureData {
        GLuint texture;
        GLuint fbo;
        QSize size;
        QHash<glyph_t, TexCoord> texCoords;
        QDataBuffer<glyph_t> pendingGlyphs;
        QHash<glyph_t, quint32> glyphRefCount;
        QSet<glyph_t> unusedGlyphs;
        int currX;
        int currY;
        QImage image;
        bool doubleGlyphResolution;

        DistanceFieldTextureData(const QGLContext *)
            : texture(0)
            , fbo(0)
            , pendingGlyphs(64)
            , currX(0)
            , currY(0)
            , doubleGlyphResolution(false)
        { }
    };
    DistanceFieldTextureData *textureData();
    DistanceFieldTextureData *m_textureData;
    static QHash<QFontEngine *, QGLContextGroupResource<DistanceFieldTextureData> > m_textures_data;

    const QGLContext *ctx;
    QGLShaderProgram *m_blitProgram;
    GLfloat m_vertexCoordinateArray[8];
    GLfloat m_textureCoordinateArray[8];

    ThresholdFunc m_threshold_func;
    AntialiasingSpreadFunc m_antialiasingSpread_func;
};

QT_END_NAMESPACE

#endif // DISTANCEFIELDGLYPHCACHE_H
