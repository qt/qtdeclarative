/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qsgd3d12rendercontext_p.h"
#include "qsgd3d12renderer_p.h"
#include "qsgd3d12texture_p.h"

QT_BEGIN_NAMESPACE

// NOTE: Avoid categorized logging. It is slow.

#define DECLARE_DEBUG_VAR(variable) \
    static bool debug_ ## variable() \
    { static bool value = qgetenv("QSG_RENDERER_DEBUG").contains(QT_STRINGIFY(variable)); return value; }

DECLARE_DEBUG_VAR(render)

QSGD3D12RenderContext::QSGD3D12RenderContext(QSGContext *ctx)
    : QSGRenderContext(ctx)
{
}

bool QSGD3D12RenderContext::isValid() const
{
    // The render thread sets an engine when it starts up and resets when it
    // quits. The rc is initialized and functional between those two points,
    // regardless of any calls to invalidate(). See setEngine().
    return m_engine != nullptr;
}

void QSGD3D12RenderContext::initialize(const InitParams *)
{
    if (m_initialized)
        return;

    m_initialized = true;
    emit initialized();
}

void QSGD3D12RenderContext::invalidate()
{
    if (!m_initialized)
        return;

    m_initialized = false;

    if (Q_UNLIKELY(debug_render()))
        qDebug("rendercontext invalidate engine %p, %d/%d/%d", m_engine,
               m_texturesToDelete.count(), m_textures.count(), m_fontEnginesToClean.count());

    qDeleteAll(m_texturesToDelete);
    m_texturesToDelete.clear();

    qDeleteAll(m_textures);
    m_textures.clear();

    for (QSet<QFontEngine *>::const_iterator it = m_fontEnginesToClean.constBegin(),
         end = m_fontEnginesToClean.constEnd(); it != end; ++it) {
        (*it)->clearGlyphCache(m_engine);
        if (!(*it)->ref.deref())
            delete *it;
    }
    m_fontEnginesToClean.clear();

    m_sg->renderContextInvalidated(this);
    emit invalidated();
}

QSGTexture *QSGD3D12RenderContext::createTexture(const QImage &image, uint flags) const
{
    Q_ASSERT(m_engine);
    QSGD3D12Texture *t = new QSGD3D12Texture(m_engine);
    t->create(image, flags);
    return t;
}

QSGRenderer *QSGD3D12RenderContext::createRenderer()
{
    return new QSGD3D12Renderer(this);
}

int QSGD3D12RenderContext::maxTextureSize() const
{
    return 16384; // D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION
}

void QSGD3D12RenderContext::renderNextFrame(QSGRenderer *renderer, uint fbo)
{
    static_cast<QSGD3D12Renderer *>(renderer)->renderScene(fbo);
}

void QSGD3D12RenderContext::setEngine(QSGD3D12Engine *engine)
{
    if (m_engine == engine)
        return;

    m_engine = engine;

    if (m_engine)
        initialize(nullptr);
}

QSGRendererInterface::GraphicsApi QSGD3D12RenderContext::graphicsApi() const
{
    return Direct3D12;
}

void *QSGD3D12RenderContext::getResource(QQuickWindow *window, Resource resource) const
{
    if (!m_engine) {
        qWarning("getResource: No D3D12 engine available yet (window not exposed?)");
        return nullptr;
    }
    // window can be ignored since the rendercontext and engine are both per window
    return m_engine->getResource(window, resource);
}

QSGRendererInterface::ShaderType QSGD3D12RenderContext::shaderType() const
{
    return HLSL;
}

QSGRendererInterface::ShaderCompilationTypes QSGD3D12RenderContext::shaderCompilationType() const
{
    return RuntimeCompilation | OfflineCompilation;
}

QSGRendererInterface::ShaderSourceTypes QSGD3D12RenderContext::shaderSourceType() const
{
    return ShaderSourceString | ShaderSourceFile | ShaderByteCode;
}

QT_END_NAMESPACE
