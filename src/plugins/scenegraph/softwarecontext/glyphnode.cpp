/******************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick 2d Renderer module.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
******************************************************************************/

#include "glyphnode.h"

GlyphNode::GlyphNode()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 0)
    , m_style(QQuickText::Normal)
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
    m_style = style;
}

void GlyphNode::setStyleColor(const QColor &color)
{
    m_styleColor = color;
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
    painter->setBrush(QBrush());
    QPointF pos = m_position - QPointF(0, m_glyphRun.rawFont().ascent());

    switch (m_style) {
    case QQuickText::Normal: break;
    case QQuickText::Outline:
        painter->setPen(m_styleColor);
        painter->drawGlyphRun(pos + QPointF(0, 1), m_glyphRun);
        painter->drawGlyphRun(pos + QPointF(0, -1), m_glyphRun);
        painter->drawGlyphRun(pos + QPointF(1, 0), m_glyphRun);
        painter->drawGlyphRun(pos + QPointF(-1, 0), m_glyphRun);
        break;
    case QQuickText::Raised:
        painter->setPen(m_styleColor);
        painter->drawGlyphRun(pos + QPointF(0, 1), m_glyphRun);
        break;
    case QQuickText::Sunken:
        painter->setPen(m_styleColor);
        painter->drawGlyphRun(pos + QPointF(0, -1), m_glyphRun);
        break;
    }

    painter->setPen(m_color);
    painter->drawGlyphRun(pos, m_glyphRun);
}
