// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgbasicinternalrectanglenode_p.h"

#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE

namespace
{
    struct Color4ub
    {
        unsigned char r, g, b, a;
    };

    Color4ub operator *(Color4ub c, float t) { c.a *= t; c.r *= t; c.g *= t; c.b *= t; return c; }
    Color4ub operator +(Color4ub a, Color4ub b) {  a.a += b.a; a.r += b.r; a.g += b.g; a.b += b.b; return a; }

    inline Color4ub colorToColor4ub(const QColor &c)
    {
        float r, g, b, a;
        c.getRgbF(&r, &g, &b, &a);
        Color4ub color = { uchar(qRound(r * a * 255)),
                           uchar(qRound(g * a * 255)),
                           uchar(qRound(b * a * 255)),
                           uchar(qRound(a * 255))
                         };
        return color;
    }

    // Same layout as QSGGeometry::ColoredPoint2D, but uses Color4ub for convenience.
    struct Vertex
    {
        float x, y;
        Color4ub color;

        void set(float primary, float secondary, Color4ub ncolor, bool vertical)
        {
            if (vertical) {
                x = secondary; y = primary;
            } else {
                x = primary; y = secondary;
            }
            color = ncolor;
        }
    };

    struct SmoothVertex : public Vertex
    {
        float dx, dy;

        void set(float primary, float secondary, Color4ub ncolor, float dPrimary, float dSecondary, bool vertical)
        {
            Vertex::set(primary, secondary, ncolor, vertical);
            if (vertical) {
                dx = dSecondary; dy = dPrimary;
            } else {
                dx = dPrimary; dy = dSecondary;
            }
        }
    };

    const QSGGeometry::AttributeSet &smoothAttributeSet()
    {
        static QSGGeometry::Attribute data[] = {
            QSGGeometry::Attribute::createWithAttributeType(0, 2, QSGGeometry::FloatType, QSGGeometry::PositionAttribute),
            QSGGeometry::Attribute::createWithAttributeType(1, 4, QSGGeometry::UnsignedByteType, QSGGeometry::ColorAttribute),
            QSGGeometry::Attribute::createWithAttributeType(2, 2, QSGGeometry::FloatType, QSGGeometry::TexCoordAttribute)
        };
        static QSGGeometry::AttributeSet attrs = { 3, sizeof(SmoothVertex), data };
        return attrs;
    }
}

QSGBasicInternalRectangleNode::QSGBasicInternalRectangleNode()
    : QSGInternalRectangleNode()
    , m_aligned(true)
    , m_antialiasing(false)
    , m_gradient_is_opaque(true)
    , m_dirty_geometry(false)
    , m_gradient_is_vertical(true)
    , m_geometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0)
{
    setGeometry(&m_geometry);

#ifdef QSG_RUNTIME_DESCRIPTION
    qsgnode_set_description(this, QLatin1String("internalrectangle"));
#endif
}

void QSGBasicInternalRectangleNode::setRect(const QRectF &rect)
{
    if (rect == m_rect)
        return;
    m_rect = rect;
    m_dirty_geometry = true;
}

void QSGBasicInternalRectangleNode::setColor(const QColor &color)
{
    if (color == m_color)
        return;
    m_color = color;
    if (m_gradient_stops.isEmpty())
        m_dirty_geometry = true;
}

void QSGBasicInternalRectangleNode::setPenColor(const QColor &color)
{
    if (color == m_border_color)
        return;
    m_border_color = color;
    if (m_pen_width > 0)
        m_dirty_geometry = true;
}

void QSGBasicInternalRectangleNode::setPenWidth(qreal width)
{
    if (width == m_pen_width)
        return;
    m_pen_width = width;
    m_dirty_geometry = true;
}


void QSGBasicInternalRectangleNode::setGradientStops(const QGradientStops &stops)
{
    if (stops.constData() == m_gradient_stops.constData())
        return;

    m_gradient_stops = stops;

    m_gradient_is_opaque = true;
    for (int i = 0; i < stops.size(); ++i)
        m_gradient_is_opaque &= stops.at(i).second.alpha() == 0xff;
    m_dirty_geometry = true;
}

void QSGBasicInternalRectangleNode::setGradientVertical(bool vertical)
{
    if (vertical == m_gradient_is_vertical)
        return;
    m_gradient_is_vertical = vertical;
    m_dirty_geometry = true;
}


void QSGBasicInternalRectangleNode::setRadius(qreal radius)
{
    if (radius == m_radius)
        return;
    m_radius = radius;
    m_dirty_geometry = true;
}

void QSGBasicInternalRectangleNode::setTopLeftRadius(qreal radius)
{
    if (radius == m_topLeftRadius)
        return;
    m_topLeftRadius = radius;
    m_dirty_geometry = true;
}
void QSGBasicInternalRectangleNode::setTopRightRadius(qreal radius)
{
    if (radius == m_topRightRadius)
        return;
    m_topRightRadius = radius;
    m_dirty_geometry = true;
}
void QSGBasicInternalRectangleNode::setBottomLeftRadius(qreal radius)
{
    if (radius == m_bottomLeftRadius)
        return;
    m_bottomLeftRadius = radius;
    m_dirty_geometry = true;
}
void QSGBasicInternalRectangleNode::setBottomRightRadius(qreal radius)
{
    if (radius == m_bottomRightRadius)
        return;
    m_bottomRightRadius = radius;
    m_dirty_geometry = true;
}

void QSGBasicInternalRectangleNode::setAntialiasing(bool antialiasing)
{
    if (!supportsAntialiasing())
        return;

    if (antialiasing == m_antialiasing)
        return;
    m_antialiasing = antialiasing;
    if (m_antialiasing) {
        setGeometry(new QSGGeometry(smoothAttributeSet(), 0));
        setFlag(OwnsGeometry, true);
    } else {
        setGeometry(&m_geometry);
        setFlag(OwnsGeometry, false);
    }
    updateMaterialAntialiasing();
    m_dirty_geometry = true;
}

void QSGBasicInternalRectangleNode::setAligned(bool aligned)
{
    if (aligned == m_aligned)
        return;
    m_aligned = aligned;
    m_dirty_geometry = true;
}

void QSGBasicInternalRectangleNode::update()
{
    if (m_dirty_geometry) {
        updateGeometry();
        m_dirty_geometry = false;

        QSGNode::DirtyState state = QSGNode::DirtyGeometry;
        updateMaterialBlending(&state);
        markDirty(state);
    }
}

void QSGBasicInternalRectangleNode::updateGeometry()
{
    float width = float(m_rect.width());
    float height = float(m_rect.height());
    float penWidth = qMin(qMin(width, height) * 0.5f, float(m_pen_width));

    if (m_aligned)
        penWidth = qRound(penWidth);

    QSGGeometry *g = geometry();
    g->setDrawingMode(QSGGeometry::DrawTriangleStrip);
    int vertexStride = g->sizeOfVertex();

    union {
        Vertex *vertices;
        SmoothVertex *smoothVertices;
    };

    Color4ub fillColor = colorToColor4ub(m_color);
    Color4ub borderColor = colorToColor4ub(m_border_color);
    Color4ub transparent = { 0, 0, 0, 0 };
    const QGradientStops &stops = m_gradient_stops;

    float gradientStart = (m_gradient_is_vertical ? m_rect.top() : m_rect.left());
    float gradientLength = (m_gradient_is_vertical ? height : width);
    float secondaryLength = (m_gradient_is_vertical ? width : height);

    int nextGradientStop = 0;
    float gradientPos = penWidth / gradientLength;
    while (nextGradientStop < stops.size() && stops.at(nextGradientStop).first <= gradientPos)
        ++nextGradientStop;
    int lastGradientStop = stops.size() - 1;
    float lastGradientPos = 1.0f - penWidth / gradientLength;
    while (lastGradientStop >= nextGradientStop && stops.at(lastGradientStop).first >= lastGradientPos)
        --lastGradientStop;
    int gradientIntersections = (lastGradientStop - nextGradientStop + 1);

    if (m_radius > 0
        || m_topLeftRadius > 0
        || m_topRightRadius > 0
        || m_bottomLeftRadius > 0
        || m_bottomRightRadius > 0) {
        // Rounded corners.

        // Radius should never exceed half the width or half the height.
        float radiusTL = qMin(qMin(width, height) * 0.4999f, float(m_topLeftRadius < 0 ? m_radius : m_topLeftRadius));
        float radiusTR = qMin(qMin(width, height) * 0.4999f, float(m_topRightRadius < 0 ? m_radius : m_topRightRadius));
        float radiusBL = qMin(qMin(width, height) * 0.4999f, float(m_bottomLeftRadius < 0 ? m_radius : m_bottomLeftRadius));
        float radiusBR = qMin(qMin(width, height) * 0.4999f, float(m_bottomRightRadius < 0 ? m_radius : m_bottomRightRadius));

        // The code produces some artefacts when radius <= 0.5. A radius of half a pixel
        // does not make much sense anyway, so we draw a normal corner in such a case.
        if (radiusTL <= 0.5)
            radiusTL = 0;
        if (radiusTR <= 0.5)
            radiusTR = 0;
        if (radiusBL <= 0.5)
            radiusBL = 0;
        if (radiusBR <= 0.5)
            radiusBR = 0;

        // We want to keep a minimal inner radius in order to make the inner
        // x-coordinates of an arc mathematically unique and identifiable.
        const float innerRadiusTL = qMax(radiusTL - penWidth * 1.0f, 0.01);
        const float innerRadiusTR = qMax(radiusTR - penWidth * 1.0f, 0.01);
        const float innerRadiusBL = qMax(radiusBL - penWidth * 1.0f, 0.01);
        const float innerRadiusBR = qMax(radiusBR - penWidth * 1.0f, 0.01);
        const float outerRadiusTL = radiusTL;
        const float outerRadiusTR = radiusTR;
        const float outerRadiusBL = radiusBL;
        const float outerRadiusBR = radiusBR;
        const float delta = qMin(width, height) * 0.5f;

        int segmentsTL = radiusTL == 0 ? 0 : qBound(3, qCeil(radiusTL * (M_PI / 6)), 18);
        int segmentsTR = radiusTR == 0 ? 0 : qBound(3, qCeil(radiusTR * (M_PI / 6)), 18);
        int segmentsBL = radiusBL == 0 ? 0 : qBound(3, qCeil(radiusBL * (M_PI / 6)), 18);
        int segmentsBR = radiusBR == 0 ? 0 : qBound(3, qCeil(radiusBR * (M_PI / 6)), 18);

        // If the radii on opposite sites in genraration direction are the same,
        // we will set the segments of one side to 0 as these points would be
        // calculated twice. Also, this optimizes for the case of similar radii
        if (m_gradient_is_vertical) {
            if (innerRadiusTL == innerRadiusTR) {
                if (segmentsTL <= segmentsTR)
                    segmentsTL = 0;
                else
                    segmentsTR = 0;
            }
            if (innerRadiusBL == innerRadiusBR){
                if (segmentsBL <= segmentsBR)
                    segmentsBL = 0;
                else
                    segmentsBR = 0;
            }
        } else {
            if (innerRadiusTL == innerRadiusBL) {
                if (segmentsTL <= segmentsBL)
                    segmentsTL = 0;
                else
                    segmentsBL = 0;
            }
            if (innerRadiusTR == innerRadiusBR) {
                if (segmentsTR <= segmentsBR)
                    segmentsTR = 0;
                else
                    segmentsBR = 0;
            }
        }

        const int sumSegments = segmentsTL + segmentsTR + segmentsBL + segmentsBR;

        /*

        --+--__
        --+--__--__
          |    --__--__
          |  seg   --__--+
        --+-__  ment  _+  \
        --+-__--__   -  \  \
              --__--+ se \  \
                  +  \  g \  \
                   \  \  m \  \
         -----------+--+  e \  \     <- gradient line
                     \  \  nt\  \
           fill       +--+----+--+
                      |  |    |  |
                         border
                  inner AA    outer AA (AA = antialiasing)

        */

        const int innerVertexCount = (sumSegments + 4) * 2 + gradientIntersections * 2;
        const int outerVertexCount = (sumSegments + 4) * 2;
        int vertexCount = innerVertexCount;
        if (m_antialiasing || penWidth)
            vertexCount += innerVertexCount;
        if (penWidth)
            vertexCount += outerVertexCount;
        if (m_antialiasing && penWidth)
            vertexCount += outerVertexCount;


        const int fillIndexCount = innerVertexCount;
        const int innerAAIndexCount = innerVertexCount * 2 + 2;
        const int borderIndexCount = innerVertexCount * 2 + 2;
        const int outerAAIndexCount = outerVertexCount * 2 + 2;
        int indexCount = 0;
        int fillHead = 0;
        int innerAAHead = 0;
        int innerAATail = 0;
        int borderHead = 0;
        int borderTail = 0;
        int outerAAHead = 0;
        int outerAATail = 0;
        bool hasFill = m_color.alpha() > 0 || !stops.isEmpty();
        if (hasFill)
            indexCount += fillIndexCount;
        if (m_antialiasing) {
            innerAATail = innerAAHead = indexCount + (innerAAIndexCount >> 1) + 1;
            indexCount += innerAAIndexCount;
        }
        if (penWidth) {
            borderTail = borderHead = indexCount + (borderIndexCount >> 1) + 1;
            indexCount += borderIndexCount;
        }
        if (m_antialiasing && penWidth) {
            outerAATail = outerAAHead = indexCount + (outerAAIndexCount >> 1) + 1;
            indexCount += outerAAIndexCount;
        }

        g->allocate(vertexCount, indexCount);
        vertices = reinterpret_cast<Vertex *>(g->vertexData());
        memset(vertices, 0, vertexCount * vertexStride);
        quint16 *indices = g->indexDataAsUShort();
        quint16 index = 0;


        float innerXPrev = 0.; // previous inner primary coordinate, both sides.
        float innerYLeftPrev = 0.; // previous inner secondary coordinate, left.
        float innerYRightPrev = 0.; // previous inner secondary coordinate, right.

        const float angleTL = 0.5f * float(M_PI) / segmentsTL;
        const float cosStepTL = qFastCos(angleTL);
        const float sinStepTL = qFastSin(angleTL);
        const float angleTR = 0.5f * float(M_PI) / segmentsTR;
        const float cosStepTR = qFastCos(angleTR);
        const float sinStepTR = qFastSin(angleTR);
        const float angleBL = 0.5f * float(M_PI) / segmentsBL;
        const float cosStepBL = qFastCos(angleBL);
        const float sinStepBL = qFastSin(angleBL);
        const float angleBR = 0.5f * float(M_PI) / segmentsBR;
        const float cosStepBR = qFastCos(angleBR);
        const float sinStepBR = qFastSin(angleBR);

        //The x- and y-Axis are transposed, depending on gradient being vertical or horizontal
        //Lets define some coordinates and radii. The first index is the part, the second index
        //is the left or right side, as seen when moving from part0 to part1

        //    left vertices  |  right vertices
        //                   |
        //      *************|**************
        //    * |            |             | *
        //   *--o            |             o--*
        //   *   innerX/Y    |     innerX/Y   *
        //   *               |                *
        //   *               |                *  part 0
        //   *               |                *
        //   *               |                *
        //   *               |                *
        //  -----------------+--------------------> y
        //   *               |                *
        //   *               |                *
        //   *               |                *
        //   *               |                *  part 1
        //   *               |                *
        //   *   innerX/Y    |     innerX/Y   *
        //   *--o            |             o--*
        //    * |            |             | *
        //      *************|**************
        //                   |
        //                   v  x
        //
        //     direction of vertex generation

        const float outerXCenter[][2] = {{
                float(m_gradient_is_vertical ? m_rect.top()  + radiusTL : m_rect.left() + radiusTL),
                float(m_gradient_is_vertical ? m_rect.top() + radiusTR : m_rect.left() + radiusBL)
            }, {
                float(m_gradient_is_vertical ? m_rect.bottom() - radiusBL : m_rect.right() - radiusTR),
                float(m_gradient_is_vertical ? m_rect.bottom() - radiusBR : m_rect.right() - radiusBR)
            }};

        const float outerYCenter[][2] = {{
                float(!m_gradient_is_vertical ? m_rect.top()  + outerRadiusTL : m_rect.left() + outerRadiusTL),
                float(!m_gradient_is_vertical ? m_rect.bottom() - outerRadiusBL : m_rect.right() - outerRadiusTR)
            }, {
                float(!m_gradient_is_vertical ? m_rect.top() + outerRadiusTR : m_rect.left() + outerRadiusBL),
                float(!m_gradient_is_vertical ? m_rect.bottom() - outerRadiusBR : m_rect.right() - outerRadiusBR)
            }};

        const float innerXCenter[][2] = { {
                float(m_gradient_is_vertical ? m_rect.top()  + innerRadiusTL + penWidth : m_rect.left() + innerRadiusTL + penWidth),
                float(m_gradient_is_vertical ? m_rect.top() + innerRadiusTR + penWidth: m_rect.left() + innerRadiusBL + penWidth)
            }, {
                float(m_gradient_is_vertical ? m_rect.bottom() - innerRadiusBL - penWidth: m_rect.right() - innerRadiusTR - penWidth),
                float(m_gradient_is_vertical ? m_rect.bottom() - innerRadiusBR - penWidth: m_rect.right() - innerRadiusBR - penWidth)
            }};

        const float innerYCenter[][2] = { {
                float(!m_gradient_is_vertical ? m_rect.top()  + innerRadiusTL + penWidth : m_rect.left() + innerRadiusTL + penWidth),
                float(!m_gradient_is_vertical ? m_rect.bottom() - innerRadiusBL - penWidth : m_rect.right() - innerRadiusTR - penWidth)
            },{
                float(!m_gradient_is_vertical ? m_rect.top() + innerRadiusTR + penWidth : m_rect.left() + innerRadiusBL + penWidth),
                float(!m_gradient_is_vertical ? m_rect.bottom() - innerRadiusBR - penWidth : m_rect.right() - innerRadiusBR - penWidth)
            }};

        const float innerRadius[][2] = {{
                innerRadiusTL,
                !m_gradient_is_vertical ? innerRadiusBL : innerRadiusTR
            }, {
                !m_gradient_is_vertical ? innerRadiusTR : innerRadiusBL,
                innerRadiusBR
            }};

        const float outerRadius[][2] = {{
                outerRadiusTL,
                !m_gradient_is_vertical ? outerRadiusBL : outerRadiusTR
            }, {
                !m_gradient_is_vertical ? outerRadiusTR : outerRadiusBL,
                outerRadiusBR
            }};

        const int segments[][2] = {{
                segmentsTL,
                !m_gradient_is_vertical ? segmentsBL : segmentsTR
            }, {
                !m_gradient_is_vertical ? segmentsTR : segmentsBL,
                segmentsBR
            }};

        const float cosStep[][2] = {{
                cosStepTL,
                !m_gradient_is_vertical ? cosStepBL : cosStepTR
            }, {
                !m_gradient_is_vertical ? cosStepTR : cosStepBL,
                cosStepBR
            }};

        const float sinStep[][2] = {{
                sinStepTL,
                !m_gradient_is_vertical ? sinStepBL : sinStepTR
            }, {
                !m_gradient_is_vertical ? sinStepTR : sinStepBL,
                sinStepBR
            }};

        auto fillColorFromX = [&](float x) {

            float t = (x - gradientStart) / gradientLength;
            t = qBound(0.0, t, 1.0);

            int i = 1;
            if (t < stops.first().first)
                return colorToColor4ub(stops.first().second);
            while (i < stops.size()) {
                const QGradientStop &prev = stops.at(i - 1);
                const QGradientStop &next = stops.at(i);
                if (prev.first <= t && next.first > t) {
                    t = (t - prev.first) / (next.first - prev.first);
                    return colorToColor4ub(prev.second) * (1. - t) + colorToColor4ub(next.second) * t; }
                i++;
            }
            return colorToColor4ub(stops.last().second);
        };

        for (int part = 0; part < 2; ++part) {
            // cosine of the angle of the current segment, starting at 1 for part 0 and 0 for part 1
            float cosSegmentAngleLeft = 1. - part;
            // sine of the angle of the current segment
            float sinSegmentAngleLeft = part;

            float cosSegmentAngleRight = 1. - part;
            float sinSegmentAngleRight = part;

            bool advanceLeft = true;

            // We draw both the left part and the right part of the rectangle at the same time.
            // We also draw a vertex on the left side for every vertex on the right side. This
            // syncronisation is required to make sure that all gradient stops can be inserted.
            for (int iLeft = 0, iRight = 0; iLeft <= segments[part][0] || iRight <= segments[part][1]; ) {

                float xLeft, yLeft,
                      xRight, yRight;

                float outerXLeft, outerYLeft,
                      outerXRight, outerYRight;

                float sinAngleLeft, cosAngleLeft,
                      sinAngleRight, cosAngleRight;

                // calculate inner x-coordinates
                xLeft = innerXCenter[part][0] - innerRadius[part][0] * cosSegmentAngleLeft;
                xRight = innerXCenter[part][1] - innerRadius[part][1] * cosSegmentAngleRight;

                // calcuate inner y-coordinates
                yLeft = innerYCenter[part][0] - innerRadius[part][0] * sinSegmentAngleLeft;
                yRight = innerYCenter[part][1] + innerRadius[part][1] * sinSegmentAngleRight;

                // Synchronize left and right hand x-coordinates. This is required to
                // make sure that we can insert all gradient stops that require exactly two triangles at
                // every x-coordinate. Take the smaller of both x-coordinates and then find the matching
                // y-coordinates.
                if ((iLeft <= segments[part][0] && xLeft <= xRight) || iRight > segments[part][1]) {
                    advanceLeft = true;
                } else {
                    advanceLeft = false;
                }

                // Inner: Find the matching y-coordinates for the x-coordinate found above.
                // Outer: Also set the sine and cosine to make sure that outer vertices are
                // drawn correctly.
                if (innerRadius[part][0] == innerRadius[part][1]) {
                // Special case of equal radii. Optimize to avoid performance regression:
                // Left and right is always equal and we can just copy the angles and
                // mirror the coordinates.
                    if (advanceLeft) {
                        if (outerRadius[part][0] == 0) {
                            sinAngleLeft = 1.;
                            cosAngleLeft = part ? -1. : 1.;
                        } else {
                            sinAngleLeft = sinSegmentAngleLeft;
                            cosAngleLeft = cosSegmentAngleLeft;
                        }
                        if (outerRadius[part][1] == 0) {
                            sinAngleRight = 1.;
                            cosAngleRight = part ? -1. : 1.;
                        } else {
                            sinAngleRight = sinSegmentAngleLeft;
                            cosAngleRight = cosSegmentAngleLeft;
                        }
                        xRight = xLeft;
                        yRight = innerYCenter[part][1] + innerRadius[part][1] * sinAngleRight;
                    } else {
                        if (outerRadius[part][0] == 0) {
                            sinAngleLeft = 1.;
                            cosAngleLeft = part ? -1. : 1.;
                        } else {
                            sinAngleLeft = sinSegmentAngleRight;
                            cosAngleLeft = cosSegmentAngleRight;
                        }
                        if (outerRadius[part][1] == 0) {
                            sinAngleRight = 1.;
                            cosAngleRight = part ? -1. : 1.;
                        } else {
                            sinAngleRight = sinSegmentAngleRight;
                            cosAngleRight = cosSegmentAngleRight;
                        }
                        xLeft = xRight;
                        yLeft = innerYCenter[part][0] - innerRadius[part][0] * sinAngleLeft;
                    }
                } else if (advanceLeft) {
                    if (outerRadius[part][0] == 0) {
                        sinAngleLeft = 1.;
                        cosAngleLeft = part ? -1. : 1.;
                    } else {
                        sinAngleLeft = sinSegmentAngleLeft;
                        cosAngleLeft = cosSegmentAngleLeft;
                    }
                    if (outerRadius[part][1] == 0) {
                        // Outer: If the outer radius is zero we can return both sin and cos = 1
                        // to form a nice corner. Inner: Accept the x-coordinate from the other
                        // side and match the y-coordinate
                        sinAngleRight = 1.;
                        cosAngleRight = part ? -1. : 1.;
                        xRight = xLeft;
                        yRight = innerYCenter[part][1] + innerRadius[part][1] * sinAngleRight;
                    } else if (xLeft >= innerXCenter[0][1] && xLeft <= innerXCenter[1][1]) {
                        // Outer: If we are on the straight line between the inner centers, we can
                        // just return sin = 1 and cos = 0. Inner: Accept the x-coordinate from the
                        // other side and match the y-coordinate
                        sinAngleRight = 1.;
                        cosAngleRight = 0.;
                        xRight = xLeft;
                        yRight = innerYCenter[part][1] + innerRadius[part][1] * sinAngleRight;
                    } else {
                        // Inner: If we are on the rounded part of the oposite side, we have to find a vertex
                        // on that curve that matches the x-coordinate we selected.
                        // We always select the smaller x-coordinate and can therefore use a linear
                        // interpolation between the last point on this side and the point on this side
                        // that was not accepted because it was too large.
                        if (xRight != innerXPrev) {
                            float t = (xLeft - innerXPrev) / (xRight - innerXPrev);
                            yRight = innerYRightPrev * (1. - t) + yRight * t;
                            xRight = xLeft;
                        }
                        // Outer: With the coordinates from the interpolation we can calculate the sine
                        // and cosine of the respective angle quickly.
                        sinAngleRight = (yRight - innerYCenter[part][1]) / innerRadius[part][1];
                        cosAngleRight = -(xRight - innerXCenter[part][1]) / innerRadius[part][1];
                    }
                } else {
                    // same as above but for the other side.
                    if (outerRadius[part][1] == 0) {
                        sinAngleRight = 1.;
                        cosAngleRight = part ? -1. : 1.;
                    } else {
                        sinAngleRight = sinSegmentAngleRight;
                        cosAngleRight = cosSegmentAngleRight;
                    }
                    if (outerRadius[part][0] == 0) {
                        sinAngleLeft = 1.;
                        cosAngleLeft = part ? -1. : 1.;
                        xLeft = xRight;
                        yLeft = innerYCenter[part][0] - innerRadius[part][0] * sinAngleLeft;
                    } else if (xRight >= innerXCenter[0][0] && xRight <= innerXCenter[1][0]) {
                        sinAngleLeft = 1.;
                        cosAngleLeft = 0.;
                        xLeft = xRight;
                        yLeft = innerYCenter[part][0] - innerRadius[part][0] * sinAngleLeft;
                    } else {
                        if (xLeft != innerXPrev) {
                            float t = (xRight - innerXPrev) / (xLeft - innerXPrev);
                            yLeft = innerYLeftPrev * (1. - t) + yLeft * t;
                            xLeft = xRight;
                        }
                        sinAngleLeft = -(yLeft - innerYCenter[part][0]) / innerRadius[part][0];
                        cosAngleLeft = -(xLeft - innerXCenter[part][0]) / innerRadius[part][0];
                    }
                }

                gradientPos = (xLeft - gradientStart) / gradientLength;

                // calculate the matching outer coordinates
                outerXLeft = outerXCenter[part][0]  - outerRadius[part][0] * cosAngleLeft;
                outerYLeft = outerYCenter[part][0] - outerRadius[part][0] * sinAngleLeft;
                outerXRight = outerXCenter[part][1] - outerRadius[part][1] * cosAngleRight;
                outerYRight = outerYCenter[part][1] + outerRadius[part][1] * sinAngleRight;

                // insert gradient stops as required
                while (nextGradientStop <= lastGradientStop && stops.at(nextGradientStop).first <= gradientPos) {
                    float gradientX;
                    float gradientYLeft;
                    float gradientYRight;

                    // Insert vertices at gradient stops
                    gradientX = gradientStart + stops.at(nextGradientStop).first * gradientLength;
                    // bilinear interpolation of known vertices
                    float t = (gradientX - innerXPrev) / (xLeft - innerXPrev);
                    gradientYLeft = innerYLeftPrev * (1. - t) + t * yLeft;
                    gradientYRight =  innerYRightPrev * (1. - t) + t * yRight;

                    fillColor = fillColorFromX(gradientX);

                    if (hasFill) {
                        indices[fillHead++] = index;
                        indices[fillHead++] = index + 1;
                    }

                    if (penWidth) {
                        --borderHead;
                        indices[borderHead] = indices[borderHead + 2];
                        indices[--borderHead] = index + 2;
                        indices[borderTail++] = index + 3;
                        indices[borderTail] = indices[borderTail - 2];
                        ++borderTail;
                    }

                    if (m_antialiasing) {
                        indices[--innerAAHead] = index + 2;
                        indices[--innerAAHead] = index;
                        indices[innerAATail++] = index + 1;
                        indices[innerAATail++] = index + 3;

                        bool lower = stops.at(nextGradientStop).first > 0.5f;
                        float dp = lower ? qMin(0.0f, gradientLength - gradientX - delta) : qMax(0.0f, delta - gradientX);
                        smoothVertices[index++].set(gradientX, gradientYRight, fillColor, dp, secondaryLength - gradientYRight - delta, m_gradient_is_vertical);
                        smoothVertices[index++].set(gradientX, gradientYLeft, fillColor, dp, delta - gradientYLeft, m_gradient_is_vertical);
                        if (penWidth) {
                            smoothVertices[index++].set(gradientX, gradientYRight, borderColor, -0.49f * penWidth * cosAngleRight, 0.49f * penWidth * sinAngleRight, m_gradient_is_vertical);
                            smoothVertices[index++].set(gradientX, gradientYLeft, borderColor, -0.49f * penWidth * cosAngleLeft, -0.49f * penWidth * sinAngleLeft, m_gradient_is_vertical);
                        } else {
                            dp = lower ? delta : -delta;
                            smoothVertices[index++].set(gradientX, gradientYRight, transparent, dp, delta, m_gradient_is_vertical);
                            smoothVertices[index++].set(gradientX, gradientYLeft, transparent, dp, -delta, m_gradient_is_vertical);
                        }
                    } else {
                        vertices[index++].set(gradientX, gradientYRight, fillColor, m_gradient_is_vertical);
                        vertices[index++].set(gradientX, gradientYLeft, fillColor, m_gradient_is_vertical);
                        if (penWidth) {
                            vertices[index++].set(gradientX, gradientYRight, borderColor, m_gradient_is_vertical);
                            vertices[index++].set(gradientX, gradientYLeft, borderColor, m_gradient_is_vertical);
                        }
                    }

                    innerXPrev = gradientX;
                    innerYLeftPrev = gradientYLeft;
                    innerYRightPrev = gradientYRight;

                    nextGradientStop++;
                }

                if (!stops.isEmpty()) {
                    fillColor = fillColorFromX(xLeft);
                }

                if (hasFill) {
                    indices[fillHead++] = index;
                    indices[fillHead++] = index + 1;
                }

                if (penWidth) {
                    indices[--borderHead] = index + 4;
                    indices[--borderHead] = index + 2;
                    indices[borderTail++] = index + 3;
                    indices[borderTail++] = index + 5;
                }

                if (m_antialiasing) {
                    indices[--innerAAHead] = index + 2;
                    indices[--innerAAHead] = index;
                    indices[innerAATail++] = index + 1;
                    indices[innerAATail++] = index + 3;

                    float dp = part ? qMin(0.0f, gradientLength - xRight - delta) : qMax(0.0f, delta - xRight);
                    smoothVertices[index++].set(xRight, yRight, fillColor, dp, secondaryLength - yRight - delta, m_gradient_is_vertical);
                    smoothVertices[index++].set(xLeft, yLeft, fillColor, dp, delta - yLeft, m_gradient_is_vertical);

                    dp = part ? delta : -delta;
                    if (penWidth) {
                        smoothVertices[index++].set(xRight, yRight, borderColor, -0.49f * penWidth * cosAngleRight, 0.49f * penWidth * sinAngleRight, m_gradient_is_vertical);
                        smoothVertices[index++].set(xLeft, yLeft, borderColor, -0.49f * penWidth * cosAngleLeft, -0.49f * penWidth * sinAngleLeft, m_gradient_is_vertical);
                        smoothVertices[index++].set(outerXRight, outerYRight, borderColor, 0.49f * penWidth * cosAngleRight, -0.49f * penWidth * sinAngleRight, m_gradient_is_vertical);
                        smoothVertices[index++].set(outerXLeft, outerYLeft, borderColor, 0.49f * penWidth * cosAngleLeft, 0.49f * penWidth * sinAngleLeft, m_gradient_is_vertical);
                        smoothVertices[index++].set(outerXRight, outerYRight, transparent, dp, delta, m_gradient_is_vertical);
                        smoothVertices[index++].set(outerXLeft, outerYLeft, transparent, dp, -delta, m_gradient_is_vertical);

                        indices[--outerAAHead] = index - 2;
                        indices[--outerAAHead] = index - 4;
                        indices[outerAATail++] = index - 3;
                        indices[outerAATail++] = index - 1;
                    } else {
                        smoothVertices[index++].set(xRight, yRight, transparent, dp, delta, m_gradient_is_vertical);
                        smoothVertices[index++].set(xLeft, yLeft, transparent, dp, -delta, m_gradient_is_vertical);
                    }
                } else {
                    vertices[index++].set(xRight, yRight, fillColor, m_gradient_is_vertical);
                    vertices[index++].set(xLeft, yLeft, fillColor, m_gradient_is_vertical);
                    if (penWidth) {
                        vertices[index++].set(xRight, yRight, borderColor, m_gradient_is_vertical);
                        vertices[index++].set(xLeft, yLeft, borderColor, m_gradient_is_vertical);
                        vertices[index++].set(outerXRight, outerYRight, borderColor, m_gradient_is_vertical);
                        vertices[index++].set(outerXLeft, outerYLeft, borderColor, m_gradient_is_vertical);
                    }
                }

                innerXPrev = xLeft;
                innerYLeftPrev = yLeft;
                innerYRightPrev = yRight;

                // Advance the point. This corresponds to a rotation of the respective segment
                if (advanceLeft) {
                    iLeft++;
                    qreal tmp = cosSegmentAngleLeft;
                    cosSegmentAngleLeft = cosSegmentAngleLeft * cosStep[part][0] - sinSegmentAngleLeft * sinStep[part][0];
                    sinSegmentAngleLeft = sinSegmentAngleLeft * cosStep[part][0] + tmp * sinStep[part][0];
                } else {
                    iRight++;
                    qreal tmp = cosSegmentAngleRight;
                    cosSegmentAngleRight = cosSegmentAngleRight * cosStep[part][1] - sinSegmentAngleRight * sinStep[part][1];
                    sinSegmentAngleRight = sinSegmentAngleRight * cosStep[part][1] + tmp * sinStep[part][1];
                }
            }
        }

        Q_ASSERT(index == vertexCount);

        // Close the triangle strips.
        if (m_antialiasing) {
            indices[--innerAAHead] = indices[innerAATail - 1];
            indices[--innerAAHead] = indices[innerAATail - 2];
            Q_ASSERT(innerAATail <= indexCount);
        }
        if (penWidth) {
            indices[--borderHead] = indices[borderTail - 1];
            indices[--borderHead] = indices[borderTail - 2];
            Q_ASSERT(borderTail <= indexCount);
        }
        if (m_antialiasing && penWidth) {
            indices[--outerAAHead] = indices[outerAATail - 1];
            indices[--outerAAHead] = indices[outerAATail - 2];
            Q_ASSERT(outerAATail == indexCount);
        }
    } else {
        // Straight corners.
        QRectF innerRect = m_rect;
        QRectF outerRect = m_rect;

        if (penWidth)
            innerRect.adjust(1.0f * penWidth, 1.0f * penWidth, -1.0f * penWidth, -1.0f * penWidth);

        float delta = qMin(width, height) * 0.5f;
        int innerVertexCount = 4 + gradientIntersections * 2;
        int outerVertexCount = 4;
        int vertexCount = innerVertexCount;
        if (m_antialiasing || penWidth)
            vertexCount += innerVertexCount;
        if (penWidth)
            vertexCount += outerVertexCount;
        if (m_antialiasing && penWidth)
            vertexCount += outerVertexCount;

        int fillIndexCount = innerVertexCount;
        int innerAAIndexCount = innerVertexCount * 2 + 2;
        int borderIndexCount = innerVertexCount * 2 + 2;
        int outerAAIndexCount = outerVertexCount * 2 + 2;
        int indexCount = 0;
        int fillHead = 0;
        int innerAAHead = 0;
        int innerAATail = 0;
        int borderHead = 0;
        int borderTail = 0;
        int outerAAHead = 0;
        int outerAATail = 0;
        bool hasFill = m_color.alpha() > 0 || !stops.isEmpty();
        if (hasFill)
            indexCount += fillIndexCount;
        if (m_antialiasing) {
            innerAATail = innerAAHead = indexCount + (innerAAIndexCount >> 1) + 1;
            indexCount += innerAAIndexCount;
        }
        if (penWidth) {
            borderTail = borderHead = indexCount + (borderIndexCount >> 1) + 1;
            indexCount += borderIndexCount;
        }
        if (m_antialiasing && penWidth) {
            outerAATail = outerAAHead = indexCount + (outerAAIndexCount >> 1) + 1;
            indexCount += outerAAIndexCount;
        }

        g->allocate(vertexCount, indexCount);
        vertices = reinterpret_cast<Vertex *>(g->vertexData());
        memset(vertices, 0, vertexCount * vertexStride);
        quint16 *indices = g->indexDataAsUShort();
        quint16 index = 0;

        float innerStart = (m_gradient_is_vertical ? innerRect.top() : innerRect.left());
        float innerEnd = (m_gradient_is_vertical ? innerRect.bottom() : innerRect.right());
        float outerStart = (m_gradient_is_vertical ? outerRect.top() : outerRect.left());
        float outerEnd = (m_gradient_is_vertical ? outerRect.bottom() : outerRect.right());

        float innerSecondaryStart = (m_gradient_is_vertical ? innerRect.left() : innerRect.top());
        float innerSecondaryEnd = (m_gradient_is_vertical ? innerRect.right() : innerRect.bottom());
        float outerSecondaryStart = (m_gradient_is_vertical ? outerRect.left() : outerRect.top());
        float outerSecondaryEnd = (m_gradient_is_vertical ? outerRect.right() : outerRect.bottom());

        for (int part = -1; part <= 1; part += 2) {
            float innerEdge = (part == 1 ? innerEnd : innerStart);
            float outerEdge = (part == 1 ? outerEnd : outerStart);
            gradientPos = (innerEdge - innerStart + penWidth) / gradientLength;

            while (nextGradientStop <= lastGradientStop && stops.at(nextGradientStop).first <= gradientPos) {
                // Insert vertices at gradient stops.
                float gp = (innerStart - penWidth) + stops.at(nextGradientStop).first * gradientLength;

                fillColor = colorToColor4ub(stops.at(nextGradientStop).second);

                if (hasFill) {
                    indices[fillHead++] = index;
                    indices[fillHead++] = index + 1;
                }

                if (penWidth) {
                    --borderHead;
                    indices[borderHead] = indices[borderHead + 2];
                    indices[--borderHead] = index + 2;
                    indices[borderTail++] = index + 3;
                    indices[borderTail] = indices[borderTail - 2];
                    ++borderTail;
                }

                if (m_antialiasing) {
                    indices[--innerAAHead] = index + 2;
                    indices[--innerAAHead] = index;
                    indices[innerAATail++] = index + 1;
                    indices[innerAATail++] = index + 3;

                    bool lower = stops.at(nextGradientStop).first > 0.5f;
                    float dp = lower ? qMin(0.0f, gradientLength - gp - delta) : qMax(0.0f, delta - gp);
                    smoothVertices[index++].set(gp, innerSecondaryEnd, fillColor, dp, secondaryLength - innerSecondaryEnd - delta, m_gradient_is_vertical);
                    smoothVertices[index++].set(gp, innerSecondaryStart, fillColor, dp, delta - innerSecondaryStart, m_gradient_is_vertical);
                    if (penWidth) {
                        smoothVertices[index++].set(gp, innerSecondaryEnd, borderColor, (lower ? 0.49f : -0.49f) * penWidth, 0.49f * penWidth, m_gradient_is_vertical);
                        smoothVertices[index++].set(gp, innerSecondaryStart, borderColor, (lower ? 0.49f : -0.49f) * penWidth, -0.49f * penWidth, m_gradient_is_vertical);
                    } else {
                        smoothVertices[index++].set(gp, innerSecondaryEnd, transparent, lower ? delta : -delta, delta, m_gradient_is_vertical);
                        smoothVertices[index++].set(gp, innerSecondaryStart, transparent, lower ? delta : -delta, -delta, m_gradient_is_vertical);
                    }
                } else {
                    vertices[index++].set(gp, innerSecondaryEnd, fillColor, m_gradient_is_vertical);
                    vertices[index++].set(gp, innerSecondaryStart, fillColor, m_gradient_is_vertical);
                    if (penWidth) {
                        vertices[index++].set(gp, innerSecondaryEnd, borderColor, m_gradient_is_vertical);
                        vertices[index++].set(gp, innerSecondaryStart, borderColor, m_gradient_is_vertical);
                    }
                }
                ++nextGradientStop;
            }

            if (!stops.isEmpty()) {
                if (nextGradientStop == 0) {
                    fillColor = colorToColor4ub(stops.at(0).second);
                } else if (nextGradientStop == stops.size()) {
                    fillColor = colorToColor4ub(stops.last().second);
                } else {
                    const QGradientStop &prev = stops.at(nextGradientStop - 1);
                    const QGradientStop &next = stops.at(nextGradientStop);
                    float t = (gradientPos - prev.first) / (next.first - prev.first);
                    fillColor = colorToColor4ub(prev.second) * (1 - t) + colorToColor4ub(next.second) * t;
                }
            }

            if (hasFill) {
                indices[fillHead++] = index;
                indices[fillHead++] = index + 1;
            }

            if (penWidth) {
                indices[--borderHead] = index + 4;
                indices[--borderHead] = index + 2;
                indices[borderTail++] = index + 3;
                indices[borderTail++] = index + 5;
            }

            if (m_antialiasing) {
                indices[--innerAAHead] = index + 2;
                indices[--innerAAHead] = index;
                indices[innerAATail++] = index + 1;
                indices[innerAATail++] = index + 3;

                float dp = part == 1 ? qMin(0.0f, gradientLength - innerEdge - delta) : qMax(0.0f, delta - innerEdge);
                smoothVertices[index++].set(innerEdge, innerSecondaryEnd, fillColor, dp, secondaryLength - innerSecondaryEnd - delta, m_gradient_is_vertical);
                smoothVertices[index++].set(innerEdge, innerSecondaryStart, fillColor, dp, delta - innerSecondaryStart, m_gradient_is_vertical);

                if (penWidth) {
                    smoothVertices[index++].set(innerEdge, innerSecondaryEnd, borderColor, 0.49f * penWidth * part, 0.49f * penWidth, m_gradient_is_vertical);
                    smoothVertices[index++].set(innerEdge, innerSecondaryStart, borderColor, 0.49f * penWidth * part, -0.49f * penWidth, m_gradient_is_vertical);
                    smoothVertices[index++].set(outerEdge, outerSecondaryEnd, borderColor, -0.49f * penWidth * part, -0.49f * penWidth, m_gradient_is_vertical);
                    smoothVertices[index++].set(outerEdge, outerSecondaryStart, borderColor, -0.49f * penWidth * part, 0.49f * penWidth, m_gradient_is_vertical);
                    smoothVertices[index++].set(outerEdge, outerSecondaryEnd, transparent, delta * part, delta, m_gradient_is_vertical);
                    smoothVertices[index++].set(outerEdge, outerSecondaryStart, transparent, delta * part, -delta, m_gradient_is_vertical);

                    indices[--outerAAHead] = index - 2;
                    indices[--outerAAHead] = index - 4;
                    indices[outerAATail++] = index - 3;
                    indices[outerAATail++] = index - 1;
                } else {
                    smoothVertices[index++].set(innerEdge, innerSecondaryEnd, transparent, delta * part, delta, m_gradient_is_vertical);
                    smoothVertices[index++].set(innerEdge, innerSecondaryStart, transparent, delta * part, -delta, m_gradient_is_vertical);
                }
            } else {
                vertices[index++].set(innerEdge, innerSecondaryEnd, fillColor, m_gradient_is_vertical);
                vertices[index++].set(innerEdge, innerSecondaryStart, fillColor, m_gradient_is_vertical);
                if (penWidth) {
                    vertices[index++].set(innerEdge, innerSecondaryEnd, borderColor, m_gradient_is_vertical);
                    vertices[index++].set(innerEdge, innerSecondaryStart, borderColor, m_gradient_is_vertical);
                    vertices[index++].set(outerEdge, outerSecondaryEnd, borderColor, m_gradient_is_vertical);
                    vertices[index++].set(outerEdge, outerSecondaryStart, borderColor, m_gradient_is_vertical);
                }
            }
        }
        Q_ASSERT(index == vertexCount);

        // Close the triangle strips.
        if (m_antialiasing) {
            indices[--innerAAHead] = indices[innerAATail - 1];
            indices[--innerAAHead] = indices[innerAATail - 2];
            Q_ASSERT(innerAATail <= indexCount);
        }
        if (penWidth) {
            indices[--borderHead] = indices[borderTail - 1];
            indices[--borderHead] = indices[borderTail - 2];
            Q_ASSERT(borderTail <= indexCount);
        }
        if (m_antialiasing && penWidth) {
            indices[--outerAAHead] = indices[outerAATail - 1];
            indices[--outerAAHead] = indices[outerAATail - 2];
            Q_ASSERT(outerAATail == indexCount);
        }
    }
}

QT_END_NAMESPACE
