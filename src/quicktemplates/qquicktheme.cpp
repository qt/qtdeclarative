// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicktheme_p.h"
#include "qquicktheme_p_p.h"

#include <QtGui/qpa/qplatformtheme.h>
#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

std::unique_ptr<QQuickTheme> QQuickThemePrivate::instance;

static void cleanup_instance()
{
    QQuickThemePrivate::instance.reset();
}

static void install_instance_cleanuper()
{
    qAddPostRoutine(cleanup_instance);
}

Q_COREAPP_STARTUP_FUNCTION(install_instance_cleanuper)

static QPlatformTheme::Font platformFont(QQuickTheme::Scope scope)
{
    switch (scope) {
    case QQuickTheme::Button: return QPlatformTheme::PushButtonFont;
    case QQuickTheme::CheckBox: return QPlatformTheme::CheckBoxFont;
    case QQuickTheme::ComboBox: return QPlatformTheme::ComboMenuItemFont;
    case QQuickTheme::GroupBox: return QPlatformTheme::GroupBoxTitleFont;
    case QQuickTheme::ItemView: return QPlatformTheme::ItemViewFont;
    case QQuickTheme::Label: return QPlatformTheme::LabelFont;
    case QQuickTheme::ListView: return QPlatformTheme::ListViewFont;
    case QQuickTheme::Menu: return QPlatformTheme::MenuFont;
    case QQuickTheme::MenuBar: return QPlatformTheme::MenuBarFont;
    case QQuickTheme::RadioButton: return QPlatformTheme::RadioButtonFont;
    case QQuickTheme::SpinBox: return QPlatformTheme::EditorFont;
    case QQuickTheme::Switch: return QPlatformTheme::CheckBoxFont;
    case QQuickTheme::TabBar: return QPlatformTheme::TabButtonFont;
    case QQuickTheme::TextArea: return QPlatformTheme::EditorFont;
    case QQuickTheme::TextField: return QPlatformTheme::EditorFont;
    case QQuickTheme::ToolBar: return QPlatformTheme::ToolButtonFont;
    case QQuickTheme::ToolTip: return QPlatformTheme::TipLabelFont;
    case QQuickTheme::Tumbler: return QPlatformTheme::ItemViewFont;
    default: return QPlatformTheme::SystemFont;
    }
}

static QPlatformTheme::Palette platformPalette(QQuickTheme::Scope scope)
{
    switch (scope) {
    case QQuickTheme::Button: return QPlatformTheme::ButtonPalette;
    case QQuickTheme::CheckBox: return QPlatformTheme::CheckBoxPalette;
    case QQuickTheme::ComboBox: return QPlatformTheme::ComboBoxPalette;
    case QQuickTheme::GroupBox: return QPlatformTheme::GroupBoxPalette;
    case QQuickTheme::ItemView: return QPlatformTheme::ItemViewPalette;
    case QQuickTheme::Label: return QPlatformTheme::LabelPalette;
    case QQuickTheme::ListView: return QPlatformTheme::ItemViewPalette;
    case QQuickTheme::Menu: return QPlatformTheme::MenuPalette;
    case QQuickTheme::MenuBar: return QPlatformTheme::MenuBarPalette;
    case QQuickTheme::RadioButton: return QPlatformTheme::RadioButtonPalette;
    case QQuickTheme::SpinBox: return QPlatformTheme::TextLineEditPalette;
    case QQuickTheme::Switch: return QPlatformTheme::CheckBoxPalette;
    case QQuickTheme::TabBar: return QPlatformTheme::TabBarPalette;
    case QQuickTheme::TextArea: return QPlatformTheme::TextEditPalette;
    case QQuickTheme::TextField: return QPlatformTheme::TextLineEditPalette;
    case QQuickTheme::ToolBar: return QPlatformTheme::ToolButtonPalette;
    case QQuickTheme::ToolTip: return QPlatformTheme::ToolTipPalette;
    case QQuickTheme::Tumbler: return QPlatformTheme::ItemViewPalette;
    default: return QPlatformTheme::SystemPalette;
    }
}

QQuickTheme::QQuickTheme()
    : d_ptr(new QQuickThemePrivate)
{
}

QQuickTheme::~QQuickTheme()
{
}

QQuickTheme *QQuickTheme::instance()
{
    return QQuickThemePrivate::instance.get();
}

QFont QQuickTheme::font(Scope scope)
{
    const QFont *font = nullptr;
    if (QQuickTheme *theme = instance())
        font = QQuickThemePrivate::get(theme)->fonts[scope].data();
    else if (QPlatformTheme *theme = QGuiApplicationPrivate::platformTheme())
        font = theme->font(platformFont(scope));

    if (font) {
        QFont f = *font;
        if (scope == System)
            f.setResolveMask(0);
        return f;
    }

    if (scope != System)
        return QQuickTheme::font(System);

    return QFont();
}

QPalette QQuickTheme::palette(Scope scope)
{
    const QPalette *palette = nullptr;

    if (auto theme = instance()) {
        if (theme->usePlatformPalette()) {
            if (auto platformTheme = QGuiApplicationPrivate::platformTheme()) {
                palette = platformTheme->palette(platformPalette(scope));
                // In case, if palettes are provided through configuration file
                // (qtquickcontrols2.conf), then respect configuration palette
                // and resolve it with platform palette
                if (palette) {
                    QQuickThemePrivate *p = QQuickThemePrivate::get(theme);
                    if (p->defaultPalette && p->defaultPalette->resolveMask() != 0) {
                        QPalette defPalette = *p->defaultPalette;
                        defPalette.resolve(*palette);
                        if (scope == System)
                            defPalette.setResolveMask(0);
                        return defPalette;
                    }
                }
            }
        } else {
            palette = QQuickThemePrivate::get(theme)->palettes[scope].data();
        }
    }

    if (palette) {
        QPalette f = *palette;
        if (scope == System)
            f.setResolveMask(0);
        return f;
    }

    if (scope != System)
        return QQuickTheme::palette(System);

    return QPalette();
}

void QQuickTheme::setFont(Scope scope, const QFont &font)
{
    Q_D(QQuickTheme);
    d->fonts[scope] = QSharedPointer<QFont>::create(d->defaultFont ? d->defaultFont->resolve(font) : font);
}

void QQuickTheme::setPalette(Scope scope, const QPalette &palette)
{
    Q_D(QQuickTheme);
    d->palettes[scope] = QSharedPointer<QPalette>::create(d->defaultPalette ? d->defaultPalette->resolve(palette) : palette);
}

QT_END_NAMESPACE
