/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt SceneGraph Raster Add-on.
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
    qreal m_pixelRatio;
    QMargins m_margins;
};

#endif // NINEPATCHNODE_H
