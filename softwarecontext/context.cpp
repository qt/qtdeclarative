/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Scenegraph Playground module of the Qt Toolkit.
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

#include "context.h"

#include "rectanglenode.h"
#include "imagenode.h"
#include "pixmaptexture.h"

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
    if (!backingStore)
        backingStore.reset(new QBackingStore(currentWindow));

    if (backingStore->size() != currentWindow->size())
        backingStore->resize(currentWindow->size());

    const QRect rect(0, 0, currentWindow->width(), currentWindow->height());
    backingStore->beginPaint(rect);

    QPaintDevice *device = backingStore->paintDevice();
    QPainter painter(device);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(rect, Qt::white);
    renderNode(&painter, rootNode());

    backingStore->endPaint();
    backingStore->flush(rect);
}

void Renderer::renderNode(QPainter *painter, QSGNode *node)
{
    bool restore = false;

    if (node->type() == QSGNode::TransformNodeType) {
        QSGTransformNode *tn = static_cast<QSGTransformNode*>(node);
        painter->save();
        restore = true;
        painter->setTransform(tn->matrix().toTransform(), /*combine*/true);
    } else if (node->type() == QSGNode::ClipNodeType) {
        QSGClipNode *cn = static_cast<QSGClipNode*>(node);
        painter->save();
        restore = true;
        painter->setClipRect(cn->clipRect(), Qt::IntersectClip);
    }

    node->paint(painter);

    for (QSGNode *child = node->firstChild(); child; child = child->nextSibling())
        renderNode(painter, child);

    if (restore)
        painter->restore();
}

RenderContext::RenderContext(QSGContext *ctx)
    : QSGRenderContext(ctx)
    , currentWindow(0)
{
}
Context::Context(QObject *parent)
    : QSGContext(parent)
{
}

QSGRectangleNode *Context::createRectangleNode()
{
    return new RectangleNode();
}

QSGImageNode *Context::createImageNode()
{
    return new ImageNode();
}

void RenderContext::initialize(QOpenGLContext *context)
{
    QSGRenderContext::initialize(context);
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
