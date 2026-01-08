// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickellipseshape_p.h"
#include "qquickellipseshape_p_p.h"
#include <algorithm>
#include <QtMath>

QT_BEGIN_NAMESPACE

namespace {
inline bool is_equal(qreal a, qreal b, qreal epsilon = DBL_EPSILON)
{
    return std::fabs(a - b) <= epsilon;
}

inline qreal arc_angle(qreal angle)
{
    return angle - 90;
}

inline QVector2D arc_point(QVector2D center, QVector2D radius, qreal angle)
{
    return QVector2D(center.x() + radius.x() * qCos(qDegreesToRadians(angle)),
                     center.y() + radius.y() * qSin(qDegreesToRadians(angle)));
}

// counter-clockwise
inline QVector2D tangent_ccw(QVector2D radius, qreal angle)
{
    return QVector2D(-radius.x() * qSin(qDegreesToRadians(angle)),
                     radius.y() * qCos(qDegreesToRadians(angle)));
}

inline qreal cross(QVector2D a, QVector2D b)
{
    return a.x() * b.y() - a.y() * b.x();
}

// cross product of two vectors defined by points A->B x C->D
inline qreal cross(QVector2D a, QVector2D b, QVector2D c, QVector2D d)
{
    return cross(b - a, d - c);
}

qreal angle_between_vectors(QVector2D a, QVector2D b)
{
    const QVector2D uA = a.normalized();
    const QVector2D uB = b.normalized();
    const qreal angle = qAtan2(cross(uA, uB), QVector2D::dotProduct(uA, uB));
    if (std::fabs(angle) < FLT_EPSILON)
        return 0.0f;
    return angle;
}

// the intersection point can be calculated as C + T * (D - C) or A + S * (B - A)
bool lines_intersect(QVector2D a, QVector2D b, QVector2D c, QVector2D d, qreal *s, qreal *t)
{
    // lines undefined
    if ((a.x() == b.x() && a.y() == b.y()) || (c.x() == d.x() && c.y() == d.y()))
        return false;

    const qreal denom = cross(a, b, c, d);

    // lines are parallel or overlap
    if (denom == 0)
        return false;

    if (s != nullptr)
        *s = cross(c, d, c, a) / denom;
    if (t != nullptr)
        *t = cross(a, b, c, a) / denom;

    return true;
}
} // namespace

QQuickEllipseShapePrivate::QQuickEllipseShapePrivate() = default;

QQuickEllipseShapePrivate::~QQuickEllipseShapePrivate() = default;

void QQuickEllipseShapePrivate::addLine(QVector2D point)
{
    auto line = new QQuickPathLine(path);
    line->setX(point.x());
    line->setY(point.y());
    QQuickPathPrivate::get(path)->appendPathElement(line);
}

void QQuickEllipseShapePrivate::addArc(QVector2D point, QVector2D arcRadius,
                                       QQuickPathArc::ArcDirection dir, bool largeArc)
{
    auto arc = new QQuickPathArc(path);
    arc->setX(point.x());
    arc->setY(point.y());
    arc->setRadiusX(arcRadius.x());
    arc->setRadiusY(arcRadius.y());
    arc->setDirection(dir);
    arc->setUseLargeArc(largeArc);
    QQuickPathPrivate::get(path)->appendPathElement(arc);
}

qreal QQuickEllipseShapePrivate::getBorderOffset() const
{
    if (QQuickEllipseShape::BorderMode::Middle == borderMode)
        return 0;
    else if (QQuickEllipseShape::BorderMode::Outside == borderMode)
        return -path->strokeWidth() * 0.5;
    return path->strokeWidth() * 0.5f; // inside
}

void QQuickEllipseShapePrivate::roundCenter(QVector2D center, QVector2D ellipseRadius)
{
    const qreal endAngle = arc_angle(startAngle + sweepAngle);
    const QVector2D endPoint = arc_point(center, ellipseRadius, endAngle);
    const qreal beginAngle = arc_angle(startAngle);
    const QVector2D beginPoint = arc_point(center, ellipseRadius, beginAngle);

    const QVector2D AB = endPoint - center;
    const QVector2D AC = beginPoint - center;

    const qreal a = angle_between_vectors(AB, AC);
    const qreal halfAngle = std::fabs(a) * 0.5f;

    const qreal maxCornerRadius = (std::min(AB.length(), AC.length()) * 0.5f) * qTan(halfAngle);
    const qreal corner_radius = std::min(cornerRadius, maxCornerRadius);

    // calculate B and C based on the corner radius
    const qreal edgeOffset = corner_radius / qTan(halfAngle);
    const QVector2D B = center + (AB.normalized() * edgeOffset);
    const QVector2D C = center + (AC.normalized() * edgeOffset);

    // update
    auto &rc = roundedCorners[RoundedCornerIndex::Center];
    rc.A = center;
    rc.B = B;
    rc.C = C;
    rc.alpha = a;
    rc.radius = corner_radius;
}

void QQuickEllipseShapePrivate::roundBeginEnd(QVector2D center, QVector2D ellipseRadius)
{
    qreal deg = 0.0f;
    const qreal endAngle = startAngle + sweepAngle;
    bool e_outer = false, b_outer = false, e_inner = false, b_inner = false;
    while (deg < 45.0f) {
        deg += 1.0f;
        if (e_outer && b_outer && e_inner && b_inner)
            break;
        if (!b_outer)
            b_outer = roundOuter(center, ellipseRadius, deg, startAngle, endAngle,
                                 RoundedCornerIndex::OuterBegin);
        if (!e_outer)
            e_outer = roundOuter(center, ellipseRadius, deg, endAngle, startAngle,
                                 RoundedCornerIndex::OuterEnd);
        if (!e_inner)
            e_inner = roundInner(center, ellipseRadius, deg, endAngle, startAngle,
                                 RoundedCornerIndex::InnerEnd);
        if (!b_inner)
            b_inner = roundInner(center, ellipseRadius, deg, startAngle, endAngle,
                                 RoundedCornerIndex::InnerBegin);
    }
}

bool QQuickEllipseShapePrivate::roundOuter(QVector2D center, QVector2D ellipseRadius, qreal deg,
                                           qreal arcAngle1, qreal arcAngle2,
                                           RoundedCornerIndex index)
{
    bool done = false;

    const qreal arcAngle = arc_angle(arcAngle1);
    const QVector2D arcPoint = arc_point(center, ellipseRadius, arcAngle);

    const qreal angle = arcAngle1 > arcAngle2 ? arcAngle - deg : arcAngle + deg;

    const QVector2D B = arc_point(center, ellipseRadius, angle); // point on arc

    // calculate tangent vector
    const QVector2D uV = tangent_ccw(ellipseRadius, angle).normalized();
    const QVector2D b1 = B + uV;

    qreal s = 0, t = 0;
    bool res = lines_intersect(center, arcPoint, B, b1, &s, &t);
    if (res) {
        const QVector2D A = center + s * (arcPoint - center);

        const QVector2D AB = B - A;
        const QVector2D AC = center - A;

        const qreal a = angle_between_vectors(AB, AC);
        const qreal halfAngle = std::fabs(a) * 0.5;
        const qreal edgeOffset = AB.length();

        const qreal corner_radius = edgeOffset * qTan(halfAngle);

        // constrain by sweep
        const qreal sweep = std::fabs(arcAngle2 - arcAngle1) * 0.5;
        const qreal degMax = std::min(sweep, 45.0);

        const QVector2D C = A + AC.normalized() * edgeOffset;

        const qreal ptoc = (arcPoint - C).length();
        qreal edge = 0;
        if (innerArcRatio > 0) {
            const QVector2D ellipseInnerRadius(innerArcRatio * ellipseRadius.x(),
                                               innerArcRatio * ellipseRadius.y());
            const QVector2D innerArcPoint =
                    arc_point(center, ellipseInnerRadius, arcAngle); // point on the inner arc
            edge = (arcPoint - innerArcPoint).length();
        } else {
            edge = (arcPoint - center).length();
        }

        const qreal diff = std::fabs(corner_radius - cornerRadius); // closest to target radius

        auto &rc = roundedCorners[index];

        bool canUpdate = diff < rc.diff && !(corner_radius > cornerRadius) && !(deg > degMax)
                && !(rc.radius > corner_radius) && !(ptoc > edge * 0.5f);

        // 1st loop or if constraints are met
        if (rc.radius == 0 || canUpdate)
            rc.update(diff, A, B, C, a, corner_radius);

        done =
                // corner radius is bigger or equal to the target radius
                corner_radius > cornerRadius
                || is_equal(corner_radius, cornerRadius, 0.01f)
                // angle used to define point B is bigger than available sweep
                || deg > degMax
                || is_equal(deg, degMax, 0.01f)
                // the corner radius starts to decline (from previously calculated)
                || (rc.radius != 0 && rc.radius > corner_radius)
                // point C is beyond the half of the ellipse edge
                // (closer to the ellipse center or inner arc point)
                || (ptoc > edge * 0.5f);
    }

    return done;
}

bool QQuickEllipseShapePrivate::roundInner(QVector2D center, QVector2D ellipseRadius, qreal deg,
                                           qreal arcAngle1, qreal arcAngle2,
                                           RoundedCornerIndex index)
{
    // make rounding corner bigger and produces smoother result
    const qreal smoothFactor = 1.5;

    deg *= smoothFactor;

    bool done = false;

    const QVector2D ellipseInnerRadius(innerArcRatio * ellipseRadius.x(),
                                       innerArcRatio * ellipseRadius.y());

    const qreal arcAngle = arc_angle(arcAngle1);
    const QVector2D innerArcPoint =
            arc_point(center, ellipseInnerRadius, arcAngle); // point on the inner arc

    const qreal angle = arcAngle1 > arcAngle2 ? arcAngle - deg : arcAngle + deg;

    const QVector2D B = arc_point(center, ellipseInnerRadius, angle); // point on arc

    // calculate tangent vector
    const QVector2D uV = tangent_ccw(ellipseInnerRadius, angle).normalized();
    const QVector2D b1 = B + uV;

    qreal s = 0, t = 0;
    bool res = lines_intersect(center, innerArcPoint, B, b1, &s, &t);

    if (res) {
        // hit point
        const QVector2D A = center + s * (innerArcPoint - center); // point on edge

        const auto arcPoint = arc_point(center, ellipseRadius, arcAngle);

        const QVector2D AB = B - A;
        const QVector2D AC = A - innerArcPoint;

        const qreal a = angle_between_vectors(AB, AC);
        const qreal halfAngle = std::fabs(a) * 0.5;
        const qreal edgeOffset = AB.length();

        const qreal corner_radius = edgeOffset * qTan(halfAngle);

        // constrain by sweep
        const qreal sweep = std::fabs(arcAngle2 - arcAngle1) * 0.5;
        const qreal degMax = std::min(sweep, 45.0);

        const QVector2D C = A + AC.normalized() * edgeOffset;

        const qreal ptoc = (innerArcPoint - C).length();
        const qreal edge = (innerArcPoint - arcPoint).length();

        const qreal diff =
                std::fabs(corner_radius - cornerRadius * smoothFactor); // closest to target radius

        auto &rc = roundedCorners[index];

        bool canUpdate = diff < rc.diff && !(corner_radius > cornerRadius * smoothFactor)
                && !(deg > degMax) && !(rc.radius > corner_radius) && !(ptoc > edge * 0.5f);

        // 1st loop or if constraints are met
        if (rc.radius == 0 || canUpdate)
            rc.update(diff, A, B, C, a, corner_radius);

        done =
                // corner radius is bigger or equal to the target radius
                corner_radius > cornerRadius * smoothFactor
                || is_equal(corner_radius, cornerRadius * smoothFactor, 0.01f)
                // angle used to define point B is bigger than available sweep
                || deg > degMax
                || is_equal(deg, degMax, 0.01f)
                // the corner radius starts to decline (from previously calculated)
                || (rc.radius != 0 && rc.radius > corner_radius)
                // point C is beyond the half of the ellipse edge
                // (closer to the inner arc end point)
                || (ptoc > edge * 0.5f);
    }

    return done;
}

void QQuickEllipseShapePrivate::drawCenterCorner()
{
    auto &rc = roundedCorners[RoundedCornerIndex::Center];
    path->setStartX(rc.B.x());
    path->setStartY(rc.B.y());

    addArc(rc.C, QVector2D(rc.radius, rc.radius),
           rc.alpha < 0 ? QQuickPathArc::Clockwise : QQuickPathArc::Counterclockwise);
}

void QQuickEllipseShapePrivate::drawInnerEndCorner()
{
    auto &rc = roundedCorners[RoundedCornerIndex::InnerEnd];

    addLine(rc.C);

    addArc(rc.B, QVector2D(rc.radius, rc.radius),
           rc.alpha > 0 ? QQuickPathArc::Clockwise : QQuickPathArc::Counterclockwise);
}

void QQuickEllipseShapePrivate::drawInnerBeginCorner()
{
    auto &rc = roundedCorners[RoundedCornerIndex::InnerBegin];
    path->setStartX(rc.B.x());
    path->setStartY(rc.B.y());

    addArc(rc.C, QVector2D(rc.radius, rc.radius),
           rc.alpha < 0 ? QQuickPathArc::Clockwise : QQuickPathArc::Counterclockwise);
}

void QQuickEllipseShapePrivate::drawOuterBeginCorner()
{
    auto &rc = roundedCorners[RoundedCornerIndex::OuterBegin];

    addLine(rc.C);

    addArc(rc.B, QVector2D(rc.radius, rc.radius),
           rc.alpha > 0 ? QQuickPathArc::Clockwise : QQuickPathArc::Counterclockwise);
}

void QQuickEllipseShapePrivate::drawOuterArcRounded(QVector2D center, QVector2D ellipseRadius)
{
    // split outer arc in two parts to avoid issues of the large arc
    const qreal endAngle = startAngle + sweepAngle;

    const qreal angle = startAngle > endAngle
            ? arc_angle(startAngle - std::fabs(sweepAngle * 0.5f))
            : arc_angle(startAngle + std::fabs(sweepAngle * 0.5f));
    const auto point = arc_point(center, ellipseRadius, angle); // mid point of the arc

    // from begin to mid point
    addArc(point, ellipseRadius,
           sweepAngle > 0.0f ? QQuickPathArc::Clockwise : QQuickPathArc::Counterclockwise);

    auto &rc = roundedCorners[RoundedCornerIndex::OuterEnd];

    // from mid point to end rounded corner
    addArc(rc.B, ellipseRadius,
           sweepAngle > 0.0f ? QQuickPathArc::Clockwise : QQuickPathArc::Counterclockwise);

    // rounded corner
    addArc(rc.C, QVector2D(rc.radius, rc.radius),
           rc.alpha < 0 ? QQuickPathArc::Clockwise : QQuickPathArc::Counterclockwise);
}

void QQuickEllipseShapePrivate::drawInnerArcRounded(QVector2D center, QVector2D ellipseRadius)
{
    // split inner arc in two parts to avoid issues of the large arc
    const qreal endAngle = startAngle + sweepAngle;

    const QVector2D ellipseInnerRadius(innerArcRatio * ellipseRadius.x(),
                                       innerArcRatio * ellipseRadius.y());

    const qreal angle = endAngle > startAngle ? arc_angle(endAngle - std::fabs(sweepAngle * 0.5f))
                                              : arc_angle(endAngle + std::fabs(sweepAngle * 0.5f));
    const auto point = arc_point(center, ellipseInnerRadius, angle); // mid point of the arc

    // from end to mid point
    addArc(point, ellipseInnerRadius,
           sweepAngle > 0.0f ? QQuickPathArc::Counterclockwise : QQuickPathArc::Clockwise);

    // from mid point to begin rounded corner
    auto &rc = roundedCorners[RoundedCornerIndex::InnerBegin];
    addArc(rc.B, ellipseInnerRadius,
           sweepAngle > 0.0f ? QQuickPathArc::Counterclockwise : QQuickPathArc::Clockwise);
}

void QQuickEllipseShapePrivate::drawOuterArc(QVector2D center, QVector2D ellipseRadius)
{
    const qreal beginAngle = arc_angle(startAngle);
    const qreal endAngle = arc_angle(startAngle + sweepAngle);

    const qreal alpha = std::clamp(std::fabs(sweepAngle), 0.0, 359.9);
    bool isFull = (alpha <= 0.0f || alpha >= 359.0f);

    // QQuickPathArc has some weird behavior when it starts and ends at the same point
    // leave some gap between the start and the end points in order to avoid it
    const auto beginPoint = arc_point(center, ellipseRadius, isFull ? 0 : beginAngle);
    const auto endPoint = arc_point(center, ellipseRadius, isFull ? 359.9 : endAngle);

    path->setStartX(beginPoint.x());
    path->setStartY(beginPoint.y());

    addArc(endPoint, ellipseRadius,
           isFull ? QQuickPathArc::Clockwise
                  : (sweepAngle > 0.0f ? QQuickPathArc::Clockwise
                                       : QQuickPathArc::Counterclockwise),
           isFull ? true : alpha > 180.0f);

    // add reverse arc to hide fill color
    if (qFuzzyCompare(innerArcRatio, 1)) {
        addArc(beginPoint, ellipseRadius,
               isFull ? QQuickPathArc::Counterclockwise
                      : (sweepAngle > 0.0f ? QQuickPathArc::Counterclockwise
                                           : QQuickPathArc::Clockwise),
               isFull ? true : alpha > 180.0f);
    }
}

void QQuickEllipseShapePrivate::drawFullInnerArc(QVector2D center, QVector2D ellipseRadius)
{
    const qreal beginAngle = arc_angle(startAngle);

    auto arc = new QQuickPathAngleArc(path);
    arc->setCenterX(center.x());
    arc->setCenterY(center.y());
    arc->setStartAngle(beginAngle);
    arc->setRadiusX(innerArcRatio * ellipseRadius.x());
    arc->setRadiusY(innerArcRatio * ellipseRadius.y());
    arc->setSweepAngle(sweepAngle);
    QQuickPathPrivate::get(path)->appendPathElement(arc);
}

void QQuickEllipseShapePrivate::drawWithInnerRadius(QVector2D center, QVector2D ellipseRadius)
{
    drawInnerBeginCorner(); // path starts at the begin rounded corner on the inner arc

    drawOuterBeginCorner(); // path continues to the begin rounded corner on the outer arc

    // outer arc connecting begin and end rounded corners
    drawOuterArcRounded(center, ellipseRadius);

    // path continues to the end rounded corner on the inner arc
    drawInnerEndCorner();

    // inner arc connecting end and begin rounded corners
    drawInnerArcRounded(center, ellipseRadius);
}

void QQuickEllipseShapePrivate::drawWithoutInnerRadius(QVector2D center, QVector2D ellipseRadius)
{
    drawCenterCorner(); // path starts at rounded corner of ellipse center

    drawOuterBeginCorner(); // path continues to the begin rounded corner on the outer arc

    // outer arc connecting begin and end rounded corners
    drawOuterArcRounded(center, ellipseRadius);

    // path ends at the ellipse's center rounded corner
    const auto &rc = roundedCorners[RoundedCornerIndex::Center];
    addLine(rc.B);
}

void QQuickEllipseShapePrivate::updatePath()
{
    const qreal borderOffset = getBorderOffset();
    const QVector2D center =
            QVector2D(width.valueBypassingBindings(), height.valueBypassingBindings()) * 0.5f;
    const QVector2D ellipseRadius = center - QVector2D(borderOffset, borderOffset);

    QQuickPathPrivate::get(path)->clearPathElements(QQuickPathPrivate::DeleteElementPolicy::Delete);

    const qreal alpha = std::clamp(std::fabs(sweepAngle), 0.0, 359.9);
    const bool isFull = alpha >= 359.0;

    if (qFuzzyCompare(alpha, 0))
        return;

    // just an arc
    if (qFuzzyCompare(innerArcRatio, 1)) {
        drawOuterArc(center, ellipseRadius);
        return;
    }

    roundedCorners.reset(); // cleanup old results

    if (innerArcRatio != 0 && isFull) {
        // this is a donut
        drawOuterArc(center, ellipseRadius);
        drawFullInnerArc(center, ellipseRadius);
    } else if (innerArcRatio != 0 && !isFull) {
        // this is an outlined arc
        roundBeginEnd(center, ellipseRadius);
        drawWithInnerRadius(center, ellipseRadius);
    } else if (!isFull) {
        // this is a pie
        roundCenter(center, ellipseRadius);
        roundBeginEnd(center, ellipseRadius);
        drawWithoutInnerRadius(center, ellipseRadius);
    } else {
        drawOuterArc(center, ellipseRadius);
    }
}

/*!
    \qmltype EllipseShape
    \inqmlmodule QtQuick.Shapes.DesignHelpers
    \brief A shape component that can render an ellipse, an arc, or a pie slice.
    \since QtQuick 6.10

    The EllipseShape item paints an ellipse, which can be customized to appear
    as a full ellipse, an arc, or a filled pie slice. Its appearance is
    controlled by the \l startAngle and \l sweepAngle properties.

    \section1 Basic Ellipse
    By default, the item renders a full ellipse. The interior is filled with the
    \l fillColor, and the outline is drawn according to the \l strokeColor, \l
    strokeWidth, and \l strokeStyle properties.

    \section1 Arc and Pie Slices
    To create an arc or a pie slice, set the \l startAngle (0-360 degrees) and
    \l sweepAngle (0-360 degrees) to define the segment of the ellipse to draw.

    \b {Arc Mode}: To create a simple arc (just the outline), set the \l
    fillColor to \c "transparent". The arc's line style can be customized with
    \l dashPattern and \l dashOffset.

    \b {Pie Mode}: To create a filled pie slice (a segment connected to the
    center), simply set the \l fillColor. The outline of the slice will also be
    stroked.

    \b {Donut Mode}: To create a donut ring (a hollow ellipse), set the
    \l innerArcRatio to a value between 0.0 and 1.0. This defines the ratio of
    the inner ellipse's radius to the outer ellipse's radius.

    The area inside the stroke is painted using either a solid fill color,
    specified using the \l fillColor property, or a gradient, defined using one
    of the \l ShapeGradient subtypes and set using the \l fillGradient
    property. If both a color and a gradient are specified, the gradient is
    used.

    An optional border can be added to an ellipse with its own color and
    thickness by setting the \l strokeColor and \l strokeWidth properties.
    Setting the color to \c transparent creates a border without a fill color.

    Ellipse can be drawn with rounded corners using the \l cornerRadius
    property. The default value of the \l cornerRadius is 10 degrees.

    EllipseShape's default value for \l {QtQuick.Shapes::Shape::preferredRendererType} is
    \c Shape.CurveRenderer.

    \section1 Example Usage

    \snippet ellipseshape.qml ellipseShape

    \image path-ellipseshape.png
*/
QQuickEllipseShape::QQuickEllipseShape(QQuickItem *parent)
    : QQuickShape(*(new QQuickEllipseShapePrivate), parent)
{
    Q_D(QQuickEllipseShape);

    setPreferredRendererType(CurveRenderer);

    setWidth(200);
    setHeight(200);

    d->path = new QQuickShapePath(this);
    d->path->setParent(this);
    d->path->setAsynchronous(true);
    d->path->setStrokeWidth(1);
    d->path->setStrokeColor(QColorConstants::Black);
    d->path->setFillColor(QColorConstants::White);

    d->sp.append(d->path);
    d->path->setParent(this);
    d->extra.value().resourcesList.append(d->path);
}

QQuickEllipseShape::~QQuickEllipseShape() = default;

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::EllipseShape::sweepAngle

    The angular extent in degrees to be drawn from the \l startAngle.

    If set to positive value, the arc is drawn in clockwise direction.
    If set to negative value, the arc is drawn in counter-clockwise direction.

    The default value is \c 360.
*/
qreal QQuickEllipseShape::sweepAngle() const
{
    Q_D(const QQuickEllipseShape);
    return d->sweepAngle;
}

void QQuickEllipseShape::setSweepAngle(qreal sweepAngle)
{
    Q_D(QQuickEllipseShape);
    if (qFuzzyCompare(d->sweepAngle, sweepAngle))
        return;
    d->sweepAngle = sweepAngle;
    d->updatePath();
    emit sweepAngleChanged();
}

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::EllipseShape::startAngle

    The property defines the starting angle in degrees from which to begin
    drawing the ellipse.

    0 degrees points to the top. Angle increases in clockwise direction.

    The default value is \c 0.
*/
qreal QQuickEllipseShape::startAngle() const
{
    Q_D(const QQuickEllipseShape);
    return d->startAngle;
}

void QQuickEllipseShape::setStartAngle(qreal startAngle)
{
    Q_D(QQuickEllipseShape);
    if (qFuzzyCompare(d->startAngle, startAngle))
        return;
    d->startAngle = startAngle;
    d->updatePath();
    emit startAngleChanged();
}

/*!
    \include shapepath.qdocinc {dashOffset-property} {QtQuick.Shapes.DesignHelpers::EllipseShape}
*/

qreal QQuickEllipseShape::dashOffset() const
{
    Q_D(const QQuickEllipseShape);
    return d->path->dashOffset();
}

void QQuickEllipseShape::setDashOffset(qreal offset)
{
    Q_D(QQuickEllipseShape);
    if (qFuzzyCompare(d->path->dashOffset(), offset))
        return;
    d->path->setDashOffset(offset);
    d->updatePath();
    emit dashOffsetChanged();
}

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::EllipseShape::cornerRadius

    Controls the rounding of corners where the radial lines meet the elliptical
    arcs. For pie segments, this rounds the connection to the outer arc. For
    donut segments, this also rounds the connections to both inner and outer arcs.

    The default value is \c 10.
*/
qreal QQuickEllipseShape::cornerRadius() const
{
    Q_D(const QQuickEllipseShape);
    return d->cornerRadius;
}

void QQuickEllipseShape::setCornerRadius(qreal cornerRadius)
{
    Q_D(QQuickEllipseShape);
    if (qFuzzyCompare(d->cornerRadius, cornerRadius))
        return;
    d->cornerRadius = cornerRadius;
    d->updatePath();
    emit cornerRadiusChanged();
}

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::EllipseShape::innerArcRatio

    This property defines the ratio between the inner and outer arcs.

    Value range is between 0.0 and 1.0. Setting the value to 0.0 will cause
    the inner arc to collapse toward the center, drawing a solid filled
    ellipse. Setting the value to 1.0 makes the inner arc the same size as the
    outer ellipse, resulting in just an arc. Values between 0.0 and 1.0 create
    hollow elliptical rings.

    The default value is \c 0.
*/
qreal QQuickEllipseShape::innerArcRatio() const
{
    Q_D(const QQuickEllipseShape);
    return d->innerArcRatio;
}

void QQuickEllipseShape::setInnerArcRatio(qreal innerArcRatio)
{
    Q_D(QQuickEllipseShape);
    if (qFuzzyCompare(d->innerArcRatio, innerArcRatio))
        return;
    d->innerArcRatio = innerArcRatio;
    d->updatePath();
    emit innerArcRatioChanged();
}

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::EllipseShape::strokeWidth

    This property holds the stroke width.

    When set to a negative value, no stroking occurs.

    The default value is \c 1.
*/

qreal QQuickEllipseShape::strokeWidth() const
{
    Q_D(const QQuickEllipseShape);
    return d->path->strokeWidth();
}

void QQuickEllipseShape::setStrokeWidth(qreal width)
{
    Q_D(QQuickEllipseShape);
    if (qFuzzyCompare(d->path->strokeWidth(), width))
        return;
    d->path->setStrokeWidth(width);
    d->updatePath();
    emit strokeWidthChanged();
}

/*!
    \qmlproperty color QtQuick.Shapes.DesignHelpers::EllipseShape::fillColor

    This property holds the fill color.

    When set to \c transparent, no filling occurs.

    The default value is \c "white".

    \note If either \l fillGradient is set to something other than \c null, it
    will be used instead of \c fillColor.
*/

QColor QQuickEllipseShape::fillColor() const
{
    Q_D(const QQuickEllipseShape);
    return d->path->fillColor();
}

void QQuickEllipseShape::setFillColor(const QColor &color)
{
    Q_D(QQuickEllipseShape);
    d->path->setFillColor(color);
    d->updatePath();
    emit fillColorChanged();
}

/*!
    \qmlproperty color QtQuick.Shapes.DesignHelpers::EllipseShape::strokeColor

    This property holds the stroking color.

    When set to \c transparent, no stroking occurs.

    The default value is \c "black".
*/

QColor QQuickEllipseShape::strokeColor() const
{
    Q_D(const QQuickEllipseShape);
    return d->path->strokeColor();
}

void QQuickEllipseShape::setStrokeColor(const QColor &color)
{
    Q_D(QQuickEllipseShape);
    d->path->setStrokeColor(color);
    d->updatePath();
    emit strokeColorChanged();
}

/*!
    \include shapepath.qdocinc {capStyle-property} {QtQuick.Shapes.DesignHelpers::EllipseShape}
*/

QQuickShapePath::CapStyle QQuickEllipseShape::capStyle() const
{
    Q_D(const QQuickEllipseShape);
    return d->path->capStyle();
}

void QQuickEllipseShape::setCapStyle(QQuickShapePath::CapStyle style)
{
    Q_D(QQuickEllipseShape);
    if (d->path->capStyle() == style)
        return;
    d->path->setCapStyle(style);
    d->updatePath();
    emit capStyleChanged();
}

/*!
    \include shapepath.qdocinc {joinStyle-property} {QtQuick.Shapes.DesignHelpers::EllipseShape}
*/

QQuickShapePath::JoinStyle QQuickEllipseShape::joinStyle() const
{
    Q_D(const QQuickEllipseShape);
    return d->path->joinStyle();
}

void QQuickEllipseShape::setJoinStyle(QQuickShapePath::JoinStyle style)
{
    Q_D(QQuickEllipseShape);
    if (d->path->joinStyle() == style)
        return;
    d->path->setJoinStyle(style);
    d->updatePath();
    emit joinStyleChanged();
}

/*!
    \include shapepath.qdocinc {strokeStyle-property} {QtQuick.Shapes.DesignHelpers::EllipseShape}
*/

QQuickShapePath::StrokeStyle QQuickEllipseShape::strokeStyle() const
{
    Q_D(const QQuickEllipseShape);
    return d->path->strokeStyle();
}

void QQuickEllipseShape::setStrokeStyle(QQuickShapePath::StrokeStyle style)
{
    Q_D(QQuickEllipseShape);
    if (d->path->strokeStyle() == style)
        return;
    d->path->setStrokeStyle(style);
    d->updatePath();
    emit strokeStyleChanged();
}

/*!
    \include shapepath.qdocinc {fillRule-property} {QtQuick.Shapes.DesignHelpers::EllipseShape}
*/

QQuickShapePath::FillRule QQuickEllipseShape::fillRule() const
{
    Q_D(const QQuickEllipseShape);
    return d->path->fillRule();
}

void QQuickEllipseShape::setFillRule(QQuickShapePath::FillRule fillRule)
{
    Q_D(QQuickEllipseShape);
    if (d->path->fillRule() == fillRule)
        return;
    d->path->setFillRule(fillRule);
    d->updatePath();
    emit fillRuleChanged();
}

/*!
    \include shapepath.qdocinc {dashPattern-property} {QtQuick.Shapes.DesignHelpers::EllipseShape}
*/

QList<qreal> QQuickEllipseShape::dashPattern() const
{
    Q_D(const QQuickEllipseShape);
    return d->path->dashPattern();
}

void QQuickEllipseShape::setDashPattern(const QList<qreal> &array)
{
    Q_D(QQuickEllipseShape);
    d->path->setDashPattern(array);
    d->updatePath();
    emit dashPatternChanged();
}

/*!
    \qmlproperty ShapeGradient QtQuick.Shapes.DesignHelpers::EllipseShape::fillGradient

    The fillGradient of the ellipse fill color.

    By default, no fillGradient is enabled and the value is null. In this case, the
    fill uses a solid color based on the value of \l fillColor.

    When set, \l fillColor is ignored and filling is done using one of the
    \l ShapeGradient subtypes.

    \note The \l Gradient type cannot be used here. Rather, prefer using one of
    the advanced subtypes, like \l LinearGradient.
*/
QQuickShapeGradient *QQuickEllipseShape::fillGradient() const
{
    Q_D(const QQuickEllipseShape);
    return d->path->fillGradient();
}

void QQuickEllipseShape::setFillGradient(QQuickShapeGradient *fillGradient)
{
    Q_D(QQuickEllipseShape);
    d->path->setFillGradient(fillGradient);
    d->updatePath();
    emit gradientChanged();
}

void QQuickEllipseShape::resetFillGradient()
{
    setFillGradient(nullptr);
}

/*!
    \qmlproperty enumeration QtQuick.Shapes.DesignHelpers::EllipseShape::borderMode

    The \l borderMode property determines where the border is drawn along the
    edge of the ellipse.

    \value EllipseShape.Inside
        The border is drawn along the inside edge of the item and does not
        affect the item width.

        This is the default value.
    \value EllipseShape.Middle
        The border is drawn over the edge of the item and does not
        affect the item width.
    \value EllipseShape.Outside
        The border is drawn along the outside edge of the item and increases
        the item width by the value of \l strokeWidth.

    \sa strokeWidth
*/
QQuickEllipseShape::BorderMode QQuickEllipseShape::borderMode() const
{
    Q_D(const QQuickEllipseShape);
    return d->borderMode;
}

void QQuickEllipseShape::setBorderMode(BorderMode borderMode)
{
    Q_D(QQuickEllipseShape);
    if (borderMode == d->borderMode)
        return;
    d->borderMode = borderMode;
    d->updatePath();
    emit borderModeChanged();
}

void QQuickEllipseShape::resetBorderMode()
{
    setBorderMode(BorderMode::Inside);
}

void QQuickEllipseShape::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickEllipseShape);

    if (d->path)
        d->updatePath();

    QQuickItem::itemChange(change, value);
}

QT_END_NAMESPACE

#include "moc_qquickellipseshape_p.cpp"
