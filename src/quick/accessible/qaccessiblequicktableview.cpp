// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qaccessiblequicktableview_p.h"

#include <QtQuick/private/qquicktableview_p_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

QAccessibleQuickTableView::QAccessibleQuickTableView(QQuickTableView *tableView)
    : QAccessibleQuickFlickable(tableView)
{
}

// The view holds the focus, but assistive technology follows the current cell.
// Platforms that ask for the focused element, rather than taking it from the
// focus event, find the delegate of that cell here.
QAccessibleInterface *QAccessibleQuickTableView::focusChild() const
{
    if (auto *view = qobject_cast<QQuickTableView *>(object())) {
        if (QQuickItem *item = QQuickTableViewPrivate::get(view)->accessibleFocusItem)
            return QAccessible::queryAccessibleInterface(item);
    }
    return nullptr;
}

#endif // accessibility

QT_END_NAMESPACE
