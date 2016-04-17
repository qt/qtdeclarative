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

#include <QtCore/qdebug.h>
#include <QtCore/qsettings.h>
#include <QtQml/qqmlinfo.h>
#include <QtLabsControls/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

static const QRgb colors[][14] = {
    // Red
    {
        0xFFFFEBEE, // Shade50
        0xFFFFCDD2, // Shade100
        0xFFEF9A9A, // Shade200
        0xFFE57373, // Shade300
        0xFFEF5350, // Shade400
        0xFFF44336, // Shade500
        0xFFE53935, // Shade600
        0xFFD32F2F, // Shade700
        0xFFC62828, // Shade800
        0xFFB71C1C, // Shade900
        0xFFFF8A80, // ShadeA100
        0xFFFF5252, // ShadeA200
        0xFFFF1744, // ShadeA400
        0xFFD50000  // ShadeA700
    },
    // Pink
    {
        0xFFFCE4EC, // Shade50
        0xFFF8BBD0, // Shade100
        0xFFF48FB1, // Shade200
        0xFFF06292, // Shade300
        0xFFEC407A, // Shade400
        0xFFE91E63, // Shade500
        0xFFD81B60, // Shade600
        0xFFC2185B, // Shade700
        0xFFAD1457, // Shade800
        0xFF880E4F, // Shade900
        0xFFFF80AB, // ShadeA100
        0xFFFF4081, // ShadeA200
        0xFFF50057, // ShadeA400
        0xFFC51162  // ShadeA700
    },
    // Purple
    {
        0xFFF3E5F5, // Shade50
        0xFFE1BEE7, // Shade100
        0xFFCE93D8, // Shade200
        0xFFBA68C8, // Shade300
        0xFFAB47BC, // Shade400
        0xFF9C27B0, // Shade500
        0xFF8E24AA, // Shade600
        0xFF7B1FA2, // Shade700
        0xFF6A1B9A, // Shade800
        0xFF4A148C, // Shade900
        0xFFEA80FC, // ShadeA100
        0xFFE040FB, // ShadeA200
        0xFFD500F9, // ShadeA400
        0xFFAA00FF  // ShadeA700
    },
    // DeepPurple
    {
        0xFFEDE7F6, // Shade50
        0xFFD1C4E9, // Shade100
        0xFFB39DDB, // Shade200
        0xFF9575CD, // Shade300
        0xFF7E57C2, // Shade400
        0xFF673AB7, // Shade500
        0xFF5E35B1, // Shade600
        0xFF512DA8, // Shade700
        0xFF4527A0, // Shade800
        0xFF311B92, // Shade900
        0xFFB388FF, // ShadeA100
        0xFF7C4DFF, // ShadeA200
        0xFF651FFF, // ShadeA400
        0xFF6200EA  // ShadeA700
    },
    // Indigo
    {
        0xFFE8EAF6, // Shade50
        0xFFC5CAE9, // Shade100
        0xFF9FA8DA, // Shade200
        0xFF7986CB, // Shade300
        0xFF5C6BC0, // Shade400
        0xFF3F51B5, // Shade500
        0xFF3949AB, // Shade600
        0xFF303F9F, // Shade700
        0xFF283593, // Shade800
        0xFF1A237E, // Shade900
        0xFF8C9EFF, // ShadeA100
        0xFF536DFE, // ShadeA200
        0xFF3D5AFE, // ShadeA400
        0xFF304FFE  // ShadeA700
    },
    // Blue
    {
        0xFFE3F2FD, // Shade50
        0xFFBBDEFB, // Shade100
        0xFF90CAF9, // Shade200
        0xFF64B5F6, // Shade300
        0xFF42A5F5, // Shade400
        0xFF2196F3, // Shade500
        0xFF1E88E5, // Shade600
        0xFF1976D2, // Shade700
        0xFF1565C0, // Shade800
        0xFF0D47A1, // Shade900
        0xFF82B1FF, // ShadeA100
        0xFF448AFF, // ShadeA200
        0xFF2979FF, // ShadeA400
        0xFF2962FF  // ShadeA700
    },
    // LightBlue
    {
        0xFFE1F5FE, // Shade50
        0xFFB3E5FC, // Shade100
        0xFF81D4FA, // Shade200
        0xFF4FC3F7, // Shade300
        0xFF29B6F6, // Shade400
        0xFF03A9F4, // Shade500
        0xFF039BE5, // Shade600
        0xFF0288D1, // Shade700
        0xFF0277BD, // Shade800
        0xFF01579B, // Shade900
        0xFF80D8FF, // ShadeA100
        0xFF40C4FF, // ShadeA200
        0xFF00B0FF, // ShadeA400
        0xFF0091EA  // ShadeA700
    },
    // Cyan
    {
        0xFFE0F7FA, // Shade50
        0xFFB2EBF2, // Shade100
        0xFF80DEEA, // Shade200
        0xFF4DD0E1, // Shade300
        0xFF26C6DA, // Shade400
        0xFF00BCD4, // Shade500
        0xFF00ACC1, // Shade600
        0xFF0097A7, // Shade700
        0xFF00838F, // Shade800
        0xFF006064, // Shade900
        0xFF84FFFF, // ShadeA100
        0xFF18FFFF, // ShadeA200
        0xFF00E5FF, // ShadeA400
        0xFF00B8D4  // ShadeA700
    },
    // Teal
    {
        0xFFE0F2F1, // Shade50
        0xFFB2DFDB, // Shade100
        0xFF80CBC4, // Shade200
        0xFF4DB6AC, // Shade300
        0xFF26A69A, // Shade400
        0xFF009688, // Shade500
        0xFF00897B, // Shade600
        0xFF00796B, // Shade700
        0xFF00695C, // Shade800
        0xFF004D40, // Shade900
        0xFFA7FFEB, // ShadeA100
        0xFF64FFDA, // ShadeA200
        0xFF1DE9B6, // ShadeA400
        0xFF00BFA5  // ShadeA700
    },
    // Green
    {
        0xFFE8F5E9, // Shade50
        0xFFC8E6C9, // Shade100
        0xFFA5D6A7, // Shade200
        0xFF81C784, // Shade300
        0xFF66BB6A, // Shade400
        0xFF4CAF50, // Shade500
        0xFF43A047, // Shade600
        0xFF388E3C, // Shade700
        0xFF2E7D32, // Shade800
        0xFF1B5E20, // Shade900
        0xFFB9F6CA, // ShadeA100
        0xFF69F0AE, // ShadeA200
        0xFF00E676, // ShadeA400
        0xFF00C853  // ShadeA700
    },
    // LightGreen
    {
        0xFFF1F8E9, // Shade50
        0xFFDCEDC8, // Shade100
        0xFFC5E1A5, // Shade200
        0xFFAED581, // Shade300
        0xFF9CCC65, // Shade400
        0xFF8BC34A, // Shade500
        0xFF7CB342, // Shade600
        0xFF689F38, // Shade700
        0xFF558B2F, // Shade800
        0xFF33691E, // Shade900
        0xFFCCFF90, // ShadeA100
        0xFFB2FF59, // ShadeA200
        0xFF76FF03, // ShadeA400
        0xFF64DD17  // ShadeA700
    },
    // Lime
    {
        0xFFF9FBE7, // Shade50
        0xFFF0F4C3, // Shade100
        0xFFE6EE9C, // Shade200
        0xFFDCE775, // Shade300
        0xFFD4E157, // Shade400
        0xFFCDDC39, // Shade500
        0xFFC0CA33, // Shade600
        0xFFAFB42B, // Shade700
        0xFF9E9D24, // Shade800
        0xFF827717, // Shade900
        0xFFF4FF81, // ShadeA100
        0xFFEEFF41, // ShadeA200
        0xFFC6FF00, // ShadeA400
        0xFFAEEA00  // ShadeA700
    },
    // Yellow
    {
        0xFFFFFDE7, // Shade50
        0xFFFFF9C4, // Shade100
        0xFFFFF59D, // Shade200
        0xFFFFF176, // Shade300
        0xFFFFEE58, // Shade400
        0xFFFFEB3B, // Shade500
        0xFFFDD835, // Shade600
        0xFFFBC02D, // Shade700
        0xFFF9A825, // Shade800
        0xFFF57F17, // Shade900
        0xFFFFFF8D, // ShadeA100
        0xFFFFFF00, // ShadeA200
        0xFFFFEA00, // ShadeA400
        0xFFFFD600  // ShadeA700
    },
    // Amber
    {
        0xFFFFF8E1, // Shade50
        0xFFFFECB3, // Shade100
        0xFFFFE082, // Shade200
        0xFFFFD54F, // Shade300
        0xFFFFCA28, // Shade400
        0xFFFFC107, // Shade500
        0xFFFFB300, // Shade600
        0xFFFFA000, // Shade700
        0xFFFF8F00, // Shade800
        0xFFFF6F00, // Shade900
        0xFFFFE57F, // ShadeA100
        0xFFFFD740, // ShadeA200
        0xFFFFC400, // ShadeA400
        0xFFFFAB00  // ShadeA700
    },
    // Orange
    {
        0xFFFFF3E0, // Shade50
        0xFFFFE0B2, // Shade100
        0xFFFFCC80, // Shade200
        0xFFFFB74D, // Shade300
        0xFFFFA726, // Shade400
        0xFFFF9800, // Shade500
        0xFFFB8C00, // Shade600
        0xFFF57C00, // Shade700
        0xFFEF6C00, // Shade800
        0xFFE65100, // Shade900
        0xFFFFD180, // ShadeA100
        0xFFFFAB40, // ShadeA200
        0xFFFF9100, // ShadeA400
        0xFFFF6D00  // ShadeA700
    },
    // DeepOrange
    {
        0xFFFBE9E7, // Shade50
        0xFFFFCCBC, // Shade100
        0xFFFFAB91, // Shade200
        0xFFFF8A65, // Shade300
        0xFFFF7043, // Shade400
        0xFFFF5722, // Shade500
        0xFFF4511E, // Shade600
        0xFFE64A19, // Shade700
        0xFFD84315, // Shade800
        0xFFBF360C, // Shade900
        0xFFFF9E80, // ShadeA100
        0xFFFF6E40, // ShadeA200
        0xFFFF3D00, // ShadeA400
        0xFFDD2C00  // ShadeA700
    },
    // Brown
    {
        0xFFEFEBE9, // Shade50
        0xFFD7CCC8, // Shade100
        0xFFBCAAA4, // Shade200
        0xFFA1887F, // Shade300
        0xFF8D6E63, // Shade400
        0xFF795548, // Shade500
        0xFF6D4C41, // Shade600
        0xFF5D4037, // Shade700
        0xFF4E342E, // Shade800
        0xFF3E2723, // Shade900
        0xFF000000, // ShadeA100
        0xFF000000, // ShadeA200
        0xFF000000, // ShadeA400
        0xFF000000  // ShadeA700
    },
    // Grey
    {
        0xFFFAFAFA, // Shade50
        0xFFF5F5F5, // Shade100
        0xFFEEEEEE, // Shade200
        0xFFE0E0E0, // Shade300
        0xFFBDBDBD, // Shade400
        0xFF9E9E9E, // Shade500
        0xFF757575, // Shade600
        0xFF616161, // Shade700
        0xFF424242, // Shade800
        0xFF212121, // Shade900
        0xFF000000, // ShadeA100
        0xFF000000, // ShadeA200
        0xFF000000, // ShadeA400
        0xFF000000  // ShadeA700
    },
    // BlueGrey
    {
        0xFFECEFF1, // Shade50
        0xFFCFD8DC, // Shade100
        0xFFB0BEC5, // Shade200
        0xFF90A4AE, // Shade300
        0xFF78909C, // Shade400
        0xFF607D8B, // Shade500
        0xFF546E7A, // Shade600
        0xFF455A64, // Shade700
        0xFF37474F, // Shade800
        0xFF263238, // Shade900
        0xFF000000, // ShadeA100
        0xFF000000, // ShadeA200
        0xFF000000, // ShadeA400
        0xFF000000  // ShadeA700
    }
};

static QQuickMaterialStyle::Theme defaultTheme = QQuickMaterialStyle::Light;
static uint defaultPrimary = QQuickMaterialStyle::Indigo;
static uint defaultAccent = QQuickMaterialStyle::Pink;
static bool defaultPrimaryCustom = false;
static bool defaultAccentCustom = false;
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

static QColor alphaBlend(const QColor &bg, const QColor &fg)
{
    QColor result;
    result.setRedF(fg.redF() * fg.alphaF() + bg.redF() * (1.0 - fg.alphaF()));
    result.setGreenF(fg.greenF() * fg.alphaF() + bg.greenF() * (1.0 - fg.alphaF()));
    result.setBlueF(fg.blueF() * fg.alphaF() + bg.blueF() * (1.0 - fg.alphaF()));
    result.setAlphaF(bg.alphaF() + fg.alphaF() * (1.0 - bg.alphaF()));
    return result;
}

QQuickMaterialStyle::QQuickMaterialStyle(QObject *parent) : QQuickStyle(parent),
    m_explicitTheme(false),
    m_explicitPrimary(false),
    m_explicitAccent(false),
    m_customPrimary(defaultPrimaryCustom),
    m_customAccent(defaultAccentCustom),
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

QVariant QQuickMaterialStyle::primary() const
{
    return primaryColor();
}

void QQuickMaterialStyle::setPrimary(const QVariant &var)
{
    QRgb primary = 0;
    bool custom = false;
    if (var.type() == QVariant::Int) {
        int val = var.toInt();
        if (val > BlueGrey) {
            qmlInfo(parent()) << "unknown Material.primary value: " << val;
            return;
        }
        primary = val;
    } else {
        int val = QMetaEnum::fromType<Color>().keyToValue(var.toByteArray());
        if (val != -1) {
            primary = val;
        } else {
            QColor color(var.toString());
            if (!color.isValid()) {
                qmlInfo(parent()) << "unknown Material.primary value: " << var.toString();
                return;
            }
            custom = true;
            primary = color.rgba();
        }
    }

    m_explicitPrimary = true;
    if (m_primary != primary) {
        m_customPrimary = custom;
        m_primary = primary;
        propagatePrimary();
        emit primaryChanged();
        emit paletteChanged();
    }
}

void QQuickMaterialStyle::inheritPrimary(uint primary, bool custom)
{
    if (!m_explicitPrimary && m_primary != primary) {
        m_customPrimary = custom;
        m_primary = primary;
        propagatePrimary();
        emit primaryChanged();
        emit paletteChanged();
    }
}

void QQuickMaterialStyle::propagatePrimary()
{
    foreach (QQuickStyle *child, childStyles()) {
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(child);
        if (material)
            material->inheritPrimary(m_primary, m_customPrimary);
    }
}

void QQuickMaterialStyle::resetPrimary()
{
    if (m_explicitPrimary) {
        m_customPrimary = false;
        m_explicitPrimary = false;
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(parentStyle());
        inheritPrimary(material ? material->m_primary : defaultPrimary, true);
    }
}

QVariant QQuickMaterialStyle::accent() const
{
    return accentColor();
}

void QQuickMaterialStyle::setAccent(const QVariant &var)
{
    QRgb accent = 0;
    bool custom = false;
    if (var.type() == QVariant::Int) {
        int val = var.toInt();
        if (val > BlueGrey) {
            qmlInfo(parent()) << "unknown Material.accent value: " << val;
            return;
        }
        accent = val;
    } else {
        int val = QMetaEnum::fromType<Color>().keyToValue(var.toByteArray());
        if (val != -1) {
            accent = val;
        } else {
            QColor color(var.toString());
            if (!color.isValid()) {
                qmlInfo(parent()) << "unknown Material.accent value: " << var.toString();
                return;
            }
            custom = true;
            accent = color.rgba();
        }
    }

    m_explicitAccent = true;
    if (m_accent != accent) {
        m_customAccent = custom;
        m_accent = accent;
        propagateAccent();
        emit accentChanged();
        emit paletteChanged();
    }
}

void QQuickMaterialStyle::inheritAccent(uint accent, bool custom)
{
    if (!m_explicitAccent && m_accent != accent) {
        m_customAccent = custom;
        m_accent = accent;
        propagateAccent();
        emit accentChanged();
        emit paletteChanged();
    }
}

void QQuickMaterialStyle::propagateAccent()
{
    foreach (QQuickStyle *child, childStyles()) {
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(child);
        if (material)
            material->inheritAccent(m_accent, m_customAccent);
    }
}

void QQuickMaterialStyle::resetAccent()
{
    if (m_explicitAccent) {
        m_customAccent = false;
        m_explicitAccent = false;
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(parentStyle());
        inheritAccent(material ? material->m_accent : defaultAccent, true);
    }
}

QColor QQuickMaterialStyle::primaryColor() const
{
    if (m_customPrimary)
        return QColor::fromRgba(m_primary);
    if (m_primary > BlueGrey)
        return QColor();
    return colors[m_primary][Shade500];
}

QColor QQuickMaterialStyle::accentColor() const
{
    if (m_customAccent)
        return QColor::fromRgba(m_accent);
    if (m_accent > BlueGrey)
        return QColor();
    return colors[m_accent][m_theme == Light ? Shade500 : Shade200];
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
    // Add overlaying black shadow 12% opacity
    return alphaBlend(accentColor(), QColor::fromRgba(0x1F000000));
}

QColor QQuickMaterialStyle::raisedHighlightedButtonPressColor() const
{
    // Add overlaying black shadow 12% opacity
    return alphaBlend(shade(accentColor(), m_theme == Light ? Shade700 : Shade100), QColor::fromRgba(0x1F000000));
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
    QColor pressColor = accentColor();
    // TODO: find out actual value
    pressColor.setAlpha(m_theme == Light ? 30 : 50);
    return pressColor;
}

QColor QQuickMaterialStyle::switchUncheckedTrackColor() const
{
    return QColor::fromRgba(m_theme == Light ? switchUncheckedTrackColorLight : switchUncheckedTrackColorDark);
}

QColor QQuickMaterialStyle::switchCheckedTrackColor() const
{
    QColor trackColor = m_theme == Light ? accentColor() : shade(accentColor(), Shade200);
    trackColor.setAlphaF(0.5);
    return trackColor;
}

QColor QQuickMaterialStyle::switchUncheckedHandleColor() const
{
    return m_theme == Light ? color(Grey, Shade50) : color(Grey, Shade400);
}

QColor QQuickMaterialStyle::switchCheckedHandleColor() const
{
    return m_theme == Light ? accentColor() : shade(accentColor(), Shade200);
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

QColor QQuickMaterialStyle::listHighlightColor() const
{
    return QColor::fromRgba(m_theme == Light ? 0x1e000000 : 0x1effffff);
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

static QColor lighterShade(const QColor &color, qreal amount)
{
    QColor hsl = color.toHsl();
    hsl.setHslF(hsl.hueF(), hsl.saturationF(), qBound<qreal>(0.0, hsl.lightnessF() + amount, 1.0), color.alphaF());
    return hsl.convertTo(color.spec());
}

QColor darkerShade(const QColor &color, qreal amount)
{
    QColor hsl = color.toHsl();
    hsl.setHslF(hsl.hueF(), hsl.saturationF(), qBound<qreal>(0.0, hsl.lightnessF() - amount, 1.0), color.alphaF());
    return hsl.convertTo(color.spec());
}

/*
 * The following lightness values originate from the Material Design Color Generator project.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 mbitson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
QColor QQuickMaterialStyle::shade(const QColor &color, Shade shade) const
{
    switch (shade) {
    case Shade50:
        return lighterShade(color, m_theme == Light ? 0.52 : 0.26);
    case Shade100:
        return lighterShade(color, m_theme == Light ? 0.37 : 0.11);
    case Shade200:
        return m_theme == Light ? lighterShade(color, 0.26) : color;
    case Shade300:
        return m_theme == Light ? lighterShade(color, 0.12) : darkerShade(color, 0.14);
    case Shade400:
        return m_theme == Light ? lighterShade(color, 0.06) : darkerShade(color, 0.20);
    case Shade500:
        return m_theme == Light ? color : darkerShade(color, 0.26);
    case Shade600:
        return darkerShade(color, m_theme == Light ? 0.06 : 0.32);
    case Shade700:
        return darkerShade(color, m_theme == Light ? 0.12 : 0.38);
    case Shade800:
        return darkerShade(color, m_theme == Light ? 0.18 : 0.44);
    case Shade900:
        return darkerShade(color, m_theme == Light ? 0.24 : 0.50);
    case ShadeA100:
        return lighterShade(color, m_theme == Light ? 0.54 : 0.28);
    case ShadeA200:
        return lighterShade(color, m_theme == Light ? 0.37 : 0.11);
    case ShadeA400:
        return m_theme == Light ? lighterShade(color, 0.06) : darkerShade(color, 0.20);
    case ShadeA700:
        return darkerShade(color, m_theme == Light ? 0.12 : 0.38);
    default:
        Q_UNREACHABLE();
        return QColor();
    }
}

void QQuickMaterialStyle::parentStyleChange(QQuickStyle *newParent, QQuickStyle *oldParent)
{
    Q_UNUSED(oldParent);
    QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(newParent);
    if (material) {
        inheritPrimary(material->m_primary, material->m_customPrimary);
        inheritAccent(material->m_accent, material->m_customAccent);
        inheritTheme(material->theme());
    }
}

template <typename Enum>
static Enum toEnumValue(const QByteArray &value, bool *ok)
{
    QMetaEnum enumeration = QMetaEnum::fromType<Enum>();
    return static_cast<Enum>(enumeration.keyToValue(value, ok));
}

void QQuickMaterialStyle::init()
{
    static bool defaultsInitialized = false;
    if (!defaultsInitialized) {
        QSharedPointer<QSettings> settings = QQuickStyle::settings(QStringLiteral("Material"));
        if (!settings.isNull()) {
            bool ok = false;
            QByteArray value = settings->value(QStringLiteral("Theme")).toByteArray();
            Theme theme = toEnumValue<Theme>(value, &ok);
            if (ok)
                defaultTheme = m_theme = theme;
            else if (!value.isEmpty())
                qWarning().nospace().noquote() << settings->fileName() << ": unknown Material theme value: " << value;

            value = settings->value(QStringLiteral("Primary")).toByteArray();
            Color primary = toEnumValue<Color>(value, &ok);
            if (ok) {
                defaultPrimaryCustom = m_customPrimary = false;
                defaultPrimary = m_primary = primary;
            } else {
                QColor color(value.constData());
                if (color.isValid()) {
                    defaultPrimaryCustom = m_customPrimary = true;
                    defaultPrimary = m_primary = color.rgba();
                } else if (!value.isEmpty()) {
                    qWarning().nospace().noquote() << settings->fileName() << ": unknown Material primary value: " << value;
                }
            }

            value = settings->value(QStringLiteral("Accent")).toByteArray();
            Color accent = toEnumValue<Color>(value, &ok);
            if (ok) {
                defaultAccentCustom = m_customAccent = false;
                defaultAccent = m_accent = accent;
            } else {
                QColor color(value.constData());
                if (color.isValid()) {
                    defaultAccentCustom = m_customAccent = true;
                    defaultAccent = m_accent = color.rgba();
                } else if (!value.isEmpty()) {
                    qWarning().nospace().noquote() << settings->fileName() << ": unknown Material accent value: " << value;
                }
            }
        }
        defaultsInitialized = true;
    }

    QQuickStyle::init(); // TODO: lazy init?
}

QT_END_NAMESPACE
