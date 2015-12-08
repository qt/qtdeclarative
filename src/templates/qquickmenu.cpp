/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Templates module of the Qt Toolkit.
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
#include "qquickmenuitem_p.h"

#include <QtGui/qevent.h>
#include <QtQml/private/qqmlobjectmodel_p.h>
#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuick/private/qquickevents_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Menu
    \inherits Popup
    \instantiates QQuickMenu
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-menus
    \brief A menu control.

    \image qtlabscontrols-menu.png

    Menu has two main use cases:
    \list
        \li Context menus; for example, a menu that is shown after right clicking
        \li Popup menus; for example, a menu that is shown after clicking a button
    \endlist

    \code
    Button {
        id: fileButton
        text: "File"
        onClicked: menu.show()
    }
    Menu {
        id: menu
        contentItem.y: fileButton.height

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
    \endcode

    \sa {Customizing Menu}, {Menu Controls}
*/

QQuickMenuPrivate::QQuickMenuPrivate() :
    contentModel(Q_NULLPTR),
    dummyFocusItem(Q_NULLPTR),
    ignoreActiveFocusChanges(false)
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
    QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
    contentModel->insert(index, item);
}

void QQuickMenuPrivate::moveItem(int from, int to)
{
    contentModel->move(from, to);
}

void QQuickMenuPrivate::removeItem(int index, QQuickItem *item)
{
    contentData.removeOne(item);

    QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
    item->setParentItem(Q_NULLPTR);
    contentModel->remove(index);
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
        removeItem(contentModel->indexOf(item, Q_NULLPTR), item);
}

void QQuickMenuPrivate::itemSiblingOrderChanged(QQuickItem *)
{
    // reorder the restacked items (eg. by a Repeater)
    Q_Q(QQuickMenu);
    QList<QQuickItem *> siblings = contentItem->childItems();
    for (int i = 0; i < siblings.count(); ++i) {
        QQuickItem* sibling = siblings.at(i);
        int index = contentModel->indexOf(sibling, Q_NULLPTR);
        q->moveItem(index, i);
    }
}

void QQuickMenuPrivate::itemDestroyed(QQuickItem *item)
{
    int index = contentModel->indexOf(item, Q_NULLPTR);
    if (index != -1)
        removeItem(index, item);
}

void QQuickMenuPrivate::onContentItemChanged()
{
    Q_Q(QQuickMenu);
    if (contentItem) {
        contentItem->installEventFilter(q);
        contentItem->setFlag(QQuickItem::ItemIsFocusScope);
        contentItem->setActiveFocusOnTab(true);

        // Trying to give active focus to the contentItem (ListView, by default)
        // when the menu first opens, without also giving it to the first delegate item
        // doesn't seem to be possible, but this is what we need to do. QMenu behaves
        // similarly to this; it receives focus if a button that has it as a menu is clicked,
        // and only after pressing tab is the first menu item then given active focus.
        if (!dummyFocusItem) {
            dummyFocusItem = new QQuickItem(contentItem);
            dummyFocusItem->setObjectName(QStringLiteral("dummyMenuFocusItem"));
        } else {
            dummyFocusItem->setParentItem(contentItem);
        }

        dummyFocusItem->setActiveFocusOnTab(true);
        dummyFocusItem->stackBefore(contentItem->childItems().first());

        QObjectPrivate::connect(q, &QQuickMenu::visibleChanged, this, &QQuickMenuPrivate::onMenuVisibleChanged);
        QObjectPrivate::connect(dummyFocusItem, &QQuickItem::activeFocusChanged, this, &QQuickMenuPrivate::maybeUnsetDummyFocusOnTab);
    }
}

void QQuickMenuPrivate::onItemPressed()
{
    Q_Q(QQuickMenu);
    QQuickItem *item = qobject_cast<QQuickItem*>(q->sender());
    int itemIndex = contentModel->indexOf(item, Q_NULLPTR);
    Q_ASSERT(itemIndex != -1);

    if (!contentItem->property("currentIndex").isValid())
        return;

    contentItem->setProperty("currentIndex", itemIndex);
}

void QQuickMenuPrivate::onItemActiveFocusChanged()
{
    if (ignoreActiveFocusChanges)
        return;

    Q_Q(QQuickMenu);
    QQuickItem *item = qobject_cast<QQuickItem*>(q->sender());
    if (!item->hasActiveFocus())
        return;

    if (!contentItem->property("currentIndex").isValid())
        return;

    int indexOfItem = contentModel->indexOf(item, Q_NULLPTR);
    contentItem->setProperty("currentIndex", indexOfItem);
}

void QQuickMenuPrivate::onMenuVisibleChanged()
{
    Q_Q(QQuickMenu);
    if (q->isVisible()) {
        // Don't react to active focus changes here, as we're causing them.
        ignoreActiveFocusChanges = true;
        for (int i = 0; i < contentModel->count(); ++i) {
            QQuickItem *item = qobject_cast<QQuickItem*>(contentModel->get(i));
            item->setFocus(true);
        }
        ignoreActiveFocusChanges = false;

        // We must do this last so that none of the menu items have focus.
        dummyFocusItem->forceActiveFocus();
    } else {
        // Ensure that when the menu isn't visible, there's no current item
        // the next time it's opened.
        if (contentItem->property("currentIndex").isValid())
            contentItem->setProperty("currentIndex", -1);

        // The menu items are sneaky and will steal the focus if they can.
        for (int i = 0; i < contentModel->count(); ++i) {
            QQuickItem *item = qobject_cast<QQuickItem*>(contentModel->get(i));
            item->setFocus(false);
        }
    }

}

void QQuickMenuPrivate::maybeUnsetDummyFocusOnTab()
{
    if (!dummyFocusItem->hasActiveFocus()) {
        // Only unset the flag once the dummy item no longer has focus, otherwise we get warnings.
        dummyFocusItem->setActiveFocusOnTab(false);
    }
}

void QQuickMenuPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickMenuPrivate *p = static_cast<QQuickMenuPrivate *>(prop->data);
    QQuickMenu *q = static_cast<QQuickMenu *>(prop->object);
    QQuickItem *item = qobject_cast<QQuickItem *>(obj);
    if (item) {
        if (QQuickItemPrivate::get(item)->isTransparentForPositioner()) {
            QQuickItemPrivate::get(item)->addItemChangeListener(p, QQuickItemPrivate::SiblingOrder);
            item->setParentItem(p->contentItem);
        } else if (p->contentModel->indexOf(item, Q_NULLPTR) == -1) {
            q->addItem(item);

            QQuickMenuItem *menuItem = qobject_cast<QQuickMenuItem *>(item);
            if (menuItem) {
                QObjectPrivate::connect(menuItem, &QQuickMenuItem::pressed, p, &QQuickMenuPrivate::onItemPressed);
                QObject::connect(menuItem, &QQuickMenuItem::triggered, q, &QQuickPopup::hide);
                QObjectPrivate::connect(menuItem, &QQuickItem::activeFocusChanged, p, &QQuickMenuPrivate::onItemActiveFocusChanged);
            }
        }
    } else {
        p->contentData.append(obj);
    }
}

int QQuickMenuPrivate::contentData_count(QQmlListProperty<QObject> *prop)
{
    QQuickMenuPrivate *p = static_cast<QQuickMenuPrivate *>(prop->data);
    return p->contentData.count();
}

QObject *QQuickMenuPrivate::contentData_at(QQmlListProperty<QObject> *prop, int index)
{
    QQuickMenuPrivate *p = static_cast<QQuickMenuPrivate *>(prop->data);
    return p->contentData.value(index);
}

void QQuickMenuPrivate::contentData_clear(QQmlListProperty<QObject> *prop)
{
    QQuickMenuPrivate *p = static_cast<QQuickMenuPrivate *>(prop->data);
    p->contentData.clear();
}

QQuickMenu::QQuickMenu(QObject *parent) :
    QQuickPopup(*(new QQuickMenuPrivate), parent)
{
    Q_D(QQuickMenu);
    connect(this, &QQuickMenu::pressedOutside, this, &QQuickMenu::hide);
    connect(this, &QQuickMenu::releasedOutside, this, &QQuickMenu::hide);
    QObjectPrivate::connect(this, &QQuickMenu::contentItemChanged, d, &QQuickMenuPrivate::onContentItemChanged);
}

/*!
    \qmlmethod Item Qt.labs.controls::Menu::itemAt(int index)

    Returns the item at \a index, or \c null if it does not exist.
*/
QQuickItem *QQuickMenu::itemAt(int index) const
{
    Q_D(const QQuickMenu);
    return d->itemAt(index);
}

/*!
    \qmlmethod void Qt.labs.controls::Menu::addItem(Item item)

    Adds \a item to the end of the list of items.
*/
void QQuickMenu::addItem(QQuickItem *item)
{
    Q_D(QQuickMenu);
    insertItem(d->contentModel->count(), item);
}

/*!
    \qmlmethod void Qt.labs.controls::Menu::insertItem(int index, Item item)

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

    int oldIndex = d->contentModel->indexOf(item, Q_NULLPTR);
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
    \qmlmethod void Qt.labs.controls::Menu::moveItem(int from, int to)

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
    \qmlmethod void Qt.labs.controls::Menu::removeItem(int index)

    Removes an item at \a index.

    \note The ownership of the item is transferred to the caller.
*/
void QQuickMenu::removeItem(int index)
{
    Q_D(QQuickMenu);
    const int count = d->contentModel->count();
    if (index < 0 || index >= count)
        return;

    QQuickItem *item = itemAt(index);
    if (item)
        d->removeItem(index, item);
}

/*!
    \qmlproperty model Qt.labs.controls::Menu::contentModel
    \readonly

    This property holds the model used to display menu items.

    By default, the model is an \l ObjectModel, in order to allow declaring
    menu items as children of the menu.
*/
QVariant QQuickMenu::contentModel() const
{
    Q_D(const QQuickMenu);
    return QVariant::fromValue(d->contentModel);
}

/*!
    \qmlproperty list<Object> Qt.labs.controls::Menu::contentData
    \default

    This property holds the list of content data.

    \sa Item::data
*/
QQmlListProperty<QObject> QQuickMenu::contentData()
{
    Q_D(QQuickMenu);
    return QQmlListProperty<QObject>(this, d,
        QQuickMenuPrivate::contentData_append,
        QQuickMenuPrivate::contentData_count,
        QQuickMenuPrivate::contentData_at,
        QQuickMenuPrivate::contentData_clear);
}

bool QQuickMenu::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QQuickMenu);
    if (d->contentModel->count() == 0)
        return false;

    if (object != d->contentItem || event->type() != QEvent::KeyRelease)
        return false;

    // QTBUG-17051
    // Work around the fact that ListView has no way of distinguishing between
    // mouse and keyboard interaction, thanks to the "interactive" bool in Flickable.
    // What we actually want is to have a way to always allow keyboard interaction but
    // only allow flicking with the mouse when there are too many menu items to be
    // shown at once.
    QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
    if (keyEvent->key() == Qt::Key_Up) {
        if (d->contentItem->metaObject()->indexOfMethod("decrementCurrentIndex()") != -1)
            QMetaObject::invokeMethod(d->contentItem, "decrementCurrentIndex");
        return true;
    } else if (keyEvent->key() == Qt::Key_Down) {
        if (d->contentItem->metaObject()->indexOfMethod("incrementCurrentIndex()") != -1)
            QMetaObject::invokeMethod(d->contentItem, "incrementCurrentIndex");
        return true;
    } else if (keyEvent->key() == Qt::Key_Escape) {
        hide();
        return true;
    }

    return false;
}

QT_END_NAMESPACE

#include "moc_qquickmenu_p.cpp"
