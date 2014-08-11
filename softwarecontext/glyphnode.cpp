/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Purchasing Add-on.
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
#include "glyphnode.h"

GlyphNode::GlyphNode()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
{
    setMaterial((QSGMaterial*)1);
    setGeometry(&m_geometry);
}


void GlyphNode::setGlyphs(const QPointF &position, const QGlyphRun &glyphs)
{
    m_position = position;
    m_glyphRun = glyphs;
}

void GlyphNode::setColor(const QColor &color)
{
    m_color = color;
}

void GlyphNode::setStyle(QQuickText::TextStyle style)
{
}

void GlyphNode::setStyleColor(const QColor &color)
{
}

QPointF GlyphNode::baseLine() const
{
    return QPointF();
}

void GlyphNode::setPreferredAntialiasingMode(QSGGlyphNode::AntialiasingMode)
{
}

void GlyphNode::update()
{
}

void GlyphNode::paint(QPainter *painter)
{
    painter->setPen(m_color);
    painter->drawGlyphRun(m_position - QPointF(0, m_glyphRun.rawFont().ascent()), m_glyphRun);
}
