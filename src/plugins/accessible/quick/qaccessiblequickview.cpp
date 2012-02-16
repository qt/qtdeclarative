/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaccessiblequickview.h"

#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitem_p.h>

#include "qaccessiblequickitem.h"
#include "qqmlaccessible.h"

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

QAccessibleQuickView::QAccessibleQuickView(QQuickView *object)
    :QAccessibleObject(object)
{
}

int QAccessibleQuickView::childCount() const
{
    return view()->rootItem() ? 1 : 0;
}

QAccessibleInterface *QAccessibleQuickView::parent() const
{
    // FIXME: for now we assume to be a top level window...
    return QAccessible::queryAccessibleInterface(qApp);
}

QAccessibleInterface *QAccessibleQuickView::child(int index) const
{
    if (index == 0) {
        if (QQuickItem *declarativeRoot = view()->rootObject())
            return new QAccessibleQuickItem(declarativeRoot);
    }
    return 0;
}

QAccessible::Role QAccessibleQuickView::role() const
{
    return QAccessible::Window; // FIXME
}

QAccessible::State QAccessibleQuickView::state() const
{
    return QAccessible::State(); // FIXME
}

QRect QAccessibleQuickView::rect() const
{
    return QRect(view()->x(), view()->y(), view()->width(), view()->height());
}

QString QAccessibleQuickView::text(QAccessible::Text text) const
{
#ifdef Q_ACCESSIBLE_QUICK_ITEM_ENABLE_DEBUG_DESCRIPTION
    if (text == QAccessible::DebugDescription) {
        return QString::fromAscii(object()->metaObject()->className()) ;
    }
#else
    Q_UNUSED(text)
#endif
    return view()->windowTitle();
}


/*!
  \internal

  Can also return \a item itself
  */
static QQuickItem *childAt_helper(QQuickItem *item, int x, int y)
{
    if (item->opacity() == 0.0 || !item->isVisible() || !item->isEnabled())
        return 0;

    if (item->flags() & QQuickItem::ItemClipsChildrenToShape) {
        if (!itemScreenRect(item).contains(x, y))
            return 0;
    }

    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

    QList<QQuickItem *> children = itemPrivate->paintOrderChildItems();
    for (int i = children.count() - 1; i >= 0; --i) {
        QQuickItem *child = children.at(i);
        if (QQuickItem *childChild = childAt_helper(child, x, y))
            return childChild;
    }

    QRect screenRect = itemScreenRect(item);

    if (screenRect.contains(x, y))
        return item;

    return 0;
}

QAccessibleInterface *QAccessibleQuickView::childAt(int x, int y) const
{
    Q_ASSERT(view());
    QQuickItem *root = view()->rootItem();
    if (root) {
        if (QQuickItem *item = childAt_helper(root, x, y))
            return QAccessible::queryAccessibleInterface(item);
    }
    return 0;
}

int QAccessibleQuickView::indexOfChild(const QAccessibleInterface *iface) const
{
    if (iface) {
        QQuickItem *declarativeRoot = view()->rootObject();
        if (declarativeRoot == iface->object())
            return 0;
    }
    return -1;

}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
