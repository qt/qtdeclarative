/****************************************************************************
**
** Copyright (C) 2014 Digia Plc
** All rights reserved.
** For any questions to Digia, please use contact form at http://qt.digia.com
**
** This file is part of the Qt Purchasing Add-on.
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
#ifndef GLYPHNODE_H
#define GLYPHNODE_H

#include <private/qsgadaptationlayer_p.h>

class GlyphNode : public QSGGlyphNode
{
public:
    GlyphNode();

    virtual void setGlyphs(const QPointF &position, const QGlyphRun &glyphs);
    virtual void setColor(const QColor &color);
    virtual void setStyle(QQuickText::TextStyle style);
    virtual void setStyleColor(const QColor &color);
    virtual QPointF baseLine() const;
    virtual void setPreferredAntialiasingMode(AntialiasingMode);
    virtual void update();

    void paint(QPainter *painter);

private:
    QPointF m_position;
    QGlyphRun m_glyphRun;
    QColor m_color;
    QSGGeometry m_geometry;
};

#endif // GLYPHNODE_H
