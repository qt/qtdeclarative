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

#include "qsgsoftwarepixmaprenderer_p.h"
#include "qsgsoftwarecontext_p.h"

#include <QtQuick/QSGSimpleRectNode>

#include <QElapsedTimer>

Q_LOGGING_CATEGORY(lcPixmapRenderer, "qt.scenegraph.softwarecontext.pixmapRenderer")

QT_BEGIN_NAMESPACE

QSGSoftwarePixmapRenderer::QSGSoftwarePixmapRenderer(QSGRenderContext *context)
    : QSGAbstractSoftwareRenderer(context)
{

}

QSGSoftwarePixmapRenderer::~QSGSoftwarePixmapRenderer()
{

}

void QSGSoftwarePixmapRenderer::renderScene(uint)
{
    class B : public QSGBindable
    {
    public:
        void bind() const override { }
    } bindable;
    QSGRenderer::renderScene(bindable);
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
