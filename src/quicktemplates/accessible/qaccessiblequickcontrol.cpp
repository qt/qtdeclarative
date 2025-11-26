// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qaccessiblequickcontrol_p.h"
#include "qquickcontrol_p.h"

QT_BEGIN_NAMESPACE

QAccessibleQuickControl::QAccessibleQuickControl(QQuickControl *control)
    : QAccessibleQuickItem(control)
{
}

QList<QAccessible::Attribute> QAccessibleQuickControl::attributeKeys() const
{
    auto keys = QAccessibleQuickItem::attributeKeys();
    if (!keys.contains(QAccessible::Attribute::Locale))
        keys << QAccessible::Attribute::Locale;

    return keys;
}

QVariant QAccessibleQuickControl::attributeValue(QAccessible::Attribute key) const
{
    if (key == QAccessible::Attribute::Locale)
        return QVariant::fromValue(control()->locale());

    return QAccessibleQuickItem::attributeValue(key);
}

QQuickControl *QAccessibleQuickControl::control() const
{
    return static_cast<QQuickControl *>(object());
}

QT_END_NAMESPACE
