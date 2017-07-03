/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Templates 2 module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickmenu_p.h"
#include "qquickmenu_p_p.h"
#include "qquickmenuitem_p_p.h"
#include "qquickpopupitem_p_p.h"
#include "qquickaction_p.h"

#include <QtGui/qevent.h>
#include <QtGui/qcursor.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlcomponent.h>
#include <QtQml/private/qqmlengine_p.h>
#include <QtQml/private/qv4scopedvalue_p.h>
#include <QtQml/private/qv4variantobject_p.h>
#include <QtQml/private/qv4qobjectwrapper_p.h>
#include <QtQml/private/qqmlobjectmodel_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/private/qquickitemview_p.h>
#include <QtQuick/private/qquickevents_p_p.h>
#include <QtQuick/private/qquickwindow_p.h>

QT_BEGIN_NAMESPACE

// copied from qfusionstyle.cpp
static const int SUBMENU_DELAY = 225;

/*!
    \qmltype Menu
    \inherits Popup
    \instantiates QQuickMenu
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-menus
    \ingroup qtquickcontrols2-popups
    \brief Menu popup that can be used as a context menu or popup menu.

    \image qtquickcontrols2-menu.png

    Menu has two main use cases:
    \list
        \li Context menus; for example, a menu that is shown after right clicking
        \li Popup menus; for example, a menu that is shown after clicking a button
    \endlist

    When used as a context menu, the recommended way of opening the menu is to call
    \l popup(). Unless a position is explicitly specified, the menu is positioned at
    the mouse cursor on desktop platforms that have a mouse cursor available, and
    otherwise centered over its parent item.

    \code
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: {
            if (mouse.button === Qt.RightButton)
                contextMenu.popup()
        }
        onPressAndHold: {
            if (mouse.source === Qt.MouseEventNotSynthesized)
                contextMenu.popup()
        }

        Menu {
            id: contextMenu
            MenuItem { text: "Cut" }
            MenuItem { text: "Copy" }
            MenuItem { text: "Paste" }
        }
    }
    \endcode

    When used as a popup menu, it is easiest to specify the position by specifying
    the desired \l {Popup::}{x} and \l {Popup::}{y} coordinates using the respective
    properties, and call \l {Popup::}{open()} to open the menu.

    \code
    Button {
        id: fileButton
        text: "File"
        onClicked: menu.open()

        Menu {
            id: menu
            y: fileButton.height

            MenuItem {
                text: "New..."
            }
            MenuItem {
                text: "Open..."
            }
            MenuItem {
                text: "Save"
            }
        }
    }
    \endcode

    Since QtQuick.Controls 2.3 (Qt 5.10), it is also possible to create sub-menus
    and declare Action objects inside Menu:

    \code
    Menu {
        Action { text: "Cut" }
        Action { text: "Copy" }
        Action { text: "Paste" }

        MenuSeparator { }

        Menu {
            title: "Find/Replace"
            Action { text: "Find Next" }
            Action { text: "Find Previous" }
            Action { text: "Replace" }
        }
    }
    \endcode

    Sub-menus are \l {cascade}{cascading} by default on desktop platforms
    that have a mouse cursor available. Non-cascading menus are shown one
    menu at a time, and centered over the parent menu.

    Typically, menu items are statically declared as children of the menu, but
    Menu also provides API to \l {addItem}{add}, \l {insertItem}{insert},
    \l {moveItem}{move} and \l {removeItem}{remove} items dynamically. The
    items in a menu can be accessed using \l itemAt() or
    \l {Popup::}{contentChildren}.

    Although \l {MenuItem}{MenuItems} are most commonly used with Menu, it can
    contain any type of item.

    \sa {Customizing Menu}, MenuItem, {Menu Controls}, {Popup Controls}
*/

static const QQuickPopup::ClosePolicy defaultMenuClosePolicy = QQuickPopup::CloseOnEscape | QQuickPopup::CloseOnPressOutside;
static const QQuickPopup::ClosePolicy cascadingSubMenuClosePolicy = QQuickPopup::CloseOnEscape | QQuickPopup::CloseOnPressOutsideParent;

static bool shouldCascade()
{
#if QT_CONFIG(cursor)
    return QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::MultipleWindows);
#else
    return false;
#endif
}

QQuickMenuPrivate::QQuickMenuPrivate()
    : cascade(shouldCascade()),
      hoverTimer(0),
      currentIndex(-1),
      overlap(0),
      contentItem(nullptr),
      contentModel(nullptr),
      delegate(nullptr)
{
    Q_Q(QQuickMenu);
    contentModel = new QQmlObjectModel(q);
}

QQuickItem *QQuickMenuPrivate::itemAt(int index) const
{
    return qobject_cast<QQuickItem *>(contentModel->get(index));
}

void QQuickMenuPrivate::insertItem(int index, QQuickItem *item)
{
    contentData.append(item);
    item->setParentItem(contentItem);
    if (qobject_cast<QQuickItemView *>(contentItem))
        QQuickItemPrivate::get(item)->setCulled(true); // QTBUG-53262
    if (complete)
        resizeItem(item);
    QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
    contentModel->insert(index, item);

    QQuickMenuItem *menuItem = qobject_cast<QQuickMenuItem *>(item);
    if (menuItem) {
        Q_Q(QQuickMenu);
        QQuickMenuItemPrivate::get(menuItem)->setMenu(q);
        QObjectPrivate::connect(menuItem, &QQuickMenuItem::triggered, this, &QQuickMenuPrivate::onItemTriggered);
        QObjectPrivate::connect(menuItem, &QQuickItem::activeFocusChanged, this, &QQuickMenuPrivate::onItemActiveFocusChanged);
        QObjectPrivate::connect(menuItem, &QQuickControl::hoveredChanged, this, &QQuickMenuPrivate::onItemHovered);
    }
}

void QQuickMenuPrivate::moveItem(int from, int to)
{
    contentModel->move(from, to);
}

void QQuickMenuPrivate::removeItem(int index, QQuickItem *item)
{
    contentData.removeOne(item);

    QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
    item->setParentItem(nullptr);
    contentModel->remove(index);

    QQuickMenuItem *menuItem = qobject_cast<QQuickMenuItem *>(item);
    if (menuItem) {
        QQuickMenuItemPrivate::get(menuItem)->setMenu(nullptr);
        QObjectPrivate::disconnect(menuItem, &QQuickMenuItem::triggered, this, &QQuickMenuPrivate::onItemTriggered);
        QObjectPrivate::disconnect(menuItem, &QQuickItem::activeFocusChanged, this, &QQuickMenuPrivate::onItemActiveFocusChanged);
        QObjectPrivate::disconnect(menuItem, &QQuickControl::hoveredChanged, this, &QQuickMenuPrivate::onItemHovered);
    }
}

QQuickItem *QQuickMenuPrivate::beginCreateItem()
{
    Q_Q(QQuickMenu);
    if (!delegate)
        return nullptr;

    QQmlContext *creationContext = delegate->creationContext();
    if (!creationContext)
        creationContext = qmlContext(q);
    QQmlContext *context = new QQmlContext(creationContext, q);
    context->setContextObject(q);

    QObject *object = delegate->beginCreate(context);
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item)
        delete object;

    QQml_setParent_noEvent(item, q);

    return item;
}

void QQuickMenuPrivate::completeCreateItem()
{
    if (!delegate)
        return;

    delegate->completeCreate();
}

QQuickItem *QQuickMenuPrivate::createItem(QQuickMenu *menu)
{
    QQuickItem *item = beginCreateItem();
    if (QQuickMenuItem *menuItem = qobject_cast<QQuickMenuItem *>(item))
        QQuickMenuItemPrivate::get(menuItem)->setSubMenu(menu);
    completeCreateItem();
    return item;
}

QQuickItem *QQuickMenuPrivate::createItem(QQuickAction *action)
{
    QQuickItem *item = beginCreateItem();
    if (QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(item))
        button->setAction(action);
    completeCreateItem();
    return item;
}

void QQuickMenuPrivate::resizeItem(QQuickItem *item)
{
    if (!item || !contentItem)
        return;

    QQuickItemPrivate *p = QQuickItemPrivate::get(item);
    if (!p->widthValid) {
        item->setWidth(contentItem->width());
        p->widthValid = false;
    }
}

void QQuickMenuPrivate::resizeItems()
{
    if (!contentModel)
        return;

    for (int i = 0; i < contentModel->count(); ++i)
        resizeItem(itemAt(i));
}

void QQuickMenuPrivate::itemChildAdded(QQuickItem *, QQuickItem *child)
{
    // add dynamically reparented items (eg. by a Repeater)
    if (!QQuickItemPrivate::get(child)->isTransparentForPositioner() && !contentData.contains(child))
        insertItem(contentModel->count(), child);
}

void QQuickMenuPrivate::itemParentChanged(QQuickItem *item, QQuickItem *parent)
{
    // remove dynamically unparented items (eg. by a Repeater)
    if (!parent)
        removeItem(contentModel->indexOf(item, nullptr), item);
}

void QQuickMenuPrivate::itemSiblingOrderChanged(QQuickItem *)
{
    // reorder the restacked items (eg. by a Repeater)
    Q_Q(QQuickMenu);
    QList<QQuickItem *> siblings = contentItem->childItems();

    int to = 0;
    for (int i = 0; i < siblings.count(); ++i) {
        QQuickItem* sibling = siblings.at(i);
        if (QQuickItemPrivate::get(sibling)->isTransparentForPositioner())
            continue;
        int index = contentModel->indexOf(sibling, nullptr);
        q->moveItem(index, to++);
    }
}

void QQuickMenuPrivate::itemDestroyed(QQuickItem *item)
{
    QQuickPopupPrivate::itemDestroyed(item);
    int index = contentModel->indexOf(item, nullptr);
    if (index != -1)
        removeItem(index, item);
}

void QQuickMenuPrivate::itemGeometryChanged(QQuickItem *, QQuickGeometryChange, const QRectF &)
{
    if (complete)
        resizeItems();
}

bool QQuickMenuPrivate::blockInput(QQuickItem *item, const QPointF &point) const
{
    // keep the parent menu open when a cascading sub-menu (this menu) is interacted with
    return (cascade && parentMenu && contains(point)) || QQuickPopupPrivate::blockInput(item, point);
}

void QQuickMenuPrivate::onItemHovered()
{
    Q_Q(QQuickMenu);
    QQuickAbstractButton *button = qobject_cast<QQuickAbstractButton *>(q->sender());
    if (!button || !button->isHovered() || QQuickAbstractButtonPrivate::get(button)->touchId != -1)
        return;

    QQuickMenuItem *oldCurrentItem = currentItem;

    int index = contentModel->indexOf(button, nullptr);
    if (index != -1) {
        setCurrentIndex(index, Qt::OtherFocusReason);
        if (oldCurrentItem != currentItem) {
            if (oldCurrentItem)
                closeSubMenu(oldCurrentItem->subMenu());
            if (currentItem) {
                QQuickMenu *subMenu = currentItem->menu();
                if (subMenu && subMenu->cascade())
                    startHoverTimer();
            }
        }
    }
}

void QQuickMenuPrivate::onItemTriggered()
{
    Q_Q(QQuickMenu);
    QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(q->sender());
    if (!item)
        return;

    if (item->subMenu()) {
        openSubMenu(item, true);
    } else {
        // close the whole chain of menus
        q->close();
        while (parentMenu) {
            parentMenu->close();
            parentMenu = QQuickMenuPrivate::get(parentMenu)->parentMenu;
        }
    }
}

void QQuickMenuPrivate::onItemActiveFocusChanged()
{
    Q_Q(QQuickMenu);
    QQuickItem *item = qobject_cast<QQuickItem*>(q->sender());
    if (!item->hasActiveFocus())
        return;

    int indexOfItem = contentModel->indexOf(item, nullptr);
    QQuickControl *control = qobject_cast<QQuickControl *>(item);
    setCurrentIndex(indexOfItem, control ? control->focusReason() : Qt::OtherFocusReason);
}

void QQuickMenuPrivate::openSubMenu(QQuickMenuItem *item, bool activate)
{
    Q_Q(QQuickMenu);
    QQuickMenu *subMenu = item ? item->subMenu() : nullptr;
    if (!subMenu)
        return;

    if (cascade) {
        subMenu->setParentItem(item);
        subMenu->setClosePolicy(cascadingSubMenuClosePolicy);
        if (popupItem->isMirrored()) {
            subMenu->setTransformOrigin(QQuickPopup::TopRight);
            subMenu->setPosition(QPointF(-subMenu->width() - q->leftPadding() + subMenu->overlap(), -subMenu->topPadding()));
        } else {
            subMenu->setTransformOrigin(QQuickPopup::TopLeft);
            subMenu->setPosition(QPointF(item->width() + q->rightPadding() - subMenu->overlap(), -subMenu->topPadding()));
        }
    } else {
        subMenu->setParentItem(parentItem);
        subMenu->setClosePolicy(defaultMenuClosePolicy);
        subMenu->setTransformOrigin(QQuickPopup::Center);
        subMenu->setPosition(QPointF(q->x() + (q->width() - subMenu->width()) / 2,
                                     q->y() + (q->height() - subMenu->height()) / 2));
    }

    QQuickMenuPrivate *p = QQuickMenuPrivate::get(subMenu);
    p->allowHorizontalFlip = cascade;
    p->parentMenu = q;
    if (activate)
        p->setCurrentIndex(0, Qt::PopupFocusReason);
    subMenu->setCascade(cascade);
    subMenu->open();

    // transfer focus to the sub-menu
    if (focus)
        subMenu->popupItem()->setFocus(true);

    if (!subMenu->cascade())
        q->close();
}

void QQuickMenuPrivate::closeSubMenu(QQuickMenu *subMenu)
{
    if (!subMenu || !subMenu->isVisible())
        return;

    // transfer focus back to the parent menu
    QQuickMenu *parentMenu = QQuickMenuPrivate::get(subMenu)->parentMenu;
    if (parentMenu && parentMenu->hasFocus()) {
        parentMenu->popupItem()->setFocus(true);
        if (!subMenu->cascade())
            parentMenu->open();
    }

    // close the whole chain of sub-menus
    while (subMenu) {
        QPointer<QQuickMenuItem> currentSubMenuItem = QQuickMenuPrivate::get(subMenu)->currentItem;
        subMenu->close();
        subMenu = currentSubMenuItem ? currentSubMenuItem->subMenu() : nullptr;
    }
}

void QQuickMenuPrivate::startHoverTimer()
{
    Q_Q(QQuickMenu);
    stopHoverTimer();
    hoverTimer = q->startTimer(SUBMENU_DELAY);
}

void QQuickMenuPrivate::stopHoverTimer()
{
    Q_Q(QQuickMenu);
    if (!hoverTimer)
        return;

    q->killTimer(hoverTimer);
    hoverTimer = 0;
}

void QQuickMenuPrivate::setCurrentIndex(int index, Qt::FocusReason reason)
{
    Q_Q(QQuickMenu);
    if (currentIndex == index)
        return;

    QQuickMenuItem *newCurrentItem = qobject_cast<QQuickMenuItem *>(itemAt(index));
    if (currentItem != newCurrentItem) {
        stopHoverTimer();
        if (currentItem) {
            currentItem->setHighlighted(false);
            if (!newCurrentItem && window) {
                QQuickItem *focusItem = QQuickItemPrivate::get(contentItem)->subFocusItem;
                if (focusItem)
                    QQuickWindowPrivate::get(window)->clearFocusInScope(contentItem, focusItem, Qt::OtherFocusReason);
            }
        }
        if (newCurrentItem) {
            newCurrentItem->setHighlighted(true);
            newCurrentItem->forceActiveFocus(reason);
        }
        currentItem = newCurrentItem;
    }

    currentIndex = index;
    emit q->currentIndexChanged();
}

void QQuickMenuPrivate::activateNextItem()
{
    int index = currentIndex;
    int count = contentModel->count();
    while (++index < count) {
        QQuickItem *item = itemAt(index);
        if (!item || !item->activeFocusOnTab())
            continue;
        setCurrentIndex(index, Qt::TabFocusReason);
        break;
    }
}

void QQuickMenuPrivate::activatePreviousItem()
{
    int index = currentIndex;
    while (--index >= 0) {
        QQuickItem *item = itemAt(index);
        if (!item || !item->activeFocusOnTab())
            continue;
        setCurrentIndex(index, Qt::BacktabFocusReason);
        break;
    }
}

void QQuickMenuPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickMenu *q = qobject_cast<QQuickMenu *>(prop->object);
    QQuickMenuPrivate *p = QQuickMenuPrivate::get(q);

    QQuickItem *item = qobject_cast<QQuickItem *>(obj);
    if (!item) {
        if (QQuickAction *action = qobject_cast<QQuickAction *>(obj))
            item = p->createItem(action);
        else if (QQuickMenu *menu = qobject_cast<QQuickMenu *>(obj))
            item = p->createItem(menu);
    }

    if (item) {
        if (QQuickItemPrivate::get(item)->isTransparentForPositioner()) {
            QQuickItemPrivate::get(item)->addItemChangeListener(p, QQuickItemPrivate::SiblingOrder);
            item->setParentItem(p->contentItem);
        } else if (p->contentModel->indexOf(item, nullptr) == -1) {
            q->addItem(item);
        }
    } else {
        p->contentData.append(obj);
    }
}

int QQuickMenuPrivate::contentData_count(QQmlListProperty<QObject> *prop)
{
    QQuickMenu *q = static_cast<QQuickMenu *>(prop->object);
    return QQuickMenuPrivate::get(q)->contentData.count();
}

QObject *QQuickMenuPrivate::contentData_at(QQmlListProperty<QObject> *prop, int index)
{
    QQuickMenu *q = static_cast<QQuickMenu *>(prop->object);
    return QQuickMenuPrivate::get(q)->contentData.value(index);
}

void QQuickMenuPrivate::contentData_clear(QQmlListProperty<QObject> *prop)
{
    QQuickMenu *q = static_cast<QQuickMenu *>(prop->object);
    QQuickMenuPrivate::get(q)->contentData.clear();
}

QQuickMenu::QQuickMenu(QObject *parent)
    : QQuickPopup(*(new QQuickMenuPrivate), parent)
{
    setFocus(true);
}

/*!
    \qmlmethod Item QtQuick.Controls::Menu::itemAt(int index)

    Returns the item at \a index, or \c null if it does not exist.
*/
QQuickItem *QQuickMenu::itemAt(int index) const
{
    Q_D(const QQuickMenu);
    return d->itemAt(index);
}

/*!
    \qmlmethod void QtQuick.Controls::Menu::addItem(Item item)

    Adds \a item to the end of the list of items.
*/
void QQuickMenu::addItem(QQuickItem *item)
{
    Q_D(QQuickMenu);
    insertItem(d->contentModel->count(), item);
}

/*!
    \qmlmethod void QtQuick.Controls::Menu::insertItem(int index, Item item)

    Inserts \a item at \a index.
*/
void QQuickMenu::insertItem(int index, QQuickItem *item)
{
    Q_D(QQuickMenu);
    if (!item)
        return;
    const int count = d->contentModel->count();
    if (index < 0 || index > count)
        index = count;

    int oldIndex = d->contentModel->indexOf(item, nullptr);
    if (oldIndex != -1) {
        if (oldIndex < index)
            --index;
        if (oldIndex != index)
            d->moveItem(oldIndex, index);
    } else {
        d->insertItem(index, item);
    }
}

/*!
    \qmlmethod void QtQuick.Controls::Menu::moveItem(int from, int to)

    Moves an item \a from one index \a to another.
*/
void QQuickMenu::moveItem(int from, int to)
{
    Q_D(QQuickMenu);
    const int count = d->contentModel->count();
    if (from < 0 || from > count - 1)
        return;
    if (to < 0 || to > count - 1)
        to = count - 1;

    if (from != to)
        d->moveItem(from, to);
}

/*!
    \deprecated
    \qmlmethod void QtQuick.Controls::Menu::removeItem(int index)

    Use Menu::removeItem(Item) or Menu::takeItem(int) instead.
*/
void QQuickMenu::removeItem(const QVariant &var)
{
    if (var.userType() == QMetaType::Nullptr)
        return;

    if (QQuickItem *item = var.value<QQuickItem *>())
        removeItem(item);
    else
        takeItem(var.toInt());
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::removeItem(Item item)

    Removes and destroys the specified \a item.
*/
void QQuickMenu::removeItem(QQuickItem *item)
{
    Q_D(QQuickMenu);
    if (!item)
        return;

    const int index = d->contentModel->indexOf(item, nullptr);
    if (index == -1)
        return;

    d->removeItem(index, item);
    item->deleteLater();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod MenuItem QtQuick.Controls::Menu::takeItem(int index)

    Removes and returns the item at \a index.

    \note The ownership of the item is transferred to the caller.
*/
QQuickItem *QQuickMenu::takeItem(int index)
{
    Q_D(QQuickMenu);
    const int count = d->contentModel->count();
    if (index < 0 || index >= count)
        return nullptr;

    QQuickItem *item = itemAt(index);
    if (item)
        d->removeItem(index, item);
    return item;
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::addMenu(Menu menu)

    Adds \a menu as a sub-menu to the end of this menu.
*/
void QQuickMenu::addMenu(QQuickMenu *menu)
{
    Q_D(QQuickMenu);
    insertMenu(d->contentModel->count(), menu);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::insertMenu(int index, Menu menu)

    Inserts \a menu as a sub-menu at \a index. The index is within all items in the menu.
*/
void QQuickMenu::insertMenu(int index, QQuickMenu *menu)
{
    Q_D(QQuickMenu);
    if (!menu)
        return;

    insertItem(index, d->createItem(menu));
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::removeMenu(Menu menu)

    Removes and destroys the specified \a menu.
*/
void QQuickMenu::removeMenu(QQuickMenu *menu)
{
    Q_D(QQuickMenu);
    if (!menu)
        return;

    const int count = d->contentModel->count();
    for (int i = 0; i < count; ++i) {
        QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(d->itemAt(i));
        if (!item || item->subMenu() != menu)
            continue;

        removeItem(item);
        break;
    }

    menu->deleteLater();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod Menu QtQuick.Controls::Menu::takeMenu(int index)

    Removes and returns the menu at \a index. The index is within all items in the menu.

    \note The ownership of the menu is transferred to the caller.
*/
QQuickMenu *QQuickMenu::takeMenu(int index)
{
    Q_D(QQuickMenu);
    QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(d->itemAt(index));
    if (!item)
        return nullptr;

    QQuickMenu *subMenu = item->subMenu();
    if (!subMenu)
        return nullptr;

    d->removeItem(index, item);
    item->deleteLater();
    return subMenu;
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::addAction(Action action)

    Adds \a action to the end of this menu.
*/
void QQuickMenu::addAction(QQuickAction *action)
{
    Q_D(QQuickMenu);
    insertAction(d->contentModel->count(), action);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::insertAction(int index, Action action)

    Inserts \a action at \a index. The index is within all items in the menu.
*/
void QQuickMenu::insertAction(int index, QQuickAction *action)
{
    Q_D(QQuickMenu);
    if (!action)
        return;

    insertItem(index, d->createItem(action));
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::removeAction(Action action)

    Removes and destroys the specified \a action.
*/
void QQuickMenu::removeAction(QQuickAction *action)
{
    Q_D(QQuickMenu);
    if (!action)
        return;

    const int count = d->contentModel->count();
    for (int i = 0; i < count; ++i) {
        QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(d->itemAt(i));
        if (!item || item->action() != action)
            continue;

        removeItem(item);
        break;
    }

    action->deleteLater();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod Action QtQuick.Controls::Menu::takeAction(int index)

    Removes and returns the action at \a index. The index is within all items in the menu.

    \note The ownership of the action is transferred to the caller.
*/
QQuickAction *QQuickMenu::takeAction(int index)
{
    Q_D(QQuickMenu);
    QQuickMenuItem *item = qobject_cast<QQuickMenuItem *>(d->itemAt(index));
    if (!item)
        return nullptr;

    QQuickAction *action = item->action();
    if (!action)
        return nullptr;

    d->removeItem(index, item);
    item->deleteLater();
    return action;
}

/*!
    \qmlproperty model QtQuick.Controls::Menu::contentModel
    \readonly

    This property holds the model used to display menu items.

    The content model is provided for visualization purposes. It can be assigned
    as a model to a content item that presents the contents of the menu.

    \code
    Menu {
        id: menu
        contentItem: ListView {
            model: menu.contentModel
        }
    }
    \endcode

    The model allows menu items to be statically declared as children of the
    menu.
*/
QVariant QQuickMenu::contentModel() const
{
    Q_D(const QQuickMenu);
    return QVariant::fromValue(d->contentModel);
}

/*!
    \qmlproperty list<Object> QtQuick.Controls::Menu::contentData
    \default

    This property holds the list of content data.

    The list contains all objects that have been declared in QML as children
    of the menu, and also items that have been dynamically added or
    inserted using the \l addItem() and \l insertItem() methods, respectively.

    \note Unlike \c contentChildren, \c contentData does include non-visual QML
    objects. It is not re-ordered when items are inserted or moved.

    \sa Item::data, {Popup::}{contentChildren}
*/
QQmlListProperty<QObject> QQuickMenu::contentData()
{
    return QQmlListProperty<QObject>(this, nullptr,
        QQuickMenuPrivate::contentData_append,
        QQuickMenuPrivate::contentData_count,
        QQuickMenuPrivate::contentData_at,
        QQuickMenuPrivate::contentData_clear);
}

/*!
    \qmlproperty string QtQuick.Controls::Menu::title

    This property holds the title for the menu.

    The title of a menu is often displayed in the text of a menu item when the
    menu is a submenu, and in the text of a tool button when it is in a
    menubar.
*/
QString QQuickMenu::title() const
{
    Q_D(const QQuickMenu);
    return d->title;
}

void QQuickMenu::setTitle(QString &title)
{
    Q_D(QQuickMenu);
    if (title == d->title)
        return;
    d->title = title;
    emit titleChanged(title);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty bool QtQuick.Controls::Menu::cascade

    This property holds whether the menu cascades its sub-menus.

    The default value is platform-specific. Menus are cascading by default on
    desktop platforms that have a mouse cursor available. Non-cascading menus
    are shown one menu at a time, and centered over the parent menu.

    \note Changing the value of the property has no effect while the menu is open.

    \sa overlap
*/
bool QQuickMenu::cascade() const
{
    Q_D(const QQuickMenu);
    return d->cascade;
}

void QQuickMenu::setCascade(bool cascade)
{
    Q_D(QQuickMenu);
    if (d->cascade == cascade)
        return;
    d->cascade = cascade;
    emit cascadeChanged();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty real QtQuick.Controls::Menu::overlap

    This property holds the amount of pixels by which the menu horizontally overlaps its parent menu.

    The property only has effect when the menu is used as a cascading sub-menu.

    The default value is style-specific.

    \note Changing the value of the property has no effect while the menu is open.

    \sa cascade
*/
qreal QQuickMenu::overlap() const
{
    Q_D(const QQuickMenu);
    return d->overlap;
}

void QQuickMenu::setOverlap(qreal overlap)
{
    Q_D(QQuickMenu);
    if (d->overlap == overlap)
        return;
    d->overlap = overlap;
    emit cascadeChanged();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty Component QtQuick.Controls::Menu::delegate

    This property holds the component that is used to create items
    to present actions.

    \code
    Menu {
        Action { text: "Cut" }
        Action { text: "Copy" }
        Action { text: "Paste" }
    }
    \endcode

    \sa Action
*/
QQmlComponent *QQuickMenu::delegate() const
{
    Q_D(const QQuickMenu);
    return d->delegate;
}

void QQuickMenu::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickMenu);
    if (d->delegate == delegate)
        return;

    d->delegate = delegate;
    emit delegateChanged();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty int QtQuick.Controls::Menu::currentIndex

    This property holds the index of the currently highlighted item.

    Menu items can be highlighted by mouse hover or keyboard navigation.

    \sa MenuItem::highlighted
*/
int QQuickMenu::currentIndex() const
{
    Q_D(const QQuickMenu);
    return d->currentIndex;
}

void QQuickMenu::setCurrentIndex(int index)
{
    Q_D(QQuickMenu);
    d->setCurrentIndex(index, Qt::OtherFocusReason);
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::popup(MenuItem item = null)

    Opens the menu at the mouse cursor on desktop platforms that have a mouse cursor
    available, and otherwise centers the menu over its parent item.

    The menu can be optionally aligned to a specific menu \a item.

    \sa Popup::open()
*/

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::popup(point pos, MenuItem item = null)

    Opens the menu at the specified position \a pos in the popups coordinate system,
    that is, a coordinate relative to its parent item.

    The menu can be optionally aligned to a specific menu \a item.

    \sa Popup::open()
*/

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlmethod void QtQuick.Controls::Menu::popup(real x, real y, MenuItem item = null)

    Opens the menu at the specified position \a x, \a y in the popups coordinate system,
    that is, a coordinate relative to its parent item.

    The menu can be optionally aligned to a specific menu \a item.

    \sa Popup::open()
*/
void QQuickMenu::popup(QQmlV4Function *args)
{
    Q_D(QQuickMenu);
    const int len = args->length();
    if (len > 3) {
        args->v4engine()->throwTypeError();
        return;
    }

    QV4::ExecutionEngine *v4 = args->v4engine();
    QV4::Scope scope(v4);

    QQmlNullableValue<QPointF> pos;
    QQuickMenuItem *menuItem = nullptr;

    if (len > 0) {
        // MenuItem item
        QV4::ScopedValue lastArg(scope, (*args)[len - 1]);
        const QV4::QObjectWrapper *obj = lastArg->as<QV4::QObjectWrapper>();
        if (obj)
            menuItem = qobject_cast<QQuickMenuItem *>(obj->object());
    }

    if (len >= 2) {
        // real x, real y
        QV4::ScopedValue firstArg(scope, (*args)[0]);
        QV4::ScopedValue secondArg(scope, (*args)[1]);
        if (firstArg->isNumber() && secondArg->isNumber())
            pos = QPointF(firstArg->asDouble(), secondArg->asDouble());
    }

    if (pos.isNull && len >= 1) {
        // point pos
        QV4::ScopedValue firstArg(scope, (*args)[0]);
        const QVariant var = v4->toVariant(firstArg, -1);
        if (var.userType() == QMetaType::QPointF)
            pos = var.toPointF();
    }

    // Unless the position has been explicitly specified, position the menu at
    // the mouse cursor on desktop platforms that have a mouse cursor available
    // and support multiple windows.
#if QT_CONFIG(cursor)
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::MultipleWindows)) {
        if (pos.isNull && d->parentItem)
            pos = d->parentItem->mapFromGlobal(QCursor::pos());
        if (menuItem)
            pos.value.ry() -= d->popupItem->mapFromItem(menuItem, QPointF(0, 0)).y();
    }
#endif

    // As a fallback, center the menu over its parent item.
    if (pos.isNull && d->parentItem)
        pos = QPointF((d->parentItem->width() - width()) / 2, (d->parentItem->height() - height()) / 2);

    if (!pos.isNull)
        setPosition(pos);

    if (menuItem)
        d->setCurrentIndex(d->contentModel->indexOf(menuItem, nullptr), Qt::PopupFocusReason);

    open();
}

void QQuickMenu::componentComplete()
{
    Q_D(QQuickMenu);
    QQuickPopup::componentComplete();
    d->resizeItems();
}

void QQuickMenu::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickMenu);
    QQuickPopup::contentItemChange(newItem, oldItem);

    if (oldItem)
        QQuickItemPrivate::get(oldItem)->removeItemChangeListener(d, QQuickItemPrivate::Children);
    if (newItem)
        QQuickItemPrivate::get(newItem)->addItemChangeListener(d, QQuickItemPrivate::Children);

    d->contentItem = newItem;
}

void QQuickMenu::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data)
{
    Q_D(QQuickMenu);
    QQuickPopup::itemChange(change, data);

    if (change == QQuickItem::ItemVisibleHasChanged) {
        if (!data.boolValue && d->cascade) {
            // Ensure that when the menu isn't visible, there's no current item
            // the next time it's opened.
            d->setCurrentIndex(-1, Qt::OtherFocusReason);
        }
    }
}

void QQuickMenu::keyReleaseEvent(QKeyEvent *event)
{
    Q_D(QQuickMenu);
    QQuickPopup::keyReleaseEvent(event);
    if (d->contentModel->count() == 0)
        return;

    // QTBUG-17051
    // Work around the fact that ListView has no way of distinguishing between
    // mouse and keyboard interaction, thanks to the "interactive" bool in Flickable.
    // What we actually want is to have a way to always allow keyboard interaction but
    // only allow flicking with the mouse when there are too many menu items to be
    // shown at once.
    switch (event->key()) {
    case Qt::Key_Up:
        d->activatePreviousItem();
        break;

    case Qt::Key_Down:
        d->activateNextItem();
        break;

    case Qt::Key_Left:
    case Qt::Key_Right:
        if (d->popupItem->isMirrored() == (event->key() == Qt::Key_Right)) {
            if (d->parentMenu && d->currentItem)
                d->closeSubMenu(this);
        } else {
            d->openSubMenu(d->currentItem, true);
        }
        return;

    default:
        break;
    }
}

void QQuickMenu::timerEvent(QTimerEvent *event)
{
    Q_D(QQuickMenu);
    if (event->timerId() == d->hoverTimer) {
        d->openSubMenu(d->currentItem, false);
        d->stopHoverTimer();
    }
}

QFont QQuickMenu::defaultFont() const
{
    return QQuickControlPrivate::themeFont(QPlatformTheme::MenuFont);
}

QPalette QQuickMenu::defaultPalette() const
{
    return QQuickControlPrivate::themePalette(QPlatformTheme::MenuPalette);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickMenu::accessibleRole() const
{
    return QAccessible::PopupMenu;
}
#endif

QT_END_NAMESPACE

#include "moc_qquickmenu_p.cpp"
