/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsgcontext2dtile_p.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLFramebufferObjectFormat>
#include <QOpenGLPaintDevice>

QSGContext2DTile::QSGContext2DTile()
    : m_dirty(true)
    , m_rect(QRect(0, 0, 1, 1))
    , m_device(0)
{
}

QSGContext2DTile::~QSGContext2DTile()
{
    if (m_painter.isActive())
        m_painter.end();
}

QPainter* QSGContext2DTile::createPainter(bool smooth)
{
    if (m_painter.isActive())
        m_painter.end();

    if (m_device) {
        aboutToDraw();
        m_painter.begin(m_device);
        m_painter.resetTransform();
        m_painter.setCompositionMode(QPainter::CompositionMode_Source);

#ifdef QSGCONTEXT2D_DEBUG
        int v = 100;
        int gray = (m_rect.x() / m_rect.width() + m_rect.y() / m_rect.height()) % 2;
        if (gray)
            v = 150;
        m_painter.fillRect(QRect(0, 0, m_rect.width(), m_rect.height()), QColor(v, v, v, 255));
#endif
        if (smooth)
            m_painter.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing
                                   | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
        else
            m_painter.setRenderHints(QPainter::Antialiasing | QPainter::HighQualityAntialiasing
                                     | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform, false);

        m_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        m_painter.translate(-m_rect.left(), -m_rect.top());
        m_painter.setClipRect(m_rect);
        m_painter.setClipping(false);
        return &m_painter;
    }

    return 0;
}

QSGContext2DFBOTile::QSGContext2DFBOTile()
    : QSGContext2DTile()
    , m_fbo(0)
{
}


QSGContext2DFBOTile::~QSGContext2DFBOTile()
{
    delete m_fbo;
}

void QSGContext2DFBOTile::aboutToDraw()
{
    m_fbo->bind();
    if (!m_device) {
        QOpenGLPaintDevice *gl_device = new QOpenGLPaintDevice(rect().size());
        m_device = gl_device;
        QPainter p(m_device);
        p.fillRect(QRectF(0, 0, m_fbo->width(), m_fbo->height()), QColor(qRgba(0, 0, 0, 0)));
        p.end();
    }
}

void QSGContext2DFBOTile::drawFinished()
{
    m_fbo->release();
}

void QSGContext2DFBOTile::setRect(const QRect& r)
{
    if (m_rect == r)
        return;
    m_rect = r;
    m_dirty = true;
    if (!m_fbo || m_fbo->size() != r.size()) {
        QOpenGLFramebufferObjectFormat format;
        format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
        format.setInternalTextureFormat(GL_RGBA);
        format.setMipmap(false);

        if (m_painter.isActive())
            m_painter.end();

        delete m_fbo;
        m_fbo = new QOpenGLFramebufferObject(r.size(), format);
    }
}


QSGContext2DImageTile::QSGContext2DImageTile()
    : QSGContext2DTile()
{
}

QSGContext2DImageTile::~QSGContext2DImageTile()
{
}

void QSGContext2DImageTile::setRect(const QRect& r)
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