// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickregularpolygonshape_p.h"
#include "qquickregularpolygonshape_p_p.h"
#include <algorithm>
#include <QtMath>
#include <cstddef>

QT_BEGIN_NAMESPACE

QQuickRegularPolygonShapePrivate::QQuickRegularPolygonShapePrivate() = default;

QQuickRegularPolygonShapePrivate::~QQuickRegularPolygonShapePrivate() = default;

namespace {

inline int wrapIndex(int index, int size)
{
    return (index + size) % size;
}

qreal intersect(QVector2D p, QVector2D dir1, QVector2D q, QVector2D dir2)
{
    const auto r = dir1.normalized();
    const auto s = dir2.normalized();

    const QVector2D pq(q.x() - p.x(), q.y() - p.y());
    const QVector2D snv(s.y(), -s.x());

    return QVector2D::dotProduct(pq, snv) / QVector2D::dotProduct(r, snv);
}

} // namespace

void QQuickRegularPolygonShapePrivate::updatePoints()
{
    points.clear();

    const qreal sliceAngle = 360.0f / sideCount;
    const qreal a = width.valueBypassingBindings() / 2.0f; // x-radius
    const qreal b = height.valueBypassingBindings() / 2.0f; // y-radius
    const QVector2D center(a, b);

    for (int i = 0; i < sideCount; ++i) {
        const qreal angleToCorner = qDegreesToRadians(i * sliceAngle);

        // Start at top center
        const QVector2D xy = center + QVector2D(a * qSin(angleToCorner), -b * qCos(angleToCorner));

        points.push_back(std::move(xy));
    }
}

void QQuickRegularPolygonShapePrivate::updateBisectors()
{
    const auto size = points.size();

    bisectors.clear();

    // A list of vectors that are the bisectors of the inner angles of the polygon.
    // This is used to calculate the intersection point of neighboring bisectors for a corner.
    // The minimum length of the two neighboring corner bisectors intersection point is the
    // maximum for the center of the circle that make up the corner radius.
    for (size_t i = 0; i < size; ++i) {
        const auto &a = points[wrapIndex(i, size)];
        const auto &b = points[wrapIndex(i - 1, size)];
        const auto &c = points[wrapIndex(i + 1, size)];

        const QVector2D vAB(b.x() - a.x(), b.y() - a.y());
        const QVector2D vAC(c.x() - a.x(), c.y() - a.y());
        const auto bisector = (vAB + vAC).normalized();

        bisectors.push_back(std::move(bisector));
    }
}

void QQuickRegularPolygonShapePrivate::constructPolygonPath()
{
    auto *ppath = QQuickShapePathPrivate::get(path);

    // start path
    path->setStartX(points[0].x());
    path->setStartY(points[0].y());

    for (const auto &p : points) {
        auto line = new QQuickPathLine(path);
        line->setX(p.x());
        line->setY(p.y());
        ppath->appendPathElement(line, QQuickPathPrivate::ProcessPathPolicy::DontProcess);
    }

    // close path
    auto line = new QQuickPathLine(path);
    line->setX(points[0].x());
    line->setY(points[0].y());
    ppath->appendPathElement(line, QQuickPathPrivate::ProcessPathPolicy::DontProcess);

    path->processPath();
}

void QQuickRegularPolygonShapePrivate::constructRoundedPolygonPath()
{
    auto *ppath = QQuickShapePathPrivate::get(path);

    updateBisectors();

    const auto size = points.size();
    for (size_t i = 0; i < size; ++i) {
        const auto &a = points[wrapIndex(i, size)];
        const auto &b = points[wrapIndex(i - 1, size)];
        const auto &c = points[wrapIndex(i + 1, size)];
        qreal r = cornerRadius;

        const QVector2D vAB(b.x() - a.x(), b.y() - a.y());
        const QVector2D vAC(c.x() - a.x(), c.y() - a.y());

        // Calculate the intersection points of the two neighboring bisectors
        const qreal tAB =
                intersect(a, bisectors[wrapIndex(i, size)], b, bisectors[wrapIndex(i - 1, size)]);
        const qreal tAC =
                intersect(a, bisectors[wrapIndex(i, size)], c, bisectors[wrapIndex(i + 1, size)]);
        const qreal tMax = std::min(tAB, tAC);

        // Angle between the two vectors AB and AC as radians
        const qreal alpha = qAcos(QVector2D::dotProduct(vAB, vAC) / (vAB.length() * vAC.length()));

        // The maximum radius of the circle that can be drawn at the corner. This is another
        // constraint that Figma uses to calculate the corner radius. The corner radius shouldn't
        // be bigger than half of the distance between the two neighboring corners.
        const qreal maxRadius = std::round(QVector2D(c.x() - b.x(), c.y() - b.y()).length() / 2);
        r = std::min(r, maxRadius);

        // The optimal length of the corner bisector to place the center of the circle.
        const qreal cLength = r / (qSin(alpha / 2));

        // Clamp c to the maximum value found from the intersection points of the bisectors.
        const qreal realC = std::min(cLength, tMax);

        if (realC < cLength)
            r = realC * qSin(alpha / 2);

        const qreal t = qSqrt(qPow(realC, 2) - qPow(r, 2));

        const auto p1 = (vAB.normalized() * t) + QVector2D(a.x(), a.y());
        const auto p2 = (vAC.normalized() * t) + QVector2D(a.x(), a.y());

        if (i == 0) {
            path->setStartX(p1.x());
            path->setStartY(p1.y());
        } else {
            auto line = new QQuickPathLine(path);
            line->setX(p1.x());
            line->setY(p1.y());
            ppath->appendPathElement(line, QQuickPathPrivate::ProcessPathPolicy::DontProcess);
        }

        auto arc = new QQuickPathArc(path);
        arc->setX(p2.x());
        arc->setY(p2.y());
        arc->setRadiusX(r);
        arc->setRadiusY(r);
        ppath->appendPathElement(arc, QQuickPathPrivate::ProcessPathPolicy::DontProcess);
    }

    // Close the polygon
    auto line = new QQuickPathLine(path);
    line->setX(path->startX());
    line->setY(path->startY());
    ppath->appendPathElement(line, QQuickPathPrivate::ProcessPathPolicy::DontProcess);

    path->processPath();
}

void QQuickRegularPolygonShapePrivate::updatePath()
{
    QQuickShapePathPrivate::get(path)->clearPathElements(
            QQuickPathPrivate::DeleteElementPolicy::Delete);

    updatePoints();

    if (qFuzzyCompare(cornerRadius, 0.0))
        constructPolygonPath();
    else
        constructRoundedPolygonPath();
}

/*!
    \qmltype RegularPolygonShape
    \inqmlmodule QtQuick.Shapes.DesignHelpers
    \brief A filled regular polygon with an optional border.
    \since QtQuick 6.10

    A regular polygon can be just a 2D polygon shaped stroke, a filling, or a
    stroke with filling. The \l strokeColor, \l strokeWidth, and \l strokeStyle
    properties specify the appearance of the outline. The \l dashPattern and
    \l dashOffset properties specify the appearance of dashed stroke.

    The area inside the stroke is painted using either a solid fill color,
    specified using the \l fillColor property, or a gradient, defined using
    one of the \l ShapeGradient subtypes and set using the \l gradient
    property. If both a color and a gradient are specified, the gradient is
    used.

    To create a polygon with a stroke, set the \sideCount property between 3 to
    100 and the \l strokeWidth property greater than 0. The \l strokeWidth
    property specifies the width of the polygon stroke. The default \l sideCount
    value is 6 and the default \l strokeWidth value is 4. Setting the
    \l strokeWidth value to a negetive value hides the border.

    The \l cornerRadius property specifies whether the polygon corners are rounded.
*/

QQuickRegularPolygonShape::QQuickRegularPolygonShape(QQuickItem *parent)
    : QQuickShape(*(new QQuickRegularPolygonShapePrivate), parent)
{
    Q_D(QQuickRegularPolygonShape);

    setPreferredRendererType(CurveRenderer);

    setWidth(200);
    setHeight(200);

    d->path = new QQuickShapePath(this);
    d->path->setAsynchronous(true);
    d->path->setStrokeWidth(1);
    d->path->setStrokeColor(QColorConstants::Black);
    d->path->setFillColor(QColorConstants::White);

    d->sp.append(d->path);
    d->path->setParent(this);
    d->extra.value().resourcesList.append(d->path);
}

QQuickRegularPolygonShape::~QQuickRegularPolygonShape() = default;

/*!
    \include shapepath.qdocinc {dashOffset-property}
   {QtQuick.Shapes.DesignHelpers::RegularPolygonShape}
*/

qreal QQuickRegularPolygonShape::dashOffset() const
{
    Q_D(const QQuickRegularPolygonShape);
    return d->path->dashOffset();
}

void QQuickRegularPolygonShape::setDashOffset(qreal offset)
{
    Q_D(QQuickRegularPolygonShape);
    if (qFuzzyCompare(d->path->dashOffset(), offset))
        return;
    d->path->setDashOffset(offset);
    emit dashOffsetChanged();
}

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::RegularPolygonShape::cornerRadius

    The property property specifies whether the polygon corners are rounded.

    The default value is \c 10.
*/

qreal QQuickRegularPolygonShape::cornerRadius() const
{
    Q_D(const QQuickRegularPolygonShape);
    return d->cornerRadius;
}

void QQuickRegularPolygonShape::setCornerRadius(qreal radius)
{
    Q_D(QQuickRegularPolygonShape);
    if (qFuzzyCompare(d->cornerRadius, radius))
        return;
    d->cornerRadius = radius;
    d->updatePath();
    emit cornerRadiusChanged();
}

/*!
    \qmlproperty int QtQuick.Shapes.DesignHelpers::RegularPolygonShape::sideCount

    The number of edges on the regular polygon. The minimum number of edges can
    be 3.

    The default value is \c 6.
*/

int QQuickRegularPolygonShape::sideCount() const
{
    Q_D(const QQuickRegularPolygonShape);
    return d->sideCount;
}

void QQuickRegularPolygonShape::setSideCount(int sideCount)
{
    Q_D(QQuickRegularPolygonShape);
    if (d->sideCount == sideCount)
        return;
    d->sideCount = sideCount;
    d->updatePath();
    emit sideCountChanged();
}

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::RegularPolygonShape::strokeWidth

    This property holds the stroke width.

    When set to a negative value, no stroking occurs.

    The default value is \c 1.
*/

qreal QQuickRegularPolygonShape::strokeWidth() const
{
    Q_D(const QQuickRegularPolygonShape);
    return d->path->strokeWidth();
}

void QQuickRegularPolygonShape::setStrokeWidth(qreal width)
{
    Q_D(QQuickRegularPolygonShape);
    if (qFuzzyCompare(d->path->strokeWidth(), width))
        return;
    d->path->setStrokeWidth(width);
    emit strokeWidthChanged();
}

/*!
    \qmlproperty color QtQuick.Shapes.DesignHelpers::RegularPolygonShape::fillColor

    This property holds the fill color.

    When set to \c transparent, no filling occurs.

    The default value is \c "white".

    \note If either \l fillGradient is set to something other than \c null, it
    will be used instead of \c fillColor.
*/

QColor QQuickRegularPolygonShape::fillColor() const
{
    Q_D(const QQuickRegularPolygonShape);
    return d->path->fillColor();
}

void QQuickRegularPolygonShape::setFillColor(const QColor &color)
{
    Q_D(QQuickRegularPolygonShape);
    d->path->setFillColor(color);
    d->updatePath();
    emit fillColorChanged();
}

/*!
    \qmlproperty color QtQuick.Shapes.DesignHelpers::RegularPolygonShape::strokeColor

    This property holds the stroking color.

    When set to \c transparent, no stroking occurs.

    The default value is \c "black".
*/

QColor QQuickRegularPolygonShape::strokeColor() const
{
    Q_D(const QQuickRegularPolygonShape);
    return d->path->strokeColor();
}

void QQuickRegularPolygonShape::setStrokeColor(const QColor &color)
{
    Q_D(QQuickRegularPolygonShape);
    d->path->setStrokeColor(color);
    emit strokeColorChanged();
}

/*!
    \include shapepath.qdocinc {capStyle-property}
   {QtQuick.Shapes.DesignHelpers::RegularPolygonShape}

   Since a polygon is drawn, the path forms a loop with no line end points.
   Therefore, capStyle is only needed when strokeStyle == ShapePath.DashLine
*/

QQuickShapePath::CapStyle QQuickRegularPolygonShape::capStyle() const
{
    Q_D(const QQuickRegularPolygonShape);
    return d->path->capStyle();
}

void QQuickRegularPolygonShape::setCapStyle(QQuickShapePath::CapStyle style)
{
    Q_D(QQuickRegularPolygonShape);
    if (d->path->capStyle() == style)
        return;
    d->path->setCapStyle(style);
    emit capStyleChanged();
}

/*!
    \include shapepath.qdocinc {joinStyle-property}
   {QtQuick.Shapes.DesignHelpers::RegularPolygonShape}

   The joinStyle is only meaningful if cornerRadius == 0.
*/

QQuickShapePath::JoinStyle QQuickRegularPolygonShape::joinStyle() const
{
    Q_D(const QQuickRegularPolygonShape);
    return d->path->joinStyle();
}

void QQuickRegularPolygonShape::setJoinStyle(QQuickShapePath::JoinStyle style)
{
    Q_D(QQuickRegularPolygonShape);
    if (d->path->joinStyle() == style)
        return;
    d->path->setJoinStyle(style);
    emit joinStyleChanged();
}

/*!
    \include shapepath.qdocinc {strokeStyle-property}
   {QtQuick.Shapes.DesignHelpers::RegularPolygonShape}
*/

QQuickShapePath::StrokeStyle QQuickRegularPolygonShape::strokeStyle() const
{
    Q_D(const QQuickRegularPolygonShape);
    return d->path->strokeStyle();
}

void QQuickRegularPolygonShape::setStrokeStyle(QQuickShapePath::StrokeStyle style)
{
    Q_D(QQuickRegularPolygonShape);
    if (d->path->strokeStyle() == style)
        return;
    d->path->setStrokeStyle(style);
    emit strokeStyleChanged();
}

/*!
    \include shapepath.qdocinc {dashPattern-property}
   {QtQuick.Shapes.DesignHelpers::RegularPolygonShape}
*/

QList<qreal> QQuickRegularPolygonShape::dashPattern() const
{
    Q_D(const QQuickRegularPolygonShape);
    return d->path->dashPattern();
}

void QQuickRegularPolygonShape::setDashPattern(const QList<qreal> &array)
{
    Q_D(QQuickRegularPolygonShape);
    d->path->setDashPattern(array);
    emit dashPatternChanged();
}

/*!
    \qmlproperty ShapeGradient QtQuick.Shapes.DesignHelpers::RegularPolygonShape::fillGradient

    The fillGradient of the polygon fill color.

    By default, no fillGradient is enabled and the value is null. In this case, the
    fill uses a solid color based on the value of \l fillColor.

    When set, \l fillColor is ignored and filling is done using one of the
    \l ShapeGradient subtypes.

    \note The \l Gradient type cannot be used here. Rather, prefer using one of
    the advanced subtypes, like \l LinearGradient.
*/
QQuickShapeGradient *QQuickRegularPolygonShape::fillGradient() const
{
    Q_D(const QQuickRegularPolygonShape);
    return d->path->fillGradient();
}

void QQuickRegularPolygonShape::setFillGradient(QQuickShapeGradient *fillGradient)
{
    Q_D(QQuickRegularPolygonShape);
    d->path->setFillGradient(fillGradient);
    emit gradientChanged();
}

void QQuickRegularPolygonShape::resetFillGradient()
{
    setFillGradient(nullptr);
}

void QQuickRegularPolygonShape::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickRegularPolygonShape);

    if (d->path)
        d->updatePath();

    QQuickItem::itemChange(change, value);
}

QT_END_NAMESPACE

#include "moc_qquickregularpolygonshape_p.cpp"
