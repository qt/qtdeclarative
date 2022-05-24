/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

    QColor background;
    QColor placeholderText;
    QColor blue;
    QColor white;
    QColor disabled;
    QColor grey;
    QColor dark;
#ifdef Q_OS_IOS
    background = qt_mac_toQColor(UIColor.systemBackgroundColor.CGColor);
    placeholderText = qt_mac_toQColor(UIColor.placeholderTextColor.CGColor);
    blue = qt_mac_toQColor(UIColor.systemBlueColor.CGColor);
    disabled = qt_mac_toQColor(UIColor.tertiarySystemFillColor.CGColor);
    white = qt_mac_toQColor(UIColor.whiteColor.CGColor);
    grey = qt_mac_toQColor(UIColor.opaqueSeparatorColor.CGColor);
    dark = qt_mac_toQColor(UIColor.systemGray6Color.CGColor);
#else
    background = QQuickStylePrivate::isDarkSystemTheme() ? QColor(Qt::black) : QColor(Qt::white);
    placeholderText = QColor(qRgba(60, 60, 67, 77));
    blue = QColor(qRgba(0, 122, 255, 255));
    white = QColor(qRgba(255, 255, 255, 255));
    disabled = QColor(qRgba(118, 118, 128, 31));
    grey = QColor(qRgba(142, 142, 147, 255));
    dark = QColor(qRgba(28, 28, 30, 255));
#endif
    systemPalette.setColor(QPalette::Window, background);
    systemPalette.setColor(QPalette::Base, background);

    systemPalette.setColor(QPalette::PlaceholderText, placeholderText);

    systemPalette.setColor(QPalette::Button, blue);
    systemPalette.setColor(QPalette::Disabled, QPalette::Button, disabled);

    systemPalette.setColor(QPalette::ButtonText, white);
    white.setAlphaF(0.5);
    systemPalette.setColor(QPalette::Disabled, QPalette::ButtonText, white);

    blue.setAlphaF(0.8);
    systemPalette.setColor(QPalette::Highlight, blue);

    systemPalette.setColor(QPalette::Mid, grey);
    systemPalette.setColor(QPalette::Dark, dark);

    theme->setPalette(QQuickTheme::System, systemPalette);
}

QT_END_NAMESPACE

