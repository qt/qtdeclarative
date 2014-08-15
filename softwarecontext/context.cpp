/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt SceneGraph Raster Add-on.
**
** $QT_BEGIN_LICENSE$
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.
**
** If you have questions regarding the use of this file, please use
** contact form at http://qt.digia.com
** $QT_END_LICENSE$
**
****************************************************************************/

#include "context.h"

#include "rectanglenode.h"
#include "imagenode.h"
#include "pixmaptexture.h"
#include "glyphnode.h"
#include "ninepatchnode.h"
#include "renderingvisitor.h"
#include "softwarelayer.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>

#include <QtGui/QWindow>

#include <QtQuick/QSGFlatColorMaterial>
#include <QtQuick/QSGVertexColorMaterial>
#include <QtQuick/QSGOpaqueTextureMaterial>
#include <QtQuick/QSGTextureMaterial>
#include <private/qsgdefaultimagenode_p.h>
#include <private/qsgdefaultrectanglenode_p.h>
#include <private/qsgdistancefieldglyphnode_p_p.h>
#include <private/qsgdefaultglyphnode_p.h>

#ifndef QSG_NO_RENDERER_TIMING
static bool qsg_render_timing = !qgetenv("QSG_RENDER_TIMING").isEmpty();
#endif

namespace SoftwareContext
{

Renderer::Renderer(QSGRenderContext *context)
    : QSGRenderer(context)
{
}

void Renderer::renderScene(GLuint fboId)
{
    class B : public QSGBindable
    {
    public:
        void bind() const { }
    } bindable;
    QSGRenderer::renderScene(bindable);
}

void Renderer::render()
{
    QWindow *currentWindow = static_cast<RenderContext*>(m_context)->currentWindow;
    if (!m_backingStore)
        m_backingStore.reset(new QBackingStore(currentWindow));

    if (m_backingStore->size() != currentWindow->size())
        m_backingStore->resize(currentWindow->size());

    const QRect rect(0, 0, currentWindow->width(), currentWindow->height());
    m_backingStore->beginPaint(rect);

    QPaintDevice *device = m_backingStore->paintDevice();
    QPainter painter(device);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect, clearColor());
    RenderingVisitor(&painter).visitChildren(rootNode());

    m_backingStore->endPaint();
    m_backingStore->flush(rect);
}

void Renderer::nodeChanged(QSGNode *node, QSGNode::DirtyState state)
{

    QSGRenderer::nodeChanged(node, state);
}

PixmapRenderer::PixmapRenderer(QSGRenderContext *context)
    : QSGRenderer(context)
{

}

void PixmapRenderer::renderScene(GLuint)
{
    Q_UNREACHABLE();
}

void PixmapRenderer::render()
{
    Q_UNREACHABLE();
}

void PixmapRenderer::render(QPixmap *target)
{
    const QRect rect(0, 0, target->width(), target->height());
    target->fill(clearColor());
    QPainter painter(target);
    painter.setRenderHint(QPainter::Antialiasing);

    RenderingVisitor(&painter).visitChildren(rootNode());
}

RenderContext::RenderContext(QSGContext *ctx)
    : QSGRenderContext(ctx)
    , currentWindow(0)
    , m_initialized(false)
{
}
Context::Context(QObject *parent)
    : QSGContext(parent)
{
    setDistanceFieldEnabled(false);
}

QSGRectangleNode *Context::createRectangleNode()
{
    return new RectangleNode();
}

QSGImageNode *Context::createImageNode()
{
    return new ImageNode();
}

QSGGlyphNode *Context::createGlyphNode(QSGRenderContext *rc, bool preferNativeGlyphNode)
{
    Q_UNUSED(rc);
    Q_UNUSED(preferNativeGlyphNode);
    return new GlyphNode();
}

QSGNinePatchNode *Context::createNinePatchNode()
{
    return new NinePatchNode();
}

QSGLayer *Context::createLayer(QSGRenderContext *renderContext)
{
    return new SoftwareLayer(renderContext);
}

void RenderContext::initialize(QOpenGLContext *context)
{
    Q_UNUSED(context)
    Q_UNREACHABLE();
}

void RenderContext::initializeIfNeeded()
{
    if (m_initialized)
        return;
    m_initialized = true;
    emit initialized();
}

void RenderContext::invalidate()
{
    QSGRenderContext::invalidate();
}

QSGTexture *RenderContext::createTexture(const QImage &image) const
{
    return new PixmapTexture(image);
}

QSGTexture *RenderContext::createTextureNoAtlas(const QImage &image) const
{
    return QSGRenderContext::createTextureNoAtlas(image);
}

QSGRenderer *RenderContext::createRenderer()
{
    return new Renderer(this);
}


void RenderContext::renderNextFrame(QSGRenderer *renderer, GLuint fbo)
{
    QSGRenderContext::renderNextFrame(renderer, fbo);
}

} // namespace
