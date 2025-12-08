// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qqstylekitstyleandthemebase_p.h"

QT_BEGIN_NAMESPACE

QQStyleKitStyleAndThemeBase::QQStyleKitStyleAndThemeBase(QObject *parent)
    : QQStyleKitControls(parent)
{
}

QQStyleKitFont *QQStyleKitStyleAndThemeBase::fonts()
{
    return &m_fonts;
}

QQStyleKitPalette *QQStyleKitStyleAndThemeBase::palettes()
{
    return &m_palettes;
}

QT_END_NAMESPACE

#include "moc_qqstylekitstyleandthemebase_p.cpp"


