// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfluentwinui3theme_p.h"

#include <QtCore/qoperatingsystemversion.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/qguiapplication.h>
#include <QtGui/qstylehints.h>
#include <QtGui/qcolor.h>
#include <QtGui/qfontdatabase.h>
#include <QtQuickTemplates2/private/qquicktheme_p.h>

QT_BEGIN_NAMESPACE

// If on a Windows11 or above, the platform theme will be used to populate the palette
// We need to fallback to hardcoded colors when using the style on other platforms,
// that's why we need the following
// The colors for Windows 11 are taken from the official WinUI3 Figma style at
// https://www.figma.com/community/file/1159947337437047524
// Try to keep these consistent with the widget windows11 style
enum WinUI3Color {
    solidBackground,                    // Solid background color used for the bottom most layer
    acrylicBackgroundDefault,           // Acrylic background for popups/tooltips
    textPrimary,                        // Color for UI labels and static text
    textSecondary,                      // Color for text in pressed controls
    textDisabled,                       // Color for disabled text
    textOnAccentPrimary,                // Color of text on controls filled in accent color
    textOnAccentSecondary,              // Color of text on sunken controls in accent color
    textOnAccentDisabled,               // Color of text on disabled controls in accent color
    controlDefault,                     // Color for standard controls
    controlDisabled,                    // Color for disabled controls
    controlStrokeDefault,               // Color for gradient stops in elevations (pressed or disabled state)
    controlStrokeSecondary,             // Color for gradient stops in elevations
    controlStrokeAccentDefault,         // Color for gradient stops in elevations for accent controls
    controlStrokeAccentSecondary,       // Color for gradient stops in elevations for accent controls
    accentDefault,                      // Default color for accent fills on controls
    accentDisabled,                     // Default color for accent fills on disabled controls
    accentSecondary,                    // Color for accent fills on hovered controls
};

const static QColor WINUI3ColorsLight [] {
    QColor(0xF3,0xF3,0xF3,0xFF), //solidBackgroundColor
    QColor(0xFC,0xFC,0xFC,0xFF), //acrylicBackgroundDefault
    QColor(0x00,0x00,0x00,0xE4), //textPrimary
    QColor(0x00,0x00,0x00,0x9E), //textSecondary
    QColor(0x00,0x00,0x00,0x5C), //textDisabled
    QColor(0xFF,0xFF,0xFF,0xFF), //textOnAccentPrimary
    QColor(0xFF,0xFF,0xFF,0x7F), //textOnAccentSecondary
    QColor(0xFF,0xFF,0xFF,0xFF), //textOnAccentDisabled
    QColor(0xFF,0xFF,0xFF,0xB3), //controlDefault
    QColor(0xF9,0xF9,0xF9,0x4D), //controlDisabled
    QColor(0x00,0x00,0x00,0x0F), //controlStrokeDefault
    QColor(0x00,0x00,0x00,0x29), //controlStrokeSecondary
    QColor(0xFF,0xFF,0xFF,0x14), //controlStrokeAccentDefault
    QColor(0x00,0x00,0x00,0x66), //controlStrokeAccentSecondary
    QColor(0x00,0x5F,0xB8,0xFF), //accentDefault
    QColor(0x00,0x00,0x00,0x37), //accentDisabled
    QColor(0x00,0x5F,0xB8,0xE6), //accentSecondary
};

const static QColor WINUI3ColorsDark[] {
    QColor(0x20,0x20,0x20,0xFF), //solidBackgroundColor
    QColor(0x2C,0x2C,0x2C,0xFF), //acrylicBackgroundDefault
    QColor(0xFF,0xFF,0xFF,0xFF), //textPrimary
    QColor(0xFF,0xFF,0xFF,0xC5), //textSecondary
    QColor(0xFF,0xFF,0xFF,0x5D), //textDisabled
    QColor(0x00,0x00,0x00,0xFF), //textOnAccentPrimary
    QColor(0x00,0x00,0x00,0x80), //textOnAccentSecondary
    QColor(0xFF,0xFF,0xFF,0x87), //textOnAccentDisabled
    QColor(0xFF,0xFF,0xFF,0x0F), //controlDefault
    QColor(0xFF,0xFF,0xFF,0x11), //controlDisabled
    QColor(0xFF,0xFF,0xFF,0x12), //controlStrokeDefault
    QColor(0xFF,0xFF,0xFF,0x18), //controlStrokeSecondary
    QColor(0xFF,0xFF,0xFF,0x14), //controlStrokeAccentDefault
    QColor(0x00,0x00,0x00,0x23), //controlStrokeAccentSecondary
    QColor(0x60,0xCD,0xFF,0xFF), //accentDefault
    QColor(0xFF,0xFF,0xFF,0x28), //accentDisabled
    QColor(0x60,0xCD,0xFF,0xE6) // accentSecondary
};

const static QColor* WINUI3Colors[] {
    WINUI3ColorsLight,
    WINUI3ColorsDark
};

static void populateSystemPalette(QPalette &palette)
{
    const auto colorSchemeIndex = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light ? 0 : 1;

    palette.setColor(QPalette::All, QPalette::Window, WINUI3Colors[colorSchemeIndex][solidBackground]);

    palette.setColor(QPalette::All, QPalette::WindowText, WINUI3Colors[colorSchemeIndex][textPrimary]);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, WINUI3Colors[colorSchemeIndex][textDisabled]);

    palette.setColor(QPalette::All, QPalette::Text, WINUI3Colors[colorSchemeIndex][textPrimary]);
    palette.setColor(QPalette::Disabled, QPalette::Text, WINUI3Colors[colorSchemeIndex][textDisabled]);

    palette.setColor(QPalette::All, QPalette::PlaceholderText, WINUI3Colors[colorSchemeIndex][textSecondary]);
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, WINUI3Colors[colorSchemeIndex][textDisabled]);

    palette.setColor(QPalette::All, QPalette::Button, WINUI3Colors[colorSchemeIndex][controlDefault]);
    palette.setColor(QPalette::Disabled, QPalette::Button, WINUI3Colors[colorSchemeIndex][controlDisabled]);
    palette.setColor(QPalette::All, QPalette::ButtonText, WINUI3Colors[colorSchemeIndex][textPrimary]);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, WINUI3Colors[colorSchemeIndex][textDisabled]);

    palette.setColor(QPalette::All, QPalette::ToolTipBase, WINUI3Colors[colorSchemeIndex][acrylicBackgroundDefault]);
    palette.setColor(QPalette::All, QPalette::ToolTipText, WINUI3Colors[colorSchemeIndex][textPrimary]);
    palette.setColor(QPalette::Disabled, QPalette::ToolTipText, WINUI3Colors[colorSchemeIndex][textDisabled]);

    palette.setColor(QPalette::Disabled, QPalette::Accent, WINUI3Colors[colorSchemeIndex][accentDisabled]);
    palette.setColor(QPalette::Disabled, QPalette::Highlight, WINUI3Colors[colorSchemeIndex][accentDisabled]);

    palette.setColor(QPalette::All, QPalette::HighlightedText, Qt::white);

    palette.setColor(QPalette::All, QPalette::Light, WINUI3Colors[colorSchemeIndex][controlStrokeAccentDefault]);
    palette.setColor(QPalette::All, QPalette::Midlight, WINUI3Colors[colorSchemeIndex][controlStrokeDefault]);
    palette.setColor(QPalette::All, QPalette::Dark, WINUI3Colors[colorSchemeIndex][controlStrokeSecondary]);
    palette.setColor(QPalette::All, QPalette::Mid, WINUI3Colors[colorSchemeIndex][controlStrokeAccentSecondary]);
}

static void populateThemeFont(QQuickTheme *theme)
{
    QFont systemFont;
    QFont toolBarFont;
    QFont toolTipFont;

    const QLatin1String segoeUiFamilyName("Segoe UI Variable");
    if (QFontDatabase::families().contains(segoeUiFamilyName)) {
        const QFont segoeFont(segoeUiFamilyName);
        const QStringList families{segoeFont.family()};
        systemFont.setFamilies(families);
        toolBarFont.setFamilies(families);
    }
    systemFont.setWeight(QFont::Weight::Normal);
    toolBarFont.setWeight(QFont::Weight::Normal);
    toolTipFont.setWeight(QFont::Weight::Normal);

    systemFont.setPixelSize(14);
    toolBarFont.setPixelSize(12);
    toolTipFont.setPixelSize(12);

    theme->setFont(QQuickTheme::System, systemFont);
    theme->setFont(QQuickTheme::ToolBar, toolBarFont);
    theme->setFont(QQuickTheme::ToolTip, toolTipFont);
}

void QQuickFluentWinUI3Theme::updatePalette(QPalette &palette)
{
    populateSystemPalette(palette);
}

void QQuickFluentWinUI3Theme::initialize(QQuickTheme *theme)
{
    populateThemeFont(theme);
    QPalette systemPalette;
    updatePalette(systemPalette);

    if (auto platformTheme = QGuiApplicationPrivate::platformTheme()) {
        const auto platformPalette = platformTheme->palette();
        if (platformPalette)
            // style palette takes precedence over platform's theme
            systemPalette = systemPalette.resolve(*platformPalette);
    }

    {
        const auto colorSchemeIndex = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Light ? 0 : 1;
        if (!systemPalette.isBrushSet(QPalette::Active, QPalette::Accent))
            systemPalette.setColor(QPalette::Active, QPalette::Accent, WINUI3Colors[colorSchemeIndex][accentDefault]);

        systemPalette.setColor(QPalette::Active, QPalette::Highlight, systemPalette.accent().color());
        systemPalette.setColor(QPalette::Inactive, QPalette::Accent, systemPalette.accent().color());
        systemPalette.setColor(QPalette::Inactive, QPalette::Highlight, systemPalette.highlight().color());
    }

    // Finally QGuiApp::palette() should take precedence over style palette
    systemPalette = QGuiApplication::palette().resolve(systemPalette);
    theme->setPalette(QQuickTheme::System, systemPalette);
}

QT_END_NAMESPACE
