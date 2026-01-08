// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickrectangleshape_p.h"
#include "qquickrectangleshape_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QQuickRectangleShape
    \inmodule QtQuickShapes
    \internal
*/

Q_STATIC_LOGGING_CATEGORY(lcCalculateIndependentRadii, "qt.quick.shapes.designhelpers.rectangleshape.calculateindependentradii")
Q_STATIC_LOGGING_CATEGORY(lcUpdatePolish, "qt.quick.shapes.designhelpers.rectangleshape.updatepolish")
Q_STATIC_LOGGING_CATEGORY(lcMaybeUpdateElements, "qt.quick.shapes.designhelpers.rectangleshape.maybeupdateelements")

void QQuickRectangleShapePrivate::setTopLeftRadius(int topLeftRadius,
    QQml::PropertyUtils::State propertyState)
{
    Q_Q(QQuickRectangleShape);
    const int oldEffectiveTopLeftRadius = q->topLeftRadius();
    explicitTopLeftRadius = isExplicitlySet(propertyState);
    this->topLeftRadius = topLeftRadius;

    if (q->topLeftRadius() == oldEffectiveTopLeftRadius)
        return;

    q->polish();
    emit q->topLeftRadiusChanged();
}

void QQuickRectangleShapePrivate::setTopRightRadius(int topRightRadius,
    QQml::PropertyUtils::State propertyState)
{
    Q_Q(QQuickRectangleShape);
    const int oldEffectiveTopRightRadius = q->topRightRadius();
    explicitTopRightRadius = isExplicitlySet(propertyState);
    this->topRightRadius = topRightRadius;

    if (q->topRightRadius() == oldEffectiveTopRightRadius)
        return;

    q->polish();
    emit q->topRightRadiusChanged();
}

void QQuickRectangleShapePrivate::setBottomLeftRadius(int bottomLeftRadius,
    QQml::PropertyUtils::State propertyState)
{
    Q_Q(QQuickRectangleShape);
    const int oldEffectiveBottomLeftRadius = q->bottomLeftRadius();
    explicitBottomLeftRadius = isExplicitlySet(propertyState);
    this->bottomLeftRadius = bottomLeftRadius;

    if (q->bottomLeftRadius() == oldEffectiveBottomLeftRadius)
        return;

    q->polish();
    emit q->bottomLeftRadiusChanged();
}

void QQuickRectangleShapePrivate::setBottomRightRadius(int bottomRightRadius,
    QQml::PropertyUtils::State propertyState)
{
    Q_Q(QQuickRectangleShape);
    const int oldEffectiveBottomRightRadius = q->bottomRightRadius();
    explicitBottomRightRadius = isExplicitlySet(propertyState);
    this->bottomRightRadius = bottomRightRadius;

    if (q->bottomRightRadius() == oldEffectiveBottomRightRadius)
        return;

    q->polish();
    emit q->bottomRightRadiusChanged();
}

void QQuickRectangleShapePrivate::setTopLeftBevel(bool topLeftBevel,
    QQml::PropertyUtils::State propertyState)
{
    Q_Q(QQuickRectangleShape);
    const bool oldTopLeftBevel = q->hasTopLeftBevel();
    explicitTopLeftBevel = isExplicitlySet(propertyState);
    this->topLeftBevel = topLeftBevel;

    if (q->hasTopLeftBevel() == oldTopLeftBevel)
        return;

    q->polish();
    emit q->topLeftBevelChanged();
}

void QQuickRectangleShapePrivate::setTopRightBevel(bool topRightBevel,
    QQml::PropertyUtils::State propertyState)
{
    Q_Q(QQuickRectangleShape);
    const bool oldTopRightBevel = q->hasTopRightBevel();
    explicitTopRightBevel = isExplicitlySet(propertyState);
    this->topRightBevel = topRightBevel;

    if (q->hasTopRightBevel() == oldTopRightBevel)
        return;

    q->polish();
    emit q->topRightBevelChanged();
}

void QQuickRectangleShapePrivate::setBottomLeftBevel(bool bottomLeftBevel,
    QQml::PropertyUtils::State propertyState)
{
    Q_Q(QQuickRectangleShape);
    const bool oldBottomLeftBevel = q->hasBottomLeftBevel();
    explicitBottomLeftBevel = isExplicitlySet(propertyState);
    this->bottomLeftBevel = bottomLeftBevel;

    if (q->hasBottomLeftBevel() == oldBottomLeftBevel)
        return;

    q->polish();
    emit q->bottomLeftBevelChanged();
}

void QQuickRectangleShapePrivate::setBottomRightBevel(bool bottomRightBevel,
    QQml::PropertyUtils::State propertyState)
{
    Q_Q(QQuickRectangleShape);
    const bool oldBottomRightBevel = q->hasBottomRightBevel();
    explicitBottomRightBevel = isExplicitlySet(propertyState);
    this->bottomRightBevel = bottomRightBevel;

    if (q->hasBottomRightBevel() == oldBottomRightBevel)
        return;

    q->polish();
    emit q->bottomRightBevelChanged();
}

template <typename T>
T *createElement(QQuickShapePath *shapePath, const char *objectName)
{
    auto *element = new T(shapePath);
    element->setObjectName(objectName);
    return element;
}

/*!
    \internal

    Constructs and append the individual path elements to the ShapePath.
*/
void QQuickRectangleShapePrivate::maybeUpdateElements()
{
    if (!componentComplete)
        return;

    auto *shapePathPrivate = QQuickShapePathPrivate::get(shapePath);

    // We need to delete the elements since we create them ourselves.
    qCDebug(lcMaybeUpdateElements).nospace() << "maybeUpdateElements called on "
        // Avoid printing the QQuickItem as it results in assertion failure in QQuickItemLayer::componentComplete.
        << static_cast<void *>(q_func()) << "; deleting and clearing path elements";
    shapePathPrivate->clearPathElements(QQuickPathPrivate::DeleteElementPolicy::Delete);

    static const QQuickPathPrivate::ProcessPathPolicy DontProcessPath
        = QQuickPathPrivate::ProcessPathPolicy::DontProcess;

    // We avoid rendering issues (QDS-14861) by rotating the start position
    // clockwise until the first visible edge after a hidden edge was found.
    // Set the start position (startIndex) to that edge and continue the construction of
    // the border item from there.
    const std::array visibleEdges = {drawTop, drawRight, drawBottom, drawLeft };
    int startIndex = 0;
    for (int currentEdge = static_cast<int>(Edge::Top); currentEdge < static_cast<int>(Edge::NEdges); ++currentEdge) {
        const int previousEdge = (currentEdge + 3) % 4;
        if (!visibleEdges[previousEdge] && visibleEdges[currentEdge]) {
            startIndex = currentEdge;
            break;
        }
    }

    firstVisibleEdge = static_cast<Edge>(startIndex);
    qCDebug(lcMaybeUpdateElements) << "firstVisibleEdge:" << startIndex; // (Can't print the enum itself)

    for (int i = 0; i < 4; i++) {
        const int currentEdge = (startIndex + i) % 4;
        const int nextEdge = (startIndex + i + 1) % 4;

        switch (static_cast<Edge>(currentEdge)) {
        case Edge::Top:
            if (visibleEdges[currentEdge]) {
                topPathLine = createElement<QQuickPathLine>(shapePath, "topPathLine");
                shapePathPrivate->appendPathElement(topPathLine, DontProcessPath);
                topPathMove = nullptr;
            } else {
                topPathLine = nullptr;
                topPathMove = createElement<QQuickPathMove>(shapePath, "topPathMove");
                shapePathPrivate->appendPathElement(topPathMove, DontProcessPath);
            }

            // Top right corner
            if (visibleEdges[currentEdge] && visibleEdges[nextEdge]) {
                topRightPathArc = createElement<QQuickPathArc>(shapePath, "topRightPathArc");
                shapePathPrivate->appendPathElement(topRightPathArc, DontProcessPath);
            } else {
                topRightPathArc = nullptr;
            }
            break;
        case Edge::Right:
            if (visibleEdges[currentEdge]) {
                rightPathLine = createElement<QQuickPathLine>(shapePath, "rightPathLine");
                shapePathPrivate->appendPathElement(rightPathLine, DontProcessPath);
                rightPathMove = nullptr;
            } else {
                rightPathLine = nullptr;
                rightPathMove = createElement<QQuickPathMove>(shapePath, "rightPathMove");
                shapePathPrivate->appendPathElement(rightPathMove, DontProcessPath);
            }

            // Bottom right corner
            if (visibleEdges[currentEdge] && visibleEdges[nextEdge]) {
                bottomRightPathArc = createElement<QQuickPathArc>(shapePath, "bottomRightPathArc");
                shapePathPrivate->appendPathElement(bottomRightPathArc, DontProcessPath);
            } else {
                bottomRightPathArc = nullptr;
            }
            break;
        case Edge::Bottom:
            if (visibleEdges[currentEdge]) {
                bottomPathLine = createElement<QQuickPathLine>(shapePath, "bottomPathLine");
                shapePathPrivate->appendPathElement(bottomPathLine, DontProcessPath);
                bottomPathMove = nullptr;
            } else {
                bottomPathLine = nullptr;
                bottomPathMove = createElement<QQuickPathMove>(shapePath, "bottomPathMove");
                shapePathPrivate->appendPathElement(bottomPathMove, DontProcessPath);
            }

            // Bottom left corner
            if (visibleEdges[currentEdge] && visibleEdges[nextEdge]) {
                bottomLeftPathArc = createElement<QQuickPathArc>(shapePath, "bottomLeftPathArc");
                shapePathPrivate->appendPathElement(bottomLeftPathArc, DontProcessPath);
            } else {
                bottomLeftPathArc = nullptr;
            }
            break;
        case Edge::Left:
            if (visibleEdges[currentEdge]) {
                leftPathLine = createElement<QQuickPathLine>(shapePath, "leftPathLine");
                shapePathPrivate->appendPathElement(leftPathLine, DontProcessPath);
            } else {
                leftPathLine = nullptr;
                // There isn't a leftPathMove because it will only be applicable for the case where
                // there is only a top and bottom edge (the only configuration where a
                // left-path-move could be needed), and if that is the case, it must start with the
                // top edge, and must end at the left edge, so there is never need to move the path
                // at the last (left in this case) edge.
            }

            // Top left corner
            if (visibleEdges[currentEdge] && visibleEdges[nextEdge]) {
                topLeftPathArc = createElement<QQuickPathArc>(shapePath, "topLeftPathArc");
                shapePathPrivate->appendPathElement(topLeftPathArc, DontProcessPath);
            } else {
                topLeftPathArc = nullptr;
            }
            break;
        case Edge::NEdges:
            Q_UNREACHABLE();
        }
    }

    qCDebug(lcMaybeUpdateElements) << "about to process path";
    shapePath->processPath();
    qCDebug(lcMaybeUpdateElements) << "about to call _q_shapePathChanged (i.e. polish and update implicit size)";
    _q_shapePathChanged();
}

void QQuickRectangleShapePrivate::calculateIndependentRadii()
{
    Q_Q(const QQuickRectangleShape);
    const qreal rectWidth = width.valueBypassingBindings();
    const qreal rectHeight = height.valueBypassingBindings();
    const int minDimension = qMin(rectWidth, rectHeight);
    const int maxRadius = minDimension / 2;
    const int topLeftRadius = q->topLeftRadius();
    const int topRightRadius = q->topRightRadius();
    const int bottomRightRadius = q->bottomRightRadius();
    const int bottomLeftRadius = q->bottomLeftRadius();
    const bool mixed = !(radius == topLeftRadius
        && radius == topRightRadius
        && radius == bottomLeftRadius
        && radius == bottomRightRadius);

    // Uniform radii
    if (!mixed) {
        effectiveTopLeftRadius = qMin(topLeftRadius, maxRadius);
        effectiveTopRightRadius = qMin(topRightRadius, maxRadius);
        effectiveBottomRightRadius = qMin(bottomRightRadius, maxRadius);
        effectiveBottomLeftRadius = qMin(bottomLeftRadius, maxRadius);
        qCDebug(lcCalculateIndependentRadii) << "calculateIndependentRadii: using uniform radii of" << radius
            << "width" << rectWidth
            << "height" << rectHeight
            << "minDimension" << minDimension
            << "tlr" << topLeftRadius
            << "etlr" << effectiveTopLeftRadius
            << "trr" << topRightRadius
            << "etrr" << effectiveTopRightRadius
            << "blr" << bottomLeftRadius
            << "eblr" << effectiveBottomLeftRadius
            << "brr" << bottomRightRadius
            << "ebrr" << effectiveBottomRightRadius;
        return;
    }

    // Mixed radii
    qreal topLeftRadiusMin = qMin(minDimension, topLeftRadius);
    qreal topRightRadiusMin = qMin(minDimension, topRightRadius);
    qreal bottomLeftRadiusMin = qMin(minDimension, bottomLeftRadius);
    qreal bottomRightRadiusMin = qMin(minDimension, bottomRightRadius);

    // Top radii
    const qreal topRadii = topLeftRadius + topRightRadius;

    if (topRadii > rectWidth) {
        const qreal topLeftRadiusFactor = topLeftRadius / topRadii;
        const qreal tlr = qRound(rectWidth * topLeftRadiusFactor);

        topLeftRadiusMin = qMin(topLeftRadiusMin, tlr);
        topRightRadiusMin = qMin(topRightRadiusMin, rectWidth - tlr);
    }

    // Right radii
    const qreal rightRadii = topRightRadius + bottomRightRadius;

    if (rightRadii > rectHeight) {
        const qreal topRightRadiusFactor = topRightRadius / rightRadii;
        const qreal trr = qRound(rectHeight * topRightRadiusFactor);

        topRightRadiusMin = qMin(topRightRadiusMin, trr);
        bottomRightRadiusMin = qMin(bottomRightRadiusMin, rectHeight - trr);
    }

    // Bottom radii
    const qreal bottomRadii = bottomRightRadius + bottomLeftRadius;

    if (bottomRadii > rectWidth) {
        const qreal bottomRightRadiusFactor = bottomRightRadius / bottomRadii;
        const qreal brr = qRound(rectWidth * bottomRightRadiusFactor);

        bottomRightRadiusMin = qMin(bottomRightRadiusMin, brr);
        bottomLeftRadiusMin = qMin(bottomLeftRadiusMin, rectWidth - brr);
    }

    // Left radii
    const qreal leftRadii = bottomLeftRadius + topLeftRadius;

    if (leftRadii > rectHeight) {
        const qreal bottomLeftRadiusFactor = bottomLeftRadius / leftRadii;
        const qreal blr = qRound(rectHeight * bottomLeftRadiusFactor);

        bottomLeftRadiusMin = qMin(bottomLeftRadiusMin, blr);
        topLeftRadiusMin = qMin(topLeftRadiusMin, rectHeight - blr);
    }

    effectiveTopLeftRadius = topLeftRadiusMin;
    effectiveTopRightRadius = topRightRadiusMin;
    effectiveBottomLeftRadius = bottomLeftRadiusMin;
    effectiveBottomRightRadius = bottomRightRadiusMin;

    qCDebug(lcCalculateIndependentRadii) << "calculateIndependentRadii:"
        << "width" << rectWidth
        << "height" << rectHeight
        << "borderMode" << borderMode
        << "strokeWidth" << shapePath->strokeWidth()
        << "minDimension" << minDimension
        << "tlr" << topLeftRadius
        << "etlr" << effectiveTopLeftRadius
        << "trr" << topRightRadius
        << "etrr" << effectiveTopRightRadius
        << "blr" << bottomLeftRadius
        << "eblr" << effectiveBottomLeftRadius
        << "brr" << bottomRightRadius
        << "ebrr" << effectiveBottomRightRadius
        << "borderOffset" << borderOffset
        << "startX" << shapePath->startX()
        << "startY" << shapePath->startY();
}

/*!
    \qmltype RectangleShape
    \inqmlmodule QtQuick.Shapes.DesignHelpers
    \brief A filled rectangle with an optional border.
    \since QtQuick 6.10

    RectangleShape is used to fill areas with solid color or gradients and to
    provide a rectangular border.

    Each Rectangle item is painted using either a solid fill color, specified
    using the \l fillColor property, or a gradient, defined using one of the \l
    ShapeGradient subtypes and set using the \l gradient property. If both a
    color and a gradient are specified, the gradient is used.

    An optional border can be added to a rectangle with its own color and
    thickness by setting the \l strokeColor and \l strokeWidth properties.
    Setting the color to \c transparent creates a border without a fill color.

    Rounded rectangles can be drawn using the \l radius property. The radius
    can also be specified separately for each corner. Additionally, \l bevel
    can be applied on any corner to cut it off sharply.

    RectangleShape's default value for \l {QtQuick.Shapes::Shape::preferredRendererType} is
    \c Shape.CurveRenderer.

    \section1 Example Usage

    \snippet rectangleshape-bevel.qml rectangleShape

    \image pathrectangle-bevel.png
*/

QQuickRectangleShape::QQuickRectangleShape(QQuickItem *parent)
    : QQuickShape(*(new QQuickRectangleShapePrivate), parent)
{
    Q_D(QQuickRectangleShape);

    // Create the ShapePath.
    d->shapePath = new QQuickShapePath(this);
    d->shapePath->setObjectName("rectangleShapeShapePath");
    d->shapePath->setStrokeWidth(1);
    d->shapePath->setStrokeColor(QColorConstants::Black);
    d->shapePath->setFillColor(QColorConstants::White);
    d->shapePath->setJoinStyle(QQuickShapePath::BevelJoin);
    // Don't make it asynchronous, as it results in brief periods of incorrect rendering.

    connect(d->shapePath, &QQuickShapePath::strokeColorChanged, this, &QQuickRectangleShape::strokeColorChanged);
    connect(d->shapePath, &QQuickShapePath::strokeWidthChanged, this, &QQuickRectangleShape::strokeWidthChanged);
    connect(d->shapePath, &QQuickShapePath::fillColorChanged, this, &QQuickRectangleShape::fillColorChanged);
    connect(d->shapePath, &QQuickShapePath::joinStyleChanged, this, &QQuickRectangleShape::joinStyleChanged);
    connect(d->shapePath, &QQuickShapePath::capStyleChanged, this, &QQuickRectangleShape::capStyleChanged);
    connect(d->shapePath, &QQuickShapePath::strokeStyleChanged, this, &QQuickRectangleShape::strokeStyleChanged);
    connect(d->shapePath, &QQuickShapePath::dashOffsetChanged, this, &QQuickRectangleShape::dashOffsetChanged);
    connect(d->shapePath, &QQuickShapePath::dashPatternChanged, this, &QQuickRectangleShape::dashPatternChanged);
    // QQuickShapePath has no change signal for fillGradient.

    // Add the path as a child of us.
    // The individual path elements will be added to the shape path in maybeUpdateElements().
    // Do what vpe_append in qquickshape.cpp does except without the overhead of the QQmlListProperty stuff.
    d->sp.append(d->shapePath);
    // Similar, but for QQuickItemPrivate::data_append...
    d->shapePath->setParent(this);
    // ... which calls QQuickItemPrivate::resources_append.
    d->extra.value().resourcesList.append(d->shapePath);

    setWidth(200);
    setHeight(200);
    setPreferredRendererType(CurveRenderer);

    // QQuickShape::componentComplete sets up the connections to each path.
    // It also calls _q_shapePathChanged, which will call polish (for our updatePolish).
}

QQuickRectangleShape::~QQuickRectangleShape()
{
}

/*!
    \since 6.11

    This property holds whether the top border is drawn.

    The default value is \c true.
*/
bool QQuickRectangleShape::drawTop() const
{
    Q_D(const QQuickRectangleShape);
    return d->drawTop;
}

void QQuickRectangleShape::setDrawTop(bool drawTop)
{
    Q_D(QQuickRectangleShape);
    if (drawTop == d->drawTop)
        return;

    d->drawTop = drawTop;
    d->maybeUpdateElements();
    emit drawTopChanged();
}

void QQuickRectangleShape::resetDrawTop()
{
    setDrawTop(QQuickRectangleShapePrivate::defaultDrawEdge);
}

/*!
    \since 6.11

    This property holds whether the right border is drawn.

    The default value is \c true.
*/
bool QQuickRectangleShape::drawRight() const
{
    Q_D(const QQuickRectangleShape);
    return d->drawRight;
}

void QQuickRectangleShape::setDrawRight(bool drawRight)
{
    Q_D(QQuickRectangleShape);
    if (drawRight == d->drawRight)
        return;

    d->drawRight = drawRight;
    d->maybeUpdateElements();
    emit drawRightChanged();
}

void QQuickRectangleShape::resetDrawRight()
{
    setDrawRight(QQuickRectangleShapePrivate::defaultDrawEdge);
}

/*!
    \since 6.11

    This property holds whether the bottom border is drawn.

    The default value is \c true.
*/
bool QQuickRectangleShape::drawBottom() const
{
    Q_D(const QQuickRectangleShape);
    return d->drawBottom;
}

void QQuickRectangleShape::setDrawBottom(bool drawBottom)
{
    Q_D(QQuickRectangleShape);
    if (drawBottom == d->drawBottom)
        return;

    d->drawBottom = drawBottom;
    d->maybeUpdateElements();
    emit drawBottomChanged();
}

void QQuickRectangleShape::resetDrawBottom()
{
    setDrawBottom(QQuickRectangleShapePrivate::defaultDrawEdge);
}

/*!
    \since 6.11

    This property holds whether the left border is drawn.

    The default value is \c true.
*/
bool QQuickRectangleShape::drawLeft() const
{
    Q_D(const QQuickRectangleShape);
    return d->drawLeft;
}

void QQuickRectangleShape::setDrawLeft(bool drawLeft)
{
    Q_D(QQuickRectangleShape);
    if (drawLeft == d->drawLeft)
        return;

    d->drawLeft = drawLeft;
    d->maybeUpdateElements();
    emit drawLeftChanged();
}

void QQuickRectangleShape::resetDrawLeft()
{
    setDrawLeft(QQuickRectangleShapePrivate::defaultDrawEdge);
}

/*!
    \include pathrectangle.qdocinc {radius-property}
        {QtQuick.Shapes.DesignHelpers::RectangleShape}

    The default value is \c 10.
*/

int QQuickRectangleShape::radius() const
{
    Q_D(const QQuickRectangleShape);
    return d->radius;
}

void QQuickRectangleShape::setRadius(int radius)
{
    Q_D(QQuickRectangleShape);
    if (radius == d->radius)
        return;

    const int oldTopLeftRadius = topLeftRadius();
    const int oldTopRightRadius = topRightRadius();
    const int oldBottomRightRadius = bottomRightRadius();
    const int oldBottomLeftRadius = bottomLeftRadius();

    d->radius = radius;
    polish();
    emit radiusChanged();
    if (topLeftRadius() != oldTopLeftRadius)
        emit topLeftRadiusChanged();
    if (topRightRadius() != oldTopRightRadius)
        emit topRightRadiusChanged();
    if (bottomRightRadius() != oldBottomRightRadius)
        emit bottomRightRadiusChanged();
    if (bottomLeftRadius() != oldBottomLeftRadius)
        emit bottomLeftRadiusChanged();
}

void QQuickRectangleShape::resetRadius()
{
    setRadius(QQuickRectangleShapePrivate::defaultRadius);
}

/*!
    \include pathrectangle.qdocinc {radius-properties}
        {QtQuick.Shapes.DesignHelpers::RectangleShape} {rectangleshape.qml} {rectangleShape}
*/

int QQuickRectangleShape::topLeftRadius() const
{
    Q_D(const QQuickRectangleShape);
    return d->explicitTopLeftRadius ? d->topLeftRadius : d->radius;
}

void QQuickRectangleShape::setTopLeftRadius(int topLeftRadius)
{
    Q_D(QQuickRectangleShape);
    d->setTopLeftRadius(topLeftRadius, QQml::PropertyUtils::State::ExplicitlySet);
}

void QQuickRectangleShape::resetTopLeftRadius()
{
    Q_D(QQuickRectangleShape);
    d->setTopLeftRadius(QQuickRectangleShapePrivate::defaultRadius,
        QQml::PropertyUtils::State::ImplicitlySet);
}

int QQuickRectangleShape::topRightRadius() const
{
    Q_D(const QQuickRectangleShape);
    return d->explicitTopRightRadius ? d->topRightRadius : d->radius;
}

void QQuickRectangleShape::setTopRightRadius(int topRightRadius)
{
    Q_D(QQuickRectangleShape);
    d->setTopRightRadius(topRightRadius, QQml::PropertyUtils::State::ExplicitlySet);
}

void QQuickRectangleShape::resetTopRightRadius()
{
    Q_D(QQuickRectangleShape);
    d->setTopRightRadius(QQuickRectangleShapePrivate::defaultRadius,
        QQml::PropertyUtils::State::ImplicitlySet);
}

int QQuickRectangleShape::bottomLeftRadius() const
{
    Q_D(const QQuickRectangleShape);
    return d->explicitBottomLeftRadius ? d->bottomLeftRadius : d->radius;
}

void QQuickRectangleShape::setBottomLeftRadius(int bottomLeftRadius)
{
    Q_D(QQuickRectangleShape);
    d->setBottomLeftRadius(bottomLeftRadius, QQml::PropertyUtils::State::ExplicitlySet);
}

void QQuickRectangleShape::resetBottomLeftRadius()
{
    Q_D(QQuickRectangleShape);
    d->setBottomLeftRadius(QQuickRectangleShapePrivate::defaultRadius,
        QQml::PropertyUtils::State::ImplicitlySet);
}

int QQuickRectangleShape::bottomRightRadius() const
{
    Q_D(const QQuickRectangleShape);
    return d->explicitBottomRightRadius ? d->bottomRightRadius : d->radius;
}

void QQuickRectangleShape::setBottomRightRadius(int bottomRightRadius)
{
    Q_D(QQuickRectangleShape);
    d->setBottomRightRadius(bottomRightRadius, QQml::PropertyUtils::State::ExplicitlySet);
}

void QQuickRectangleShape::resetBottomRightRadius()
{
    Q_D(QQuickRectangleShape);
    d->setBottomRightRadius(QQuickRectangleShapePrivate::defaultRadius,
        QQml::PropertyUtils::State::ImplicitlySet);
}

/*!
    \include pathrectangle.qdocinc {bevel-property}
        {QtQuick.Shapes.DesignHelpers::RectangleShape}
        {rectangleshape-bevel.qml}{rectangleShape}
*/

bool QQuickRectangleShape::hasBevel() const
{
    Q_D(const QQuickRectangleShape);
    return d->bevel;
}

void QQuickRectangleShape::setBevel(bool bevel)
{
    Q_D(QQuickRectangleShape);
    if (bevel == d->bevel)
        return;

    const bool oldTopLeftBevel = hasTopLeftBevel();
    const bool oldTopRightBevel = hasTopRightBevel();
    const bool oldBottomRightBevel = hasBottomRightBevel();
    const bool oldBottomLeftBevel = hasBottomLeftBevel();

    d->bevel = bevel;
    polish();
    emit bevelChanged();
    if (hasTopLeftBevel() != oldTopLeftBevel)
        emit topLeftBevelChanged();
    if (hasTopRightBevel() != oldTopRightBevel)
        emit topRightBevelChanged();
    if (hasBottomRightBevel() != oldBottomRightBevel)
        emit bottomRightBevelChanged();
    if (hasBottomLeftBevel() != oldBottomLeftBevel)
        emit bottomLeftBevelChanged();
}

void QQuickRectangleShape::resetBevel()
{
    setBevel(false);
}

/*!
    \include pathrectangle.qdocinc {bevel-properties}
        {QtQuick.Shapes.DesignHelpers::RectangleShape}
        {rectangleshape.qml} {rectangleShape}
*/

bool QQuickRectangleShape::hasTopLeftBevel() const
{
    Q_D(const QQuickRectangleShape);
    return d->explicitTopLeftBevel ? d->topLeftBevel : d->bevel;
}

void QQuickRectangleShape::setTopLeftBevel(bool topLeftBevel)
{
    Q_D(QQuickRectangleShape);
    d->setTopLeftBevel(topLeftBevel, QQml::PropertyUtils::State::ExplicitlySet);
}

void QQuickRectangleShape::resetTopLeftBevel()
{
    Q_D(QQuickRectangleShape);
    d->setTopLeftBevel(QQuickRectangleShapePrivate::defaultBevel,
        QQml::PropertyUtils::State::ImplicitlySet);
}

bool QQuickRectangleShape::hasTopRightBevel() const
{
    Q_D(const QQuickRectangleShape);
    return d->explicitTopRightBevel ? d->topRightBevel : d->bevel;
}

void QQuickRectangleShape::setTopRightBevel(bool topRightBevel)
{
    Q_D(QQuickRectangleShape);
    d->setTopRightBevel(topRightBevel, QQml::PropertyUtils::State::ExplicitlySet);
}

void QQuickRectangleShape::resetTopRightBevel()
{
    Q_D(QQuickRectangleShape);
    d->setTopRightBevel(QQuickRectangleShapePrivate::defaultBevel,
        QQml::PropertyUtils::State::ImplicitlySet);
}

bool QQuickRectangleShape::hasBottomLeftBevel() const
{
    Q_D(const QQuickRectangleShape);
    return d->explicitBottomLeftBevel ? d->bottomLeftBevel : d->bevel;
}

void QQuickRectangleShape::setBottomLeftBevel(bool bottomLeftBevel)
{
    Q_D(QQuickRectangleShape);
    d->setBottomLeftBevel(bottomLeftBevel, QQml::PropertyUtils::State::ExplicitlySet);
}

void QQuickRectangleShape::resetBottomLeftBevel()
{
    Q_D(QQuickRectangleShape);
    d->setBottomLeftBevel(QQuickRectangleShapePrivate::defaultBevel,
        QQml::PropertyUtils::State::ImplicitlySet);
}

bool QQuickRectangleShape::hasBottomRightBevel() const
{
    Q_D(const QQuickRectangleShape);
    return d->explicitBottomRightBevel ? d->bottomRightBevel : d->bevel;
}

void QQuickRectangleShape::setBottomRightBevel(bool bottomRightBevel)
{
    Q_D(QQuickRectangleShape);
    d->setBottomRightBevel(bottomRightBevel, QQml::PropertyUtils::State::ExplicitlySet);
}

void QQuickRectangleShape::resetBottomRightBevel()
{
    Q_D(QQuickRectangleShape);
    d->setBottomRightBevel(QQuickRectangleShapePrivate::defaultBevel,
        QQml::PropertyUtils::State::ImplicitlySet);
}

/*!
    \qmlproperty color QtQuick.Shapes.DesignHelpers::RectangleShape::strokeColor

    This property holds the stroking color.

    When set to \c transparent, no stroking occurs.

    The default value is \c "black".
*/

QColor QQuickRectangleShape::strokeColor() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->strokeColor();
}

void QQuickRectangleShape::setStrokeColor(const QColor &color)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setStrokeColor(color);
}

/*!
    \qmlproperty real QtQuick.Shapes.DesignHelpers::RectangleShape::strokeWidth

    This property holds the stroke width.

    When set to a negative value, no stroking occurs.

    The default value is \c 1.
*/

qreal QQuickRectangleShape::strokeWidth() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->strokeWidth();
}

void QQuickRectangleShape::setStrokeWidth(qreal width)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setStrokeWidth(width);
}

/*!
    \qmlproperty color QtQuick.Shapes.DesignHelpers::RectangleShape::fillColor

    This property holds the fill color.

    When set to \c transparent, no filling occurs.

    The default value is \c "white".

    \note If either \l fillGradient is set to something other than \c null, it
    will be used instead of \c fillColor.
*/

QColor QQuickRectangleShape::fillColor() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->fillColor();
}

void QQuickRectangleShape::setFillColor(const QColor &color)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setFillColor(color);
}

/*!
    \include shapepath.qdocinc {fillRule-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

QQuickShapePath::FillRule QQuickRectangleShape::fillRule() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->fillRule();
}

void QQuickRectangleShape::setFillRule(QQuickShapePath::FillRule fillRule)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setFillRule(fillRule);
}

/*!
    \include shapepath.qdocinc {joinStyle-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

QQuickShapePath::JoinStyle QQuickRectangleShape::joinStyle() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->joinStyle();
}

void QQuickRectangleShape::setJoinStyle(QQuickShapePath::JoinStyle style)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setJoinStyle(style);
}

int QQuickRectangleShape::miterLimit() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->miterLimit();
}

void QQuickRectangleShape::setMiterLimit(int limit)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setMiterLimit(limit);
}

/*!
    \include shapepath.qdocinc {capStyle-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

QQuickShapePath::CapStyle QQuickRectangleShape::capStyle() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->capStyle();
}

void QQuickRectangleShape::setCapStyle(QQuickShapePath::CapStyle style)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setCapStyle(style);
}

/*!
    \include shapepath.qdocinc {strokeStyle-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

QQuickShapePath::StrokeStyle QQuickRectangleShape::strokeStyle() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->strokeStyle();
}

void QQuickRectangleShape::setStrokeStyle(QQuickShapePath::StrokeStyle style)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setStrokeStyle(style);
}

/*!
    \include shapepath.qdocinc {dashOffset-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

qreal QQuickRectangleShape::dashOffset() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->dashOffset();
}

void QQuickRectangleShape::setDashOffset(qreal offset)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setDashOffset(offset);
}

/*!
    \include shapepath.qdocinc {dashPattern-property} {QtQuick.Shapes.DesignHelpers::RectangleShape}
*/

QList<qreal> QQuickRectangleShape::dashPattern() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->dashPattern();
}

void QQuickRectangleShape::setDashPattern(const QList<qreal> &array)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setDashPattern(array);
}

/*!
    \qmlproperty ShapeGradient QtQuick.Shapes.DesignHelpers::RectangleShape::fillGradient

    The fillGradient of the rectangle fill color.

    By default, no fillGradient is enabled and the value is null. In this case, the
    fill uses a solid color based on the value of \l fillColor.

    When set, \l fillColor is ignored and filling is done using one of the
    \l ShapeGradient subtypes.

    \note The \l Gradient type cannot be used here. Rather, prefer using one of
    the advanced subtypes, like \l LinearGradient.
*/
QQuickShapeGradient *QQuickRectangleShape::fillGradient() const
{
    Q_D(const QQuickRectangleShape);
    return d->shapePath->fillGradient();
}

void QQuickRectangleShape::setFillGradient(QQuickShapeGradient *fillGradient)
{
    Q_D(QQuickRectangleShape);
    d->shapePath->setFillGradient(fillGradient);
}

void QQuickRectangleShape::resetFillGradient()
{
    setFillGradient(nullptr);
}

/*!
    \qmlproperty enumeration QtQuick.Shapes.DesignHelpers::RectangleShape::borderMode

    The \l borderMode property determines where the border is drawn along the
    edge of the rectangle.

    \value RectangleShape.Inside
        The border is drawn along the inside edge of the item and does not
        affect the item width.

        This is the default value.
    \value RectangleShape.Middle
        The border is drawn over the edge of the item and does not
        affect the item width.
    \value RectangleShape.Outside
        The border is drawn along the outside edge of the item and increases
        the item width by the value of \l strokeWidth.

    \sa strokeWidth
*/
QQuickRectangleShape::BorderMode QQuickRectangleShape::borderMode() const
{
    Q_D(const QQuickRectangleShape);
    return d->borderMode;
}

void QQuickRectangleShape::setBorderMode(BorderMode borderMode)
{
    Q_D(QQuickRectangleShape);
    if (borderMode == d->borderMode)
        return;

    d->borderMode = borderMode;
    polish();
    emit borderModeChanged();
}

void QQuickRectangleShape::resetBorderMode()
{
    setBorderMode(BorderMode::Inside);
}

void QQuickRectangleShape::componentComplete()
{
    Q_D(QQuickRectangleShape);
    d->componentComplete = true;
    // Do this before componentComplete(), because we need the elements to be
    // present in order for the connections to them to be made.
    d->maybeUpdateElements();
    QQuickShape::componentComplete();
}

void QQuickRectangleShape::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickRectangleShape);
    d->maybeUpdateElements();

    QQuickItem::itemChange(change, value);
}

void QQuickRectangleShape::updatePolish()
{
    Q_D(QQuickRectangleShape);
    const qreal rectWidth = d->width.valueBypassingBindings();
    const qreal rectHeight = d->height.valueBypassingBindings();

    d->calculateIndependentRadii();

    switch (d->borderMode) {
    case QQuickRectangleShape::BorderMode::Inside:
        d->borderOffset = d->shapePath->strokeWidth() * 0.5;
        break;
    case QQuickRectangleShape::BorderMode::Middle:
        d->borderOffset = 0;
        break;
    case QQuickRectangleShape::BorderMode::Outside:
        d->borderOffset = -d->shapePath->strokeWidth() * 0.5;
        break;
    }

    switch (d->borderMode) {
    case QQuickRectangleShape::BorderMode::Outside:
        d->borderRadiusAdjustment = d->shapePath->strokeWidth() * 0.5;
        break;
    case QQuickRectangleShape::BorderMode::Middle:
        d->borderRadiusAdjustment = d->shapePath->strokeWidth();
        break;
    default:
        d->borderRadiusAdjustment = 0;
        break;
    }

    switch (d->firstVisibleEdge) {
    case QQuickRectangleShapePrivate::Edge::Top:
        d->shapePath->setStartX(d->effectiveTopLeftRadius + d->borderOffset + d->borderRadiusAdjustment);
        d->shapePath->setStartY(d->borderOffset);
        break;
    case QQuickRectangleShapePrivate::Edge::Right:
        d->shapePath->setStartX(rectWidth - d->borderOffset);
        d->shapePath->setStartY(d->effectiveTopRightRadius + d->borderOffset + d->borderRadiusAdjustment);
        break;
    case QQuickRectangleShapePrivate::Edge::Bottom:
        d->shapePath->setStartX(rectWidth - d->effectiveBottomRightRadius - d->borderOffset - d->borderRadiusAdjustment);
        d->shapePath->setStartY(rectHeight - d->borderOffset);
        break;
    case QQuickRectangleShapePrivate::Edge::Left:
        d->shapePath->setStartX(d->borderOffset);
        d->shapePath->setStartY(rectHeight - d->effectiveBottomLeftRadius - d->borderOffset - d->borderRadiusAdjustment);
        break;
    default:
        Q_UNREACHABLE();
    }

    if (d->topPathLine) {
        d->topPathLine->setX(rectWidth - d->effectiveTopRightRadius - d->borderOffset - d->borderRadiusAdjustment);
        d->topPathLine->setY(d->borderOffset);
    } else {
        d->topPathMove->setX(rectWidth - d->borderOffset);
        d->topPathMove->setY(d->effectiveTopRightRadius + d->borderOffset + d->borderRadiusAdjustment);
    }

    if (d->topRightPathArc) {
        d->topRightPathArc->setX(rectWidth - d->borderOffset);
        d->topRightPathArc->setY(d->effectiveTopRightRadius + d->borderOffset + d->borderRadiusAdjustment);
        d->topRightPathArc->setRadiusX(d->topRightBevel ? 50000 : d->effectiveTopRightRadius + d->borderRadiusAdjustment);
        d->topRightPathArc->setRadiusY(d->topRightBevel ? 50000 : d->effectiveTopRightRadius + d->borderRadiusAdjustment);
    }

    if (d->rightPathLine) {
        d->rightPathLine->setX(rectWidth - d->borderOffset);
        d->rightPathLine->setY(rectHeight - d->effectiveBottomRightRadius - d->borderOffset - d->borderRadiusAdjustment);
    } else {
        d->rightPathMove->setX(rectWidth - d->effectiveBottomRightRadius - d->borderOffset - d->borderRadiusAdjustment);
        d->rightPathMove->setY(rectHeight - d->borderOffset);
    }

    if (d->bottomRightPathArc) {
        d->bottomRightPathArc->setX(rectWidth - d->effectiveBottomRightRadius - d->borderOffset - d->borderRadiusAdjustment);
        d->bottomRightPathArc->setY(rectHeight - d->borderOffset);
        d->bottomRightPathArc->setRadiusX(d->bottomRightBevel ? 50000 : d->effectiveBottomRightRadius + d->borderRadiusAdjustment);
        d->bottomRightPathArc->setRadiusY(d->bottomRightBevel ? 50000 : d->effectiveBottomRightRadius + d->borderRadiusAdjustment);
    }

    if (d->bottomPathLine) {
        d->bottomPathLine->setX(d->effectiveBottomLeftRadius + d->borderOffset + d->borderRadiusAdjustment);
        d->bottomPathLine->setY(rectHeight - d->borderOffset);
    } else {
        d->bottomPathMove->setX(d->borderOffset);
        d->bottomPathMove->setY(rectHeight - d->effectiveBottomLeftRadius - d->borderOffset - d->borderRadiusAdjustment);
    }

    if (d->bottomLeftPathArc) {
        d->bottomLeftPathArc->setX(d->borderOffset);
        d->bottomLeftPathArc->setY(rectHeight - d->effectiveBottomLeftRadius - d->borderOffset - d->borderRadiusAdjustment);
        d->bottomLeftPathArc->setRadiusX(d->bottomLeftBevel ? 50000 : d->effectiveBottomLeftRadius + d->borderRadiusAdjustment);
        d->bottomLeftPathArc->setRadiusY(d->bottomLeftBevel ? 50000 : d->effectiveBottomLeftRadius + d->borderRadiusAdjustment);
    }

    if (d->leftPathLine) {
        d->leftPathLine->setX(d->borderOffset);
        d->leftPathLine->setY(d->effectiveTopLeftRadius + d->borderOffset + d->borderRadiusAdjustment);
    }

    if (d->topLeftPathArc) {
        d->topLeftPathArc->setX(d->effectiveTopLeftRadius + d->borderOffset + d->borderRadiusAdjustment);
        d->topLeftPathArc->setY(d->borderOffset);
        d->topLeftPathArc->setRadiusX(d->topLeftBevel ? 50000 : d->effectiveTopLeftRadius + d->borderRadiusAdjustment);
        d->topLeftPathArc->setRadiusY(d->topLeftBevel ? 50000 : d->effectiveTopLeftRadius + d->borderRadiusAdjustment);
    }

    // This does stuff with each path, so we want to call it after we've made our own changes.
    QQuickShape::updatePolish();

    qCDebug(lcUpdatePolish) << "updatePolish:"
        << "width" << rectWidth
        << "height" << rectHeight
        << "borderMode" << d->borderMode
        << "strokeWidth" << d->shapePath->strokeWidth()
        << "etlr" << d->effectiveTopLeftRadius
        << "etrr" << d->effectiveTopRightRadius
        << "eblr" << d->effectiveBottomLeftRadius
        << "ebrr" << d->effectiveBottomRightRadius
        << "borderOffset" << d->borderOffset
        << "startX" << d->shapePath->startX()
        << "startY" << d->shapePath->startY();
}

QT_END_NAMESPACE

#include "moc_qquickrectangleshape_p.cpp"
