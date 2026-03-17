// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#ifndef QQSTYLEKITTSTYLEANDTHEMEBASE_P_H
#define QQSTYLEKITTSTYLEANDTHEMEBASE_P_H

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
#include "qqstylekitcontrols_p.h"
#include "qqstylekitfont_p.h"
#include "qqstylekitpalette_p.h"

QT_BEGIN_NAMESPACE

class QQStyleKitStyleAndThemeBase : public QQStyleKitControls
{
    Q_OBJECT
    Q_PROPERTY(QQStyleKitFont *fonts READ fonts NOTIFY fontsChanged FINAL)
    Q_PROPERTY(QQStyleKitPalette *palettes READ palettes NOTIFY palettesChanged FINAL)
    QML_UNCREATABLE("This component is abstract, and cannot be instantiated")
    QML_NAMED_ELEMENT(AbstractStyle)

public:
    QQStyleKitStyleAndThemeBase(QObject *parent = nullptr);

    QQStyleKitFont *fonts();
    QQStyleKitPalette *palettes();

signals:
    void fontsChanged();
    void palettesChanged();

private:

    QQStyleKitFont m_fonts;
    QQStyleKitPalette m_palettes;

    bool m_hasVariationsThatAffectExistingStyleReaders = false;

    Q_DISABLE_COPY(QQStyleKitStyleAndThemeBase)

    friend class QQStyleKitPropertyResolver;
};

QT_END_NAMESPACE

#endif // QQSTYLEKITTSTYLEANDTHEMEBASE_P_H
