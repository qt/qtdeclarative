// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquadpath_p.h"

#include <private/qsgcurveprocessor_p.h>

#include <QtGui/private/qbezier_p.h>
#include <QtMath>
#include <QtCore/QLoggingCategory>
#include <QtCore/QVarLengthArray>

QT_BEGIN_NAMESPACE

static qreal qt_scoreQuadratic(const QBezier &b, QPointF qcp)
{
    static bool init = false;
    const int numSteps = 21;
    Q_STATIC_ASSERT(numSteps % 2 == 1); // numTries must be odd
    static qreal t2s[numSteps];
    static qreal tmts[numSteps];
    if (!init) {
        // Precompute bezier factors
        qreal t = 0.20;
        const qreal step = (1 - (2 * t)) / (numSteps - 1);
        for (int i = 0; i < numSteps; i++) {
            t2s[i] = t * t;
            tmts[i] = 2 * t * (1 - t);
            t += step;
        }
        init = true;
    }

    const QPointF midPoint = b.midPoint();
    auto distForIndex = [&](int i) -> qreal {
        QPointF qp = (t2s[numSteps - 1 - i] * b.pt1()) + (tmts[i] * qcp) + (t2s[i] * b.pt4());
        QPointF d = midPoint - qp;
        return QPointF::dotProduct(d, d);
    };

    const int halfSteps = (numSteps - 1) / 2;
    bool foundIt = false;
    const qreal centerDist = distForIndex(halfSteps);
    qreal minDist = centerDist;
    // Search for the minimum in right half
    for (int i = 0; i < halfSteps; i++) {
        qreal tDist = distForIndex(halfSteps + 1 + i);
        if (tDist < minDist) {
            minDist = tDist;
        } else {
            foundIt = (i > 0);
            break;
        }
    }
    if (!foundIt) {
        // Search in left half
        minDist = centerDist;
        for (int i = 0; i < halfSteps; i++) {
            qreal tDist = distForIndex(halfSteps - 1 - i);
            if (tDist < minDist) {
                minDist = tDist;
            } else {
                foundIt = (i > 0);
                break;
            }
        }
    }
    return foundIt ? minDist : centerDist;
}

static QPointF qt_quadraticForCubic(const QBezier &b)
{
    const QLineF st = b.startTangent();
    const QLineF et = b.endTangent();
    const QPointF midPoint = b.midPoint();
    bool valid = true;
    QPointF quadControlPoint;
    if (st.intersects(et, &quadControlPoint) == QLineF::NoIntersection) {
        valid = false;
    } else {
        // Check if intersection is on wrong side
        const QPointF bl = b.pt4() - b.pt1();
        const QPointF ml = midPoint - b.pt1();
        const QPointF ql = quadControlPoint - b.pt1();
        qreal cx1 = (ml.x() * bl.y()) - (ml.y() * bl.x());
        qreal cx2 = (ql.x() * bl.y()) - (ql.y() * bl.x());
        valid = (std::signbit(cx1) == std::signbit(cx2));
    }
    return valid ? quadControlPoint : midPoint;
}

static int qt_getInflectionPoints(const QBezier &orig, qreal *tpoints)
{
    auto isValidRoot = [](qreal r) {
        return qIsFinite(r) && (r > 0) && (!qFuzzyIsNull(float(r))) && (r < 1)
                && (!qFuzzyIsNull(float(r - 1)));
    };

    // normalize so pt1.x,pt1.y,pt4.y == 0
    QTransform xf;
    const QLineF l(orig.pt1(), orig.pt4());
    xf.rotate(l.angle());
    xf.translate(-orig.pt1().x(), -orig.pt1().y());
    const QBezier n = orig.mapBy(xf);

    const qreal x2 = n.pt2().x();
    const qreal x3 = n.pt3().x();
    const qreal x4 = n.pt4().x();
    const qreal y2 = n.pt2().y();
    const qreal y3 = n.pt3().y();

    const qreal p = x3 * y2;
    const qreal q = x4 * y2;
    const qreal r = x2 * y3;
    const qreal s = x4 * y3;

    const qreal a = 18 * ((-3 * p) + (2 * q) + (3 * r) - s);
    if (qFuzzyIsNull(float(a))) {
        if (std::signbit(y2) != std::signbit(y3) && qFuzzyCompare(float(x4 - x3), float(x2))) {
            tpoints[0] = 0.5; // approx
            return 1;
        } else if (!a) {
            return 0;
        }
    }
    const qreal b = 18 * (((3 * p) - q) - (3 * r));
    const qreal c = 18 * (r - p);
    const qreal rad = (b * b) - (4 * a * c);
    if (rad < 0)
        return 0;
    const qreal sqr = qSqrt(rad);
    const qreal root1 = (-b + sqr) / (2 * a);
    const qreal root2 = (-b - sqr) / (2 * a);

    int res = 0;
    if (isValidRoot(root1))
        tpoints[res++] = root1;
    if (root2 != root1 && isValidRoot(root2))
        tpoints[res++] = root2;

    if (res == 2 && tpoints[0] > tpoints[1])
        qSwap(tpoints[0], tpoints[1]);

    return res;
}

static void qt_addToQuadratics(const QBezier &b, QPolygonF *p, int maxSplits, qreal maxDiff)
{
    QPointF qcp = qt_quadraticForCubic(b);
    if (maxSplits <= 0 || qt_scoreQuadratic(b, qcp) < maxDiff) {
        p->append(qcp);
        p->append(b.pt4());
    } else {
        QBezier rhs = b;
        QBezier lhs;
        rhs.parameterSplitLeft(0.5, &lhs);
        qt_addToQuadratics(lhs, p, maxSplits - 1, maxDiff);
        qt_addToQuadratics(rhs, p, maxSplits - 1, maxDiff);
    }
}

static void qt_toQuadratics(const QBezier &b, QPolygonF *out, qreal errorLimit = 0.01)
{
    out->resize(0);
    out->append(b.pt1());

    {
        // Shortcut if the cubic is really a quadratic
        const qreal f = 3.0 / 2.0;
        const QPointF c1 = b.pt1() + f * (b.pt2() - b.pt1());
        const QPointF c2 = b.pt4() + f * (b.pt3() - b.pt4());
        if (c1 == c2) {
            out->append(c1);
            out->append(b.pt4());
            return;
        }
    }

    const QRectF cpr = b.bounds();
    const QPointF dim = cpr.bottomRight() - cpr.topLeft();
    qreal maxDiff = QPointF::dotProduct(dim, dim) * errorLimit * errorLimit; // maxdistance^2

    qreal infPoints[2];
    int numInfPoints = qt_getInflectionPoints(b, infPoints);
    const int maxSubSplits = numInfPoints > 0 ? 2 : 3;
    qreal t0 = 0;
    // number of main segments == #inflectionpoints + 1
    for (int i = 0; i < numInfPoints + 1; i++) {
        qreal t1 = (i < numInfPoints) ? infPoints[i] : 1;
        QBezier segment = b.bezierOnInterval(t0, t1);
        qt_addToQuadratics(segment, out, maxSubSplits, maxDiff);
        t0 = t1;
    }
}

QVector2D QQuadPath::Element::pointAtFraction(float t) const
{
    if (isLine()) {
        return sp + t * (ep - sp);
    } else {
        const float r = 1 - t;
        return (r * r * sp) + (2 * t * r * cp) + (t * t * ep);
    }
}

QQuadPath::Element QQuadPath::Element::segmentFromTo(float t0, float t1) const
{
    if (t0 <= 0 && t1 >= 1)
        return *this;

    Element part;
    part.sp = pointAtFraction(t0);
    part.ep = pointAtFraction(t1);

    if (isLine()) {
        part.cp = 0.5f * (part.sp + part.ep);
        part.m_isLine = true;
    } else {
        // Split curve right at t0, yields { t0, rcp, endPoint } quad segment
        const QVector2D rcp = (1 - t0) * controlPoint() + t0 * endPoint();
        // Split that left at t1, yields  { t0, lcp, t1 } quad segment
        float segmentT = (t1 - t0) / (1 - t0);
        part.cp = (1 - segmentT) * part.sp + segmentT * rcp;
    }
    return part;
}

QQuadPath::Element QQuadPath::Element::reversed() const {
    Element swappedElement;
    swappedElement.ep = sp;
    swappedElement.cp = cp;
    swappedElement.sp = ep;
    swappedElement.m_isLine = m_isLine;
    return swappedElement;
}

float QQuadPath::Element::extent() const
{
    // TBD: cache this value if we start using it a lot
    QVector2D min(qMin(sp.x(), ep.x()), qMin(sp.y(), ep.y()));
    QVector2D max(qMax(sp.x(), ep.x()), qMax(sp.y(), ep.y()));
    if (!isLine()) {
        min = QVector2D(qMin(min.x(), cp.x()), qMin(min.y(), cp.y()));
        max = QVector2D(qMax(max.x(), cp.x()), qMax(max.y(), cp.y()));
    }
    return (max - min).length();
}

// Returns the number of intersections between element and a horizontal line at y.
// The t values of max 2 intersection(s) are stored in the fractions array
int QQuadPath::Element::intersectionsAtY(float y, float *fractions, bool swapXY) const
{
    Q_ASSERT(!isLine());

    auto getY = [=](QVector2D p) -> float { return swapXY ? -p.x() : p.y(); };

    const float y0 = getY(startPoint()) - y;
    const float y1 = getY(controlPoint()) - y;
    const float y2 = getY(endPoint()) - y;

    int numRoots = 0;
    const float a = y0 - (2 * y1) + y2;
    if (a) {
        const float b = (y1 * y1) - (y0 * y2);
        if (b >= 0) {
            const float sqr = qSqrt(b);
            const float root1 = -(-y0 + y1 + sqr) / a;
            if (qIsFinite(root1) && root1 >= 0 && root1 <= 1)
                fractions[numRoots++] = root1;
            const float root2 = (y0 - y1 + sqr) / a;
            if (qIsFinite(root2) && root2 != root1 && root2 >= 0 && root2 <= 1)
                fractions[numRoots++] = root2;
        }
    } else if (y1 != y2) {
        const float root1 = (y2 - (2 * y1)) / (2 * (y2 - y1));
        if (qIsFinite(root1) && root1 >= 0 && root1 <= 1)
            fractions[numRoots++] = root1;
    }

    return numRoots;
}

static float crossProduct(const QVector2D &sp, const QVector2D &p, const QVector2D &ep)
{
    QVector2D v1 = ep - sp;
    QVector2D v2 = p - sp;
    return (v2.x() * v1.y()) - (v2.y() * v1.x());
}

bool QQuadPath::isPointOnLeft(const QVector2D &p, const QVector2D &sp, const QVector2D &ep)
{
    // Use cross product to compare directions of base vector and vector from start to p
    return crossProduct(sp, p, ep) >= 0.0f;
}

bool QQuadPath::isPointOnLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep)
{
    return qFuzzyIsNull(crossProduct(sp, p, ep));
}

// Assumes sp != ep
bool QQuadPath::isPointNearLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep)
{
    // epsilon is max length of p-to-baseline relative to length of baseline. So 0.01 means that
    // the distance from p to the baseline must be less than 1% of the length of the baseline.
    constexpr float epsilon = 0.01f;
    QVector2D bv = ep - sp;
    float bl2 = QVector2D::dotProduct(bv, bv);
    float t = QVector2D::dotProduct(p - sp, bv) / bl2;
    QVector2D pv = p - (sp + t * bv);
    return (QVector2D::dotProduct(pv, pv) / bl2) < (epsilon * epsilon);
}

QVector2D QQuadPath::closestPointOnLine(const QVector2D &p, const QVector2D &sp, const QVector2D &ep)
{
    QVector2D line = ep - sp;
    float t = QVector2D::dotProduct(p - sp, line) / QVector2D::dotProduct(line, line);
    return sp + qBound(0.0f, t, 1.0f) * line;
}

// NOTE: it is assumed that subpaths are closed
bool QQuadPath::contains(const QVector2D &point) const
{
    return contains(point, 0, elementCount() - 1);
}

bool QQuadPath::contains(const QVector2D &point, int fromIndex, int toIndex) const
{
    // if (!controlPointRect().contains(pt) : good opt when we add cpr caching
    //     return false;

    int winding_number = 0;
    for (int ei = fromIndex; ei <= toIndex; ei++) {
        const Element &e = m_elements.at(ei);
        int dir = 1;
        float y1 = e.startPoint().y();
        float y2 = e.endPoint().y();
        if (y2 < y1) {
            qSwap(y1, y2);
            dir = -1;
        }
        if (e.m_isLine) {
            if (point.y() < y1 || point.y() >= y2 || y1 == y2)
                continue;
            const float t = (point.y() - e.startPoint().y()) / (e.endPoint().y() - e.startPoint().y());
            const float x = e.startPoint().x() + t * (e.endPoint().x() - e.startPoint().x());
            if (x <= point.x())
                winding_number += dir;
        } else {
            y1 = qMin(y1, e.controlPoint().y());
            y2 = qMax(y2, e.controlPoint().y());
            if (point.y() < y1 || point.y() >= y2)
                continue;
            float ts[2];
            const int numRoots = e.intersectionsAtY(point.y(), ts);
            // Count if there is exactly one intersection to the left
            bool oneHit = false;
            float tForHit = -1;
            for (int i = 0; i < numRoots; i++) {
                if (e.pointAtFraction(ts[i]).x() <= point.x()) {
                    oneHit = !oneHit;
                    tForHit = ts[i];
                }
            }
            if (oneHit) {
                dir = e.tangentAtFraction(tForHit).y() < 0 ? -1 : 1;
                winding_number += dir;
            }
        }
    };

    return (fillRule() == Qt::WindingFill ? (winding_number != 0) : ((winding_number % 2) != 0));
}

// similar as contains. But we treat the element with the index elementIdx in a special way
// that should be numerically more stable. The result is a contains for a point on the left
// and for the right side of the element.
QQuadPath::Element::FillSide QQuadPath::fillSideOf(int elementIdx, float elementT) const
{
    constexpr float toleranceT = 1e-3f;
    const QVector2D point = m_elements.at(elementIdx).pointAtFraction(elementT);
    const QVector2D tangent = m_elements.at(elementIdx).tangentAtFraction(elementT);

    const bool swapXY = qAbs(tangent.x()) > qAbs(tangent.y());
    auto getX = [=](QVector2D p) -> float { return swapXY ? p.y() : p.x(); };
    auto getY = [=](QVector2D p) -> float { return swapXY ? -p.x() : p.y(); };

    int winding_number = 0;
    for (int i = 0; i < elementCount(); i++) {
        const Element &e = m_elements.at(i);
        int dir =  1;
        float y1 = getY(e.startPoint());
        float y2 = getY(e.endPoint());
        if (y2 < y1) {
            qSwap(y1, y2);
            dir = -1;
        }
        if (e.m_isLine) {
            if (getY(point) < y1 || getY(point) >= y2 || y1 == y2)
                continue;
            const float t = (getY(point) - getY(e.startPoint())) / (getY(e.endPoint()) - getY(e.startPoint()));
            const float x = getX(e.startPoint()) + t * (getX(e.endPoint()) - getX(e.startPoint()));
            if (x <= getX(point) && (i != elementIdx || qAbs(t - elementT) > toleranceT))
                winding_number += dir;
        } else {
            y1 = qMin(y1, getY(e.controlPoint()));
            y2 = qMax(y2, getY(e.controlPoint()));
            if (getY(point) < y1 || getY(point) >= y2)
                continue;
            float ts[2];
            const int numRoots = e.intersectionsAtY(getY(point), ts, swapXY);
            // Count if there is exactly one intersection to the left
            bool oneHit = false;
            float tForHit = -1;
            for (int j = 0; j < numRoots; j++) {
                const float x = getX(e.pointAtFraction(ts[j]));
                if (x <= getX(point) && (i != elementIdx || qAbs(ts[j] - elementT) > toleranceT)) {
                    oneHit = !oneHit;
                    tForHit = ts[j];
                }
            }
            if (oneHit) {
                dir = getY(e.tangentAtFraction(tForHit)) < 0 ? -1 : 1;
                winding_number += dir;
            }
        }
    };

    int left_winding_number = winding_number;
    int right_winding_number = winding_number;

    int dir = getY(tangent) < 0 ? -1 : 1;

    if (dir > 0)
        left_winding_number += dir;
    else
        right_winding_number += dir;

    bool leftInside = (fillRule() == Qt::WindingFill ? (left_winding_number != 0) : ((left_winding_number % 2) != 0));
    bool rightInside = (fillRule() == Qt::WindingFill ? (right_winding_number != 0) : ((right_winding_number % 2) != 0));

    if (leftInside && rightInside)
        return QQuadPath::Element::FillSideBoth;
    else if (leftInside)
        return QQuadPath::Element::FillSideLeft;
    else if (rightInside)
        return QQuadPath::Element::FillSideRight;
    else
        return QQuadPath::Element::FillSideUndetermined; //should not happen except for numerical error.
}

void QQuadPath::addElement(const QVector2D &control, const QVector2D &endPoint, bool isLine)
{
    if (qFuzzyCompare(m_currentPoint, endPoint))
        return; // 0 length element, skip

    isLine = isLine || isPointNearLine(control, m_currentPoint, endPoint); // Turn flat quad into line

    m_elements.resize(m_elements.size() + 1);
    Element &elem = m_elements.last();
    elem.sp = m_currentPoint;
    elem.cp = isLine ? (0.5f * (m_currentPoint + endPoint)) : control;
    elem.ep = endPoint;
    elem.m_isLine = isLine;
    elem.m_isSubpathStart = m_subPathToStart;
    m_subPathToStart = false;
    m_currentPoint = endPoint;
}

void QQuadPath::addElement(const Element &e)
{
    m_subPathToStart = false;
    m_currentPoint = e.endPoint();
    m_elements.append(e);
}

#if !defined(QQUADPATH_CONVEX_CHECK_ERROR_MARGIN)
#  define QQUICKSHAPECURVERENDERER_CONVEX_CHECK_ERROR_MARGIN (1.0f / 32.0f)
#endif

QQuadPath::Element::FillSide QQuadPath::coordinateOrderOfElement(const QQuadPath::Element &element) const
{
    QVector2D baseLine = element.endPoint() - element.startPoint();
    QVector2D midPoint = element.midPoint();
    // At the midpoint, the tangent of a quad is parallel to the baseline
    QVector2D normal = QVector2D(-baseLine.y(), baseLine.x()).normalized();
    float delta = qMin(element.extent() / 100, QQUICKSHAPECURVERENDERER_CONVEX_CHECK_ERROR_MARGIN);
    QVector2D offset = (normal * delta);
    bool pathContainsPointToRight = contains(midPoint + offset);
    bool pathContainsPointToLeft = contains(midPoint - offset);
    Element::FillSide res = Element::FillSideUndetermined;
    if (pathContainsPointToRight)
        res = (pathContainsPointToLeft ? Element::FillSideBoth : Element::FillSideRight);
    else if (pathContainsPointToLeft)
        res = Element::FillSideLeft;
    return res;
}

QQuadPath QQuadPath::fromPainterPath(const QPainterPath &path, PathHints hints)
{
    QQuadPath res;
    res.reserve(path.elementCount());
    res.setFillRule(path.fillRule());

    const bool isQuadratic = hints & PathQuadratic;

    QPolygonF quads;
    QPointF sp;
    for (int i = 0; i < path.elementCount(); ++i) {
        QPainterPath::Element element = path.elementAt(i);

        QPointF ep(element);
        switch (element.type) {
        case QPainterPath::MoveToElement:
            res.moveTo(QVector2D(ep));
            break;
        case QPainterPath::LineToElement:
            res.lineTo(QVector2D(ep));
            break;
        case QPainterPath::CurveToElement: {
            QPointF cp1 = ep;
            QPointF cp2(path.elementAt(++i));
            ep = path.elementAt(++i);
            if (isQuadratic) {
                const qreal f = 3.0 / 2.0;
                const QPointF cp = sp + f * (cp1 - sp);
                res.quadTo(QVector2D(cp), QVector2D(ep));
            } else {
                QBezier b = QBezier::fromPoints(sp, cp1, cp2, ep);
                qt_toQuadratics(b, &quads);
                for (int i = 1; i < quads.size(); i += 2) {
                    QVector2D cp(quads[i]);
                    QVector2D ep(quads[i + 1]);
                    res.quadTo(cp, ep);
                }
            }
            break;
        }
        default:
            Q_UNREACHABLE();
            break;
        }
        sp = ep;
    }

    res.setPathHints(hints | PathQuadratic);
    return res;
}

void QQuadPath::addCurvatureData()
{
    // We use the convention that the inside of a curve is on the *right* side of the
    // direction of the baseline.Thus, as long as this is true: if the control point is
    // on the left side of the baseline, the curve is convex  and otherwise it is
    // concave. The paths we get can be arbitrary order, but each subpath will have a
    // consistent order. Therefore, for the first curve element in a subpath, we can
    // determine if the direction already follows the convention or not, and then we
    // can easily detect curvature of all subsequent elements in the subpath.

    auto isSingleSided = [](Element::FillSide fillSide) {
        return fillSide == Element::FillSideLeft || fillSide == Element::FillSideRight;
    };

    auto flagFromFillSide = [](Element::FillSide fillSide) {
        if (fillSide == Element::FillSideRight || fillSide == Element::FillSideBoth)
            return Element::FillOnRight;
        else
            return Element::CurvatureUndetermined;
    };

    static bool checkAnomaly = qEnvironmentVariableIntValue("QT_QUICKSHAPES_CHECK_ALL_CURVATURE") != 0;
    const bool pathHasFillOnRight = testHint(PathFillOnRight);

    Element::CurvatureFlags flags = Element::CurvatureUndetermined;
    for (int i = 0; i < m_elements.size(); i++) {
        QQuadPath::Element &element = m_elements[i];
        Q_ASSERT(element.childCount() == 0);
        if (element.isSubpathStart()) {
            if (pathHasFillOnRight && !checkAnomaly) {
                flags = Element::FillOnRight;
            } else {
                Element::FillSide fillSide = Element::FillSideUndetermined;
                for (int j = i; !isSingleSided(fillSide) && j < m_elements.size(); j++) {
                    const QQuadPath::Element &subElem = m_elements.at(j);
                    if (j > i && subElem.isSubpathStart())
                        break;
                    fillSide = coordinateOrderOfElement(subElem);
                }
                flags = flagFromFillSide(fillSide);
            }
        } else if (checkAnomaly) {
            Element::FillSide fillSide = coordinateOrderOfElement(element);
            if (isSingleSided(fillSide)) {
                Element::CurvatureFlags newFlags = flagFromFillSide(fillSide);
                if (flags != newFlags) {
                    qCDebug(lcSGCurveProcessor)
                            << "Curvature anomaly detected:" << element
                            << "Subpath fill on right:" << (flags & Element::FillOnRight)
                            << "Element fill on right:" << (newFlags & Element::FillOnRight);
                    flags = newFlags;
                }
            }
        }

        if (element.isLine()) {
            element.m_curvatureFlags = flags;
        } else {
            bool controlPointOnLeft = element.isControlPointOnLeft();
            bool isFillOnRight = flags & Element::FillOnRight;
            bool isConvex = controlPointOnLeft == isFillOnRight;

            if (isConvex)
                element.m_curvatureFlags = Element::CurvatureFlags(flags | Element::Convex);
            else
                element.m_curvatureFlags = flags;
        }
    }
}

QRectF QQuadPath::controlPointRect() const
{
    QRectF res;
    if (elementCount()) {
        QVector2D min, max;
        min = max = m_elements.constFirst().sp;
        // No need to recurse, as split curve's controlpoints are within the parent curve's
        for (const QQuadPath::Element &e : std::as_const(m_elements)) {
            min.setX(std::min({ min.x(), e.sp.x(), e.cp.x(), e.ep.x() }));
            min.setY(std::min({ min.y(), e.sp.y(), e.cp.y(), e.ep.y() }));
            max.setX(std::max({ max.x(), e.sp.x(), e.cp.x(), e.ep.x() }));
            max.setY(std::max({ max.y(), e.sp.y(), e.cp.y(), e.ep.y() }));
        }
        res = QRectF(min.toPointF(), max.toPointF());
    }
    return res;
}

// Count leaf elements
int QQuadPath::elementCountRecursive() const
{
    int count = 0;
    iterateElements([&](const QQuadPath::Element &, int) { count++; });
    return count;
}

QPainterPath QQuadPath::toPainterPath() const
{
    // Currently only converts the main, unsplit path; no need for the split path identified
    QPainterPath res;
    res.reserve(elementCount());
    res.setFillRule(fillRule());
    for (const Element &element : m_elements) {
        if (element.m_isSubpathStart)
            res.moveTo(element.startPoint().toPointF());
        if (element.m_isLine)
            res.lineTo(element.endPoint().toPointF());
        else
            res.quadTo(element.controlPoint().toPointF(), element.endPoint().toPointF());
    };
    return res;
}

QString QQuadPath::asSvgString() const
{
    QString res;
    QTextStream str(&res);
    for (const Element &element : m_elements) {
        if (element.isSubpathStart())
            str << "M " << element.startPoint().x() << " " << element.startPoint().y() << " ";
        if (element.isLine())
            str << "L " << element.endPoint().x() << " " << element.endPoint().y() << " ";
        else
            str << "Q " << element.controlPoint().x() << " " << element.controlPoint().y() << " "
                << element.endPoint().x() << " " << element.endPoint().y() << " ";
    }
    return res;
}

// Returns a new path since doing it inline would probably be less efficient
// (technically changing it from O(n) to O(n^2))
// Note that this function should be called before splitting any elements,
// so we can assume that the structure is a list and not a tree
QQuadPath QQuadPath::subPathsClosed(bool *didClose) const
{
    Q_ASSERT(m_childElements.isEmpty());
    bool closed = false;
    QQuadPath res = *this;
    res.m_subPathToStart = false;
    res.m_elements = {};
    res.m_elements.reserve(elementCount());
    int subStart = -1;
    int prevElement = -1;
    for (int i = 0; i < elementCount(); i++) {
        const auto &element = m_elements.at(i);
        if (element.m_isSubpathStart) {
            if (subStart >= 0 && m_elements[i - 1].ep != m_elements[subStart].sp) {
                res.m_currentPoint = m_elements[i - 1].ep;
                res.lineTo(m_elements[subStart].sp);
                closed = true;
                auto &endElement = res.m_elements.last();
                endElement.m_isSubpathEnd = true;
                // lineTo() can bail out if the points are too close.
                // In that case, just change the end point to be equal to the start
                // (No need to test because the assignment is a no-op otherwise).
                endElement.ep = m_elements[subStart].sp;
            } else if (prevElement >= 0) {
                res.m_elements[prevElement].m_isSubpathEnd = true;
            }
            subStart = i;
        }
        res.m_elements.append(element);
        prevElement = res.m_elements.size() - 1;
    }

    if (subStart >= 0 && m_elements.last().ep != m_elements[subStart].sp) {
        res.m_currentPoint = m_elements.last().ep;
        res.lineTo(m_elements[subStart].sp);
        closed = true;
    }
    if (!res.m_elements.isEmpty()) {
        auto &endElement = res.m_elements.last();
        endElement.m_isSubpathEnd = true;
        endElement.ep = m_elements[subStart].sp;
    }

    if (didClose)
        *didClose = closed;
    return res;
}

QQuadPath QQuadPath::flattened() const
{
    QQuadPath res;
    res.reserve(elementCountRecursive());
    iterateElements([&](const QQuadPath::Element &elem, int) { res.m_elements.append(elem); });
    res.setPathHints(pathHints());
    res.setFillRule(fillRule());
    return res;
}

class ElementCutter
{
public:
    ElementCutter(const QQuadPath::Element &element)
        : m_element(element)
    {
        m_currentPoint = m_element.startPoint();
        if (m_element.isLine())
            m_lineLength = (m_element.endPoint() - m_element.startPoint()).length();
        else
            fillLUT();
    }

    bool consume(float length)
    {
        m_lastT = m_currentT;
        m_lastPoint = m_currentPoint;
        float nextCut = m_consumed + length;
        float cutT = m_element.isLine() ? nextCut / m_lineLength : tForLength(nextCut);
        if (cutT < 1) {
            m_currentT = cutT;
            m_currentPoint = m_element.pointAtFraction(m_currentT);
            m_consumed = nextCut;
            return true;
        } else {
            m_currentT = 1;
            m_currentPoint = m_element.endPoint();
            return false;
        }
    }

    QVector2D currentCutPoint()
    {
        return m_currentPoint;
    }

    QVector2D currentControlPoint()
    {
        Q_ASSERT(!m_element.isLine());
        // Split curve right at lastT, yields { lastPoint, rcp, endPoint } quad segment
        QVector2D rcp = (1 - m_lastT) * m_element.controlPoint() + m_lastT * m_element.endPoint();
        // Split that left at currentT, yields  { lastPoint, lcp, currentPoint } quad segment
        float segmentT = (m_currentT - m_lastT) / (1 - m_lastT);
        QVector2D lcp = (1 - segmentT) * m_lastPoint + segmentT * rcp;
        return lcp;
    }

    float lastLength()
    {
        float elemLength = m_element.isLine() ? m_lineLength : m_lut.last();
        return elemLength - m_consumed;
    }

private:
    void fillLUT()
    {
        Q_ASSERT(!m_element.isLine());
        QVector2D ap = m_element.startPoint() - 2 * m_element.controlPoint() + m_element.endPoint();
        QVector2D bp = 2 * m_element.controlPoint() - 2 * m_element.startPoint();
        float A = 4 * QVector2D::dotProduct(ap, ap);
        float B = 4 * QVector2D::dotProduct(ap, bp);
        float C = QVector2D::dotProduct(bp, bp);
        float b = B / (2 * A);
        float c = C / A;
        float k = c - (b * b);
        float l2 = b * std::sqrt(b * b + k);
        float lnom = b + std::sqrt(b * b + k);
        float l0 = 0.5f * std::sqrt(A);

        m_lut.resize(LUTSize, 0);
        for (int i = 1; i < LUTSize; i++) {
            float t = float(i) / (LUTSize - 1);
            float u = t + b;
            float w = std::sqrt(u * u + k);
            float l1 = u * w;
            float lden = u + w;
            float l3 = k * std::log(std::fabs(lden / lnom));
            float res = l0 * (l1 - l2 + l3);
            m_lut[i] = res;
        }
    }

    float tForLength(float length)
    {
        Q_ASSERT(!m_element.isLine());
        Q_ASSERT(!m_lut.isEmpty());

        float res = 2; // I.e. invalid, outside [0, 1] range
        auto it = std::upper_bound(m_lut.cbegin(), m_lut.cend(), length);
        if (it != m_lut.cend()) {
            float nextLength = *it--;
            float prevLength = *it;
            int prevIndex = std::distance(m_lut.cbegin(), it);
            float fraction = (length - prevLength) / (nextLength - prevLength);
            res = (prevIndex + fraction) / (LUTSize - 1);
        }
        return res;
    }

    const QQuadPath::Element &m_element;
    float m_lastT = 0;
    float m_currentT = 0;
    QVector2D m_lastPoint;
    QVector2D m_currentPoint;
    float m_consumed = 0;
    // For line elements:
    float m_lineLength;
    // For quadratic curve elements:
    static constexpr int LUTSize = 21;
    QVarLengthArray<float, LUTSize> m_lut;
};

QQuadPath QQuadPath::dashed(qreal lineWidth, const QList<qreal> &dashPattern, qreal dashOffset) const
{
    QVarLengthArray<float, 16> pattern;
    float patternLength = 0;
    for (int i = 0; i < 2 * (dashPattern.length() / 2); i++) {
        float dashLength = qMax(lineWidth * dashPattern[i], qreal(0));
        pattern.append(dashLength);
        patternLength += dashLength;
    }
    if (patternLength == 0)
        return {};

    int startIndex = 0;
    float startOffset = std::fmod(lineWidth * dashOffset, patternLength);
    if (startOffset < 0)
        startOffset += patternLength;
    for (float dashLength : pattern) {
        if (dashLength > startOffset)
            break;
        startIndex = (startIndex + 1) % pattern.size(); // The % guards against accuracy issues
        startOffset -= dashLength;
    }

    int dashIndex = startIndex;
    float offset = startOffset;
    QQuadPath res;
    for (int i = 0; i < elementCount(); i++) {
        const Element &element = elementAt(i);
        if (element.isSubpathStart()) {
            res.moveTo(element.startPoint());
            dashIndex = startIndex;
            offset = startOffset;
        }
        ElementCutter cutter(element);
        while (true) {
            bool gotAll = cutter.consume(pattern.at(dashIndex) - offset);
            QVector2D nextPoint = cutter.currentCutPoint();
            if (dashIndex & 1)
                res.moveTo(nextPoint); // gap
            else if (element.isLine())
                res.lineTo(nextPoint); // dash in line
            else
                res.quadTo(cutter.currentControlPoint(), nextPoint); // dash in curve
            if (gotAll) {
                offset = 0;
                dashIndex = (dashIndex + 1) % pattern.size();
            } else {
                offset += cutter.lastLength();
                break;
            }
        }
    }
    res.setFillRule(fillRule());
    res.setPathHints(pathHints());
    return res;
}

void QQuadPath::splitElementAt(int index)
{
    const int newChildIndex = m_childElements.size();
    m_childElements.resize(newChildIndex + 2);
    Element &parent = elementAt(index);
    parent.m_numChildren = 2;
    parent.m_firstChildIndex = newChildIndex;

    Element &quad1 = m_childElements[newChildIndex];
    const QVector2D mp = parent.midPoint();
    quad1.sp = parent.sp;
    quad1.cp = 0.5f * (parent.sp + parent.cp);
    quad1.ep = mp;
    quad1.m_isSubpathStart = parent.m_isSubpathStart;
    quad1.m_isSubpathEnd = false;
    quad1.m_curvatureFlags = parent.m_curvatureFlags;
    quad1.m_isLine = parent.m_isLine; //### || isPointNearLine(quad1.cp, quad1.sp, quad1.ep);

    Element &quad2 = m_childElements[newChildIndex + 1];
    quad2.sp = mp;
    quad2.cp = 0.5f * (parent.ep + parent.cp);
    quad2.ep = parent.ep;
    quad2.m_isSubpathStart = false;
    quad2.m_isSubpathEnd = parent.m_isSubpathEnd;
    quad2.m_curvatureFlags = parent.m_curvatureFlags;
    quad2.m_isLine = parent.m_isLine; //### || isPointNearLine(quad2.cp, quad2.sp, quad2.ep);

#ifndef QT_NO_DEBUG
    if (qFuzzyCompare(quad1.sp, quad1.ep) || qFuzzyCompare(quad2.sp, quad2.ep))
        qCDebug(lcSGCurveProcessor) << "Splitting has resulted in ~null quad";
#endif
}

static void printElement(QDebug stream, const QQuadPath::Element &element)
{
    auto printPoint = [&](QVector2D p) { stream << "(" << p.x() << ", " << p.y() << ") "; };
    stream << "{ ";
    printPoint(element.startPoint());
    printPoint(element.controlPoint());
    printPoint(element.endPoint());
    stream << "} " << (element.isLine() ? "L " : "C ") << (element.isConvex() ? "X " : "O ")
           << (element.isSubpathStart() ? "S" : element.isSubpathEnd() ? "E" : "");
}

QDebug operator<<(QDebug stream, const QQuadPath::Element &element)
{
    QDebugStateSaver saver(stream);
    stream.nospace();
    stream << "QuadPath::Element( ";
    printElement(stream, element);
    stream << " )";
    return stream;
}

QDebug operator<<(QDebug stream, const QQuadPath &path)
{
    QDebugStateSaver saver(stream);
    stream.nospace();
    stream << "QuadPath(" << path.elementCount() << " main elements, "
           << path.elementCountRecursive() << " leaf elements, "
           << (path.fillRule() == Qt::OddEvenFill ? "OddEven" : "Winding") << Qt::endl;
    int count = 0;
    path.iterateElements([&](const QQuadPath::Element &e, int) {
        stream << " " << count++ << (e.isSubpathStart() ? " >" : " ");
        printElement(stream, e);
        stream << Qt::endl;
    });
    stream << ")";
    return stream;
}

QT_END_NAMESPACE
