#include "ninepatchnode.h"
#include "pixmaptexture.h"

NinePatchNode::NinePatchNode()
{
    setMaterial((QSGMaterial*)1);
    setGeometry((QSGGeometry*)1);
}

void NinePatchNode::setTexture(QSGTexture *texture)
{
    PixmapTexture *pt = qobject_cast<PixmapTexture*>(texture);
    if (!pt) {
        qWarning() << "Image used with invalid texture format.";
        return;
    }
    m_pixmap = pt->pixmap();
}

void NinePatchNode::setBounds(const QRectF &bounds)
{
    m_bounds = bounds;
}

void NinePatchNode::setDevicePixelRatio(qreal ratio)
{

}

void NinePatchNode::setPadding(int left, int top, int right, int bottom)
{

}

void NinePatchNode::update()
{

}

void NinePatchNode::paint(QPainter *painter)
{
    painter->drawPixmap(m_bounds, m_pixmap, QRectF(0, 0, m_pixmap.width(), m_pixmap.height()));
}
