/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickmaterialstyle_p.h"

#include <QtLabsControls/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Material
    \inherits QtObject
    \instantiates QQuickMaterialStyleAttached
    \inqmlmodule QtQuick.Controls.Material
    \ingroup utilities
    \brief A style interface.

    TODO
*/

/*!
    \qmlattachedproperty color QtQuickControls2::Material::textColorPrimaray
*/

static const QColor colors[][14] = {
    // Red
    {
        "#FFEBEE", // Shade50
        "#FFCDD2", // Shade100
        "#EF9A9A", // Shade200
        "#E57373", // Shade300
        "#EF5350", // Shade400
        "#F44336", // Shade500
        "#E53935", // Shade600
        "#D32F2F", // Shade700
        "#C62828", // Shade800
        "#B71C1C", // Shade900
        "#FF8A80", // ShadeA100
        "#FF5252", // ShadeA200
        "#FF1744", // ShadeA400
        "#D50000"  // ShadeA700
    },
    // Pink
    {
        "#FCE4EC", // Shade50
        "#F8BBD0", // Shade100
        "#F48FB1", // Shade200
        "#F06292", // Shade300
        "#EC407A", // Shade400
        "#E91E63", // Shade500
        "#D81B60", // Shade600
        "#C2185B", // Shade700
        "#AD1457", // Shade800
        "#880E4F", // Shade900
        "#FF80AB", // ShadeA100
        "#FF4081", // ShadeA200
        "#F50057", // ShadeA400
        "#C51162"  // ShadeA700
    },
    // Purple
    {
        "#F3E5F5", // Shade50
        "#E1BEE7", // Shade100
        "#CE93D8", // Shade200
        "#BA68C8", // Shade300
        "#AB47BC", // Shade400
        "#9C27B0", // Shade500
        "#8E24AA", // Shade600
        "#7B1FA2", // Shade700
        "#6A1B9A", // Shade800
        "#4A148C", // Shade900
        "#EA80FC", // ShadeA100
        "#E040FB", // ShadeA200
        "#D500F9", // ShadeA400
        "#AA00FF"  // ShadeA700
    },
    // DeepPurple
    {
        "#EDE7F6", // Shade50
        "#D1C4E9", // Shade100
        "#B39DDB", // Shade200
        "#9575CD", // Shade300
        "#7E57C2", // Shade400
        "#673AB7", // Shade500
        "#5E35B1", // Shade600
        "#512DA8", // Shade700
        "#4527A0", // Shade800
        "#311B92", // Shade900
        "#B388FF", // ShadeA100
        "#7C4DFF", // ShadeA200
        "#651FFF", // ShadeA400
        "#6200EA"  // ShadeA700
    },
    // Indigo
    {
        "#E8EAF6", // Shade50
        "#C5CAE9", // Shade100
        "#9FA8DA", // Shade200
        "#7986CB", // Shade300
        "#5C6BC0", // Shade400
        "#3F51B5", // Shade500
        "#3949AB", // Shade600
        "#303F9F", // Shade700
        "#283593", // Shade800
        "#1A237E", // Shade900
        "#8C9EFF", // ShadeA100
        "#536DFE", // ShadeA200
        "#3D5AFE", // ShadeA400
        "#304FFE"  // ShadeA700
    },
    // Blue
    {
        "#E3F2FD", // Shade50
        "#BBDEFB", // Shade100
        "#90CAF9", // Shade200
        "#64B5F6", // Shade300
        "#42A5F5", // Shade400
        "#2196F3", // Shade500
        "#1E88E5", // Shade600
        "#1976D2", // Shade700
        "#1565C0", // Shade800
        "#0D47A1", // Shade900
        "#82B1FF", // ShadeA100
        "#448AFF", // ShadeA200
        "#2979FF", // ShadeA400
        "#2962FF"  // ShadeA700
    },
    // LightBlue
    {
        "#E1F5FE", // Shade50
        "#B3E5FC", // Shade100
        "#81D4FA", // Shade200
        "#4FC3F7", // Shade300
        "#29B6F6", // Shade400
        "#03A9F4", // Shade500
        "#039BE5", // Shade600
        "#0288D1", // Shade700
        "#0277BD", // Shade800
        "#01579B", // Shade900
        "#80D8FF", // ShadeA100
        "#40C4FF", // ShadeA200
        "#00B0FF", // ShadeA400
        "#0091EA"  // ShadeA700
    },
    // Cyan
    {
        "#E0F7FA", // Shade50
        "#B2EBF2", // Shade100
        "#80DEEA", // Shade200
        "#4DD0E1", // Shade300
        "#26C6DA", // Shade400
        "#00BCD4", // Shade500
        "#00ACC1", // Shade600
        "#0097A7", // Shade700
        "#00838F", // Shade800
        "#006064", // Shade900
        "#84FFFF", // ShadeA100
        "#18FFFF", // ShadeA200
        "#00E5FF", // ShadeA400
        "#00B8D4"  // ShadeA700
    },
    // Teal
    {
        "#E0F2F1", // Shade50
        "#B2DFDB", // Shade100
        "#80CBC4", // Shade200
        "#4DB6AC", // Shade300
        "#26A69A", // Shade400
        "#009688", // Shade500
        "#00897B", // Shade600
        "#00796B", // Shade700
        "#00695C", // Shade800
        "#004D40", // Shade900
        "#A7FFEB", // ShadeA100
        "#64FFDA", // ShadeA200
        "#1DE9B6", // ShadeA400
        "#00BFA5"  // ShadeA700
    },
    // Green
    {
        "#E8F5E9", // Shade50
        "#C8E6C9", // Shade100
        "#A5D6A7", // Shade200
        "#81C784", // Shade300
        "#66BB6A", // Shade400
        "#4CAF50", // Shade500
        "#43A047", // Shade600
        "#388E3C", // Shade700
        "#2E7D32", // Shade800
        "#1B5E20", // Shade900
        "#B9F6CA", // ShadeA100
        "#69F0AE", // ShadeA200
        "#00E676", // ShadeA400
        "#00C853"  // ShadeA700
    },
    // LightGreen
    {
        "#F1F8E9", // Shade50
        "#DCEDC8", // Shade100
        "#C5E1A5", // Shade200
        "#AED581", // Shade300
        "#9CCC65", // Shade400
        "#8BC34A", // Shade500
        "#7CB342", // Shade600
        "#689F38", // Shade700
        "#558B2F", // Shade800
        "#33691E", // Shade900
        "#CCFF90", // ShadeA100
        "#B2FF59", // ShadeA200
        "#76FF03", // ShadeA400
        "#64DD17"  // ShadeA700
    },
    // Lime
    {
        "#F9FBE7", // Shade50
        "#F0F4C3", // Shade100
        "#E6EE9C", // Shade200
        "#DCE775", // Shade300
        "#D4E157", // Shade400
        "#CDDC39", // Shade500
        "#C0CA33", // Shade600
        "#AFB42B", // Shade700
        "#9E9D24", // Shade800
        "#827717", // Shade900
        "#F4FF81", // ShadeA100
        "#EEFF41", // ShadeA200
        "#C6FF00", // ShadeA400
        "#AEEA00"  // ShadeA700
    },
    // Yellow
    {
        "#FFFDE7", // Shade50
        "#FFF9C4", // Shade100
        "#FFF59D", // Shade200
        "#FFF176", // Shade300
        "#FFEE58", // Shade400
        "#FFEB3B", // Shade500
        "#FDD835", // Shade600
        "#FBC02D", // Shade700
        "#F9A825", // Shade800
        "#F57F17", // Shade900
        "#FFFF8D", // ShadeA100
        "#FFFF00", // ShadeA200
        "#FFEA00", // ShadeA400
        "#FFD600"  // ShadeA700
    },
    // Amber
    {
        "#FFF8E1", // Shade50
        "#FFECB3", // Shade100
        "#FFE082", // Shade200
        "#FFD54F", // Shade300
        "#FFCA28", // Shade400
        "#FFC107", // Shade500
        "#FFB300", // Shade600
        "#FFA000", // Shade700
        "#FF8F00", // Shade800
        "#FF6F00", // Shade900
        "#FFE57F", // ShadeA100
        "#FFD740", // ShadeA200
        "#FFC400", // ShadeA400
        "#FFAB00"  // ShadeA700
    },
    // Orange
    {
        "#FFF3E0", // Shade50
        "#FFE0B2", // Shade100
        "#FFCC80", // Shade200
        "#FFB74D", // Shade300
        "#FFA726", // Shade400
        "#FF9800", // Shade500
        "#FB8C00", // Shade600
        "#F57C00", // Shade700
        "#EF6C00", // Shade800
        "#E65100", // Shade900
        "#FFD180", // ShadeA100
        "#FFAB40", // ShadeA200
        "#FF9100", // ShadeA400
        "#FF6D00"  // ShadeA700
    },
    // DeepOrange
    {
        "#FBE9E7", // Shade50
        "#FFCCBC", // Shade100
        "#FFAB91", // Shade200
        "#FF8A65", // Shade300
        "#FF7043", // Shade400
        "#FF5722", // Shade500
        "#F4511E", // Shade600
        "#E64A19", // Shade700
        "#D84315", // Shade800
        "#BF360C", // Shade900
        "#FF9E80", // ShadeA100
        "#FF6E40", // ShadeA200
        "#FF3D00", // ShadeA400
        "#DD2C00"  // ShadeA700
    },
    // Brown
    {
        "#EFEBE9", // Shade50
        "#D7CCC8", // Shade100
        "#BCAAA4", // Shade200
        "#A1887F", // Shade300
        "#8D6E63", // Shade400
        "#795548", // Shade500
        "#6D4C41", // Shade600
        "#5D4037", // Shade700
        "#4E342E", // Shade800
        "#3E2723", // Shade900
        "#000000", // ShadeA100
        "#000000", // ShadeA200
        "#000000", // ShadeA400
        "#000000"  // ShadeA700
    },
    // Grey
    {
        "#FAFAFA", // Shade50
        "#F5F5F5", // Shade100
        "#EEEEEE", // Shade200
        "#E0E0E0", // Shade300
        "#BDBDBD", // Shade400
        "#9E9E9E", // Shade500
        "#757575", // Shade600
        "#616161", // Shade700
        "#424242", // Shade800
        "#212121", // Shade900
        "#000000", // ShadeA100
        "#000000", // ShadeA200
        "#000000", // ShadeA400
        "#000000"  // ShadeA700
    },
    // BlueGrey
    {
        "#ECEFF1", // Shade50
        "#CFD8DC", // Shade100
        "#B0BEC5", // Shade200
        "#90A4AE", // Shade300
        "#78909C", // Shade400
        "#607D8B", // Shade500
        "#546E7A", // Shade600
        "#455A64", // Shade700
        "#37474F", // Shade800
        "#263238", // Shade900
        "#000000", // ShadeA100
        "#000000", // ShadeA200
        "#000000", // ShadeA400
        "#000000"  // ShadeA700
    }
};

static const QQuickMaterialStyle::Theme defaultTheme = QQuickMaterialStyle::Light;
static const QQuickMaterialStyle::Color defaultPrimary = QQuickMaterialStyle::BlueGrey;
static const QQuickMaterialStyle::Color defaultAccent = QQuickMaterialStyle::Teal;
static const QColor backgroundColorLight = "#FFFAFAFA";
static const QColor backgroundColorDark = "#FF303030";
static const QColor dialogColorLight = "#FFFFFFFF";
static const QColor dialogColorDark = "#FF303030";
static const QColor primaryTextColorLight = "#DD000000";
static const QColor primaryTextColorDark = "#FFFFFFFF";
static const QColor secondaryTextColorLight = "#89000000";
static const QColor secondaryTextColorDark = "#B2FFFFFF";
static const QColor hintTextColorLight = "#60000000";
static const QColor hintTextColorDark = "#4CFFFFFF";
static const QColor dividerTextColorLight = "#1E000000";
static const QColor dividerTextColorDark = "#1EFFFFFF";
static const QColor raisedButtonColorLight = "#FFD6D7D7";
// TODO: find out actual value
static const QColor raisedButtonPressColorLight = "#FFCCCDCD";
static const QColor raisedButtonDisabledColorLight = dividerTextColorLight;
static const QColor raisedButtonDisabledColorDark = dividerTextColorDark;
static const QColor flatButtonPressColorLight = "#66999999";
static const QColor flatButtonPressColorDark = "#3FCCCCCC";
static const QColor flatButtonFocusColorLight = "#33CCCCCC";
static const QColor flatButtonFocusColorDark = "#26CCCCCC";
static const QColor frameColorLight = hintTextColorLight;
static const QColor frameColorDark = hintTextColorDark;
static const QColor switchUncheckedTrackColorLight = "#42000000";
static const QColor switchUncheckedTrackColorDark = "#4CFFFFFF";
static const QColor switchDisabledTrackColorLight = "#1E000000";
static const QColor switchDisabledTrackColorDark = "#19FFFFFF";
// TODO: find out actual values
static const QColor checkBoxUncheckedRippleColorLight = "#10000000";
static const QColor checkBoxUncheckedRippleColorDark = "#20FFFFFF";

QQuickMaterialStyle::QQuickMaterialStyle(QObject *parent) : QQuickStyle(parent),
    m_explicitTheme(false),
    m_explicitPrimary(false),
    m_explicitAccent(false),
    m_theme(defaultTheme),
    m_primary(defaultPrimary),
    m_accent(defaultAccent)
{
    init(); // TODO: lazy init?
}

QQuickMaterialStyle *QQuickMaterialStyle::qmlAttachedProperties(QObject *object)
{
    return new QQuickMaterialStyle(object);
}

QQuickMaterialStyle::Theme QQuickMaterialStyle::theme() const
{
    return m_theme;
}

void QQuickMaterialStyle::setTheme(Theme theme)
{
    m_explicitTheme = true;
    if (m_theme != theme) {
        m_theme = theme;
        propagateTheme();
        emit themeChanged();
        emit paletteChanged();
    }
}

void QQuickMaterialStyle::inheritTheme(Theme theme)
{
    if (!m_explicitTheme && m_theme != theme) {
        m_theme = theme;
        propagateTheme();
        emit themeChanged();
        emit paletteChanged();
    }
}

void QQuickMaterialStyle::propagateTheme()
{
    foreach (QQuickStyle *child, childStyles()) {
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(child);
        if (material)
            material->inheritTheme(m_theme);
    }
}

void QQuickMaterialStyle::resetTheme()
{
    if (m_explicitTheme) {
        m_explicitTheme = false;
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(parentStyle());
        inheritTheme(material ? material->theme() : defaultTheme);
    }
}

QQuickMaterialStyle::Color QQuickMaterialStyle::primary() const
{
    return m_primary;
}

void QQuickMaterialStyle::setPrimary(QQuickMaterialStyle::Color color)
{
    m_explicitPrimary = true;
    if (m_primary != color) {
        m_primary = color;
        propagatePrimary();
        emit primaryChanged();
        emit paletteChanged();
    }
}

void QQuickMaterialStyle::inheritPrimary(QQuickMaterialStyle::Color color)
{
    if (!m_explicitPrimary && m_primary != color) {
        m_primary = color;
        propagatePrimary();
        emit primaryChanged();
    }
}

void QQuickMaterialStyle::propagatePrimary()
{
    foreach (QQuickStyle *child, childStyles()) {
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(child);
        if (material)
            material->inheritPrimary(m_primary);
    }
}

void QQuickMaterialStyle::resetPrimary()
{
    if (m_explicitPrimary) {
        m_explicitPrimary = false;
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(parentStyle());
        inheritPrimary(material ? material->primary() : defaultPrimary);
    }
}

QQuickMaterialStyle::Color QQuickMaterialStyle::accent() const
{
    return m_accent;
}

void QQuickMaterialStyle::setAccent(QQuickMaterialStyle::Color color)
{
    m_explicitAccent = true;
    if (m_accent != color) {
        m_accent = color;
        propagateAccent();
        emit accentChanged();
        emit paletteChanged();
    }
}

void QQuickMaterialStyle::inheritAccent(QQuickMaterialStyle::Color color)
{
    if (!m_explicitAccent && m_accent != color) {
        m_accent = color;
        propagateAccent();
        emit accentChanged();
    }
}

void QQuickMaterialStyle::propagateAccent()
{
    foreach (QQuickStyle *child, childStyles()) {
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(child);
        if (material)
            material->inheritAccent(m_accent);
    }
}

void QQuickMaterialStyle::resetAccent()
{
    if (m_explicitAccent) {
        m_explicitAccent = false;
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(parentStyle());
        inheritAccent(material ? material->accent() : defaultAccent);
    }
}

QColor QQuickMaterialStyle::accentColor() const
{
    return color(m_accent, Shade500);
}

QColor QQuickMaterialStyle::backgroundColor() const
{
    return m_theme == Light ? backgroundColorLight : backgroundColorDark;
}

QColor QQuickMaterialStyle::primaryTextColor() const
{
    return m_theme == Light ? primaryTextColorLight : primaryTextColorDark;
}

QColor QQuickMaterialStyle::primaryHighlightedTextColor() const
{
    return primaryTextColorDark;
}

QColor QQuickMaterialStyle::secondaryTextColor() const
{
    return m_theme == Light ? secondaryTextColorLight : secondaryTextColorDark;
}

QColor QQuickMaterialStyle::hintTextColor() const
{
    return m_theme == Light ? hintTextColorLight : hintTextColorDark;
}

QColor QQuickMaterialStyle::textSelectionColor() const
{
    QColor color = accentColor();
    color.setAlphaF(0.4);
    return color;
}

QColor QQuickMaterialStyle::dropShadowColor() const
{
    return QColor("#40000000");
}

QColor QQuickMaterialStyle::dividerColor() const
{
    return m_theme == Light ? dividerTextColorLight : dividerTextColorDark;
}

QColor QQuickMaterialStyle::raisedButtonColor() const
{
    return m_theme == Light ? raisedButtonColorLight : flatButtonFocusColorDark;
}

QColor QQuickMaterialStyle::raisedButtonHoverColor() const
{
    // The specs don't specify different colors here for the light theme.
    return m_theme == Light ? raisedButtonColorLight : flatButtonPressColorDark;
}

QColor QQuickMaterialStyle::raisedButtonPressColor() const
{
    return m_theme == Light ? raisedButtonPressColorLight : flatButtonPressColorDark;
}

QColor QQuickMaterialStyle::raisedButtonDisabledColor() const
{
    return m_theme == Light ? raisedButtonDisabledColorLight : raisedButtonDisabledColorDark;
}

QColor QQuickMaterialStyle::raisedHighlightedButtonColor() const
{
    return accentColor();
}

QColor QQuickMaterialStyle::raisedHighlightedButtonHoverColor() const
{
    return color(m_accent, Shade600);
}

QColor QQuickMaterialStyle::raisedHighlightedButtonPressColor() const
{
    return color(m_accent, Shade700);
}

QColor QQuickMaterialStyle::raisedHighlightedButtonDisabledColor() const
{
    return m_theme == Light ? raisedButtonDisabledColorLight : raisedButtonDisabledColorDark;
}

QColor QQuickMaterialStyle::flatButtonPressColor() const
{
    return m_theme == Light ? flatButtonPressColorLight : flatButtonPressColorDark;
}

QColor QQuickMaterialStyle::flatButtonFocusColor() const
{
    return m_theme == Light ? flatButtonFocusColorLight : flatButtonFocusColorDark;
}

QColor QQuickMaterialStyle::frameColor() const
{
    return m_theme == Light ? frameColorLight : frameColorDark;
}

QColor QQuickMaterialStyle::checkBoxUncheckedRippleColor() const
{
    return m_theme == Light ? checkBoxUncheckedRippleColorLight : checkBoxUncheckedRippleColorDark;
}

QColor QQuickMaterialStyle::checkBoxCheckedRippleColor() const
{
    QColor pressColor = color(m_accent, Shade500);
    // TODO: find out actual value
    pressColor.setAlpha(30);
    return pressColor;
}

QColor QQuickMaterialStyle::switchUncheckedTrackColor() const
{
    return m_theme == Light ? switchUncheckedTrackColorLight : switchUncheckedTrackColorDark;
}

QColor QQuickMaterialStyle::switchCheckedTrackColor() const
{
    QColor trackColor = m_theme == Light ? accentColor() : color(m_accent, Shade200);
    trackColor.setAlphaF(0.5);
    return trackColor;
}

QColor QQuickMaterialStyle::switchUncheckedHandleColor() const
{
    return m_theme == Light ? color(Grey, Shade50) : color(Grey, Shade400);
}

QColor QQuickMaterialStyle::switchCheckedHandleColor() const
{
    return m_theme == Light ? accentColor() : color(m_accent, Shade200);
}

QColor QQuickMaterialStyle::switchDisabledTrackColor() const
{
    return m_theme == Light ? switchDisabledTrackColorLight : switchDisabledTrackColorDark;
}

QColor QQuickMaterialStyle::switchDisabledHandleColor() const
{
    return m_theme == Light ? color(Grey, Shade400) : color(Grey, Shade800);
}

QColor QQuickMaterialStyle::scrollBarColor() const
{
    return m_theme == Light ? "#40000000" : "#40FFFFFF";
}

QColor QQuickMaterialStyle::scrollBarPressedColor() const
{
    return m_theme == Light ? "#80000000" : "#80FFFFFF";
}

QColor QQuickMaterialStyle::drawerBackgroundColor() const
{
    return dividerTextColorLight;
}

QColor QQuickMaterialStyle::color(QQuickMaterialStyle::Color color, QQuickMaterialStyle::Shade shade) const
{
    int count = sizeof(colors) / sizeof(colors[0]);
    if (color < 0 || color >= count)
        return QColor();

    count = sizeof(colors[0]) / sizeof(colors[0][0]);
    if (shade < 0 || shade >= count)
        return QColor();

    return colors[color][shade];
}

void QQuickMaterialStyle::parentStyleChange(QQuickStyle *newParent, QQuickStyle *oldParent)
{
    Q_UNUSED(oldParent);
    QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(newParent);
    if (material) {
        inheritPrimary(material->primary());
        inheritAccent(material->accent());
        inheritTheme(material->theme());
    }
}

QT_END_NAMESPACE
