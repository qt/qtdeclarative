// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtQuick/private/qquickpalette_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#include "qqstylekittheme_p.h"
#include "qqstylekitstyle_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Theme
    \inqmlmodule Qt.labs.StyleKit
    \inherits AbstractStyle
    \brief Defines color and style overrides for a color scheme.

    A Theme overrides properties defined in a \l Style to provide a
    distinct visual appearance for a particular color scheme. A style
    typically configures structural properties such as implicit size,
    padding, and radii, while a theme specifies colors, shadows, and
    other visual attributes. However, this is not a restriction — a
    theme can override any property that a style can set.

    A style can provide a \l {Style::light}{light} and a
    \l {Style::dark}{dark} theme. When \l {Style::themeName}{themeName}
    is set to \c "System" (the default), the active theme follows the
    \l {QStyleHints::colorScheme}{operating system's color scheme}.
    You can also create additional themes using \l CustomTheme and
    \l {Style::themeName}{switch between them} at runtime.

    Properties that are not set in the theme fall back to those defined
    in the \l Style.

    The following example defines light, dark, and high-contrast
    themes, each providing different colors for the controls:

    \snippet ThemeSnippets.qml themes

    For a complete example showing themes in action, see the
    \l {StyleKit Example}.

    \labs

    \sa Style, CustomTheme
*/

static QQuickTheme::Scope scopeForType(QQStyleKitExtendableControlType type)
{
    switch (type) {
    case QQStyleKitReader::ControlType::AbstractButton:
    case QQStyleKitReader::ControlType::Button:
    case QQStyleKitReader::ControlType::FlatButton:
        return QQuickTheme::Button;
    case QQStyleKitReader::ControlType::CheckBox:
        return QQuickTheme::CheckBox;
    case QQStyleKitReader::ControlType::ComboBox:
        return QQuickTheme::ComboBox;
    case QQStyleKitReader::ControlType::GroupBox:
        return QQuickTheme::GroupBox;
    case QQStyleKitReader::ControlType::ItemDelegate:
        return QQuickTheme::ItemView;
    case QQStyleKitReader::ControlType::Label:
        return QQuickTheme::Label;
    case QQStyleKitReader::ControlType::RadioButton:
        return QQuickTheme::RadioButton;
    case QQStyleKitReader::ControlType::SpinBox:
        return QQuickTheme::SpinBox;
    case QQStyleKitReader::ControlType::SwitchControl:
        return QQuickTheme::Switch;
    case QQStyleKitReader::ControlType::TabBar:
    case QQStyleKitReader::ControlType::TabButton:
        return QQuickTheme::TabBar;
    case QQStyleKitReader::ControlType::TextArea:
        return QQuickTheme::TextArea;
    case QQStyleKitReader::ControlType::TextInput:
    case QQStyleKitReader::ControlType::TextField:
        return QQuickTheme::TextField;
    case QQStyleKitReader::ControlType::ToolBar:
    case QQStyleKitReader::ControlType::ToolButton:
    case QQStyleKitReader::ControlType::ToolSeparator:
        return QQuickTheme::ToolBar;
    default:
        return QQuickTheme::System;
    }
    Q_UNREACHABLE();
}

QQStyleKitTheme::QQStyleKitTheme(QObject *parent)
    : QQStyleKitStyleAndThemeBase(parent)
{
}

QQStyleKitStyle *QQStyleKitTheme::style() const
{
    QObject *parentObj = parent();
    if (!parentObj)
        return nullptr;
    Q_ASSERT(qobject_cast<QQStyleKitStyle *>(parentObj));
    return static_cast<QQStyleKitStyle *>(parentObj);
}

QPalette QQStyleKitTheme::paletteForControlType(QQStyleKitExtendableControlType type) const
{
    const QQuickTheme::Scope scope = scopeForType(type);
    return m_effectivePalettes[int(scope)];
}

QFont QQStyleKitTheme::fontForControlType(QQStyleKitExtendableControlType type) const
{
    const QQuickTheme::Scope scope = scopeForType(type);
    return m_effectiveFonts[int(scope)];
}

void QQStyleKitTheme::updateThemePalettes()
{
    const QQStyleKitPalette *pals = palettes();
    if (!pals)
        return;

    // Collect palette fallback chain
    QVector<const QQStyleKitPalette *> fbChain;
    for (auto *fb = pals; fb; fb = fb->fallbackPalette())
        fbChain.append(fb);

    auto resolveFromPaletteChain = [&](QQuickTheme::Scope scope,
        QQuickPalette* (QQStyleKitPalette::*getter)() const) -> QPalette
    {
        QPalette result;

        // Apply from farthest fallback -> local
        for (int i = fbChain.size() - 1; i >= 0; --i) {
            const QQStyleKitPalette *fb = fbChain[i];
            // Apply system palette first as a base, and override with scope-specific palette
            if (scope != QQuickTheme::System && fb->isSet(QQuickTheme::System)) {
                if (auto *sys = fb->system())
                    result = sys->toQPalette().resolve(result);
            }
            if (fb->isSet(scope)) {
                if (auto *p = (fb->*getter)())
                    result = p->toQPalette().resolve(result);
            }
        }

        return result;
    };

    auto setResolved = [&](QQuickPalette* (QQStyleKitPalette::*getter)() const,
        QQuickTheme::Scope scope)
    {
        m_effectivePalettes[int(scope)] = resolveFromPaletteChain(scope, getter);
    };

    setResolved(&QQStyleKitPalette::system, QQuickTheme::System);
    setResolved(&QQStyleKitPalette::button, QQuickTheme::Button);
    setResolved(&QQStyleKitPalette::checkBox, QQuickTheme::CheckBox);
    setResolved(&QQStyleKitPalette::comboBox, QQuickTheme::ComboBox);
    setResolved(&QQStyleKitPalette::groupBox, QQuickTheme::GroupBox);
    setResolved(&QQStyleKitPalette::itemDelegate, QQuickTheme::ItemView);
    setResolved(&QQStyleKitPalette::label, QQuickTheme::Label);
    setResolved(&QQStyleKitPalette::radioButton, QQuickTheme::RadioButton);
    setResolved(&QQStyleKitPalette::spinBox, QQuickTheme::SpinBox);
    setResolved(&QQStyleKitPalette::tabBar, QQuickTheme::TabBar);
    setResolved(&QQStyleKitPalette::textArea, QQuickTheme::TextArea);
    setResolved(&QQStyleKitPalette::textField, QQuickTheme::TextField);
    setResolved(&QQStyleKitPalette::toolBar, QQuickTheme::ToolBar);
}

void QQStyleKitTheme::updateThemeFonts()
{
    const QQStyleKitFont *fonts = this->fonts();
    if (!fonts)
        return;

    // Collect font fallback chain
    QVector<const QQStyleKitFont *> fbChain;
    for (auto *fb = fonts; fb; fb = fb->fallbackFont())
        fbChain.append(fb);

    auto resolveFromFontChain = [&](QQuickTheme::Scope scope) -> QFont
    {
        QFont result;

        // Apply from farthest fallback -> local
        for (int i = fbChain.size() - 1; i >= 0; --i) {
            const QQStyleKitFont *fb = fbChain[i];
            // Apply system font first as a base, and override with scope-specific font
            if (scope != QQuickTheme::System && fb->isSet(QQuickTheme::System)) {
                result = fb->fontForScope(QQuickTheme::System).resolve(result);
            }
            if (fb->isSet(scope)) {
                result = fb->fontForScope(scope).resolve(result);
            }
        }

        return result;
    };

    for (int i = 0; i < NScopes; ++i) {
        const QQuickTheme::Scope scope = static_cast<QQuickTheme::Scope>(i);
        m_effectiveFonts[i] = resolveFromFontChain(scope);
    }
}

void QQStyleKitTheme::componentComplete()
{
    QQStyleKitControls::componentComplete();
    m_completed = true;
}

QT_END_NAMESPACE

#include "moc_qqstylekittheme_p.cpp"
