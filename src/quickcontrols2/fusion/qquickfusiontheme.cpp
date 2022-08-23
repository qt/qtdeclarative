// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickfusiontheme_p.h"

#include <QtQuickTemplates2/private/qquicktheme_p.h>
#include <QtQuickControls2/private/qquickstyle_p.h>

QT_BEGIN_NAMESPACE

extern QPalette qt_fusionPalette();

void QQuickFusionTheme::initialize(QQuickTheme *theme)
{
    theme->setPalette(QQuickTheme::System, qt_fusionPalette());
}

QT_END_NAMESPACE
