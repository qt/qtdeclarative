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

#include "qsgdistancefieldglyphnode_p_p.h"
#include <QtQuick/private/qsgdistancefieldutil_p.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtGui/qopenglfunctions.h>
#include <QtGui/qsurface.h>
#include <QtGui/qwindow.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE

class QSGDistanceFieldTextMaterialShader : public QSGMaterialShader
{
public:
    QSGDistanceFieldTextMaterialShader();

    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect);
    virtual char const *const *attributeNames() const;

protected:
    virtual void initialize();
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;

    void updateAlphaRange(ThresholdFunc thresholdFunc, AntialiasingSpreadFunc spreadFunc);

    float m_fontScale;
    float m_matrixScale;

    int m_matrix_id;
    int m_textureScale_id;
    int m_alphaMin_id;
    int m_alphaMax_id;
    int m_color_id;
};

const char *QSGDistanceFieldTextMaterialShader::vertexShader() const {
    return
        "uniform highp mat4 matrix;                     \n"
        "uniform highp vec2 textureScale;               \n"
        "attribute highp vec4 vCoord;                   \n"
        "attribute highp vec2 tCoord;                   \n"
        "varying highp vec2 sampleCoord;                \n"
        "void main() {                                  \n"
        "     sampleCoord = tCoord * textureScale;      \n"
        "     gl_Position = matrix * vCoord;            \n"
        "}";
}

const char *QSGDistanceFieldTextMaterialShader::fragmentShader() const {
    return
        "varying highp vec2 sampleCoord;                                             \n"
        "uniform mediump sampler2D texture;                                          \n"
        "uniform lowp vec4 color;                                                    \n"
        "uniform mediump float alphaMin;                                             \n"
        "uniform mediump float alphaMax;                                             \n"
        "void main() {                                                               \n"
        "    gl_FragColor = color * smoothstep(alphaMin,                             \n"
        "                                      alphaMax,                             \n"
        "                                      texture2D(texture, sampleCoord).a);   \n"
        "}";
}

char const *const *QSGDistanceFieldTextMaterialShader::attributeNames() const {
    static char const *const attr[] = { "vCoord", "tCoord", 0 };
    return attr;
}

QSGDistanceFieldTextMaterialShader::QSGDistanceFieldTextMaterialShader()
    : m_fontScale(1.0)
    , m_matrixScale(1.0)
{
}

void QSGDistanceFieldTextMaterialShader::updateAlphaRange(ThresholdFunc thresholdFunc, AntialiasingSpreadFunc spreadFunc)
{
    float combinedScale = m_fontScale * m_matrixScale;
    float base = thresholdFunc(combinedScale);
    float range = spreadFunc(combinedScale);
    float alphaMin = qMax(0.0f, base - range);
    float alphaMax = qMin(base + range, 1.0f);
    program()->setUniformValue(m_alphaMin_id, GLfloat(alphaMin));
    program()->setUniformValue(m_alphaMax_id, GLfloat(alphaMax));
}

void QSGDistanceFieldTextMaterialShader::initialize()
{
    QSGMaterialShader::initialize();
    m_matrix_id = program()->uniformLocation("matrix");
    m_textureScale_id = program()->uniformLocation("textureScale");
    m_color_id = program()->uniformLocation("color");
    m_alphaMin_id = program()->uniformLocation("alphaMin");
    m_alphaMax_id = program()->uniformLocation("alphaMax");
}

void QSGDistanceFieldTextMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    Q_ASSERT(oldEffect == 0 || newEffect->type() == oldEffect->type());
    QSGDistanceFieldTextMaterial *material = static_cast<QSGDistanceFieldTextMaterial *>(newEffect);
    QSGDistanceFieldTextMaterial *oldMaterial = static_cast<QSGDistanceFieldTextMaterial *>(oldEffect);

    bool updated = material->updateTextureSize();

    if (oldMaterial == 0
           || material->color() != oldMaterial->color()
           || state.isOpacityDirty()) {
        QVector4D color = material->color();
        color *= state.opacity();
        program()->setUniformValue(m_color_id, color);
    }

    bool updateRange = false;
    if (oldMaterial == 0
            || material->fontScale() != oldMaterial->fontScale()) {
        m_fontScale = material->fontScale();
        updateRange = true;
    }
    if (state.isMatrixDirty()) {
        program()->setUniformValue(m_matrix_id, state.combinedMatrix());
        m_matrixScale = qSqrt(qAbs(state.determinant())) * state.devicePixelRatio();
        updateRange = true;
    }
    if (updateRange) {
        updateAlphaRange(material->glyphCache()->manager()->thresholdFunc(),
                         material->glyphCache()->manager()->antialiasingSpreadFunc());
    }

    Q_ASSERT(material->glyphCache());

    if (updated
            || oldMaterial == 0
            || oldMaterial->texture()->textureId != material->texture()->textureId) {
        program()->setUniformValue(m_textureScale_id, QVector2D(1.0 / material->textureSize().width(),
                                                                1.0 / material->textureSize().height()));
        glBindTexture(GL_TEXTURE_2D, material->texture()->textureId);

        if (updated) {
            // Set the mag/min filters to be linear. We only need to do this when the texture
            // has been recreated.
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }
    }
}

QSGDistanceFieldTextMaterial::QSGDistanceFieldTextMaterial()
    : m_glyph_cache(0)
    , m_texture(0)
    , m_fontScale(1.0)
{
   setFlag(Blending | RequiresDeterminant, true);
}

QSGDistanceFieldTextMaterial::~QSGDistanceFieldTextMaterial()
{
}

QSGMaterialType *QSGDistanceFieldTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

void QSGDistanceFieldTextMaterial::setColor(const QColor &color)
{
    m_color = QVector4D(color.redF() * color.alphaF(),
                        color.greenF() * color.alphaF(),
                        color.blueF() * color.alphaF(),
                        color.alphaF());
}

QSGMaterialShader *QSGDistanceFieldTextMaterial::createShader() const
{
    return new QSGDistanceFieldTextMaterialShader;
}

bool QSGDistanceFieldTextMaterial::updateTextureSize()
{
    if (!m_texture)
        m_texture = m_glyph_cache->glyphTexture(0); // invalid texture

    if (m_texture->size != m_size) {
        m_size = m_texture->size;
        return true;
    } else {
        return false;
    }
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
    int t0 = m_texture ? m_texture->textureId : 0;
    int t1 = other->m_texture ? other->m_texture->textureId : 0;
    return t0 - t1;
}


class DistanceFieldStyledTextMaterialShader : public QSGDistanceFieldTextMaterialShader
{
public:
    DistanceFieldStyledTextMaterialShader();

    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect);

protected:
    virtual void initialize();
    virtual const char *fragmentShader() const = 0;

    int m_styleColor_id;
};

DistanceFieldStyledTextMaterialShader::DistanceFieldStyledTextMaterialShader()
    : QSGDistanceFieldTextMaterialShader()
{
}

void DistanceFieldStyledTextMaterialShader::initialize()
{
    QSGDistanceFieldTextMaterialShader::initialize();
    m_styleColor_id = program()->uniformLocation("styleColor");
}

void DistanceFieldStyledTextMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    QSGDistanceFieldTextMaterialShader::updateState(state, newEffect, oldEffect);

    QSGDistanceFieldStyledTextMaterial *material = static_cast<QSGDistanceFieldStyledTextMaterial *>(newEffect);
    QSGDistanceFieldStyledTextMaterial *oldMaterial = static_cast<QSGDistanceFieldStyledTextMaterial *>(oldEffect);

    if (oldMaterial == 0
           || material->styleColor() != oldMaterial->styleColor()
           || (state.isOpacityDirty())) {
        QVector4D color = material->styleColor();
        color *= state.opacity();
        program()->setUniformValue(m_styleColor_id, color);
    }
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
    m_styleColor = QVector4D(color.redF() * color.alphaF(),
                             color.greenF() * color.alphaF(),
                             color.blueF() * color.alphaF(),
                             color.alphaF());
}

int QSGDistanceFieldStyledTextMaterial::compare(const QSGMaterial *o) const
{
    Q_ASSERT(o && type() == o->type());
    const QSGDistanceFieldStyledTextMaterial *other = static_cast<const QSGDistanceFieldStyledTextMaterial *>(o);
    if (m_styleColor != other->m_color)
        return &m_styleColor < &other->m_styleColor ? -1 : 1;
    return QSGDistanceFieldTextMaterial::compare(o);
}


class DistanceFieldOutlineTextMaterialShader : public DistanceFieldStyledTextMaterialShader
{
public:
    DistanceFieldOutlineTextMaterialShader();

    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect);

protected:
    virtual void initialize();
    virtual const char *fragmentShader() const;

    void updateOutlineAlphaRange(ThresholdFunc thresholdFunc, AntialiasingSpreadFunc spreadFunc, int dfRadius);

    int m_outlineAlphaMax0_id;
    int m_outlineAlphaMax1_id;
};

const char *DistanceFieldOutlineTextMaterialShader::fragmentShader() const {
    return
            "varying highp vec2 sampleCoord;                                                      \n"
            "uniform sampler2D texture;                                                           \n"
            "uniform lowp vec4 color;                                                             \n"
            "uniform lowp vec4 styleColor;                                                        \n"
            "uniform mediump float alphaMin;                                                      \n"
            "uniform mediump float alphaMax;                                                      \n"
            "uniform mediump float outlineAlphaMax0;                                              \n"
            "uniform mediump float outlineAlphaMax1;                                              \n"
            "void main() {                                                                        \n"
            "    mediump float d = texture2D(texture, sampleCoord).a;                             \n"
            "    gl_FragColor = mix(styleColor, color, smoothstep(alphaMin, alphaMax, d))         \n"
            "                       * smoothstep(outlineAlphaMax0, outlineAlphaMax1, d);          \n"
            "}";
}

DistanceFieldOutlineTextMaterialShader::DistanceFieldOutlineTextMaterialShader()
    : DistanceFieldStyledTextMaterialShader()
{
}

void DistanceFieldOutlineTextMaterialShader::initialize()
{
    DistanceFieldStyledTextMaterialShader::initialize();
    m_outlineAlphaMax0_id = program()->uniformLocation("outlineAlphaMax0");
    m_outlineAlphaMax1_id = program()->uniformLocation("outlineAlphaMax1");
}

void DistanceFieldOutlineTextMaterialShader::updateOutlineAlphaRange(ThresholdFunc thresholdFunc,
                                                                     AntialiasingSpreadFunc spreadFunc,
                                                                     int dfRadius)
{
    float combinedScale = m_fontScale * m_matrixScale;
    float base = thresholdFunc(combinedScale);
    float range = spreadFunc(combinedScale);
    float outlineLimit = qMax(0.2f, base - 0.5f / dfRadius / m_fontScale);

    float alphaMin = qMax(0.0f, base - range);
    float styleAlphaMin0 = qMax(0.0f, outlineLimit - range);
    float styleAlphaMin1 = qMin(outlineLimit + range, alphaMin);
    program()->setUniformValue(m_outlineAlphaMax0_id, GLfloat(styleAlphaMin0));
    program()->setUniformValue(m_outlineAlphaMax1_id, GLfloat(styleAlphaMin1));
}

void DistanceFieldOutlineTextMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    DistanceFieldStyledTextMaterialShader::updateState(state, newEffect, oldEffect);

    QSGDistanceFieldOutlineTextMaterial *material = static_cast<QSGDistanceFieldOutlineTextMaterial *>(newEffect);
    QSGDistanceFieldOutlineTextMaterial *oldMaterial = static_cast<QSGDistanceFieldOutlineTextMaterial *>(oldEffect);

    if (oldMaterial == 0
            || material->fontScale() != oldMaterial->fontScale()
            || state.isMatrixDirty())
        updateOutlineAlphaRange(material->glyphCache()->manager()->thresholdFunc(),
                                material->glyphCache()->manager()->antialiasingSpreadFunc(),
                                material->glyphCache()->distanceFieldRadius());
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

QSGMaterialShader *QSGDistanceFieldOutlineTextMaterial::createShader() const
{
    return new DistanceFieldOutlineTextMaterialShader;
}


class DistanceFieldShiftedStyleTextMaterialShader : public DistanceFieldStyledTextMaterialShader
{
public:
    DistanceFieldShiftedStyleTextMaterialShader();

    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect);

protected:
    virtual void initialize();
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;

    void updateShift(qreal fontScale, const QPointF& shift);

    int m_shift_id;
};

DistanceFieldShiftedStyleTextMaterialShader::DistanceFieldShiftedStyleTextMaterialShader()
    : DistanceFieldStyledTextMaterialShader()
{
}

void DistanceFieldShiftedStyleTextMaterialShader::initialize()
{
    DistanceFieldStyledTextMaterialShader::initialize();
    m_shift_id = program()->uniformLocation("shift");
}

void DistanceFieldShiftedStyleTextMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    DistanceFieldStyledTextMaterialShader::updateState(state, newEffect, oldEffect);

    QSGDistanceFieldShiftedStyleTextMaterial *material = static_cast<QSGDistanceFieldShiftedStyleTextMaterial *>(newEffect);
    QSGDistanceFieldShiftedStyleTextMaterial *oldMaterial = static_cast<QSGDistanceFieldShiftedStyleTextMaterial *>(oldEffect);

    if (oldMaterial == 0
            || oldMaterial->fontScale() != material->fontScale()
            || oldMaterial->shift() != material->shift()
            || oldMaterial->textureSize() != material->textureSize()) {
        updateShift(material->fontScale(), material->shift());
    }
}

void DistanceFieldShiftedStyleTextMaterialShader::updateShift(qreal fontScale, const QPointF &shift)
{
    QPointF texel(1.0 / fontScale * shift.x(),
                  1.0 / fontScale * shift.y());
    program()->setUniformValue(m_shift_id, texel);
}

const char *DistanceFieldShiftedStyleTextMaterialShader::vertexShader() const
{
    return
            "uniform highp mat4 matrix;                                 \n"
            "uniform highp vec2 textureScale;                           \n"
            "attribute highp vec4 vCoord;                               \n"
            "attribute highp vec2 tCoord;                               \n"
            "uniform highp vec2 shift;                                  \n"
            "varying highp vec2 sampleCoord;                            \n"
            "varying highp vec2 shiftedSampleCoord;                     \n"
            "void main() {                                              \n"
            "     sampleCoord = tCoord * textureScale;                  \n"
            "     shiftedSampleCoord = (tCoord - shift) * textureScale; \n"
            "     gl_Position = matrix * vCoord;                        \n"
            "}";
}

const char *DistanceFieldShiftedStyleTextMaterialShader::fragmentShader() const {
    return
            "varying highp vec2 sampleCoord;                                                       \n"
            "varying highp vec2 shiftedSampleCoord;                                                \n"
            "uniform sampler2D texture;                                                            \n"
            "uniform lowp vec4 color;                                                              \n"
            "uniform lowp vec4 styleColor;                                                         \n"
            "uniform mediump float alphaMin;                                                       \n"
            "uniform mediump float alphaMax;                                                       \n"
            "void main() {                                                                         \n"
            "    highp float a = smoothstep(alphaMin, alphaMax, texture2D(texture, sampleCoord).a);\n"
            "    highp vec4 shifted = styleColor * smoothstep(alphaMin,                            \n"
            "                                                 alphaMax,                            \n"
            "                                                 texture2D(texture, shiftedSampleCoord).a); \n"
            "    gl_FragColor = mix(shifted, color, a);                                            \n"
            "}";
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

QSGMaterialShader *QSGDistanceFieldShiftedStyleTextMaterial::createShader() const
{
    return new DistanceFieldShiftedStyleTextMaterialShader;
}


class QSGHiQSubPixelDistanceFieldTextMaterialShader : public QSGDistanceFieldTextMaterialShader
{
public:
    virtual void initialize();
    virtual void activate();
    virtual void deactivate();
    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect);

protected:
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;

private:
    int m_fontScale_id;
    int m_vecDelta_id;
};

const char *QSGHiQSubPixelDistanceFieldTextMaterialShader::vertexShader() const {
    return
        "uniform highp mat4 matrix;                                             \n"
        "uniform highp vec2 textureScale;                                       \n"
        "uniform highp float fontScale;                                         \n"
        "uniform highp vec4 vecDelta;                                           \n"
        "attribute highp vec4 vCoord;                                           \n"
        "attribute highp vec2 tCoord;                                           \n"
        "varying highp vec2 sampleCoord;                                        \n"
        "varying highp vec3 sampleFarLeft;                                      \n"
        "varying highp vec3 sampleNearLeft;                                     \n"
        "varying highp vec3 sampleNearRight;                                    \n"
        "varying highp vec3 sampleFarRight;                                     \n"
        "void main() {                                                          \n"
        "     sampleCoord = tCoord * textureScale;                              \n"
        "     gl_Position = matrix * vCoord;                                    \n"
        // Calculate neighbour pixel position in item space.
        "     highp vec3 wDelta = gl_Position.w * vecDelta.xyw;                 \n"
        "     highp vec3 farLeft = vCoord.xyw - 0.667 * wDelta;                 \n"
        "     highp vec3 nearLeft = vCoord.xyw - 0.333 * wDelta;                \n"
        "     highp vec3 nearRight = vCoord.xyw + 0.333 * wDelta;               \n"
        "     highp vec3 farRight = vCoord.xyw + 0.667 * wDelta;                \n"
        // Calculate neighbour texture coordinate.
        "     highp vec2 scale = textureScale / fontScale;                      \n"
        "     highp vec2 base = sampleCoord - scale * vCoord.xy;                \n"
        "     sampleFarLeft = vec3(base * farLeft.z + scale * farLeft.xy, farLeft.z); \n"
        "     sampleNearLeft = vec3(base * nearLeft.z + scale * nearLeft.xy, nearLeft.z); \n"
        "     sampleNearRight = vec3(base * nearRight.z + scale * nearRight.xy, nearRight.z); \n"
        "     sampleFarRight = vec3(base * farRight.z + scale * farRight.xy, farRight.z); \n"
        "}";
}

const char *QSGHiQSubPixelDistanceFieldTextMaterialShader::fragmentShader() const {
    return
        "varying highp vec2 sampleCoord;                                        \n"
        "varying highp vec3 sampleFarLeft;                                      \n"
        "varying highp vec3 sampleNearLeft;                                     \n"
        "varying highp vec3 sampleNearRight;                                    \n"
        "varying highp vec3 sampleFarRight;                                     \n"
        "uniform sampler2D texture;                                             \n"
        "uniform lowp vec4 color;                                               \n"
        "uniform mediump float alphaMin;                                        \n"
        "uniform mediump float alphaMax;                                        \n"
        "void main() {                                                          \n"
        "    highp vec4 n;                                                      \n"
        "    n.x = texture2DProj(texture, sampleFarLeft).a;                     \n"
        "    n.y = texture2DProj(texture, sampleNearLeft).a;                    \n"
        "    highp float c = texture2D(texture, sampleCoord).a;                 \n"
        "    n.z = texture2DProj(texture, sampleNearRight).a;                   \n"
        "    n.w = texture2DProj(texture, sampleFarRight).a;                    \n"
#if 0
        // Blurrier, faster.
        "    n = smoothstep(alphaMin, alphaMax, n);                             \n"
        "    c = smoothstep(alphaMin, alphaMax, c);                             \n"
#else
        // Sharper, slower.
        "    highp vec2 d = min(abs(n.yw - n.xz) * 2., 0.67);                   \n"
        "    highp vec2 lo = mix(vec2(alphaMin), vec2(0.5), d);                 \n"
        "    highp vec2 hi = mix(vec2(alphaMax), vec2(0.5), d);                 \n"
        "    n = smoothstep(lo.xxyy, hi.xxyy, n);                               \n"
        "    c = smoothstep(lo.x + lo.y, hi.x + hi.y, 2. * c);                  \n"
#endif
        "    gl_FragColor = vec4(0.333 * (n.xyz + n.yzw + c), c) * color.w;     \n"
        "}";
}

//const char *QSGHiQSubPixelDistanceFieldTextMaterialShader::fragmentShader() const {
//    return
//        "#extension GL_OES_standard_derivatives: enable                         \n"
//        "varying highp vec2 sampleCoord;                                        \n"
//        "uniform sampler2D texture;                                             \n"
//        "uniform lowp vec4 color;                                               \n"
//        "uniform highp float alphaMin;                                          \n"
//        "uniform highp float alphaMax;                                          \n"
//        "void main() {                                                          \n"
//        "    highp vec2 delta = dFdx(sampleCoord);                              \n"
//        "    highp vec4 n;                                                      \n"
//        "    n.x = texture2D(texture, sampleCoord - 0.667 * delta).a;           \n"
//        "    n.y = texture2D(texture, sampleCoord - 0.333 * delta).a;           \n"
//        "    highp float c = texture2D(texture, sampleCoord).a;                 \n"
//        "    n.z = texture2D(texture, sampleCoord + 0.333 * delta).a;           \n"
//        "    n.w = texture2D(texture, sampleCoord + 0.667 * delta).a;           \n"
//        "    n = smoothstep(alphaMin, alphaMax, n);                             \n"
//        "    c = smoothstep(alphaMin, alphaMax, c);                             \n"
//        "    gl_FragColor = vec4(0.333 * (n.xyz + n.yzw + c), c) * color.w;     \n"
//        "}";
//}

void QSGHiQSubPixelDistanceFieldTextMaterialShader::initialize()
{
    QSGDistanceFieldTextMaterialShader::initialize();
    m_fontScale_id = program()->uniformLocation("fontScale");
    m_vecDelta_id = program()->uniformLocation("vecDelta");
}

void QSGHiQSubPixelDistanceFieldTextMaterialShader::activate()
{
    QSGDistanceFieldTextMaterialShader::activate();
    glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_COLOR);
}

void QSGHiQSubPixelDistanceFieldTextMaterialShader::deactivate()
{
    QSGDistanceFieldTextMaterialShader::deactivate();
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

void QSGHiQSubPixelDistanceFieldTextMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    Q_ASSERT(oldEffect == 0 || newEffect->type() == oldEffect->type());
    QSGDistanceFieldTextMaterial *material = static_cast<QSGDistanceFieldTextMaterial *>(newEffect);
    QSGDistanceFieldTextMaterial *oldMaterial = static_cast<QSGDistanceFieldTextMaterial *>(oldEffect);

    if (oldMaterial == 0 || material->color() != oldMaterial->color()) {
        QVector4D c = material->color();
        state.context()->functions()->glBlendColor(c.x(), c.y(), c.z(), 1.0f);
    }

    if (oldMaterial == 0 || material->fontScale() != oldMaterial->fontScale())
        program()->setUniformValue(m_fontScale_id, GLfloat(material->fontScale()));

    if (oldMaterial == 0 || state.isMatrixDirty()) {
        int viewportWidth = state.viewportRect().width();
        QMatrix4x4 mat = state.combinedMatrix().inverted();
        program()->setUniformValue(m_vecDelta_id, mat.column(0) * (qreal(2) / viewportWidth));
    }

    QSGDistanceFieldTextMaterialShader::updateState(state, newEffect, oldEffect);
}

QSGMaterialType *QSGHiQSubPixelDistanceFieldTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGHiQSubPixelDistanceFieldTextMaterial::createShader() const
{
    return new QSGHiQSubPixelDistanceFieldTextMaterialShader;
}


class QSGLoQSubPixelDistanceFieldTextMaterialShader : public QSGHiQSubPixelDistanceFieldTextMaterialShader
{
protected:
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;
};

const char *QSGLoQSubPixelDistanceFieldTextMaterialShader::vertexShader() const {
    return
        "uniform highp mat4 matrix;                                             \n"
        "uniform highp vec2 textureScale;                                       \n"
        "uniform highp float fontScale;                                         \n"
        "uniform highp vec4 vecDelta;                                           \n"
        "attribute highp vec4 vCoord;                                           \n"
        "attribute highp vec2 tCoord;                                           \n"
        "varying highp vec3 sampleNearLeft;                                     \n"
        "varying highp vec3 sampleNearRight;                                    \n"
        "void main() {                                                          \n"
        "     highp vec2 sampleCoord = tCoord * textureScale;                   \n"
        "     gl_Position = matrix * vCoord;                                    \n"
        // Calculate neighbour pixel position in item space.
        "     highp vec3 wDelta = gl_Position.w * vecDelta.xyw;                 \n"
        "     highp vec3 nearLeft = vCoord.xyw - 0.25 * wDelta;                 \n"
        "     highp vec3 nearRight = vCoord.xyw + 0.25 * wDelta;                \n"
        // Calculate neighbour texture coordinate.
        "     highp vec2 scale = textureScale / fontScale;                      \n"
        "     highp vec2 base = sampleCoord - scale * vCoord.xy;                \n"
        "     sampleNearLeft = vec3(base * nearLeft.z + scale * nearLeft.xy, nearLeft.z); \n"
        "     sampleNearRight = vec3(base * nearRight.z + scale * nearRight.xy, nearRight.z); \n"
        "}";
}

const char *QSGLoQSubPixelDistanceFieldTextMaterialShader::fragmentShader() const {
    return
        "varying highp vec3 sampleNearLeft;                                     \n"
        "varying highp vec3 sampleNearRight;                                    \n"
        "uniform sampler2D texture;                                             \n"
        "uniform lowp vec4 color;                                               \n"
        "uniform mediump float alphaMin;                                        \n"
        "uniform mediump float alphaMax;                                        \n"
        "void main() {                                                          \n"
        "    highp vec2 n;                                                      \n"
        "    n.x = texture2DProj(texture, sampleNearLeft).a;                    \n"
        "    n.y = texture2DProj(texture, sampleNearRight).a;                   \n"
        "    n = smoothstep(alphaMin, alphaMax, n);                             \n"
        "    highp float c = 0.5 * (n.x + n.y);                                 \n"
        "    gl_FragColor = vec4(n.x, c, n.y, c) * color.w;                     \n"
        "}";
}

QSGMaterialType *QSGLoQSubPixelDistanceFieldTextMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader *QSGLoQSubPixelDistanceFieldTextMaterial::createShader() const
{
    return new QSGLoQSubPixelDistanceFieldTextMaterialShader;
}

QT_END_NAMESPACE
