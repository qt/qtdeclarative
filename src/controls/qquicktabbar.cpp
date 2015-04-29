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

class QQuickTabBarPrivate : public QQuickContainerPrivate
{
    Q_DECLARE_PUBLIC(QQuickTabBar)

public:
    QQuickTabBarPrivate() : currentIndex(0), model(Q_NULLPTR), group(Q_NULLPTR) { }

    void updateLayout();
    void updateCurrent();

    // TODO: implement the whole list property
    static void contentData_append(QQmlListProperty<QObject> *prop, QObject *obj);

    int currentIndex;
    QQmlObjectModel *model;
    QQuickExclusiveGroup *group;
};

void QQuickTabBarPrivate::updateLayout()
{
    const int count = model->count();
    if (count > 0 && contentItem) {
        const qreal spacing = contentItem->property("spacing").toReal();
        const qreal itemWidth = (contentItem->width() - qMax(0, count - 1) * spacing) / count;

        for (int i = 0; i < count; ++i) {
            QQuickItem *item = qobject_cast<QQuickItem *>(model->get(i));
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
    q->setCurrentIndex(model->indexOf(group->current(), Q_NULLPTR));
}

void QQuickTabBarPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickTabBar *bar = static_cast<QQuickTabBar *>(prop->object);
    QQuickItem *item = qobject_cast<QQuickItem *>(obj);
    if (item)
        bar->addItem(item);
    else
        QQuickItemPrivate::data_append(prop, obj);
}

QQuickTabBar::QQuickTabBar(QQuickItem *parent) :
    QQuickContainer(*(new QQuickTabBarPrivate), parent)
{
    Q_D(QQuickTabBar);
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);

    d->model = new QQmlObjectModel(this);
    connect(d->model, &QQmlObjectModel::countChanged, this, &QQuickTabBar::countChanged);
    QObjectPrivate::connect(d->model, &QQmlObjectModel::countChanged, d, &QQuickTabBarPrivate::updateLayout);

    d->group = new QQuickExclusiveGroup(this);
    connect(d->group, &QQuickExclusiveGroup::currentChanged, this, &QQuickTabBar::currentItemChanged);
    QObjectPrivate::connect(d->group, &QQuickExclusiveGroup::currentChanged, d, &QQuickTabBarPrivate::updateCurrent);
}

/*!
    \qmlproperty int QtQuickControls2::TabBar::count
    \readonly

    TODO
*/
int QQuickTabBar::count() const
{
    Q_D(const QQuickTabBar);
    return d->model->count();
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
    \qmlproperty model QtQuickControls2::TabBar::model
    \readonly

    TODO
*/
QVariant QQuickTabBar::model() const
{
    Q_D(const QQuickTabBar);
    return QVariant::fromValue(d->model);
}

QQmlListProperty<QObject> QQuickTabBar::contentData()
{
    Q_D(QQuickTabBar);
    // TODO: implement the whole list property
    return QQmlListProperty<QObject>(this, d,
                                     QQuickTabBarPrivate::contentData_append,
                                     QQuickItemPrivate::data_count,
                                     QQuickItemPrivate::data_at,
                                     QQuickItemPrivate::data_clear);
}

/*!
    \qmlmethod Item QtQuickControls2::TabBar::itemAt(int index)

    TODO
*/
QQuickItem *QQuickTabBar::itemAt(int index) const
{
    Q_D(const QQuickTabBar);
    return qobject_cast<QQuickItem *>(d->model->get(index));
}

/*!
    \qmlmethod void QtQuickControls2::TabBar::addItem(Item item)

    TODO
*/
void QQuickTabBar::addItem(QQuickItem *item)
{
    Q_D(QQuickTabBar);
    insertItem(d->model->count(), item);
}

/*!
    \qmlmethod void QtQuickControls2::TabBar::insertItem(int index, Item item)

    TODO
*/
void QQuickTabBar::insertItem(int index, QQuickItem *item)
{
    Q_D(QQuickTabBar);
    d->model->insert(index, item);
    d->group->addCheckable(item);
    if (d->currentIndex == index)
        d->group->setCurrent(item);
}

/*!
    \qmlmethod void QtQuickControls2::TabBar::moveItem(int from, int to)

    TODO
*/
void QQuickTabBar::moveItem(int from, int to)
{
    Q_D(QQuickTabBar);
    d->model->move(from, to);
}

/*!
    \qmlmethod void QtQuickControls2::TabBar::removeItem(int index)

    TODO
*/
void QQuickTabBar::removeItem(int index)
{
    Q_D(QQuickTabBar);
    d->group->removeCheckable(d->model->get(index));
    d->model->remove(index);
}

void QQuickTabBar::componentComplete()
{
    Q_D(QQuickTabBar);
    QQuickContainer::componentComplete();
    d->updateCurrent();
}

void QQuickTabBar::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTabBar);
    QQuickContainer::geometryChanged(newGeometry, oldGeometry);
    d->updateLayout();
}

QT_END_NAMESPACE
