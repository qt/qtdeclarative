/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include "qquickplatformtheme_p.h"

#include <QtGui/private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

/*!
    \internal

    Exposes platform theme hints to QML so that we have a more accurate way of checking
    for platform-specific behavior than \c {Qt.platform.os === "foo"}.
*/
QQuickPlatformTheme::QQuickPlatformTheme(QObject *parent) :
    QObject(parent)
{
}

QVariant QQuickPlatformTheme::themeHint(QPlatformTheme::ThemeHint themeHint) const
{
    return getThemeHint(themeHint);
}

/*!
    \internal

    This is static to allow us to call it from C++, as we're only available as a singleton in QML.
*/
QVariant QQuickPlatformTheme::getThemeHint(QPlatformTheme::ThemeHint themeHint)
{
    // Allow tests to force some theme hint values, otherwise they get very messy and difficult to understand.
    switch (themeHint) {
    case QPlatformTheme::ShowDirectoriesFirst: {
        const QVariant showDirsFirst = qEnvironmentVariable("QT_QUICK_DIALOGS_SHOW_DIRS_FIRST");
        if (showDirsFirst.isValid() && showDirsFirst.canConvert<bool>())
            return showDirsFirst.toBool();
        break;
    }
    default:
        break;
    }
    return QGuiApplicationPrivate::platformTheme()->themeHint(themeHint);
}

QT_END_NAMESPACE

#include "moc_qquickplatformtheme_p.cpp"
