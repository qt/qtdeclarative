// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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
