// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickiostheme_p.h"

#include <QtGui/private/qcoregraphics_p.h>

#ifdef Q_OS_IOS
#include <UIKit/UIInterface.h>
#endif

#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtQuickControls2/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

void QQuickIOSTheme::initialize(QQuickTheme *theme)
{
    QPalette systemPalette;

    QColor window;
    QColor base;
    QColor text;
    QColor disabledText;
    QColor placeholderText;
    QColor button;
    QColor disabledButton;
    QColor white;
    QColor lightGray;
    QColor gray;
    QColor darkGray;
#ifdef Q_OS_IOS
    window = qt_mac_toQColor(UIColor.systemGroupedBackgroundColor.CGColor);
    base = qt_mac_toQColor(UIColor.secondarySystemGroupedBackgroundColor.CGColor);
    text = qt_mac_toQColor(UIColor.labelColor.CGColor);
    disabledText = qt_mac_toQColor(UIColor.tertiaryLabelColor.CGColor);
    placeholderText = qt_mac_toQColor(UIColor.placeholderTextColor.CGColor);
    button = qt_mac_toQColor(UIColor.systemBlueColor.CGColor);
    disabledButton = qt_mac_toQColor(UIColor.tertiarySystemFillColor.CGColor);
    white = qt_mac_toQColor(UIColor.whiteColor.CGColor);
    lightGray = qt_mac_toQColor(UIColor.systemGray6Color.CGColor);
    gray = qt_mac_toQColor(UIColor.opaqueSeparatorColor.CGColor);
    darkGray = qt_mac_toQColor(UIColor.systemGrayColor.CGColor);
#else
    bool isDarkSystemTheme = QQuickStylePrivate::isDarkSystemTheme();
    window = isDarkSystemTheme ? QColor(qRgba(0, 0, 0, 255)) : QColor(qRgba(242, 242, 247, 255));
    base = isDarkSystemTheme ? QColor(qRgba(28, 28, 30, 255)) : QColor(Qt::white);
    text = isDarkSystemTheme ? QColor(Qt::white) : QColor(Qt::black);
    disabledText = isDarkSystemTheme ? QColor(qRgba(60, 60, 67, 76)) : QColor(qRgba(235, 235, 245, 76));
    placeholderText = isDarkSystemTheme ? QColor(qRgba(235, 235, 245, 76)) : QColor(qRgba(60, 60, 67, 77));
    button = isDarkSystemTheme ? QColor(qRgba(10, 132, 255, 255)) : QColor(qRgba(0, 122, 255, 255));
    disabledButton = isDarkSystemTheme ? QColor(qRgba(118, 118, 128, 61)) : QColor(qRgba(118, 118, 128, 31));
    white = QColor(Qt::white);
    lightGray = isDarkSystemTheme ? QColor(qRgba(28, 28, 30, 255)) : QColor(qRgba(242, 242, 247, 255));
    gray = isDarkSystemTheme ? QColor(qRgba(56, 56, 58, 255)) : QColor(qRgba(198, 198, 200, 2555));
    darkGray = QColor(qRgba(142, 142, 147, 255));
#endif
    systemPalette.setColor(QPalette::Window, window);
    systemPalette.setColor(QPalette::Base, base);

    systemPalette.setColor(QPalette::WindowText, text);
    systemPalette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    systemPalette.setColor(QPalette::Text, text);
    systemPalette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    systemPalette.setColor(QPalette::PlaceholderText, placeholderText);

    systemPalette.setColor(QPalette::Button, button);
    systemPalette.setColor(QPalette::Disabled, QPalette::Button, disabledButton);
    systemPalette.setColor(QPalette::ButtonText, white);
    systemPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);

    systemPalette.setColor(QPalette::ToolTipText, text);
    systemPalette.setColor(QPalette::Disabled, QPalette::ToolTipText, disabledText);

    systemPalette.setColor(QPalette::Highlight, button.lighter(115));

    systemPalette.setColor(QPalette::Light, lightGray);
    systemPalette.setColor(QPalette::Mid, gray);
    systemPalette.setColor(QPalette::Dark, darkGray);

    theme->setPalette(QQuickTheme::System, systemPalette);
}

QT_END_NAMESPACE
