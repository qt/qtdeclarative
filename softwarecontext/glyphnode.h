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

    virtual void paint(QPainter *painter);

private:
    QPointF m_position;
    QGlyphRun m_glyphRun;
    QColor m_color;
    QSGGeometry m_geometry;
};

#endif // GLYPHNODE_H
