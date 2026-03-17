// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qquickcolor_p.h"

#include <private/qquickvaluetypes_p.h>
#include <private/qv4engine_p.h>

QT_BEGIN_NAMESPACE

/*!
 * \qmltype Color
 * \nativetype QQuickColor
 * \inqmlmodule QtQuick
 * \brief Holds utility functions for colors.
 * \since 6.12
 */
QQuickColor::QQuickColor(QObject *parent) : QObject(parent)
{
}

/*!
 * \qmlmethod color Color::rgba(real red, real green, real blue, real alpha)
 *
 * Returns a color with the specified \a red, \a green, \a blue, and \a alpha
 * components. All components should be in the range 0-1 (inclusive).
 */
QColor QQuickColor::rgba(double r, double g, double b, double a) const
{
    return QColor::fromRgbF(r, g, b, a);
}

/*!
 * \qmlmethod color Color::hsla(real hue, real saturation, real lightness, real alpha)
 *
 * Returns a color with the specified \a hue, \a saturation, \a lightness, and \a alpha
 * components. All components should be in the range 0-1 (inclusive).
 */
QColor QQuickColor::hsla(double h, double s, double l, double a) const
{
    return QColor::fromHslF(h, s, l, a);
}

/*!
 * \qmlmethod color Color::hsva(real hue, real saturation, real value, real alpha)
 *
 * Returns a color with the specified \a hue, \a saturation, \a value and \a alpha
 * components. All components should be in the range 0-1 (inclusive).
 */
QColor QQuickColor::hsva(double h, double s, double v, double a) const
{
    return QColor::fromHsvF(h, s, v, a);
}

/*!
 * \qmlmethod color Color::fromString(string name)
 *
 * Returns the color corresponding to the given \a name (i.e. red or #ff0000).
 * If there is no such color, an exception is thrown.
 */
QColor QQuickColor::fromString(const QString &name) const
{
    QColor c = QColor::fromString(name);
    if (!c.isValid()) {
        const auto *qmlContext = QQmlData::get(this)->outerContext;
        Q_ASSERT(qmlContext);
        if (qmlContext)
            qmlContext->engine()->throwError(
                    QStringLiteral("\"%1\" is not a valid color name").arg(name));
        return {};
    }

    return c;
}

/*!
 * \qmlmethod bool Color::equal(color lhs, color rhs)
 *
 * Returns true if \a lhs and \a rhs are equal.
 * See \l{QColor::}{operator==()}.
 *
 * \note This function is a typed version of \l{Qt::colorEqual()}
 */
bool QQuickColor::equal(const QColor &lhs, const QColor &rhs) const
{
    return lhs == rhs;
}

/*!
 * \qmlmethod color Color::transparent(color color, real opacity)
 *
 * Returns \a color with an alpha value of \a opacity, which ranges from 0
 * (completely transparent) to 1 (completely opaque).
 */
QColor QQuickColor::transparent(const QColor &color, qreal opacity) const
{
    QColor newColor = color;
    newColor.setAlphaF(opacity);
    return newColor;
}

/*!
 * \qmlmethod color Color::blend(color a, color b, real factor)
 *
 * Returns a color that is a blend of \a a and \a b. The resulting color is constructed from the
 * independent linear interpolations of the RGB color channels of \a a and \a b.
 *
 * If \a factor is smaller than 0.0, \a a is returned. If factor is larger than 1.0, \a b is
 * returned.
 */
QColor QQuickColor::blend(const QColor &a, const QColor &b, qreal factor) const
{
    if (std::isnan(factor)) {
        qWarning("NaN given as argument to Color.blend()");
        return QColor();
    }

    if (factor <= 0.0)
        return a;
    if (factor >= 1.0)
        return b;

    const auto rgbA = a.toRgb();
    const auto rgbB = b.toRgb();
    QColor color;
    color.setRedF(rgbA.redF() * (1.0 - factor) + rgbB.redF() * factor);
    color.setGreenF(rgbA.greenF() * (1.0 - factor) + rgbB.greenF() * factor);
    color.setBlueF(rgbA.blueF() * (1.0 - factor) + rgbB.blueF() * factor);
    return color;
}

/*!
 * \qmlmethod color Color::darker(color baseColor, real factor)
 *
 * Returns a color darker than \a baseColor by the \a factor provided.
 *
 * The function converts the current RGB color to HSV, divides the value (V) component
 * by factor and converts the color back to RGB.
 *
 * If the factor is greater than 1.0, this function returns a darker color. Setting factor to 3.0
 * returns a color that has one-third the brightness. If the factor is less than 1.0, the return
 * color is lighter, but we recommend using the \l{Color::lighter()} function for this purpose. If
 * the factor is 0 or negative, the return value is unspecified.
 */
QColor QQuickColor::darker(const QColor &baseColor, double factor) const
{
    return QQuickColorValueType(baseColor).darker(factor);
}

/*!
 * \qmlmethod color Color::lighter(color baseColor, real factor)
 *
 * Returns a color lighter than \a baseColor by the \a factor provided.
 *
 * The function converts the current RGB color to HSV, multiplies the value (V) component
 * by factor and converts the color back to RGB.
 *
 * If the factor is greater than 1.0, this functions returns a lighter color. Setting factor to 1.5
 * returns a color that is 50% brighter. If the factor is less than 1.0, the return color is
 * darker, but we recommend using the \l{Color::darker()} function for this purpose. If the factor
 * is 0 or negative, the return value is unspecified.
 */
QColor QQuickColor::lighter(const QColor &baseColor, double factor) const
{
    return QQuickColorValueType(baseColor).lighter(factor);
}

/*!
 * \qmlmethod color Color::tint(color baseColor, color tintColor)
 *
 * This function allows tinting one color (\a baseColor) with another (\a tintColor).
 *
 * The tint color should usually be mostly transparent, or you will not be
 * able to see the underlying color. The example below provides a slight red
 * tint by having the tint color be pure red, which is only 1/16th opaque.
 *
 * \qml
 * Item {
 *     Rectangle {
 *         x: 0
 *         width: 80
 *         height: 80
 *         color: "lightsteelblue"
 *     }
 *     Rectangle {
 *         x: 100; width: 80; height: 80
 *         color: Color.tint("lightsteelblue", "#10FF0000")
 *     }
 * }
 * \endqml
 * \image declarative-rect_tint.png {Side-by-side representation of a light
 * steel blue square and a light steel blue square with a tint applied}
 *
 * Tint is most useful when a subtle change is intended to be conveyed due to some event;
 * you can then use tinting to more effectively tune the visible color.
 */
QColor QQuickColor::tint(const QColor &baseColor, const QColor &tintColor) const
{
    return QQuickColorValueType(baseColor).tint(tintColor);
}

QT_END_NAMESPACE

#include "moc_qquickcolor_p.cpp"
