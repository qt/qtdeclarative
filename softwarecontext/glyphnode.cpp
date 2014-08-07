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
