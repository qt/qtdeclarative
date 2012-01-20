/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgdistancefieldglyphnode_p.h"
#include "qsgdistancefieldglyphnode_p_p.h"
#include <QtQuick/private/qsgdistancefieldutil_p.h>
#include <QtQuick/private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

QSGDistanceFieldGlyphNode::QSGDistanceFieldGlyphNode(QSGDistanceFieldGlyphCacheManager *cacheManager)
    : m_material(0)
    , m_glyph_cacheManager(cacheManager)
    , m_glyph_cache(0)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
    , m_style(QQuickText::Normal)
    , m_antialiasingMode(GrayAntialiasing)
    , m_texture(0)
    , m_dirtyGeometry(false)
    , m_dirtyMaterial(false)
{
    setFlag(UsePreprocess);
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
        QVector<quint32> glyphIndexes;
        for (int i = 0; i < m_allGlyphs.count(); ++i)
            glyphIndexes.append(m_allGlyphs.at(i).glyphIndex);
        m_glyph_cache->release(glyphIndexes);
        m_glyph_cache->unregisterGlyphNode(this);
    }

    for (int i = 0; i < m_nodesToDelete.count(); ++i)
        delete m_nodesToDelete.at(i);
    m_nodesToDelete.clear();
}

void QSGDistanceFieldGlyphNode::setColor(const QColor &color)
{
    m_color = color;
    if (m_material != 0) {
        m_material->setColor(color);
        markDirty(DirtyMaterial);
    } else {
        m_dirtyMaterial = true;
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
    m_originalPosition = position;
    m_position = QPointF(position.x(), position.y() - font.ascent());
    m_glyphs = glyphs;

    QSGDistanceFieldGlyphCache *oldCache = m_glyph_cache;
    m_glyph_cache = m_glyph_cacheManager->cache(m_glyphs.rawFont());
    if (m_glyph_cache != oldCache) {
        if (oldCache)
            oldCache->unregisterGlyphNode(this);
        m_glyph_cache->registerGlyphNode(this);
    }
    m_glyph_cache->populate(glyphs.glyphIndexes());

    const QVector<quint32> &glyphIndexes = m_glyphs.glyphIndexes();
    const QVector<QPointF> &glyphPositions = m_glyphs.positions();
    for (int i = 0; i < glyphIndexes.count(); ++i) {
        GlyphInfo gi;
        gi.glyphIndex = glyphIndexes.at(i);
        gi.position = glyphPositions.at(i);
        m_allGlyphs.append(gi);
        m_allGlyphIndexesLookup.insert(gi.glyphIndex);
    }

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
    if (m_dirtyMaterial)
        updateMaterial();
}

void QSGDistanceFieldGlyphNode::preprocess()
{
    Q_ASSERT(m_glyph_cache);

    m_glyph_cache->update();

    for (int i = 0; i < m_nodesToDelete.count(); ++i)
        delete m_nodesToDelete.at(i);
    m_nodesToDelete.clear();

    if (m_dirtyGeometry)
        updateGeometry();
}

void QSGDistanceFieldGlyphNode::invalidateGlyphs(const QVector<quint32> &glyphs)
{
    if (m_dirtyGeometry)
        return;

    for (int i = 0; i < glyphs.count(); ++i) {
        if (m_allGlyphIndexesLookup.contains(glyphs.at(i))) {
            m_dirtyGeometry = true;
            return;
        }
    }
}

void QSGDistanceFieldGlyphNode::updateGeometry()
{
    Q_ASSERT(m_glyph_cache);

    // Remove previously created sub glyph nodes
    QHash<const QSGDistanceFieldGlyphCache::Texture *, QSGDistanceFieldGlyphNode *>::iterator it = m_subNodes.begin();
    while (it != m_subNodes.end()) {
        removeChildNode(it.value());
        // We can't delete the node now as it might be in the preprocess list
        // It will be deleted in the next preprocess
        m_nodesToDelete.append(it.value());
        it = m_subNodes.erase(it);
    }

    QSGGeometry *g = geometry();

    Q_ASSERT(g->indexType() == GL_UNSIGNED_SHORT);

    QHash<const QSGDistanceFieldGlyphCache::Texture *, QList<GlyphInfo> > glyphsInOtherTextures;

    QVector<QSGGeometry::TexturedPoint2D> vp;
    vp.reserve(m_allGlyphs.size() * 4);
    QVector<ushort> ip;
    ip.reserve(m_allGlyphs.size() * 6);

    QPointF margins(2, 2);
    QPointF texMargins = margins / m_glyph_cache->fontScale();

    for (int i = 0; i < m_allGlyphs.size(); ++i) {
        GlyphInfo glyphInfo = m_allGlyphs.at(i);
        QSGDistanceFieldGlyphCache::TexCoord c = m_glyph_cache->glyphTexCoord(glyphInfo.glyphIndex);

        if (c.isNull())
            continue;

        const QSGDistanceFieldGlyphCache::Texture *texture = m_glyph_cache->glyphTexture(glyphInfo.glyphIndex);
        if (texture->textureId && !m_texture)
            m_texture = texture;

        if (m_texture != texture) {
            if (texture->textureId)
                glyphsInOtherTextures[texture].append(glyphInfo);
            continue;
        }

        QSGDistanceFieldGlyphCache::Metrics metrics = m_glyph_cache->glyphMetrics(glyphInfo.glyphIndex);

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

        qreal x = glyphInfo.position.x() + metrics.baselineX + m_position.x();
        qreal y = glyphInfo.position.y() - metrics.baselineY + m_position.y();

        m_boundingRect |= QRectF(x, y, metrics.width, metrics.height);

        float cx1 = x;
        float cx2 = x + metrics.width;
        float cy1 = y;
        float cy2 = y + metrics.height;

        float tx1 = c.x + c.xMargin;
        float tx2 = tx1 + c.width;
        float ty1 = c.y + c.yMargin;
        float ty2 = ty1 + c.height;

        if (m_baseLine.isNull())
            m_baseLine = glyphInfo.position;

        int o = vp.size();

        QSGGeometry::TexturedPoint2D v1;
        v1.set(cx1, cy1, tx1, ty1);
        QSGGeometry::TexturedPoint2D v2;
        v2.set(cx2, cy1, tx2, ty1);
        QSGGeometry::TexturedPoint2D v3;
        v3.set(cx1, cy2, tx1, ty2);
        QSGGeometry::TexturedPoint2D v4;
        v4.set(cx2, cy2, tx2, ty2);
        vp.append(v1);
        vp.append(v2);
        vp.append(v3);
        vp.append(v4);

        ip.append(o + 0);
        ip.append(o + 2);
        ip.append(o + 3);
        ip.append(o + 3);
        ip.append(o + 1);
        ip.append(o + 0);
    }

    QHash<const QSGDistanceFieldGlyphCache::Texture *, QList<GlyphInfo> >::const_iterator ite = glyphsInOtherTextures.constBegin();
    while (ite != glyphsInOtherTextures.constEnd()) {
        QHash<const QSGDistanceFieldGlyphCache::Texture *, QSGDistanceFieldGlyphNode *>::iterator subIt = m_subNodes.find(ite.key());
        if (subIt == m_subNodes.end()) {
            QSGDistanceFieldGlyphNode *subNode = new QSGDistanceFieldGlyphNode(m_glyph_cacheManager);
            subNode->setColor(m_color);
            subNode->setStyle(m_style);
            subNode->setStyleColor(m_styleColor);
            subNode->update();
            appendChildNode(subNode);
            subIt = m_subNodes.insert(ite.key(), subNode);
        }

        QVector<quint32> glyphIndexes;
        QVector<QPointF> positions;
        const QList<GlyphInfo> &subNodeGlyphs = ite.value();
        for (int i = 0; i < subNodeGlyphs.count(); ++i) {
            const GlyphInfo &info = subNodeGlyphs.at(i);
            glyphIndexes.append(info.glyphIndex);
            positions.append(info.position);
        }
        QGlyphRun subNodeGlyphRun(m_glyphs);
        subNodeGlyphRun.setGlyphIndexes(glyphIndexes);
        subNodeGlyphRun.setPositions(positions);

        subIt.value()->setGlyphs(m_originalPosition, subNodeGlyphRun);
        subIt.value()->update();
        subIt.value()->updateGeometry(); // we have to explicity call this now as preprocess won't be called before it's rendered

        ++ite;
    }

    g->allocate(vp.size(), ip.size());
    memcpy(g->vertexDataAsTexturedPoint2D(), vp.constData(), vp.size() * sizeof(QSGGeometry::TexturedPoint2D));
    memcpy(g->indexDataAsUShort(), ip.constData(), ip.size() * sizeof(quint16));

    setBoundingRect(m_boundingRect);
    markDirty(DirtyGeometry);
    m_dirtyGeometry = false;

    m_material->setTexture(m_texture);
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
