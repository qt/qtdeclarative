/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls module of the Qt Toolkit.
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

#include "qquickcontainer_p.h"
#include "qquickcontainer_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype Container
    \inherits Control
    \instantiates QQuickContainer
    \inqmlmodule QtQuick.Controls
    \qmlabstract
    \internal
*/

QQuickContainerPrivate::QQuickContainerPrivate() : contentModel(Q_NULLPTR)
{
}

void QQuickContainerPrivate::init()
{
    Q_Q(QQuickContainer);
    contentModel = new QQmlObjectModel(q);
    QObject::connect(contentModel, &QQmlObjectModel::countChanged, q, &QQuickContainer::countChanged);
}

void QQuickContainerPrivate::cleanup()
{
    // ensure correct destruction order (QTBUG-46798)
    delete contentItem;
    const int count = contentModel->count();
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = itemAt(i);
        if (item) {
            QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
            delete item;
        }
    }
    delete contentModel;
}

QQuickItem *QQuickContainerPrivate::itemAt(int index) const
{
    return qobject_cast<QQuickItem *>(contentModel->get(index));
}

void QQuickContainerPrivate::insertItem(int index, QQuickItem *item)
{
    QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
    contentData.append(item);
    contentModel->insert(index, item);
}

void QQuickContainerPrivate::moveItem(int from, int to)
{
    contentModel->move(from, to);
}

void QQuickContainerPrivate::removeItem(int index, QQuickItem *item)
{
    QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
    contentData.removeOne(item);
    contentModel->remove(index);
}

void QQuickContainerPrivate::itemChildAdded(QQuickItem *, QQuickItem *child)
{
    // add dynamically reparented items (eg. by a Repeater)
    if (!QQuickItemPrivate::get(child)->isTransparentForPositioner() && contentModel->indexOf(child, Q_NULLPTR) == -1)
        insertItem(contentModel->count(), child);
}

void QQuickContainerPrivate::itemParentChanged(QQuickItem *item, QQuickItem *parent)
{
    // remove dynamically unparented items (eg. by a Repeater)
    if (!parent)
        removeItem(contentModel->indexOf(item, Q_NULLPTR), item);
}

void QQuickContainerPrivate::itemSiblingOrderChanged(QQuickItem *)
{
    // reorder the restacked items (eg. by a Repeater)
    Q_Q(QQuickContainer);
    QList<QQuickItem *> siblings = contentItem->childItems();
    for (int i = 0; i < siblings.count(); ++i) {
        QQuickItem* sibling = siblings.at(i);
        int index = contentModel->indexOf(sibling, Q_NULLPTR);
        q->moveItem(index, i);
    }
}

void QQuickContainerPrivate::itemDestroyed(QQuickItem *item)
{
    int index = contentModel->indexOf(item, Q_NULLPTR);
    if (index != -1)
        removeItem(index, item);
}

void QQuickContainerPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickContainerPrivate *p = static_cast<QQuickContainerPrivate *>(prop->data);
    QQuickContainer *q = static_cast<QQuickContainer *>(prop->object);
    QQuickItem *item = qobject_cast<QQuickItem *>(obj);
    if (item) {
        if (QQuickItemPrivate::get(item)->isTransparentForPositioner()) {
            QQuickItemPrivate::get(item)->addItemChangeListener(p, QQuickItemPrivate::SiblingOrder);
            item->setParentItem(p->contentItem);
        } else if (p->contentModel->indexOf(item, Q_NULLPTR) == -1) {
            q->addItem(item);
        }
    } else {
        p->contentData.append(obj);
    }
}

int QQuickContainerPrivate::contentData_count(QQmlListProperty<QObject> *prop)
{
    QQuickContainerPrivate *p = static_cast<QQuickContainerPrivate *>(prop->data);
    return p->contentData.count();
}

QObject *QQuickContainerPrivate::contentData_at(QQmlListProperty<QObject> *prop, int index)
{
    QQuickContainerPrivate *p = static_cast<QQuickContainerPrivate *>(prop->data);
    return p->contentData.value(index);
}

void QQuickContainerPrivate::contentData_clear(QQmlListProperty<QObject> *prop)
{
    QQuickContainerPrivate *p = static_cast<QQuickContainerPrivate *>(prop->data);
    p->contentData.clear();
}

void QQuickContainerPrivate::contentChildren_append(QQmlListProperty<QQuickItem> *prop, QQuickItem *item)
{
    QQuickContainer *q = static_cast<QQuickContainer *>(prop->object);
    q->addItem(item);
}

int QQuickContainerPrivate::contentChildren_count(QQmlListProperty<QQuickItem> *prop)
{
    QQuickContainerPrivate *p = static_cast<QQuickContainerPrivate *>(prop->data);
    return p->contentModel->count();
}

QQuickItem *QQuickContainerPrivate::contentChildren_at(QQmlListProperty<QQuickItem> *prop, int index)
{
    QQuickContainer *q = static_cast<QQuickContainer *>(prop->object);
    return q->itemAt(index);
}

void QQuickContainerPrivate::contentChildren_clear(QQmlListProperty<QQuickItem> *prop)
{
    QQuickContainerPrivate *p = static_cast<QQuickContainerPrivate *>(prop->data);
    p->contentModel->clear();
}

QQuickContainer::QQuickContainer(QQuickItem *parent) :
    QQuickControl(*(new QQuickContainerPrivate), parent)
{
    Q_D(QQuickContainer);
    d->init();
}

QQuickContainer::QQuickContainer(QQuickContainerPrivate &dd, QQuickItem *parent) :
    QQuickControl(dd, parent)
{
    Q_D(QQuickContainer);
    d->init();
}

QQuickContainer::~QQuickContainer()
{
    Q_D(QQuickContainer);
    d->cleanup();
}

/*!
    \qmlproperty int QtQuickControls2::Container::count
    \readonly

    This property holds the number of items.
*/
int QQuickContainer::count() const
{
    Q_D(const QQuickContainer);
    return d->contentModel->count();
}

/*!
    \qmlmethod Item QtQuickControls2::Container::itemAt(int index)

    Returns the item at \a index, or \c null if it does not exist.
*/
QQuickItem *QQuickContainer::itemAt(int index) const
{
    Q_D(const QQuickContainer);
    return d->itemAt(index);
}

/*!
    \qmlmethod void QtQuickControls2::Container::addItem(Item item)

    Adds an \a item.
*/
void QQuickContainer::addItem(QQuickItem *item)
{
    Q_D(QQuickContainer);
    insertItem(d->contentModel->count(), item);
}

/*!
    \qmlmethod void QtQuickControls2::Container::insertItem(int index, Item item)

    Inserts an \a item at \a index.
*/
void QQuickContainer::insertItem(int index, QQuickItem *item)
{
    Q_D(QQuickContainer);
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
    \qmlmethod void QtQuickControls2::Container::moveItem(int from, int to)

    Moves an item \a from one index \a to another.
*/
void QQuickContainer::moveItem(int from, int to)
{
    Q_D(QQuickContainer);
    const int count = d->contentModel->count();
    if (from < 0 || from > count - 1)
        return;
    if (to < 0 || to > count - 1)
        to = count - 1;

    if (from != to)
        d->moveItem(from, to);
}

/*!
    \qmlmethod void QtQuickControls2::Container::removeItem(int index)

    Removes an item at \a index.

    \note The ownership of the item is transferred to the caller.
*/
void QQuickContainer::removeItem(int index)
{
    Q_D(QQuickContainer);
    const int count = d->contentModel->count();
    if (index < 0 || index >= count)
        return;

    QQuickItem *item = itemAt(index);
    if (item)
        d->removeItem(index, item);
}

/*!
    \qmlproperty model QtQuickControls2::Container::contentModel
    \readonly

    This property holds the content model of items.
*/
QVariant QQuickContainer::contentModel() const
{
    Q_D(const QQuickContainer);
    return QVariant::fromValue(d->contentModel);
}

/*!
    \qmlproperty list<Object> QtQuickControls2::Container::contentData
    \default

    This property holds the list of content data.

    \sa Item::data
*/
QQmlListProperty<QObject> QQuickContainer::contentData()
{
    Q_D(QQuickContainer);
    return QQmlListProperty<QObject>(this, d,
                                     QQuickContainerPrivate::contentData_append,
                                     QQuickContainerPrivate::contentData_count,
                                     QQuickContainerPrivate::contentData_at,
                                     QQuickContainerPrivate::contentData_clear);
}

/*!
    \qmlproperty list<Item> QtQuickControls2::Container::contentChildren

    This property holds the list of content children.

    \sa Item::children
*/
QQmlListProperty<QQuickItem> QQuickContainer::contentChildren()
{
    Q_D(QQuickContainer);
    return QQmlListProperty<QQuickItem>(this, d,
                                        QQuickContainerPrivate::contentChildren_append,
                                        QQuickContainerPrivate::contentChildren_count,
                                        QQuickContainerPrivate::contentChildren_at,
                                        QQuickContainerPrivate::contentChildren_clear);
}

void QQuickContainer::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickContainer);
    QQuickControl::itemChange(change, data);
    if (change == QQuickItem::ItemChildAddedChange && isComponentComplete() && data.item != d->background && data.item != d->contentItem) {
        if (!QQuickItemPrivate::get(data.item)->isTransparentForPositioner() && d->contentModel->indexOf(data.item, Q_NULLPTR) == -1)
            addItem(data.item);
    }
}

void QQuickContainer::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickContainer);
    QQuickControl::contentItemChange(newItem, oldItem);
    if (oldItem)
        QQuickItemPrivate::get(oldItem)->removeItemChangeListener(d, QQuickItemPrivate::Children);
    if (newItem)
        QQuickItemPrivate::get(newItem)->addItemChangeListener(d, QQuickItemPrivate::Children);
}

QT_END_NAMESPACE
