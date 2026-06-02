// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include "qaccessiblequickflickable_p.h"

#include <QtQuick/private/qquickflickable_p.h>

QT_BEGIN_NAMESPACE

#if QT_CONFIG(accessibility)

QAccessibleQuickFlickable::QAccessibleQuickFlickable(QQuickFlickable *flickable)
    : QAccessibleQuickItem(flickable)
{
}

QSizeF QAccessibleQuickFlickable::contentSize() const
{
    if (auto *f = flickable())
        return { f->contentWidth(), f->contentHeight() };

    return { };
}

QPointF QAccessibleQuickFlickable::position() const
{
    if (auto *f = flickable()) {
        const qreal contentWidth = f->contentWidth();
        const qreal contentHeight = f->contentHeight();

        if (contentWidth == 0 || contentHeight == 0)
            return { 0, 0 };

        // The QML Flickable uses the contentX property to realize the leftMargin
        // so the normalized position needs to account for this. The same goes
        // for the Y position.
        const qreal effectiveXPosition = f->contentX() + f->leftMargin() + f->originX();
        const qreal effectiveYPosition = f->contentY() + f->topMargin() + f->originY();
        return { effectiveXPosition / contentWidth, effectiveYPosition / contentHeight };
    }

    return { };
}

QSizeF QAccessibleQuickFlickable::viewportSize() const
{
    if (auto *f = flickable()) {
        const qreal contentWidth = f->contentWidth();
        const qreal contentHeight = f->contentHeight();

        if (contentWidth == 0 || contentHeight == 0)
            return { 1, 1 };

        const qreal effectiveWidth = f->width() - f->leftMargin() - f->rightMargin();
        const qreal effectiveHeight = f->height() - f->topMargin() - f->bottomMargin();
        return { std::min(1.0, effectiveWidth / contentWidth),
                 std::min(1.0, effectiveHeight / contentHeight) };
    }

    return { };
}

bool QAccessibleQuickFlickable::isIndexed() const
{
    return false;
}

void QAccessibleQuickFlickable::setPosition(const QPointF &position)
{
    if (auto *f = flickable()) {
        const qreal contentWidth = f->contentWidth();
        const qreal contentHeight = f->contentHeight();

        if (contentWidth == 0 || contentHeight == 0) {
            f->setContentX(0);
            f->setContentY(0);
            return;
        }

        f->setContentX(position.x() * contentWidth - f->leftMargin() - f->originX());
        f->setContentY(position.y() * contentHeight - f->topMargin() - f->originY());
    }
}

void *QAccessibleQuickFlickable::interface_cast(QAccessible::InterfaceType t)
{
    if (t == QAccessible::ViewportInterface)
        return static_cast<QAccessibleViewportInterface *>(this);

    return QAccessibleQuickItem::interface_cast(t);
}

QQuickFlickable *QAccessibleQuickFlickable::flickable() const
{
    return qobject_cast<QQuickFlickable *>(object());
}

#endif // accessibility

QT_END_NAMESPACE
