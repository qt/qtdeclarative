#include "ninepatchnode.h"
#include "pixmaptexture.h"
#include "imagenode.h"

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
    m_pixelRatio = ratio;
}

void NinePatchNode::setPadding(qreal left, qreal top, qreal right, qreal bottom)
{
    m_margins = QMargins(qRound(left), qRound(top), qRound(right), qRound(bottom));
}

void NinePatchNode::update()
{
}

void NinePatchNode::paint(QPainter *painter)
{
    if (m_margins.isNull())
        painter->drawPixmap(m_bounds, m_pixmap, QRectF(0, 0, m_pixmap.width(), m_pixmap.height()));
    else
        SoftwareContext::qDrawBorderPixmap(painter, m_bounds.toRect(), m_margins, m_pixmap, QRect(0, 0, m_pixmap.width(), m_pixmap.height()),
                                           m_margins, Qt::StretchTile, QDrawBorderPixmap::DrawingHints(0));
}
