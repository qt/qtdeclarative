// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicklabsplatformmenuseparator_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype MenuSeparator
    \inherits MenuItem
//!     \nativetype QQuickLabsPlatformMenuSeparator
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A native menu separator.

    The MenuSeparator type is provided for convenience. It is a MenuItem
    that has the \l {MenuItem::}{separator} property set to \c true by default.

    \image qtlabsplatform-menubar.png

    \labs

    \sa Menu, MenuItem
*/

QQuickLabsPlatformMenuSeparator::QQuickLabsPlatformMenuSeparator(QObject *parent)
    : QQuickLabsPlatformMenuItem(parent)
{
    setSeparator(true);
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformmenuseparator_p.cpp"
