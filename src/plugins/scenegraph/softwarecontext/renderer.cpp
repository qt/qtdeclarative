/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick 2D Renderer module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "renderer.h"

#include "renderablenodeupdater.h"
#include "renderlistbuilder.h"
#include "context.h"
#include "renderablenode.h"

#include <QtGui/QWindow>
#include <QtQuick/QSGSimpleRectNode>

#include <QElapsedTimer>

Q_LOGGING_CATEGORY(lcRenderer, "qt.scenegraph.softwarecontext.renderer")

namespace SoftwareContext {

Renderer::Renderer(QSGRenderContext *context)
    : AbstractSoftwareRenderer(context)
{
}

Renderer::~Renderer()
{
}

void Renderer::renderScene(GLuint)
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
    QElapsedTimer renderTimer;

    QWindow *currentWindow = static_cast<RenderContext*>(m_context)->currentWindow;
    if (!m_backingStore)
        m_backingStore.reset(new QBackingStore(currentWindow));

    if (m_backingStore->size() != currentWindow->size()) {
        m_backingStore->resize(currentWindow->size());
    }

    setBackgroundColor(clearColor());
    setBackgroundSize(currentWindow->size());

    const QRect rect(0, 0, currentWindow->width(), currentWindow->height());
    m_backingStore->beginPaint(rect);

    QPaintDevice *device = m_backingStore->paintDevice();
#ifndef QTQUICK2D_DEBUG_FLUSH
    QPainter painter(device);
#else
    if (m_outputBuffer.size() != m_backingStore->size()) {
        m_outputBuffer = QImage(m_backingStore->size(), QImage::Format_ARGB32_Premultiplied);
        m_outputBuffer.fill(Qt::transparent);
    }
    QPainter painter(&m_outputBuffer);
#endif
    painter.setRenderHint(QPainter::Antialiasing);

    // Build Renderlist
    // The renderlist is created by visiting each node in the tree and when a
    // renderable node is reach, we find the coorosponding RenderableNode object
    // and append it to the renderlist.  At this point the RenderableNode object
    // should not need any further updating so it is just a matter of appending
    // RenderableNodes
    renderTimer.start();
    buildRenderList();
    qint64 buildRenderListTime = renderTimer.restart();

    // Optimize Renderlist
    // This is a pass through the renderlist to determine what actually needs to
    // be painted.  Without this pass the renderlist will simply render each item
    // from back to front, with a high potential for overdraw.  It would also lead
    // to the entire window being flushed every frame.  The objective of the
    // optimization pass is to only paint dirty nodes that are not occuluded.  A
    // side effect of this is that additional nodes may need to be marked dirty to
    // force a repaint.  It is also important that any item that needs to be
    // repainted only paints what is needed, via the use of clip regions.
    optimizeRenderList();
    qint64 optimizeRenderListTime = renderTimer.restart();

    // Render the contents Renderlist
    QRegion dirtyRegion = renderNodes(&painter);
    qint64 renderTime = renderTimer.elapsed();

    qCDebug(lcRenderer) << "render" << dirtyRegion << buildRenderListTime << optimizeRenderListTime << renderTime;

#ifdef QTQUICK2D_DEBUG_FLUSH
    // Keep up with the last 5 flushes
    if (m_previousFlushes.count() == 5)
        m_previousFlushes.pop_front();
    m_previousFlushes.append(dirtyRegion);

    QPainter backingStorePainter(device);
    backingStorePainter.drawImage(QRect(0, 0, m_backingStore->size().width(), m_backingStore->size().height()), m_outputBuffer, m_outputBuffer.rect());
    QPen pen(Qt::NoPen);
    QBrush brush(QColor(255, 0, 0, 50));
    backingStorePainter.setPen(pen);
    backingStorePainter.setBrush(brush);
    for (auto region : qAsConst(m_previousFlushes)) {
        backingStorePainter.drawRects(region.rects());
    }
    m_backingStore->endPaint();

    m_backingStore->flush(rect);
#else
    m_backingStore->endPaint();
    // Flush the updated regions to the window
    m_backingStore->flush(dirtyRegion);
#endif
}

} // namespace
