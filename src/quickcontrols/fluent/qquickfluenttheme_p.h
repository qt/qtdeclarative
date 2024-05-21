// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QQUICKFLUENTTHEME_P_H
#define QQUICKFLUENTTHEME_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class QQuickTheme;
class QPalette;
class QQuickFluentTheme
{
public:
    static void initialize(QQuickTheme *theme);
    static void updatePalette(QPalette &palette);
};

QT_END_NAMESPACE

#endif // QQUICKFLUENTTHEME_P_H
