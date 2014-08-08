#include "rectanglenode.h"

SimpleRectangleNode::SimpleRectangleNode(const QRectF &rect, const QColor &color)
    : m_rect(rect)
    , m_color(color)
{
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}

void SimpleRectangleNode::setRect(const QRectF &rect)
{
    m_rect = rect;
}

void SimpleRectangleNode::setColor(const QColor &color)
{
    m_color = color;
}

void SimpleRectangleNode::update()
{
}

void SimpleRectangleNode::paint(QPainter *p)
{
    p->fillRect(m_rect, m_color);
}


RectangleNode::RectangleNode()
    : m_penWidth(0)
    , m_radius(0)
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
    m_color = color;
}

void RectangleNode::setPenColor(const QColor &color)
{
    m_penColor = color;
}

void RectangleNode::setPenWidth(qreal width)
{
    m_penWidth = width;
}

void RectangleNode::setGradientStops(const QGradientStops &stops)
{
    m_stops = stops;
}

void RectangleNode::setRadius(qreal radius)
{
    m_radius = radius;
}

void RectangleNode::setAligned(bool /*aligned*/)
{
}

void RectangleNode::update()
{
    if (!m_penWidth || m_penColor == Qt::transparent) {
        m_pen = Qt::NoPen;
    } else {
        m_pen = QPen(m_penColor);
        m_pen.setWidthF(m_penWidth);
    }

    if (!m_stops.isEmpty()) {
        QLinearGradient gradient(QPoint(0,0), QPoint(0,1));
        gradient.setStops(m_stops);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        m_brush = QBrush(gradient);
    } else {
        m_brush = QBrush(m_color);
    }
}

void RectangleNode::paint(QPainter *painter)
{
    painter->setPen(m_pen);
    painter->setBrush(m_brush);
    if (m_radius)
        painter->drawRoundedRect(m_rect, m_radius, m_radius);
    else
        painter->drawRect(m_rect);
}
