// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfusiontheme_p.h"

#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtQuickControls2/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

void QQuickFusionTheme::initialize(QQuickTheme *theme)
{
    // Enable platform palettes for fusion theme.
    if (theme)
        theme->setUsePlatformPalette(true);
}

QT_END_NAMESPACE
