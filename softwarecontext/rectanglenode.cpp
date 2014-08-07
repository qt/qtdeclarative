#include "rectanglenode.h"

RectangleNode::RectangleNode()
{
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}

void RectangleNode::setRect(const QRectF &rect)
{
    m_rect = rect;
}

void RectangleNode::setColor(const QColor &color)
{
    m_brush = color;
}

void RectangleNode::setPenColor(const QColor &color)
{
    m_pen.setColor(color);
}

void RectangleNode::setPenWidth(qreal width)
{
    m_pen.setWidthF(width);
}

void RectangleNode::setGradientStops(const QGradientStops &stops)
{
}

void RectangleNode::setRadius(qreal radius)
{
}

void RectangleNode::setAligned(bool aligned)
{
}

void RectangleNode::update()
{
}

void RectangleNode::paint(QPainter *painter)
{
    painter->setPen(m_pen);
    painter->setBrush(m_brush);
    painter->drawRect(m_rect);
}
