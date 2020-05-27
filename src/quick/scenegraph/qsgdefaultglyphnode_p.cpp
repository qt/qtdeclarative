/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgdefaultglyphnode_p_p.h"
#include <private/qsgmaterialshader_p.h>

#include <qopenglshaderprogram.h>
#include <qopenglframebufferobject.h>

#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qplatformintegration.h>
#include <private/qfontengine_p.h>
#include <private/qopenglextensions_p.h>

#include <QtQuick/qquickwindow.h>
#include <QtQuick/private/qsgtexture_p.h>
#include <QtQuick/private/qsgdefaultrendercontext_p.h>

#include <private/qrawfont_p.h>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif

#ifndef GL_FRAMEBUFFER_SRGB_CAPABLE
#define GL_FRAMEBUFFER_SRGB_CAPABLE 0x8DBA
#endif

static inline QVector4D qsg_premultiply(const QVector4D &c, float globalOpacity)
{
    float o = c.w() * globalOpacity;
    return QVector4D(c.x() * o, c.y() * o, c.z() * o, o);
}

static inline qreal qt_sRGB_to_linear_RGB(qreal f)
{
    return f > 0.04045 ? qPow((f + 0.055) / 1.055, 2.4) : f / 12.92;
}

static inline QVector4D qt_sRGB_to_linear_RGB(const QVector4D &color)
{
    return QVector4D(qt_sRGB_to_linear_RGB(color.x()),
                     qt_sRGB_to_linear_RGB(color.y()),
                     qt_sRGB_to_linear_RGB(color.z()),
                     color.w());
}

static inline qreal fontSmoothingGamma()
{
    static qreal fontSmoothingGamma = QGuiApplicationPrivate::platformIntegration()->styleHint(QPlatformIntegration::FontSmoothingGamma).toReal();
    return fontSmoothingGamma;
}


// ***** legacy (GL) material shader implementations

static inline qreal qsg_device_pixel_ratio(QOpenGLContext *ctx)
{
    qreal devicePixelRatio = 1;
    if (ctx->surface()->surfaceClass() == QSurface::Window) {
        QWindow *w = static_cast<QWindow *>(ctx->surface());
        if (QQuickWindow *qw = qobject_cast<QQuickWindow *>(w))
            devicePixelRatio = qw->effectiveDevicePixelRatio();
        else
            devicePixelRatio = w->devicePixelRatio();
    } else {
        devicePixelRatio = ctx->screen() ? ctx->screen()->devicePixelRatio() : qGuiApp->devicePixelRatio();
    }
    return devicePixelRatio;
}

class QSGTextMaskShader : public QSGMaterialShader
{
public:
    QSGTextMaskShader(QFontEngine::GlyphFormat glyphFormat);

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
    char const *const *attributeNames() const override;

protected:
    void initialize() override;

    int m_matrix_id;
    int m_color_id;
    int m_textureScale_id;
    float m_devicePixelRatio;

    QFontEngine::GlyphFormat m_glyphFormat;
};

char const *const *QSGTextMaskShader::attributeNames() const
{
    static char const *const attr[] = { "vCoord", "tCoord", nullptr };
    return attr;
}

QSGTextMaskShader::QSGTextMaskShader(QFontEngine::GlyphFormat glyphFormat)
    : QSGMaterialShader(*new QSGMaterialShaderPrivate)
    , m_matrix_id(-1)
    , m_color_id(-1)
    , m_textureScale_id(-1)
    , m_glyphFormat(glyphFormat)
{
    setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/scenegraph/shaders/textmask.vert"));
    setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/textmask.frag"));
}

void QSGTextMaskShader::initialize()
{
    m_matrix_id = program()->uniformLocation("matrix");
    m_color_id = program()->uniformLocation("color");
    m_textureScale_id = program()->uniformLocation("textureScale");
    m_devicePixelRatio = (float) qsg_device_pixel_ratio(QOpenGLContext::currentContext());
    program()->setUniformValue("dpr", m_devicePixelRatio);
}

void QSGTextMaskShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    QSGTextMaskMaterial *material = static_cast<QSGTextMaskMaterial *>(newEffect);
    QSGTextMaskMaterial *oldMaterial = static_cast<QSGTextMaskMaterial *>(oldEffect);
    Q_ASSERT(oldEffect == nullptr || newEffect->type() == oldEffect->type());
    bool updated = material->ensureUpToDate();
    Q_ASSERT(material->texture());

    Q_ASSERT(oldMaterial == nullptr || oldMaterial->texture());
    if (updated
            || oldMaterial == nullptr
            || oldMaterial->texture()->textureId() != material->texture()->textureId()) {
        program()->setUniformValue(m_textureScale_id, QVector2D(1.0 / material->openglGlyphCache()->width(),
                                                               1.0 / material->openglGlyphCache()->height()));
        QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
        funcs->glBindTexture(GL_TEXTURE_2D, material->texture()->textureId());

        // Set the mag/min filters to be nearest. We only need to do this when the texture
        // has been recreated.
        if (updated) {
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
    }

    float devicePixelRatio = (float) qsg_device_pixel_ratio(QOpenGLContext::currentContext());
    if (m_devicePixelRatio != devicePixelRatio) {
        m_devicePixelRatio = devicePixelRatio;
        program()->setUniformValue("dpr", m_devicePixelRatio);
    }

    if (state.isMatrixDirty())
        program()->setUniformValue(m_matrix_id, state.combinedMatrix());
}

class QSG8BitTextMaskShader : public QSGTextMaskShader
{
public:
    QSG8BitTextMaskShader(QFontEngine::GlyphFormat glyphFormat)
        : QSGTextMaskShader(glyphFormat)
    {
        setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/8bittextmask.frag"));
    }

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
};

void QSG8BitTextMaskShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    QSGTextMaskShader::updateState(state, newEffect, oldEffect);
    QSGTextMaskMaterial *material = static_cast<QSGTextMaskMaterial *>(newEffect);
    QSGTextMaskMaterial *oldMaterial = static_cast<QSGTextMaskMaterial *>(oldEffect);

    if (oldMaterial == nullptr || material->color() != oldMaterial->color() || state.isOpacityDirty()) {
        QVector4D color = qsg_premultiply(material->color(), state.opacity());
        program()->setUniformValue(m_color_id, color);
    }
}

class QSG24BitTextMaskShader : public QSGTextMaskShader
{
public:
    QSG24BitTextMaskShader(QFontEngine::GlyphFormat glyphFormat)
        : QSGTextMaskShader(glyphFormat)
        , m_useSRGB(false)
    {
        setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/24bittextmask.frag"));
    }

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
    void initialize() override;
    void activate() override;
    void deactivate() override;

    bool useSRGB() const;
    uint m_useSRGB : 1;
};

void QSG24BitTextMaskShader::initialize()
{
    QSGTextMaskShader::initialize();
    // 0.25 was found to be acceptable error margin by experimentation. On Mac, the gamma is 2.0,
    // but using sRGB looks okay.
    if (QOpenGLContext::currentContext()->hasExtension(QByteArrayLiteral("GL_ARB_framebuffer_sRGB"))
            && m_glyphFormat == QFontEngine::Format_A32
            && qAbs(fontSmoothingGamma() - 2.2) < 0.25) {
        QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
        GLint srgbCapable = 0;
        funcs->glGetIntegerv(GL_FRAMEBUFFER_SRGB_CAPABLE, &srgbCapable);
        if (srgbCapable)
            m_useSRGB = true;
    }
}

bool QSG24BitTextMaskShader::useSRGB() const
{
#ifdef Q_OS_MACOS
    if (!m_useSRGB)
        return false;

    // m_useSRGB is true, but if some QOGLFBO was bound check it's texture format:
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    QOpenGLFramebufferObject *qfbo = QOpenGLContextPrivate::get(ctx)->qgl_current_fbo;
    bool fboInvalid = QOpenGLContextPrivate::get(ctx)->qgl_current_fbo_invalid;
    return !qfbo || fboInvalid || qfbo->format().internalTextureFormat() == GL_SRGB8_ALPHA8_EXT;
#else
    return m_useSRGB;
#endif
}

void QSG24BitTextMaskShader::activate()
{
    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    funcs->glBlendFunc(GL_CONSTANT_COLOR, GL_ONE_MINUS_SRC_COLOR);
    if (useSRGB())
        funcs->glEnable(GL_FRAMEBUFFER_SRGB);
}

void QSG24BitTextMaskShader::deactivate()
{
    QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
    funcs->glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    if (useSRGB())
        funcs->glDisable(GL_FRAMEBUFFER_SRGB);
}

void QSG24BitTextMaskShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    QSGTextMaskShader::updateState(state, newEffect, oldEffect);
    QSGTextMaskMaterial *material = static_cast<QSGTextMaskMaterial *>(newEffect);
    QSGTextMaskMaterial *oldMaterial = static_cast<QSGTextMaskMaterial *>(oldEffect);

    if (oldMaterial == nullptr || material->color() != oldMaterial->color() || state.isOpacityDirty()) {
        QVector4D color = material->color();
        if (useSRGB())
            color = qt_sRGB_to_linear_RGB(color);
        QOpenGLContext::currentContext()->functions()->glBlendColor(color.x(), color.y(), color.z(), color.w());
        color = qsg_premultiply(color, state.opacity());
        program()->setUniformValue(m_color_id, color.w());
    }
}

class QSG32BitColorTextShader : public QSGTextMaskShader
{
public:
    QSG32BitColorTextShader(QFontEngine::GlyphFormat glyphFormat)
        : QSGTextMaskShader(glyphFormat)
    {
        setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/32bitcolortext.frag"));
    }

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;
};

void QSG32BitColorTextShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    QSGTextMaskShader::updateState(state, newEffect, oldEffect);
    QSGTextMaskMaterial *material = static_cast<QSGTextMaskMaterial *>(newEffect);
    QSGTextMaskMaterial *oldMaterial = static_cast<QSGTextMaskMaterial *>(oldEffect);

    if (oldMaterial == nullptr || material->color() != oldMaterial->color() || state.isOpacityDirty()) {
        float opacity = material->color().w() * state.opacity();
        program()->setUniformValue(m_color_id, opacity);
    }
}

class QSGStyledTextShader : public QSG8BitTextMaskShader
{
public:
    QSGStyledTextShader(QFontEngine::GlyphFormat glyphFormat)
        : QSG8BitTextMaskShader(glyphFormat)
    {
        setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/scenegraph/shaders/styledtext.vert"));
        setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/styledtext.frag"));
    }

    void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect) override;

private:
    void initialize() override;

    int m_shift_id;
    int m_styleColor_id;
};

void QSGStyledTextShader::initialize()
{
    QSG8BitTextMaskShader::initialize();
    m_shift_id = program()->uniformLocation("shift");
    m_styleColor_id = program()->uniformLocation("styleColor");
}

void QSGStyledTextShader::updateState(const RenderState &state,
                                      QSGMaterial *newEffect,
                                      QSGMaterial *oldEffect)
{
    Q_ASSERT(oldEffect == nullptr || newEffect->type() == oldEffect->type());

    QSGStyledTextMaterial *material = static_cast<QSGStyledTextMaterial *>(newEffect);
    QSGStyledTextMaterial *oldMaterial = static_cast<QSGStyledTextMaterial *>(oldEffect);

    if (oldMaterial == nullptr || oldMaterial->styleShift() != material->styleShift())
        program()->setUniformValue(m_shift_id, material->styleShift());

    if (oldMaterial == nullptr || material->color() != oldMaterial->color() || state.isOpacityDirty()) {
        QVector4D color = qsg_premultiply(material->color(), state.opacity());
        program()->setUniformValue(m_color_id, color);
    }

    if (oldMaterial == nullptr || material->styleColor() != oldMaterial->styleColor() || state.isOpacityDirty()) {
        QVector4D styleColor = qsg_premultiply(material->styleColor(), state.opacity());
        program()->setUniformValue(m_styleColor_id, styleColor);
    }

    bool updated = material->ensureUpToDate();
    Q_ASSERT(material->texture());

    Q_ASSERT(oldMaterial == nullptr || oldMaterial->texture());
    if (updated
            || oldMaterial == nullptr
            || oldMaterial->texture()->textureId() != material->texture()->textureId()) {
        program()->setUniformValue(m_textureScale_id, QVector2D(1.0 / material->openglGlyphCache()->width(),
                                                                1.0 / material->openglGlyphCache()->height()));
        QOpenGLFunctions *funcs = QOpenGLContext::currentContext()->functions();
        funcs->glBindTexture(GL_TEXTURE_2D, material->texture()->textureId());

        // Set the mag/min filters to be nearest. We only need to do this when the texture
        // has been recreated.
        if (updated) {
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            funcs->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }
    }

    if (state.isMatrixDirty())
        program()->setUniformValue(m_matrix_id, state.combinedMatrix());
}

class QSGOutlinedTextShader : public QSGStyledTextShader
{
public:
    QSGOutlinedTextShader(QFontEngine::GlyphFormat glyphFormat)
        : QSGStyledTextShader(glyphFormat)
    {
        setShaderSourceFile(QOpenGLShader::Vertex, QStringLiteral(":/qt-project.org/scenegraph/shaders/outlinedtext.vert"));
        setShaderSourceFile(QOpenGLShader::Fragment, QStringLiteral(":/qt-project.org/scenegraph/shaders/outlinedtext.frag"));
    }
};


// ***** RHI shader implementations

class QSGTextMaskRhiShader : public QSGMaterialRhiShader
{
public:
    QSGTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat);

    bool updateUniformData(RenderState &state,
                           QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture,
                            QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;

protected:
    QFontEngine::GlyphFormat m_glyphFormat;
};

QSGTextMaskRhiShader::QSGTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat)
    : m_glyphFormat(glyphFormat)
{
    setShaderFileName(VertexStage,
                      QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/textmask.vert.qsb"));
    setShaderFileName(FragmentStage,
                      QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/textmask.frag.qsb"));
}

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
    Q_ASSERT(buf->size() >= 92);

    if (state.isMatrixDirty()) {
        const QMatrix4x4 m = state.combinedMatrix();
        memcpy(buf->data(), m.constData(), 64);
        changed = true;
    }

    QRhiTexture *oldRtex = oldMat ? QSGTexturePrivate::get(oldMat->texture())->rhiTexture() : nullptr;
    QRhiTexture *newRtex = QSGTexturePrivate::get(mat->texture())->rhiTexture();
    if (updated || !oldMat || oldRtex != newRtex) {
        const QVector2D textureScale = QVector2D(1.0f / mat->rhiGlyphCache()->width(),
                                                 1.0f / mat->rhiGlyphCache()->height());
        memcpy(buf->data() + 64 + 16, &textureScale, 8);
        changed = true;
    }

    if (!oldMat) {
        float dpr = state.devicePixelRatio();
        memcpy(buf->data() + 64 + 16 + 8, &dpr, 4);
    }

    // move texture uploads/copies onto the renderer's soon-to-be-committed list
    mat->rhiGlyphCache()->commitResourceUpdates(state.resourceUpdateBatch());

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
    QSG8BitTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat, bool alphaTexture)
        : QSGTextMaskRhiShader(glyphFormat)
    {
        if (alphaTexture)
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/8bittextmask_a.frag.qsb"));
        else
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/8bittextmask.frag.qsb"));
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
    Q_ASSERT(buf->size() >= 80);

    if (oldMat == nullptr || mat->color() != oldMat->color() || state.isOpacityDirty()) {
        const QVector4D color = qsg_premultiply(mat->color(), state.opacity());
        memcpy(buf->data() + 64, &color, 16);
        changed = true;
    }

    return changed;
}

class QSG24BitTextMaskRhiShader : public QSGTextMaskRhiShader
{
public:
    QSG24BitTextMaskRhiShader(QFontEngine::GlyphFormat glyphFormat)
        : QSGTextMaskRhiShader(glyphFormat)
    {
        setFlag(UpdatesGraphicsPipelineState, true);
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/24bittextmask.frag.qsb"));
    }

    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
    bool updateGraphicsPipelineState(RenderState &state, GraphicsPipelineState *ps,
                                     QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override;
};

// ### gamma correction (sRGB) Unsurprisingly, the GL approach is not portable
// to anything else - it just does not work that way, there is no opt-in/out
// switch and magic winsys-provided maybe-sRGB buffers. When requesting an sRGB
// QRhiSwapChain (which we do not do), it is full sRGB, with the sRGB
// framebuffer update and blending always on... Could we do gamma correction in
// the shader for text? (but that's bad for blending?)

bool QSG24BitTextMaskRhiShader::updateUniformData(RenderState &state,
                                                  QSGMaterial *newMaterial, QSGMaterial *oldMaterial)
{
    bool changed = QSGTextMaskRhiShader::updateUniformData(state, newMaterial, oldMaterial);

    QSGTextMaskMaterial *mat = static_cast<QSGTextMaskMaterial *>(newMaterial);
    QSGTextMaskMaterial *oldMat = static_cast<QSGTextMaskMaterial *>(oldMaterial);

    QByteArray *buf = state.uniformData();
    Q_ASSERT(buf->size() >= 92);

    if (oldMat == nullptr || mat->color() != oldMat->color() || state.isOpacityDirty()) {
        // shader takes vec4 but uses alpha only; coloring happens via the blend constant
        const QVector4D color = qsg_premultiply(mat->color(), state.opacity());
        memcpy(buf->data() + 64, &color, 16);
        changed = true;
    }

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

    QVector4D color = qsg_premultiply(mat->color(), state.opacity());
    //        if (useSRGB())
    //            color = qt_sRGB_to_linear_RGB(color);

    // this is dynamic state but it's - magic! - taken care of by the renderer
    ps->blendConstant = QColor::fromRgbF(color.x(), color.y(), color.z(), color.w());

    return true;
}

class QSG32BitColorTextRhiShader : public QSGTextMaskRhiShader
{
public:
    QSG32BitColorTextRhiShader(QFontEngine::GlyphFormat glyphFormat)
        : QSGTextMaskRhiShader(glyphFormat)
    {
        setShaderFileName(FragmentStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/32bitcolortext.frag.qsb"));
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
    Q_ASSERT(buf->size() >= 92);

    if (oldMat == nullptr || mat->color() != oldMat->color() || state.isOpacityDirty()) {
        // shader takes vec4 but uses alpha only
        const QVector4D color(0, 0, 0, mat->color().w() * state.opacity());
        memcpy(buf->data() + 64, &color, 16);
        changed = true;
    }

    return changed;
}

class QSGStyledTextRhiShader : public QSG8BitTextMaskRhiShader
{
public:
    QSGStyledTextRhiShader(QFontEngine::GlyphFormat glyphFormat, bool alphaTexture)
        : QSG8BitTextMaskRhiShader(glyphFormat, alphaTexture)
    {
        setShaderFileName(VertexStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/styledtext.vert.qsb"));
        if (alphaTexture)
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/styledtext_a.frag.qsb"));
        else
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/styledtext.frag.qsb"));
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
    Q_ASSERT(buf->size() >= 120);

    // matrix..dpr + 1 float padding (vec4 must be aligned to 16)
    const int startOffset = 64 + 16 + 8 + 4 + 4;

    if (oldMat == nullptr || mat->styleColor() != oldMat->styleColor() || state.isOpacityDirty()) {
        const QVector4D styleColor = qsg_premultiply(mat->styleColor(), state.opacity());
        memcpy(buf->data() + startOffset, &styleColor, 16);
        changed = true;
    }

    if (oldMat == nullptr || oldMat->styleShift() != mat->styleShift()) {
        const QVector2D v = mat->styleShift();
        memcpy(buf->data() + startOffset + 16, &v, 8);
        changed = true;
    }

    return changed;
}

class QSGOutlinedTextRhiShader : public QSGStyledTextRhiShader
{
public:
    QSGOutlinedTextRhiShader(QFontEngine::GlyphFormat glyphFormat, bool alphaTexture)
        : QSGStyledTextRhiShader(glyphFormat, alphaTexture)
    {
        setShaderFileName(VertexStage,
                          QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/outlinedtext.vert.qsb"));
        if (alphaTexture)
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/outlinedtext_a.frag.qsb"));
        else
            setShaderFileName(FragmentStage,
                              QStringLiteral(":/qt-project.org/scenegraph/shaders_ng/outlinedtext.frag.qsb"));
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

    setFlag(SupportsRhiShader, true);
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

        QOpenGLContext *ctx = nullptr;
        qreal devicePixelRatio;
        void *cacheKey;
        if (m_rhi) {
            cacheKey = m_rhi;
            // Get the dpr the modern way. This value retrieved via the
            // rendercontext matches what RenderState::devicePixelRatio()
            // exposes to the material shaders later on.
            devicePixelRatio = m_rc->currentDevicePixelRatio();
        } else {
            ctx = const_cast<QOpenGLContext *>(QOpenGLContext::currentContext());
            Q_ASSERT(ctx != nullptr);
            cacheKey = ctx;
            devicePixelRatio = qsg_device_pixel_ratio(ctx); // this is technically incorrect, see other branch above
        }

        QTransform glyphCacheTransform = QTransform::fromScale(devicePixelRatio, devicePixelRatio);
        if (!fontEngine->supportsTransformation(glyphCacheTransform))
            glyphCacheTransform = QTransform();

        QColor color = glyphFormat == QFontEngine::Format_ARGB ? QColor::fromRgbF(m_color.x(), m_color.y(), m_color.z(), m_color.w()) : QColor();
        m_glyphCache = fontEngine->glyphCache(cacheKey, glyphFormat, glyphCacheTransform, color);
        if (!m_glyphCache || int(m_glyphCache->glyphFormat()) != glyphFormat) {
            if (m_rhi)
                m_glyphCache = new QSGRhiTextureGlyphCache(m_rhi, glyphFormat, glyphCacheTransform, color);
            else
                m_glyphCache = new QOpenGLTextureGlyphCache(glyphFormat, glyphCacheTransform, color);

            fontEngine->setGlyphCache(cacheKey, m_glyphCache.data());
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
    cache->populate(fontD->fontEngine, glyphIndexes.size(), glyphIndexes.constData(),
                    fixedPointPositions.data());
    cache->fillInPendingGlyphs();

    int margin = fontD->fontEngine->glyphMargin(cache->glyphFormat());

    qreal glyphCacheScaleX = cache->transform().m11();
    qreal glyphCacheScaleY = cache->transform().m22();
    qreal glyphCacheInverseScaleX = 1.0 / glyphCacheScaleX;
    qreal glyphCacheInverseScaleY = 1.0 / glyphCacheScaleY;
    qreal scaledMargin = margin * glyphCacheInverseScaleX;

    Q_ASSERT(geometry->indexType() == GL_UNSIGNED_SHORT);
    geometry->allocate(glyphIndexes.size() * 4, glyphIndexes.size() * 6);
    QVector4D *vp = (QVector4D *)geometry->vertexDataAsTexturedPoint2D();
    Q_ASSERT(geometry->sizeOfVertex() == sizeof(QVector4D));
    ushort *ip = geometry->indexDataAsUShort();

    bool supportsSubPixelPositions = fontD->fontEngine->supportsSubPixelPositions();
    for (int i=0; i<glyphIndexes.size(); ++i) {
         QPointF glyphPosition = glyphPositions.at(i) + position;
         QFixed subPixelPosition;
         if (supportsSubPixelPositions)
             subPixelPosition = fontD->fontEngine->subPixelPositionForX(QFixed::fromReal(glyphPosition.x()));

         QTextureGlyphCache::GlyphAndSubPixelPosition glyph(glyphIndexes.at(i), subPixelPosition);
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

QOpenGLTextureGlyphCache *QSGTextMaskMaterial::openglGlyphCache() const
{
    return static_cast<QOpenGLTextureGlyphCache *>(glyphCache());
}

QSGRhiTextureGlyphCache *QSGTextMaskMaterial::rhiGlyphCache() const
{
    return static_cast<QSGRhiTextureGlyphCache *>(glyphCache());
}

QSGMaterialShader *QSGTextMaskMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted)) {
        QSGRhiTextureGlyphCache *gc = rhiGlyphCache();
        const QFontEngine::GlyphFormat glyphFormat = gc->glyphFormat();
        switch (glyphFormat) {
        case QFontEngine::Format_ARGB:
            return new QSG32BitColorTextRhiShader(glyphFormat);
        case QFontEngine::Format_A32:
            return new QSG24BitTextMaskRhiShader(glyphFormat);
        case QFontEngine::Format_A8:
        default:
            return new QSG8BitTextMaskRhiShader(glyphFormat, gc->eightBitFormatIsAlphaSwizzled());
        }
    } else {
        switch (QFontEngine::GlyphFormat glyphFormat = glyphCache()->glyphFormat()) {
        case QFontEngine::Format_ARGB:
            return new QSG32BitColorTextShader(glyphFormat);
        case QFontEngine::Format_A32:
            return new QSG24BitTextMaskShader(glyphFormat);
        case QFontEngine::Format_A8:
        default:
            return new QSG8BitTextMaskShader(glyphFormat);
        }
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
    if (m_rhi) {
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

    } else {
        QSize glyphCacheSize(openglGlyphCache()->width(), openglGlyphCache()->height());
        if (glyphCacheSize != m_size) {
            if (m_texture)
                delete m_texture;
            m_texture = new QSGPlainTexture();
            m_texture->setTextureId(openglGlyphCache()->texture());
            m_texture->setTextureSize(QSize(openglGlyphCache()->width(), openglGlyphCache()->height()));
            m_texture->setOwnsTexture(false);
            m_size = glyphCacheSize;
            return true;
        }
        return false;
    }
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

QSGMaterialShader *QSGStyledTextMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted)) {
        QSGRhiTextureGlyphCache *gc = rhiGlyphCache();
        return new QSGStyledTextRhiShader(gc->glyphFormat(), gc->eightBitFormatIsAlphaSwizzled());
    } else {
        return new QSGStyledTextShader(glyphCache()->glyphFormat());
    }
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

QSGMaterialShader *QSGOutlinedTextMaterial::createShader() const
{
    if (flags().testFlag(RhiShaderWanted)) {
        QSGRhiTextureGlyphCache *gc = rhiGlyphCache();
        return new QSGOutlinedTextRhiShader(gc->glyphFormat(), gc->eightBitFormatIsAlphaSwizzled());
    } else {
        return new QSGOutlinedTextShader(glyphCache()->glyphFormat());
    }
}

QT_END_NAMESPACE
