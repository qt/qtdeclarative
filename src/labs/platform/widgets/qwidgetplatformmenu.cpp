// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwidgetplatformmenu_p.h"
#include "qwidgetplatformmenuitem_p.h"

#include <QtGui/qaction.h>
#include <QtGui/qwindow.h>
#include <QtGui/private/qhighdpiscaling_p.h>
#include <QtWidgets/qmenu.h>

QT_BEGIN_NAMESPACE

QWidgetPlatformMenu::QWidgetPlatformMenu(QObject *parent)
    : m_menu(new QMenu)
{
    setParent(parent);

    connect(m_menu.data(), &QMenu::aboutToShow, this, &QPlatformMenu::aboutToShow);
    connect(m_menu.data(), &QMenu::aboutToHide, this, &QPlatformMenu::aboutToHide);
}

QWidgetPlatformMenu::~QWidgetPlatformMenu()
{
}

QMenu *QWidgetPlatformMenu::menu() const
{
    return m_menu.data();
}

void QWidgetPlatformMenu::insertMenuItem(QPlatformMenuItem *item, QPlatformMenuItem *before)
{
    QWidgetPlatformMenuItem *widgetItem = qobject_cast<QWidgetPlatformMenuItem *>(item);
    if (!widgetItem)
        return;

    QWidgetPlatformMenuItem *widgetBefore = qobject_cast<QWidgetPlatformMenuItem *>(before);
    m_menu->insertAction(widgetBefore ? widgetBefore->action() : nullptr, widgetItem->action());
    int index = m_items.indexOf(widgetBefore);
    if (index < 0)
        index = m_items.size();
    m_items.insert(index, widgetItem);
}

void QWidgetPlatformMenu::removeMenuItem(QPlatformMenuItem *item)
{
    QWidgetPlatformMenuItem *widgetItem = qobject_cast<QWidgetPlatformMenuItem *>(item);
    if (!widgetItem)
        return;

    m_items.removeOne(widgetItem);
    m_menu->removeAction(widgetItem->action());
}

void QWidgetPlatformMenu::syncMenuItem(QPlatformMenuItem *item)
{
    Q_UNUSED(item);
}

void QWidgetPlatformMenu::syncSeparatorsCollapsible(bool enable)
{
    m_menu->setSeparatorsCollapsible(enable);
}

void QWidgetPlatformMenu::setText(const QString &text)
{
    m_menu->setTitle(text);
}

void QWidgetPlatformMenu::setIcon(const QIcon &icon)
{
    m_menu->setIcon(icon);
}

void QWidgetPlatformMenu::setEnabled(bool enabled)
{
    m_menu->menuAction()->setEnabled(enabled);
}

bool QWidgetPlatformMenu::isEnabled() const
{
    return m_menu->menuAction()->isEnabled();
}

void QWidgetPlatformMenu::setVisible(bool visible)
{
    m_menu->menuAction()->setVisible(visible);
}

void QWidgetPlatformMenu::setMinimumWidth(int width)
{
    if (width > 0)
        m_menu->setMinimumWidth(width);
}

void QWidgetPlatformMenu::setFont(const QFont &font)
{
    m_menu->setFont(font);
}

void QWidgetPlatformMenu::setMenuType(MenuType type)
{
    Q_UNUSED(type);
}

void QWidgetPlatformMenu::showPopup(const QWindow *window, const QRect &targetRect, const QPlatformMenuItem *item)
{
    m_menu->createWinId();
    QWindow *handle = m_menu->windowHandle();
    Q_ASSERT(handle);
    handle->setTransientParent(const_cast<QWindow *>(window));

    QPoint targetPos = targetRect.bottomLeft();
    if (window)
        targetPos = window->mapToGlobal(QHighDpi::fromNativeLocalPosition(targetPos, window));

    const QWidgetPlatformMenuItem *widgetItem = qobject_cast<const QWidgetPlatformMenuItem *>(item);
    m_menu->popup(targetPos, widgetItem ? widgetItem->action() : nullptr);
}

void QWidgetPlatformMenu::dismiss()
{
    m_menu->close();
}

QPlatformMenuItem *QWidgetPlatformMenu::menuItemAt(int position) const
{
    return m_items.value(position);
}

QPlatformMenuItem *QWidgetPlatformMenu::menuItemForTag(quintptr tag) const
{
    for (QWidgetPlatformMenuItem *item : m_items) {
        if (item->tag() == tag)
            return item;
    }
    return nullptr;
}

QPlatformMenuItem *QWidgetPlatformMenu::createMenuItem() const
{
    return new QWidgetPlatformMenuItem;
}

QPlatformMenu *QWidgetPlatformMenu::createSubMenu() const
{
    return new QWidgetPlatformMenu;
}

QT_END_NAMESPACE

#include "moc_qwidgetplatformmenu_p.cpp"
