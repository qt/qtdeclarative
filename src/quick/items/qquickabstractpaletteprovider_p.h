// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial
#ifndef QQUICKABSTRACTPALETTEPROVIDER_P_H
#define QQUICKABSTRACTPALETTEPROVIDER_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>

QT_BEGIN_NAMESPACE

class QQuickAbstractPaletteProvider
{
public:
    virtual ~QQuickAbstractPaletteProvider() = default;
    virtual QPalette defaultPalette() const = 0;
    virtual QPalette parentPalette(const QPalette &fallbackPalette) const { return fallbackPalette; }
};

QT_END_NAMESPACE

#endif // QQUICKABSTRACTPALETTEPROVIDER_P_H
