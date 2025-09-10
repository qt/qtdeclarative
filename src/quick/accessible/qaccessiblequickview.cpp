// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qaccessiblequickview_p.h"

#include <QtGui/qguiapplication.h>

#include <QtQuick/qquickitem.h>
#include <QtQuick/private/qquickitem_p.h>

#include "qaccessiblequickitem_p.h"

#if QT_CONFIG(accessibility)

QT_BEGIN_NAMESPACE

QAccessibleQuickWindow::QAccessibleQuickWindow(QQuickWindow *object)
    :QAccessibleObject(object)
{
}

QList<QQuickItem *> QAccessibleQuickWindow::rootItems() const
{
    if (QQuickItem *ci = window() ? window()->contentItem() : nullptr)
        return accessibleUnignoredChildren(ci);
    return QList<QQuickItem *>();
}

int QAccessibleQuickWindow::childCount() const
{
    return rootItems().size();
}

QAccessibleInterface *QAccessibleQuickWindow::parent() const
{
    // FIXME: for now we assume to be a top level window...
    return QAccessible::queryAccessibleInterface(qApp);
}

QAccessibleInterface *QAccessibleQuickWindow::child(int index) const
{
    const QList<QQuickItem*> &kids = rootItems();
    if (index >= 0 && index < kids.size())
        return QAccessible::queryAccessibleInterface(kids.at(index));
    return nullptr;
}

QAccessibleInterface *QAccessibleQuickWindow::focusChild() const
{
    QObject *focusObject = window() ? window()->focusObject() : nullptr;
    if (focusObject) {
        QAccessibleInterface *iface = QAccessible::queryAccessibleInterface(focusObject);
        if (!iface || iface == this || !iface->focusChild())
            return iface;
        return iface->focusChild();
    }
    return nullptr;
}

QAccessible::Role QAccessibleQuickWindow::role() const
{
    return QAccessible::Window;
}

QAccessible::State QAccessibleQuickWindow::state() const
{
    QAccessible::State st;
    if (window() == QGuiApplication::focusWindow())
        st.active = true;
    if (!window() || !window()->isVisible())
        st.invisible = true;
    return st;
}

QRect QAccessibleQuickWindow::rect() const
{
    if (!window())
        return {};
    return QRect(window()->x(), window()->y(), window()->width(), window()->height());
}

QString QAccessibleQuickWindow::text(QAccessible::Text text) const
{
    if (!window())
        return {};
#ifdef Q_ACCESSIBLE_QUICK_ITEM_ENABLE_DEBUG_DESCRIPTION
    if (text == QAccessible::DebugDescription) {
        return QString::fromLatin1(object()->metaObject()->className()) ;
    }
#endif
    if (text == QAccessible::Name)
        return window()->title();
    else
        return {};
}

QAccessibleInterface *QAccessibleQuickWindow::childAt(int x, int y) const
{
    Q_ASSERT(window());
    for (int i = childCount() - 1; i >= 0; --i) {
        QAccessibleInterface *childIface = child(i);
        if (childIface && !childIface->state().invisible) {
            if (QAccessibleInterface *iface = childIface->childAt(x, y))
                return iface;
            if (childIface->rect().contains(x, y))
                return childIface;
        }
    }
    return nullptr;
}

int QAccessibleQuickWindow::indexOfChild(const QAccessibleInterface *iface) const
{
    int i = -1;
    if (iface) {
        const QList<QQuickItem *> &roots = rootItems();
        i = roots.size() - 1;
        while (i >= 0) {
            if (iface->object() == roots.at(i))
                break;
            --i;
        }
    }
    return i;
}

QT_END_NAMESPACE

#endif // accessibility
