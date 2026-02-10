// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitfont_p.h"

QT_BEGIN_NAMESPACE

QQStyleKitFont::QQStyleKitFont(QObject *parent)
    : QObject(parent)
{
}

#define DEFINE_FONT_GETTER(scopeName, scopeEnum) \
    QFont QQStyleKitFont::scopeName() const \
    { \
        return m_fonts[int(QQuickTheme::scopeEnum)]; \
    }

DEFINE_FONT_GETTER(system, System)
DEFINE_FONT_GETTER(button, Button)
DEFINE_FONT_GETTER(checkBox, CheckBox)
DEFINE_FONT_GETTER(comboBox, ComboBox)
DEFINE_FONT_GETTER(groupBox, GroupBox)
DEFINE_FONT_GETTER(itemDelegate, ItemView)
DEFINE_FONT_GETTER(label, Label)
DEFINE_FONT_GETTER(radioButton, RadioButton)
DEFINE_FONT_GETTER(spinBox, SpinBox)
DEFINE_FONT_GETTER(switchControl, Switch)
DEFINE_FONT_GETTER(tabBar, TabBar)
DEFINE_FONT_GETTER(textArea, TextArea)
DEFINE_FONT_GETTER(textField, TextField)
DEFINE_FONT_GETTER(toolBar, ToolBar)

#define DEFINE_FONT_SETTER(scopeName, scopeEnum, signal) \
    void QQStyleKitFont::set##scopeName(const QFont &font) \
    { \
        setFontForScope(QQuickTheme::scopeEnum, font, &QQStyleKitFont::signal); \
    }

DEFINE_FONT_SETTER(System, System, systemChanged)
DEFINE_FONT_SETTER(Button, Button, buttonChanged)
DEFINE_FONT_SETTER(CheckBox, CheckBox, checkBoxChanged)
DEFINE_FONT_SETTER(ComboBox, ComboBox, comboBoxChanged)
DEFINE_FONT_SETTER(GroupBox, GroupBox, groupBoxChanged)
DEFINE_FONT_SETTER(ItemDelegate, ItemView, itemDelegateChanged)
DEFINE_FONT_SETTER(Label, Label, labelChanged)
DEFINE_FONT_SETTER(RadioButton, RadioButton, radioButtonChanged)
DEFINE_FONT_SETTER(SpinBox, SpinBox, spinBoxChanged)
DEFINE_FONT_SETTER(SwitchControl, Switch, switchControlChanged)
DEFINE_FONT_SETTER(TabBar, TabBar, tabBarChanged)
DEFINE_FONT_SETTER(TextArea, TextArea, textAreaChanged)
DEFINE_FONT_SETTER(TextField, TextField, textFieldChanged)
DEFINE_FONT_SETTER(ToolBar, ToolBar, toolBarChanged)

void QQStyleKitFont::setFontForScope(QQuickTheme::Scope scope, const QFont &font, void (QQStyleKitFont::*signal)())
{
    const int index = int(scope);
    if (isSet(scope) && m_fonts[index] == font)
        return;

    m_fonts[index] = font;
    markSet(scope);
    emit (this->*signal)();
}

// The fallback font is used to resolve unset fonts
// The theme fonts fallback to the style fonts and
// style fonts fallback to the fallback style fonts
QQStyleKitFont *QQStyleKitFont::fallbackFont() const
{
    return m_fallback;
}

void QQStyleKitFont::setFallbackFont(QQStyleKitFont *fallback)
{
    if (m_fallback == fallback)
        return;

    m_fallback = fallback;
    emit fallbackFontChanged();
}

QFont QQStyleKitFont::fontForScope(QQuickTheme::Scope scope) const
{
    return m_fonts[int(scope)];
}

QT_END_NAMESPACE
#include "moc_qqstylekitfont_p.cpp"
