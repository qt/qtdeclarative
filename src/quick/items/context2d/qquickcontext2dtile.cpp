/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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
******************************************************************************/

#include "qquickcontext2dtile_p.h"

QT_BEGIN_NAMESPACE

QQuickContext2DTile::QQuickContext2DTile()
    : m_dirty(true)
    , m_rect(QRect(0, 0, 1, 1))
    , m_device(nullptr)
{
}

QQuickContext2DTile::~QQuickContext2DTile()
{
    if (m_painter.isActive())
        m_painter.end();
}

QPainter* QQuickContext2DTile::createPainter(bool smooth, bool antialiasing)
{
    if (m_painter.isActive())
        m_painter.end();

    aboutToDraw();
    if (m_device) {
        m_painter.begin(m_device);
        m_painter.resetTransform();
        m_painter.setCompositionMode(QPainter::CompositionMode_Source);

#ifdef QQUICKCONTEXT2D_DEBUG
        int v = 100;
        int gray = (m_rect.x() / m_rect.width() + m_rect.y() / m_rect.height()) % 2;
        if (gray)
            v = 150;
        m_painter.fillRect(QRect(0, 0, m_rect.width(), m_rect.height()), QColor(v, v, v, 255));
#endif

        if (antialiasing)
            m_painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, true);
        else
            m_painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing, false);

        if (smooth)
            m_painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
        else
            m_painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

        m_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        m_painter.translate(-m_rect.left(), -m_rect.top());
        m_painter.setClipRect(m_rect);
        m_painter.setClipping(false);
        return &m_painter;
    }

    return nullptr;
}

QQuickContext2DImageTile::QQuickContext2DImageTile()
    : QQuickContext2DTile()
{
}

QQuickContext2DImageTile::~QQuickContext2DImageTile()
{
}

void QQuickContext2DImageTile::setRect(const QRect& r)
{
    if (m_rect == r)
        return;
    m_rect = r;
    m_dirty = true;
    if (m_image.size() != r.size()) {
        m_image = QImage(r.size(), QImage::Format_ARGB32_Premultiplied);
    }
    m_device = &m_image;
}

QT_END_NAMESPACE

