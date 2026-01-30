// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstarshape_p.h"
#include "qquickstarshape_p_p.h"
#include <algorithm>
#include <QtMath>
#include <cstddef>

QT_BEGIN_NAMESPACE

namespace {

inline qreal arc_angle(qreal angle)
{
    return angle - 90;
}

inline QVector2D arc_point(QVector2D center, QVector2D radius, qreal angle)
{
    return QVector2D(center.x() + radius.x() * qCos(qDegreesToRadians(angle)),
                     center.y() + radius.y() * qSin(qDegreesToRadians(angle)));
}

inline qreal cross(QVector2D a, QVector2D b)
{
    return a.x() * b.y() - a.y() * b.x();
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

} // namespace

QQuickStarShapePrivate::QQuickStarShapePrivate() = default;

QQuickStarShapePrivate::~QQuickStarShapePrivate() = default;

void QQuickStarShapePrivate::updatePoints()
{
    points.clear();

    const qreal rectWidth = width.valueBypassingBindings();
    const qreal rectHeight = height.valueBypassingBindings();

    const QVector2D center(rectWidth * 0.5, rectHeight * 0.5);
    const QVector2D radius(rectWidth * 0.5, rectHeight * 0.5);
    const QVector2D inner_radius = radius * std::min(std::max(ratio, 0.001), 1.0);

    const int numPoints = pointCount * 2;
    const qreal sliceAngle = (360.0f / numPoints);
    for (int i = 0; i < numPoints; ++i) {
        const qreal angle = i * sliceAngle;
        const auto p = arc_point(center, i % 2 == 0 ? radius : inner_radius, arc_angle(angle));
        points.emplace_back(std::move(p));
    }
}

void QQuickStarShapePrivate::constructPolygonPath()
{
    auto *ppath = QQuickShapePathPrivate::get(path);

    path->setStartX(points[0].x());
    path->setStartY(points[0].y());

    for (const auto &p : points) {
        auto line = new QQuickPathLine(path);
        line->setX(p.x());
        line->setY(p.y());
        ppath->appendPathElement(line, QQuickPathPrivate::ProcessPathPolicy::DontProcess);
    }

    auto line = new QQuickPathLine(path);
    line->setX(points[0].x());
    line->setY(points[0].y());
    ppath->appendPathElement(line, QQuickPathPrivate::ProcessPathPolicy::DontProcess);

    path->processPath();
}

void QQuickStarShapePrivate::constructRoundedPolygonPath()
{
    const auto size = points.size();

    auto *ppath = QQuickShapePathPrivate::get(path);

    for (size_t i = 0; i < size; ++i) {
        const auto &a = points[i];
        const auto &b = points[(i == 0 ? size : i) - 1];
        const auto &c = points[(i + 1 == size) ? 0 : (i + 1)];

        const QVector2D ab = b - a;
        const QVector2D ac = c - a;

        const qreal alpha = angle_between_vectors(ab, ac);
        const qreal halfAngle = std::fabs(alpha) * 0.5;

        qreal corner_radius = cornerRadius;

        auto edgeOffset = corner_radius / qTan(halfAngle);
        const qreal edge = std::min(ab.length(), ac.length());
        if (edgeOffset > edge * 0.5) {
            edgeOffset = edge * 0.5;
            corner_radius = edgeOffset * qTan(halfAngle);
        }

        const auto B = a + ab.normalized() * edgeOffset;
        const auto C = a + ac.normalized() * edgeOffset;

        if (i == 0) {
            path->setStartX(B.x());
            path->setStartY(B.y());
        } else {
            auto line = new QQuickPathLine(path);
            line->setX(B.x());
            line->setY(B.y());
            ppath->appendPathElement(line, QQuickPathPrivate::ProcessPathPolicy::DontProcess);
        }

        auto arc = new QQuickPathArc(path);
        arc->setX(C.x());
        arc->setY(C.y());
        arc->setRadiusX(corner_radius);
        arc->setRadiusY(corner_radius);
        arc->setDirection(alpha > 0 ? QQuickPathArc::ArcDirection::Counterclockwise
                                    : QQuickPathArc::ArcDirection::Clockwise);
        ppath->appendPathElement(arc, QQuickPathPrivate::ProcessPathPolicy::DontProcess);
    }

    auto line = new QQuickPathLine(path);
    line->setX(path->startX());
    line->setY(path->startY());
    ppath->appendPathElement(line, QQuickPathPrivate::ProcessPathPolicy::DontProcess);

    path->processPath();
}

void QQuickStarShapePrivate::updatePath()
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
    \qmltype StarShape
    \inqmlmodule QtQuick.Shapes.DesignHelpers
    \brief A filled star-shaped polygon with an optional border.
    \since QtQuick 6.10

    A star can be a star shaped stroke, a filling, or a stroke with filling.
    The \l strokeColor, \l strokeWidth, and \l strokeStyle properties specify
    the appearance of the outline. The \l dashPattern and \l dashOffset
    properties specify the appearance of dashed stroke.

    Set the \l pointCount property between 3 and 60 to specify the number of
    points of the star. Set the \l ratio between 0.1 and 1 to specify the
    distance of the inner points of the star from the center.

    The area inside the stroke is painted using either a solid fill color, specified using the
    \l fillColor property, or a gradient, defined using one of the \l ShapeGradient subtypes and set
    using the \l gradient property. If both a color and a gradient are specified, the gradient is
    used.

    To create a star with a stroke, set the \l strokeWidth property to a value greater than 0. The
    \l strokeWidth property specifies the width of the polygon stroke. The default \l pointCount
   value is 6 and the default \l strokeWidth value is 4. Setting the \l strokeWidth value to a
   negative value hides the border.

    The \l cornerRadius property specifies whether the star corners are rounded.
*/
QQuickStarShape::QQuickStarShape(QQuickItem *parent)
    : QQuickShape(*(new QQuickStarShapePrivate), parent)
{
    Q_D(QQuickStarShape);

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

QQuickStarShape::~QQuickStarShape() = default;

/*!
    \include shapepath.qdocinc {dashOffset-property}
   {QtQuick.Shapes.DesignHelpers::StarShape}
*/

qreal QQuickStarShape::dashOffset() const
{
    Q_D(const QQuickStarShape);
    return d->path->dashOffset();
}

void QQuickStarShape::setDashOffset(qreal offset)
{
    Q_D(QQuickStarShape);
    if (qFuzzyCompare(d->path->dashOffset(), offset))
        return;
    d->path->setDashOffset(offset);
    emit dashOffsetChanged();
}

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::StarShape::cornerRadius

    The property controls the rounding of both the star's outer points and
    inner points.

    The default value is \c 10.
*/

qreal QQuickStarShape::cornerRadius() const
{
    Q_D(const QQuickStarShape);
    return d->cornerRadius;
}

void QQuickStarShape::setCornerRadius(qreal radius)
{
    Q_D(QQuickStarShape);
    if (qFuzzyCompare(d->cornerRadius, radius))
        return;
    d->cornerRadius = radius;
    d->updatePath();
    emit cornerRadiusChanged();
}

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::StarShape::ratio

    The property defines the distance of the inner points of the star from the
    center.

    The default value is \c 0.5.
*/

qreal QQuickStarShape::ratio() const
{
    Q_D(const QQuickStarShape);
    return d->ratio;
}

void QQuickStarShape::setRatio(qreal ratio)
{
    Q_D(QQuickStarShape);
    if (qFuzzyCompare(d->ratio, ratio))
        return;
    d->ratio = ratio;
    d->updatePath();
    emit ratioChanged();
}

/*!
    \qmlproperty int QtQuick.Shapes.DesignHelpers::StarShape::pointCount

    The property defines the total number of points the star has.

    The default value is \c 6.
*/

int QQuickStarShape::pointCount() const
{
    Q_D(const QQuickStarShape);
    return d->pointCount;
}

void QQuickStarShape::setPointCount(int count)
{
    Q_D(QQuickStarShape);
    if (d->pointCount == count)
        return;
    d->pointCount = count;
    d->updatePath();
    emit pointCountChanged();
}

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::StarShape::strokeWidth

    This property holds the stroke width.

    When set to a negative value, no stroking occurs.

    The default value is \c 1.
*/

qreal QQuickStarShape::strokeWidth() const
{
    Q_D(const QQuickStarShape);
    return d->path->strokeWidth();
}

void QQuickStarShape::setStrokeWidth(qreal width)
{
    Q_D(QQuickStarShape);
    if (qFuzzyCompare(d->path->strokeWidth(), width))
        return;
    d->path->setStrokeWidth(width);
    emit strokeWidthChanged();
}

/*!
    \qmlproperty color QtQuick.Shapes.DesignHelpers::StarShape::fillColor

    This property holds the fill color.

    When set to \c transparent, no filling occurs.

    The default value is \c "white".

    \note If either \l fillGradient is set to something other than \c null, it
    will be used instead of \c fillColor.
*/

QColor QQuickStarShape::fillColor() const
{
    Q_D(const QQuickStarShape);
    return d->path->fillColor();
}

void QQuickStarShape::setFillColor(const QColor &color)
{
    Q_D(QQuickStarShape);
    d->path->setFillColor(color);
    d->updatePath();
    emit fillColorChanged();
}

/*!
    \qmlproperty color QtQuick.Shapes.DesignHelpers::StarShape::strokeColor

    This property holds the stroking color.

    When set to \c transparent, no stroking occurs.

    The default value is \c "black".
*/

QColor QQuickStarShape::strokeColor() const
{
    Q_D(const QQuickStarShape);
    return d->path->strokeColor();
}

void QQuickStarShape::setStrokeColor(const QColor &color)
{
    Q_D(QQuickStarShape);
    d->path->setStrokeColor(color);
    emit strokeColorChanged();
}

/*!
    \include shapepath.qdocinc {capStyle-property}
   {QtQuick.Shapes.DesignHelpers::StarShape}

   Since a star is drawn, the path forms a loop with no line end points.
   Therefore, capStyle is only needed when strokeStyle == ShapePath.DashLine
*/

QQuickShapePath::CapStyle QQuickStarShape::capStyle() const
{
    Q_D(const QQuickStarShape);
    return d->path->capStyle();
}

void QQuickStarShape::setCapStyle(QQuickShapePath::CapStyle style)
{
    Q_D(QQuickStarShape);
    if (d->path->capStyle() == style)
        return;
    d->path->setCapStyle(style);
    emit capStyleChanged();
}

/*!
    \include shapepath.qdocinc {joinStyle-property}
   {QtQuick.Shapes.DesignHelpers::StarShape}

   The joinStyle is only meaningful if cornerRadius == 0.
*/

QQuickShapePath::JoinStyle QQuickStarShape::joinStyle() const
{
    Q_D(const QQuickStarShape);
    return d->path->joinStyle();
}

void QQuickStarShape::setJoinStyle(QQuickShapePath::JoinStyle style)
{
    Q_D(QQuickStarShape);
    if (d->path->joinStyle() == style)
        return;
    d->path->setJoinStyle(style);
    emit joinStyleChanged();
}

/*!
    \include shapepath.qdocinc {strokeStyle-property}
   {QtQuick.Shapes.DesignHelpers::StarShape}
*/

QQuickShapePath::StrokeStyle QQuickStarShape::strokeStyle() const
{
    Q_D(const QQuickStarShape);
    return d->path->strokeStyle();
}

void QQuickStarShape::setStrokeStyle(QQuickShapePath::StrokeStyle style)
{
    Q_D(QQuickStarShape);
    if (d->path->strokeStyle() == style)
        return;
    d->path->setStrokeStyle(style);
    emit strokeStyleChanged();
}

/*!
    \include shapepath.qdocinc {dashPattern-property}
   {QtQuick.Shapes.DesignHelpers::StarShape}
*/

QList<qreal> QQuickStarShape::dashPattern() const
{
    Q_D(const QQuickStarShape);
    return d->path->dashPattern();
}

void QQuickStarShape::setDashPattern(const QList<qreal> &array)
{
    Q_D(QQuickStarShape);
    d->path->setDashPattern(array);
    emit dashPatternChanged();
}

/*!
    \qmlproperty ShapeGradient QtQuick.Shapes.DesignHelpers::StarShape::fillGradient

    The fillGradient of the star fill color.

    By default, no fillGradient is enabled and the value is null. In this case, the
    fill uses a solid color based on the value of \l fillColor.

    When set, \l fillColor is ignored and filling is done using one of the
    \l ShapeGradient subtypes.

    \note The \l Gradient type cannot be used here. Rather, prefer using one of
    the advanced subtypes, like \l LinearGradient.
*/
QQuickShapeGradient *QQuickStarShape::fillGradient() const
{
    Q_D(const QQuickStarShape);
    return d->path->fillGradient();
}

void QQuickStarShape::setFillGradient(QQuickShapeGradient *fillGradient)
{
    Q_D(QQuickStarShape);
    d->path->setFillGradient(fillGradient);
    emit gradientChanged();
}

void QQuickStarShape::resetFillGradient()
{
    setFillGradient(nullptr);
}

void QQuickStarShape::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickStarShape);

    if (d->path)
        d->updatePath();

    QQuickItem::itemChange(change, value);
}

QT_END_NAMESPACE

#include "moc_qquickstarshape_p.cpp"
