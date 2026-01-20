// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITTHEME_P_H
#define QQSTYLEKITTHEME_P_H

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

#include <QtQml/QtQml>

#include "qqstylekitstyleandthemebase_p.h"
#include "qqstylekitpalette_p.h"

QT_BEGIN_NAMESPACE

class QQStyleKitControls;
class QQStyleKitPropertyResolver;

class QQStyleKitTheme : public QQStyleKitStyleAndThemeBase
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Theme)

public:
    QQStyleKitTheme(QObject *parent = nullptr);

    QQStyleKitStyle *style() const;

    QPalette paletteForReader(QQStyleKitReader *reader) const;

signals:
    void targetChanged();

protected:
    void componentComplete() override;

private:
    void updateThemePalette();
    void updateQuickTheme();
    QPalette effectivePaletteForScope(QQuickTheme::Scope scope) const;

    bool m_completed = false;
    static int const NScopes = int(QQuickTheme::Tumbler) + 1;
    QPalette m_effectivePalettes[NScopes];

    friend class QQStyleKitAttached;
    friend class QQStyleKitStyle;

    Q_DISABLE_COPY(QQStyleKitTheme)
};

QT_END_NAMESPACE

#endif // QQSTYLEKITTHEME_P_H
