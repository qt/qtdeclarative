// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgsoftwarepixmaprenderer_p.h"
#include "qsgsoftwarecontext_p.h"

#include <QtQuick/QSGSimpleRectNode>

#include <QElapsedTimer>

Q_STATIC_LOGGING_CATEGORY(lcPixmapRenderer, "qt.scenegraph.softwarecontext.pixmapRenderer")

QT_BEGIN_NAMESPACE

QSGSoftwarePixmapRenderer::QSGSoftwarePixmapRenderer(QSGRenderContext *context)
    : QSGAbstractSoftwareRenderer(context)
{

}

QSGSoftwarePixmapRenderer::~QSGSoftwarePixmapRenderer()
{

}

void QSGSoftwarePixmapRenderer::renderScene()
{
    QSGRenderer::renderScene();
}

void QSGSoftwarePixmapRenderer::render()
{

}

void QSGSoftwarePixmapRenderer::render(QPaintDevice *target)
{
    QElapsedTimer renderTimer;

    // Setup background item
    setBackgroundRect(m_projectionRect.normalized(), qreal(1));
    setBackgroundColor(clearColor());

    renderTimer.start();
    buildRenderList();
    qint64 buildRenderListTime = renderTimer.restart();

    // Optimize Renderlist
    // Right now there is an assumption that when possible the same pixmap will
    // be reused.  So we can treat it like a backing store in that we can assume
    // that when the pixmap is not being resized, the data can be reused.  What is
    // different though is that everything should be marked as dirty on a resize.
    optimizeRenderList();
    qint64 optimizeRenderListTime = renderTimer.restart();

    if (!isOpaque() && target->devType() == QInternal::Pixmap) {
        // This fill here is wasteful, but necessary because it is the only way
        // to force a QImage based pixmap to have an alpha channel.
        static_cast<QPixmap *>(target)->fill(Qt::transparent);
    }

    QPainter painter(target);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setWindow(m_projectionRect);
    auto rc = static_cast<QSGSoftwareRenderContext *>(context());
    QPainter *prevPainter = rc->m_activePainter;
    rc->m_activePainter = &painter;

    QRegion paintedRegion = renderNodes(&painter);
    qint64 renderTime = renderTimer.elapsed();

    rc->m_activePainter = prevPainter;
    qCDebug(lcPixmapRenderer) << "pixmapRender" << paintedRegion << buildRenderListTime << optimizeRenderListTime << renderTime;
}

void QSGSoftwarePixmapRenderer::setProjectionRect(const QRect &projectionRect)
{
    m_projectionRect = projectionRect;
}

QT_END_NAMESPACE
