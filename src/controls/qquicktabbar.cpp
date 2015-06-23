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

#include "qquicktabbar_p.h"
#include "qquickcontainer_p_p.h"
#include "qquickexclusivegroup_p.h"

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickitemchangelistener_p.h>
#include <QtQml/private/qqmlobjectmodel_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype TabBar
    \inherits Container
    \instantiates QQuickTabBar
    \inqmlmodule QtQuick.Controls
    \ingroup tabs
    \brief A tab bar control.

    TODO
*/

class QQuickTabBarPrivate : public QQuickContainerPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickTabBar)

public:
    QQuickTabBarPrivate() : currentIndex(0), contentModel(Q_NULLPTR), group(Q_NULLPTR) { }

    void updateLayout();
    void updateCurrent();

    void itemChildAdded(QQuickItem *item, QQuickItem *child) Q_DECL_OVERRIDE;
    void itemSiblingOrderChanged(QQuickItem *item) Q_DECL_OVERRIDE;
    void itemParentChanged(QQuickItem *item, QQuickItem *parent) Q_DECL_OVERRIDE;
    void itemDestroyed(QQuickItem *item) Q_DECL_OVERRIDE;

    static void contentData_append(QQmlListProperty<QObject> *prop, QObject *obj);
    static int contentData_count(QQmlListProperty<QObject> *prop);
    static QObject *contentData_at(QQmlListProperty<QObject> *prop, int index);
    static void contentData_clear(QQmlListProperty<QObject> *prop);

    static void contentChildren_append(QQmlListProperty<QQuickItem> *prop, QQuickItem *obj);
    static int contentChildren_count(QQmlListProperty<QQuickItem> *prop);
    static QQuickItem *contentChildren_at(QQmlListProperty<QQuickItem> *prop, int index);
    static void contentChildren_clear(QQmlListProperty<QQuickItem> *prop);

    int currentIndex;
    QObjectList contentData;
    QQmlObjectModel *contentModel;
    QQuickExclusiveGroup *group;
};

void QQuickTabBarPrivate::updateLayout()
{
    Q_Q(QQuickTabBar);
    const int count = contentModel->count();
    if (count > 0 && contentItem) {
        const qreal spacing = contentItem->property("spacing").toReal();
        const qreal itemWidth = (contentItem->width() - qMax(0, count - 1) * spacing) / count;

        for (int i = 0; i < count; ++i) {
            QQuickItem *item = q->itemAt(i);
            if (item) {
                QQuickItemPrivate *p = QQuickItemPrivate::get(item);
                if (!p->widthValid) {
                    item->setWidth(itemWidth);
                    p->widthValid = false;
                }
            }
        }
    }
}

void QQuickTabBarPrivate::updateCurrent()
{
    Q_Q(QQuickTabBar);
    q->setCurrentIndex(contentModel->indexOf(group->current(), Q_NULLPTR));
}

void QQuickTabBarPrivate::itemChildAdded(QQuickItem *, QQuickItem *child)
{
    // add dynamically reparented items (eg. by a Repeater)
    Q_Q(QQuickTabBar);
    if (!QQuickItemPrivate::get(child)->isTransparentForPositioner() && contentModel->indexOf(child, Q_NULLPTR) == -1)
        q->addItem(child);
}

void QQuickTabBarPrivate::itemParentChanged(QQuickItem *item, QQuickItem *parent)
{
    // remove dynamically unparented items (eg. by a Repeater)
    Q_Q(QQuickTabBar);
    if (!parent)
        q->removeItem(contentModel->indexOf(item, Q_NULLPTR));
}

void QQuickTabBarPrivate::itemSiblingOrderChanged(QQuickItem *)
{
    // reorder the restacked items (eg. by a Repeater)
    Q_Q(QQuickTabBar);
    QList<QQuickItem *> siblings = contentItem->childItems();
    for (int i = 0; i < siblings.count(); ++i) {
        QQuickItem* sibling = siblings.at(i);
        int index = contentModel->indexOf(sibling, Q_NULLPTR);
        q->moveItem(index, i);
    }
}

void QQuickTabBarPrivate::itemDestroyed(QQuickItem *item)
{
    Q_Q(QQuickTabBar);
    int index = contentModel->indexOf(item, Q_NULLPTR);
    if (index != -1)
        q->removeItem(index);
}

void QQuickTabBarPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickTabBarPrivate *p = static_cast<QQuickTabBarPrivate *>(prop->data);
    QQuickTabBar *bar = static_cast<QQuickTabBar *>(prop->object);
    QQuickItem *item = qobject_cast<QQuickItem *>(obj);
    if (item) {
        if (QQuickItemPrivate::get(item)->isTransparentForPositioner()) {
            QQuickItemPrivate::get(item)->addItemChangeListener(p, QQuickItemPrivate::SiblingOrder);
            item->setParentItem(p->contentItem);
        } else if (p->contentModel->indexOf(item, Q_NULLPTR) == -1) {
            bar->addItem(item);
        }
    } else {
        p->contentData.append(obj);
    }
}

int QQuickTabBarPrivate::contentData_count(QQmlListProperty<QObject> *prop)
{
    QQuickTabBarPrivate *p = static_cast<QQuickTabBarPrivate *>(prop->data);
    return p->contentData.count();
}

QObject *QQuickTabBarPrivate::contentData_at(QQmlListProperty<QObject> *prop, int index)
{
    QQuickTabBarPrivate *p = static_cast<QQuickTabBarPrivate *>(prop->data);
    return p->contentData.value(index);
}

void QQuickTabBarPrivate::contentData_clear(QQmlListProperty<QObject> *prop)
{
    QQuickTabBarPrivate *p = static_cast<QQuickTabBarPrivate *>(prop->data);
    p->contentData.clear();
}

void QQuickTabBarPrivate::contentChildren_append(QQmlListProperty<QQuickItem> *prop, QQuickItem *item)
{
    QQuickTabBar *bar = static_cast<QQuickTabBar *>(prop->object);
    bar->addItem(item);
}

int QQuickTabBarPrivate::contentChildren_count(QQmlListProperty<QQuickItem> *prop)
{
    QQuickTabBarPrivate *p = static_cast<QQuickTabBarPrivate *>(prop->data);
    return p->contentModel->count();
}

QQuickItem *QQuickTabBarPrivate::contentChildren_at(QQmlListProperty<QQuickItem> *prop, int index)
{
    QQuickTabBar *bar = static_cast<QQuickTabBar *>(prop->object);
    return bar->itemAt(index);
}

void QQuickTabBarPrivate::contentChildren_clear(QQmlListProperty<QQuickItem> *prop)
{
    QQuickTabBarPrivate *p = static_cast<QQuickTabBarPrivate *>(prop->data);
    p->contentModel->clear();
}

QQuickTabBar::QQuickTabBar(QQuickItem *parent) :
    QQuickContainer(*(new QQuickTabBarPrivate), parent)
{
    Q_D(QQuickTabBar);
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);

    d->contentModel = new QQmlObjectModel(this);
    connect(d->contentModel, &QQmlObjectModel::countChanged, this, &QQuickTabBar::countChanged);
    connect(d->contentModel, &QQmlObjectModel::modelUpdated, this, &QQuickTabBar::polish);

    d->group = new QQuickExclusiveGroup(this);
    connect(d->group, &QQuickExclusiveGroup::currentChanged, this, &QQuickTabBar::currentItemChanged);
    QObjectPrivate::connect(d->group, &QQuickExclusiveGroup::currentChanged, d, &QQuickTabBarPrivate::updateCurrent);
}

QQuickTabBar::~QQuickTabBar()
{
    Q_D(QQuickTabBar);
    delete d->contentItem;
    const int count = d->contentModel->count();
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = itemAt(i);
        if (item) {
            QQuickItemPrivate::get(item)->removeItemChangeListener(d, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
            delete item;
        }
    }
    delete d->contentModel;
}

/*!
    \qmlproperty int QtQuickControls2::TabBar::count
    \readonly

    TODO
*/
int QQuickTabBar::count() const
{
    Q_D(const QQuickTabBar);
    return d->contentModel->count();
}

/*!
    \qmlproperty int QtQuickControls2::TabBar::currentIndex

    TODO
*/
int QQuickTabBar::currentIndex() const
{
    Q_D(const QQuickTabBar);
    return d->currentIndex;
}

void QQuickTabBar::setCurrentIndex(int index)
{
    Q_D(QQuickTabBar);
    if (d->currentIndex != index) {
        d->currentIndex = index;
        emit currentIndexChanged();
        if (isComponentComplete())
            d->group->setCurrent(d->contentModel->get(index));
    }
}

/*!
    \qmlproperty Item QtQuickControls2::TabBar::currentItem

    TODO
*/
QQuickItem *QQuickTabBar::currentItem() const
{
    Q_D(const QQuickTabBar);
    return qobject_cast<QQuickItem *>(d->group->current());
}

/*!
    \qmlproperty model QtQuickControls2::TabBar::contentModel
    \readonly

    TODO
*/
QVariant QQuickTabBar::contentModel() const
{
    Q_D(const QQuickTabBar);
    return QVariant::fromValue(d->contentModel);
}

QQmlListProperty<QObject> QQuickTabBar::contentData()
{
    Q_D(QQuickTabBar);
    return QQmlListProperty<QObject>(this, d,
                                     QQuickTabBarPrivate::contentData_append,
                                     QQuickTabBarPrivate::contentData_count,
                                     QQuickTabBarPrivate::contentData_at,
                                     QQuickTabBarPrivate::contentData_clear);
}

QQmlListProperty<QQuickItem> QQuickTabBar::contentChildren()
{
    Q_D(QQuickTabBar);
    return QQmlListProperty<QQuickItem>(this, d,
                                        QQuickTabBarPrivate::contentChildren_append,
                                        QQuickTabBarPrivate::contentChildren_count,
                                        QQuickTabBarPrivate::contentChildren_at,
                                        QQuickTabBarPrivate::contentChildren_clear);
}

/*!
    \qmlmethod Item QtQuickControls2::TabBar::itemAt(int index)

    TODO
*/
QQuickItem *QQuickTabBar::itemAt(int index) const
{
    Q_D(const QQuickTabBar);
    return qobject_cast<QQuickItem *>(d->contentModel->get(index));
}

/*!
    \qmlmethod void QtQuickControls2::TabBar::addItem(Item item)

    TODO
*/
void QQuickTabBar::addItem(QQuickItem *item)
{
    Q_D(QQuickTabBar);
    insertItem(d->contentModel->count(), item);
}

/*!
    \qmlmethod void QtQuickControls2::TabBar::insertItem(int index, Item item)

    TODO
*/
void QQuickTabBar::insertItem(int index, QQuickItem *item)
{
    Q_D(QQuickTabBar);
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
            moveItem(oldIndex, index);
    } else {
        QQuickItemPrivate::get(item)->addItemChangeListener(d, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
        d->contentData.append(item);
        d->contentModel->insert(index, item);
        d->group->addCheckable(item);

        if (count == 0 || d->currentIndex == index)
            d->group->setCurrent(item);
        else
            d->updateCurrent();
    }
}

/*!
    \qmlmethod void QtQuickControls2::TabBar::moveItem(int from, int to)

    TODO
*/
void QQuickTabBar::moveItem(int from, int to)
{
    Q_D(QQuickTabBar);
    const int count = d->contentModel->count();
    if (from < 0 || from > count - 1)
        return;
    if (to < 0 || to > count - 1)
        to = count - 1;

    if (from != to) {
        d->contentModel->move(from, to);
        d->updateCurrent();
    }
}

/*!
    \qmlmethod void QtQuickControls2::TabBar::removeItem(int index)

    TODO
*/
void QQuickTabBar::removeItem(int index)
{
    Q_D(QQuickTabBar);
    const int count = d->contentModel->count();
    if (index < 0 || index >= count)
        return;

    QQuickItem *item = itemAt(index);
    if (item) {
        bool currentChanged = false;
        if (index == d->currentIndex) {
            d->group->setCurrent(d->contentModel->get(index - 1));
        } else if (index < d->currentIndex) {
            --d->currentIndex;
            currentChanged = true;
        }

        QQuickItemPrivate::get(item)->removeItemChangeListener(d, QQuickItemPrivate::Destroyed | QQuickItemPrivate::Parent);
        d->group->removeCheckable(item);
        d->contentData.removeOne(item);
        d->contentModel->remove(index);

        if (currentChanged)
            emit currentIndexChanged();
    }
}

void QQuickTabBar::updatePolish()
{
    Q_D(QQuickTabBar);
    QQuickContainer::updatePolish();
    d->updateLayout();
}

void QQuickTabBar::componentComplete()
{
    Q_D(QQuickTabBar);
    QQuickContainer::componentComplete();
    d->updateCurrent();
    d->updateLayout();
}

void QQuickTabBar::itemChange(ItemChange change, const ItemChangeData &data)
{
    Q_D(QQuickTabBar);
    QQuickContainer::itemChange(change, data);
    if (change == QQuickItem::ItemChildAddedChange && isComponentComplete() && data.item != d->background && data.item != d->contentItem) {
        if (!QQuickItemPrivate::get(data.item)->isTransparentForPositioner() && d->contentModel->indexOf(data.item, Q_NULLPTR) == -1)
            addItem(data.item);
    }
}

void QQuickTabBar::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickTabBar);
    QQuickContainer::contentItemChange(newItem, oldItem);
    if (oldItem)
        QQuickItemPrivate::get(oldItem)->removeItemChangeListener(d, QQuickItemPrivate::Children);
    if (newItem)
        QQuickItemPrivate::get(newItem)->addItemChangeListener(d, QQuickItemPrivate::Children);
}

void QQuickTabBar::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTabBar);
    QQuickContainer::geometryChanged(newGeometry, oldGeometry);
    d->updateLayout();
}

QT_END_NAMESPACE
