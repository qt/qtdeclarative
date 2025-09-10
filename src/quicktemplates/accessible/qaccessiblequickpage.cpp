// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblequickpage_p.h"
#include "qquickpage_p.h"

QT_BEGIN_NAMESPACE

QAccessibleQuickPage::QAccessibleQuickPage(QQuickPage *page)
    : QAccessibleQuickItem(page)
{
}

QAccessibleInterface *QAccessibleQuickPage::child(int index) const
{
    const QList<QQuickItem*> kids = orderedChildItems();
    if (QQuickItem *item = kids.value(index))
        return QAccessible::queryAccessibleInterface(item);
    return nullptr;
}

int QAccessibleQuickPage::indexOfChild(const QAccessibleInterface *iface) const
{
    const QList<QQuickItem*> kids = orderedChildItems();
    return (int)kids.indexOf(static_cast<QQuickItem*>(iface->object()));
}

QList<QQuickItem *> QAccessibleQuickPage::orderedChildItems() const
{
    // Just ensures that the header is first, and footer is last. Other existing order is kept.
    const QQuickPage *p = page();
    QList<QQuickItem*> kids = childItems();
    const qsizetype hidx = kids.indexOf(p->header());
    if (hidx != -1)
        kids.move(hidx, 0);
    const qsizetype fidx = kids.indexOf(p->footer());
    if (fidx != -1)
        kids.move(fidx, kids.size() - 1);
    return kids;
}

QQuickPage *QAccessibleQuickPage::page() const
{
    return static_cast<QQuickPage*>(object());
}

QT_END_NAMESPACE

