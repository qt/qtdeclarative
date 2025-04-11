// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qtypes.h"
#include <private/qquickrectangularshadow_p_p.h>
#include <private/qquickshadereffect_p.h>
#include <private/qquickitem_p.h>
#include <QtCore/qurl.h>
#include <QtGui/qvector4d.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype RectangularShadow
    \inqmlmodule QtQuick.Effects
    \inherits Item
    \ingroup qtquick-effects
    \brief Creates smoothed rectangle, suitable for example for
    shadow and glow effects.

    RectangularShadow is a rounded rectangle with blur applied.
    The performance of RectangularShadow is much better than a general
    one that creates blurred shadow/glow of any shaped item.

    The following example shows how to add a shadow to a \l Rectangle:
    \table 70%
    \row
    \li \image rectangularshadow-example-1.png
    \li \qml
    import QtQuick
    import QtQuick.Effects

    ...
    RectangularShadow {
        anchors.fill: myRectangle
        offset.x: -10
        offset.y: -5
        radius: myRectangle.radius
        blur: 30
        spread: 10
        color: Qt.darker(myRectangle.color, 1.6)
    }
    Rectangle {
        id: myRectangle
        anchors.centerIn: parent
        width: 200
        height: 100
        radius: 50
        color: "#c0d0f0"
    }
    \endqml
    \endtable

    The API of RectangularShadow is similar to CSS box-shadow, with color,
    offset, spread, and blur values. Additionally, RectangularShadow API
    contains:

    \list
    \li \c real \l radius: Controls the rounding radius applied to rectangle
    corners. Compared to CSS box-shadow, which inherits radius from the parent
    element, RectangularShadow expects user to set it. This allows you to use
    different radiuses and move the RectangularShadow separately from its parent
    item.
    \li \c bool \l cached: Allows caching the blurred shadow texture. This
    increases memory usage while potentially improving rendering performance,
    especially with bigger shadows that don't change dynamically.
    \li \c item \l material: Contains the ShaderEffect element of the
    RectangularShadow for advanced use. This allows, for example, extending the
    effect with a custom shader.
    \endlist

    The rendering output also matches the CSS box-shadow, with few notable
    differences. These differences exist to make the RectangularShadow as
    high-performance as possible.
    \list
    \li Blurring is calculated mathematically in the shader rather than using
    Gaussian blur, which CSS box-shadow implementations often use. This makes
    the shadow look slightly different, especially with larger blur values.
    \li All the rectangle corners must have an even radius. When creating a
    shadow for a \l Rectangle with different radiuses, select the best-matching
    radius for the shadow or use an alternative shadow method, for example,
    \l MultiEffect.
    \endlist

    Here is a table with screenshots to compare the rendering output of
    RectangularShadow and CSS box-shadow in the Chrome browser. The right-most
    element is RectangularShadow in blur multiplied with \c {1.2}
    (so \c 24, \c 48, \c 48) for a closer match.

    \table 80%
    \header
    \li type
    \li CSS box-shadow
    \li RectangularShadow
    \li RectangularShadow + extra blur
    \row
    \li offset: (0, 0) \br
        blur: 20 \br
        spread: 0 \br
    \li \image rectangularshadow-css-1.png
    \li \image rectangularshadow-item-1.png
    \li \image rectangularshadow-itemblur-1.png
    \row
    \li offset: (-10, -20) \br
        blur: 40 \br
        spread: 0 \br
    \li \image rectangularshadow-css-2.png
    \li \image rectangularshadow-item-2.png
    \li \image rectangularshadow-itemblur-2.png
    \row
    \li offset: (-10, -20) \br
        blur: 40 \br
        spread: 10 \br
    \li \image rectangularshadow-css-3.png
    \li \image rectangularshadow-item-3.png
    \li \image rectangularshadow-itemblur-3.png
    \endtable


    RectangularShadow extends the shadow size with an exact amount regarding
    the blur amount, while some other shadows (including CSS box-shadow) have
    a multiplier for the size. The size of the shadow item inside a
    RectangularShadow is:
    \badcode
        width = rectangularShadow.width + 2 * blur +  2 * spread
        height = rectangularShadow.height + 2 * blur + 2 * spread
    \endcode

    For example, the shadow item size of the code below is 280x180 pixels.
    Radius or offset values do not affect the shadow item size.
    \qml
    RectangularShadow {
        width: 200
        height: 100
        blur: 30
        spread: 10
        radius: 20
    }
    \endqml

*/

/*!
    \qmlproperty bool QtQuick.Effects::RectangularShadow::antialiasing

    Used to decide if the shadow should use antialiasing or not.
    When this is \c true, a single pixel antialiasing is used even
    when the \l blur is \c 0.

    The default value is \c true.
*/

QQuickRectangularShadow::QQuickRectangularShadow(QQuickItem *parent)
    : QQuickItem(*new QQuickRectangularShadowPrivate, parent)
{
    setFlag(ItemHasContents);
}

/*!
    \qmlproperty vector2d QtQuick.Effects::RectangularShadow::offset

    This property defines the position offset that is used for the shadow.
    This offset is appended to the shadow position, relative to the
    RectangularShadow item position.

    The default value is \c {Qt.vector2d(0.0, 0.0)} (no offset).
*/
QVector2D QQuickRectangularShadow::offset() const
{
    Q_D(const QQuickRectangularShadow);
    return d->m_offset;
}

void QQuickRectangularShadow::setOffset(const QVector2D &offset)
{
    Q_D(QQuickRectangularShadow);
    if (offset == d->m_offset)
        return;

    d->m_offset = offset;
    d->updateSizeProperties();
    update();
    Q_EMIT offsetChanged();
}

/*!
    \qmlproperty color QtQuick.Effects::RectangularShadow::color

    This property defines the RGBA color value that is used for the shadow.

    The default value is \c {Qt.rgba(0.0, 0.0, 0.0, 1.0)} (black).
*/
QColor QQuickRectangularShadow::color() const
{
    Q_D(const QQuickRectangularShadow);
    return d->m_color;
}

void QQuickRectangularShadow::setColor(const QColor &color)
{
    Q_D(QQuickRectangularShadow);
    if (color == d->m_color)
        return;

    d->m_color = color;
    d->updateColor();
    update();
    Q_EMIT colorChanged();
}

/*!
    \qmlproperty real QtQuick.Effects::RectangularShadow::blur

    This property defines how many pixels outside the item area are reached
    by the shadow.

    The value ranges from 0.0 (no blur) to inf (infinite blur).
    The default value is \c 10.0.

    \note To match with the CSS box-shadow rendering output, the optimal blur
    amount is something like: \c {1.2 * cssBlur}
*/
qreal QQuickRectangularShadow::blur() const
{
    Q_D(const QQuickRectangularShadow);
    return d->m_blur;
}

void QQuickRectangularShadow::setBlur(qreal blur)
{
    Q_D(QQuickRectangularShadow);
    blur = qMax(qreal(0), blur);
    if (blur == d->m_blur)
        return;

    d->m_blur = blur;
    d->updateSizeProperties();
    update();
    Q_EMIT blurChanged();
}

/*!
    \qmlproperty real QtQuick.Effects::RectangularShadow::radius

    This property defines the corner radius that is used to draw a shadow with
    rounded corners.

    The value ranges from 0.0 to half of the effective width or height of
    the item, whichever is smaller.

    The default value is \c 0.
*/
qreal QQuickRectangularShadow::radius() const
{
    Q_D(const QQuickRectangularShadow);
    return d->m_radius;
}

void QQuickRectangularShadow::setRadius(qreal radius)
{
    Q_D(QQuickRectangularShadow);
    radius = qMax(qreal(0), radius);
    if (radius == d->m_radius)
        return;

    d->m_radius = radius;
    d->updateSizeProperties();
    update();
    Q_EMIT radiusChanged();
}

/*!
    \qmlproperty real QtQuick.Effects::RectangularShadow::spread

    This property defines how much the shadow is spread (extended) in
    pixels. This spread is appended to the shadow size, relative to the
    RectangularShadow item size.

    The value ranges from -inf to inf. The default value is \c 0.0.

    \note The radius behavior with spread matches to CSS box-shadow
    standard. So when the spread is smaller than the radius, the
    shadow radius grows by the amount of spread. When the spread grows
    bigger, radius grows only partially. See \l
    {https://www.w3.org/TR/css-backgrounds-3/#shadow-shape}.
    If the shadow radius should grow in sync when the shadow grows (like
    with the Firefox CSS box-shadow implementation), increase the
    RectangularShadow \c width and \c height instead of using the \c spread.
*/
qreal QQuickRectangularShadow::spread() const
{
    Q_D(const QQuickRectangularShadow);
    return d->m_spread;
}

void QQuickRectangularShadow::setSpread(qreal spread)
{
    Q_D(QQuickRectangularShadow);
    if (spread == d->m_spread)
        return;

    d->m_spread = spread;
    d->updateSizeProperties();
    update();
    Q_EMIT spreadChanged();
}

/*!
    \qmlproperty bool QtQuick.Effects::RectangularShadow::cached
    This property allows the effect output pixels to be cached in order to
    improve the rendering performance.

    Every time the effect properties are changed, the pixels in
    the cache must be updated. Memory consumption is increased, because an
    extra buffer of memory is required for storing the effect output.

    It is recommended to disable the cache when the source or the effect
    properties are animated.

    The default value is \c false.
*/
bool QQuickRectangularShadow::isCached() const
{
    Q_D(const QQuickRectangularShadow);
    return d->m_cached;
}

void QQuickRectangularShadow::setCached(bool cached)
{
    Q_D(QQuickRectangularShadow);
    if (cached == d->m_cached)
        return;

    d->m_cached = cached;
    d->updateCached();
    update();
    Q_EMIT cachedChanged();
}

/*!
    \qmlproperty item QtQuick.Effects::RectangularShadow::material

    This property contains the \l ShaderEffect item of the shadow. You can use
    this property to visualize the reach of the shadow, because the effect item
    often has different position and size than the
    RectangularShadow item, due to \l blur, \l offset and \l spread.

    The material can also be replaced with a custom one. The default material
    is a \l ShaderEffect with the following \l {ShaderEffect::}{fragmentShader}:

    \badcode
    #version 440

    layout(location = 0) in vec2 texCoord;
    layout(location = 1) in vec2 fragCoord;
    layout(location = 0) out vec4 fragColor;

    layout(std140, binding = 0) uniform buf {
        mat4 qt_Matrix;
        float qt_Opacity;
        vec4 color;
        vec3 iResolution;
        vec2 rectSize;
        float radius;
        float blur;
    };

    float roundedBox(vec2 centerPos, vec2 size, float radii) {
        return length(max(abs(centerPos) - size + radii, 0.0)) - radii;
    }

    void main()
    {
        float box = roundedBox(fragCoord - iResolution.xy * 0.5, rectSize, radius);
        float a = 1.0 - smoothstep(0.0, blur, box);
        fragColor = color * qt_Opacity * a * a;
    }
    \endcode

    Qt Quick Effect Maker contains the RectangularShadow node that can be used
    as a starting point for a custom material. You can directly use the exported
    effect containing that node as a RectangularShadow material.
    \qml
        RectangularShadow {
            ...
            material: MyShadowEffect { }
        }
    \endqml

    To return to use the default material, set the material property to \c null.
*/
QQuickItem *QQuickRectangularShadow::material() const
{
    Q_D(const QQuickRectangularShadow);
    return d->currentMaterial();
}

void QQuickRectangularShadow::setMaterial(QQuickItem *item)
{
    Q_D(QQuickRectangularShadow);
    if (item == d->m_material)
        return;

    if (item) {
        item->setParentItem(this);
        item->setZ(-1);
    }
    if (d->m_material)
        d->m_material->setVisible(false);

    d->m_material = item;
    d->updateShaderSource();
    update();
    Q_EMIT materialChanged();
}

// *** protected ***

void QQuickRectangularShadow::componentComplete()
{
    Q_D(QQuickRectangularShadow);
    QQuickItem::componentComplete();
    d->initialize();
}

void QQuickRectangularShadow::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickRectangularShadow);
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (width() > 0 && height() > 0)
        d->handleGeometryChange(newGeometry, oldGeometry);
}

void QQuickRectangularShadow::itemChange(ItemChange change, const ItemChangeData &value)
{
    Q_D(QQuickRectangularShadow);
    d->handleItemChange(change, value);
    QQuickItem::itemChange(change, value);
}

// *** private ***

QQuickRectangularShadowPrivate::QQuickRectangularShadowPrivate()
{
    Q_Q(QQuickRectangularShadow);
    m_defaultMaterial = new QQuickShaderEffect(q);
    // Initial values to not get warnings of missing properties.
    // Proper values are updated later.
    m_defaultMaterial->setProperty("iResolution", QVector3D());
    m_defaultMaterial->setProperty("rectSize", QPointF());
    m_defaultMaterial->setProperty("color", QColorConstants::Black);
    m_defaultMaterial->setProperty("radius", 0.0);
    m_defaultMaterial->setProperty("blur", 10.0);
}

void QQuickRectangularShadowPrivate::initialize()
{
    Q_Q(QQuickRectangularShadow);
    if (m_initialized)
        return;
    if (!q->isComponentComplete())
        return;
    if (!q->window())
        return;
    if (q->width() <= 0 || q->height() <= 0)
        return;

    m_defaultMaterial->setParentItem(q);
    m_defaultMaterial->setZ(-1);
    // Default to antialiased
    setImplicitAntialiasing(true);

    QUrl fs = QUrl(QStringLiteral("qrc:/data/shaders/rectangularshadow.frag.qsb"));
    m_defaultMaterial->setFragmentShader(fs);
    QUrl vs = QUrl(QStringLiteral("qrc:/data/shaders/rectangularshadow.vert.qsb"));
    m_defaultMaterial->setVertexShader(vs);

    updateShaderSource();
    m_initialized = true;
}

void QQuickRectangularShadowPrivate::handleItemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    Q_UNUSED(value);
    if (change == QQuickItem::ItemSceneChange)
        initialize();
}

void QQuickRectangularShadowPrivate::handleGeometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_UNUSED(newGeometry);
    Q_UNUSED(oldGeometry);
    initialize();
    updateSizeProperties();
}

qreal QQuickRectangularShadowPrivate::getPadding() const
{
    return qreal(m_blur * 2 + m_spread * 2);
}

void QQuickRectangularShadowPrivate::updateColor()
{
    currentMaterial()->setProperty("color", m_color);
}

void QQuickRectangularShadowPrivate::updateShaderSource()
{
    Q_Q(QQuickRectangularShadow);
    if (!q->isComponentComplete())
        return;

    if (m_material)
        m_defaultMaterial->setVisible(false);

    updateSizeProperties();
    updateColor();
    currentMaterial()->setVisible(true);
}

void QQuickRectangularShadowPrivate::updateSizeProperties()
{
    Q_Q(QQuickRectangularShadow);
    auto *material = currentMaterial();

    const qreal padding = getPadding();
    const qreal clampedRad = clampedRadius();
    const qreal effectWidth = q->width() + padding;
    const qreal effectHeight = q->height() + padding;

    const qreal effectX = (q->width() - effectWidth) * 0.5 + m_offset.x();
    const qreal effectY = (q->height() - effectHeight) * 0.5 + m_offset.y();
    material->setX(effectX);
    material->setY(effectY);
    material->setWidth(effectWidth);
    material->setHeight(effectHeight);

    const qreal aa = q->antialiasing() ? 1.0 : 0.0;
    material->setProperty("iResolution", QVector3D(effectWidth, effectHeight, 1.0));

    // The shrinking ratio when the amount of blur increases
    // so blur extends also towards inner direction.
    const qreal blurReduction = m_blur * 1.8 + aa;
    QPointF rectSize = QPointF((effectWidth * 0.5 - blurReduction),
                               (effectHeight * 0.5 - blurReduction));
    material->setProperty("rectSize", rectSize);
    material->setProperty("radius", clampedRad);
    // Extend blur amount to match with how the CSS box-shadow blur behaves.
    // and to fully utilize the item size.
    const qreal shaderBlur = m_blur * 2.1 + aa;
    material->setProperty("blur", shaderBlur);
}

void QQuickRectangularShadowPrivate::updateCached()
{
    QQuickItemPrivate *effectPrivate = QQuickItemPrivate::get(currentMaterial());
    effectPrivate->layer()->setEnabled(m_cached);
}

qreal QQuickRectangularShadowPrivate::clampedRadius() const
{
    Q_Q(const QQuickRectangularShadow);
    qreal maxRadius = qMin(q->width(), q->height()) * 0.5;
    maxRadius += m_spread * 2;
    qreal spreadRadius = m_radius + m_spread;
    if (m_radius < m_spread && !qFuzzyIsNull(m_spread)) {
        // CSS box-shadow has a specific math to calculate radius with spread
        // https://www.w3.org/TR/css-backgrounds-3/#shadow-shape
        // "the spread distance is first multiplied by the proportion 1 + (r-1)^3,
        // where r is the ratio of the border radius to the spread distance".
        qreal r = (m_radius / m_spread) - 1;
        spreadRadius = m_radius + m_spread * (1 + r * r * r);
    }
    // Reduce the radius when the blur increases
    const qreal blurReduce = m_blur * 0.75;
    maxRadius -= blurReduce;
    const qreal limitedRadius = qMax(0.0, spreadRadius - blurReduce);
    return qMin(limitedRadius, maxRadius);
}

QQuickItem *QQuickRectangularShadowPrivate::currentMaterial() const
{
    if (m_material)
        return m_material;
    else
        return m_defaultMaterial;
}

QT_END_NAMESPACE
