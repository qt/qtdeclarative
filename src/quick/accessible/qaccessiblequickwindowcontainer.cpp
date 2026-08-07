// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qaccessiblequickwindowcontainer_p.h"

#include <QtQuick/private/qquickwindowcontainer_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

QAccessibleQuickWindowContainer::QAccessibleQuickWindowContainer(QQuickWindowContainer *container)
    : QAccessibleQuickItem(container)
{
}

QQuickWindowContainer *QAccessibleQuickWindowContainer::container() const
{
    return static_cast<QQuickWindowContainer *>(object());
}

/*
    Resolves the accessibility interface of the contained window.

    Goes via accessibleRoot(), rather than querying an interface for the
    window itself, as the latter is driven by the window's class name,
    and not all QWindow subclasses may have one. Going via the root is
    also the entry point the platform bridges use.
*/
QAccessibleInterface *QAccessibleQuickWindowContainer::accessibleRoot() const
{
    if (auto *item = container()) {
        if (QWindow *window = item->containedWindow())
            return window->accessibleRoot();
    }

    return nullptr;
}

int QAccessibleQuickWindowContainer::childCount() const
{
    return accessibleRoot() ? 1 : 0;
}

QAccessibleInterface *QAccessibleQuickWindowContainer::child(int index) const
{
    if (index == 0)
        return accessibleRoot();

    return nullptr;
}

int QAccessibleQuickWindowContainer::indexOfChild(const QAccessibleInterface *iface) const
{
    if (!iface)
        return -1;

    // The root is not necessarily an interface for the contained window
    // itself. For a hosted widget window it's the interface of the widget.
    if (QAccessibleInterface *root = accessibleRoot()) {
        if (root->object() == iface->object())
            return 0;
    }

    return -1;
}

QAccessibleInterface *QAccessibleQuickWindowContainer::childAt(int x, int y) const
{
    QAccessibleInterface *root = accessibleRoot();
    if (!root || root->state().invisible)
        return nullptr;

    if (QAccessibleInterface *descendant = root->childAt(x, y))
        return descendant;

    return root->rect().contains(x, y) ? root : nullptr;
}

#endif // accessibility

QT_END_NAMESPACE
