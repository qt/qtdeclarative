// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtGui/private/qfont_p.h>

#include "qquicklayoutstyleinfo_p.h"


QT_BEGIN_NAMESPACE

QQuickLayoutStyleInfo::QQuickLayoutStyleInfo()
{
}

qreal QQuickLayoutStyleInfo::spacing(Qt::Orientation /*orientation*/) const
{
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(Q_OS_QNX)
    // On Android and iOS the default spacing between each UI element is 8dp
    qreal spacing = 8.0;
#else
    qreal spacing = 5.0;
#endif

#ifndef Q_OS_MACOS
    // On OS X the DPI is always 72 so we should not scale it
    spacing = qRound(spacing * (qreal(qt_defaultDpiX()) / 96.0));
#endif

    return spacing;
}

qreal QQuickLayoutStyleInfo::windowMargin(Qt::Orientation /*orientation*/) const
{
    return 0;
}

bool QQuickLayoutStyleInfo::hasChangedCore() const
{
    // never changes
    return false;
}

QT_END_NAMESPACE

