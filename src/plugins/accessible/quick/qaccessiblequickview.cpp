/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaccessiblequickview.h"

#include <QtGui/qguiapplication.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitem_p.h>

#include "qaccessiblequickitem.h"
#include "qqmlaccessible.h"

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

QAccessibleQuickWindow::QAccessibleQuickWindow(QQuickWindow *object)
    :QAccessibleObject(object)
{
}

QQuickItem *QAccessibleQuickWindow::rootItem() const
{
    if (QQuickItem *ci = window()->contentItem()) {
        const QList<QQuickItem *> &childItems = ci->childItems();
        if (!childItems.isEmpty())
            return childItems.first();
    }
    return 0;
}

int QAccessibleQuickWindow::childCount() const
{
    return rootItem() ? 1 : 0;
}

QAccessibleInterface *QAccessibleQuickWindow::parent() const
{
    // FIXME: for now we assume to be a top level window...
    return QAccessible::queryAccessibleInterface(qApp);
}

QAccessibleInterface *QAccessibleQuickWindow::child(int index) const
{
    if (index == 0)
        return QAccessible::queryAccessibleInterface(rootItem());
    return 0;
}

QAccessible::Role QAccessibleQuickWindow::role() const
{
    return QAccessible::Window; // FIXME
}

QAccessible::State QAccessibleQuickWindow::state() const
{
    QAccessible::State st;
    if (window() == QGuiApplication::focusWindow())
        st.active = true;
    if (!window()->isVisible())
        st.invisible = true;
    return st;
}

QRect QAccessibleQuickWindow::rect() const
{
    return QRect(window()->x(), window()->y(), window()->width(), window()->height());
}

QString QAccessibleQuickWindow::text(QAccessible::Text text) const
{
#ifdef Q_ACCESSIBLE_QUICK_ITEM_ENABLE_DEBUG_DESCRIPTION
    if (text == QAccessible::DebugDescription) {
        return QString::fromLatin1(object()->metaObject()->className()) ;
    }
#else
    Q_UNUSED(text)
#endif
    return window()->title();
}


/*!
  \internal

  Can also return \a item itself
  */
static QQuickItem *childAt_helper(QQuickItem *item, int x, int y)
{
    if (!item->isVisible() || !item->isEnabled())
        return 0;

    if (item->flags() & QQuickItem::ItemClipsChildrenToShape) {
        if (!itemScreenRect(item).contains(x, y))
            return 0;
    }

    QAccessibleInterface *accessibleInterface = QAccessible::queryAccessibleInterface(item);
    // this item has no Accessible attached property
    if (!accessibleInterface)
        return 0;

    if (accessibleInterface->childCount() == 0) {
        return (itemScreenRect(item).contains(x, y)) ? item : 0;
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

QAccessibleInterface *QAccessibleQuickWindow::childAt(int x, int y) const
{
    Q_ASSERT(window());
    QQuickItem *root = rootItem();
    if (root) {
        if (QQuickItem *item = childAt_helper(root, x, y))
            return QAccessible::queryAccessibleInterface(item);
        return QAccessible::queryAccessibleInterface(root);
    }
    return 0;
}

int QAccessibleQuickWindow::indexOfChild(const QAccessibleInterface *iface) const
{
    if (iface) {
        QQuickItem *declarativeRoot = rootItem();
        if (declarativeRoot == iface->object())
            return 0;
    }
    return -1;
}

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
