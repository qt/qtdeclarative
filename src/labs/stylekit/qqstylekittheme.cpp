// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtQuick/private/qquickpalette_p.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/qpa/qplatformtheme.h>

#include "qqstylekittheme_p.h"
#include "qqstylekitstyle_p.h"

QT_BEGIN_NAMESPACE

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

QPalette QQStyleKitTheme::paletteForReader(QQStyleKitReader *reader) const
{
    Q_ASSERT(reader);
    const QQuickTheme::Scope scope = scopeForType(reader->type());
    return effectivePaletteForScope(scope);
}

QPalette QQStyleKitTheme::effectivePaletteForScope(QQuickTheme::Scope scope) const
{
    if (scope < QQuickTheme::System || scope >= NScopes) {
        qWarning() << Q_FUNC_INFO << "Invalid scope" << int(scope);
        return QPalette();
    }

    return m_effectivePalettes[int(scope)];
}

void QQStyleKitTheme::updateThemePalette()
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
            if (fb->isSet(scope)) {
                if (auto *p = (fb->*getter)())
                    result = p->toQPalette().resolve(result);
            } else if (scope != QQuickTheme::System && fb->isSet(QQuickTheme::System)) {
                if (auto *sys = fb->system())
                    result = sys->toQPalette().resolve(result);
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
