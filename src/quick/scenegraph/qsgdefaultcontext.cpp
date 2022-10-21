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

#include "qsgdefaultcontext_p.h"

#include <QtQuick/private/qsgdefaultinternalrectanglenode_p.h>
#include <QtQuick/private/qsgdefaultinternalimagenode_p.h>
#include <QtQuick/private/qsgdefaultpainternode_p.h>
#include <QtQuick/private/qsgdefaultglyphnode_p.h>
#include <QtQuick/private/qsgdistancefieldglyphnode_p.h>
#include <QtQuick/private/qsgdistancefieldglyphnode_p_p.h>
#include <QtQuick/private/qsgrhisupport_p.h>
#include <QtQuick/private/qsgrhilayer_p.h>
#include <QtQuick/private/qsgdefaultrendercontext_p.h>
#include <QtQuick/private/qsgdefaultrectanglenode_p.h>
#include <QtQuick/private/qsgdefaultimagenode_p.h>
#include <QtQuick/private/qsgdefaultninepatchnode_p.h>
#if QT_CONFIG(quick_sprite)
#include <QtQuick/private/qsgdefaultspritenode_p.h>
#endif
#include <QtQuick/private/qsgrhishadereffectnode_p.h>

#include <QOpenGLContext>

#include <QtQuick/private/qquickwindow_p.h>
#include <QtQuick/private/qquickitem_p.h>

#include <private/qqmlglobal_p.h>

#include <algorithm>

#include <QtGui/private/qrhi_p.h>
#include <QtGui/private/qrhigles2_p.h>

QT_BEGIN_NAMESPACE

namespace QSGMultisampleAntialiasing {
    class ImageNode : public QSGDefaultInternalImageNode {
    public:
        ImageNode(QSGDefaultRenderContext *rc) : QSGDefaultInternalImageNode(rc) { }
        void setAntialiasing(bool) override { }
    };

    class RectangleNode : public QSGDefaultInternalRectangleNode {
    public:
        void setAntialiasing(bool) override { }
    };
}

DEFINE_BOOL_CONFIG_OPTION(qmlDisableDistanceField, QML_DISABLE_DISTANCEFIELD)

QSGDefaultContext::QSGDefaultContext(QObject *parent)
    : QSGContext (parent)
    , m_antialiasingMethod(QSGContext::UndecidedAntialiasing)
    , m_distanceFieldDisabled(qmlDisableDistanceField())
    , m_distanceFieldAntialiasing(QSGGlyphNode::HighQualitySubPixelAntialiasing)
    , m_distanceFieldAntialiasingDecided(false)
{
    if (Q_UNLIKELY(!qEnvironmentVariableIsEmpty("QSG_DISTANCEFIELD_ANTIALIASING"))) {
        const QByteArray mode = qgetenv("QSG_DISTANCEFIELD_ANTIALIASING");
        m_distanceFieldAntialiasingDecided = true;
        if (mode == "subpixel")
            m_distanceFieldAntialiasing = QSGGlyphNode::HighQualitySubPixelAntialiasing;
        else if (mode == "subpixel-lowq")
            m_distanceFieldAntialiasing = QSGGlyphNode::LowQualitySubPixelAntialiasing;
        else if (mode == "gray")
            m_distanceFieldAntialiasing = QSGGlyphNode::GrayAntialiasing;
    }

    // Adds compatibility with Qt 5.3 and earlier's QSG_RENDER_TIMING
    if (qEnvironmentVariableIsSet("QSG_RENDER_TIMING")) {
        const_cast<QLoggingCategory &>(QSG_LOG_TIME_GLYPH()).setEnabled(QtDebugMsg, true);
        const_cast<QLoggingCategory &>(QSG_LOG_TIME_TEXTURE()).setEnabled(QtDebugMsg, true);
        const_cast<QLoggingCategory &>(QSG_LOG_TIME_RENDERER()).setEnabled(QtDebugMsg, true);
        const_cast<QLoggingCategory &>(QSG_LOG_TIME_RENDERLOOP()).setEnabled(QtDebugMsg, true);
        const_cast<QLoggingCategory &>(QSG_LOG_TIME_COMPILATION()).setEnabled(QtDebugMsg, true);
    }
}

QSGDefaultContext::~QSGDefaultContext()
{

}

void QSGDefaultContext::renderContextInitialized(QSGRenderContext *renderContext)
{
    m_mutex.lock();

    auto rc = static_cast<const QSGDefaultRenderContext *>(renderContext);
    if (m_antialiasingMethod == UndecidedAntialiasing) {
        if (Q_UNLIKELY(qEnvironmentVariableIsSet("QSG_ANTIALIASING_METHOD"))) {
            const QByteArray aaType = qgetenv("QSG_ANTIALIASING_METHOD");
            if (aaType == "msaa")
                m_antialiasingMethod = MsaaAntialiasing;
            else if (aaType == "vertex")
                m_antialiasingMethod = VertexAntialiasing;
        }
        if (m_antialiasingMethod == UndecidedAntialiasing)
            m_antialiasingMethod = rc->msaaSampleCount() > 1 ? MsaaAntialiasing : VertexAntialiasing;
    }

#if QT_CONFIG(opengl)
    // With OpenGL ES, use GrayAntialiasing, unless
    // some value had been requested explicitly. This could not be decided
    // before without a context. Now the context is ready.
    if (!m_distanceFieldAntialiasingDecided) {
        m_distanceFieldAntialiasingDecided = true;
        Q_ASSERT(rc->rhi());
        if (rc->rhi()->backend() == QRhi::OpenGLES2
                && static_cast<const QRhiGles2NativeHandles *>(rc->rhi()->nativeHandles())->context->isOpenGLES())
        {
            m_distanceFieldAntialiasing = QSGGlyphNode::GrayAntialiasing;
        }
    }
#endif

    m_mutex.unlock();
}

void QSGDefaultContext::renderContextInvalidated(QSGRenderContext *)
{
}

QSGRenderContext *QSGDefaultContext::createRenderContext()
{
    return new QSGDefaultRenderContext(this);
}

QSGInternalRectangleNode *QSGDefaultContext::createInternalRectangleNode()
{
    return m_antialiasingMethod == MsaaAntialiasing
            ? new QSGMultisampleAntialiasing::RectangleNode
            : new QSGDefaultInternalRectangleNode;
}

QSGInternalImageNode *QSGDefaultContext::createInternalImageNode(QSGRenderContext *renderContext)
{
    return m_antialiasingMethod == MsaaAntialiasing
            ? new QSGMultisampleAntialiasing::ImageNode(static_cast<QSGDefaultRenderContext *>(renderContext))
            : new QSGDefaultInternalImageNode(static_cast<QSGDefaultRenderContext *>(renderContext));
}

QSGPainterNode *QSGDefaultContext::createPainterNode(QQuickPaintedItem *item)
{
    return new QSGDefaultPainterNode(item);
}

QSGGlyphNode *QSGDefaultContext::createGlyphNode(QSGRenderContext *rc,
                                                 bool preferNativeGlyphNode,
                                                 int renderTypeQuality)
{
    if (m_distanceFieldDisabled || preferNativeGlyphNode) {
        return new QSGDefaultGlyphNode(rc);
    } else {
        QSGDistanceFieldGlyphNode *node = new QSGDistanceFieldGlyphNode(rc);
        node->setPreferredAntialiasingMode(m_distanceFieldAntialiasing);
        node->setRenderTypeQuality(renderTypeQuality);
        return node;
    }
}

QSGLayer *QSGDefaultContext::createLayer(QSGRenderContext *renderContext)
{
    return new QSGRhiLayer(renderContext);
}

QSurfaceFormat QSGDefaultContext::defaultSurfaceFormat() const
{
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    // These depend solely on the env.vars., not QQuickGraphicsConfiguration
    // since that does not have a flag that maps 100% to QSG_NO_xx_BUFFER.
    static bool useDepth = qEnvironmentVariableIsEmpty("QSG_NO_DEPTH_BUFFER");
    static bool useStencil = qEnvironmentVariableIsEmpty("QSG_NO_STENCIL_BUFFER");
    static bool enableDebug = qEnvironmentVariableIsSet("QSG_OPENGL_DEBUG");
    if (useDepth && format.depthBufferSize() == -1)
        format.setDepthBufferSize(24);
    else if (!useDepth)
        format.setDepthBufferSize(0);
    if (useStencil && format.stencilBufferSize() == -1)
        format.setStencilBufferSize(8);
    else if (!useStencil)
        format.setStencilBufferSize(0);
    if (enableDebug)
        format.setOption(QSurfaceFormat::DebugContext);
    if (QQuickWindow::hasDefaultAlphaBuffer())
        format.setAlphaBufferSize(8);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    return format;
}

void QSGDefaultContext::setDistanceFieldEnabled(bool enabled)
{
    m_distanceFieldDisabled = !enabled;
}

bool QSGDefaultContext::isDistanceFieldEnabled() const
{
    return !m_distanceFieldDisabled;
}

QSGRendererInterface *QSGDefaultContext::rendererInterface(QSGRenderContext *renderContext)
{
    Q_UNUSED(renderContext);
    return this;
}

QSGRectangleNode *QSGDefaultContext::createRectangleNode()
{
    return new QSGDefaultRectangleNode;
}

QSGImageNode *QSGDefaultContext::createImageNode()
{
    return new QSGDefaultImageNode;
}

QSGNinePatchNode *QSGDefaultContext::createNinePatchNode()
{
    return new QSGDefaultNinePatchNode;
}

#if QT_CONFIG(quick_sprite)
QSGSpriteNode *QSGDefaultContext::createSpriteNode()
{
    return new QSGDefaultSpriteNode;
}
#endif

QSGGuiThreadShaderEffectManager *QSGDefaultContext::createGuiThreadShaderEffectManager()
{
    if (QSGRhiSupport::instance()->isRhiEnabled())
        return new QSGRhiGuiThreadShaderEffectManager;

    return nullptr;
}

QSGShaderEffectNode *QSGDefaultContext::createShaderEffectNode(QSGRenderContext *renderContext)
{
    if (QSGRhiSupport::instance()->isRhiEnabled())
        return new QSGRhiShaderEffectNode(static_cast<QSGDefaultRenderContext *>(renderContext));

    return nullptr;
}

QSGRendererInterface::GraphicsApi QSGDefaultContext::graphicsApi() const
{
    return QSGRhiSupport::instance()->graphicsApi();
}

void *QSGDefaultContext::getResource(QQuickWindow *window, Resource resource) const
{
    if (!window)
        return nullptr;

    // Unlike the graphicsApi and shaderType and similar queries, getting a
    // native resource is only possible when there is an initialized
    // rendercontext, or rather, only within rendering a frame, as per
    // QSGRendererInterface docs. This is good since getting some things is
    // only possible within a beginFrame - endFrame with the RHI.

    const QSGDefaultRenderContext *rc = static_cast<const QSGDefaultRenderContext *>(
                QQuickWindowPrivate::get(window)->context);
    QSGRhiSupport *rhiSupport = QSGRhiSupport::instance();

#if QT_CONFIG(vulkan)
    if (resource == VulkanInstanceResource)
        return window->vulkanInstance();
#endif
    return const_cast<void *>(rhiSupport->rifResource(resource, rc, window));
}

QSGRendererInterface::ShaderType QSGDefaultContext::shaderType() const
{
    return QSGRhiSupport::instance()->isRhiEnabled() ? RhiShader : GLSL;
}

QSGRendererInterface::ShaderCompilationTypes QSGDefaultContext::shaderCompilationType() const
{
    return QSGRhiSupport::instance()->isRhiEnabled() ? OfflineCompilation : RuntimeCompilation;
}

QSGRendererInterface::ShaderSourceTypes QSGDefaultContext::shaderSourceType() const
{
    return QSGRhiSupport::instance()->isRhiEnabled() ? ShaderSourceFile : (ShaderSourceString | ShaderSourceFile);
}

QT_END_NAMESPACE

static void initResources()
{
    Q_INIT_RESOURCE(scenegraph_shaders);
}

Q_CONSTRUCTOR_FUNCTION(initResources)
