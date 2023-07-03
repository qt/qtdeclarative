// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// ---------------- Cubic -> Quadratic path stuff - temporarily here -----------

#include <private/qbezier_p.h>
#include <QtMath>

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
    Q_ASSERT(n.pt1() == QPoint() && qFuzzyIsNull(float(n.pt4().y())));

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

void qt_toQuadratics(const QBezier &b, QPolygonF *out, qreal errorLimit)
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

QT_END_NAMESPACE
