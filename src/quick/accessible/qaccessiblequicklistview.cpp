// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qaccessiblequicklistview_p.h"

#include <QtQuick/private/qquicklistview_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

QAccessibleQuickListView::QAccessibleQuickListView(QQuickListView *listView)
    : QAccessibleQuickItem(listView)
{
}

QSizeF QAccessibleQuickListView::contentSize() const
{
    if (auto *l = listView()) {
        const int count = l->count();
        if (count == 0)
            return { 0, 0 };

        if (l->orientation() == QQuickListView::Horizontal)
            return { static_cast<qreal>(count), 1 };

        return { 1, static_cast<qreal>(count) };
    }

    return { };
}

QPointF QAccessibleQuickListView::position() const
{
    if (auto *l = listView()) {
        const int count = l->count();
        if (count == 0)
            return { 0, 0 };

        if (l->orientation() == QQuickListView::Horizontal) {
            // compensate for undershooting of underlying Flickable
            const int startIndex = std::max(0, l->indexAt(l->contentX(), 0));
            return { static_cast<qreal>(startIndex) / count, 0 };
        }

        // compensate for undershooting of underlying Flickable
        const int startIndex = std::max(0, l->indexAt(0, l->contentY()));
        return { 0, static_cast<qreal>(startIndex) / count };
    }

    return { };
}

QSizeF QAccessibleQuickListView::viewportSize() const
{
    if (auto *l = listView()) {
        const int count = l->count();
        if (count == 0)
            return { 1, 1 };

        if (l->orientation() == QQuickListView::Horizontal) {
            const int startIndex = std::max(0, l->indexAt(l->contentX(), 0));
            int endIndex = l->indexAt(l->contentX() + l->width() - 1, 0);
            // compensate for overshooting of underlying Flickable
            if (endIndex == -1)
                endIndex = count - 1;

            return { static_cast<qreal>(endIndex - startIndex + 1) / count, 1 };
        }
        const int startIndex = std::max(0, l->indexAt(0, l->contentY()));
        int endIndex = l->indexAt(0, l->contentY() + l->height() - 1);
        // compensate for overshooting of underlying Flickable
        if (endIndex == -1)
            endIndex = count - 1;

        return { 1, static_cast<qreal>(endIndex - startIndex + 1) / count };
    }

    return { };
}

bool QAccessibleQuickListView::isIndexed() const
{
    return true;
}

void QAccessibleQuickListView::setPosition(QPointF position)
{
    if (auto *l = listView()) {
        if (l->orientation() == QQuickListView::Horizontal) {
            l->positionViewAtIndex(position.x() * l->count(), QQuickListView::Beginning);
            return;
        }

        l->positionViewAtIndex(position.y() * l->count(), QQuickListView::Beginning);
    }
}

void *QAccessibleQuickListView::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ViewportInterface)
        return static_cast<QAccessibleViewportInterface *>(this);

    return QAccessibleQuickItem::interface_cast(t);
}

QQuickListView *QAccessibleQuickListView::listView() const
{
    return qobject_cast<QQuickListView *>(object());
}

#endif // accessibility

QT_END_NAMESPACE
