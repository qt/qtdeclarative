// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

// ---------------- Cubic -> Quadratic path stuff - temporarily here -----------

#include <private/qbezier_p.h>
#include <QtMath>

QT_BEGIN_NAMESPACE

#if 0
static bool qt_isQuadratic(const QBezier &b)
{
    const qreal f = 3.0 / 2.0;
    const QPointF c1 = b.pt1() + f * (b.pt2() - b.pt1());
    const QPointF c2 = b.pt4() + f * (b.pt3() - b.pt4());
    return c1 == c2;
}
#endif

static qreal qt_scoreQuadratic(const QBezier &b, QPointF qcp)
{
    // Construct a cubic from the quadratic, and compare its control points to the originals'
    const QRectF bounds = b.bounds();
    qreal dim = QLineF(bounds.topLeft(), bounds.bottomRight()).length();
    if (qFuzzyIsNull(dim))
        return 0;
    const qreal f = 2.0 / 3;
    const QPointF cp1 = b.pt1() + f * (qcp - b.pt1());
    const QPointF cp2 = b.pt4() + f * (qcp - b.pt4());
    const QLineF d1(b.pt2(), cp1);
    const QLineF d2(b.pt3(), cp2);
    return qMax(d1.length(), d2.length()) / dim;
}

static qreal qt_quadraticForCubic(const QBezier &b, QPointF *qcp)
{
    const QLineF st = b.startTangent();
    const QLineF et = b.endTangent();
    if (st.intersects(et, qcp) == QLineF::NoIntersection)
        *qcp = b.midPoint();
    return qt_scoreQuadratic(b, *qcp);
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

    const qreal p = n.pt3().x() * n.pt2().y();
    const qreal q = n.pt4().x() * n.pt2().y();
    const qreal r = n.pt2().x() * n.pt3().y();
    const qreal s = n.pt4().x() * n.pt3().y();

    const qreal a = 36 * ((-3 * p) + (2 * q) + (3 * r) - s);
    if (!a)
        return 0;
    const qreal b = -18 * (((3 * p) - q) - (3 * r));
    const qreal c = 18 * (r - p);
    const qreal rad = (b * b) - (2 * a * c);
    if (rad < 0)
        return 0;
    const qreal sqr = qSqrt(rad);
    const qreal root1 = (b + sqr) / a;
    const qreal root2 = (b - sqr) / a;

    int res = 0;
    if (isValidRoot(root1))
        tpoints[res++] = root1;
    if (root2 != root1 && isValidRoot(root2))
        tpoints[res++] = root2;

    if (res == 2 && tpoints[0] > tpoints[1])
        qSwap(tpoints[0], tpoints[1]);

    return res;
}

static void qt_addToQuadratics(const QBezier &b, QPolygonF *p, qreal spanlength, qreal errorLimit)
{
    Q_ASSERT((spanlength > 0) && !(spanlength > 1));

    QPointF qcp;
    bool isOk = (qt_quadraticForCubic(b, &qcp) < errorLimit); // error limit, careful
    if (isOk || spanlength < 0.1) {
        p->append(qcp);
        p->append(b.pt4());
    } else {
        QBezier rhs = b;
        QBezier lhs;
        rhs.parameterSplitLeft(0.5, &lhs);
        qt_addToQuadratics(lhs, p, spanlength / 2, errorLimit);
        qt_addToQuadratics(rhs, p, spanlength / 2, errorLimit);
    }
}

QPolygonF qt_toQuadratics(const QBezier &b, qreal errorLimit)
{

    QPolygonF res;
    res.reserve(16);
    res.append(b.pt1());
    const QRectF cpr = b.bounds();
    qreal epsilon = QLineF(cpr.topLeft(), cpr.bottomRight()).length() * 0.5 * errorLimit;
    bool degenerate = false;
    if (QLineF(b.pt2(), b.pt1()).length() < epsilon) {
        res.append(b.pt3());
        degenerate = true;
    } else if (QLineF(b.pt4(), b.pt3()).length() < epsilon) {
        res.append(b.pt2());
        degenerate = true;
    }
    if (degenerate) {
        res.append(b.pt4());
        return res;
    }
    qreal infPoints[2];
    int numInfPoints = qt_getInflectionPoints(b, infPoints);
    qreal t0 = 0;
    for (int i = 0; i < numInfPoints + 1; i++) { // #segments == #inflectionpoints + 1
        qreal t1 = (i < numInfPoints) ? infPoints[i] : 1;
        QBezier segment = b.bezierOnInterval(t0, t1);
        qt_addToQuadratics(segment, &res, t1 - t0, errorLimit);
        t0 = t1;
    }
    return res;
}

QT_END_NAMESPACE
