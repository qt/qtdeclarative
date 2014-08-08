#ifndef NINEPATCHNODE_H
#define NINEPATCHNODE_H

#include <private/qsgadaptationlayer_p.h>

class NinePatchNode : public QSGNinePatchNode
{
public:    
    NinePatchNode();

    virtual void setTexture(QSGTexture *texture);
    virtual void setBounds(const QRectF &bounds);
    virtual void setDevicePixelRatio(qreal ratio);
    virtual void setPadding(qreal left, qreal top, qreal right, qreal bottom);
    virtual void update();

    void paint(QPainter *painter);

private:
    QPixmap m_pixmap;
    QRectF m_bounds;
};

#endif // NINEPATCHNODE_H
