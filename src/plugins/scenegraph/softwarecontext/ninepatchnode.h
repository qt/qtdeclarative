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
