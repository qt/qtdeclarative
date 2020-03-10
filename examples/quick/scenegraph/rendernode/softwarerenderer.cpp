/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "softwarerenderer.h"
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QPainter>
#include <QPainterPath>

SoftwareRenderNode::~SoftwareRenderNode()
{
    releaseResources();
}

void SoftwareRenderNode::releaseResources()
{
}

void SoftwareRenderNode::render(const RenderState *renderState)
{
    Q_ASSERT(m_window);

    QSGRendererInterface *rif = m_window->rendererInterface();
    QPainter *p = static_cast<QPainter *>(rif->getResource(m_window, QSGRendererInterface::PainterResource));
    Q_ASSERT(p);

    const QRegion *clipRegion = renderState->clipRegion();
    if (clipRegion && !clipRegion->isEmpty())
        p->setClipRegion(*clipRegion, Qt::ReplaceClip); // must be done before setTransform

    p->setTransform(matrix()->toTransform());
    p->setOpacity(inheritedOpacity());

    const QPointF p0(m_width - 1, m_height - 1);
    const QPointF p1(0, 0);
    const QPointF p2(0, m_height - 1);
    QPainterPath path(p0);
    path.lineTo(p1);
    path.lineTo(p2);
    path.closeSubpath();

    QLinearGradient gradient(QPointF(0, 0), QPointF(m_width, m_height));
    gradient.setColorAt(0, Qt::green);
    gradient.setColorAt(1, Qt::red);

    p->fillPath(path, gradient);
}

QSGRenderNode::StateFlags SoftwareRenderNode::changedStates() const
{
    return {};
}

QSGRenderNode::RenderingFlags SoftwareRenderNode::flags() const
{
    return BoundedRectRendering;
}

QRectF SoftwareRenderNode::rect() const
{
    return QRect(0, 0, m_width, m_height);
}

void SoftwareRenderNode::sync(QQuickItem *item)
{
    m_window = item->window();
    m_width = item->width();
    m_height = item->height();
}
