// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickmaterialstyle_p.h"
#include "qquickmaterialtheme_p.h"

#include <QtCore/qdebug.h>
#if QT_CONFIG(settings)
#include <QtCore/qsettings.h>
#endif
#include <QtQml/qqmlinfo.h>
#include <QtQuickControls2/private/qquickstyle_p.h>

#include <algorithm>

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

// If no value was inherited from a parent or explicitly set, the "global" values are used.
// The initial, default values of the globals are hard-coded here, but the environment
// variables and .conf file override them if specified.
static QQuickMaterialStyle::Theme globalTheme = QQuickMaterialStyle::Light;
static uint globalPrimary = QQuickMaterialStyle::Indigo;
static uint globalAccent = QQuickMaterialStyle::Pink;
static uint globalForeground = 0xDD000000; // primaryTextColorLight
static uint globalBackground = 0xFFFAFAFA; // backgroundColorLight
// These represent whether a global foreground/background was set.
// Each style's m_hasForeground/m_hasBackground are initialized to these values.
static bool hasGlobalForeground = false;
static bool hasGlobalBackground = false;
// These represent whether or not the global color value was specified as one of the
// values that QColor accepts, as opposed to one of the pre-defined colors like Red.
static bool globalPrimaryCustom = false;
static bool globalAccentCustom = false;
static bool globalForegroundCustom = false;
static bool globalBackgroundCustom = false;
// This is global because:
// 1) The theme needs access to it to determine font sizes.
// 2) There can only be one variant used for the whole application.
static QQuickMaterialStyle::Variant globalVariant = QQuickMaterialStyle::Normal;
static const QRgb backgroundColorLight = 0xFFFFFBFE;
static const QRgb backgroundColorDark = 0xFF1C1B1F;
static const QRgb dialogColorLight = 0xFFFFFFFF;
static const QRgb dialogColorDark = 0xFF424242;
static const QRgb primaryTextColorLight = 0xDD000000;
static const QRgb primaryTextColorDark = 0xFFFFFFFF;
static const QRgb secondaryTextColorLight = 0x89000000;
static const QRgb secondaryTextColorDark = 0xB2FFFFFF;
static const QRgb hintTextColorLight = 0x60000000;
static const QRgb hintTextColorDark = 0x4CFFFFFF;
static const QRgb dividerColorLight = 0x1E000000;
static const QRgb dividerColorDark = 0x1EFFFFFF;
static const QRgb iconColorLight = 0x89000000;
static const QRgb iconColorDark = 0xFFFFFFFF;
static const QRgb iconDisabledColorLight = 0x42000000;
static const QRgb iconDisabledColorDark = 0x4CFFFFFF;
static const QRgb raisedButtonColorLight = 0xFFD6D7D7;
static const QRgb raisedButtonColorDark = 0x3FCCCCCC;
static const QRgb raisedButtonDisabledColorLight = dividerColorLight;
static const QRgb raisedButtonDisabledColorDark = dividerColorDark;
static const QRgb frameColorLight = hintTextColorLight;
static const QRgb frameColorDark = hintTextColorDark;
static const QRgb rippleColorLight = 0x10000000;
static const QRgb rippleColorDark = 0x20FFFFFF;
static const QRgb spinBoxDisabledIconColorLight = 0xFFCCCCCC;
static const QRgb spinBoxDisabledIconColorDark = 0xFF666666;
static const QRgb sliderDisabledColorLight = 0xFF9E9E9E;
static const QRgb sliderDisabledColorDark = 0xFF616161;
/*
    https://m3.material.io/components/switch/specs#57a434cd-5fcc-4d79-9bff-12b2a9768789

                     light  / dark
    surface:         #FFFBFE/#1C1B1F
    on-surface:      #1C1B1F/#E6E1E5
    surface-variant: #E7E0EC/#49454F

    12% = 1E
    38% = 61

    handle

                 unchecked                                     checked
    disabled     #1C1B1F/#E6E1E5 @ 38% (#611C1B1F/#61E6E1E5)   #FFFBFE/#1C1B1F @ 100%

    track

                 unchecked                                     checked
    disabled     #E7E0EC/#49454F @ 12% (#1EE7E0EC/#1E49454F)   #1C1B1F/#E6E1E5 @ 12% (#1E1C1B1F/#1EE6E1E5)

    track outline

                 unchecked                                     checked
    disabled     #1C1B1F/#E6E1E5 @ 12% (#1E1C1B1F/#1EE6E1E5)   same as track
*/
static const QRgb switchUncheckedTrackColorLight = 0xFFE7E0EC;
static const QRgb switchUncheckedTrackColorDark = 0x49454F;
static const QRgb switchDisabledUncheckedTrackColorLight = 0x1EE7E0EC;
static const QRgb switchDisabledUncheckedTrackColorDark = 0x1E49454F;
static const QRgb switchDisabledUncheckedTrackBorderColorLight = 0x1E1C1B1F;
static const QRgb switchDisabledUncheckedTrackBorderColorDark = 0x1EE6E1E5;
static const QRgb switchDisabledCheckedTrackColorLight = 0x1E1C1B1F;
static const QRgb switchDisabledCheckedTrackColorDark = 0x1EE6E1E5;
static const QRgb switchDisabledUncheckedIconColorLight = 0x611C1B1F;
static const QRgb switchDisabledUncheckedIconColorDark = 0x61E6E1E5;
static const QRgb textFieldFilledContainerColorLight = 0xFFE7E0EC;
static const QRgb textFieldFilledContainerColorDark = 0xFF49454F;

static QQuickMaterialStyle::Theme effectiveTheme(QQuickMaterialStyle::Theme theme)
{
    if (theme == QQuickMaterialStyle::System)
        theme = QQuickStylePrivate::isDarkSystemTheme() ? QQuickMaterialStyle::Dark : QQuickMaterialStyle::Light;
    return theme;
}

QQuickMaterialStyle::QQuickMaterialStyle(QObject *parent) : QQuickAttachedPropertyPropagator(parent),
    m_customPrimary(globalPrimaryCustom),
    m_customAccent(globalAccentCustom),
    m_customForeground(globalForegroundCustom),
    m_customBackground(globalBackgroundCustom),
    m_hasForeground(hasGlobalForeground),
    m_hasBackground(hasGlobalBackground),
    m_systemTheme(globalTheme == System),
    m_theme(effectiveTheme(globalTheme)),
    m_primary(globalPrimary),
    m_accent(globalAccent),
    m_foreground(globalForeground),
    m_background(globalBackground)
{
    QQuickAttachedPropertyPropagator::initialize();
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

    // If theme is System: m_theme is set to system's theme (Dark/Light)
    // and m_systemTheme is set to true.
    // If theme is Dark/Light: m_theme is set to the input theme (Dark/Light)
    // and m_systemTheme is set to false.
    const bool systemThemeChanged = (m_systemTheme != (theme == System));
    // Check m_theme and m_systemTheme are changed.
    if ((m_theme == effectiveTheme(theme)) && !systemThemeChanged)
        return;

    m_theme = effectiveTheme(theme);
    m_systemTheme = (theme == System);
    if (systemThemeChanged) {
        if (m_systemTheme)
            QQuickMaterialTheme::registerSystemStyle(this);
        else
            QQuickMaterialTheme::unregisterSystemStyle(this);
    }

    propagateTheme();
    themeChange();
    if (!m_customAccent)
        accentChange();
    if (!m_customBackground)
        backgroundChange();
    if (!m_customForeground)
        foregroundChange();
}

void QQuickMaterialStyle::inheritTheme(Theme theme)
{
    const bool systemThemeChanged = (m_systemTheme != (theme == System));
    const bool themeChanged = systemThemeChanged || (m_theme != effectiveTheme(theme));
    if (m_explicitTheme || !themeChanged)
        return;

    m_theme = effectiveTheme(theme);
    m_systemTheme = (theme == System);

    propagateTheme();
    themeChange();
    if (!m_customAccent)
        accentChange();
    if (!m_customBackground)
        backgroundChange();
    if (!m_customForeground)
        foregroundChange();
}

void QQuickMaterialStyle::propagateTheme()
{
    const auto styles = attachedChildren();
    for (QQuickAttachedPropertyPropagator *child : styles) {
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(child);
        if (material)
            // m_theme is the effective theme, either Dark or Light.
            // m_systemTheme indicates whether the theme is set by
            // the system (true) or manually (false).
            material->inheritTheme(m_systemTheme ? System : m_theme);
    }
}

void QQuickMaterialStyle::resetTheme()
{
    if (!m_explicitTheme)
        return;

    m_explicitTheme = false;
    QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(attachedParent());
    inheritTheme(material ? material->theme() : globalTheme);
}

void QQuickMaterialStyle::themeChange()
{
    emit themeChanged();
    emit themeOrAccentChanged();
    emit primaryHighlightedTextColor();
    emit dialogColorChanged();
    emit tooltipColorChanged();
    emit toolBarColorChanged();
    emit toolTextColorChanged();
}

QVariant QQuickMaterialStyle::primary() const
{
    return primaryColor();
}

void QQuickMaterialStyle::setPrimary(const QVariant &var)
{
    QRgb primary = 0;
    bool custom = false;
    if (!variantToRgba(var, "primary", &primary, &custom))
        return;

    m_explicitPrimary = true;
    if (m_primary == primary)
        return;

    m_customPrimary = custom;
    m_primary = primary;
    propagatePrimary();
    primaryChange();
}

void QQuickMaterialStyle::inheritPrimary(uint primary, bool custom)
{
    if (m_explicitPrimary || m_primary == primary)
        return;

    m_customPrimary = custom;
    m_primary = primary;
    propagatePrimary();
    primaryChange();
}

void QQuickMaterialStyle::propagatePrimary()
{
    const auto styles = attachedChildren();
    for (QQuickAttachedPropertyPropagator *child : styles) {
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(child);
        if (material)
            material->inheritPrimary(m_primary, m_customPrimary);
    }
}

void QQuickMaterialStyle::resetPrimary()
{
    if (!m_explicitPrimary)
        return;

    m_customPrimary = false;
    m_explicitPrimary = false;
    QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(attachedParent());
    if (material)
        inheritPrimary(material->m_primary, material->m_customPrimary);
    else
        inheritPrimary(globalPrimary, false);
}

void QQuickMaterialStyle::primaryChange()
{
    emit primaryChanged();
    emit toolBarColorChanged();
    emit toolTextColorChanged();
}

QVariant QQuickMaterialStyle::accent() const
{
    return accentColor();
}

void QQuickMaterialStyle::setAccent(const QVariant &var)
{
    QRgb accent = 0;
    bool custom = false;
    if (!variantToRgba(var, "accent", &accent, &custom))
        return;

    m_explicitAccent = true;
    if (m_accent == accent)
        return;

    m_customAccent = custom;
    m_accent = accent;
    propagateAccent();
    accentChange();
}

void QQuickMaterialStyle::inheritAccent(uint accent, bool custom)
{
    if (m_explicitAccent || m_accent == accent)
        return;

    m_customAccent = custom;
    m_accent = accent;
    propagateAccent();
    accentChange();
}

void QQuickMaterialStyle::propagateAccent()
{
    const auto styles = attachedChildren();
    for (QQuickAttachedPropertyPropagator *child : styles) {
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(child);
        if (material)
            material->inheritAccent(m_accent, m_customAccent);
    }
}

void QQuickMaterialStyle::resetAccent()
{
    if (!m_explicitAccent)
        return;

    m_customAccent = false;
    m_explicitAccent = false;
    QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(attachedParent());
    if (material)
        inheritAccent(material->m_accent, material->m_customAccent);
    else
        inheritAccent(globalAccent, false);
}

void QQuickMaterialStyle::accentChange()
{
    emit accentChanged();
    emit themeOrAccentChanged();
}

QVariant QQuickMaterialStyle::foreground() const
{
    if (!m_hasForeground) {
        if (!m_customBackground && m_background == m_primary) {
            return toolTextColor();
        }
         return QColor::fromRgba(m_theme == Light ? primaryTextColorLight : primaryTextColorDark);
    }
    if (m_customForeground)
        return QColor::fromRgba(m_foreground);
    if (m_foreground > BlueGrey)
        return QColor();
    return QColor::fromRgba(colors[m_foreground][Shade500]);
}

void QQuickMaterialStyle::setForeground(const QVariant &var)
{
    QRgb foreground = 0;
    bool custom = false;
    if (!variantToRgba(var, "foreground", &foreground, &custom))
        return;

    // Reset the foreground if the new color matches the current theme's default.
    // This allows the color to update dynamically when the theme changes.
    if (!hasGlobalForeground) {
        const QRgb themeDefault = (m_theme == Light ? primaryTextColorLight : primaryTextColorDark);
        if (foreground == themeDefault) {
            resetForeground();
            return;
        }
    }

    m_hasForeground = true;
    m_explicitForeground = true;
    if (m_foreground == foreground)
        return;

    m_customForeground = custom;
    m_foreground = foreground;
    propagateForeground();
    foregroundChange();
}

void QQuickMaterialStyle::inheritForeground(uint foreground, bool custom, bool has)
{
    if (m_explicitForeground || m_foreground == foreground)
        return;

    m_hasForeground = has;
    m_customForeground = custom;
    m_foreground = foreground;
    propagateForeground();
    foregroundChange();
}

void QQuickMaterialStyle::propagateForeground()
{
    const auto styles = attachedChildren();
    for (QQuickAttachedPropertyPropagator *child : styles) {
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(child);
        if (material)
            material->inheritForeground(m_foreground, m_customForeground, m_hasForeground);
    }
}

void QQuickMaterialStyle::resetForeground()
{
    if (!m_explicitForeground)
        return;

    m_hasForeground = false;
    m_customForeground = false;
    m_explicitForeground = false;
    QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(attachedParent());
    inheritForeground(material ? material->m_foreground : globalForeground, true, material ? material->m_hasForeground : false);
}

void QQuickMaterialStyle::foregroundChange()
{
    emit foregroundChanged();
    emit primaryHighlightedTextColorChanged();
}

QVariant QQuickMaterialStyle::background() const
{
    return backgroundColor();
}

void QQuickMaterialStyle::setBackground(const QVariant &var)
{
    QRgb background = 0;
    bool custom = false;
    if (!variantToRgba(var, "background", &background, &custom))
        return;

    // Reset the background if the new color matches the current theme's default.
    // This allows the color to update dynamically when the theme changes.
    if (!hasGlobalBackground) {
        const QRgb themeDefault = (m_theme == Light ? backgroundColorLight : backgroundColorDark);
        if (background == themeDefault) {
            resetBackground();
            return;
        }
    }

    // Do not treat the background as custom if it resolves to the same RGB as the
    // current primary color. This allows the foreground color to map to
    // toolTextColor, which is resolved dynamically based on the primary color.
    if (custom && background == primaryColor().rgb()) {
        background = m_primary;
        custom = false;
    }

    m_hasBackground = true;
    m_explicitBackground = true;
    if (m_background == background)
        return;

    m_customBackground = custom;
    m_background = background;
    propagateBackground();
    backgroundChange();

    // Foreground color depends on the background color if the background is set to primary.
    if (!m_customBackground && m_background == m_primary)
        foregroundChange();
}

void QQuickMaterialStyle::inheritBackground(uint background, bool custom, bool has)
{
    if (m_explicitBackground || m_background == background)
        return;

    m_hasBackground = has;
    m_customBackground = custom;
    m_background = background;
    propagateBackground();
    backgroundChange();
    if (!m_customBackground && m_background == m_primary)
        foregroundChange();
}

void QQuickMaterialStyle::propagateBackground()
{
    const auto styles = attachedChildren();
    for (QQuickAttachedPropertyPropagator *child : styles) {
        QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(child);
        if (material)
            material->inheritBackground(m_background, m_customBackground, m_hasBackground);
    }
}

void QQuickMaterialStyle::resetBackground()
{
    if (!m_explicitBackground)
        return;

    m_hasBackground = false;
    m_customBackground = false;
    m_explicitBackground = false;
    QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(attachedParent());
    inheritBackground(material ? material->m_background : globalBackground, true, material ? material->m_hasBackground : false);
}

void QQuickMaterialStyle::backgroundChange()
{
    emit backgroundChanged();
    emit dialogColorChanged();
    emit tooltipColorChanged();
    emit toolBarColorChanged();
}

int QQuickMaterialStyle::elevation() const
{
    return m_elevation;
}

void QQuickMaterialStyle::setElevation(int elevation)
{
    if (m_elevation == elevation)
        return;

    m_elevation = elevation;
    elevationChange();
}

void QQuickMaterialStyle::resetElevation()
{
    setElevation(0);
}

void QQuickMaterialStyle::elevationChange()
{
    emit elevationChanged();
}

QQuickMaterialStyle::RoundedScale QQuickMaterialStyle::roundedScale() const
{
    return m_roundedScale;
}

void QQuickMaterialStyle::setRoundedScale(RoundedScale roundedScale)
{
    if (m_roundedScale == roundedScale)
        return;

    m_roundedScale = roundedScale;
    emit roundedScaleChanged();
}

void QQuickMaterialStyle::resetRoundedScale()
{
    setRoundedScale(RoundedScale::NotRounded);
}

QQuickMaterialStyle::ContainerStyle QQuickMaterialStyle::containerStyle() const
{
    return m_containerStyle;
}

void QQuickMaterialStyle::setContainerStyle(ContainerStyle containerStyle)
{
    if (m_containerStyle == containerStyle)
        return;

    m_containerStyle = containerStyle;
    emit containerStyleChanged();
}

void QQuickMaterialStyle::resetContainerStyle()
{
    setContainerStyle(ContainerStyle::Filled);
}

QColor QQuickMaterialStyle::primaryColor() const
{
    if (m_customPrimary)
        return QColor::fromRgba(m_primary);
    if (m_primary > BlueGrey)
        return QColor();
    return colors[m_primary][Shade500];
}

QColor QQuickMaterialStyle::accentColor(Shade shade) const
{
    if (m_customAccent)
        return shade == themeShade() ? QColor::fromRgba(m_accent)
                                     : this->shade(QColor::fromRgba(m_accent), shade);
    if (m_accent > BlueGrey)
        return QColor();
    return colors[m_accent][shade];
}

QColor QQuickMaterialStyle::accentColor() const
{
    return accentColor(themeShade());
}

QColor QQuickMaterialStyle::backgroundColor(Shade shade) const
{
    if (!m_hasBackground)
        return QColor::fromRgba(m_theme == Light ? backgroundColorLight : backgroundColorDark);
    // The application bars (such as ToolBar) set the background color from the primary color,
    // and in that case, it's better to validate with the custom primary and use it accordingly.
    if (m_customBackground || (m_customPrimary && m_background == m_primary))
        return shade == themeShade() ? QColor::fromRgba(m_background)
                                     : this->shade(QColor::fromRgba(m_background), shade);
    if (m_background > BlueGrey)
        return QColor();
    return colors[m_background][shade];
}

QColor QQuickMaterialStyle::backgroundColor() const
{
    return backgroundColor(themeShade());
}

QColor QQuickMaterialStyle::primaryTextColor() const
{
    return QColor::fromRgba(m_theme == Light ? primaryTextColorLight : primaryTextColorDark);
}

QColor QQuickMaterialStyle::primaryHighlightedTextColor() const
{
    if (m_explicitForeground)
        return primaryTextColor();
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
    color.setAlphaF(0.4f);
    return color;
}

QColor QQuickMaterialStyle::dropShadowColor() const
{
    return QColor::fromRgba(0x40000000);
}

QColor QQuickMaterialStyle::dividerColor() const
{
    return QColor::fromRgba(m_theme == Light ? dividerColorLight : dividerColorDark);
}

QColor QQuickMaterialStyle::iconColor() const
{
    return QColor::fromRgba(m_theme == Light ? iconColorLight : iconColorDark);
}

QColor QQuickMaterialStyle::iconDisabledColor() const
{
    return QColor::fromRgba(m_theme == Light ? iconDisabledColorLight : iconDisabledColorDark);
}

QColor QQuickMaterialStyle::buttonColor(Theme theme, const QVariant &background, const QVariant &accent,
    bool enabled, bool flat, bool highlighted, bool checked) const
{
    if (!enabled && !flat) {
        return QColor::fromRgba(m_theme == Light
            ? raisedButtonDisabledColorLight : raisedButtonDisabledColorDark);
    }

    // We don't use theme (and other arguments) here even though we pass it in, as it's
    // simpler to just re-use themeShade. We still need the arguments because they allow
    // us to be re-called whenever they change.
    Shade shade = themeShade();
    Q_UNUSED(theme);
    Q_UNUSED(background);
    Q_UNUSED(accent);

    QColor color = Qt::transparent;

    if (m_explicitBackground) {
        color = backgroundColor(shade);
    } else if (highlighted) {
        if (m_theme == Light) {
            color = accentColor(shade);
            if (checked)
                color = color.lighter();
        } else {
            // A highlighted + checked button should become darker.
            color = accentColor(checked ? Shade100 : shade);
        }
        // Flat, highlighted buttons need to have a semi-transparent background,
        // otherwise the text won't be visible.
        if (flat)
            color.setAlphaF(0.25);
    } else if (!flat) {
        // Even if the elevation is zero, it should still have a background if it's not flat.
        color = QColor::fromRgba(m_theme == Light ? raisedButtonColorLight
                                                  : raisedButtonColorDark);
    }

    return color;
}

QColor QQuickMaterialStyle::frameColor() const
{
    return QColor::fromRgba(m_theme == Light ? frameColorLight : frameColorDark);
}

QColor QQuickMaterialStyle::rippleColor() const
{
    return QColor::fromRgba(m_theme == Light ? rippleColorLight : rippleColorDark);
}

QColor QQuickMaterialStyle::highlightedRippleColor() const
{
    QColor pressColor = accentColor();
    pressColor.setAlpha(m_theme == Light ? 30 : 50);
    return pressColor;
}

QColor QQuickMaterialStyle::switchUncheckedTrackColor() const
{
    return QColor::fromRgba(m_theme == Light ? switchUncheckedTrackColorLight : switchUncheckedTrackColorDark);
}

QColor QQuickMaterialStyle::switchCheckedTrackColor() const
{
    return accentColor(m_theme == Light ? themeShade() : Shade100);
}

QColor QQuickMaterialStyle::switchDisabledUncheckedTrackColor() const
{
    return QColor::fromRgba(m_theme == Light
        ? switchDisabledUncheckedTrackColorLight : switchDisabledUncheckedTrackColorDark);
}

QColor QQuickMaterialStyle::switchDisabledCheckedTrackColor() const
{
    return QColor::fromRgba(m_theme == Light
        ? switchDisabledCheckedTrackColorLight : switchDisabledCheckedTrackColorDark);
}

QColor QQuickMaterialStyle::switchDisabledUncheckedTrackBorderColor() const
{
    return QColor::fromRgba(m_theme == Light
        ? switchDisabledUncheckedTrackBorderColorLight : switchDisabledUncheckedTrackBorderColorDark);
}

QColor QQuickMaterialStyle::switchUncheckedHandleColor() const
{
    return m_theme == Light ? color(Grey, Shade600) : color(Grey, Shade400);
}

QColor QQuickMaterialStyle::switchUncheckedHoveredHandleColor() const
{
    const QColor color = switchUncheckedHandleColor();
    return m_theme == Light ? color.darker(140) : color.lighter(120);
}

QColor QQuickMaterialStyle::switchCheckedHandleColor() const
{
    return m_theme == Light ? QColor::fromRgb(0xFFFFFF) : accentColor(Shade800);
}

QColor QQuickMaterialStyle::switchDisabledUncheckedHandleColor() const
{
    if (m_theme == Light)
        return QColor::fromRgba(0x611C1B1F);

    QColor darkHandleColor = color(Grey, Shade800);
    darkHandleColor.setAlphaF(0.38f);
    return darkHandleColor;
}

QColor QQuickMaterialStyle::switchDisabledCheckedHandleColor() const
{
    return QColor::fromRgb(m_theme == Light ? 0xFFFBFE : 0x1C1B1F);
}

QColor QQuickMaterialStyle::switchDisabledCheckedIconColor() const
{
    return QColor::fromRgba(m_theme == Light ? 0x611C1B1F : 0x61E6E1E5);
}

QColor QQuickMaterialStyle::switchDisabledUncheckedIconColor() const
{
    return QColor::fromRgba(m_theme == Light
        ? switchDisabledUncheckedIconColorLight : switchDisabledUncheckedIconColorDark);
}

QColor QQuickMaterialStyle::scrollBarColor() const
{
    return QColor::fromRgba(m_theme == Light ? 0x40000000 : 0x40FFFFFF);
}

QColor QQuickMaterialStyle::scrollBarHoveredColor() const
{
    return QColor::fromRgba(m_theme == Light ? 0x60000000 : 0x60FFFFFF);
}

QColor QQuickMaterialStyle::scrollBarPressedColor() const
{
    return QColor::fromRgba(m_theme == Light ? 0x80000000 : 0x80FFFFFF);
}

QColor QQuickMaterialStyle::dialogColor() const
{
    if (m_hasBackground)
        return backgroundColor();
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

QColor QQuickMaterialStyle::tooltipColor() const
{
    if (m_explicitBackground)
        return backgroundColor();
    return color(Grey, Shade700);
}

QColor QQuickMaterialStyle::toolBarColor() const
{
    if (m_explicitBackground)
        return backgroundColor();
    return primaryColor();
}

QColor QQuickMaterialStyle::toolTextColor() const
{
    if (m_hasForeground || m_customPrimary)
        return primaryTextColor();

    switch (m_primary) {
    case Red:
    case Pink:
    case Purple:
    case DeepPurple:
    case Indigo:
    case Blue:
    case Teal:
    case DeepOrange:
    case Brown:
    case BlueGrey:
        return QColor::fromRgba(primaryTextColorDark);

    case LightBlue:
    case Cyan:
    case Green:
    case LightGreen:
    case Lime:
    case Yellow:
    case Amber:
    case Orange:
    case Grey:
        return QColor::fromRgba(primaryTextColorLight);

    default:
        break;
    }

    return primaryTextColor();
}

QColor QQuickMaterialStyle::spinBoxDisabledIconColor() const
{
    return QColor::fromRgba(m_theme == Light ? spinBoxDisabledIconColorLight : spinBoxDisabledIconColorDark);
}

QColor QQuickMaterialStyle::sliderDisabledColor() const
{
    return QColor::fromRgba(m_theme == Light ? sliderDisabledColorLight : sliderDisabledColorDark);
}

QColor QQuickMaterialStyle::textFieldFilledContainerColor() const
{
    return QColor::fromRgba(m_theme == Light ? textFieldFilledContainerColorLight : textFieldFilledContainerColorDark);
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
    hsl.setHslF(hsl.hueF(), hsl.saturationF(),
                std::clamp(hsl.lightnessF() + amount, qreal(0.0), qreal(1.0)), color.alphaF());
    return hsl.convertTo(color.spec());
}

static QColor darkerShade(const QColor &color, qreal amount)
{
    QColor hsl = color.toHsl();
    hsl.setHslF(hsl.hueF(), hsl.saturationF(),
                std::clamp(hsl.lightnessF() - amount, qreal(0.0), qreal(1.0)), color.alphaF());
    return hsl.convertTo(color.spec());
}

QQuickMaterialStyle::Shade QQuickMaterialStyle::themeShade() const
{
    return m_theme == Light ? Shade500 : Shade200;
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

// Returns the same color, if shade == themeShade()
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
        Q_UNREACHABLE_RETURN(QColor());
    }
}

int QQuickMaterialStyle::touchTarget() const
{
    // https://material.io/guidelines/components/buttons.html#buttons-style
    return globalVariant == Dense ? 44 : 48;
}

int QQuickMaterialStyle::buttonVerticalPadding() const
{
    return globalVariant == Dense ? 10 : 14;
}

// https://m3.material.io/components/buttons/specs#256326ad-f934-40e7-b05f-0bcb41aa4382
int QQuickMaterialStyle::buttonLeftPadding(bool flat, bool hasIcon) const
{
    static const int noIconPadding = globalVariant == Dense ? 12 : 24;
    static const int iconPadding = globalVariant == Dense ? 8 : 16;
    static const int flatPadding = globalVariant == Dense ? 6 : 12;
    return !flat ? (!hasIcon ? noIconPadding : iconPadding) : flatPadding;
}

int QQuickMaterialStyle::buttonRightPadding(bool flat, bool hasIcon, bool hasText) const
{
    static const int noTextPadding = globalVariant == Dense ? 8 : 16;
    static const int textPadding = globalVariant == Dense ? 12 : 24;
    static const int flatNoIconPadding = globalVariant == Dense ? 6 : 12;
    static const int flatNoTextPadding = globalVariant == Dense ? 6 : 12;
    static const int flatTextPadding = globalVariant == Dense ? 8 : 16;
    return !flat
        ? (!hasText ? noTextPadding : textPadding)
        : (!hasIcon ? flatNoIconPadding : (!hasText ? flatNoTextPadding : flatTextPadding));
}

int QQuickMaterialStyle::buttonHeight() const
{
    // https://m3.material.io/components/buttons/specs#256326ad-f934-40e7-b05f-0bcb41aa4382
    return globalVariant == Dense ? 32 : 40;
}

int QQuickMaterialStyle::delegateHeight() const
{
    // https://material.io/guidelines/components/lists.html#lists-specs
    return globalVariant == Dense ? 40 : 48;
}

int QQuickMaterialStyle::dialogButtonBoxHeight() const
{
    return globalVariant == Dense ? 48 : 52;
}

int QQuickMaterialStyle::dialogTitleFontPixelSize() const
{
    return globalVariant == Dense ? 16 : 24;
}

// https://m3.material.io/components/dialogs/specs#6771d107-624e-47cc-b6d8-2b7b620ba2f1
QQuickMaterialStyle::RoundedScale QQuickMaterialStyle::dialogRoundedScale() const
{
    return globalVariant == Dense
        ? QQuickMaterialStyle::RoundedScale::LargeScale
        : QQuickMaterialStyle::RoundedScale::ExtraLargeScale;
}

int QQuickMaterialStyle::frameVerticalPadding() const
{
    return globalVariant == Dense ? 8 : 12;
}

int QQuickMaterialStyle::menuItemHeight() const
{
    // https://material.io/guidelines/components/menus.html#menus-simple-menus
    return globalVariant == Dense ? 32 : 48;
}

int QQuickMaterialStyle::menuItemVerticalPadding() const
{
    return globalVariant == Dense ? 8 : 12;
}

int QQuickMaterialStyle::switchIndicatorWidth() const
{
    return globalVariant == Dense ? 40 : 52;
}

int QQuickMaterialStyle::switchIndicatorHeight() const
{
    return globalVariant == Dense ? 22 : 32;
}

int QQuickMaterialStyle::switchNormalHandleHeight() const
{
    return globalVariant == Dense ? 10 : 16;
}

int QQuickMaterialStyle::switchCheckedHandleHeight() const
{
    return globalVariant == Dense ? 16 : 24;
}

int QQuickMaterialStyle::switchLargestHandleHeight() const
{
    return globalVariant == Dense ? 18 : 28;
}

int QQuickMaterialStyle::switchDelegateVerticalPadding() const
{
    // SwitchDelegate's indicator is much larger than the others due to the shadow,
    // so we must reduce its padding to ensure its implicitHeight is 40 when dense.
    return globalVariant == Dense ? 4 : 8;
}

int QQuickMaterialStyle::textFieldHeight() const
{
    // filled: https://m3.material.io/components/text-fields/specs#8c032848-e442-46df-b25d-28f1315f234b
    // outlined: https://m3.material.io/components/text-fields/specs#605e24f1-1c1f-4c00-b385-4bf50733a5ef
    return globalVariant == Dense ? 44 : 56;
}
int QQuickMaterialStyle::textFieldHorizontalPadding() const
{
    return globalVariant == Dense ? 12 : 16;
}
int QQuickMaterialStyle::textFieldVerticalPadding() const
{
    return globalVariant == Dense ? 4 : 8;
}

int QQuickMaterialStyle::tooltipHeight() const
{
    // https://material.io/guidelines/components/tooltips.html
    return globalVariant == Dense ? 22 : 32;
}

QQuickMaterialStyle::Variant QQuickMaterialStyle::variant()
{
    return globalVariant;
}

template <typename Enum>
static Enum toEnumValue(const QByteArray &value, bool *ok)
{
    QMetaEnum enumeration = QMetaEnum::fromType<Enum>();
    return static_cast<Enum>(enumeration.keyToValue(value, ok));
}

static QByteArray resolveSetting(const QByteArray &env, const QSharedPointer<QSettings> &settings, const QString &name)
{
    QByteArray value = qgetenv(env);
#if QT_CONFIG(settings)
    if (value.isNull() && !settings.isNull())
        value = settings->value(name).toByteArray();
#endif
    return value;
}

void QQuickMaterialStyle::initGlobals()
{
    QSharedPointer<QSettings> settings = QQuickStylePrivate::settings(QStringLiteral("Material"));

    bool ok = false;
    QByteArray themeValue = resolveSetting("QT_QUICK_CONTROLS_MATERIAL_THEME", settings, QStringLiteral("Theme"));
    Theme themeEnum = toEnumValue<Theme>(themeValue, &ok);
    if (ok)
        globalTheme = themeEnum;
    else if (!themeValue.isEmpty())
        qWarning().nospace().noquote() << "Material: unknown theme value: " << themeValue;

    QByteArray variantValue = resolveSetting("QT_QUICK_CONTROLS_MATERIAL_VARIANT", settings, QStringLiteral("Variant"));
    Variant variantEnum = toEnumValue<Variant>(variantValue, &ok);
    if (ok)
        globalVariant = variantEnum;
    else if (!variantValue.isEmpty())
        qWarning().nospace().noquote() << "Material: unknown variant value: " << variantValue;

    QByteArray primaryValue = resolveSetting("QT_QUICK_CONTROLS_MATERIAL_PRIMARY", settings, QStringLiteral("Primary"));
    Color primaryEnum = toEnumValue<Color>(primaryValue, &ok);
    if (ok) {
        globalPrimaryCustom = false;
        globalPrimary = primaryEnum;
    } else {
        QColor color = QColor::fromString(primaryValue);
        if (color.isValid()) {
            globalPrimaryCustom = true;
            globalPrimary = color.rgba();
        } else if (!primaryValue.isEmpty()) {
            qWarning().nospace().noquote() << "Material: unknown primary value: " << primaryValue;
        }
    }

    QByteArray accentValue = resolveSetting("QT_QUICK_CONTROLS_MATERIAL_ACCENT", settings, QStringLiteral("Accent"));
    Color accentEnum = toEnumValue<Color>(accentValue, &ok);
    if (ok) {
        globalAccentCustom = false;
        globalAccent = accentEnum;
    } else if (!accentValue.isEmpty()) {
        QColor color = QColor::fromString(accentValue);
        if (color.isValid()) {
            globalAccentCustom = true;
            globalAccent = color.rgba();
        } else {
            qWarning().nospace().noquote() << "Material: unknown accent value: " << accentValue;
        }
    }

    QByteArray foregroundValue = resolveSetting("QT_QUICK_CONTROLS_MATERIAL_FOREGROUND", settings, QStringLiteral("Foreground"));
    Color foregroundEnum = toEnumValue<Color>(foregroundValue, &ok);
    if (ok) {
        globalForegroundCustom = false;
        globalForeground = foregroundEnum;
        hasGlobalForeground = true;
    } else if (!foregroundValue.isEmpty()) {
        QColor color = QColor::fromString(foregroundValue);
        if (color.isValid()) {
            globalForegroundCustom = true;
            globalForeground = color.rgba();
            hasGlobalForeground = true;
        } else {
            qWarning().nospace().noquote() << "Material: unknown foreground value: " << foregroundValue;
        }
    }

    QByteArray backgroundValue = resolveSetting("QT_QUICK_CONTROLS_MATERIAL_BACKGROUND", settings, QStringLiteral("Background"));
    Color backgroundEnum = toEnumValue<Color>(backgroundValue, &ok);
    if (ok) {
        globalBackgroundCustom = false;
        globalBackground = backgroundEnum;
        hasGlobalBackground = true;
    } else if (!backgroundValue.isEmpty()) {
        QColor color = QColor::fromString(backgroundValue);
        if (color.isValid()) {
            globalBackgroundCustom = true;
            globalBackground = color.rgba();
            hasGlobalBackground = true;
        } else {
            qWarning().nospace().noquote() << "Material: unknown background value: " << backgroundValue;
        }
    }
}

void QQuickMaterialStyle::attachedParentChange(QQuickAttachedPropertyPropagator *newParent, QQuickAttachedPropertyPropagator *oldParent)
{
    Q_UNUSED(oldParent);
    QQuickMaterialStyle *material = qobject_cast<QQuickMaterialStyle *>(newParent);
    if (material) {
        inheritPrimary(material->m_primary, material->m_customPrimary);
        inheritAccent(material->m_accent, material->m_customAccent);
        inheritForeground(material->m_foreground, material->m_customForeground, material->m_hasForeground);
        inheritBackground(material->m_background, material->m_customBackground, material->m_hasBackground);
        inheritTheme(material->theme());
    }
}

bool QQuickMaterialStyle::variantToRgba(const QVariant &var, const char *name, QRgb *rgba, bool *custom) const
{
    *custom = false;
    if (var.metaType().id() == QMetaType::Int) {
        int val = var.toInt();
        if (val > BlueGrey) {
            qmlWarning(parent()) << "unknown Material." << name << " value: " << val;
            return false;
        }
        *rgba = val;
    } else {
        int val = QMetaEnum::fromType<Color>().keyToValue(var.toByteArray());
        if (val != -1) {
            *rgba = val;
        } else {
            QColor color = QColor::fromString(var.toString());
            if (!color.isValid()) {
                qmlWarning(parent()) << "unknown Material." << name << " value: " << var.toString();
                return false;
            }
            *custom = true;
            *rgba = color.rgba();
        }
    }
    return true;
}

QT_END_NAMESPACE

#include "moc_qquickmaterialstyle_p.cpp"
