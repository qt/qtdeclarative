#ifndef RECTANGLENODE_H
#define RECTANGLENODE_H

#include <private/qsgadaptationlayer_p.h>

#include <QPen>
#include <QBrush>

class RectangleNode : public QSGRectangleNode
{
public:
    RectangleNode();

    virtual void setRect(const QRectF &rect);
    virtual void setColor(const QColor &color);
    virtual void setPenColor(const QColor &color);
    virtual void setPenWidth(qreal width);
    virtual void setGradientStops(const QGradientStops &stops);
    virtual void setRadius(qreal radius);
    virtual void setAntialiasing(bool antialiasing) { Q_UNUSED(antialiasing) }
    virtual void setAligned(bool aligned);

    virtual void update();

    virtual void paint(QPainter *);

private:
    QPen m_pen;
    QBrush m_brush;
    QRectF m_rect;
};

#endif // RECTANGLENODE_H
