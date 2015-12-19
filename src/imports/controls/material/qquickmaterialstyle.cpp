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

#include <QtCore/qsettings.h>
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

static const QRgb colors[][14] = {
    // Red
    {
        0xFFEBEE, // Shade50
        0xFFCDD2, // Shade100
        0xEF9A9A, // Shade200
        0xE57373, // Shade300
        0xEF5350, // Shade400
        0xF44336, // Shade500
        0xE53935, // Shade600
        0xD32F2F, // Shade700
        0xC62828, // Shade800
        0xB71C1C, // Shade900
        0xFF8A80, // ShadeA100
        0xFF5252, // ShadeA200
        0xFF1744, // ShadeA400
        0xD50000  // ShadeA700
    },
    // Pink
    {
        0xFCE4EC, // Shade50
        0xF8BBD0, // Shade100
        0xF48FB1, // Shade200
        0xF06292, // Shade300
        0xEC407A, // Shade400
        0xE91E63, // Shade500
        0xD81B60, // Shade600
        0xC2185B, // Shade700
        0xAD1457, // Shade800
        0x880E4F, // Shade900
        0xFF80AB, // ShadeA100
        0xFF4081, // ShadeA200
        0xF50057, // ShadeA400
        0xC51162  // ShadeA700
    },
    // Purple
    {
        0xF3E5F5, // Shade50
        0xE1BEE7, // Shade100
        0xCE93D8, // Shade200
        0xBA68C8, // Shade300
        0xAB47BC, // Shade400
        0x9C27B0, // Shade500
        0x8E24AA, // Shade600
        0x7B1FA2, // Shade700
        0x6A1B9A, // Shade800
        0x4A148C, // Shade900
        0xEA80FC, // ShadeA100
        0xE040FB, // ShadeA200
        0xD500F9, // ShadeA400
        0xAA00FF  // ShadeA700
    },
    // DeepPurple
    {
        0xEDE7F6, // Shade50
        0xD1C4E9, // Shade100
        0xB39DDB, // Shade200
        0x9575CD, // Shade300
        0x7E57C2, // Shade400
        0x673AB7, // Shade500
        0x5E35B1, // Shade600
        0x512DA8, // Shade700
        0x4527A0, // Shade800
        0x311B92, // Shade900
        0xB388FF, // ShadeA100
        0x7C4DFF, // ShadeA200
        0x651FFF, // ShadeA400
        0x6200EA  // ShadeA700
    },
    // Indigo
    {
        0xE8EAF6, // Shade50
        0xC5CAE9, // Shade100
        0x9FA8DA, // Shade200
        0x7986CB, // Shade300
        0x5C6BC0, // Shade400
        0x3F51B5, // Shade500
        0x3949AB, // Shade600
        0x303F9F, // Shade700
        0x283593, // Shade800
        0x1A237E, // Shade900
        0x8C9EFF, // ShadeA100
        0x536DFE, // ShadeA200
        0x3D5AFE, // ShadeA400
        0x304FFE  // ShadeA700
    },
    // Blue
    {
        0xE3F2FD, // Shade50
        0xBBDEFB, // Shade100
        0x90CAF9, // Shade200
        0x64B5F6, // Shade300
        0x42A5F5, // Shade400
        0x2196F3, // Shade500
        0x1E88E5, // Shade600
        0x1976D2, // Shade700
        0x1565C0, // Shade800
        0x0D47A1, // Shade900
        0x82B1FF, // ShadeA100
        0x448AFF, // ShadeA200
        0x2979FF, // ShadeA400
        0x2962FF  // ShadeA700
    },
    // LightBlue
    {
        0xE1F5FE, // Shade50
        0xB3E5FC, // Shade100
        0x81D4FA, // Shade200
        0x4FC3F7, // Shade300
        0x29B6F6, // Shade400
        0x03A9F4, // Shade500
        0x039BE5, // Shade600
        0x0288D1, // Shade700
        0x0277BD, // Shade800
        0x01579B, // Shade900
        0x80D8FF, // ShadeA100
        0x40C4FF, // ShadeA200
        0x00B0FF, // ShadeA400
        0x0091EA  // ShadeA700
    },
    // Cyan
    {
        0xE0F7FA, // Shade50
        0xB2EBF2, // Shade100
        0x80DEEA, // Shade200
        0x4DD0E1, // Shade300
        0x26C6DA, // Shade400
        0x00BCD4, // Shade500
        0x00ACC1, // Shade600
        0x0097A7, // Shade700
        0x00838F, // Shade800
        0x006064, // Shade900
        0x84FFFF, // ShadeA100
        0x18FFFF, // ShadeA200
        0x00E5FF, // ShadeA400
        0x00B8D4  // ShadeA700
    },
    // Teal
    {
        0xE0F2F1, // Shade50
        0xB2DFDB, // Shade100
        0x80CBC4, // Shade200
        0x4DB6AC, // Shade300
        0x26A69A, // Shade400
        0x009688, // Shade500
        0x00897B, // Shade600
        0x00796B, // Shade700
        0x00695C, // Shade800
        0x004D40, // Shade900
        0xA7FFEB, // ShadeA100
        0x64FFDA, // ShadeA200
        0x1DE9B6, // ShadeA400
        0x00BFA5  // ShadeA700
    },
    // Green
    {
        0xE8F5E9, // Shade50
        0xC8E6C9, // Shade100
        0xA5D6A7, // Shade200
        0x81C784, // Shade300
        0x66BB6A, // Shade400
        0x4CAF50, // Shade500
        0x43A047, // Shade600
        0x388E3C, // Shade700
        0x2E7D32, // Shade800
        0x1B5E20, // Shade900
        0xB9F6CA, // ShadeA100
        0x69F0AE, // ShadeA200
        0x00E676, // ShadeA400
        0x00C853  // ShadeA700
    },
    // LightGreen
    {
        0xF1F8E9, // Shade50
        0xDCEDC8, // Shade100
        0xC5E1A5, // Shade200
        0xAED581, // Shade300
        0x9CCC65, // Shade400
        0x8BC34A, // Shade500
        0x7CB342, // Shade600
        0x689F38, // Shade700
        0x558B2F, // Shade800
        0x33691E, // Shade900
        0xCCFF90, // ShadeA100
        0xB2FF59, // ShadeA200
        0x76FF03, // ShadeA400
        0x64DD17  // ShadeA700
    },
    // Lime
    {
        0xF9FBE7, // Shade50
        0xF0F4C3, // Shade100
        0xE6EE9C, // Shade200
        0xDCE775, // Shade300
        0xD4E157, // Shade400
        0xCDDC39, // Shade500
        0xC0CA33, // Shade600
        0xAFB42B, // Shade700
        0x9E9D24, // Shade800
        0x827717, // Shade900
        0xF4FF81, // ShadeA100
        0xEEFF41, // ShadeA200
        0xC6FF00, // ShadeA400
        0xAEEA00  // ShadeA700
    },
    // Yellow
    {
        0xFFFDE7, // Shade50
        0xFFF9C4, // Shade100
        0xFFF59D, // Shade200
        0xFFF176, // Shade300
        0xFFEE58, // Shade400
        0xFFEB3B, // Shade500
        0xFDD835, // Shade600
        0xFBC02D, // Shade700
        0xF9A825, // Shade800
        0xF57F17, // Shade900
        0xFFFF8D, // ShadeA100
        0xFFFF00, // ShadeA200
        0xFFEA00, // ShadeA400
        0xFFD600  // ShadeA700
    },
    // Amber
    {
        0xFFF8E1, // Shade50
        0xFFECB3, // Shade100
        0xFFE082, // Shade200
        0xFFD54F, // Shade300
        0xFFCA28, // Shade400
        0xFFC107, // Shade500
        0xFFB300, // Shade600
        0xFFA000, // Shade700
        0xFF8F00, // Shade800
        0xFF6F00, // Shade900
        0xFFE57F, // ShadeA100
        0xFFD740, // ShadeA200
        0xFFC400, // ShadeA400
        0xFFAB00  // ShadeA700
    },
    // Orange
    {
        0xFFF3E0, // Shade50
        0xFFE0B2, // Shade100
        0xFFCC80, // Shade200
        0xFFB74D, // Shade300
        0xFFA726, // Shade400
        0xFF9800, // Shade500
        0xFB8C00, // Shade600
        0xF57C00, // Shade700
        0xEF6C00, // Shade800
        0xE65100, // Shade900
        0xFFD180, // ShadeA100
        0xFFAB40, // ShadeA200
        0xFF9100, // ShadeA400
        0xFF6D00  // ShadeA700
    },
    // DeepOrange
    {
        0xFBE9E7, // Shade50
        0xFFCCBC, // Shade100
        0xFFAB91, // Shade200
        0xFF8A65, // Shade300
        0xFF7043, // Shade400
        0xFF5722, // Shade500
        0xF4511E, // Shade600
        0xE64A19, // Shade700
        0xD84315, // Shade800
        0xBF360C, // Shade900
        0xFF9E80, // ShadeA100
        0xFF6E40, // ShadeA200
        0xFF3D00, // ShadeA400
        0xDD2C00  // ShadeA700
    },
    // Brown
    {
        0xEFEBE9, // Shade50
        0xD7CCC8, // Shade100
        0xBCAAA4, // Shade200
        0xA1887F, // Shade300
        0x8D6E63, // Shade400
        0x795548, // Shade500
        0x6D4C41, // Shade600
        0x5D4037, // Shade700
        0x4E342E, // Shade800
        0x3E2723, // Shade900
        0x000000, // ShadeA100
        0x000000, // ShadeA200
        0x000000, // ShadeA400
        0x000000  // ShadeA700
    },
    // Grey
    {
        0xFAFAFA, // Shade50
        0xF5F5F5, // Shade100
        0xEEEEEE, // Shade200
        0xE0E0E0, // Shade300
        0xBDBDBD, // Shade400
        0x9E9E9E, // Shade500
        0x757575, // Shade600
        0x616161, // Shade700
        0x424242, // Shade800
        0x212121, // Shade900
        0x000000, // ShadeA100
        0x000000, // ShadeA200
        0x000000, // ShadeA400
        0x000000  // ShadeA700
    },
    // BlueGrey
    {
        0xECEFF1, // Shade50
        0xCFD8DC, // Shade100
        0xB0BEC5, // Shade200
        0x90A4AE, // Shade300
        0x78909C, // Shade400
        0x607D8B, // Shade500
        0x546E7A, // Shade600
        0x455A64, // Shade700
        0x37474F, // Shade800
        0x263238, // Shade900
        0x000000, // ShadeA100
        0x000000, // ShadeA200
        0x000000, // ShadeA400
        0x000000  // ShadeA700
    }
};

static QQuickMaterialStyle::Theme defaultTheme = QQuickMaterialStyle::Light;
static QQuickMaterialStyle::Color defaultPrimary = QQuickMaterialStyle::BlueGrey;
static QQuickMaterialStyle::Color defaultAccent = QQuickMaterialStyle::Teal;
static const QRgb backgroundColorLight = 0xFFFAFAFA;
static const QRgb backgroundColorDark = 0xFF303030;
static const QRgb dialogColorLight = 0xFFFFFFFF;
static const QRgb dialogColorDark = 0xFF303030;
static const QRgb primaryTextColorLight = 0xDD000000;
static const QRgb primaryTextColorDark = 0xFFFFFFFF;
static const QRgb secondaryTextColorLight = 0x89000000;
static const QRgb secondaryTextColorDark = 0xB2FFFFFF;
static const QRgb hintTextColorLight = 0x60000000;
static const QRgb hintTextColorDark = 0x4CFFFFFF;
static const QRgb dividerTextColorLight = 0x1E000000;
static const QRgb dividerTextColorDark = 0x1EFFFFFF;
static const QRgb raisedButtonColorLight = 0xFFD6D7D7;
// TODO: find out actual value
static const QRgb raisedButtonPressColorLight = 0xFFCCCDCD;
static const QRgb raisedButtonDisabledColorLight = dividerTextColorLight;
static const QRgb raisedButtonDisabledColorDark = dividerTextColorDark;
static const QRgb flatButtonPressColorLight = 0x66999999;
static const QRgb flatButtonPressColorDark = 0x3FCCCCCC;
static const QRgb flatButtonFocusColorLight = 0x33CCCCCC;
static const QRgb flatButtonFocusColorDark = 0x26CCCCCC;
static const QRgb frameColorLight = hintTextColorLight;
static const QRgb frameColorDark = hintTextColorDark;
static const QRgb switchUncheckedTrackColorLight = 0x42000000;
static const QRgb switchUncheckedTrackColorDark = 0x4CFFFFFF;
static const QRgb switchDisabledTrackColorLight = 0x1E000000;
static const QRgb switchDisabledTrackColorDark = 0x19FFFFFF;
// TODO: find out actual values
static const QRgb checkBoxUncheckedRippleColorLight = 0x10000000;
static const QRgb checkBoxUncheckedRippleColorDark = 0x20FFFFFF;

QQuickMaterialStyle::QQuickMaterialStyle(QObject *parent) : QQuickStyle(parent),
    m_explicitTheme(false),
    m_explicitPrimary(false),
    m_explicitAccent(false),
    m_theme(defaultTheme),
    m_primary(defaultPrimary),
    m_accent(defaultAccent)
{
    init();
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
    return QColor::fromRgba(m_theme == Light ? backgroundColorLight : backgroundColorDark);
}

QColor QQuickMaterialStyle::primaryTextColor() const
{
    return QColor::fromRgba(m_theme == Light ? primaryTextColorLight : primaryTextColorDark);
}

QColor QQuickMaterialStyle::primaryHighlightedTextColor() const
{
    return QColor::fromRgba(primaryTextColorDark);
}

QColor QQuickMaterialStyle::secondaryTextColor() const
{
    return QColor::fromRgba(m_theme == Light ? secondaryTextColorLight : secondaryTextColorDark);
}

QColor QQuickMaterialStyle::hintTextColor() const
{
    return QColor::fromRgba(m_theme == Light ? hintTextColorLight : hintTextColorDark);
}

QColor QQuickMaterialStyle::textSelectionColor() const
{
    QColor color = accentColor();
    color.setAlphaF(0.4);
    return color;
}

QColor QQuickMaterialStyle::dropShadowColor() const
{
    return QColor::fromRgba(0x40000000);
}

QColor QQuickMaterialStyle::dividerColor() const
{
    return QColor::fromRgba(m_theme == Light ? dividerTextColorLight : dividerTextColorDark);
}

QColor QQuickMaterialStyle::raisedButtonColor() const
{
    return QColor::fromRgba(m_theme == Light ? raisedButtonColorLight : flatButtonFocusColorDark);
}

QColor QQuickMaterialStyle::raisedButtonHoverColor() const
{
    // The specs don't specify different colors here for the light theme.
    return QColor::fromRgba(m_theme == Light ? raisedButtonColorLight : flatButtonPressColorDark);
}

QColor QQuickMaterialStyle::raisedButtonPressColor() const
{
    return QColor::fromRgba(m_theme == Light ? raisedButtonPressColorLight : flatButtonPressColorDark);
}

QColor QQuickMaterialStyle::raisedButtonDisabledColor() const
{
    return QColor::fromRgba(m_theme == Light ? raisedButtonDisabledColorLight : raisedButtonDisabledColorDark);
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
    return QColor::fromRgba(m_theme == Light ? raisedButtonDisabledColorLight : raisedButtonDisabledColorDark);
}

QColor QQuickMaterialStyle::flatButtonPressColor() const
{
    return QColor::fromRgba(m_theme == Light ? flatButtonPressColorLight : flatButtonPressColorDark);
}

QColor QQuickMaterialStyle::flatButtonFocusColor() const
{
    return QColor::fromRgba(m_theme == Light ? flatButtonFocusColorLight : flatButtonFocusColorDark);
}

QColor QQuickMaterialStyle::frameColor() const
{
    return QColor::fromRgba(m_theme == Light ? frameColorLight : frameColorDark);
}

QColor QQuickMaterialStyle::checkBoxUncheckedRippleColor() const
{
    return QColor::fromRgba(m_theme == Light ? checkBoxUncheckedRippleColorLight : checkBoxUncheckedRippleColorDark);
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
    return QColor::fromRgba(m_theme == Light ? switchUncheckedTrackColorLight : switchUncheckedTrackColorDark);
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
    return QColor::fromRgba(m_theme == Light ? switchDisabledTrackColorLight : switchDisabledTrackColorDark);
}

QColor QQuickMaterialStyle::switchDisabledHandleColor() const
{
    return m_theme == Light ? color(Grey, Shade400) : color(Grey, Shade800);
}

QColor QQuickMaterialStyle::scrollBarColor() const
{
    return QColor::fromRgba(m_theme == Light ? 0x40000000 : 0x40FFFFFF);
}

QColor QQuickMaterialStyle::scrollBarPressedColor() const
{
    return QColor::fromRgba(m_theme == Light ? 0x80000000 : 0x80FFFFFF);
}

QColor QQuickMaterialStyle::drawerBackgroundColor() const
{
    return QColor::fromRgba(dividerTextColorLight);
}

QColor QQuickMaterialStyle::dialogColor() const
{
    return QColor::fromRgba(m_theme == Light ? dialogColorLight : dialogColorDark);
}

QColor QQuickMaterialStyle::backgroundDimColor() const
{
    return QColor::fromRgba(m_theme == Light ? 0x99303030 : 0x99fafafa);
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

template <typename Enum>
static Enum readEnumValue(QSettings *settings, const QString &name, Enum fallback)
{
    Enum result = fallback;
    if (settings->contains(name)) {
        QMetaEnum enumeration = QMetaEnum::fromType<Enum>();
        bool ok = false;
        int value = enumeration.keyToValue(settings->value(name).toByteArray(), &ok);
        if (ok)
            result = static_cast<Enum>(value);
    }
    return result;
}

void QQuickMaterialStyle::init()
{
    static bool defaultsInitialized = false;
    if (!defaultsInitialized) {
        QSharedPointer<QSettings> settings = QQuickStyle::settings(QStringLiteral("Material"));
        if (!settings.isNull()) {
            defaultTheme = m_theme = readEnumValue<Theme>(settings.data(), QStringLiteral("Theme"), m_theme);
            defaultAccent = m_accent = readEnumValue<Color>(settings.data(), QStringLiteral("Accent"), m_accent);
            defaultPrimary = m_primary = readEnumValue<Color>(settings.data(), QStringLiteral("Primary"), m_primary);
        }
        defaultsInitialized = true;
    }

    QQuickStyle::init(); // TODO: lazy init?
}

QT_END_NAMESPACE
