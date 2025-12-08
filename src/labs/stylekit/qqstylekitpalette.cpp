// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitpalette_p.h"

QT_BEGIN_NAMESPACE

QQStyleKitPalette::QQStyleKitPalette(QObject *parent)
    : QObject(parent)
{
}

#define STYLEKIT_PALETTE_GETTER(PROP, SCOPE)                                   \
QQuickPalette *QQStyleKitPalette::PROP() const {                              \
    if (!m_##PROP) {                                                          \
        m_##PROP.reset(new QQuickPalette());                                  \
        QObject::connect(m_##PROP.get(), &QQuickPalette::changed,             \
                         const_cast<QQStyleKitPalette*>(this),                \
                         [this]() {                                           \
                             auto *self = const_cast<QQStyleKitPalette*>(this);\
                             self->markSet(SCOPE);                            \
                             emit self->PROP##Changed();                      \
                         });                                                  \
    }                                                                         \
    return m_##PROP.get();                                                    \
}

STYLEKIT_PALETTE_GETTER(system, QQuickTheme::System)
STYLEKIT_PALETTE_GETTER(checkBox, QQuickTheme::CheckBox)
STYLEKIT_PALETTE_GETTER(button, QQuickTheme::Button)
STYLEKIT_PALETTE_GETTER(comboBox, QQuickTheme::ComboBox)
STYLEKIT_PALETTE_GETTER(groupBox, QQuickTheme::GroupBox)
STYLEKIT_PALETTE_GETTER(itemView, QQuickTheme::ItemView)
STYLEKIT_PALETTE_GETTER(label, QQuickTheme::Label)
STYLEKIT_PALETTE_GETTER(listView, QQuickTheme::ListView)
STYLEKIT_PALETTE_GETTER(menu, QQuickTheme::Menu)
STYLEKIT_PALETTE_GETTER(menuBar, QQuickTheme::MenuBar)
STYLEKIT_PALETTE_GETTER(radioButton, QQuickTheme::RadioButton)
STYLEKIT_PALETTE_GETTER(spinBox, QQuickTheme::SpinBox)
STYLEKIT_PALETTE_GETTER(switchControl, QQuickTheme::Switch)
STYLEKIT_PALETTE_GETTER(tabBar, QQuickTheme::TabBar)
STYLEKIT_PALETTE_GETTER(textArea, QQuickTheme::TextArea)
STYLEKIT_PALETTE_GETTER(textField, QQuickTheme::TextField)
STYLEKIT_PALETTE_GETTER(toolBar, QQuickTheme::ToolBar)
STYLEKIT_PALETTE_GETTER(toolTip, QQuickTheme::ToolTip)
STYLEKIT_PALETTE_GETTER(tumbler, QQuickTheme::Tumbler)

QQStyleKitPalette *QQStyleKitPalette::fallbackPalette() const
{
    return m_fallbackPalette;
}

void QQStyleKitPalette::setFallbackPalette(QQStyleKitPalette *palette)
{
    if (m_fallbackPalette == palette)
        return;

    m_fallbackPalette = palette;
    emit fallbackPaletteChanged();
}

QT_END_NAMESPACE

#include "moc_qqstylekitpalette_p.cpp"
