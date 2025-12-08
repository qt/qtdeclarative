// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtQuick/private/qquickpalette_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#include "qqstylekittheme_p.h"
#include "qqstylekitstyle_p.h"

QT_BEGIN_NAMESPACE

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

void QQStyleKitTheme::updateThemePalette()
{
    auto *theme = QQuickTheme::instance();
    if (!theme)
        return;

    // QQuickTheme currently offers only one default platform palette for all controls.
    // This implementation will instead inherit the corresponding platform palette for each
    // control type. Hence, for now, we don't use QQuickTheme::usePlatformPalette, but roll
    // our own version instead.
    theme->setUsePlatformPalette(false);
    auto resolveFromPaletteChain = [&](QQuickTheme::Scope scope,
        QQuickPalette* (QQStyleKitPalette::*getter)() const) -> QPalette
    {
        QPalette result;
        if (!palettes())
            return result;

        // Find the nearest fallback that explicitly set this scope
        const QQStyleKitPalette *nearestFallback = nullptr;
        for (auto *fb = palettes()->fallbackPalette(); fb; fb = fb->fallbackPalette()) {
            if (fb->isSet(scope)) {
                nearestFallback = fb;
                break;
            }
        }

        if (nearestFallback) {
            if (auto *p = (nearestFallback->*getter)())
                result = result.resolve(p->toQPalette());
        }

        if (palettes()->isSet(scope)) {
            if (auto *p = (palettes()->*getter)())
                result = result.resolve(p->toQPalette());
        }

        return result;
    };

    auto setResolved = [&](QQuickPalette* (QQStyleKitPalette::*getter)() const,
        QQuickTheme::Scope scope)
    {
        const QPalette resolved = resolveFromPaletteChain(scope, getter);
        theme->setPalette(scope, resolved);
    };

    setResolved(&QQStyleKitPalette::system, QQuickTheme::System);
    setResolved(&QQStyleKitPalette::button, QQuickTheme::Button);
    setResolved(&QQStyleKitPalette::checkBox, QQuickTheme::CheckBox);
    setResolved(&QQStyleKitPalette::comboBox, QQuickTheme::ComboBox);
    setResolved(&QQStyleKitPalette::groupBox, QQuickTheme::GroupBox);
    setResolved(&QQStyleKitPalette::itemView, QQuickTheme::ItemView);
    setResolved(&QQStyleKitPalette::label, QQuickTheme::Label);
    setResolved(&QQStyleKitPalette::listView, QQuickTheme::ListView);
    setResolved(&QQStyleKitPalette::menu, QQuickTheme::Menu);
    setResolved(&QQStyleKitPalette::menuBar, QQuickTheme::MenuBar);
    setResolved(&QQStyleKitPalette::radioButton, QQuickTheme::RadioButton);
    setResolved(&QQStyleKitPalette::spinBox, QQuickTheme::SpinBox);
    setResolved(&QQStyleKitPalette::switchControl, QQuickTheme::Switch);
    setResolved(&QQStyleKitPalette::tabBar, QQuickTheme::TabBar);
    setResolved(&QQStyleKitPalette::textArea, QQuickTheme::TextArea);
    setResolved(&QQStyleKitPalette::textField, QQuickTheme::TextField);
    setResolved(&QQStyleKitPalette::toolBar, QQuickTheme::ToolBar);
    setResolved(&QQStyleKitPalette::toolTip, QQuickTheme::ToolTip);
    setResolved(&QQStyleKitPalette::tumbler, QQuickTheme::Tumbler);

    QEvent event(QEvent::ApplicationPaletteChange);
    QGuiApplication::sendEvent(qGuiApp, &event);
}

void QQStyleKitTheme::updateQuickTheme()
{
    updateThemePalette();
}

void QQStyleKitTheme::componentComplete()
{
    QQStyleKitControls::componentComplete();
    m_completed = true;
}

QT_END_NAMESPACE

#include "moc_qqstylekittheme_p.cpp"
