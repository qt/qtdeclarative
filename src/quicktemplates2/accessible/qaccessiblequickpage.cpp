/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
******************************************************************************/

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
        kids.move(fidx, kids.count() - 1);
    return kids;
}

QQuickPage *QAccessibleQuickPage::page() const
{
    return static_cast<QQuickPage*>(object());
}

QT_END_NAMESPACE

