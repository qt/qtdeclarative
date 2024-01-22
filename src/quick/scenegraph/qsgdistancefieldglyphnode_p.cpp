// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgdistancefieldglyphnode_p_p.h"
#include "qsgrhidistancefieldglyphcache_p.h"
#include <QtGui/qsurface.h>
#include <QtGui/qwindow.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE

static float qt_sg_envFloat(const char *name, float defaultValue)
{
    if (Q_LIKELY(!qEnvironmentVariableIsSet(name)))
        return defaultValue;
    bool ok = false;
    const float value = qgetenv(name).toFloat(&ok);
    return ok ? value : defaultValue;
}

static float thresholdFunc(float glyphScale)
{
    static const float base = qt_sg_envFloat("QT_DF_BASE", 0.5f);
    static const float baseDev = qt_sg_envFloat("QT_DF_BASEDEVIATION", 0.065f);
    static const float devScaleMin = qt_sg_envFloat("QT_DF_SCALEFORMAXDEV", 0.15f);
    static const float devScaleMax = qt_sg_envFloat("QT_DF_SCALEFORNODEV", 0.3f);
    return base - ((qBound(devScaleMin, glyphScale, devScaleMax) - devScaleMin) / (devScaleMax - devScaleMin) * -baseDev + baseDev);
}

static float spreadFunc(float glyphScale)
{
    static const float range = qt_sg_envFloat("QT_DF_RANGE", 0.06f);
    return range / glyphScale;
}

class QSGDistanceFieldTextMaterialRhiShader : public QSGMaterialShader
{
public:
    QSGDistanceFieldTextMaterialRhiShader(bool alphaTexture, int viewCount);

    bool updateUniformData(RenderState &state,
                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

protected:
    float m_fontScale = 1.0;
    float m_matrixScale = 1.0;
    quint32 m_currentUbufOffset;
};

QSGDistanceFieldTextMaterialRhiShader::QSGDistanceFieldTextMaterialRhiShader(bool alphaTexture, int viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldtext.vert.qsb"), viewCount);
    if (alphaTexture)
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldtext_a.frag.qsb"), viewCount);
    else
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldtext.frag.qsb"), viewCount);
}

bool QSGDistanceFieldTextMaterialRhiShader::updateUniformData(RenderState &state,
                                                              QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_ASSERT(oldMaterial == nullptr || newMaterial->type() == oldMaterial->type());
    QSGDistanceFieldTextMaterial *mat = static_cast<QSGDistanceFieldTextMaterial *>(newMaterial);
    QSGDistanceFieldTextMaterial *oldMat = static_cast<QSGDistanceFieldTextMaterial *>(oldMaterial);

    // updateUniformData() is called before updateSampledImage() by the
    // renderer. Hence updating the glyph cache stuff here.
    const bool textureUpdated = mat->updateTextureSizeAndWrapper();
    Q_ASSERT(mat->wrapperTexture());
    Q_ASSERT(oldMat == nullptr || oldMat->texture());

    bool changed = false;
    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 104);

    bool updateRange = false;
    if (!oldMat || mat->fontScale() != oldMat->fontScale()) {
        m_fontScale = mat->fontScale();
        updateRange = true;
    }
    if (state.isMatrixDirty()) {
        m_matrixScale = qSqrt(qAbs(state.determinant())) * state.devicePixelRatio();
        updateRange = true;
    }
    quint32 offset = 0;
    const int matrixCount = qMin(state.projectionMatrixCount(), newMaterial->viewCount());
    for (int viewIndex = 0; viewIndex < matrixCount; ++viewIndex) {
        if (state.isMatrixDirty()) {
            const QMatrix4x4 m = state.combinedMatrix(viewIndex);
            memcpy(buf->data() + 64 * viewIndex, m.constData(), 64);
            changed = true;
        }
        offset += 64;
    }
    if (textureUpdated || !oldMat || oldMat->texture()->texture != mat->texture()->texture) {
        const QVector2D ts(1.0f / mat->textureSize().width(), 1.0f / mat->textureSize().height());
        Q_ASSERT(sizeof(ts) == 8);
        memcpy(buf->data() + offset, &ts, 8);
        changed = true;
    }
    offset += 8 + 8; // 8 is padding for vec4 alignment
    if (!oldMat || mat->color() != oldMat->color() || state.isOpacityDirty()) {
        const QVector4D color = mat->color() * state.opacity();
        Q_ASSERT(sizeof(color) == 16);
        memcpy(buf->data() + offset, &color, 16);
        changed = true;
    }
    offset += 16;
    if (updateRange) { // deferred because depends on m_fontScale and m_matrixScale
        const float combinedScale = m_fontScale * m_matrixScale;
        const float base = thresholdFunc(combinedScale);
        const float range = spreadFunc(combinedScale);
        const QVector2D alphaMinMax(qMax(0.0f, base - range), qMin(base + range, 1.0f));
        memcpy(buf->data() + offset, &alphaMinMax, 8);
        changed = true;
    }
    offset += 8; // not adding any padding here since we are not sure what comes afterwards in the subclasses' shaders

    // move texture uploads/copies onto the renderer's soon-to-be-committed list
    static_cast<QSGRhiDistanceFieldGlyphCache *>(mat->glyphCache())->commitResourceUpdates(state.resourceUpdateBatch());

    m_currentUbufOffset = offset;
    return changed;
}

void QSGDistanceFieldTextMaterialRhiShader::updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                                                               QSGMaterial *newMaterial, QSGMaterial *)
{
    Q_UNUSED(state);
    if (binding != 1)
        return;

    QSGDistanceFieldTextMaterial *mat = static_cast<QSGDistanceFieldTextMaterial *>(newMaterial);
    QSGTexture *t = mat->wrapperTexture();
    t->setFiltering(QSGTexture::Linear);
    *texture = t;
}

class DistanceFieldAnisotropicTextMaterialRhiShader : public QSGDistanceFieldTextMaterialRhiShader
{
public:
    DistanceFieldAnisotropicTextMaterialRhiShader(bool alphaTexture, int viewCount);
};

DistanceFieldAnisotropicTextMaterialRhiShader::DistanceFieldAnisotropicTextMaterialRhiShader(bool alphaTexture, int viewCount)
    : QSGDistanceFieldTextMaterialRhiShader(alphaTexture, viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldtext.vert.qsb"), viewCount);
    if (alphaTexture)
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldtext_a_fwidth.frag.qsb"), viewCount);
    else
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldtext_fwidth.frag.qsb"), viewCount);
}

QSGDistanceFieldTextMaterial::QSGDistanceFieldTextMaterial()
    : m_glyph_cache(nullptr)
    , m_texture(nullptr)
    , m_fontScale(1.0)
    , m_sgTexture(nullptr)
{
   setFlag(Blending | RequiresDeterminant, true);
}

QSGDistanceFieldTextMaterial::~QSGDistanceFieldTextMaterial()
{
    delete m_sgTexture;
}

QSGMaterialType *QSGDistanceFieldTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

void QSGDistanceFieldTextMaterial::setColor(const QColor &color)
{
    float r, g, b, a;
    color.getRgbF(&r, &g, &b, &a);
    m_color = QVector4D(r * a, g * a, b * a, a);
}

QSGMaterialShader *QSGDistanceFieldTextMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    if (renderMode == QSGRendererInterface::RenderMode3D && m_glyph_cache->screenSpaceDerivativesSupported())
        return new DistanceFieldAnisotropicTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled(), viewCount());
    else
        return new QSGDistanceFieldTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled(), viewCount());
}

bool QSGDistanceFieldTextMaterial::updateTextureSize()
{
    if (!m_texture)
        m_texture = m_glyph_cache->glyphTexture(0); // invalid texture

    if (m_texture->size != m_size) {
        m_size = m_texture->size;
        return true;
    }

    return false;
}

// When using the RHI we need a QSGTexture wrapping the QRhiTexture, just
// exposing a QRhiTexture * (which would be the equivalent of GLuint textureId)
// is not sufficient to play nice with the material.
bool QSGDistanceFieldTextMaterial::updateTextureSizeAndWrapper()
{
    bool updated = updateTextureSize();
    if (updated) {
        if (m_sgTexture)
            delete m_sgTexture;
        m_sgTexture = new QSGPlainTexture;
        m_sgTexture->setTexture(m_texture->texture);
        m_sgTexture->setTextureSize(m_size);
        m_sgTexture->setOwnsTexture(false);
    }
    return updated;
}

int QSGDistanceFieldTextMaterial::compare(const QSGMaterial *o) const
{
    Q_ASSERT(o && type() == o->type());
    const QSGDistanceFieldTextMaterial *other = static_cast<const QSGDistanceFieldTextMaterial *>(o);
    if (m_glyph_cache != other->m_glyph_cache)
        return m_glyph_cache - other->m_glyph_cache;
    if (m_fontScale != other->m_fontScale) {
        return int(other->m_fontScale < m_fontScale) - int(m_fontScale < other->m_fontScale);
    }
    if (m_color != other->m_color)
        return &m_color < &other->m_color ? -1 : 1;
    qintptr t0 = m_texture ? qintptr(m_texture->texture) : 0;
    qintptr t1 = other->m_texture ? qintptr(other->m_texture->texture) : 0;
    const qintptr diff = t0 - t1;
    return diff < 0 ? -1 : (diff > 0 ? 1 : 0);
}
class DistanceFieldStyledTextMaterialRhiShader : public QSGDistanceFieldTextMaterialRhiShader
{
public:
    DistanceFieldStyledTextMaterialRhiShader(bool alphaTexture, int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

DistanceFieldStyledTextMaterialRhiShader::DistanceFieldStyledTextMaterialRhiShader(bool alphaTexture, int viewCount)
    : QSGDistanceFieldTextMaterialRhiShader(alphaTexture, viewCount)
{
}

bool DistanceFieldStyledTextMaterialRhiShader::updateUniformData(RenderState &state,
                                                                 QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSGDistanceFieldTextMaterialRhiShader::updateUniformData(state, newMaterial, oldMaterial);
    QSGDistanceFieldStyledTextMaterial *mat = static_cast<QSGDistanceFieldStyledTextMaterial *>(newMaterial);
    QSGDistanceFieldStyledTextMaterial *oldMat = static_cast<QSGDistanceFieldStyledTextMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 128);

    // must add 8 bytes padding for vec4 alignment, the base class did not do this
    m_currentUbufOffset += 8; // now at StyleColor

    if (!oldMat || mat->styleColor() != oldMat->styleColor() || state.isOpacityDirty()) {
        QVector4D styleColor = mat->styleColor();
        styleColor *= state.opacity();

        memcpy(buf->data() + m_currentUbufOffset, &styleColor, 16);
        changed = true;
    }
    m_currentUbufOffset += 16;

    return changed;
}

QSGDistanceFieldStyledTextMaterial::QSGDistanceFieldStyledTextMaterial()
    : QSGDistanceFieldTextMaterial()
{
}

QSGDistanceFieldStyledTextMaterial::~QSGDistanceFieldStyledTextMaterial()
{
}

void QSGDistanceFieldStyledTextMaterial::setStyleColor(const QColor &color)
{
    float r, g, b, a;
    color.getRgbF(&r, &g, &b, &a);
    m_styleColor = QVector4D(r * a, g * a, b * a, a);
}

int QSGDistanceFieldStyledTextMaterial::compare(const QSGMaterial *o) const
{
    Q_ASSERT(o && type() == o->type());
    const QSGDistanceFieldStyledTextMaterial *other = static_cast<const QSGDistanceFieldStyledTextMaterial *>(o);
    if (m_styleColor != other->m_styleColor)
        return &m_styleColor < &other->m_styleColor ? -1 : 1;
    return QSGDistanceFieldTextMaterial::compare(o);
}

class DistanceFieldOutlineTextMaterialRhiShader : public DistanceFieldStyledTextMaterialRhiShader
{
public:
    DistanceFieldOutlineTextMaterialRhiShader(bool alphaTexture, int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

DistanceFieldOutlineTextMaterialRhiShader::DistanceFieldOutlineTextMaterialRhiShader(bool alphaTexture, int viewCount)
    : DistanceFieldStyledTextMaterialRhiShader(alphaTexture, viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldoutlinetext.vert.qsb"), viewCount);
    if (alphaTexture)
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldoutlinetext_a.frag.qsb"), viewCount);
    else
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldoutlinetext.frag.qsb"), viewCount);
}

class DistanceFieldAnisotropicOutlineTextMaterialRhiShader : public DistanceFieldOutlineTextMaterialRhiShader
{
public:
    DistanceFieldAnisotropicOutlineTextMaterialRhiShader(bool alphaTexture, int viewCount);
};

DistanceFieldAnisotropicOutlineTextMaterialRhiShader::DistanceFieldAnisotropicOutlineTextMaterialRhiShader(bool alphaTexture, int viewCount)
    : DistanceFieldOutlineTextMaterialRhiShader(alphaTexture, viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldoutlinetext.vert.qsb"), viewCount);
    if (alphaTexture)
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldoutlinetext_a_fwidth.frag.qsb"), viewCount);
    else
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldoutlinetext_fwidth.frag.qsb"), viewCount);
}

bool DistanceFieldOutlineTextMaterialRhiShader::updateUniformData(RenderState &state,
                                                                  QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = DistanceFieldStyledTextMaterialRhiShader::updateUniformData(state, newMaterial, oldMaterial);
    QSGDistanceFieldOutlineTextMaterial *mat = static_cast<QSGDistanceFieldOutlineTextMaterial *>(newMaterial);
    QSGDistanceFieldOutlineTextMaterial *oldMat = static_cast<QSGDistanceFieldOutlineTextMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 136);

    if (!oldMat || mat->fontScale() != oldMat->fontScale() || state.isMatrixDirty()) {
        float dfRadius = mat->glyphCache()->distanceFieldRadius();
        float combinedScale = m_fontScale * m_matrixScale;
        float base = thresholdFunc(combinedScale);
        float range = spreadFunc(combinedScale);
        float outlineLimit = qMax(0.2f, base - 0.5f / dfRadius / m_fontScale);
        float alphaMin = qMax(0.0f, base - range);
        float styleAlphaMin0 = qMax(0.0f, outlineLimit - range);
        float styleAlphaMin1 = qMin(outlineLimit + range, alphaMin);
        memcpy(buf->data() + m_currentUbufOffset, &styleAlphaMin0, 4);
        memcpy(buf->data() + m_currentUbufOffset + 4, &styleAlphaMin1, 4);
        changed = true;
    }

    return changed;
}

QSGDistanceFieldOutlineTextMaterial::QSGDistanceFieldOutlineTextMaterial()
    : QSGDistanceFieldStyledTextMaterial()
{
}

QSGDistanceFieldOutlineTextMaterial::~QSGDistanceFieldOutlineTextMaterial()
{
}

QSGMaterialType *QSGDistanceFieldOutlineTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGDistanceFieldOutlineTextMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    if (renderMode == QSGRendererInterface::RenderMode3D && m_glyph_cache->screenSpaceDerivativesSupported())
        return new DistanceFieldAnisotropicOutlineTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled(), viewCount());
    else
        return new DistanceFieldOutlineTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled(), viewCount());
}

class DistanceFieldShiftedStyleTextMaterialRhiShader : public DistanceFieldStyledTextMaterialRhiShader
{
public:
    DistanceFieldShiftedStyleTextMaterialRhiShader(bool alphaTexture, int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

DistanceFieldShiftedStyleTextMaterialRhiShader::DistanceFieldShiftedStyleTextMaterialRhiShader(bool alphaTexture, int viewCount)
    : DistanceFieldStyledTextMaterialRhiShader(alphaTexture, viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldshiftedtext.vert.qsb"), viewCount);
    if (alphaTexture)
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldshiftedtext_a.frag.qsb"), viewCount);
    else
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldshiftedtext.frag.qsb"), viewCount);
}

bool DistanceFieldShiftedStyleTextMaterialRhiShader::updateUniformData(RenderState &state,
                                                                       QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = DistanceFieldStyledTextMaterialRhiShader::updateUniformData(state, newMaterial, oldMaterial);
    QSGDistanceFieldShiftedStyleTextMaterial *mat = static_cast<QSGDistanceFieldShiftedStyleTextMaterial *>(newMaterial);
    QSGDistanceFieldShiftedStyleTextMaterial *oldMat = static_cast<QSGDistanceFieldShiftedStyleTextMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 136);

    if (!oldMat || oldMat->fontScale() != mat->fontScale() || oldMat->shift() != mat->shift()
            || oldMat->textureSize() != mat->textureSize())
    {
        QVector2D shift(1.0 / mat->fontScale() * mat->shift().x(),
                        1.0 / mat->fontScale() * mat->shift().y());
        memcpy(buf->data() + m_currentUbufOffset, &shift, 8);
        changed = true;
    }

    return changed;
}

class DistanceFieldAnisotropicShiftedTextMaterialRhiShader : public DistanceFieldShiftedStyleTextMaterialRhiShader
{
public:
    DistanceFieldAnisotropicShiftedTextMaterialRhiShader(bool alphaTexture, int viewCount);
};

DistanceFieldAnisotropicShiftedTextMaterialRhiShader::DistanceFieldAnisotropicShiftedTextMaterialRhiShader(bool alphaTexture, int viewCount)
    : DistanceFieldShiftedStyleTextMaterialRhiShader(alphaTexture, viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldshiftedtext.vert.qsb"), viewCount);
    if (alphaTexture)
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldshiftedtext_a_fwidth.frag.qsb"), viewCount);
    else
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/distancefieldshiftedtext_fwidth.frag.qsb"), viewCount);
}

QSGDistanceFieldShiftedStyleTextMaterial::QSGDistanceFieldShiftedStyleTextMaterial()
    : QSGDistanceFieldStyledTextMaterial()
{
}

QSGDistanceFieldShiftedStyleTextMaterial::~QSGDistanceFieldShiftedStyleTextMaterial()
{
}

QSGMaterialType *QSGDistanceFieldShiftedStyleTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGDistanceFieldShiftedStyleTextMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    if (renderMode == QSGRendererInterface::RenderMode3D && m_glyph_cache->screenSpaceDerivativesSupported())
        return new DistanceFieldAnisotropicShiftedTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled(), viewCount());
    else
        return new DistanceFieldShiftedStyleTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled(), viewCount());
}

int QSGDistanceFieldShiftedStyleTextMaterial::compare(const QSGMaterial *o) const
{
    const QSGDistanceFieldShiftedStyleTextMaterial *other = static_cast<const QSGDistanceFieldShiftedStyleTextMaterial *>(o);
    if (m_shift != other->m_shift)
        return &m_shift < &other->m_shift ? -1 : 1;
    return QSGDistanceFieldStyledTextMaterial::compare(o);
}

class QSGHiQSubPixelDistanceFieldTextMaterialRhiShader : public QSGDistanceFieldTextMaterialRhiShader
{
public:
    QSGHiQSubPixelDistanceFieldTextMaterialRhiShader(bool alphaTexture, int viewCount);

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    bool updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                     QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

QSGHiQSubPixelDistanceFieldTextMaterialRhiShader::QSGHiQSubPixelDistanceFieldTextMaterialRhiShader(bool alphaTexture, int viewCount)
    : QSGDistanceFieldTextMaterialRhiShader(alphaTexture, viewCount)
{
    setFlag(UpdatesGraphicsPipelineState, true);
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/hiqsubpixeldistancefieldtext.vert.qsb"), viewCount);
    if (alphaTexture)
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/hiqsubpixeldistancefieldtext_a.frag.qsb"), viewCount);
    else
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/hiqsubpixeldistancefieldtext.frag.qsb"), viewCount);
}

bool QSGHiQSubPixelDistanceFieldTextMaterialRhiShader::updateUniformData(RenderState &state,
                                                                         QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSGDistanceFieldTextMaterialRhiShader::updateUniformData(state, newMaterial, oldMaterial);
    QSGHiQSubPixelDistanceFieldTextMaterial *mat = static_cast<QSGHiQSubPixelDistanceFieldTextMaterial *>(newMaterial);
    QSGHiQSubPixelDistanceFieldTextMaterial *oldMat = static_cast<QSGHiQSubPixelDistanceFieldTextMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 128);

    if (!oldMat || mat->fontScale() != oldMat->fontScale()) {
        float fontScale = mat->fontScale();
        memcpy(buf->data() + m_currentUbufOffset, &fontScale, 4);
        changed = true;
    }
    m_currentUbufOffset += 4 + 4; // 4 for padding for vec2 alignment

    if (!oldMat || state.isMatrixDirty()) {
        int viewportWidth = state.viewportRect().width();
        QMatrix4x4 mat = state.combinedMatrix().inverted();
        QVector4D vecDelta = mat.column(0) * (qreal(2) / viewportWidth);
        memcpy(buf->data() + m_currentUbufOffset, &vecDelta, 16);
    }

    return changed;
}

bool QSGHiQSubPixelDistanceFieldTextMaterialRhiShader::updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                                                                   QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    Q_UNUSED(state);
    Q_UNUSED(oldMaterial);
    QSGHiQSubPixelDistanceFieldTextMaterial *mat = static_cast<QSGHiQSubPixelDistanceFieldTextMaterial *>(newMaterial);

    ps->blendEnable = true;
    ps->srcColor = GraphicsPipelineState::ConstantColor;
    ps->dstColor = GraphicsPipelineState::OneMinusSrcColor;

    const QVector4D color = mat->color();
    // this is dynamic state but it's - magic! - taken care of by the renderer
    ps->blendConstant = QColor::fromRgbF(color.x(), color.y(), color.z(), 1.0f);

    return true;
}

QSGMaterialType *QSGHiQSubPixelDistanceFieldTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGHiQSubPixelDistanceFieldTextMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    if (renderMode == QSGRendererInterface::RenderMode3D && m_glyph_cache->screenSpaceDerivativesSupported())
        return new DistanceFieldAnisotropicTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled(), viewCount());
    else
        return new QSGHiQSubPixelDistanceFieldTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled(), viewCount());
}

class QSGLoQSubPixelDistanceFieldTextMaterialRhiShader : public QSGHiQSubPixelDistanceFieldTextMaterialRhiShader
{
public:
    QSGLoQSubPixelDistanceFieldTextMaterialRhiShader(bool alphaTexture, int viewCount);
};

QSGLoQSubPixelDistanceFieldTextMaterialRhiShader::QSGLoQSubPixelDistanceFieldTextMaterialRhiShader(bool alphaTexture, int viewCount)
    : QSGHiQSubPixelDistanceFieldTextMaterialRhiShader(alphaTexture, viewCount)
{
    setShaderFileName(VertexStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/loqsubpixeldistancefieldtext.vert.qsb"), viewCount);
    if (alphaTexture)
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/loqsubpixeldistancefieldtext_a.frag.qsb"), viewCount);
    else
        setShaderFileName(FragmentStage, QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/loqsubpixeldistancefieldtext.frag.qsb"), viewCount);
}

QSGMaterialType *QSGLoQSubPixelDistanceFieldTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGLoQSubPixelDistanceFieldTextMaterial::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    if (renderMode == QSGRendererInterface::RenderMode3D && m_glyph_cache->screenSpaceDerivativesSupported())
        return new DistanceFieldAnisotropicTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled(), viewCount());
    else
        return new QSGLoQSubPixelDistanceFieldTextMaterialRhiShader(m_glyph_cache->eightBitFormatIsAlphaSwizzled(), viewCount());
}

QT_END_NAMESPACE
