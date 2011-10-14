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

#include "qsgdistancefieldglyphnode_p.h"
#include "qsgdistancefieldglyphnode_p_p.h"
#include "qsgdistancefieldglyphcache_p.h"
#include <private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

QSGDistanceFieldGlyphNode::QSGDistanceFieldGlyphNode(QSGDistanceFieldGlyphCacheManager *cacheManager)
    : m_material(0)
    , m_glyph_cacheManager(cacheManager)
    , m_glyph_cache(0)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
    , m_style(QQuickText::Normal)
    , m_antialiasingMode(GrayAntialiasing)
    , m_dirtyFont(false)
    , m_dirtyGeometry(false)
    , m_dirtyMaterial(false)
{
    m_geometry.setDrawingMode(GL_TRIANGLES);
    setGeometry(&m_geometry);
    setPreferredAntialiasingMode(cacheManager->defaultAntialiasingMode());
#ifdef QML_RUNTIME_TESTING
    description = QLatin1String("glyphs");
#endif
}

QSGDistanceFieldGlyphNode::~QSGDistanceFieldGlyphNode()
{
    delete m_material;
    if (m_glyph_cache) {
        const QVector<quint32> &glyphIndexes = m_glyphs.glyphIndexes();
        m_glyph_cache->derefGlyphs(glyphIndexes.count(), glyphIndexes.constData());
    }
}

void QSGDistanceFieldGlyphNode::setColor(const QColor &color)
{
    m_color = color;
    if (m_material != 0) {
        m_material->setColor(color);
        setMaterial(m_material); // Indicate the material state has changed
    }
}

void QSGDistanceFieldGlyphNode::setPreferredAntialiasingMode(AntialiasingMode mode)
{
    if (mode == m_antialiasingMode)
        return;
    m_antialiasingMode = mode;
    m_dirtyMaterial = true;
}

void QSGDistanceFieldGlyphNode::setGlyphs(const QPointF &position, const QGlyphRun &glyphs)
{
    QRawFont font = glyphs.rawFont();
    m_position = QPointF(position.x(), position.y() - font.ascent());
    m_glyphs = glyphs;

    m_dirtyFont = true;
    m_dirtyGeometry = true;
    m_dirtyMaterial = true;
}

void QSGDistanceFieldGlyphNode::setStyle(QQuickText::TextStyle style)
{
    if (m_style == style)
        return;
    m_style = style;
    m_dirtyMaterial = true;
}

void QSGDistanceFieldGlyphNode::setStyleColor(const QColor &color)
{
    if (m_styleColor == color)
        return;
    m_styleColor = color;
    m_dirtyMaterial = true;
}

void QSGDistanceFieldGlyphNode::update()
{
    if (m_dirtyFont)
        updateFont();
    if (m_dirtyGeometry)
        updateGeometry();
    if (m_dirtyMaterial)
        updateMaterial();
}

void QSGDistanceFieldGlyphNode::updateGeometry()
{
    Q_ASSERT(m_glyph_cache);

    QSGGeometry *g = geometry();
    QRectF boundingRect;

    const QVector<quint32> &glyphIndexes = m_glyphs.glyphIndexes();

    m_glyph_cache->populate(glyphIndexes.count(), glyphIndexes.constData());

    Q_ASSERT(g->indexType() == GL_UNSIGNED_SHORT);

    int oldVertexCount = g->vertexCount();
    int oldIndexCount = g->indexCount();

    // We could potentially move the realloc part into the QSGGeometry object as a
    // grow() function...

    void *data = 0;
    if (oldVertexCount && oldIndexCount) {
        int byteSize = oldVertexCount * sizeof(QSGGeometry::TexturedPoint2D)
                     + oldIndexCount * sizeof(quint16);
        data = qMalloc(byteSize);
        memcpy(data, g->vertexData(), byteSize);
    }

    g->allocate(oldVertexCount + glyphIndexes.size() * 4, oldIndexCount + glyphIndexes.size() * 6);

    if (data) {
        memcpy(g->vertexData(), data, oldVertexCount * sizeof(QSGGeometry::TexturedPoint2D));
        memcpy(g->indexData(), ((char *) data) + oldVertexCount * sizeof(QSGGeometry::TexturedPoint2D),
               oldIndexCount * sizeof(quint16));
        qFree(data);
    }

    QSGGeometry::TexturedPoint2D *vp = g->vertexDataAsTexturedPoint2D() + oldVertexCount;
    ushort *ip = g->indexDataAsUShort() + oldIndexCount;

    QPointF margins(2, 2);
    QPointF texMargins = margins / m_glyph_cache->fontScale();

    QVector<QPointF> glyphPositions = m_glyphs.positions();
    for (int i = 0; i < glyphIndexes.size(); ++i) {
        quint32 glyphIndex = glyphIndexes.at(i);
        QSGDistanceFieldGlyphCache::Metrics metrics = m_glyph_cache->glyphMetrics(glyphIndex);
        QSGDistanceFieldGlyphCache::TexCoord c = m_glyph_cache->glyphTexCoord(glyphIndex);

        if (!metrics.isNull() && !c.isNull()) {
                metrics.width += margins.x() * 2;
                metrics.height += margins.y() * 2;
                metrics.baselineX -= margins.x();
                metrics.baselineY += margins.y();
                c.xMargin -= texMargins.x();
                c.yMargin -= texMargins.y();
                c.width += texMargins.x() * 2;
                c.height += texMargins.y() * 2;
        }

        const QPointF &glyphPosition = glyphPositions.at(i);
        qreal x = glyphPosition.x() + metrics.baselineX + m_position.x();
        qreal y = glyphPosition.y() - metrics.baselineY + m_position.y();

        boundingRect |= QRectF(x, y, metrics.width, metrics.height);

        float cx1 = x;
        float cx2 = x + metrics.width;
        float cy1 = y;
        float cy2 = y + metrics.height;

        float tx1 = c.x + c.xMargin;
        float tx2 = tx1 + c.width;
        float ty1 = c.y + c.yMargin;
        float ty2 = ty1 + c.height;

        if (m_baseLine.isNull())
            m_baseLine = glyphPosition;

        int vi = i & 1 ? (glyphIndexes.size() + 1) / 2 + i / 2 : i / 2;
        vp[4 * vi + 0].set(cx1, cy1, tx1, ty1);
        vp[4 * vi + 1].set(cx2, cy1, tx2, ty1);
        vp[4 * vi + 2].set(cx1, cy2, tx1, ty2);
        vp[4 * vi + 3].set(cx2, cy2, tx2, ty2);

        int o = i * 4 + oldVertexCount;
        ip[6 * i + 0] = o + 0;
        ip[6 * i + 1] = o + 2;
        ip[6 * i + 2] = o + 3;
        ip[6 * i + 3] = o + 3;
        ip[6 * i + 4] = o + 1;
        ip[6 * i + 5] = o + 0;
    }

//    printf("Vertices:\n");
//    for (int v=0; v<g->vertexCount(); ++v) {
//        QSGGeometry::TexturedPoint2D *t = g->vertexDataAsTexturedPoint2D() + v;
//        printf(" - %d -- %f %f  --  %.3f %.3f\n", v, t->x, t->y, t->tx, t->ty);
//    }

//    printf("Indices:\n");
//    for (int i=0; i<g->indexCount();) {

//        printf(" - %[ ", i);
//        printf("%d, ", g->indexDataAsUShort()[i++]);
//        printf("%d, ", g->indexDataAsUShort()[i++]);
//        printf("%d, ", g->indexDataAsUShort()[i++]);
//        printf("%d, ", g->indexDataAsUShort()[i++]);
//        printf("%d, ", g->indexDataAsUShort()[i++]);
//        printf("%d",   g->indexDataAsUShort()[i++]);
//        printf(" ]\n");
//    }

    setBoundingRect(boundingRect);
    markDirty(DirtyGeometry);
    m_dirtyGeometry = false;
}

void QSGDistanceFieldGlyphNode::updateFont()
{
    m_glyph_cache = m_glyph_cacheManager->cache(m_glyphs.rawFont());
    m_dirtyFont = false;
}

void QSGDistanceFieldGlyphNode::updateMaterial()
{
    delete m_material;

    if (m_style == QQuickText::Normal) {
        switch (m_antialiasingMode) {
        case HighQualitySubPixelAntialiasing:
            m_material = new QSGHiQSubPixelDistanceFieldTextMaterial;
            break;
        case LowQualitySubPixelAntialiasing:
            m_material = new QSGLoQSubPixelDistanceFieldTextMaterial;
            break;
        case GrayAntialiasing:
        default:
            m_material = new QSGDistanceFieldTextMaterial;
            break;
        }
    } else {
        QSGDistanceFieldStyledTextMaterial *material;
        if (m_style == QQuickText::Outline) {
            material = new QSGDistanceFieldOutlineTextMaterial;
        } else {
            QSGDistanceFieldShiftedStyleTextMaterial *sMaterial = new QSGDistanceFieldShiftedStyleTextMaterial;
            if (m_style == QQuickText::Raised)
                sMaterial->setShift(QPointF(0.0, 1.0));
            else
                sMaterial->setShift(QPointF(0.0, -1.0));
            material = sMaterial;
        }
        material->setStyleColor(m_styleColor);
        m_material = material;
    }

    m_material->setGlyphCache(m_glyph_cache);
    m_material->setColor(m_color);
    setMaterial(m_material);
    m_dirtyMaterial = false;
}

QT_END_NAMESPACE
