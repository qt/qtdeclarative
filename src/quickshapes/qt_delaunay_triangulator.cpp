// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickshapecurverenderer_p_p.h"
#include <QtGui/qvector2d.h>
#include <QtGui/private/qtriangulator_p.h>
#include <QtCore/QHash>
#include <QtCore/QSet>
#include <QtCore/QPair>
#include <QtCore/QStack>
#include <QtCore/QElapsedTimer>

// #define QQUICKSHAPES_DEBUG_IMAGE 1

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcShapeTiming, "qt.shape.timing");

// Check if a point is inside the circumcircle of a triangle defined by three vertices (v1, v2, v3).
// It uses the determinant of a matrix formed from the coordinates of the point and the vertices
// to determine if the point is inside the circumcircle.
// If the determinant is > 0, the point is inside the circumcircle.
inline bool isPointInCircumcircle(const QVector2D &point,
                                  const QVector2D &v1_, const QVector2D &v2_, const QVector2D &v3_)
{
    QVector2D v1 = v1_;
    QVector2D v2 = v2_;
    QVector2D v3 = v3_;

    // Vertices must be in ccw order
    if ( ( ( (v2.x() - v1.x()) * (v3.y() - v1.y()) ) - ( (v3.x() - v1.x()) * (v2.y() - v1.y()) ) ) <= 0.0f) {
        qSwap(v2, v3);
    }

#if 1
    float x13 = v1.x() - v3.x();
    float y13 = v1.y() - v3.y();
    float x23 = v2.x() - v3.x();
    float y23 = v2.y() - v3.y();
    float x1p = v1.x() - point.x();
    float y1p = v1.y() - point.y();
    float x2p = v2.x() - point.x();
    float y2p = v2.y() - point.y();

    return (x13*x23 + y13*y23) * (x2p*y1p - x1p*y2p) < (x23*y13 - x13*y23) * (x2p*x1p + y1p*y2p);

#else
    float ax = v1.x() - point.x();
    float ay = v1.y() - point.y();
    float bx = v2.x() - point.x();
    float by = v2.y() - point.y();
    float cx = v3.x() - point.x();
    float cy = v3.y() - point.y();

    float det = (ax * ax + ay * ay) * (bx * cy - cx * by) -
                (bx * bx + by * by) * (ax * cy - cx * ay) +
                (cx * cx + cy * cy) * (ax * by - bx * ay);

    return det > 0.0f;
#endif
}

// Retrieves quad corresponding to t1 and t2 (assuming that they share one edge)
// In the end, the new diagonal is (i1, i3) and (i1, i2, i3, i4) is the ordered vertices
// in the quad.
inline void findQuad(const QtPathTriangle &t1, const QtPathTriangle &t2,
                     quint32 *i1, quint32 *i2, quint32 *i3, quint32 *i4)
{
    // First find index in t1 which is not in t2 and call this i1 (with i2, and i4 completing t1)
    if (t1.v1Index != t2.v1Index && t1.v1Index != t2.v2Index && t1.v1Index != t2.v3Index) {
        (*i1) = t1.v1Index;
        (*i2) = t1.v2Index;
        (*i4) = t1.v3Index;
    } else if (t1.v2Index != t2.v1Index && t1.v2Index != t2.v2Index && t1.v2Index != t2.v3Index) {
        (*i1) = t1.v2Index;
        (*i2) = t1.v3Index;
        (*i4) = t1.v1Index;
    } else {
        Q_ASSERT(t1.v3Index != t2.v1Index && t1.v3Index != t2.v2Index && t1.v3Index != t2.v3Index);
        (*i1) = t1.v3Index;
        (*i2) = t1.v1Index;
        (*i4) = t1.v2Index;
    }

    // Then find (*i3) as index in t2 which is not in t1
    if (t2.v1Index != t1.v1Index && t2.v1Index != t1.v2Index && t2.v1Index != t1.v3Index) {
        (*i3) = t2.v1Index;
    } else if (t2.v2Index != t1.v1Index && t2.v2Index != t1.v2Index && t2.v2Index != t1.v3Index) {
        (*i3) = t2.v2Index;
    } else {
        Q_ASSERT(t2.v3Index != t1.v1Index && t2.v3Index != t1.v2Index && t2.v3Index != t1.v3Index);
        (*i3) = t2.v3Index;
    }
}

inline void swapDiagonal(QtPathTriangle *t1,
                         QtPathTriangle *t2,
                         quint32 idx = quint32(-1))
{
    quint32 i1, i2, i3, i4;
    findQuad(*t1, *t2, &i1, &i2, &i3, &i4);
    Q_ASSERT(idx == quint32(-1) || t2->v1Index == idx || t2->v2Index == idx || t2->v3Index == idx);

    t2->v1Index = i1;
    t2->v2Index = i3;
    t2->v3Index = i2;

    t1->v1Index = i1;
    t1->v2Index = i3;
    t1->v3Index = i4;
}

inline bool isPointInTriangle(const QVector2D &v,
                              const QVector2D &v1,
                              const QVector2D &v2,
                              const QVector2D &v3)
{
#if 0
    QPolygonF p;
    p.append(v1.toPointF());
    p.append(v2.toPointF());
    p.append(v3.toPointF());
    return p.containsPoint(v.toPointF(), Qt::OddEvenFill);
#else
     if ( (v1.x() > v.x() && v2.x() > v.x() && v3.x() > v.x())
       || (v1.x() < v.x() && v2.x() < v.x() && v3.x() < v.x())
       || (v1.y() > v.y() && v2.y() > v.y() && v3.y() > v.y())
       || (v1.y() < v.y() && v2.y() < v.y() && v3.y() < v.y())) {
         return false;
     }

     // Barycentric scalars
     double denom = (v2.y() - v3.y()) * (v1.x() - v3.x())
                 + (v3.x() - v2.x()) * (v1.y() - v3.y());
     double a = ((v2.y() - v3.y()) * (v.x() - v3.x())
              + (v3.x() - v2.x()) * (v.y() - v3.y()))
              / denom;
     double b = ((v3.y() - v1.y()) * (v.x() - v3.x())
              + (v1.x() - v3.x()) * (v.y() - v3.y()))
              / denom;
     double c = 1.0 - a - b;

     if (qFuzzyIsNull(a))
         a = 0.0;
     if (qFuzzyIsNull(b))
         b = 0.0;
     if (qFuzzyIsNull(c))
         c = 0.0;

     if (qFuzzyCompare(a, 1.0))
         a = 1.0;
     if (qFuzzyCompare(b, 1.0))
         b = 1.0;
     if (qFuzzyCompare(c, 1.0))
         c = 1.0;

     return a >= 0.0 && a <= 1.0
            && b >= 0.0 && b <= 1.0
            && c >= 0.0 && c <= 1.0;
#endif
}

inline void saveDebugImage(bool shouldDraw,
                           const QPainterPath &path,
                           const QString &label,
                           const QList<QtPathTriangle> &triangles,
                           const QList<QtPathVertex> &vertices,
                           const QList<quint32> &highlightedPoints = QList<quint32>{},
                           const QList<quint32> &highlightedTriangles = QList<quint32>{},
                           const QList<QPair<quint32, quint32> > &highlightedEdges = QList<QPair<quint32, quint32> >{})
{
#if defined(QQUICKSHAPES_DEBUG_IMAGE)
    shouldDraw = true;
#endif

    static bool zoom = qEnvironmentVariableIntValue("QT_QUICKSHAPES_ZOOM_DEBUG_IMAGE");

    if (!shouldDraw)
        return;

    static int counter = 0;

    int size = 3000;
    QImage image(size, size, QImage::Format_ARGB32);
    image.fill(Qt::black);

    QList<QVector2D> vs;

    if (zoom) {
        for (auto it : highlightedPoints)
            vs.append(vertices.at(it).point);

        for (auto it : highlightedEdges) {
            vs.append(vertices.at(it.first).point);
            vs.append(vertices.at(it.second).point);
        }

        for (auto it : highlightedTriangles) {
            vs.append(vertices.at(triangles.at(it).v1Index).point);
            vs.append(vertices.at(triangles.at(it).v2Index).point);
            vs.append(vertices.at(triangles.at(it).v3Index).point);
        }
    } else {
        for (int i = 0; i < vertices.size() - 3; ++i)
            vs.append(vertices.at(i).point);
    }

    float minX = 0.0f;
    float maxX = 0.0f;
    float minY = 0.0f;
    float maxY = 0.0f;
    for (int i = 0; i < vs.size(); ++i) {
        QVector2D p = vs.at(i);
        if (i == 0 || minX > p.x())
            minX = p.x();
        if (i == 0 || maxX < p.x())
            maxX = p.x();
        if (i == 0 || minY > p.y())
            minY = p.y();
        if (i == 0 || maxY < p.y())
            maxY = p.y();
    }

    minX -= (maxX - minX) * 0.05f;
    minY -= (maxY - minY) * 0.05f;
    maxX += (maxX - minX) * 0.05f;
    maxY += (maxY - minY) * 0.05f;

    {
        QPainter painter(&image);

        float w = maxX - minX;
        float h = maxY - minY;

        float max = qMax(w, h);

        painter.save();
        painter.scale(size / max, size / max);
        painter.translate(-QPointF(minX, minY));

        float fontSize = 18 * max / size;

        QFont font = painter.font();
        font.setPixelSize(qCeil(fontSize));

        painter.setFont(font);

        if (!path.isEmpty()) {
            painter.save();
            painter.setBrush(Qt::darkGray);
            painter.drawPath(path);
            painter.setOpacity(1);
            painter.restore();
        }

        for (int i = 0; i < triangles.size(); ++i) {
            QtPathTriangle triangle = triangles.at(i);
            QPolygonF polygon;
            polygon.append(vertices.at(triangle.v1Index).point.toPointF());
            polygon.append(vertices.at(triangle.v2Index).point.toPointF());
            polygon.append(vertices.at(triangle.v3Index).point.toPointF());

            QPen pen(Qt::red, 3);
            pen.setCosmetic(true);

            painter.setPen(pen);
            painter.drawPolygon(polygon);

            painter.setPen(Qt::white);
            painter.drawText(vertices.at(triangle.v1Index).point.toPointF(),
                             QString::number(triangle.v1Index));
            painter.drawText(vertices.at(triangle.v2Index).point.toPointF(),
                             QString::number(triangle.v2Index));
            painter.drawText(vertices.at(triangle.v3Index).point.toPointF(),
                             QString::number(triangle.v3Index));
        }

        for (int i = 0; i < highlightedTriangles.size(); ++i) {
            int highlightedTriangle = highlightedTriangles.at(i);
            QColor color = i > 0 ? Qt::magenta : Qt::green;

            QtPathTriangle triangle = triangles.at(highlightedTriangle);
            QPolygonF polygon;
            polygon.append(vertices.at(triangle.v1Index).point.toPointF());
            polygon.append(vertices.at(triangle.v2Index).point.toPointF());
            polygon.append(vertices.at(triangle.v3Index).point.toPointF());

            QPen pen(color, 3);
            pen.setCosmetic(true);

            painter.setPen(pen);
            painter.drawPolygon(polygon);

        }

        for (int i = 0; i < highlightedPoints.size(); ++i) {
            int highlightedPoint = highlightedPoints.at(i);
            QPointF point = vertices.at(highlightedPoint).point.toPointF();

            painter.setBrush(Qt::yellow);

            QPen pen(Qt::yellow, 3);
            pen.setCosmetic(true);
            painter.setPen(pen);
            painter.drawEllipse(point, 5 * max / size, 5 * max / size);
        }

        for (int i = 0; i < highlightedEdges.size(); ++i) {
            auto highlightedEdge = highlightedEdges.at(i);
            QLineF line(vertices.at(highlightedEdge.first).point.toPointF(),
                        vertices.at(highlightedEdge.second).point.toPointF());

            QColor color = i == 0 ? Qt::yellow : Qt::cyan;
            QPen pen(color, 3);
            pen.setCosmetic(true);
            painter.setPen(pen);

            painter.drawLine(line);

        }

        painter.setPen(Qt::white);
        painter.setBrush(Qt::white);

        painter.restore();

        QFont f = painter.font();
        f.setPixelSize(18);
        painter.setFont(f);
        QFontMetricsF fm(f);

        painter.fillRect(QRectF(0, size - fm.height() * 3, size, fm.height() * 3), Qt::red);
        painter.drawText(QRectF(0, size - fm.height() * 3, size, fm.height() * 3),
                         Qt::AlignCenter,
                         QStringLiteral("%1, %2 triangles").arg(label).arg(triangles.size()));
    }

    image.save(QStringLiteral("delaunay%1.png").arg(counter++));
}

// Checks if the quadrilateral made up of triangle1 and triangle2 is strictly convex
inline bool isConvexQuadrilateral(const QList<QtPathVertex> &vertices,
                                  const QtPathTriangle &triangle1,
                                  const QtPathTriangle &triangle2)
{
    quint32 quad[4];

    findQuad(triangle1, triangle2, &quad[0], &quad[1], &quad[2], &quad[3]);

    int mask = 0;
    // Check if the cross product of all adjacent lines points the same way
    for (int i = 0; i < 4 && mask <= 2; ++i) {
        quint32 p1 = quad[i];
        quint32 p2 = quad[(i + 1) % 4];
        quint32 p3 = quad[(i + 2) % 4];

        QVector2D v1 = vertices.at(p2).point - vertices.at(p1).point;
        QVector2D v2 = vertices.at(p3).point - vertices.at(p2).point;

        // Parallel lines
        if ((qFuzzyIsNull(v1.x()) && qFuzzyIsNull(v2.x()))
             || (qFuzzyIsNull(v1.y()) && qFuzzyIsNull(v2.y()))) {
            mask = 3;
        }

        if (!qFuzzyIsNull(v1.x()) && qFuzzyIsNull(v2.x()) && qFuzzyIsNull(v1.y()) && qFuzzyIsNull(v2.y())) {
            float v1Slope = v1.y() / v1.x();
            float v2Slope = v2.y() / v2.x();

            if (mask <= 2 && (qFuzzyCompare(v1Slope, v2Slope) || qFuzzyCompare(v1Slope, -v2Slope)))
                mask = 3;
        }

//        if (mask <= 2 && (v1.normalized() == v2.normalized() || v1.normalized() == -v2.normalized()))
//            mask |= 3;

        if (mask <= 2) {
            float c = v1.x() * v2.y() - v1.y() * v2.x();
            if (c >= 0.0f)
                mask |= 1;
            else
                mask |= 2;
        }


    }
    return mask <= 2;
}

inline bool lineIntersects(const QVector2D &v1_, const QVector2D &v2_,
                           const QVector2D &u1_, const QVector2D &u2_)
{
#if 1
    QPointF v1 = v1_.toPointF();
    QPointF v2 = v2_.toPointF();
    QPointF u1 = u1_.toPointF();
    QPointF u2 = u2_.toPointF();

    if ( (v1.x() < u1.x() && v1.x() < u2.x() && v2.x() < u1.x() && v2.x() < u2.x())
        || (v1.x() > u1.x() && v1.x() > u2.x() && v2.x() > u1.x() && v2.x() > u2.x())
        || (v1.y() < u1.y() && v1.y() < u2.y() && v2.y() < u1.y() && v2.y() < u2.y())
        || (v1.y() > u1.y() && v1.y() > u2.y() && v2.y() > u1.y() && v2.y() > u2.y())) {
        return false;
    }

    // Copied from QLineF::intersects() with some changes to ordering to speed
    // up for our use case
    const QPointF a = v2 - v1;
    const QPointF b = u1 - u2;
    const QPointF c = v1 - u1;

    const qreal denominator = a.y() * b.x() - a.x() * b.y();
    if (denominator == 0.0 || !qt_is_finite(denominator))
        return false;

    const qreal reciprocal = 1.0 / denominator;
    qreal na = (b.y() * c.x() - b.x() * c.y()) * reciprocal;
    if (na < 0.0 || na > 1.0)
        return false;

    qreal nb = (a.x() * c.y() - a.y() * c.x()) * reciprocal;
    if (nb < 0.0 || nb > 1.0)
        return false;

    const QPointF intersectionPoint = v1 + a * na;
    return intersectionPoint != v1 && intersectionPoint != v2 && intersectionPoint != u1 && intersectionPoint != u2;
    return true;
#else
    QLineF v(v1.toPointF(), v2.toPointF());
    QLineF u(u1.toPointF(), u2.toPointF());

    QPointF intersectionPoint;
    if (v.intersects(u, &intersectionPoint) == QLineF::BoundedIntersection) {
        return (intersectionPoint != v.p1() && intersectionPoint != v.p2()
                && intersectionPoint != u.p1() && intersectionPoint != u.p2());
    }

    return false;
#endif
}

// Returns edges (and the adjacent triangles corresponding to them) which have visibility
// to the vertex
static void findAdjacentTrianglesOnSameSide(const QList<QtPathVertex> &vertices,
                                            quint32 vertexIdx,
                                            const QtPathTriangle &triangle,
                                            QVarLengthArray<quint32, 2> *possibleTriangles,
                                            QVarLengthArray<QPair<quint32, quint32>, 2> *edges = nullptr)
{
    Q_ASSERT(possibleTriangles != nullptr);
    QVector2D v = vertices.at(vertexIdx).point;
    QVector2D v1 = vertices.at(triangle.v1Index).point;
    QVector2D v2 = vertices.at(triangle.v2Index).point;
    QVector2D v3 = vertices.at(triangle.v3Index).point;

    bool d = ( ( ( (v2.x() - v1.x()) * (v3.y() - v1.y()) ) - ( (v3.x() - v1.x()) * (v2.y() - v1.y()) ) )) <= 0.0f;

    // Check v1 - v2
    if (triangle.adjacentTriangle1 != quint32(-1)) {
        bool d1 = ((v.x() - v1.x()) * (v2.y() - v1.y()) - (v.y() - v1.y()) * (v2.x() - v1.x())) <= 0.0f;
        if (d1 == d) {
            possibleTriangles->append(triangle.adjacentTriangle1);
            if (edges != nullptr) {
                if (triangle.v1Index < triangle.v2Index)
                    edges->append(qMakePair(triangle.v1Index, triangle.v2Index));
                else
                    edges->append(qMakePair(triangle.v2Index, triangle.v1Index));
            }
        }
    }

    // Check v2 - v3
    if (triangle.adjacentTriangle2 != quint32(-1)) {
        bool d2 = ((v.x() - v2.x()) * (v3.y() - v2.y()) - (v.y() - v2.y()) * (v3.x() - v2.x())) <= 0.0f;
        if (d2 == d) {
            possibleTriangles->append(triangle.adjacentTriangle2);
            if (edges != nullptr) {
                if (triangle.v2Index < triangle.v3Index)
                    edges->append(qMakePair(triangle.v2Index, triangle.v3Index));
                else
                    edges->append(qMakePair(triangle.v3Index, triangle.v2Index));
            }
        }
    }

    // Check v3 - v1
    if (triangle.adjacentTriangle3 != quint32(-1)) {
        bool d3 = ((v.x() - v3.x()) * (v1.y() - v3.y()) - (v.y() - v3.y()) * (v1.x() - v3.x())) <= 0.0f;
        if (d3 == d) {
            possibleTriangles->append(triangle.adjacentTriangle3);
            if (edges != nullptr) {
                if (triangle.v3Index < triangle.v1Index)
                    edges->append(qMakePair(triangle.v3Index, triangle.v1Index));
                else
                    edges->append(qMakePair(triangle.v1Index, triangle.v3Index));
            }
        }
    }
}

static quint32 findAdjacentTriangleOnSameSide(const QList<QtPathVertex> &vertices,
                                              quint32 vertexIdx,
                                              const QtPathTriangle &triangle)
{
    QVarLengthArray<quint32, 2> possibleTriangles;
    findAdjacentTrianglesOnSameSide(vertices, vertexIdx, triangle, &possibleTriangles);

    if (possibleTriangles.size() == 0) {
        qCWarning(lcShapeCurveRenderer) << "Point" << vertexIdx
                                      << "is not on the outside of any edge of triangle"
                                      << triangle.v1Index << triangle.v2Index << triangle.v3Index;
        return quint32(-1);
    }

    return possibleTriangles.at(0);
}

// CDT using Incremental Algorithm approach.
// the algorithm first inserts the edge's endpoints into the triangulation.
// Then, it finds the sequence of triangles intersected by the constraint
// edge and modifies them to include the edge.
QList<QtPathTriangle> qtDelaunayTriangulator(const QList<QtPathVertex> &_vertices,
                                             const QList<QtPathEdge> &edges,
                                             const QPainterPath &path)
{
    static bool saveImageOnError = qEnvironmentVariableIntValue("QT_QUICKSHAPES_SAVE_IMAGE_ON_ERROR") != 0;
    static bool drawAllImages = qEnvironmentVariableIntValue("QT_QUICKSHAPES_SAVE_DEBUG_IMAGE") != 0;
    static bool normalizeCoordinates = qEnvironmentVariableIntValue("QT_QUICKSHAPES_NORMALIZE_COORDINATES") != 0;
    static bool validateResults = qEnvironmentVariableIntValue("QT_QUICKSHAPES_VALIDATE_RESULTS");

    if (_vertices.isEmpty())
        return {};

    QList<QtPathVertex> vertices;
    if (Q_UNLIKELY(normalizeCoordinates)) {
        vertices.reserve(_vertices.size());

        qreal minX, minY, maxX, maxY;
        for (int i = 0; i < _vertices.size(); ++i) {
            const QtPathVertex &v = _vertices.at(i);
            if (i == 0 || v.point.x() < minX)
                minX = v.point.x();
            if (i == 0 || v.point.x() > maxX)
                maxX = v.point.x();
            if (i == 0 || v.point.y() < minY)
                minY = v.point.y();
            if (i == 0 || v.point.y() > maxY)
                maxY = v.point.y();
        }

        for (int i = 0; i < _vertices.size(); ++i) {
            QtPathVertex v = _vertices.at(i);
            v.point -= QVector2D(minX, minY);
            v.point /= QVector2D(maxX - minX, maxY - minY);
            vertices.append(v);
        }
    } else {
        vertices = _vertices;
    }

    QList<QtPathTriangle> triangulation;
    triangulation.reserve(2 * vertices.size() + 1);

    // Create a super-triangle
    const quint32 l = int(vertices.size());
    QtPathTriangle superTriangle = { l - 3, l - 2, l - 1, 0};
    triangulation.append(superTriangle);

    QElapsedTimer t;
    t.start();

    typedef QPair<quint32, quint32> Pair;

    QHash<Pair, Pair> existingEdges;
    QSet<Pair> constrainedEdges;
    existingEdges.reserve(3 * (2 * vertices.size() + 1));

    auto registerAdjacency = [&triangulation](quint32 a, quint32 b, quint32 triangleIdx1, quint32 triangleIdx2) {
        Q_ASSERT(triangleIdx1 < triangulation.size());
        Q_ASSERT(triangleIdx2 < triangulation.size());

        QtPathTriangle &t1 = triangulation[triangleIdx1];
        QtPathTriangle &t2 = triangulation[triangleIdx2];

        if ((t1.v1Index == a && t1.v2Index == b) || (t1.v1Index == b && t1.v2Index == a)) {
            t1.adjacentTriangle1 = triangleIdx2;
        } else if ((t1.v2Index == a && t1.v3Index == b) || (t1.v2Index == b && t1.v3Index == a)) {
            t1.adjacentTriangle2 = triangleIdx2;
        } else {
            Q_ASSERT(((t1.v3Index == a && t1.v1Index == b) || (t1.v3Index == b && t1.v1Index == a)));
            t1.adjacentTriangle3 = triangleIdx2;
        }

        if ((t2.v1Index == a && t2.v2Index == b) || (t2.v1Index == b && t2.v2Index == a)) {
            t2.adjacentTriangle1 = triangleIdx1;
        } else if ((t2.v2Index == a && t2.v3Index == b) || (t2.v2Index == b && t2.v3Index == a)) {
            t2.adjacentTriangle2 = triangleIdx1;
        } else {
            Q_ASSERT((t2.v3Index == a && t2.v1Index == b) || (t2.v3Index == b && t2.v1Index == a));
            t2.adjacentTriangle3 = triangleIdx1;
        }
    };

    auto unregisterAdjacency = [&triangulation](quint32 triangleIdx1, quint32 triangleIdx2) {
        Q_ASSERT(triangleIdx1 < triangulation.size());
        Q_ASSERT(triangleIdx2 < triangulation.size());

        QtPathTriangle &t1 = triangulation[triangleIdx1];
        QtPathTriangle &t2 = triangulation[triangleIdx2];

        if (t1.adjacentTriangle1 == triangleIdx2) {
            t1.adjacentTriangle1 = quint32(-1);
        } else if (t1.adjacentTriangle2 == triangleIdx2) {
            t1.adjacentTriangle2 = quint32(-1);
        } else {
            Q_ASSERT(t1.adjacentTriangle3 == triangleIdx2);
            t1.adjacentTriangle3 = quint32(-1);
        }

        if (t2.adjacentTriangle1 == triangleIdx1) {
            t2.adjacentTriangle1 = quint32(-1);
        } else if (t2.adjacentTriangle2 == triangleIdx1) {
            t2.adjacentTriangle2 = quint32(-1);
        } else {
            Q_ASSERT(t2.adjacentTriangle3 == triangleIdx1);
            t2.adjacentTriangle3 = quint32(-1);
        }
    };

    auto insertOrdered = [&existingEdges, &registerAdjacency](quint32 a, quint32 b, quint32 triangleIdx) {
        if (a > b)
            qSwap(a, b);

        Pair edge = qMakePair(a, b);
        auto it = existingEdges.find(edge);
        if (it == existingEdges.end()) {
            existingEdges.insert(edge, qMakePair(triangleIdx, quint32(-1)));
        } else {
            if (it.value().first == quint32(-1)) {
                Q_ASSERT(triangleIdx != it.value().second);
                it.value().first = triangleIdx;
            } else {
                Q_ASSERT(it.value().second == quint32(-1));
                Q_ASSERT(triangleIdx != it.value().first);
                it.value().second = triangleIdx;
            }

            if (it.value().first != quint32(-1) && it.value().second != quint32(-1))
                registerAdjacency(a, b, it.value().first, it.value().second);
        }
    };

    auto removeTriangleEdge = [&existingEdges, &unregisterAdjacency](quint32 a, quint32 b, quint32 triangleIdx) {
        if (a > b)
            qSwap(a, b);

        Pair edge = qMakePair(a, b);
        auto it = existingEdges.find(edge);
        Q_ASSERT(it != existingEdges.end());

        if (it.value().first != quint32(-1) && it.value().second != quint32(-1))
            unregisterAdjacency(it.value().first, it.value().second);

        // Mark that triangleIdx no longer shares the edge
        if (it.value().first == triangleIdx)
            it.value().first = quint32(-1);
        else
            it.value().second = quint32(-1);


        // No more triangles refer to this (== it was the old diagonal), so we
        // remove it from the existing edges list
        if (it.value().first == quint32(-1) && it.value().second == quint32(-1))
            it = existingEdges.erase(it);
    };

    insertOrdered(superTriangle.v1Index, superTriangle.v2Index, 0);
    insertOrdered(superTriangle.v2Index, superTriangle.v3Index, 0);
    insertOrdered(superTriangle.v3Index, superTriangle.v1Index, 0);

    // Insert vertices one by one
    // For each vertex, we search for the triangle that contains it and then split this at the
    // point. We then check the Delaunay property of the new triangles and their neighbours, and
    // swap diagonals to restore Delaunayness when needed.
    quint32 lastFormedTriangle = 0;
    for (quint32 vertexIdx = 0; vertexIdx < l - 3; ++vertexIdx) {
        const QtPathVertex &vertex = vertices.at(vertexIdx);

        if (Q_UNLIKELY(drawAllImages)) {
            saveDebugImage(true,
                           path,
                           QStringLiteral("Adding vertex %1: %2, %3").arg(vertexIdx).arg(vertex.point.x()).arg(vertex.point.y()),
                           triangulation,
                           vertices,
                           QList<quint32>{} << vertexIdx);
        }

        QStack<QPair<quint32, quint32> > adjacentTriangles;
        auto findAdjacent = [&existingEdges, &vertexIdx, &adjacentTriangles](quint32 i1, quint32 i2, quint32 triangleIdx) {
            if (i1 != vertexIdx && i2 != vertexIdx) {

                if (i1 > i2)
                    qSwap(i1, i2);
                Pair key = qMakePair(i1, i2);
                auto it = existingEdges.find(key);
                Q_ASSERT(it.value().first == triangleIdx || it.value().second == triangleIdx);

                if (it.value().first != quint32(-1) && it.value().second != quint32(-1)) {
                    if (it.value().first == triangleIdx)
                        adjacentTriangles.append(qMakePair(it.value().second, triangleIdx));
                    else
                        adjacentTriangles.append(qMakePair(it.value().first, triangleIdx));
                }
            }
        };

        {
            quint32 triangleIdx1 = quint32(-1);

            // Start with the last formed triangle (or the super triangle on the first run)
            // Use Lawson's visibility walk to find the triangle containing the point:
            // First check the current triangle (starting with the last formed). If it does not
            // contain the point, the next triangle to check is the one of the adjacent triangles
            // in the direction of the point. This is done using a "visibility" check, i.e. whether
            // the point is on the exterior half-plane of the shared edge.)
            triangleIdx1 = lastFormedTriangle;
            while (triangleIdx1 != quint32(-1)
                   && !isPointInTriangle(vertex.point,
                                         vertices[triangulation.at(triangleIdx1).v1Index].point,
                                         vertices[triangulation.at(triangleIdx1).v2Index].point,
                                         vertices[triangulation.at(triangleIdx1).v3Index].point)) {
                QtPathTriangle &triangle = triangulation[triangleIdx1];
                if (triangle.lastSeenVertex != quint32(-1) && triangle.lastSeenVertex == vertexIdx) {
                    qCWarning(lcShapeCurveRenderer) << "Revisiting triangle which has previously been checked";
                    triangleIdx1 = quint32(-1);
                    break;
                }

                triangle.lastSeenVertex = vertexIdx;

                // If the point is not in the current triangle, we find the edge(s) where the
                // point is on the outside of the triangle. The adjacent triangle to this edge
                // is the next in the search. If there are two such edges we pick the first.
                // As long as the triangulation is a Delaunay triangulation, this should always
                // find the triangle, but we keep a fail safe in case of bugs.
                quint32 nextTriangle = findAdjacentTriangleOnSameSide(vertices,
                                                                      vertexIdx,
                                                                      triangle);
                if (Q_UNLIKELY(drawAllImages)) {
                    saveDebugImage(true,
                                   path,
                                   QStringLiteral("Marching from %1 to %2 (%3, %4, %5)").arg(triangleIdx1).arg(nextTriangle),
                                   triangulation,
                                   vertices,
                                   QList<quint32>{} << vertexIdx,
                                   QList<quint32>{} << (nextTriangle != quint32(-1) ? nextTriangle : 0) << triangleIdx1,
                                   QList<Pair>{});
                }

                triangleIdx1 = nextTriangle;
            }

            // Fail safe, if we failed to find the triangle using the optimized step, we do a brute
            // force search.
            if (validateResults || triangleIdx1 == quint32(-1)) {
                if (triangleIdx1 == quint32(-1))
                    qCWarning(lcShapeCurveRenderer) << "Can't find triangle using Lawson. Falling back to brute force";
                quint32 otherTriangle = triangleIdx1;
                triangleIdx1 = 0;
                for (; triangleIdx1 < triangulation.size(); ++triangleIdx1) {
                    const QtPathTriangle &triangle = triangulation.at(triangleIdx1);
                    if (isPointInTriangle(vertex.point, vertices[triangle.v1Index].point,
                                          vertices[triangle.v2Index].point,
                                          vertices[triangle.v3Index].point)) {
                        break;
                    }
                }

                if (validateResults && otherTriangle != triangleIdx1 && triangleIdx1 != quint32(-1) && otherTriangle != quint32(-1)) {
                    qCWarning(lcShapeCurveRenderer) << "Lawson matched" << otherTriangle
                                                  << "whereas brute force matched" << triangleIdx1
                                                  << "Lawson:" << vertices.at(triangulation.at(otherTriangle).v1Index).point
                                                  << vertices.at(triangulation.at(otherTriangle).v2Index).point
                                                  << vertices.at(triangulation.at(otherTriangle).v3Index).point
                                                  << "Brute:" << vertices.at(triangulation.at(triangleIdx1).v1Index).point
                                                  << vertices.at(triangulation.at(triangleIdx1).v2Index).point
                                                  << vertices.at(triangulation.at(triangleIdx1).v3Index).point
                                                  << "Testing point:" << vertices.at(vertexIdx).point;

                    saveDebugImage(saveImageOnError,
                                   path,
                                   QStringLiteral("Two triangles containing the same point"),
                                   triangulation,
                                   vertices,
                                   QList<quint32>{} << vertexIdx,
                                   QList<quint32>{} << triangleIdx1 << otherTriangle);
                }
            }

            if (Q_UNLIKELY(drawAllImages)) {
                saveDebugImage(true,
                               path,
                               QStringLiteral("Splitting triangle"),
                               triangulation,
                               vertices,
                               QList<quint32>{} << vertexIdx,
                               QList<quint32>{} << triangleIdx1);
            }

            if (triangleIdx1 >= triangulation.size()) {
                qCWarning(lcShapeCurveRenderer) << "Unable to find any triangle containing point"
                                              << vertex.point
                                              << ". Aborting.";
                return QList<QtPathTriangle>{};
            }

            // Split triangle into three, connecting to the new vertex
            quint32 t1v1Index;
            quint32 t1v2Index;
            quint32 t1v3Index;
            {
                QtPathTriangle &t1 = triangulation[triangleIdx1];
                t1v1Index = t1.v1Index;
                t1v2Index = t1.v2Index;
                t1v3Index = t1.v3Index;
                t1.v3Index = vertexIdx;

                removeTriangleEdge(t1v2Index, t1v3Index, triangleIdx1);
                removeTriangleEdge(t1v3Index, t1v1Index, triangleIdx1);
            }


            quint32 triangleIdx2 = triangulation.size();
            triangulation.append(QtPathTriangle(t1v2Index, t1v3Index, vertexIdx, triangulation.count()));
            quint32 triangleIdx3 = triangulation.size();
            triangulation.append(QtPathTriangle(t1v3Index, t1v1Index, vertexIdx, triangulation.count()));
            const QtPathTriangle &t2 = triangulation.at(triangleIdx2);
            const QtPathTriangle &t3 = triangulation.at(triangleIdx3);

            lastFormedTriangle = triangleIdx3;

            t1v3Index = vertexIdx;

            insertOrdered(t1v2Index, t1v3Index, triangleIdx1);
            insertOrdered(t1v3Index, t1v1Index, triangleIdx1);

            insertOrdered(t2.v1Index, t2.v2Index, triangleIdx2);
            insertOrdered(t2.v2Index, t2.v3Index, triangleIdx2);
            insertOrdered(t2.v3Index, t2.v1Index, triangleIdx2);

            insertOrdered(t3.v1Index, t3.v2Index, triangleIdx3);
            insertOrdered(t3.v2Index, t3.v3Index, triangleIdx3);
            insertOrdered(t3.v3Index, t3.v1Index, triangleIdx3);

            // Find triangles sharing edges with new triangles and push to stack
            findAdjacent(t1v1Index, t1v2Index, triangleIdx1);
            findAdjacent(t2.v1Index, t2.v2Index, triangleIdx2);
            findAdjacent(t3.v1Index, t3.v2Index, triangleIdx3);
        }

        // Check adjacent triangles to see that they do not contain the introduced vertex in their
        // circumcircle. In cases where this happens, we take the quadrilateral consisting of the
        // new triangle and its neighbour and turn it into two new triangles by swapping to the
        // other diagonal.
        while (!adjacentTriangles.isEmpty()) {
            QPair<quint32, quint32> pair = adjacentTriangles.pop();
            quint32 triangleIdx1 = pair.first;
            quint32 triangleIdx2 = pair.second;

            // If opposing triangle circumcircle contains the vertex we are adding, we swap
            // the diagonal of the quad made of the two opposing triangles
            QtPathTriangle &triangle1 = triangulation[triangleIdx1];
            QtPathTriangle &triangle2 = triangulation[triangleIdx2];
            if (isPointInCircumcircle(vertex.point, vertices[triangle1.v1Index].point,
                                      vertices[triangle1.v2Index].point,
                                      vertices[triangle1.v3Index].point)) {
                if (Q_UNLIKELY(drawAllImages)) {
                    saveDebugImage(true,
                                   path,
                                   QStringLiteral("About to swap diagonal"),
                                   triangulation,
                                   vertices,
                                   QList<quint32>{} << vertexIdx,
                                   QList<quint32>{} << triangleIdx1 << triangleIdx2);
                }

                removeTriangleEdge(triangle1.v1Index, triangle1.v2Index, triangleIdx1);
                removeTriangleEdge(triangle1.v2Index, triangle1.v3Index, triangleIdx1);
                removeTriangleEdge(triangle1.v3Index, triangle1.v1Index, triangleIdx1);

                removeTriangleEdge(triangle2.v1Index, triangle2.v2Index, triangleIdx2);
                removeTriangleEdge(triangle2.v2Index, triangle2.v3Index, triangleIdx2);
                removeTriangleEdge(triangle2.v3Index, triangle2.v1Index, triangleIdx2);

                swapDiagonal(&triangle1, &triangle2, vertexIdx);

                if (Q_UNLIKELY(drawAllImages)) {
                    saveDebugImage(true,
                                   path,
                                   QStringLiteral("Has swapped diagonal"),
                                   triangulation,
                                   vertices,
                                   QList<quint32>{} << vertexIdx,
                                   QList<quint32>{} << triangleIdx1 << triangleIdx2);
                }

                // Add back triangles sharing edges
                insertOrdered(triangle1.v1Index, triangle1.v2Index, triangleIdx1);
                insertOrdered(triangle1.v2Index, triangle1.v3Index, triangleIdx1);
                insertOrdered(triangle1.v3Index, triangle1.v1Index, triangleIdx1);

                insertOrdered(triangle2.v1Index, triangle2.v2Index, triangleIdx2);
                insertOrdered(triangle2.v2Index, triangle2.v3Index, triangleIdx2);
                insertOrdered(triangle2.v3Index, triangle2.v1Index, triangleIdx2);

                findAdjacent(triangle1.v1Index, triangle1.v2Index, triangleIdx1);
                findAdjacent(triangle1.v2Index, triangle1.v3Index, triangleIdx1);
                findAdjacent(triangle1.v3Index, triangle1.v1Index, triangleIdx1);

                findAdjacent(triangle2.v1Index, triangle2.v2Index, triangleIdx2);
                findAdjacent(triangle2.v2Index, triangle2.v3Index, triangleIdx2);
                findAdjacent(triangle2.v3Index, triangle2.v1Index, triangleIdx2);
            }
        }
    }

    qCDebug(lcShapeTiming) << "qtDelaunayTriangulator: Inserting vertices in" << t.elapsed() << "ms";

    // Make a look-up for constrained edges to check self-intersections
    for (const QtPathEdge &constraintEdge : edges) {
        quint32 a = constraintEdge.startIndex;
        quint32 b = constraintEdge.endIndex;
        if (a > b)
            qSwap(a, b);
        constrainedEdges.insert(qMakePair(a, b));
    }

    // Insert constraint edges:
    // For each constrained edge, we make sure it is in the triangulation: We first check if it
    // already happens to be, and if so we do nothing. If it is not, we check which edges in the
    // current graph it intersects, and remove these by swapping diagonals. We then go through
    // newly created edges (except the constrained edge) and swap them for the other diagonals in
    // cases where this is needed to satisfy Delaunay property.
    for (const QtPathEdge &constraintEdge : edges) {
        quint32 a = constraintEdge.startIndex;
        quint32 b = constraintEdge.endIndex;
        if (a > b)
            qSwap(a, b);

        // If edge already exists, we continue
        if (!existingEdges.contains(qMakePair(a, b))) {
            const QVector2D &newEdgeP1 = vertices.at(a).point;
            const QVector2D &newEdgeP2 = vertices.at(b).point;

            // Find all edges that intersect with [a, b]
            QList<Pair> intersectingEdges;
            static bool permitSelfIntersections = qEnvironmentVariableIntValue("QT_QUICKSHAPES_PERMIT_SELF_INTERSECTIONS") != 0;

            // We start search with any triangle containing 'a' as a vertex
            // ### Could we keep track of this as well in the vertex info?
            quint32 triangleIdx = 0;
            while (triangleIdx != quint32(-1)
                   && triangulation.at(triangleIdx).v1Index != a
                   && triangulation.at(triangleIdx).v2Index != a
                   && triangulation.at(triangleIdx).v3Index != a) {
                const QtPathTriangle &triangle = triangulation.at(triangleIdx);
                triangleIdx = findAdjacentTriangleOnSameSide(vertices,
                                                             a,
                                                             triangle);
            }

            if (triangleIdx >= triangulation.size()) {
                qCWarning(lcShapeCurveRenderer)
                    << "qtDelaunayTriangulator: Unable to find any triangle containing vertex"
                    << a << vertices.at(a).point
                    << ". Constrained edge will be missing from triangulation";
                continue;
            }

            // We now circle 'a' by looking at adjacent triangles on edges containing 'a' (making
            // sure we do not go backwards) until we find one which intersects [a, b]. We then
            // use the visibility walk to check triangles in the direction of 'b', recording
            // intersections as we go and moving via intersecting edges. We stop when there are no
            // more intersections.
            quint32 previousTriangleIdx = quint32(-1);
            quint32 startingTriangleIdx = triangleIdx;
            while (triangleIdx != quint32(-1)) {
                quint32 nextTriangle = quint32(-1);
                const QtPathTriangle &triangle = triangulation.at(triangleIdx);

                if (Q_UNLIKELY(drawAllImages)) {
                    saveDebugImage(true,
                                   path,
                                   QStringLiteral("Circling %1 to find first intersection (triangle: %2, %3, %4)").arg(a).arg(triangle.v1Index).arg(triangle.v2Index).arg(triangle.v3Index),
                                   triangulation,
                                   vertices,
                                   QList<quint32>{} << a,
                                   QList<quint32>{} << triangleIdx,
                                   QList<Pair>{} << qMakePair(a, b));
                }

                Pair existingEdge;
                quint32 adjacentTriangle;
                if (triangle.v1Index == a) {
                    existingEdge = qMakePair(triangle.v2Index, triangle.v3Index);
                    adjacentTriangle = triangle.adjacentTriangle2;

                    if (triangle.adjacentTriangle1 != previousTriangleIdx
                        && triangle.adjacentTriangle1 != quint32(-1)) {
                        nextTriangle = triangle.adjacentTriangle1; // v1-v2
                    } else {
                        nextTriangle = triangle.adjacentTriangle3; // v3-v1
                    }
                } else if (triangle.v2Index == a) {
                    existingEdge = qMakePair(triangle.v3Index, triangle.v1Index);
                    adjacentTriangle = triangle.adjacentTriangle3;

                    if (triangle.adjacentTriangle1 != previousTriangleIdx
                        && triangle.adjacentTriangle1 != quint32(-1)) {
                        nextTriangle = triangle.adjacentTriangle1; // v1-v2
                    } else {
                        nextTriangle = triangle.adjacentTriangle2; // v2-v3
                    }
                } else { // a == triangle.v3Index
                    Q_ASSERT(a == triangle.v3Index);
                    existingEdge = qMakePair(triangle.v1Index, triangle.v2Index);
                    adjacentTriangle = triangle.adjacentTriangle1;

                    if (triangle.adjacentTriangle3 != previousTriangleIdx
                        && triangle.adjacentTriangle3 != quint32(-1)) {
                        nextTriangle = triangle.adjacentTriangle3; // v3-v1
                    } else {
                        nextTriangle = triangle.adjacentTriangle2; // v2-v3
                    }
                }

                const QVector2D &existingEdgeP1 = vertices.at(existingEdge.first).point;
                const QVector2D &existingEdgeP2 = vertices.at(existingEdge.second).point;

                if (lineIntersects(newEdgeP1, newEdgeP2, existingEdgeP1, existingEdgeP2)) {
                    // Go to next phase of search, where the intersections are recorded.
                    triangleIdx = adjacentTriangle;
                    if (existingEdge.first > existingEdge.second)
                        qSwap(existingEdge.first, existingEdge.second);
                    intersectingEdges.append(existingEdge);

                    if (!permitSelfIntersections && constrainedEdges.contains(existingEdge)) {
                        qCWarning(lcShapeCurveRenderer)
                            << "qtDelaunayTriangulator: Exiting early due to self-intersection."
                            << "Edge 1:" << a << b << "(" << newEdgeP1 << newEdgeP2 << ")"
                            << "Edge 2:" << existingEdge.first << existingEdge.second << "(" << existingEdgeP1 << existingEdgeP2 << ")";
                        saveDebugImage(saveImageOnError,
                                       path,
                                       QStringLiteral("Found intersection of two constrained edges. No solution."),
                                       triangulation,
                                       vertices,
                                       QList<quint32>{},
                                       QList<quint32>{},
                                       QList<QPair<quint32, quint32> >{}
                                           << qMakePair(a, b)
                                           << qMakePair(existingEdge.first, existingEdge.second));
                        return QList<QtPathTriangle>{};
                    }

                    break;
                }

                previousTriangleIdx = triangleIdx;
                triangleIdx = nextTriangle;

                // No intersection found
                if (triangleIdx == startingTriangleIdx)
                    triangleIdx = quint32(-1);
            }

            // Now we march towards 'b' and record all intersections
            while (triangleIdx != quint32(-1)) {
                const QtPathTriangle &triangle = triangulation.at(triangleIdx);

                QVarLengthArray<quint32, 2> possibleTriangles;
                QVarLengthArray<Pair, 2> edges;

                findAdjacentTrianglesOnSameSide(vertices, b, triangle, &possibleTriangles, &edges);

                // Check edge(s) which can see 'b' for intersections
                triangleIdx = quint32(-1);
                for (int i = 0; i < edges.size(); ++i) {
                    const Pair &existingEdge = edges.at(i);

                    const QVector2D &existingEdgeP1 = vertices.at(existingEdge.first).point;
                    const QVector2D &existingEdgeP2 = vertices.at(existingEdge.second).point;

                    if (existingEdge.first != a
                            && existingEdge.second != a
                            && lineIntersects(newEdgeP1, newEdgeP2, existingEdgeP1, existingEdgeP2)) {
                        if (!permitSelfIntersections && constrainedEdges.contains(existingEdge)) {
                            qCWarning(lcShapeCurveRenderer)
                                << "qtDelaunayTriangulator: Exiting early due to self-intersection."
                                << "Edge 1:" << a << b << "(" << newEdgeP1 << newEdgeP2 << ")"
                                << "Edge 2:" << existingEdge.first << existingEdge.second << "(" << existingEdgeP1 << existingEdgeP2 << ")";
                            saveDebugImage(saveImageOnError,
                                           path,
                                           QStringLiteral("Found intersection of two constrained edges. No solution."),
                                           triangulation,
                                           vertices,
                                           QList<quint32>{},
                                           QList<quint32>{},
                                           QList<QPair<quint32, quint32> >{}
                                               << qMakePair(a, b)
                                               << qMakePair(existingEdge.first, existingEdge.second));
                            return QList<QtPathTriangle>{};
                        }

                        intersectingEdges.append(existingEdge);
                        triangleIdx = possibleTriangles[i];

                        if (Q_UNLIKELY(drawAllImages)) {
                            saveDebugImage(true,
                                           path,
                                           QStringLiteral("Marching to %1 (triangle: %2, %3, %4)")
                                               .arg(b)
                                               .arg(triangulation.at(possibleTriangles.first()).v1Index)
                                               .arg(triangulation.at(possibleTriangles.first()).v2Index)
                                               .arg(triangulation.at(possibleTriangles.first()).v3Index),
                                           triangulation,
                                           vertices,
                                           QList<quint32>{} << b,
                                           QList<quint32>{} << triangleIdx << possibleTriangles.first(),
                                           QList<Pair>{} << qMakePair(a, b) << existingEdge);
                        }
                    }
                }
            }

            if (Q_UNLIKELY(validateResults)) {
                QList<Pair> intersectingEdgesFound = intersectingEdges;
                intersectingEdges.clear();
                for (auto it = existingEdges.constBegin(); it != existingEdges.constEnd(); ++it) {
                    const Pair &existingEdge = it.key();

                    const QVector2D &existingEdgeP1 = vertices.at(existingEdge.first).point;
                    const QVector2D &existingEdgeP2 = vertices.at(existingEdge.second).point;

                    if (existingEdge.first != a && existingEdge.second != a
                            && existingEdge.first != b && existingEdge.second != b
                            && lineIntersects(newEdgeP1, newEdgeP2, existingEdgeP1, existingEdgeP2)) {
                        if (!permitSelfIntersections && constrainedEdges.contains(existingEdge)) {
                            qCWarning(lcShapeCurveRenderer)
                                    << "qtDelaunayTriangulator: Exiting early due to self-intersection."
                                    << "Edge 1:" << a << b << "(" << newEdgeP1 << newEdgeP2 << ")"
                                    << "Edge 2:" << existingEdge.first << existingEdge.second << "(" << existingEdgeP1 << existingEdgeP2 << ")";
                            saveDebugImage(saveImageOnError,
                                           path,
                                           QStringLiteral("Found intersection of two constrained edges. No solution."),
                                           triangulation,
                                           vertices,
                                           QList<quint32>{},
                                           QList<quint32>{},
                                           QList<QPair<quint32, quint32> >{} << qMakePair(a, b) << qMakePair(existingEdge.first, existingEdge.second));
                            return QList<QtPathTriangle>{};
                        }

                        intersectingEdges.append(existingEdge);

                        if (Q_UNLIKELY(drawAllImages)) {
                            saveDebugImage(true,
                                           path,
                                           QStringLiteral("Found intersection of constrained edge (%1 so far)").arg(intersectingEdges.size()),
                                           triangulation,
                                           vertices,
                                           QList<quint32>{},
                                           QList<quint32>{},
                                           QList<QPair<quint32, quint32> >{} << qMakePair(a, b) << qMakePair(existingEdge.first, existingEdge.second));
                        }
                    }
                }

                for (const Pair &intersectingEdge : intersectingEdges) {
                    bool found = false;
                    for (const Pair &otherIntersectingEdge : intersectingEdgesFound) {
                        if (otherIntersectingEdge == intersectingEdge || qMakePair(otherIntersectingEdge.second, otherIntersectingEdge.first) == intersectingEdge) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        qCWarning(lcShapeCurveRenderer) << "Intersecting edge" << intersectingEdge.first << intersectingEdge.second << "not found with smart algorithm. Constrained edge:" << a << b;

                        saveDebugImage(saveImageOnError,
                                       path,
                                       QStringLiteral("Intersecting edge not found"),
                                       triangulation,
                                       vertices,
                                       QList<quint32>{},
                                       QList<quint32>{},
                                       QList<QPair<quint32, quint32> >{} << qMakePair(a, b) << qMakePair(intersectingEdge.first, intersectingEdge.second));

                        return QList<QtPathTriangle>{};
                    }
                }

                for (const Pair &intersectingEdge : intersectingEdgesFound) {
                    bool found = false;
                    for (const Pair &otherIntersectingEdge : intersectingEdges) {
                        if (otherIntersectingEdge == intersectingEdge || qMakePair(otherIntersectingEdge.second, otherIntersectingEdge.first) == intersectingEdge) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        qCWarning(lcShapeCurveRenderer) << "Intersecting edge" << intersectingEdge.first << intersectingEdge.second << "not found with dumb algorithm. Constrained edge:" << a << b;

                        saveDebugImage(saveImageOnError,
                                       path,
                                       QStringLiteral("Intersecting edge not found"),
                                       triangulation,
                                       vertices,
                                       QList<quint32>{},
                                       QList<quint32>{},
                                       QList<QPair<quint32, quint32> >{} << qMakePair(a, b) << qMakePair(intersectingEdge.first, intersectingEdge.second));

                        return QList<QtPathTriangle>{};
                    }
                }
            }

            bool somethingChanged = false;
            auto it = intersectingEdges.begin();
            QSet<Pair> newlyCreatedEdges;
            while (!intersectingEdges.isEmpty()) {
                Q_ASSERT(it != intersectingEdges.end());

                Pair existingEdge = *it;

                Q_ASSERT(existingEdges.contains(existingEdge));

                // ### Since we are now finding edges via triangles, this information can be
                // included in the intersecting edges list and we do not need the extra lookup.
                Pair triangles = existingEdges.value(existingEdge);

                QtPathTriangle &triangle1 = triangulation[triangles.first];
                QtPathTriangle &triangle2 = triangulation[triangles.second];

                // If the triangles do not together make a convex polygon, we skip this for now
                // Otherwise, swap the diagonal
                if (isConvexQuadrilateral(vertices, triangle1, triangle2)) {
                    somethingChanged = true;

                    if (Q_UNLIKELY(drawAllImages)) {
                        saveDebugImage(true,
                                       path,
                                       QStringLiteral("About to swap diagonal"),
                                       triangulation,
                                       vertices,
                                       QList<quint32>{},
                                       QList<quint32>{} << triangles.first << triangles.second,
                                       QList<QPair<quint32, quint32>  >{} << qMakePair(existingEdge.first, existingEdge.second));
                    }

                    // The triangles are being altered, so we have to remove them from the edge
                    // list first
                    removeTriangleEdge(triangle1.v1Index, triangle1.v2Index, triangles.first);
                    removeTriangleEdge(triangle1.v2Index, triangle1.v3Index, triangles.first);
                    removeTriangleEdge(triangle1.v3Index, triangle1.v1Index, triangles.first);

                    removeTriangleEdge(triangle2.v1Index, triangle2.v2Index, triangles.second);
                    removeTriangleEdge(triangle2.v2Index, triangle2.v3Index, triangles.second);
                    removeTriangleEdge(triangle2.v3Index, triangle2.v1Index, triangles.second);

                    // If not, we swap the diagonal of the two triangles
                    swapDiagonal(&triangle1, &triangle2);
                    it = intersectingEdges.erase(it);
                    if (it == intersectingEdges.end()) { // Wrap over end of list
                        it = intersectingEdges.begin();
                        somethingChanged = false;
                    }

                    // Add back triangles sharing edges
                    insertOrdered(triangle1.v1Index, triangle1.v2Index, triangles.first);
                    insertOrdered(triangle1.v2Index, triangle1.v3Index, triangles.first);
                    insertOrdered(triangle1.v3Index, triangle1.v1Index, triangles.first);

                    insertOrdered(triangle2.v1Index, triangle2.v2Index, triangles.second);
                    insertOrdered(triangle2.v2Index, triangle2.v3Index, triangles.second);
                    insertOrdered(triangle2.v3Index, triangle2.v1Index, triangles.second);

                    Pair newDiagonal = triangle1.v1Index < triangle1.v2Index
                            ? qMakePair(triangle1.v1Index, triangle1.v2Index)
                            : qMakePair(triangle1.v2Index, triangle1.v1Index);

                    if (Q_UNLIKELY(drawAllImages)) {
                        saveDebugImage(true,
                                       path,
                                       QStringLiteral("Swapped diagonal"),
                                       triangulation,
                                       vertices,
                                       QList<quint32>{},
                                       QList<quint32>{} << triangles.first << triangles.second,
                                       QList<QPair<quint32, quint32>  >{} << qMakePair(newDiagonal.first, newDiagonal.second));
                    }

                    // By convention, the new diagonal is now the two first vertices in the triangles
                    const QVector2D &newDiagonalP1 = vertices[triangle1.v1Index].point;
                    const QVector2D &newDiagonalP2 = vertices[triangle1.v2Index].point;

                    if (newDiagonal.first != a
                            && newDiagonal.second != a
                            && newDiagonal.first != b
                            && newDiagonal.second != b
                            && lineIntersects(newEdgeP1, newEdgeP2, newDiagonalP1, newDiagonalP2)) {
                        it = intersectingEdges.insert(it, newDiagonal);
                    } else {
                        if (newDiagonal != qMakePair(a, b) && newDiagonal != qMakePair(b, a))
                            newlyCreatedEdges.insert(newDiagonal);

                        continue; // No need to increase iterator, since we erased
                    }
                } else {
                    if (Q_UNLIKELY(drawAllImages)) {
                        saveDebugImage(true,
                                       path,
                                       QStringLiteral("Skipping non-convex quad"),
                                       triangulation,
                                       vertices,
                                       QList<quint32>{},
                                       QList<quint32>{} << triangles.first << triangles.second,
                                       QList<QPair<quint32, quint32>  >{} << qMakePair(existingEdge.first, existingEdge.second));
                    }
                }

                if (++it == intersectingEdges.end()) { // Wrap over end of list
                    if (!somethingChanged && !intersectingEdges.isEmpty()) {
                        intersectingEdges.prepend(qMakePair(a, b)); // Just for visuals
                        saveDebugImage(saveImageOnError,
                                       path,
                                       QStringLiteral("Looped through all edges with no changes. No solution."),
                                       triangulation,
                                       vertices,
                                       QList<quint32>{},
                                       QList<quint32>{},
                                       intersectingEdges);
                        qCWarning(lcShapeCurveRenderer) << "Detected infinite loop. Exiting early.";
                        break;
                    }

                    it = intersectingEdges.begin();
                    somethingChanged = false;
                }
            }

            // Restore the delaunay properties
            for (const Pair &newlyCreatedEdge : newlyCreatedEdges) {
                auto it = existingEdges.find(newlyCreatedEdge);
                Q_ASSERT(it != existingEdges.end());

                Pair triangles = it.value();

                QtPathTriangle &triangle1 = triangulation[triangles.first];
                QtPathTriangle &triangle2 = triangulation[triangles.second];

                bool delaunaySatisfied =
                        !isPointInCircumcircle(vertices.at(triangle1.v1Index).point,
                                               vertices.at(triangle2.v1Index).point,
                                               vertices.at(triangle2.v2Index).point,
                                               vertices.at(triangle2.v3Index).point)
                     && !isPointInCircumcircle(vertices.at(triangle1.v2Index).point,
                                               vertices.at(triangle2.v1Index).point,
                                               vertices.at(triangle2.v2Index).point,
                                               vertices.at(triangle2.v3Index).point)
                     && !isPointInCircumcircle(vertices.at(triangle1.v3Index).point,
                                               vertices.at(triangle2.v1Index).point,
                                               vertices.at(triangle2.v2Index).point,
                                               vertices.at(triangle2.v3Index).point)
                     && !isPointInCircumcircle(vertices.at(triangle2.v1Index).point,
                                               vertices.at(triangle1.v1Index).point,
                                               vertices.at(triangle1.v2Index).point,
                                               vertices.at(triangle1.v3Index).point)
                     && !isPointInCircumcircle(vertices.at(triangle2.v2Index).point,
                                               vertices.at(triangle1.v1Index).point,
                                               vertices.at(triangle1.v2Index).point,
                                               vertices.at(triangle1.v3Index).point)
                     && !isPointInCircumcircle(vertices.at(triangle2.v3Index).point,
                                               vertices.at(triangle1.v1Index).point,
                                               vertices.at(triangle1.v2Index).point,
                                               vertices.at(triangle1.v3Index).point);
                if (!delaunaySatisfied) {
                    removeTriangleEdge(triangle1.v1Index, triangle1.v2Index, triangles.first);
                    removeTriangleEdge(triangle1.v2Index, triangle1.v3Index, triangles.first);
                    removeTriangleEdge(triangle1.v3Index, triangle1.v1Index, triangles.first);

                    removeTriangleEdge(triangle2.v1Index, triangle2.v2Index, triangles.second);
                    removeTriangleEdge(triangle2.v2Index, triangle2.v3Index, triangles.second);
                    removeTriangleEdge(triangle2.v3Index, triangle2.v1Index, triangles.second);

                    // If not, we swap the diagonal of the two triangles
                    swapDiagonal(&triangle1, &triangle2);

                    // Add back triangles sharing edges
                    insertOrdered(triangle1.v1Index, triangle1.v2Index, triangles.first);
                    insertOrdered(triangle1.v2Index, triangle1.v3Index, triangles.first);
                    insertOrdered(triangle1.v3Index, triangle1.v1Index, triangles.first);

                    insertOrdered(triangle2.v1Index, triangle2.v2Index, triangles.second);
                    insertOrdered(triangle2.v2Index, triangle2.v3Index, triangles.second);
                    insertOrdered(triangle2.v3Index, triangle2.v1Index, triangles.second);
                }
            }

            if (Q_UNLIKELY(validateResults)) {
                if (!existingEdges.contains(qMakePair(a, b))) {
                    qCWarning(lcShapeCurveRenderer)
                            << "Unable to produce triangulation with edge" << a << b;
                    saveDebugImage(saveImageOnError,
                                   path,
                                   QStringLiteral("Triangulation missing [%1, %2]").arg(a).arg(b),
                                   triangulation,
                                   vertices,
                                   QList<quint32>{},
                                   QList<quint32>{},
                                   QList<Pair>{} << qMakePair(a, b));
                }
            }
        }
    }

    qCDebug(lcShapeTiming) << "qtDelaunayTriangulator: Inserting edges in" << t.elapsed() << "ms";

    for (auto it = triangulation.begin(); it != triangulation.end(); ++it) {
        QtPathTriangle &triangle = *it;
        if (triangle.v1Index >= l - 3 || triangle.v2Index >= l - 3 || triangle.v3Index >= l - 3)
            triangle.isValid = false;
    }

    return triangulation;
}

QT_END_NAMESPACE
