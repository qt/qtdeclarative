// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquicklabsplatformmenuitemgroup_p.h"
#include "qquicklabsplatformmenuitem_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype MenuItemGroup
    \inherits QtObject
//!     \instantiates QQuickLabsPlatformMenuItemGroup
    \inqmlmodule Qt.labs.platform
    \since 5.8
    \brief A group for managing native menu items.

    The MenuItemGroup groups native menu items together.

    MenuItemGroup is exclusive by default. In an exclusive menu item
    group, only one item can be checked at any time; checking another
    item automatically unchecks the previously checked one. MenuItemGroup
    can be configured as non-exclusive, which is particularly useful for
    showing, hiding, enabling and disabling items together as a group.

    The most straight-forward way to use MenuItemGroup is to assign
    a list of items.

    \code
    Menu {
        id: verticalMenu
        title: qsTr("Vertical")

        MenuItemGroup {
            id: verticalGroup
            items: verticalMenu.items
        }

        MenuItem { text: qsTr("Top"); checkable: true }
        MenuItem { text: qsTr("Center"); checked: true }
        MenuItem { text: qsTr("Bottom"); checkable: true }
    }
    \endcode

    The same menu may sometimes contain items that should not be included
    in the same exclusive group. Such cases are best handled using the
    \l {MenuItem::group}{group} property.

    \code
    Menu {
        id: horizontalMenu
        title: qsTr("Horizontal")

        MenuItemGroup {
            id: horizontalGroup
        }

        MenuItem {
            checked: true
            text: qsTr("Left")
            group: horizontalGroup
        }
        MenuItem {
            checkable: true
            text: qsTr("Center")
            group: horizontalGroup
        }
        MenuItem {
            text: qsTr("Right")
            checkable: true
            group: horizontalGroup
        }

        MenuItem { separator: true }
        MenuItem { text: qsTr("Justify"); checkable: true }
        MenuItem { text: qsTr("Absolute"); checkable: true }
    }
    \endcode

    More advanced use cases can be handled using the addItem() and
    removeItem() methods.

    \labs

    \sa MenuItem
*/

/*!
    \qmlsignal Qt.labs.platform::MenuItemGroup::triggered(MenuItem item)

    This signal is emitted when an \a item in the group is triggered by the user.

    \sa MenuItem::triggered()
*/

/*!
    \qmlsignal Qt.labs.platform::MenuItemGroup::hovered(MenuItem item)

    This signal is emitted when an \a item in the group is hovered by the user.

    \sa MenuItem::hovered()
*/

QQuickLabsPlatformMenuItemGroup::QQuickLabsPlatformMenuItemGroup(QObject *parent)
    : QObject(parent), m_enabled(true), m_visible(true), m_exclusive(true), m_checkedItem(nullptr)
{
}

QQuickLabsPlatformMenuItemGroup::~QQuickLabsPlatformMenuItemGroup()
{
    clear();
}

/*!
    \qmlproperty bool Qt.labs.platform::MenuItemGroup::enabled

    This property holds whether the group is enabled. The default value is \c true.

    The enabled state of the group affects the enabled state of each item in the group,
    except that explicitly disabled items are not enabled even if the group is enabled.
*/
bool QQuickLabsPlatformMenuItemGroup::isEnabled() const
{
    return m_enabled;
}

void QQuickLabsPlatformMenuItemGroup::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;
    emit enabledChanged();

    for (QQuickLabsPlatformMenuItem *item : std::as_const(m_items)) {
        if (item->m_enabled) {
            item->sync();
            emit item->enabledChanged();
        }
    }
}

/*!
    \qmlproperty bool Qt.labs.platform::MenuItemGroup::visible

    This property holds whether the group is visible. The default value is \c true.

    The visibility of the group affects the visibility of each item in the group,
    except that explicitly hidden items are not visible even if the group is visible.
*/
bool QQuickLabsPlatformMenuItemGroup::isVisible() const
{
    return m_visible;
}

void QQuickLabsPlatformMenuItemGroup::setVisible(bool visible)
{
    if (m_visible == visible)
        return;

    m_visible = visible;
    emit visibleChanged();

    for (QQuickLabsPlatformMenuItem *item : std::as_const(m_items)) {
        if (item->m_visible) {
            item->sync();
            emit item->visibleChanged();
        }
    }
}

/*!
    \qmlproperty bool Qt.labs.platform::MenuItemGroup::exclusive

    This property holds whether the group is exclusive. The default value is \c true.

    In an exclusive menu item group, only one item can be checked at any time;
    checking another item automatically unchecks the previously checked one.
*/
bool QQuickLabsPlatformMenuItemGroup::isExclusive() const
{
    return m_exclusive;
}

void QQuickLabsPlatformMenuItemGroup::setExclusive(bool exclusive)
{
    if (m_exclusive == exclusive)
        return;

    m_exclusive = exclusive;
    emit exclusiveChanged();

    for (QQuickLabsPlatformMenuItem *item : std::as_const(m_items))
        item->sync();
}

/*!
    \qmlproperty MenuItem Qt.labs.platform::MenuItemGroup::checkedItem

    This property holds the currently checked item in the group, or \c null if no item is checked.
*/
QQuickLabsPlatformMenuItem *QQuickLabsPlatformMenuItemGroup::checkedItem() const
{
    return m_checkedItem;
}

void QQuickLabsPlatformMenuItemGroup::setCheckedItem(QQuickLabsPlatformMenuItem *item)
{
    if (m_checkedItem == item)
        return;

    if (m_checkedItem)
        m_checkedItem->setChecked(false);

    m_checkedItem = item;
    emit checkedItemChanged();

    if (item)
        item->setChecked(true);
}

/*!
    \qmlproperty list<MenuItem> Qt.labs.platform::MenuItemGroup::items

    This property holds the list of items in the group.
*/
QQmlListProperty<QQuickLabsPlatformMenuItem> QQuickLabsPlatformMenuItemGroup::items()
{
    return QQmlListProperty<QQuickLabsPlatformMenuItem>(this, nullptr, items_append, items_count, items_at, items_clear);
}

/*!
    \qmlmethod void Qt.labs.platform::MenuItemGroup::addItem(MenuItem item)

    Adds an \a item to the group.
*/
void QQuickLabsPlatformMenuItemGroup::addItem(QQuickLabsPlatformMenuItem *item)
{
    if (!item || m_items.contains(item))
        return;

    m_items.append(item);
    item->setGroup(this);

    connect(item, &QQuickLabsPlatformMenuItem::checkedChanged, this, &QQuickLabsPlatformMenuItemGroup::updateCurrent);
    connect(item, &QQuickLabsPlatformMenuItem::triggered, this, &QQuickLabsPlatformMenuItemGroup::activateItem);
    connect(item, &QQuickLabsPlatformMenuItem::hovered, this, &QQuickLabsPlatformMenuItemGroup::hoverItem);

    if (m_exclusive && item->isChecked())
        setCheckedItem(item);

    emit itemsChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::MenuItemGroup::removeItem(MenuItem item)

    Removes an \a item from the group.
*/
void QQuickLabsPlatformMenuItemGroup::removeItem(QQuickLabsPlatformMenuItem *item)
{
    if (!item || !m_items.contains(item))
        return;

    m_items.removeOne(item);
    item->setGroup(nullptr);

    disconnect(item, &QQuickLabsPlatformMenuItem::checkedChanged, this, &QQuickLabsPlatformMenuItemGroup::updateCurrent);
    disconnect(item, &QQuickLabsPlatformMenuItem::triggered, this, &QQuickLabsPlatformMenuItemGroup::activateItem);
    disconnect(item, &QQuickLabsPlatformMenuItem::hovered, this, &QQuickLabsPlatformMenuItemGroup::hoverItem);

    if (m_checkedItem == item)
        setCheckedItem(nullptr);

    emit itemsChanged();
}

/*!
    \qmlmethod void Qt.labs.platform::MenuItemGroup::clear()

    Removes all items from the group.
*/
void QQuickLabsPlatformMenuItemGroup::clear()
{
    if (m_items.isEmpty())
        return;

    for (QQuickLabsPlatformMenuItem *item : std::as_const(m_items)) {
        item->setGroup(nullptr);
        disconnect(item, &QQuickLabsPlatformMenuItem::checkedChanged, this, &QQuickLabsPlatformMenuItemGroup::updateCurrent);
        disconnect(item, &QQuickLabsPlatformMenuItem::triggered, this, &QQuickLabsPlatformMenuItemGroup::activateItem);
        disconnect(item, &QQuickLabsPlatformMenuItem::hovered, this, &QQuickLabsPlatformMenuItemGroup::hoverItem);
    }

    setCheckedItem(nullptr);

    m_items.clear();
    emit itemsChanged();
}

QQuickLabsPlatformMenuItem *QQuickLabsPlatformMenuItemGroup::findCurrent() const
{
    for (QQuickLabsPlatformMenuItem *item : m_items) {
        if (item->isChecked())
            return item;
    }
    return nullptr;
}

void QQuickLabsPlatformMenuItemGroup::updateCurrent()
{
    if (!m_exclusive)
        return;

    QQuickLabsPlatformMenuItem *item = qobject_cast<QQuickLabsPlatformMenuItem*>(sender());
    if (item && item->isChecked())
        setCheckedItem(item);
}

void QQuickLabsPlatformMenuItemGroup::activateItem()
{
    QQuickLabsPlatformMenuItem *item = qobject_cast<QQuickLabsPlatformMenuItem*>(sender());
    if (item)
        emit triggered(item);
}

void QQuickLabsPlatformMenuItemGroup::hoverItem()
{
    QQuickLabsPlatformMenuItem *item = qobject_cast<QQuickLabsPlatformMenuItem*>(sender());
    if (item)
        emit hovered(item);
}

void QQuickLabsPlatformMenuItemGroup::items_append(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop, QQuickLabsPlatformMenuItem *item)
{
    QQuickLabsPlatformMenuItemGroup *group = static_cast<QQuickLabsPlatformMenuItemGroup *>(prop->object);
    group->addItem(item);
}

qsizetype QQuickLabsPlatformMenuItemGroup::items_count(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop)
{
    QQuickLabsPlatformMenuItemGroup *group = static_cast<QQuickLabsPlatformMenuItemGroup *>(prop->object);
    return group->m_items.size();
}

QQuickLabsPlatformMenuItem *QQuickLabsPlatformMenuItemGroup::items_at(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop, qsizetype index)
{
    QQuickLabsPlatformMenuItemGroup *group = static_cast<QQuickLabsPlatformMenuItemGroup *>(prop->object);
    return group->m_items.value(index);
}

void QQuickLabsPlatformMenuItemGroup::items_clear(QQmlListProperty<QQuickLabsPlatformMenuItem> *prop)
{
    QQuickLabsPlatformMenuItemGroup *group = static_cast<QQuickLabsPlatformMenuItemGroup *>(prop->object);
    group->clear();
}

QT_END_NAMESPACE

#include "moc_qquicklabsplatformmenuitemgroup_p.cpp"
