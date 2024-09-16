// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgdefaultglyphnode_p_p.h"
#include <private/qsgmaterialshader_p.h>

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <private/qfontengine_p.h>

#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgdefaultrendercontext_p.h>

#include <private/qrawfont_p.h>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

static inline QVector4D qsg_premultiply(const QVector4D &c, float globalOpacity)
{
    float o = c.w() * globalOpacity;
    return QVector4D(c.x() * o, c.y() * o, c.z() * o, o);
}

class QSGTextMaskRhiShader : public QSGMaterialShader
{
public:
    QSGTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat, int viewCount);

    bool updateUniformData(RenderState &state,
                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

protected:
    QFontEngine::GlyphFormat m_glyphFormat;
    quint32 m_currentUbufOffset;
};

QSGTextMaskRhiShader::QSGTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat, int viewCount)
    : m_glyphFormat(glyphFormat)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/textmask.vert.qsb"), viewCount);
    setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/textmask.frag.qsb"), viewCount);
}

// uniform block layout:
//   mat4 modelViewMatrix
//   mat4 projectionMatrix or mat4 projectionMatrix[QSHADER_VIEW_COUNT]
//   vec2 textureScale
//   float dpr
//   vec4 color
// [styled/outline only]
//   vec4 styleColor
//   vec2 shift

bool QSGTextMaskRhiShader::updateUniformData(RenderState &state,
                                             QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_ASSERT(oldMaterial == nullptr || newMaterial->type() == oldMaterial->type());
    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);
    QSGTextMaskMaterial *oldMat = static_cast<QSGTextMaskMaterial *>(oldMaterial);

    // updateUniformData() is called before updateSampledImage() by the
    // renderer. Hence updating the glyph cache stuff here.
    const bool updated = mat->ensureUpToDate();
    Q_ASSERT(mat->texture());
    Q_ASSERT(oldMat == nullptr || oldMat->texture());

    bool changed = false;
    QByteArray *buf = state.uniformData();
    const int projectionMatrixCount = qMin(state.projectionMatrixCount(), newMaterial->viewCount());

    quint32 offset = 0; // ModelViewMatrix
    if (state.isMatrixDirty()) {
        const QMatrix4x4 mv = state.modelViewMatrix();
        memcpy(buf->data() + offset, mv.constData(), 64);
        changed = true;
    }
    offset += 64; // now at ProjectionMatrix or ProjectionMatrix[0]

    for (int viewIndex = 0; viewIndex < projectionMatrixCount; ++viewIndex) {
        if (state.isMatrixDirty()) {
            const QMatrix4x4 p = state.projectionMatrix(viewIndex);
            memcpy(buf->data() + offset, p.constData(), 64);
            changed = true;
        }
        offset += 64;
    }

    // offset is now at TextureScale
    QRhiTexture *oldRtex = oldMat ? oldMat->texture()->rhiTexture() : nullptr;
    QRhiTexture *newRtex = mat->texture()->rhiTexture();
    if (updated || !oldMat || oldRtex != newRtex) {
        const QVector2D textureScale = QVector2D(1.0f / mat->rhiGlyphCache()->width(),
                                                 1.0f / mat->rhiGlyphCache()->height());
        memcpy(buf->data() + offset, &textureScale, 8);
        changed = true;
    }
    offset += 8; // now at Dpr

    if (!oldMat) {
        float dpr = state.devicePixelRatio();
        memcpy(buf->data() + offset, &dpr, 4);
    }
    offset += 4 + 4; // now at Color (with padding to ensure vec4 alignment)

    // move texture uploads/copies onto the renderer's soon-to-be-committed list
    mat->rhiGlyphCache()->commitResourceUpdates(state.resourceUpdateBatch());

    m_currentUbufOffset = offset;
    return changed;
}

void QSGTextMaskRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                              QSGMaterial *newMaterial, QSGMaterial *)
{
    Q_UNUSED(state);
    if (binding != 1)
        return;

    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);
    QSGTexture *t = mat->texture();
    t->setFiltering(QSGTexture::Nearest);
    *texture = t;
}

class QSG8BitTextMaskRhiShader : public QSGTextMaskRhiShader
{
public:
    QSG8BitTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat, int viewCount, bool alphaTexture)
        : QSGTextMaskRhiShader(glyphFormat, viewCount)
    {
        if (alphaTexture)
            setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/8bittextmask_a.frag.qsb"), viewCount);
        else
            setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/8bittextmask.frag.qsb"), viewCount);
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

bool QSG8BitTextMaskRhiShader::updateUniformData(RenderState &state,
                                                 QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSGTextMaskRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);
    QSGTextMaskMaterial *oldMat = static_cast<QSGTextMaskMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();

    if (oldMat == nullptr || mat->color() != oldMat->color() || state.isOpacityDirty()) {
        const QVector4D color = qsg_premultiply(mat->color(), state.opacity());
        memcpy(buf->data() + m_currentUbufOffset, &color, 16);
        changed = true;
    }
    m_currentUbufOffset += 16; // now at StyleColor

    return changed;
}

class QSG24BitTextMaskRhiShader : public QSGTextMaskRhiShader
{
public:
    QSG24BitTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat, int viewCount)
        : QSGTextMaskRhiShader(glyphFormat, viewCount)
    {
        setFlag(UpdatesGraphicsPipelineState, true);
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/24bittextmask.frag.qsb"), viewCount);
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    bool updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                     QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

bool QSG24BitTextMaskRhiShader::updateUniformData(RenderState &state,
                                                  QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSGTextMaskRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);
    QSGTextMaskMaterial *oldMat = static_cast<QSGTextMaskMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();

    if (oldMat == nullptr || mat->color() != oldMat->color() || state.isOpacityDirty()) {
        // shader takes vec4 but uses alpha only; coloring happens via the blend constant
        const QVector4D color = qsg_premultiply(mat->color(), state.opacity());
        memcpy(buf->data() + m_currentUbufOffset, &color, 16);
        changed = true;
    }
    m_currentUbufOffset += 16; // now at StyleColor

    return changed;
}

bool QSG24BitTextMaskRhiShader::updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(state);
    Q_UNUSED(oldMaterial);
    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);

    ps->blendEnable = true;
    ps->srcColor = GraphicsPipelineState::ConstantColor;
    ps->dstColor = GraphicsPipelineState::OneMinusSrcColor;

    QVector4D color = mat->color();

    // this is dynamic state but it's - magic! - taken care of by the renderer
    ps->blendConstant = QColor::fromRgbF(color.x(), color.y(), color.z());

    return true;
}

class QSG32BitColorTextRhiShader : public QSGTextMaskRhiShader
{
public:
    QSG32BitColorTextRhiShader(QFontEngine::GlyphFormat glyphFormat, int viewCount)
        : QSGTextMaskRhiShader(glyphFormat, viewCount)
    {
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/32bitcolortext.frag.qsb"), viewCount);
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

bool QSG32BitColorTextRhiShader::updateUniformData(RenderState &state,
                                                   QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSGTextMaskRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);
    QSGTextMaskMaterial *oldMat = static_cast<QSGTextMaskMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();

    if (oldMat == nullptr || mat->color() != oldMat->color() || state.isOpacityDirty()) {
        // shader takes vec4 but uses alpha only
        const QVector4D color(0, 0, 0, mat->color().w() * state.opacity());
        memcpy(buf->data() + m_currentUbufOffset, &color, 16);
        changed = true;
    }
    m_currentUbufOffset += 16; // now at StyleColor

    return changed;
}

class QSGStyledTextRhiShader : public QSG8BitTextMaskRhiShader
{
public:
    QSGStyledTextRhiShader(QFontEngine::GlyphFormat glyphFormat, int viewCount, bool alphaTexture)
        : QSG8BitTextMaskRhiShader(glyphFormat, viewCount, alphaTexture)
    {
        setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/styledtext.vert.qsb"), viewCount);
        if (alphaTexture)
            setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/styledtext_a.frag.qsb"), viewCount);
        else
            setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/styledtext.frag.qsb"), viewCount);
    }

    bool updateUniformData(RenderState &state,
                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

bool QSGStyledTextRhiShader::updateUniformData(RenderState &state,
                                               QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSG8BitTextMaskRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    QSGStyledTextMaterial *mat = static_cast<QSGStyledTextMaterial *>(newMaterial);
    QSGStyledTextMaterial *oldMat = static_cast<QSGStyledTextMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();

    if (oldMat == nullptr || mat->styleColor() != oldMat->styleColor() || state.isOpacityDirty()) {
        const QVector4D styleColor = qsg_premultiply(mat->styleColor(), state.opacity());
        memcpy(buf->data() + m_currentUbufOffset, &styleColor, 16);
        changed = true;
    }
    m_currentUbufOffset += 16; // now at StyleShift

    if (oldMat == nullptr || oldMat->styleShift() != mat->styleShift()) {
        const QVector2D v = mat->styleShift();
        memcpy(buf->data() + m_currentUbufOffset, &v, 8);
        changed = true;
    }

    return changed;
}

class QSGOutlinedTextRhiShader : public QSGStyledTextRhiShader
{
public:
    QSGOutlinedTextRhiShader(QFontEngine::GlyphFormat glyphFormat, int viewCount, bool alphaTexture)
        : QSGStyledTextRhiShader(glyphFormat, viewCount, alphaTexture)
    {
        setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/outlinedtext.vert.qsb"), viewCount);
        if (alphaTexture)
            setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/outlinedtext_a.frag.qsb"), viewCount);
        else
            setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/outlinedtext.frag.qsb"), viewCount);
    }
};


// ***** common material stuff

QSGTextMaskMaterial::QSGTextMaskMaterial(QSGRenderContext *rc, const QVector4D &color, const QRawFont &font, QFontEngine::GlyphFormat glyphFormat)
    : m_rc(qobject_cast<QSGDefaultRenderContext *>(rc))
    , m_texture(nullptr)
    , m_glyphCache(nullptr)
    , m_font(font)
    , m_color(color)
{
    init(glyphFormat);
}

QSGTextMaskMaterial::~QSGTextMaskMaterial()
{
    if (m_retainedFontEngine != nullptr)
        m_rc->unregisterFontengineForCleanup(m_retainedFontEngine);
    delete m_texture;
}

void QSGTextMaskMaterial::setColor(const QVector4D &color)
{
    if (m_color == color)
        return;

    m_color = color;

    // If it is an RGB cache, then the pen color is actually part of the cache key
    // so it has to be updated
    if (m_glyphCache != nullptr && m_glyphCache->glyphFormat() == QFontEngine::Format_ARGB)
        updateCache(QFontEngine::Format_ARGB);
}

void QSGTextMaskMaterial::init(QFontEngine::GlyphFormat glyphFormat)
{
    Q_ASSERT(m_font.isValid());

    setFlag(Blending, true);

    Q_ASSERT(m_rc);
    m_rhi = m_rc->rhi();

    updateCache(glyphFormat);
}

void QSGTextMaskMaterial::updateCache(QFontEngine::GlyphFormat glyphFormat)
{
    // The following piece of code will read/write to the font engine's caches,
    // potentially from different threads. However, this is safe because this
    // code is only called from QQuickItem::updatePaintNode() which is called
    // only when the GUI is blocked, and multiple threads will call it in
    // sequence. See also QSGRenderContext::invalidate

    QRawFontPrivate *fontD = QRawFontPrivate::get(m_font);
    if (QFontEngine *fontEngine = fontD->fontEngine) {
        if (glyphFormat == QFontEngine::Format_None) {
            glyphFormat = fontEngine->glyphFormat != QFontEngine::Format_None
                        ? fontEngine->glyphFormat
                        : QFontEngine::Format_A32;
        }

        qreal devicePixelRatio;
        void *cacheKey;
        Q_ASSERT(m_rhi);
        Q_ASSERT(m_rc);
        cacheKey = m_rc;
        // Get the dpr the modern way. This value retrieved via the
        // rendercontext matches what RenderState::devicePixelRatio()
        // exposes to the material shaders later on.
        devicePixelRatio = m_rc->currentDevicePixelRatio();

        QTransform glyphCacheTransform = QTransform::fromScale(devicePixelRatio, devicePixelRatio);
        if (!fontEngine->supportsTransformation(glyphCacheTransform))
            glyphCacheTransform = QTransform();

        QColor color = glyphFormat == QFontEngine::Format_ARGB ? QColor::fromRgbF(m_color.x(), m_color.y(), m_color.z(), m_color.w()) : QColor();
        m_glyphCache = fontEngine->glyphCache(cacheKey, glyphFormat, glyphCacheTransform, color);
        if (!m_glyphCache || int(m_glyphCache->glyphFormat()) != glyphFormat) {
            m_glyphCache = new QSGRhiTextureGlyphCache(m_rc, glyphFormat, glyphCacheTransform, color);
            fontEngine->setGlyphCache(cacheKey, m_glyphCache.data());
            if (m_retainedFontEngine != nullptr)
                m_rc->unregisterFontengineForCleanup(m_retainedFontEngine);

            // Note: This is reference counted by the render context, so it will stay alive until
            // we release that reference
            m_retainedFontEngine = fontEngine;
            m_rc->registerFontengineForCleanup(fontEngine);
        }
    }
}

void QSGTextMaskMaterial::populate(const QPointF &p,
                                   const QVector<quint32> &glyphIndexes,
                                   const QVector<QPointF> &glyphPositions,
                                   QSGGeometry *geometry,
                                   QRectF *boundingRect,
                                   QPointF *baseLine,
                                   const QMargins &margins)
{
    Q_ASSERT(m_font.isValid());
    QPointF position(p.x(), p.y() - m_font.ascent());
    QVector<QFixedPoint> fixedPointPositions;
    const int glyphPositionsSize = glyphPositions.size();
    fixedPointPositions.reserve(glyphPositionsSize);
    for (int i=0; i < glyphPositionsSize; ++i)
        fixedPointPositions.append(QFixedPoint::fromPointF(position + glyphPositions.at(i)));

    QTextureGlyphCache *cache = glyphCache();

    QRawFontPrivate *fontD = QRawFontPrivate::get(m_font);
    cache->populate(fontD->fontEngine,
                    glyphIndexes.size(),
                    glyphIndexes.constData(),
                    fixedPointPositions.data(),
                    QPainter::RenderHints(),
                    true);
    cache->fillInPendingGlyphs();

    int margin = fontD->fontEngine->glyphMargin(cache->glyphFormat());

    qreal glyphCacheScaleX = cache->transform().m11();
    qreal glyphCacheScaleY = cache->transform().m22();
    qreal glyphCacheInverseScaleX = 1.0 / glyphCacheScaleX;
    qreal glyphCacheInverseScaleY = 1.0 / glyphCacheScaleY;
    qreal scaledMargin = margin * glyphCacheInverseScaleX;

    Q_ASSERT(geometry->indexType() ==  QSGGeometry::UnsignedShortType);
    geometry->allocate(glyphIndexes.size() * 4, glyphIndexes.size() * 6);
    QVector4D *vp = (QVector4D *)geometry->vertexDataAsTexturedPoint2D();
    Q_ASSERT(geometry->sizeOfVertex() == sizeof(QVector4D));
    ushort *ip = geometry->indexDataAsUShort();

    bool supportsSubPixelPositions = fontD->fontEngine->supportsHorizontalSubPixelPositions();
    for (int i=0; i<glyphIndexes.size(); ++i) {
         QPointF glyphPosition = glyphPositions.at(i) + position;
         QFixedPoint fixedPointPosition = fixedPointPositions.at(i);

         QFixed subPixelPosition;
         if (supportsSubPixelPositions)
             subPixelPosition = fontD->fontEngine->subPixelPositionForX(QFixed::fromReal(fixedPointPosition.x.toReal() * glyphCacheScaleX));

         QTextureGlyphCache::GlyphAndSubPixelPosition glyph(glyphIndexes.at(i),
                                                            QFixedPoint(subPixelPosition, 0));
         const QTextureGlyphCache::Coord &c = cache->coords.value(glyph);

         // On a retina screen the glyph positions are not pre-scaled (as opposed to
         // eg. the raster paint engine). To ensure that we get the same behavior as
         // the raster engine (and CoreText itself) when it comes to rounding of the
         // coordinates, we need to apply the scale factor before rounding, and then
         // apply the inverse scale to get back to the coordinate system of the node.

         qreal x = (qFloor(glyphPosition.x() * glyphCacheScaleX) * glyphCacheInverseScaleX) +
                        (c.baseLineX * glyphCacheInverseScaleX) - scaledMargin;
         qreal y = (qRound(glyphPosition.y() * glyphCacheScaleY) * glyphCacheInverseScaleY) -
                        (c.baseLineY * glyphCacheInverseScaleY) - scaledMargin;

         qreal w = c.w * glyphCacheInverseScaleX;
         qreal h = c.h * glyphCacheInverseScaleY;

         *boundingRect |= QRectF(x + scaledMargin, y + scaledMargin, w, h);

         float cx1 = x - margins.left();
         float cx2 = x + w + margins.right();
         float cy1 = y - margins.top();
         float cy2 = y + h + margins.bottom();

         float tx1 = c.x - margins.left();
         float tx2 = c.x + c.w + margins.right();
         float ty1 = c.y - margins.top();
         float ty2 = c.y + c.h + margins.bottom();

         if (baseLine->isNull())
             *baseLine = glyphPosition;

         vp[4 * i + 0] = QVector4D(cx1, cy1, tx1, ty1);
         vp[4 * i + 1] = QVector4D(cx2, cy1, tx2, ty1);
         vp[4 * i + 2] = QVector4D(cx1, cy2, tx1, ty2);
         vp[4 * i + 3] = QVector4D(cx2, cy2, tx2, ty2);

         int o = i * 4;
         ip[6 * i + 0] = o + 0;
         ip[6 * i + 1] = o + 2;
         ip[6 * i + 2] = o + 3;
         ip[6 * i + 3] = o + 3;
         ip[6 * i + 4] = o + 1;
         ip[6 * i + 5] = o + 0;
    }
}

QSGMaterialType *QSGTextMaskMaterial::type() const
{
    static QSGMaterialType argb, rgb, gray;
    switch (glyphCache()->glyphFormat()) {
    case QFontEngine::Format_ARGB:
        return &argb;
    case QFontEngine::Format_A32:
        return &rgb;
    case QFontEngine::Format_A8:
    default:
        return &gray;
    }
}

QTextureGlyphCache *QSGTextMaskMaterial::glyphCache() const
{
    return static_cast<QTextureGlyphCache *>(m_glyphCache.data());
}

QSGRhiTextureGlyphCache *QSGTextMaskMaterial::rhiGlyphCache() const
{
    return static_cast<QSGRhiTextureGlyphCache *>(glyphCache());
}

QSGMaterialShader *QSGTextMaskMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    QSGRhiTextureGlyphCache *gc = rhiGlyphCache();
    const QFontEngine::GlyphFormat glyphFormat = gc->glyphFormat();
    switch (glyphFormat) {
    case QFontEngine::Format_ARGB:
        return new QSG32BitColorTextRhiShader(glyphFormat, viewCount());
    case QFontEngine::Format_A32:
        return new QSG24BitTextMaskRhiShader(glyphFormat, viewCount());
    case QFontEngine::Format_A8:
    default:
        return new QSG8BitTextMaskRhiShader(glyphFormat, viewCount(), gc->eightBitFormatIsAlphaSwizzled());
    }
}

static inline int qsg_colorDiff(const QVector4D &a, const QVector4D &b)
{
    if (a.x() != b.x())
        return a.x() > b.x() ? 1 : -1;
    if (a.y() != b.y())
        return a.y() > b.y() ? 1 : -1;
    if (a.z() != b.z())
        return a.z() > b.z() ? 1 : -1;
    if (a.w() != b.w())
        return a.w() > b.w() ? 1 : -1;
    return 0;
}

int QSGTextMaskMaterial::compare(const QSGMaterial *o) const
{
    Q_ASSERT(o && type() == o->type());
    const QSGTextMaskMaterial *other = static_cast<const QSGTextMaskMaterial *>(o);
    if (m_glyphCache != other->m_glyphCache)
        return m_glyphCache.data() < other->m_glyphCache.data() ? -1 : 1;
    return qsg_colorDiff(m_color, other->m_color);
}

bool QSGTextMaskMaterial::ensureUpToDate()
{
    QSGRhiTextureGlyphCache *gc = rhiGlyphCache();
    QSize glyphCacheSize(gc->width(), gc->height());
    if (glyphCacheSize != m_size) {
        if (m_texture)
            delete m_texture;
        m_texture = new QSGPlainTexture;
        m_texture->setTexture(gc->texture());
        m_texture->setTextureSize(QSize(gc->width(), gc->height()));
        m_texture->setOwnsTexture(false);
        m_size = glyphCacheSize;
        return true;
    }
    return false;
}


QSGStyledTextMaterial::QSGStyledTextMaterial(QSGRenderContext *rc, const QRawFont &font)
    : QSGTextMaskMaterial(rc, QVector4D(), font, QFontEngine::Format_A8)
{
}

QSGMaterialType *QSGStyledTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGStyledTextMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    QSGRhiTextureGlyphCache *gc = rhiGlyphCache();
    return new QSGStyledTextRhiShader(gc->glyphFormat(), viewCount(), gc->eightBitFormatIsAlphaSwizzled());
}

int QSGStyledTextMaterial::compare(const QSGMaterial *o) const
{
    const QSGStyledTextMaterial *other = static_cast<const QSGStyledTextMaterial *>(o);

    if (m_styleShift != other->m_styleShift)
        return m_styleShift.y() - other->m_styleShift.y();

    int diff = qsg_colorDiff(m_styleColor, other->m_styleColor);
    if (diff == 0)
        return QSGTextMaskMaterial::compare(o);
    return diff;
}


QSGOutlinedTextMaterial::QSGOutlinedTextMaterial(QSGRenderContext *rc, const QRawFont &font)
    : QSGStyledTextMaterial(rc, font)
{
}

QSGMaterialType *QSGOutlinedTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGOutlinedTextMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    QSGRhiTextureGlyphCache *gc = rhiGlyphCache();
    return new QSGOutlinedTextRhiShader(gc->glyphFormat(), viewCount(), gc->eightBitFormatIsAlphaSwizzled());
}

QT_END_NAMESPACE
