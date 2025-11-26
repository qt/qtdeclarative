// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qaccessiblequickapplicationwindow_p.h"

QT_BEGIN_NAMESPACE

QAccessibleQuickApplicationWindow::QAccessibleQuickApplicationWindow(
        QQuickApplicationWindow *window)
    : QAccessibleQuickWindow(window)
{
}

void *QAccessibleQuickApplicationWindow::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::AttributesInterface)
        return static_cast<QAccessibleAttributesInterface *>(this);

    return QAccessibleQuickWindow::interface_cast(t);
}

QList<QAccessible::Attribute> QAccessibleQuickApplicationWindow::attributeKeys() const
{
    return { QAccessible::Attribute::Locale };
}

QVariant QAccessibleQuickApplicationWindow::attributeValue(QAccessible::Attribute key) const
{
    if (key == QAccessible::Attribute::Locale) {
        Q_ASSERT(applicationWindow());
        return QVariant::fromValue(applicationWindow()->locale());
    }

    return QVariant();
}

QQuickApplicationWindow *QAccessibleQuickApplicationWindow::applicationWindow() const
{
    return static_cast<QQuickApplicationWindow *>(object());
}

QT_END_NAMESPACE
