// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qsgcurveprocessor_p.h"

#include <QtGui/private/qtriangulator_p.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcSGCurveProcessor, "qt.quick.curveprocessor");
Q_LOGGING_CATEGORY(lcSGCurveIntersectionSolver, "qt.quick.curveprocessor.intersections");

namespace {
// Input coordinate space is pre-mapped so that (0, 0) maps to [0, 0] in uv space.
// v1 maps to [1,0], v2 maps to [0,1]. p is the point to be mapped to uv in this space (i.e. vector from p0)
static inline QVector2D uvForPoint(QVector2D v1, QVector2D v2, QVector2D p)
{
    double divisor = v1.x() * v2.y() - v2.x() * v1.y();

    float u = (p.x() * v2.y() - p.y() * v2.x()) / divisor;
    float v = (p.y() * v1.x() - p.x() * v1.y()) / divisor;

    return {u, v};
}

// Find uv coordinates for the point p, for a quadratic curve from p0 to p2 with control point p1
// also works for a line from p0 to p2, where p1 is on the inside of the path relative to the line
static inline QVector2D curveUv(QVector2D p0, QVector2D p1, QVector2D p2, QVector2D p)
{
    QVector2D v1 = 2 * (p1 - p0);
    QVector2D v2 = p2 - v1 - p0;
    return uvForPoint(v1, v2, p - p0);
}

static QVector3D elementUvForPoint(const QQuadPath::Element& e, QVector2D p)
{
    auto uv = curveUv(e.startPoint(), e.referencePoint(), e.endPoint(), p);
    if (e.isLine())
        return { uv.x(), uv.y(), 0.0f };
    else
        return { uv.x(), uv.y(), e.isConvex() ? -1.0f : 1.0f };
}

static inline QVector2D calcNormalVector(QVector2D a, QVector2D b)
{
    auto v = b - a;
    return {v.y(), -v.x()};
}

// The sign of the return value indicates which side of the line defined by a and n the point p falls
static inline float testSideOfLineByNormal(QVector2D a, QVector2D n, QVector2D p)
{
    float dot = QVector2D::dotProduct(p - a, n);
    return dot;
};

static inline float determinant(const QVector2D &p1, const QVector2D &p2, const QVector2D &p3)
{
    return p1.x() * (p2.y() - p3.y())
           + p2.x() * (p3.y() - p1.y())
           + p3.x() * (p1.y() - p2.y());
}

/*
  Clever triangle overlap algorithm. Stack Overflow says:

  You can prove that the two triangles do not collide by finding an edge (out of the total 6
  edges that make up the two triangles) that acts as a separating line where all the vertices
  of one triangle lie on one side and the vertices of the other triangle lie on the other side.
  If you can find such an edge then it means that the triangles do not intersect otherwise the
  triangles are colliding.
*/
using TrianglePoints = std::array<QVector2D, 3>;
using LinePoints = std::array<QVector2D, 2>;

// The sign of the determinant tells the winding order: positive means counter-clockwise

static inline double determinant(const TrianglePoints &p)
{
    return determinant(p[0], p[1], p[2]);
}

// Fix the triangle so that the determinant is positive
static void fixWinding(TrianglePoints &p)
{
    double det = determinant(p);
    if (det < 0.0) {
        qSwap(p[0], p[1]);
    }
}

// Return true if the determinant is negative, i.e. if the winding order is opposite of the triangle p1,p2,p3.
// This means that p is strictly on the other side of p1-p2 relative to p3 [where p1,p2,p3 is a triangle with
// a positive determinant].
bool checkEdge(const QVector2D &p1, const QVector2D &p2, const QVector2D &p, float epsilon)
{
    return determinant(p1, p2, p) <= epsilon;
}

// Check if lines l1 and l2 are intersecting and return the respective value. Solutions are stored to
// the optional pointer solution.
bool lineIntersection(const LinePoints &l1, const LinePoints &l2, QList<QPair<float, float>> *solution = nullptr)
{
    constexpr double eps2 = 1e-5; // Epsilon for parameter space t1-t2

    // see https://www.wolframalpha.com/input?i=solve%28A+%2B+t+*+B+%3D+C+%2B+s*D%3B+E+%2B+t+*+F+%3D+G+%2B+s+*+H+for+s+and+t%29
    const float A = l1[0].x();
    const float B = l1[1].x() - l1[0].x();
    const float C = l2[0].x();
    const float D = l2[1].x() - l2[0].x();
    const float E = l1[0].y();
    const float F = l1[1].y() - l1[0].y();
    const float G = l2[0].y();
    const float H = l2[1].y() - l2[0].y();

    float det = D * F - B * H;

    if (det == 0)
        return false;

    float s = (F * (A - C) - B * (E - G)) / det;
    float t = (H * (A - C) - D * (E - G)) / det;

    // Intersections at 0 count. Intersections at 1 do not.
    bool intersecting = (s >= 0 && s <= 1. - eps2 && t >= 0 && t <= 1. - eps2);

    if (solution && intersecting)
        solution->append(QPair<float, float>(t, s));

    return intersecting;
}


bool checkTriangleOverlap(TrianglePoints &triangle1, TrianglePoints &triangle2, float epsilon = 1.0/32)
{
    // See if there is an edge of triangle1 such that all vertices in triangle2 are on the opposite side
    fixWinding(triangle1);
    for (int i = 0; i < 3; i++) {
        int ni = (i + 1) % 3;
        if (checkEdge(triangle1[i], triangle1[ni], triangle2[0], epsilon) &&
            checkEdge(triangle1[i], triangle1[ni], triangle2[1], epsilon) &&
            checkEdge(triangle1[i], triangle1[ni], triangle2[2], epsilon))
            return false;
    }

    // See if there is an edge of triangle2 such that all vertices in triangle1 are on the opposite side
    fixWinding(triangle2);
    for (int i = 0; i < 3; i++) {
        int ni = (i + 1) % 3;

        if (checkEdge(triangle2[i], triangle2[ni], triangle1[0], epsilon) &&
            checkEdge(triangle2[i], triangle2[ni], triangle1[1], epsilon) &&
            checkEdge(triangle2[i], triangle2[ni], triangle1[2], epsilon))
            return false;
    }

    return true;
}

bool checkLineTriangleOverlap(TrianglePoints &triangle, LinePoints &line, float epsilon = 1.0/32)
{
    // See if all vertices of the triangle are on the same side of the line
    bool s1 = determinant(line[0], line[1], triangle[0]) < 0;
    bool s2 = determinant(line[0], line[1], triangle[1]) < 0;
    bool s3 = determinant(line[0], line[1], triangle[2]) < 0;
    // If all determinants have the same sign, then there is no overlap
    if (s1 == s2 && s2 == s3) {
        return false;
    }
    // See if there is an edge of triangle1 such that both vertices in line are on the opposite side
    fixWinding(triangle);
    for (int i = 0; i < 3; i++) {
        int ni = (i + 1) % 3;
        if (checkEdge(triangle[i], triangle[ni], line[0], epsilon) &&
            checkEdge(triangle[i], triangle[ni], line[1], epsilon))
            return false;
    }

    return true;
}

static bool isOverlap(const QQuadPath &path, int e1, int e2)
{
    const QQuadPath::Element &element1 = path.elementAt(e1);
    const QQuadPath::Element &element2 = path.elementAt(e2);

    if (element1.isLine()) {
        LinePoints line1{ element1.startPoint(), element1.endPoint() };
        if (element2.isLine()) {
            LinePoints line2{ element2.startPoint(), element2.endPoint() };
            return lineIntersection(line1, line2);
        } else {
            TrianglePoints t2{ element2.startPoint(), element2.controlPoint(), element2.endPoint() };
            return checkLineTriangleOverlap(t2, line1);
        }
    } else {
        TrianglePoints t1{ element1.startPoint(), element1.controlPoint(), element1.endPoint() };
        if (element2.isLine()) {
            LinePoints line{ element2.startPoint(), element2.endPoint() };
            return checkLineTriangleOverlap(t1, line);
        } else {
            TrianglePoints t2{ element2.startPoint(), element2.controlPoint(), element2.endPoint() };
            return checkTriangleOverlap(t1, t2);
        }
    }

    return false;
}

static float angleBetween(const QVector2D v1, const QVector2D v2)
{
    float dot = v1.x() * v2.x() + v1.y() * v2.y();
    float cross = v1.x() * v2.y() - v1.y() * v2.x();
    //TODO: Optimization: Maybe we don't need the atan2 here.
    return atan2(cross, dot);
}

static bool isIntersecting(const TrianglePoints &t1, const TrianglePoints &t2, QList<QPair<float, float>> *solutions = nullptr)
{
    constexpr double eps = 1e-5; // Epsilon for coordinate space x-y
    constexpr double eps2 = 1e-5; // Epsilon for parameter space t1-t2
    constexpr int maxIterations = 7; // Maximum iterations allowed for Newton

    // Convert to double to get better accuracy.
    std::array<QPointF, 3> td1 = { t1[0].toPointF(), t1[1].toPointF(), t1[2].toPointF() };
    std::array<QPointF, 3> td2 = { t2[0].toPointF(), t2[1].toPointF(), t2[2].toPointF() };

    // F = P1(t1) - P2(t2) where P1 and P2 are bezier curve functions.
    // F = (0, 0) at the intersection.
    // t is the vector of bezier curve parameters for curves P1 and P2
    auto F = [=](QPointF t) { return
                td1[0] * (1 - t.x()) * (1. - t.x()) + 2 * td1[1] * (1. - t.x()) * t.x() + td1[2] * t.x() * t.x() -
                td2[0] * (1 - t.y()) * (1. - t.y()) - 2 * td2[1] * (1. - t.y()) * t.y() - td2[2] * t.y() * t.y();};

    // J is the Jacobi Matrix dF/dt where F and t are both vectors of dimension 2.
    // Storing in a QLineF for simplicity.
    auto J = [=](QPointF t) { return QLineF(
                td1[0].x() * (-2 * (1-t.x())) + 2 * td1[1].x() * (1 - 2 * t.x()) + td1[2].x() * 2 * t.x(),
               -td2[0].x() * (-2 * (1-t.y())) - 2 * td2[1].x() * (1 - 2 * t.y()) - td2[2].x() * 2 * t.y(),
                td1[0].y() * (-2 * (1-t.x())) + 2 * td1[1].y() * (1 - 2 * t.x()) + td1[2].y() * 2 * t.x(),
               -td2[0].y() * (-2 * (1-t.y())) - 2 * td2[1].y() * (1 - 2 * t.y()) - td2[2].y() * 2 * t.y());};

    // solve the equation A(as 2x2 matrix)*x = b. Returns x.
    auto solve = [](QLineF A, QPointF b) {
        // invert A
        const double det = A.x1() * A.y2() - A.y1() * A.x2();
        QLineF Ainv(A.y2() / det, -A.y1() / det, -A.x2() / det, A.x1() / det);
        // return A^-1 * b
        return QPointF(Ainv.x1() * b.x() + Ainv.y1() * b.y(),
                       Ainv.x2() * b.x() + Ainv.y2() * b.y());
    };

#ifdef INTERSECTION_EXTRA_DEBUG
    qCDebug(lcSGCurveIntersectionSolver) << "Checking" << t1[0] << t1[1] << t1[2];
    qCDebug(lcSGCurveIntersectionSolver) << "      vs" << t2[0] << t2[1] << t2[2];
#endif

    // TODO: Try to figure out reasonable starting points to reach all 4 possible intersections.
    // This works but is kinda brute forcing it.
    constexpr std::array tref = { QPointF{0.0, 0.0}, QPointF{0.5, 0.0}, QPointF{1.0, 0.0},
                                  QPointF{0.0, 0.5}, QPointF{0.5, 0.5}, QPointF{1.0, 0.5},
                                  QPointF{0.0, 1.0}, QPointF{0.5, 1.0}, QPointF{1.0, 1.0} };

    for (auto t : tref) {
        double err = 1;
        QPointF fval = F(t);
        int i = 0;

        // TODO: Try to abort sooner, e.g. when falling out of the interval [0-1]?
        while (err > eps && i < maxIterations) { // && t.x() >= 0 && t.x() <= 1 && t.y() >= 0 && t.y() <= 1) {
            t = t - solve(J(t), fval);
            fval = F(t);
            err = qAbs(fval.x()) + qAbs(fval.y()); // Using the Manhatten length as an error indicator.
            i++;
#ifdef INTERSECTION_EXTRA_DEBUG
            qCDebug(lcSGCurveIntersectionSolver) << "         Newton iteration" << i << "t =" << t << "F =" << fval << "Error =" << err;
#endif
        }
        // Intersections at 0 count. Intersections at 1 do not.
        if (err < eps && t.x() >=0 && t.x() <= 1. - 10 * eps2 && t.y() >= 0 && t.y() <= 1. - 10 * eps2) {
#ifdef INTERSECTION_EXTRA_DEBUG
             qCDebug(lcSGCurveIntersectionSolver) << "         Newton solution (after" << i << ")=" << t << "(" << F(t) << ")";
#endif
            if (solutions) {
                bool append = true;
                for (auto solution : *solutions) {
                    if (qAbs(solution.first - t.x()) < 10 * eps2 && qAbs(solution.second - t.y()) < 10 * eps2) {
                        append = false;
                        break;
                    }
                }
                if (append)
                    solutions->append({t.x(), t.y()});
            }
            else
                return true;
        }
    }
    if (solutions)
        return solutions->size() > 0;
    else
        return false;
}

static bool isIntersecting(const QQuadPath &path, int e1, int e2, QList<QPair<float, float>> *solutions = nullptr)
{

    const QQuadPath::Element &elem1 = path.elementAt(e1);
    const QQuadPath::Element &elem2 = path.elementAt(e2);

    if (elem1.isLine() && elem2.isLine()) {
        return lineIntersection(LinePoints {elem1.startPoint(), elem1.endPoint() },
                                LinePoints {elem2.startPoint(), elem2.endPoint() },
                                solutions);
    } else {
        return isIntersecting(TrianglePoints { elem1.startPoint(), elem1.controlPoint(), elem1.endPoint() },
                              TrianglePoints { elem2.startPoint(), elem2.controlPoint(), elem2.endPoint() },
                              solutions);
    }
}

struct TriangleData
{
    TrianglePoints points;
    int pathElementIndex;
    TrianglePoints normals;
};

// Returns a normalized vector that is perpendicular to baseLine, pointing to the right
inline QVector2D normalVector(QVector2D baseLine)
{
    QVector2D normal = QVector2D(-baseLine.y(), baseLine.x()).normalized();
    return normal;
}

// Returns a vector that is normal to the path and pointing to the right. If endSide is false
// the vector is normal to the start point, otherwise to the end point
QVector2D normalVector(const QQuadPath::Element &element, bool endSide = false)
{
    if (element.isLine())
        return normalVector(element.endPoint() - element.startPoint());
    else if (!endSide)
        return normalVector(element.controlPoint() - element.startPoint());
    else
        return normalVector(element.endPoint() - element.controlPoint());
}

// Returns a vector that is parallel to the path. If endSide is false
// the vector starts at the start point and points forward,
// otherwise it starts at the end point and points backward
QVector2D tangentVector(const QQuadPath::Element &element, bool endSide = false)
{
    if (element.isLine()) {
        if (!endSide)
            return element.endPoint() - element.startPoint();
        else
            return element.startPoint() - element.endPoint();
    } else {
        if (!endSide)
            return element.controlPoint() - element.startPoint();
        else
            return element.controlPoint() - element.endPoint();
    }
}

// Really simplistic O(n^2) triangulator - only intended for five points
QList<TriangleData> simplePointTriangulator(const QList<QVector2D> &pts, const QList<QVector2D> &normals, int elementIndex)
{
    int count = pts.size();
    Q_ASSERT(count >= 3);
    Q_ASSERT(normals.size() == count);

    // First we find the convex hull: it's always in positive determinant winding order
    QList<int> hull;
    float det1 = determinant(pts[0], pts[1], pts[2]);
    if (det1 > 0)
        hull << 0 << 1 << 2;
    else
        hull << 2 << 1 << 0;
    auto connectableInHull = [&](int idx) -> QList<int> {
        QList<int> r;
        const int n = hull.size();
        const auto &pt = pts[idx];
        for (int i = 0; i < n; ++i) {
            const auto &i1 = hull.at(i);
            const auto &i2 = hull.at((i+1) % n);
            if (determinant(pts[i1], pts[i2], pt) < 0.0f)
                r << i;
        }
        return r;
    };
    for (int i = 3; i < count; ++i) {
        auto visible = connectableInHull(i);
        if (visible.isEmpty())
            continue;
        int visCount = visible.count();
        int hullCount = hull.count();
        // Find where the visible part of the hull starts. (This is the part we need to triangulate to,
        // and the part we're going to replace. "visible" contains the start point of the line segments that are visible from p.
        int boundaryStart = visible[0];
        for (int j = 0; j < visCount - 1; ++j) {
            if ((visible[j] + 1) % hullCount != visible[j+1]) {
                boundaryStart = visible[j + 1];
                break;
            }
        }
        // Finally replace the points that are now inside the hull
        // We insert the new point after boundaryStart, and before boundaryStart + visCount (modulo...)
        // and remove the points in between
        int pointsToKeep = hullCount - visCount + 1;
        QList<int> newHull;
        newHull << i;
        for (int j = 0; j < pointsToKeep; ++j) {
            newHull << hull.at((j + boundaryStart + visCount) % hullCount);
        }
        hull = newHull;
    }

    // Now that we have a convex hull, we can trivially triangulate it
    QList<TriangleData> ret;
    for (int i = 1; i < hull.size() - 1; ++i) {
        int i0 = hull[0];
        int i1 = hull[i];
        int i2 = hull[i+1];
        ret.append({{pts[i0], pts[i1], pts[i2]}, elementIndex, {normals[i0], normals[i1], normals[i2]}});
    }
    return ret;
}


inline bool needsSplit(const QQuadPath::Element &el)
{
    Q_ASSERT(!el.isLine());
    const auto v1 = el.controlPoint() - el.startPoint();
    const auto v2 = el.endPoint() - el.controlPoint();
    float cos = QVector2D::dotProduct(v1, v2) / (v1.length() * v2.length());
    return cos < 0.9;
}


inline void splitElementIfNecessary(QQuadPath *path, int index, int level) {
    if (level > 0 && needsSplit(path->elementAt(index))) {
        path->splitElementAt(index);
        splitElementIfNecessary(path, path->indexOfChildAt(index, 0), level - 1);
        splitElementIfNecessary(path, path->indexOfChildAt(index, 1), level - 1);
    }
}

static QQuadPath subdivide(const QQuadPath &path, int subdivisions)
{
    QQuadPath newPath = path;
    newPath.iterateElements([&](QQuadPath::Element &e, int index) {
        if (!e.isLine())
            splitElementIfNecessary(&newPath, index, subdivisions);
    });

    return newPath;
}

static QList<TriangleData> customTriangulator2(const QQuadPath &path, float penWidth, Qt::PenJoinStyle joinStyle, Qt::PenCapStyle capStyle, float miterLimit)
{
    const bool bevelJoin = joinStyle == Qt::BevelJoin;
    const bool roundJoin = joinStyle == Qt::RoundJoin;
    const bool miterJoin = !bevelJoin && !roundJoin;

    const bool roundCap = capStyle == Qt::RoundCap;
    const bool squareCap = capStyle == Qt::SquareCap;
    // We can't use the simple miter for miter joins, since the shader currently only supports round joins
    const bool simpleMiter = joinStyle == Qt::RoundJoin;

    Q_ASSERT(miterLimit > 0 || !miterJoin);
    float inverseMiterLimit = miterJoin ? 1.0f / miterLimit : 1.0;

    const float penFactor = penWidth / 2;

    // Returns {inner1, inner2, outer1, outer2, outerMiter}
    // where foo1 is for the end of element1 and foo2 is for the start of element2
    // and inner1 == inner2 unless we had to give up finding a decent point
    auto calculateJoin = [&](const QQuadPath::Element *element1, const QQuadPath::Element *element2,
                             bool &outerBisectorWithinMiterLimit, bool &innerIsRight, bool &giveUp) -> std::array<QVector2D, 5>
    {
        outerBisectorWithinMiterLimit = true;
        innerIsRight = true;
        giveUp = false;
        if (!element1) {
            Q_ASSERT(element2);
            QVector2D n = normalVector(*element2);
            return {n, n, -n, -n, -n};
        }
        if (!element2) {
            Q_ASSERT(element1);
            QVector2D n = normalVector(*element1, true);
            return {n, n, -n, -n, -n};
        }

        Q_ASSERT(element1->endPoint() == element2->startPoint());

        const auto p1 = element1->isLine() ? element1->startPoint() : element1->controlPoint();
        const auto p2 = element1->endPoint();
        const auto p3 = element2->isLine() ? element2->endPoint() : element2->controlPoint();

        const auto v1 = (p1 - p2).normalized();
        const auto v2 = (p3 - p2).normalized();
        const auto b = (v1 + v2);

        constexpr float epsilon = 1.0f / 32.0f;
        bool smoothJoin = qAbs(b.x()) < epsilon && qAbs(b.y()) < epsilon;

        if (smoothJoin) {
            // v1 and v2 are almost parallel and pointing in opposite directions
            // angle bisector formula will give an almost null vector: use normal of bisector of normals instead
            QVector2D n1(-v1.y(), v1.x());
            QVector2D n2(-v2.y(), v2.x());
            QVector2D n = (n2 - n1).normalized();
            return {n, n, -n, -n, -n};
        }
        // Calculate the length of the bisector, so it will cover the entire miter.
        // Using the identity sin(x/2) == sqrt((1 - cos(x)) / 2), and the fact that the
        // dot product of two unit vectors is the cosine of the angle between them
        // The length of the miter is w/sin(x/2) where x is the angle between the two elements

        const auto bisector = b.normalized();
        float cos2x = QVector2D::dotProduct(v1, v2);
        cos2x = qMin(1.0f, cos2x); // Allow for float inaccuracy
        float sine = sqrt((1.0f - cos2x) / 2);
        innerIsRight = determinant(p1, p2, p3) > 0;
        sine = qMax(sine, 0.01f); // Avoid divide by zero
        float length = penFactor / sine;

        // Check if bisector is longer than one of the lines it's trying to bisect

        auto tooLong = [](QVector2D p1, QVector2D p2, QVector2D n, float length, float margin) -> bool {
            auto v = p2 - p1;
            // It's too long if the projection onto the bisector is longer than the bisector
            // and the projection onto the normal to the bisector is shorter
            // than the pen margin (that projection is just v - proj)
            // (we're adding a 10% safety margin to make room for AA -- not exact)
            auto projLen = QVector2D::dotProduct(v, n);
            return projLen * 0.9f < length && (v - n * projLen).length() * 0.9 < margin;
        };


        // The angle bisector of the tangent lines is not correct for curved lines. We could fix this by calculating
        // the exact intersection point, but for now just give up and use the normals.

        giveUp = !element1->isLine() || !element2->isLine()
                 || tooLong(p1, p2, bisector, length, penFactor)
                 || tooLong(p3, p2, bisector, length, penFactor);
        outerBisectorWithinMiterLimit = sine >= inverseMiterLimit / 2.0f;
        bool simpleJoin = simpleMiter && outerBisectorWithinMiterLimit && !giveUp;
        const QVector2D bn = bisector / sine;

        if (simpleJoin)
            return {bn, bn, -bn, -bn, -bn}; // We only have one inner and one outer point TODO: change inner point when conflict/curve
        const QVector2D n1 = normalVector(*element1, true);
        const QVector2D n2 = normalVector(*element2);
        if (giveUp) {
            if (innerIsRight)
                return {n1, n2, -n1, -n2, -bn};
            else
                return {-n1, -n2, n1, n2, -bn};

        } else {
            if (innerIsRight)
                return {bn, bn, -n1, -n2, -bn};
            else
                return {bn, bn, n1, n2, -bn};
        }
    };

    QList<TriangleData> ret;

    auto triangulateCurve = [&](int idx, const QVector2D &p1, const QVector2D &p2, const QVector2D &p3, const QVector2D &p4,
                                const QVector2D &n1, const QVector2D &n2, const QVector2D &n3, const QVector2D &n4)
    {
        const auto &element = path.elementAt(idx);
        Q_ASSERT(!element.isLine());
        const auto &s = element.startPoint();
        const auto &c = element.controlPoint();
        const auto &e = element.endPoint();
        // TODO: Don't flatten the path in addCurveStrokeNodes, but iterate over the children here instead
        bool controlPointOnRight = determinant(s, c, e) > 0;
        QVector2D startNormal = normalVector(element);
        QVector2D endNormal = normalVector(element, true);
        QVector2D controlPointNormal = (startNormal + endNormal).normalized();
        if (controlPointOnRight)
            controlPointNormal = -controlPointNormal;
        QVector2D p5 = c + controlPointNormal * penFactor; // This is too simplistic
        TrianglePoints t1{p1, p2, p5};
        TrianglePoints t2{p3, p4, p5};
        bool simpleCase = !checkTriangleOverlap(t1, t2);

        if (simpleCase) {
            ret.append({{p1, p2, p5}, idx, {n1, n2, controlPointNormal}});
            ret.append({{p3, p4, p5}, idx, {n3, n4, controlPointNormal}});
            if (controlPointOnRight) {
                ret.append({{p1, p3, p5}, idx, {n1, n3, controlPointNormal}});
            } else {
                ret.append({{p2, p4, p5}, idx, {n2, n4, controlPointNormal}});
            }
        } else {
            ret.append(simplePointTriangulator({p1, p2, p5, p3, p4}, {n1, n2, controlPointNormal, n3, n4}, idx));
        }
    };

    // Each element is calculated independently, so we don't have to special-case closed sub-paths.
    // Take care so the end points of one element are precisely equal to the start points of the next.
    // Any additional triangles needed for joining are added at the end of the current element.

    int count = path.elementCount();
    int subStart = 0;
    while (subStart < count) {
        int subEnd = subStart;
        for (int i = subStart + 1; i < count; ++i) {
            const auto &e = path.elementAt(i);
            if (e.isSubpathStart()) {
                subEnd = i - 1;
                break;
            }
            if (i == count - 1) {
                subEnd = i;
                break;
            }
        }
        bool closed = path.elementAt(subStart).startPoint() == path.elementAt(subEnd).endPoint();
        const int subCount = subEnd - subStart + 1;

        auto addIdx = [&](int idx, int delta) -> int {
            int subIdx = idx - subStart;
            if (closed)
                subIdx = (subIdx + subCount + delta) % subCount;
            else
                subIdx += delta;
            return subStart + subIdx;
        };
        auto elementAt = [&](int idx, int delta) -> const QQuadPath::Element * {
            int subIdx = idx - subStart;
            if (closed) {
                subIdx = (subIdx + subCount + delta) % subCount;
                return &path.elementAt(subStart + subIdx);
            }
            subIdx += delta;
            if (subIdx >= 0 && subIdx < subCount)
                return &path.elementAt(subStart + subIdx);
            return nullptr;
        };

        for (int i = subStart; i <= subEnd; ++i) {
            const auto &element = path.elementAt(i);
            const auto *nextElement = elementAt(i, +1);
            const auto *prevElement = elementAt(i, -1);

            const auto &s = element.startPoint();
            const auto &e = element.endPoint();

            bool startInnerIsRight;
            bool startBisectorWithinMiterLimit; // Not used
            bool giveUpOnStartJoin; // Not used
            auto startJoin = calculateJoin(prevElement, &element,
                                           startBisectorWithinMiterLimit, startInnerIsRight,
                                           giveUpOnStartJoin);
            const QVector2D &startInner = startJoin[1];
            const QVector2D &startOuter = startJoin[3];

            bool endInnerIsRight;
            bool endBisectorWithinMiterLimit;
            bool giveUpOnEndJoin;
            auto endJoin = calculateJoin(&element, nextElement,
                                         endBisectorWithinMiterLimit, endInnerIsRight,
                                         giveUpOnEndJoin);
            QVector2D endInner = endJoin[0];
            QVector2D endOuter = endJoin[2];
            QVector2D nextOuter = endJoin[3];
            QVector2D outerB = endJoin[4];

            QVector2D p1, p2, p3, p4;
            QVector2D n1, n2, n3, n4;

            if (startInnerIsRight) {
                n1 = startInner;
                n2 = startOuter;
            } else {
                n1 = startOuter;
                n2 = startInner;
            }

            p1 = s + n1 * penFactor;
            p2 = s + n2 * penFactor;

            // repeat logic above for the other end:
            if (endInnerIsRight) {
                n3 = endInner;
                n4 = endOuter;
            } else {
                n3 = endOuter;
                n4 = endInner;
            }

            p3 = e + n3 * penFactor;
            p4 = e + n4 * penFactor;

            // End caps

            if (!prevElement) {
                QVector2D capSpace = tangentVector(element).normalized() * -penFactor;
                if (roundCap) {
                    p1 += capSpace;
                    p2 += capSpace;
                } else if (squareCap) {
                    QVector2D c1 = p1 + capSpace;
                    QVector2D c2 = p2 + capSpace;
                    ret.append({{p1, s, c1}, -1, {}});
                    ret.append({{c1, s, c2}, -1, {}});
                    ret.append({{p2, s, c2}, -1, {}});
                }
            }
            if (!nextElement) {
                QVector2D capSpace = tangentVector(element, true).normalized() * -penFactor;
                if (roundCap) {
                    p3 += capSpace;
                    p4 += capSpace;
                } else if (squareCap) {
                    QVector2D c3 = p3 + capSpace;
                    QVector2D c4 = p4 + capSpace;
                    ret.append({{p3, e, c3}, -1, {}});
                    ret.append({{c3, e, c4}, -1, {}});
                    ret.append({{p4, e, c4}, -1, {}});
                }
            }

            if (element.isLine()) {
                ret.append({{p1, p2, p3}, i, {n1, n2, n3}});
                ret.append({{p2, p3, p4}, i, {n2, n3, n4}});
            } else {
                triangulateCurve(i, p1, p2, p3, p4, n1, n2, n3, n4);
            }

            bool trivialJoin = simpleMiter && endBisectorWithinMiterLimit && !giveUpOnEndJoin;
            if (!trivialJoin && nextElement) {
                // inside of join (opposite of bevel) is defined by
                // triangle s, e, next.e
                bool innerOnRight = endInnerIsRight;

                const auto outer1 = e + endOuter * penFactor;
                const auto outer2 = e + nextOuter * penFactor;
                //const auto inner = e + endInner * penFactor;

                if (bevelJoin || (miterJoin && !endBisectorWithinMiterLimit)) {
                    ret.append({{outer1, e, outer2}, -1, {}});
                } else if (roundJoin) {
                    ret.append({{outer1, e, outer2}, i, {}});
                    QVector2D nn = calcNormalVector(outer1, outer2).normalized() * penFactor;
                    if (!innerOnRight)
                        nn = -nn;
                    ret.append({{outer1, outer1 + nn, outer2}, i, {}});
                    ret.append({{outer1 + nn, outer2, outer2 + nn}, i, {}});

                } else if (miterJoin) {
                    QVector2D outer = e + outerB * penFactor;
                    ret.append({{outer1, e, outer}, -2, {}});
                    ret.append({{outer, e, outer2}, -2, {}});
                }

                if (!giveUpOnEndJoin) {
                    QVector2D inner = e + endInner * penFactor;
                    ret.append({{inner, e, outer1}, i, {endInner, {}, endOuter}});
                    // The remaining triangle ought to be done by nextElement, but we don't have start join logic there (yet)
                    int nextIdx = addIdx(i, +1);
                    ret.append({{inner, e, outer2}, nextIdx, {endInner, {}, nextOuter}});
                }
            }
        }
        subStart = subEnd + 1;
    }
    return ret;
}

// TODO: we could optimize by preprocessing e1, since we call this function multiple times on the same
// elements
// Returns true if a change was made
static bool handleOverlap(QQuadPath &path, int e1, int e2, int recursionLevel = 0)
{
    // Splitting lines is not going to help with overlap, since we assume that lines don't intersect
    if (path.elementAt(e1).isLine() && path.elementAt(e1).isLine())
        return false;

    if (!isOverlap(path, e1, e2)) {
        return false;
    }

    if (recursionLevel > 8) {
        qCDebug(lcSGCurveProcessor) << "Triangle overlap: recursion level" << recursionLevel << "aborting!";
        return false;
    }

    if (path.elementAt(e1).childCount() > 1) {
        auto e11 = path.indexOfChildAt(e1, 0);
        auto e12 = path.indexOfChildAt(e1, 1);
        handleOverlap(path, e11, e2, recursionLevel + 1);
        handleOverlap(path, e12, e2, recursionLevel + 1);
    } else if (path.elementAt(e2).childCount() > 1) {
        auto e21 = path.indexOfChildAt(e2, 0);
        auto e22 = path.indexOfChildAt(e2, 1);
        handleOverlap(path, e1, e21, recursionLevel + 1);
        handleOverlap(path, e1, e22, recursionLevel + 1);
    } else {
        path.splitElementAt(e1);
        auto e11 = path.indexOfChildAt(e1, 0);
        auto e12 = path.indexOfChildAt(e1, 1);
        bool overlap1 = isOverlap(path, e11, e2);
        bool overlap2 = isOverlap(path, e12, e2);
        if (!overlap1 && !overlap2)
            return true; // no more overlap: success!

        // We need to split more:
        if (path.elementAt(e2).isLine()) {
            // Splitting a line won't help, so we just split e1 further
            if (overlap1)
                handleOverlap(path, e11, e2, recursionLevel + 1);
            if (overlap2)
                handleOverlap(path, e12, e2, recursionLevel + 1);
        } else {
            // See if splitting e2 works:
            path.splitElementAt(e2);
            auto e21 = path.indexOfChildAt(e2, 0);
            auto e22 = path.indexOfChildAt(e2, 1);
            if (overlap1) {
                handleOverlap(path, e11, e21, recursionLevel + 1);
                handleOverlap(path, e11, e22, recursionLevel + 1);
            }
            if (overlap2) {
                handleOverlap(path, e12, e21, recursionLevel + 1);
                handleOverlap(path, e12, e22, recursionLevel + 1);
            }
        }
    }
    return true;
}
}

// Returns true if the path was changed
bool QSGCurveProcessor::solveOverlaps(QQuadPath &path)
{
    bool changed = false;
    if (path.testHint(QQuadPath::PathNonOverlappingControlPointTriangles))
        return false;

    const auto candidates = findOverlappingCandidates(path);
    for (auto candidate : candidates)
        changed = handleOverlap(path, candidate.first, candidate.second) || changed;

    path.setHint(QQuadPath::PathNonOverlappingControlPointTriangles);
    return changed;
}

// A fast algorithm to find path elements that might overlap. We will only check the overlap of the
// triangles that define the elements.
// We will order the elements first and then pool them depending on their x-values. This should
// reduce the complexity to O(n log n), where n is the number of elements in the path.
QList<QPair<int, int>> QSGCurveProcessor::findOverlappingCandidates(const QQuadPath &path)
{
    struct BRect { float xmin; float xmax; float ymin; float ymax; };

    // Calculate all bounding rectangles
    QVarLengthArray<int, 64> elementStarts, elementEnds;
    QVarLengthArray<BRect, 64> boundingRects;
    elementStarts.reserve(path.elementCount());
    boundingRects.reserve(path.elementCount());
    for (int i = 0; i < path.elementCount(); i++) {
        QQuadPath::Element e = path.elementAt(i);

        BRect bR{qMin(qMin(e.startPoint().x(), e.controlPoint().x()), e.endPoint().x()),
                 qMax(qMax(e.startPoint().x(), e.controlPoint().x()), e.endPoint().x()),
                 qMin(qMin(e.startPoint().y(), e.controlPoint().y()), e.endPoint().y()),
                 qMax(qMax(e.startPoint().y(), e.controlPoint().y()), e.endPoint().y())};
        boundingRects.append(bR);
        elementStarts.append(i);
    }

    // Sort the bounding rectangles by x-startpoint and x-endpoint
    auto compareXmin = [&](int i, int j){return boundingRects.at(i).xmin < boundingRects.at(j).xmin;};
    auto compareXmax = [&](int i, int j){return boundingRects.at(i).xmax < boundingRects.at(j).xmax;};
    std::sort(elementStarts.begin(), elementStarts.end(), compareXmin);
    elementEnds = elementStarts;
    std::sort(elementEnds.begin(), elementEnds.end(), compareXmax);

    QList<int> bRpool;
    QList<QPair<int, int>> overlappingBB;

    // Start from x = xmin and move towards xmax. Add a rectangle to the pool and check for
    // intersections with all other rectangles in the pool. If a rectangles xmax is smaller
    // than the new xmin, it can be removed from the pool.
    int firstElementEnd = 0;
    for (const int addIndex : std::as_const(elementStarts)) {
        const BRect &newR = boundingRects.at(addIndex);
        // First remove elements from the pool that cannot touch the new one
        // because xmax is too small
        while (bRpool.size() && firstElementEnd < elementEnds.size()) {
            int removeIndex = elementEnds.at(firstElementEnd);
            if (bRpool.contains(removeIndex) && newR.xmin > boundingRects.at(removeIndex).xmax) {
                bRpool.removeOne(removeIndex);
                firstElementEnd++;
            } else {
                break;
            }
        }

        // Now compare the new element with all elements in the pool.
        for (int j = 0; j < bRpool.size(); j++) {
            int i = bRpool.at(j);
            const BRect &r1 = boundingRects.at(i);
            // We don't have to check for x because the pooling takes care of it.
            //if (r1.xmax <= newR.xmin || newR.xmax <= r1.xmin)
            //    continue;

            bool isNeighbor = false;
            if (i - addIndex == 1) {
                if (!path.elementAt(addIndex).isSubpathEnd())
                    isNeighbor = true;
            } else if (addIndex - i == 1) {
                if (!path.elementAt(i).isSubpathEnd())
                    isNeighbor = true;
            }
            // Neighbors need to be completely different (otherwise they just share a point)
            if (isNeighbor && (r1.ymax <= newR.ymin || newR.ymax <= r1.ymin))
                continue;
            // Non-neighbors can also just touch
            if (!isNeighbor && (r1.ymax < newR.ymin || newR.ymax < r1.ymin))
                continue;
            // If the bounding boxes are overlapping it is a candidate for an intersection.
            overlappingBB.append(QPair<int, int>(i, addIndex));
        }
        bRpool.append(addIndex); //Add the new element to the pool.
    }
    return overlappingBB;
}

// Remove paths that are nested inside another path and not required to fill the path correctly
bool QSGCurveProcessor::removeNestedSubpaths(QQuadPath &path)
{
    // Ensure that the path is not intersecting first
    Q_ASSERT(path.testHint(QQuadPath::PathNonIntersecting));

    if (path.fillRule() != Qt::WindingFill) {
        // If the fillingRule is odd-even, all internal subpaths matter
        return false;
    }

    // Store the starting and end elements of the subpaths to be able
    // to jump quickly between them.
    QList<int> subPathStartPoints;
    QList<int> subPathEndPoints;
    for (int i = 0; i < path.elementCount(); i++) {
        if (path.elementAt(i).isSubpathStart())
            subPathStartPoints.append(i);
        if (path.elementAt(i).isSubpathEnd()) {
            subPathEndPoints.append(i);
        }
    }
    const int subPathCount = subPathStartPoints.size();

    // If there is only one subpath, none have to be removed
    if (subPathStartPoints.size() < 2)
        return false;

    // We set up a matrix that tells us which path is nested in which other path.
    QList<bool> isInside;
    bool isAnyInside = false;
    isInside.reserve(subPathStartPoints.size() * subPathStartPoints.size());
    for (int i = 0; i < subPathCount; i++) {
        for (int j = 0; j < subPathCount; j++) {
            if (i == j) {
                isInside.append(false);
            } else {
                isInside.append(path.contains(path.elementAt(subPathStartPoints.at(i)).startPoint(),
                                              subPathStartPoints.at(j), subPathEndPoints.at(j)));
                if (isInside.last())
                    isAnyInside = true;
            }
        }
    }

    // If no nested subpaths are present we can return early.
    if (!isAnyInside)
        return false;

    // To find out which paths are filled and which not, we first calculate the
    // rotation direction (clockwise - counterclockwise).
    QList<bool> clockwise;
    clockwise.reserve(subPathCount);
    for (int i = 0; i < subPathCount; i++) {
        float sumProduct = 0;
        for (int j = subPathStartPoints.at(i); j <= subPathEndPoints.at(i); j++) {
            const QVector2D startPoint = path.elementAt(j).startPoint();
            const QVector2D endPoint = path.elementAt(j).endPoint();
            sumProduct += (endPoint.x() - startPoint.x()) * (endPoint.y() + startPoint.y());
        }
        clockwise.append(sumProduct > 0);
    }

    // Set up a list that tells us which paths create filling and which path create holes.
    // Holes in Holes and fillings in fillings can then be removed.
    QList<bool> isFilled;
    isFilled.reserve(subPathStartPoints.size() );
    for (int i = 0; i < subPathCount; i++) {
        int crossings = clockwise.at(i) ? 1 : -1;
        for (int j = 0; j < subPathStartPoints.size(); j++) {
            if (isInside.at(i * subPathCount + j))
                crossings += clockwise.at(j) ? 1 : -1;
        }
        isFilled.append(crossings != 0);
    }

    // A helper function to find the most inner subpath that is around a subpath.
    // Returns -1 if the subpath is a toplevel subpath.
    auto findClosestOuterSubpath = [&](int subPath) {
        // All paths that contain the current subPath are candidates.
        QList<int> candidates;
        for (int i = 0; i < subPathStartPoints.size(); i++) {
            if (isInside.at(subPath * subPathCount + i))
                candidates.append(i);
        }
        int maxNestingLevel = -1;
        int maxNestingLevelIndex = -1;
        for (int i = 0; i < candidates.size(); i++) {
            int nestingLevel = 0;
            for (int j = 0; j < candidates.size(); j++) {
                if (isInside.at(candidates.at(i) * subPathCount + candidates.at(j))) {
                    nestingLevel++;
                }
            }
            if (nestingLevel > maxNestingLevel) {
                maxNestingLevel = nestingLevel;
                maxNestingLevelIndex = candidates.at(i);
            }
        }
        return maxNestingLevelIndex;
    };

    bool pathChanged = false;
    QQuadPath fixedPath;
    fixedPath.setPathHints(path.pathHints());

    // Go through all subpaths and find the closest surrounding subpath.
    // If it is doing the same (create filling or create hole) we can remove it.
    for (int i = 0; i < subPathCount; i++) {
        int j = findClosestOuterSubpath(i);
        if (j >= 0 && isFilled.at(i) == isFilled.at(j)) {
            pathChanged = true;
        } else {
            for (int k = subPathStartPoints.at(i); k <= subPathEndPoints.at(i); k++)
                fixedPath.addElement(path.elementAt(k));
        }
    }

    if (pathChanged)
        path = fixedPath;
    return pathChanged;
}

// Returns true if the path was changed
bool QSGCurveProcessor::solveIntersections(QQuadPath &path, bool removeNestedPaths)
{
    if (path.testHint(QQuadPath::PathNonIntersecting)) {
        if (removeNestedPaths)
            return removeNestedSubpaths(path);
        else
            return false;
    }

    if (path.elementCount() < 2) {
        path.setHint(QQuadPath::PathNonIntersecting);
        return false;
    }

    struct IntersectionData { int e1; int e2; float t1; float t2; bool in1 = false, in2 = false, out1 = false, out2 = false; };
    QList<IntersectionData> intersections;

    // Helper function to mark an intersection as handled when the
    // path i is processed moving forward/backward
    auto markIntersectionAsHandled = [=](IntersectionData *data, int i, bool forward) {
        if (data->e1 == i) {
            if (forward)
                data->out1 = true;
            else
                data->in1 = true;
        } else if (data->e2 == i){
            if (forward)
                data->out2 = true;
            else
                data->in2 = true;
        } else {
            Q_UNREACHABLE();
        }
    };

    // First make a O(n log n) search for candidates.
    const QList<QPair<int, int>> candidates = findOverlappingCandidates(path);
    // Then check the candidates for actual intersections.
    for (const auto &candidate : candidates) {
        QList<QPair<float,float>> res;
        int e1 = candidate.first;
        int e2 = candidate.second;
        if (isIntersecting(path, e1, e2, &res)) {
            for (const auto &r : res)
                intersections.append({e1, e2, r.first, r.second});
        }
    }

    qCDebug(lcSGCurveIntersectionSolver) << "----- Checking for Intersections -----";
    qCDebug(lcSGCurveIntersectionSolver) << "Found" << intersections.length() << "intersections";
    if (lcSGCurveIntersectionSolver().isDebugEnabled()) {
        for (const auto &i : intersections) {
            auto p1 = path.elementAt(i.e1).pointAtFraction(i.t1);
            auto p2 = path.elementAt(i.e2).pointAtFraction(i.t2);
            qCDebug(lcSGCurveIntersectionSolver) << "    between" << i.e1 << "and" << i.e2 << "at" << i.t1 << "/" << i.t2 << "->" << p1 << "/" << p2;
        }
    }

    if (intersections.isEmpty()) {
        path.setHint(QQuadPath::PathNonIntersecting);
        if (removeNestedPaths) {
            qCDebug(lcSGCurveIntersectionSolver) << "No Intersections found. Looking for enclosed subpaths.";
            return removeNestedSubpaths(path);
        } else {
            qCDebug(lcSGCurveIntersectionSolver) << "Returning the path unchanged.";
            return false;
        }
    }


    // Store the starting and end elements of the subpaths to be able
    // to jump quickly between them. Also keep a list of handled paths,
    // so we know if we need to come back to a subpath or if it
    // was already united with another subpath due to an intersection.
    QList<int> subPathStartPoints;
    QList<int> subPathEndPoints;
    QList<bool> subPathHandled;
    for (int i = 0; i < path.elementCount(); i++) {
        if (path.elementAt(i).isSubpathStart())
            subPathStartPoints.append(i);
        if (path.elementAt(i).isSubpathEnd()) {
            subPathEndPoints.append(i);
            subPathHandled.append(false);
        }
    }

    // A small helper to find the subPath of an element with index
    auto subPathIndex = [&](int index) {
        for (int i = 0; i < subPathStartPoints.size(); i++) {
            if (index >= subPathStartPoints.at(i) && index <= subPathEndPoints.at(i))
                return i;
        }
        return -1;
    };

    // Helper to ensure that index i and position t are valid:
    auto ensureInBounds = [&](int *i, float *t, float deltaT) {
        if (*t <= 0.f) {
            if (path.elementAt(*i).isSubpathStart())
                *i = subPathEndPoints.at(subPathIndex(*i));
            else
                *i = *i - 1;
            *t = 1.f - deltaT;
        } else if (*t >= 1.f) {
            if (path.elementAt(*i).isSubpathEnd())
                *i = subPathStartPoints.at(subPathIndex(*i));
            else
                *i = *i + 1;
            *t = deltaT;
        }
    };

    // Helper function to find a siutable starting point between start and end.
    // A suitable starting point is where right is inside and left is outside
    // If left is inside and right is outside it works too, just move in the
    // other direction (forward = false).
    auto findStart = [=](QQuadPath &path, int start, int end, int *result, bool *forward) {
        for (int i = start; i < end; i++) {
            int adjecent;
            if (subPathStartPoints.contains(i))
                adjecent = subPathEndPoints.at(subPathStartPoints.indexOf(i));
            else
                adjecent = i - 1;

            QQuadPath::Element::FillSide fillSide = path.fillSideOf(i, 1e-4f);
            const bool leftInside = fillSide == QQuadPath::Element::FillSideLeft;
            const bool rightInside = fillSide == QQuadPath::Element::FillSideRight;
            qCDebug(lcSGCurveIntersectionSolver) << "Element" << i << "/" << adjecent << "meeting point is left/right inside:" << leftInside << "/" << rightInside;
            if (rightInside) {
                *result = i;
                *forward = true;
                return true;
            } else if (leftInside) {
                *result = adjecent;
                *forward = false;
                return true;
            }
        }
        return false;
    };

    // Also store handledElements (handled is when we touch the start point).
    // This is used to identify and abort on errors.
    QVarLengthArray<bool> handledElements(path.elementCount(), false);
    // Only store handledElements when it is not touched due to an intersection.
    bool regularVisit = true;

    QQuadPath fixedPath;
    fixedPath.setFillRule(path.fillRule());

    int i1 = 0;
    float t1 = 0;

    int i2 = 0;
    float t2 = 0;

    float t = 0;
    bool forward = true;

    int startedAtIndex = -1;
    float startedAtT = -1;

    if (!findStart(path, 0, path.elementCount(), &i1, &forward)) {
        qCDebug(lcSGCurveIntersectionSolver) << "No suitable start found. This should not happen. Returning the path unchanged.";
        return false;
    }

    // Helper function to start a new subpath and update temporary variables.
    auto startNewSubPath = [&](int i, bool forward) {
        if (forward) {
            fixedPath.moveTo(path.elementAt(i).startPoint());
            t = startedAtT = 0;
        } else {
            fixedPath.moveTo(path.elementAt(i).endPoint());
            t = startedAtT = 1;
        }
        startedAtIndex = i;
        subPathHandled[subPathIndex(i)] = true;
    };
    startNewSubPath(i1, forward);

    // If not all interactions where found correctly, we might end up in an infinite loop.
    // Therefore we count the total number of iterations and bail out at some point.
    int totalIterations = 0;

    // We need to store the last intersection so we don't jump back and forward immediately.
    int prevIntersection = -1;

    do {
        // Sanity check: Make sure that we do not process the same corner point more than once.
        if (regularVisit && (t == 0 || t == 1)) {
            int nextIndex = i1;
            if (t == 1 && path.elementAt(i1).isSubpathEnd()) {
                nextIndex = subPathStartPoints.at(subPathIndex(i1));
            } else if (t == 1) {
                nextIndex = nextIndex + 1;
            }
            if (handledElements[nextIndex]) {
                qCDebug(lcSGCurveIntersectionSolver) << "Revisiting an element when trying to solve intersections. This should not happen. Returning the path unchanged.";
                return false;
            }
            handledElements[nextIndex] = true;
        }
        // Sanity check: Keep an eye on total iterations
        totalIterations++;

        qCDebug(lcSGCurveIntersectionSolver) << "Checking section" << i1 << "from" << t << "going" << (forward ? "forward" : "backward");

        // Find the next intersection that is as close as possible to t but in direction of processing (forward or !forward).
        int iC = -1; //intersection candidate
        t1 = forward? 1 : -1; //intersection candidate t-value
        for (int j = 0; j < intersections.size(); j++) {
            if (j == prevIntersection)
                continue;
            if (i1 == intersections[j].e1 &&
                intersections[j].t1 * (forward ? 1 : -1) >=  t * (forward ? 1 : -1) &&
                intersections[j].t1 * (forward ? 1 : -1) < t1 * (forward ? 1 : -1)) {
                iC = j;
                t1 = intersections[j].t1;
                i2 = intersections[j].e2;
                t2 = intersections[j].t2;
            }
            if (i1 == intersections[j].e2 &&
                intersections[j].t2 * (forward ? 1 : -1) >= t * (forward ? 1 : -1) &&
                intersections[j].t2 * (forward ? 1 : -1) < t1 * (forward ? 1 : -1)) {
                iC = j;
                t1 = intersections[j].t2;
                i2 = intersections[j].e1;
                t2 = intersections[j].t1;
            }
        }
        prevIntersection = iC;

        if (iC < 0) {
            qCDebug(lcSGCurveIntersectionSolver) << "    No intersection found on my way. Adding the rest of the segment " << i1;
            regularVisit = true;
            // If no intersection with the current element was found, just add the rest of the element
            // to the fixed path and go on.
            // If we reached the end (going forward) or start (going backward) of a subpath, we have
            // to wrap aroud. Abort condition for the loop comes separately later.
            if (forward) {
                if (path.elementAt(i1).isLine()) {
                    fixedPath.lineTo(path.elementAt(i1).endPoint());
                } else {
                    const QQuadPath::Element rest = path.elementAt(i1).segmentFromTo(t, 1);
                    fixedPath.quadTo(rest.controlPoint(), rest.endPoint());
                }
                if (path.elementAt(i1).isSubpathEnd()) {
                    int index = subPathEndPoints.indexOf(i1);
                    qCDebug(lcSGCurveIntersectionSolver) << "    Going back to the start of subPath" << index;
                    i1 = subPathStartPoints.at(index);
                } else {
                    i1++;
                }
                t = 0;
            } else {
                if (path.elementAt(i1).isLine()) {
                    fixedPath.lineTo(path.elementAt(i1).startPoint());
                } else {
                    const QQuadPath::Element rest = path.elementAt(i1).segmentFromTo(0, t).reversed();
                    fixedPath.quadTo(rest.controlPoint(), rest.endPoint());
                }
                if (path.elementAt(i1).isSubpathStart()) {
                    int index = subPathStartPoints.indexOf(i1);
                    qCDebug(lcSGCurveIntersectionSolver) << "    Going back to the end of subPath" << index;
                    i1 = subPathEndPoints.at(index);
                } else {
                    i1--;
                }
                t = 1;
            }
        } else { // Here comes the part where we actually handle intersections.
            qCDebug(lcSGCurveIntersectionSolver) << "    Found an intersection at" << t1 << "with" << i2 << "at" << t2;

            // Mark the subpath we intersected with as visisted. We do not have to come here explicitly again.
            subPathHandled[subPathIndex(i2)] = true;

            // Mark the path that lead us to this intersection as handled on the intersection level.
            // Note the ! in front of forward. This is required because we move towards the intersection.
            markIntersectionAsHandled(&intersections[iC], i1, !forward);

            // Split the path from the last point to the newly found intersection.
            // Add the part of the current segment to the fixedPath.
            const QQuadPath::Element &elem1 = path.elementAt(i1);
            if (elem1.isLine()) {
                fixedPath.lineTo(elem1.pointAtFraction(t1));
            } else {
                QQuadPath::Element partUntilIntersection;
                if (forward) {
                    partUntilIntersection = elem1.segmentFromTo(t, t1);
                } else {
                    partUntilIntersection = elem1.segmentFromTo(t1, t).reversed();
                }
                fixedPath.quadTo(partUntilIntersection.controlPoint(), partUntilIntersection.endPoint());
            }

            // If only one unhandled path is left the decision how to proceed is trivial
            if (intersections[iC].in1 && intersections[iC].in2 && intersections[iC].out1 && !intersections[iC].out2) {
                i1 = intersections[iC].e2;
                t = intersections[iC].t2;
                forward = true;
            } else if (intersections[iC].in1 && intersections[iC].in2 && !intersections[iC].out1 && intersections[iC].out2) {
                i1 = intersections[iC].e1;
                t = intersections[iC].t1;
                forward = true;
            } else if (intersections[iC].in1 && !intersections[iC].in2 && intersections[iC].out1 && intersections[iC].out2) {
                i1 = intersections[iC].e2;
                t = intersections[iC].t2;
                forward = false;
            } else if (!intersections[iC].in1 && intersections[iC].in2 && intersections[iC].out1 && intersections[iC].out2) {
                i1 = intersections[iC].e1;
                t = intersections[iC].t1;
                forward = false;
            } else {
                // If no trivial path is left, calculate the intersection angle to decide which path to move forward.
                // For winding fill we take the left most path forward, so the inside stays on the right side
                // For odd even fill we take the right most path forward so we cut of the smallest area.
                // We come back at the intersection and add the missing pieces as subpaths later on.
                if (t1 !=0 && t1 != 1 && t2 != 0 && t2 != 1) {
                    QVector2D tangent1 = elem1.tangentAtFraction(t1);
                    if (!forward)
                        tangent1 = -tangent1;
                    const QQuadPath::Element &elem2 = path.elementAt(i2);
                    const QVector2D tangent2 = elem2.tangentAtFraction(t2);
                    const float angle = angleBetween(-tangent1, tangent2);
                    qCDebug(lcSGCurveIntersectionSolver) << "    Angle at intersection is" << angle;
                    // A small angle. Everything smaller is interpreted as tangent
                    constexpr float deltaAngle = 1e-3f;
                    if ((angle > deltaAngle && path.fillRule() == Qt::WindingFill) || (angle < -deltaAngle && path.fillRule() == Qt::OddEvenFill)) {
                        forward = true;
                        i1 = i2;
                        t = t2;
                        qCDebug(lcSGCurveIntersectionSolver) << "    Next going forward from" << t << "on" << i1;
                    } else if ((angle < -deltaAngle && path.fillRule() == Qt::WindingFill) || (angle > deltaAngle && path.fillRule() == Qt::OddEvenFill)) {
                        forward = false;
                        i1 = i2;
                        t = t2;
                        qCDebug(lcSGCurveIntersectionSolver) << "    Next going backward from" << t << "on" << i1;
                    } else { // this is basically a tangential touch and and no crossing. So stay on the current path, keep direction
                        qCDebug(lcSGCurveIntersectionSolver) << "    Found tangent. Staying on element";
                    }
                } else {
                    // If we are intersecting exactly at a corner, the trick with the angle does not help.
                    // Therefore we have to rely on finding the next path by looking forward and see if the
                    // path there is valid. This is more expensive than the method above and is therefore
                    // just used as a fallback for corner cases.
                    constexpr float deltaT = 1e-4f;
                    int i2after = i2;
                    float t2after = t2 + deltaT;
                    ensureInBounds(&i2after, &t2after, deltaT);
                    QQuadPath::Element::FillSide fillSideForwardNew = path.fillSideOf(i2after, t2after);
                    if (fillSideForwardNew == QQuadPath::Element::FillSideRight) {
                        forward = true;
                        i1 = i2;
                        t = t2;
                        qCDebug(lcSGCurveIntersectionSolver) << "    Next going forward from" << t << "on" << i1;
                    } else {
                        int i2before = i2;
                        float t2before = t2 - deltaT;
                        ensureInBounds(&i2before, &t2before, deltaT);
                        QQuadPath::Element::FillSide fillSideBackwardNew = path.fillSideOf(i2before, t2before);
                        if (fillSideBackwardNew == QQuadPath::Element::FillSideLeft) {
                            forward = false;
                            i1 = i2;
                            t = t2;
                            qCDebug(lcSGCurveIntersectionSolver) << "    Next going backward from" << t << "on" << i1;
                        } else {
                            qCDebug(lcSGCurveIntersectionSolver) << "    Staying on element.";
                        }
                    }
                }
            }

            // Mark the path that takes us away from this intersection as handled on the intersection level.
            if (!(i1 == startedAtIndex && t == startedAtT))
                markIntersectionAsHandled(&intersections[iC], i1, forward);

            // If we took all paths from an intersection it can be deleted.
            if (intersections[iC].in1 && intersections[iC].in2 && intersections[iC].out1 && intersections[iC].out2) {
                qCDebug(lcSGCurveIntersectionSolver) << "    This intersection was processed completely and will be removed";
                intersections.removeAt(iC);
                prevIntersection = -1;
            }
            regularVisit = false;
        }

        if (i1 == startedAtIndex && t == startedAtT) {
            // We reached the point on the subpath where we started. This subpath is done.
            // We have to find an unhandled subpath or a new subpath that was generated by cuts/intersections.
            qCDebug(lcSGCurveIntersectionSolver) << "Reached my starting point and try to find a new subpath.";

            // Search for the next subpath to handle.
            int nextUnhandled = -1;
            for (int i = 0; i < subPathHandled.size(); i++) {
                if (!subPathHandled.at(i)) {

                    // Not nesesarrily handled (if findStart return false) but if we find no starting
                    // point, we cannot/don't need to handle it anyway. So just mark it as handled.
                    subPathHandled[i] = true;

                    if (findStart(path, subPathStartPoints.at(i), subPathEndPoints.at(i), &i1, &forward)) {
                        nextUnhandled = i;
                        qCDebug(lcSGCurveIntersectionSolver) << "Found a new subpath" << i << "to be processed.";
                        startNewSubPath(i1, forward);
                        regularVisit = true;
                        break;
                    }
                }
            }

            // If no valid subpath is left, we have to go back to the unhandled intersections.
            while (nextUnhandled < 0) {
                qCDebug(lcSGCurveIntersectionSolver) << "All subpaths handled. Looking for unhandled intersections.";
                if (intersections.isEmpty()) {
                    qCDebug(lcSGCurveIntersectionSolver) << "All intersections handled. I am done.";
                    fixedPath.setHint(QQuadPath::PathNonIntersecting);
                    path = fixedPath;
                    return true;
                }

                IntersectionData &unhandledIntersec = intersections[0];
                prevIntersection = 0;
                regularVisit = false;
                qCDebug(lcSGCurveIntersectionSolver) << "Revisiting intersection of" << unhandledIntersec.e1 << "with" << unhandledIntersec.e2;
                qCDebug(lcSGCurveIntersectionSolver) << "Handled are" << unhandledIntersec.e1 << "in:" << unhandledIntersec.in1 << "out:" << unhandledIntersec.out1
                                                     << "/" << unhandledIntersec.e2 << "in:" << unhandledIntersec.in2 << "out:" << unhandledIntersec.out2;

                // Searching for the correct direction to go forward.
                // That requires that the intersection + small delta (here 1e-4)
                // is a valid starting point (filling only on one side)
                auto lookForwardOnIntersection = [&](bool *handledPath, int nextE, float nextT, bool nextForward) {
                    if (*handledPath)
                        return false;
                    constexpr float deltaT = 1e-4f;
                    int eForward = nextE;
                    float tForward = nextT + (nextForward ? deltaT : -deltaT);
                    ensureInBounds(&eForward, &tForward, deltaT);
                    QQuadPath::Element::FillSide fillSide = path.fillSideOf(eForward, tForward);
                    if ((nextForward && fillSide == QQuadPath::Element::FillSideRight) ||
                        (!nextForward && fillSide == QQuadPath::Element::FillSideLeft)) {
                        fixedPath.moveTo(path.elementAt(nextE).pointAtFraction(nextT));
                        i1 = startedAtIndex = nextE;
                        t = startedAtT = nextT;
                        forward = nextForward;
                        *handledPath = true;
                        return true;
                    }
                    return false;
                };

                if (lookForwardOnIntersection(&unhandledIntersec.in1, unhandledIntersec.e1, unhandledIntersec.t1, false))
                    break;
                if (lookForwardOnIntersection(&unhandledIntersec.in2, unhandledIntersec.e2, unhandledIntersec.t2, false))
                    break;
                if (lookForwardOnIntersection(&unhandledIntersec.out1, unhandledIntersec.e1, unhandledIntersec.t1, true))
                    break;
                if (lookForwardOnIntersection(&unhandledIntersec.out2, unhandledIntersec.e2, unhandledIntersec.t2, true))
                    break;

                intersections.removeFirst();
                qCDebug(lcSGCurveIntersectionSolver) << "Found no way to move forward at this intersection and removed it.";
            }
        }

    } while (totalIterations < path.elementCount() * 50);
    // Check the totalIterations as a sanity check. Should never be triggered.

    qCDebug(lcSGCurveIntersectionSolver) << "Could not solve intersections of path. This should not happen. Returning the path unchanged.";

    return false;
}


void QSGCurveProcessor::processStroke(const QQuadPath &strokePath,
                                      float miterLimit,
                                      float penWidth,
                                      Qt::PenJoinStyle joinStyle,
                                      Qt::PenCapStyle capStyle,
                                      addStrokeTriangleCallback addTriangle,
                                      int subdivisions)
{
    auto thePath = subdivide(strokePath, subdivisions).flattened(); // TODO: don't flatten, but handle it in the triangulator
    auto triangles = customTriangulator2(thePath, penWidth, joinStyle, capStyle, miterLimit);

    auto addCurveTriangle = [&](const QQuadPath::Element &element, const TriangleData &t) {
        addTriangle(t.points,
                    { element.startPoint(), element.controlPoint(), element.endPoint() },
                    t.normals,
                    element.isLine());
    };

    auto addBevelTriangle = [&](const TrianglePoints &p)
    {
        QVector2D fp1 = p[0];
        QVector2D fp2 = p[2];

        // That describes a path that passes through those points. We want the stroke
        // edge, so we need to shift everything down by the stroke offset

        QVector2D nn = calcNormalVector(p[0], p[2]);
        if (determinant(p) < 0)
            nn = -nn;
        float delta = penWidth / 2;

        QVector2D offset = nn.normalized() * delta;
        fp1 += offset;
        fp2 += offset;

        TrianglePoints n;
        // p1 is inside, so n[1] is {0,0}
        n[0] = (p[0] - p[1]).normalized();
        n[2] = (p[2] - p[1]).normalized();

        addTriangle(p, { fp1, QVector2D(0.0f, 0.0f), fp2 }, n, true);
    };

    for (const auto &triangle : triangles) {
        if (triangle.pathElementIndex < 0) {
            addBevelTriangle(triangle.points);
            continue;
        }
        const auto &element = thePath.elementAt(triangle.pathElementIndex);
        addCurveTriangle(element, triangle);
    }
}

// 2x variant of qHash(float)
inline size_t qHash(QVector2D key, size_t seed = 0) noexcept
{
    Q_STATIC_ASSERT(sizeof(QVector2D) == sizeof(quint64));
    // ensure -0 gets mapped to 0
    key[0] += 0.0f;
    key[1] += 0.0f;
    quint64 k;
    memcpy(&k, &key, sizeof(QVector2D));
    return QHashPrivate::hash(k, seed);
}

void QSGCurveProcessor::processFill(const QQuadPath &fillPath,
                                    Qt::FillRule fillRule,
                                    addTriangleCallback addTriangle)
{
    QPainterPath internalHull;
    internalHull.setFillRule(fillRule);

    QMultiHash<QVector2D, int> pointHash;

    auto roundVec2D = [](const QVector2D &p) -> QVector2D {
        return { qRound64(p.x() * 32.0f) / 32.0f, qRound64(p.y() * 32.0f) / 32.0f };
    };

    auto addCurveTriangle = [&](const QQuadPath::Element &element,
                                const QVector2D &sp,
                                const QVector2D &ep,
                                const QVector2D &cp) {
        addTriangle({ sp, cp, ep },
                    {},
                    [&element](QVector2D v) { return elementUvForPoint(element, v); });
    };

    auto addCurveTriangleWithNormals = [&](const QQuadPath::Element &element,
                                           const std::array<QVector2D, 3> &v,
                                           const std::array<QVector2D, 3> &n) {
        addTriangle(v, n, [&element](QVector2D v) { return elementUvForPoint(element, v); });
    };

    auto outsideNormal = [](const QVector2D &startPoint,
                            const QVector2D &endPoint,
                            const QVector2D &insidePoint) {

        QVector2D baseLine = endPoint - startPoint;
        QVector2D insideVector = insidePoint - startPoint;
        QVector2D normal = normalVector(baseLine);

        bool swap = QVector2D::dotProduct(insideVector, normal) < 0;

        return swap ? normal : -normal;
    };

    auto addTriangleForLine = [&](const QQuadPath::Element &element,
                                  const QVector2D &sp,
                                  const QVector2D &ep,
                                  const QVector2D &cp) {
        addCurveTriangle(element, sp, ep, cp);

        // Add triangles on the outer side to make room for AA
        const QVector2D normal = outsideNormal(sp, ep, cp);
        constexpr QVector2D null;
        addCurveTriangleWithNormals(element, {sp, sp, ep}, {null, normal, null});
        addCurveTriangleWithNormals(element, {sp, ep, ep}, {normal, normal, null});
    };

    auto addTriangleForConcave = [&](const QQuadPath::Element &element,
                                     const QVector2D &sp,
                                     const QVector2D &ep,
                                     const QVector2D &cp) {
        addTriangleForLine(element, sp, ep, cp);
    };

    auto addTriangleForConvex = [&](const QQuadPath::Element &element,
                                    const QVector2D &sp,
                                    const QVector2D &ep,
                                    const QVector2D &cp) {
        addCurveTriangle(element, sp, ep, cp);
        // Add two triangles on the outer side to get some more AA

        constexpr QVector2D null;
        // First triangle on the line sp-cp, replacing ep
        {
            const QVector2D normal = outsideNormal(sp, cp, ep);
            addCurveTriangleWithNormals(element, {sp, sp, cp}, {null, normal, null});
        }

        // Second triangle on the line ep-cp, replacing sp
        {
            const QVector2D normal = outsideNormal(ep, cp, sp);
            addCurveTriangleWithNormals(element, {ep, ep, cp}, {null, normal, null});
        }
    };

    auto addFillTriangle = [&](const QVector2D &p1, const QVector2D &p2, const QVector2D &p3) {
        constexpr QVector3D uv(0.0, 1.0, -1.0);
        addTriangle({ p1, p2, p3 },
                    {},
                    [&uv](QVector2D) { return uv; });
    };

    fillPath.iterateElements([&](const QQuadPath::Element &element, int index) {
        QVector2D sp(element.startPoint());
        QVector2D cp(element.controlPoint());
        QVector2D ep(element.endPoint());
        QVector2D rsp = roundVec2D(sp);

        if (element.isSubpathStart())
            internalHull.moveTo(sp.toPointF());
        if (element.isLine()) {
            internalHull.lineTo(ep.toPointF());
            pointHash.insert(rsp, index);
        } else {
            QVector2D rep = roundVec2D(ep);
            QVector2D rcp = roundVec2D(cp);
            if (element.isConvex()) {
                internalHull.lineTo(ep.toPointF());
                addTriangleForConvex(element, rsp, rep, rcp);
                pointHash.insert(rsp, index);
            } else {
                internalHull.lineTo(cp.toPointF());
                internalHull.lineTo(ep.toPointF());
                    addTriangleForConcave(element, rsp, rep, rcp);
                pointHash.insert(rcp, index);
            }
        }
    });

    // Points in p are already rounded do 1/32
    // Returns false if the triangle needs to be split. Adds the triangle to the graphics buffers and returns true otherwise.
    // (Does not handle ambiguous vertices that are on multiple unrelated lines/curves)
    auto onSameSideOfLine = [](const QVector2D &p1,
                               const QVector2D &p2,
                               const QVector2D &linePoint,
                               const QVector2D &lineNormal) {
        float side1 = testSideOfLineByNormal(linePoint, lineNormal, p1);
        float side2 = testSideOfLineByNormal(linePoint, lineNormal, p2);
        return side1 * side2 >= 0;
    };

    auto pointInSafeSpace = [&](const QVector2D &p, const QQuadPath::Element &element) -> bool {
        const QVector2D a = element.startPoint();
        const QVector2D b = element.endPoint();
        const QVector2D c = element.controlPoint();
        // There are "safe" areas of the curve also across the baseline: the curve can never cross:
        // line1: the line through A and B'
        // line2: the line through B and A'
        // Where A' = A "mirrored" through C and B' = B "mirrored" through C
        const QVector2D n1 = calcNormalVector(a, c + (c - b));
        const QVector2D n2 = calcNormalVector(b, c + (c - a));
        bool safeSideOf1 = onSameSideOfLine(p, c, a, n1);
        bool safeSideOf2 = onSameSideOfLine(p, c, b, n2);
        return safeSideOf1 && safeSideOf2;
    };

    // Returns false if the triangle belongs to multiple elements and need to be split.
    // Otherwise adds the triangle, optionally splitting it to avoid "unsafe space"
    auto handleTriangle = [&](const QVector2D (&p)[3]) -> bool {
        bool isLine = false;
        bool isConcave = false;
        bool isConvex = false;
        int elementIndex = -1;

        bool foundElement = false;
        int si = -1;
        int ei = -1;

        for (int i = 0; i < 3; ++i) {
            auto pointFoundRange = std::as_const(pointHash).equal_range(roundVec2D(p[i]));

            if (pointFoundRange.first == pointHash.constEnd())
                continue;

            // This point is on some element, now find the element
            int testIndex = *pointFoundRange.first;
            bool ambiguous = std::next(pointFoundRange.first) != pointFoundRange.second;
            if (ambiguous) {
                // The triangle should be on the inside of exactly one of the elements
                // We're doing the test for each of the points, which maybe duplicates some effort,
                // but optimize for simplicity for now.
                for (auto it = pointFoundRange.first; it != pointFoundRange.second; ++it) {
                    auto &el = fillPath.elementAt(*it);
                    bool fillOnLeft = !el.isFillOnRight();
                    auto sp = roundVec2D(el.startPoint());
                    auto ep = roundVec2D(el.endPoint());
                    // Check if the triangle is on the inside of el; i.e. each point is either sp, ep, or on the inside.
                    auto pointInside = [&](const QVector2D &p) {
                        return p == sp || p == ep
                                || QQuadPath::isPointOnLeft(p, el.startPoint(), el.endPoint()) == fillOnLeft;
                    };
                    if (pointInside(p[0]) && pointInside(p[1]) && pointInside(p[2])) {
                        testIndex = *it;
                        break;
                    }
                }
            }

            const auto &element = fillPath.elementAt(testIndex);
            // Now we check if p[i] -> p[j] is on the element for some j
            // For a line, the relevant line is sp-ep
            // For concave it's cp-sp/ep
            // For convex it's sp-ep again
            bool onElement = false;
            for (int j = 0; j < 3; ++j) {
                if (i == j)
                    continue;
                if (element.isConvex() || element.isLine())
                    onElement = roundVec2D(element.endPoint()) == p[j];
                else // concave
                    onElement = roundVec2D(element.startPoint()) == p[j] || roundVec2D(element.endPoint()) == p[j];
                if (onElement) {
                    if (foundElement)
                        return false; // Triangle already on some other element: must split
                    si = i;
                    ei = j;
                    foundElement = true;
                    elementIndex = testIndex;
                    isConvex = element.isConvex();
                    isLine = element.isLine();
                    isConcave = !isLine && !isConvex;
                    break;
                }
            }
        }

        if (isLine) {
            int ci = (6 - si - ei) % 3; // 1+2+3 is 6, so missing number is 6-n1-n2
            addTriangleForLine(fillPath.elementAt(elementIndex), p[si], p[ei], p[ci]);
        } else if (isConcave) {
            addCurveTriangle(fillPath.elementAt(elementIndex), p[0], p[1], p[2]);
        } else if (isConvex) {
            int oi = (6 - si - ei) % 3;
            const auto &otherPoint = p[oi];
            const auto &element = fillPath.elementAt(elementIndex);
            // We have to test whether the triangle can cross the line
            // TODO: use the toplevel element's safe space
            bool safeSpace = pointInSafeSpace(otherPoint, element);
            if (safeSpace) {
                addCurveTriangle(element, p[0], p[1], p[2]);
            } else {
                // Find a point inside the triangle that's also in the safe space
                QVector2D newPoint = (p[0] +  p[1] + p[2]) / 3;
                // We should calculate the point directly, but just do a lazy implementation for now:
                for (int i = 0; i < 7; ++i) {
                    safeSpace = pointInSafeSpace(newPoint, element);
                    if (safeSpace)
                        break;
                    newPoint = (p[si] +  p[ei] + newPoint) / 3;
                }
                if (safeSpace) {
                    // Split triangle. We know the original triangle is only on one path element, so the other triangles are both fill.
                    // Curve triangle is (sp, ep, np)
                    addCurveTriangle(element, p[si], p[ei], newPoint);
                    // The other two are (sp, op, np) and (ep, op, np)
                    addFillTriangle(p[si], p[oi], newPoint);
                    addFillTriangle(p[ei], p[oi], newPoint);
                } else {
                    // fallback to fill if we can't find a point in safe space
                    addFillTriangle(p[0], p[1], p[2]);
                }
            }

        } else {
            addFillTriangle(p[0], p[1], p[2]);
        }
        return true;
    };

    QTriangleSet triangles = qTriangulate(internalHull);
    // Workaround issue in qTriangulate() for single-triangle path
    if (triangles.indices.size() == 3)
        triangles.indices.setDataUint({ 0, 1, 2 });

    const quint32 *idxTable = static_cast<const quint32 *>(triangles.indices.data());
    for (int triangle = 0; triangle < triangles.indices.size() / 3; ++triangle) {
        const quint32 *idx = &idxTable[triangle * 3];

        QVector2D p[3];
        for (int i = 0; i < 3; ++i) {
            p[i] = roundVec2D(QVector2D(float(triangles.vertices.at(idx[i] * 2)),
                                        float(triangles.vertices.at(idx[i] * 2 + 1))));
        }
        if (qFuzzyIsNull(determinant(p[0], p[1], p[2])))
            continue; // Skip degenerate triangles
        bool needsSplit = !handleTriangle(p);
        if (needsSplit) {
            QVector2D c = (p[0] + p[1] + p[2]) / 3;
            for (int i = 0; i < 3; ++i) {
                qSwap(c, p[i]);
                handleTriangle(p);
                qSwap(c, p[i]);
            }
        }
    }
}


QT_END_NAMESPACE
