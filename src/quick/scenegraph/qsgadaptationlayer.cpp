// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgadaptationlayer_p.h"

#include <qmath.h>
#include <QtQuick/private/qsgdistancefieldglyphnode_p.h>
#include <QtQuick/private/qsgcontext_p.h>
#include <private/qrawfont_p.h>
#include <QtGui/qguiapplication.h>
#include <qdir.h>
#include <qsgrendernode.h>

#include <private/qquickprofiler_p.h>
#include <QElapsedTimer>

#include <qtquick_tracepoints_p.h>

QT_BEGIN_NAMESPACE

Q_TRACE_POINT(qtquick, QSGDistanceFieldGlyphCache_update_entry, int count)
Q_TRACE_POINT(qtquick, QSGDistanceFieldGlyphCache_update_exit)
Q_TRACE_POINT(qtquick, QSGDistanceFieldGlyphCache_glyphRender_entry)
Q_TRACE_POINT(qtquick, QSGDistanceFieldGlyphCache_glyphRender_exit)
Q_TRACE_POINT(qtquick, QSGDistanceFieldGlyphCache_glyphStore_entry)
Q_TRACE_POINT(qtquick, QSGDistanceFieldGlyphCache_glyphStore_exit)

static QElapsedTimer qsg_render_timer;

QSGDistanceFieldGlyphCache::Texture QSGDistanceFieldGlyphCache::s_emptyTexture;

QSGDistanceFieldGlyphCache::QSGDistanceFieldGlyphCache(const QRawFont &font, int renderTypeQuality)
    : m_renderTypeQuality(renderTypeQuality)
    , m_pendingGlyphs(64)
{
    Q_ASSERT(font.isValid());

    QRawFontPrivate *fontD = QRawFontPrivate::get(font);
    m_glyphCount = fontD->fontEngine->glyphCount();

    m_doubleGlyphResolution = qt_fontHasNarrowOutlines(font) && m_glyphCount < QT_DISTANCEFIELD_HIGHGLYPHCOUNT();

    m_referenceFont = font;
    // we set the same pixel size as used by the distance field internally.
    // this allows us to call pathForGlyph once and reuse the result.
    m_referenceFont.setPixelSize(baseFontSize() * QT_DISTANCEFIELD_SCALE(m_doubleGlyphResolution));
    Q_ASSERT(m_referenceFont.isValid());
}

QSGDistanceFieldGlyphCache::~QSGDistanceFieldGlyphCache()
{
}

int QSGDistanceFieldGlyphCache::baseFontSize() const
{
    return m_renderTypeQuality > 0 ? m_renderTypeQuality : QT_DISTANCEFIELD_BASEFONTSIZE(m_doubleGlyphResolution);
}

QSGDistanceFieldGlyphCache::GlyphData &QSGDistanceFieldGlyphCache::emptyData(glyph_t glyph)
{
    GlyphData gd;
    gd.texture = &s_emptyTexture;
    QHash<glyph_t, GlyphData>::iterator it = m_glyphsData.insert(glyph, gd);
    return it.value();
}

QSGDistanceFieldGlyphCache::GlyphData &QSGDistanceFieldGlyphCache::glyphData(glyph_t glyph)
{
    QHash<glyph_t, GlyphData>::iterator data = m_glyphsData.find(glyph);
    if (data == m_glyphsData.end()) {
        GlyphData &gd = emptyData(glyph);
        gd.path = m_referenceFont.pathForGlyph(glyph);
        // need bounding rect in base font size scale
        qreal scaleFactor = qreal(1) / QT_DISTANCEFIELD_SCALE(m_doubleGlyphResolution);
        QTransform scaleDown;
        scaleDown.scale(scaleFactor, scaleFactor);
        gd.boundingRect = scaleDown.mapRect(gd.path.boundingRect());
        return gd;
    }
    return data.value();
}

QSGDistanceFieldGlyphCache::Metrics QSGDistanceFieldGlyphCache::glyphMetrics(glyph_t glyph, qreal pixelSize)
{
    GlyphData &gd = glyphData(glyph);
    qreal scale = fontScale(pixelSize);

    Metrics m;
    m.width = gd.boundingRect.width() * scale;
    m.height = gd.boundingRect.height() * scale;
    m.baselineX = gd.boundingRect.x() * scale;
    m.baselineY = -gd.boundingRect.y() * scale;

    return m;
}

void QSGDistanceFieldGlyphCache::populate(const QVector<glyph_t> &glyphs)
{
    QSet<glyph_t> referencedGlyphs;
    QSet<glyph_t> newGlyphs;
    int count = glyphs.size();
    for (int i = 0; i < count; ++i) {
        glyph_t glyphIndex = glyphs.at(i);
        if ((int) glyphIndex >= glyphCount() && glyphCount() > 0) {
            qWarning("Warning: distance-field glyph is not available with index %d", glyphIndex);
            continue;
        }

        GlyphData &gd = glyphData(glyphIndex);
        ++gd.ref;
        referencedGlyphs.insert(glyphIndex);

        if (gd.texCoord.isValid() || m_populatingGlyphs.contains(glyphIndex))
            continue;

        m_populatingGlyphs.insert(glyphIndex);

        if (gd.boundingRect.isEmpty()) {
            gd.texCoord.width = 0;
            gd.texCoord.height = 0;
        } else {
            newGlyphs.insert(glyphIndex);
        }
    }

    referenceGlyphs(referencedGlyphs);
    if (!newGlyphs.isEmpty())
        requestGlyphs(newGlyphs);
}

void QSGDistanceFieldGlyphCache::release(const QVector<glyph_t> &glyphs)
{
    QSet<glyph_t> unusedGlyphs;
    int count = glyphs.size();
    for (int i = 0; i < count; ++i) {
        glyph_t glyphIndex = glyphs.at(i);
        GlyphData &gd = glyphData(glyphIndex);
        if (--gd.ref == 0 && !gd.texCoord.isNull())
            unusedGlyphs.insert(glyphIndex);
    }
    releaseGlyphs(unusedGlyphs);
}

bool QSGDistanceFieldGlyphCache::isActive() const
{
    return true;
}

void QSGDistanceFieldGlyphCache::update()
{
    m_populatingGlyphs.clear();

    if (m_pendingGlyphs.isEmpty())
        return;

    Q_TRACE_SCOPE(QSGDistanceFieldGlyphCache_update, m_pendingGlyphs.size());

    bool profileFrames = QSG_LOG_TIME_GLYPH().isDebugEnabled();
    if (profileFrames)
        qsg_render_timer.start();
    Q_QUICK_SG_PROFILE_START(QQuickProfiler::SceneGraphAdaptationLayerFrame);
    Q_TRACE(QSGDistanceFieldGlyphCache_glyphRender_entry);

    QList<QDistanceField> distanceFields;
    const int pendingGlyphsSize = m_pendingGlyphs.size();
    distanceFields.reserve(pendingGlyphsSize);
    for (int i = 0; i < pendingGlyphsSize; ++i) {
        GlyphData &gd = glyphData(m_pendingGlyphs.at(i));
        distanceFields.append(QDistanceField(gd.path,
                                             m_pendingGlyphs.at(i),
                                             m_doubleGlyphResolution));
        gd.path = QPainterPath(); // no longer needed, so release memory used by the painter path
    }

    qint64 renderTime = 0;
    int count = m_pendingGlyphs.size();
    if (profileFrames)
        renderTime = qsg_render_timer.nsecsElapsed();

    Q_TRACE(QSGDistanceFieldGlyphCache_glyphRender_exit);
    Q_QUICK_SG_PROFILE_RECORD(QQuickProfiler::SceneGraphAdaptationLayerFrame,
                              QQuickProfiler::SceneGraphAdaptationLayerGlyphRender);
    Q_TRACE(QSGDistanceFieldGlyphCache_glyphStore_entry);

    m_pendingGlyphs.reset();

    storeGlyphs(distanceFields);

#if defined(QSG_DISTANCEFIELD_CACHE_DEBUG)
    for (Texture texture : std::as_const(m_textures))
        saveTexture(texture.texture, m_referenceFont.familyName());
#endif

    if (QSG_LOG_TIME_GLYPH().isDebugEnabled()) {
        quint64 now = qsg_render_timer.elapsed();
        qCDebug(QSG_LOG_TIME_GLYPH,
                "distancefield: %d glyphs prepared in %dms, rendering=%d, upload=%d",
                count,
                (int) now,
                int(renderTime / 1000000),
                int((now - (renderTime / 1000000))));
    }
    Q_TRACE(QSGDistanceFieldGlyphCache_glyphStore_exit);
    Q_QUICK_SG_PROFILE_END_WITH_PAYLOAD(QQuickProfiler::SceneGraphAdaptationLayerFrame,
                                        QQuickProfiler::SceneGraphAdaptationLayerGlyphStore,
                                        (qint64)count);
}

void QSGDistanceFieldGlyphCache::setGlyphsPosition(const QList<GlyphPosition> &glyphs)
{
    QVector<quint32> invalidatedGlyphs;

    int count = glyphs.size();
    for (int i = 0; i < count; ++i) {
        GlyphPosition glyph = glyphs.at(i);
        GlyphData &gd = glyphData(glyph.glyph);

        if (!gd.texCoord.isNull())
            invalidatedGlyphs.append(glyph.glyph);

        gd.texCoord.xMargin = QT_DISTANCEFIELD_RADIUS(m_doubleGlyphResolution) / qreal(QT_DISTANCEFIELD_SCALE(m_doubleGlyphResolution));
        gd.texCoord.yMargin = QT_DISTANCEFIELD_RADIUS(m_doubleGlyphResolution) / qreal(QT_DISTANCEFIELD_SCALE(m_doubleGlyphResolution));
        gd.texCoord.x = glyph.position.x();
        gd.texCoord.y = glyph.position.y();
        gd.texCoord.width = gd.boundingRect.width();
        gd.texCoord.height = gd.boundingRect.height();
    }

    if (!invalidatedGlyphs.isEmpty()) {
        for (QSGDistanceFieldGlyphConsumerList::iterator iter = m_registeredNodes.begin(); iter != m_registeredNodes.end(); ++iter) {
            iter->invalidateGlyphs(invalidatedGlyphs);
        }
    }
}

void QSGDistanceFieldGlyphCache::registerOwnerElement(QQuickItem *ownerElement)
{
    Q_UNUSED(ownerElement);
}

void QSGDistanceFieldGlyphCache::unregisterOwnerElement(QQuickItem *ownerElement)
{
    Q_UNUSED(ownerElement);
}

void QSGDistanceFieldGlyphCache::processPendingGlyphs()
{
    /* Intentionally empty */
}

void QSGDistanceFieldGlyphCache::setGlyphsTexture(const QVector<glyph_t> &glyphs, const Texture &tex)
{
    int i = m_textures.indexOf(tex);
    if (i == -1) {
        m_textures.append(tex);
        i = m_textures.size() - 1;
    } else {
        m_textures[i].size = tex.size;
    }
    Texture *texture = &(m_textures[i]);

    QVector<quint32> invalidatedGlyphs;

    int count = glyphs.size();
    for (int j = 0; j < count; ++j) {
        glyph_t glyphIndex = glyphs.at(j);
        GlyphData &gd = glyphData(glyphIndex);
        if (gd.texture != &s_emptyTexture)
            invalidatedGlyphs.append(glyphIndex);
        gd.texture = texture;
    }

    if (!invalidatedGlyphs.isEmpty()) {
        for (QSGDistanceFieldGlyphConsumerList::iterator iter = m_registeredNodes.begin(); iter != m_registeredNodes.end(); ++iter) {
            iter->invalidateGlyphs(invalidatedGlyphs);
        }
    }
}

void QSGDistanceFieldGlyphCache::markGlyphsToRender(const QVector<glyph_t> &glyphs)
{
    int count = glyphs.size();
    for (int i = 0; i < count; ++i)
        m_pendingGlyphs.add(glyphs.at(i));
}

void QSGDistanceFieldGlyphCache::updateRhiTexture(QRhiTexture *oldTex, QRhiTexture *newTex, const QSize &newTexSize)
{
    int count = m_textures.size();
    for (int i = 0; i < count; ++i) {
        Texture &tex = m_textures[i];
        if (tex.texture == oldTex) {
            tex.texture = newTex;
            tex.size = newTexSize;
            return;
        }
    }
}

QSGNodeVisitorEx::~QSGNodeVisitorEx()
    = default;

void QSGNodeVisitorEx::visitChildren(QSGNode *node)
{
    for (QSGNode *child = node->firstChild(); child; child = child->nextSibling()) {
        switch (child->type()) {
        case QSGNode::ClipNodeType: {
            QSGClipNode *c = static_cast<QSGClipNode*>(child);
            if (visit(c))
                visitChildren(c);
            endVisit(c);
            break;
        }
        case QSGNode::TransformNodeType: {
            QSGTransformNode *c = static_cast<QSGTransformNode*>(child);
            if (visit(c))
                visitChildren(c);
            endVisit(c);
            break;
        }
        case QSGNode::OpacityNodeType: {
            QSGOpacityNode *c = static_cast<QSGOpacityNode*>(child);
            if (visit(c))
                visitChildren(c);
            endVisit(c);
            break;
        }
        case QSGNode::GeometryNodeType: {
            if (child->flags() & QSGNode::IsVisitableNode) {
                QSGVisitableNode *v = static_cast<QSGVisitableNode*>(child);
                v->accept(this);
            } else {
                QSGGeometryNode *c = static_cast<QSGGeometryNode*>(child);
                if (visit(c))
                    visitChildren(c);
                endVisit(c);
            }
            break;
        }
        case QSGNode::RootNodeType: {
            QSGRootNode *root = static_cast<QSGRootNode*>(child);
            if (visit(root))
                visitChildren(root);
            endVisit(root);
            break;
        }
        case QSGNode::BasicNodeType: {
            visitChildren(child);
            break;
        }
        case QSGNode::RenderNodeType: {
            QSGRenderNode *r = static_cast<QSGRenderNode*>(child);
            if (visit(r))
                visitChildren(r);
            endVisit(r);
            break;
        }
        default:
            Q_UNREACHABLE();
            break;
        }
    }
}

QSGVisitableNode::~QSGVisitableNode()
    = default;

QSGInternalRectangleNode::~QSGInternalRectangleNode()
    = default;

QSGInternalImageNode::~QSGInternalImageNode()
    = default;

QSGPainterNode::~QSGPainterNode()
    = default;

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QSGGuiThreadShaderEffectManager::ShaderInfo::Variable &v)
{
    QDebugStateSaver saver(debug);
    debug.space();
    debug << v.name;
    switch (v.type) {
    case QSGGuiThreadShaderEffectManager::ShaderInfo::Constant:
        debug << "cvar" << "offset" << v.offset << "size" << v.size;
        break;
    case QSGGuiThreadShaderEffectManager::ShaderInfo::Sampler:
        debug << "sampler" << "bindpoint" << v.bindPoint;
        break;
    case QSGGuiThreadShaderEffectManager::ShaderInfo::Texture:
        debug << "texture" << "bindpoint" << v.bindPoint;
        break;
    default:
        break;
    }
    return debug;
}

QDebug operator<<(QDebug debug, const QSGShaderEffectNode::VariableData &vd)
{
    QDebugStateSaver saver(debug);
    debug.space();
    debug << vd.specialType;
    return debug;
}
#endif

/*!
    \internal
 */
QSGLayer::QSGLayer(QSGTexturePrivate &dd)
    : QSGDynamicTexture(dd)
{
}

QSGLayer::~QSGLayer()
    = default;

#if QT_CONFIG(quick_sprite)

QSGSpriteNode::~QSGSpriteNode()
    = default;

#endif

QSGGuiThreadShaderEffectManager::~QSGGuiThreadShaderEffectManager()
    = default;

QSGShaderEffectNode::~QSGShaderEffectNode()
    = default;

QSGGlyphNode::~QSGGlyphNode()
    = default;

QSGDistanceFieldGlyphConsumer::~QSGDistanceFieldGlyphConsumer()
    = default;

QT_END_NAMESPACE

#include "moc_qsgadaptationlayer_p.cpp"
