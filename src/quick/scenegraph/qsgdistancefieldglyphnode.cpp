// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgdistancefieldglyphnode_p.h"
#include "qsgdistancefieldglyphnode_p_p.h"
#include <QtQuick/private/qsgcontext_p.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcSgText, "qt.scenegraph.text")

// all SG glyph vertices and indices; only for qCDebug metrics
static std::atomic<qint64> s_totalAllocation = 0;

QSGDistanceFieldGlyphNode::QSGDistanceFieldGlyphNode(QSGRenderContext *context)
    : m_glyphNodeType(RootGlyphNode)
    , m_context(context)
    , m_material(nullptr)
    , m_glyph_cache(nullptr)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
    , m_style(QQuickText::Normal)
    , m_antialiasingMode(GrayAntialiasing)
    , m_texture(nullptr)
    , m_renderTypeQuality(-1)
    , m_dirtyGeometry(false)
    , m_dirtyMaterial(false)
{
    m_geometry.setDrawingMode(QSGGeometry::DrawTriangles);
    setGeometry(&m_geometry);
#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(this, QLatin1String("glyphs"));
#endif
}

QSGDistanceFieldGlyphNode::~QSGDistanceFieldGlyphNode()
{
    delete m_material;

    if (m_glyphNodeType == SubGlyphNode)
        return;

    if (m_glyph_cache) {
        m_glyph_cache->release(m_glyphs.glyphIndexes());
        m_glyph_cache->unregisterGlyphNode(this);
    }
}

void QSGDistanceFieldGlyphNode::setColor(const QColor &color)
{
    m_color = color;
    if (m_material != nullptr) {
        m_material->setColor(color);
        markDirty(DirtyMaterial);
    } else {
        m_dirtyMaterial = true;
    }
}

void QSGDistanceFieldGlyphNode::setRenderTypeQuality(int renderTypeQuality)
{
    if (renderTypeQuality == m_renderTypeQuality)
        return;

    m_renderTypeQuality = renderTypeQuality;
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

    m_dirtyGeometry = true;
    m_dirtyMaterial = true;
    setFlag(UsePreprocess);

    QSGDistanceFieldGlyphCache *oldCache = m_glyph_cache;
    m_glyph_cache = m_context->distanceFieldGlyphCache(m_glyphs.rawFont(), m_renderTypeQuality);

    if (m_glyphNodeType == SubGlyphNode)
        return;

    if (m_glyph_cache != oldCache) {
        if (oldCache) {
            oldCache->unregisterGlyphNode(this);
        }
        m_glyph_cache->registerGlyphNode(this);
    }
    if (m_glyph_cache)
        m_glyph_cache->populate(glyphs.glyphIndexes());

    const QVector<quint32> glyphIndexes = m_glyphs.glyphIndexes();
    for (int i = 0; i < glyphIndexes.size(); ++i)
        m_allGlyphIndexesLookup.insert(glyphIndexes.at(i));
    qCDebug(lcSgText, "inserting %" PRIdQSIZETYPE " glyphs, %" PRIdQSIZETYPE " unique",
            glyphIndexes.size(),
            m_allGlyphIndexesLookup.size());
#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(this, QString::number(glyphs.glyphIndexes().count()) + QStringLiteral(" DF glyphs: ") +
                            m_glyphs.rawFont().familyName() + QStringLiteral(" ") + QString::number(m_glyphs.rawFont().pixelSize()));
#endif
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
    if (m_dirtyGeometry)
        updateGeometry();

    setFlag(UsePreprocess, false);
}

void QSGDistanceFieldGlyphNode::invalidateGlyphs(const QVector<quint32> &glyphs)
{
    if (m_dirtyGeometry)
        return;

    for (int i = 0; i < glyphs.size(); ++i) {
        if (m_allGlyphIndexesLookup.contains(glyphs.at(i))) {
            m_dirtyGeometry = true;
            setFlag(UsePreprocess);
            return;
        }
    }
}

void QSGDistanceFieldGlyphNode::updateGeometry()
{
    if (!m_glyph_cache)
        return;

    // Remove previously created sub glyph nodes
    // We assume all the children are sub glyph nodes
    QSGNode *subnode = firstChild();
    QSGNode *nextNode = nullptr;
    while (subnode) {
        nextNode = subnode->nextSibling();
        delete subnode;
        subnode = nextNode;
    }

    QSGGeometry *g = geometry();

    Q_ASSERT(g->indexType() == QSGGeometry::UnsignedShortType);
    m_glyphsInOtherTextures.clear();

    const QVector<quint32> indexes = m_glyphs.glyphIndexes();
    const QVector<QPointF> positions = m_glyphs.positions();
    qreal fontPixelSize = m_glyphs.rawFont().pixelSize();

    // The template parameters here are assuming that most strings are short, 64
    // characters or less.
    QVarLengthArray<QSGGeometry::TexturedPoint2D, 256> vp;
    QVarLengthArray<ushort, 384> ip;
    const qsizetype maxIndexCount = (std::numeric_limits<quint16>::max() - 1) / 4; // 16383 (see below: 0xFFFF is not allowed)
    const qsizetype maxVertexCount = maxIndexCount * 4; // 65532
    const auto likelyGlyphCount = qMin(indexes.size(), maxIndexCount);
    vp.reserve(likelyGlyphCount * 4);
    ip.reserve(likelyGlyphCount * 6);

    qreal maxTexMargin = m_glyph_cache->distanceFieldRadius();
    qreal fontScale = m_glyph_cache->fontScale(fontPixelSize);
    qreal margin = 2;
    qreal texMargin = margin / fontScale;
    if (texMargin > maxTexMargin) {
        texMargin = maxTexMargin;
        margin = maxTexMargin * fontScale;
    }

    for (int i = 0; i < indexes.size(); ++i) {
        const int glyphIndex = indexes.at(i);
        QSGDistanceFieldGlyphCache::TexCoord c = m_glyph_cache->glyphTexCoord(glyphIndex);

        if (c.isNull())
            continue;

        const QPointF position = positions.at(i);

        const QSGDistanceFieldGlyphCache::Texture *texture = m_glyph_cache->glyphTexture(glyphIndex);
        if (texture->texture && !m_texture)
            m_texture = texture;

        // As we use UNSIGNED_SHORT indexing in the geometry, we overload the
        // "glyphsInOtherTextures" concept as overflow for if there are more
        // than 65532 vertices to render, which would otherwise exceed the
        // maximum index size. (leave 0xFFFF unused in order not to clash with
        // primitive restart) This will cause sub-nodes to be
        // created to handle any number of glyphs. But only the RootGlyphNode
        // needs to do this classification; from the perspective of a SubGlyphNode,
        // it's already done, and m_glyphs contains only pointers to ranges of
        // indices and positions that the RootGlyphNode is storing.
        if (m_texture != texture || vp.size() >= maxVertexCount) {
            if (m_glyphNodeType == RootGlyphNode && texture->texture) {
                GlyphInfo &glyphInfo = m_glyphsInOtherTextures[texture];
                glyphInfo.indexes.append(glyphIndex);
                glyphInfo.positions.append(position);
            } else if (vp.size() >= maxVertexCount && m_glyphNodeType == SubGlyphNode) {
                break; // out of this loop over indices, because we won't add any more vertices
            }
            continue;
        }

        QSGDistanceFieldGlyphCache::Metrics metrics = m_glyph_cache->glyphMetrics(glyphIndex, fontPixelSize);

        if (!metrics.isNull() && !c.isNull()) {
            metrics.width += margin * 2;
            metrics.height += margin * 2;
            metrics.baselineX -= margin;
            metrics.baselineY += margin;
            c.xMargin -= texMargin;
            c.yMargin -= texMargin;
            c.width += texMargin * 2;
            c.height += texMargin * 2;
        }

        qreal x = position.x() + metrics.baselineX + m_position.x();
        qreal y = position.y() - metrics.baselineY + m_position.y();

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
            m_baseLine = position;

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

    if (m_glyphNodeType == SubGlyphNode) {
        Q_ASSERT(m_glyphsInOtherTextures.isEmpty());
    } else {
        if (!m_glyphsInOtherTextures.isEmpty())
            qCDebug(lcSgText, "%" PRIdQSIZETYPE " 'other' textures", m_glyphsInOtherTextures.size());
        QHash<const QSGDistanceFieldGlyphCache::Texture *, GlyphInfo>::const_iterator ite = m_glyphsInOtherTextures.constBegin();
        while (ite != m_glyphsInOtherTextures.constEnd()) {
            QGlyphRun subNodeGlyphRun(m_glyphs);
            for (int i = 0; i < ite->indexes.size(); i += maxIndexCount) {
                int len = qMin(maxIndexCount, ite->indexes.size() - i);
                subNodeGlyphRun.setRawData(ite->indexes.constData() + i, ite->positions.constData() + i, len);
                qCDebug(lcSgText) << "subNodeGlyphRun has" << len << "positions:"
                                  << *(ite->positions.constData() + i) << "->" << *(ite->positions.constData() + i + len - 1);

                QSGDistanceFieldGlyphNode *subNode = new QSGDistanceFieldGlyphNode(m_context);
                subNode->setGlyphNodeType(SubGlyphNode);
                subNode->setColor(m_color);
                subNode->setStyle(m_style);
                subNode->setStyleColor(m_styleColor);
                subNode->setPreferredAntialiasingMode(m_antialiasingMode);
                subNode->setGlyphs(m_originalPosition, subNodeGlyphRun);
                subNode->update();
                subNode->updateGeometry(); // we have to explicitly call this now as preprocess won't be called before it's rendered
                appendChildNode(subNode);
            }
            ++ite;
        }
    }

    s_totalAllocation += vp.size() * sizeof(QSGGeometry::TexturedPoint2D) + ip.size() * sizeof(quint16);
    qCDebug(lcSgText) << "allocating for" << vp.size() << "vtx (reserved" << likelyGlyphCount * 4 << "):" << vp.size() * sizeof(QSGGeometry::TexturedPoint2D)
            << "bytes;" << ip.size() << "idx:" << ip.size() * sizeof(quint16) << "bytes; total bytes so far" << s_totalAllocation;
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
    if (m_glyph_cache)
        m_material->setFontScale(m_glyph_cache->fontScale(m_glyphs.rawFont().pixelSize()));
    m_material->setColor(m_color);
    setMaterial(m_material);
    m_dirtyMaterial = false;
}

QT_END_NAMESPACE
