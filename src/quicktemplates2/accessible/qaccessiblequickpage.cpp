/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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

