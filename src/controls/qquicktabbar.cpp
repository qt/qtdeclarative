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
    QQuickTabBarPrivate() : currentIndex(0), group(Q_NULLPTR) { }

    void updateLayout();
    void updateCurrent();

    void insertItem(int index, QQuickItem *item) Q_DECL_OVERRIDE;
    void moveItem(int from, int to) Q_DECL_OVERRIDE;
    void removeItem(int index, QQuickItem *item) Q_DECL_OVERRIDE;

    int currentIndex;
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

void QQuickTabBarPrivate::insertItem(int index, QQuickItem *item)
{
    QQuickContainerPrivate::insertItem(index, item);

    group->addCheckable(item);
    if (contentModel->count() == 1 || currentIndex == index)
        group->setCurrent(item);
    else
        updateCurrent();
}

void QQuickTabBarPrivate::moveItem(int from, int to)
{
    QQuickContainerPrivate::moveItem(from, to);

    updateCurrent();
}

void QQuickTabBarPrivate::removeItem(int index, QQuickItem *item)
{
    Q_Q(QQuickTabBar);
    bool currentChanged = false;
    if (index == currentIndex) {
        group->setCurrent(contentModel->get(index - 1));
    } else if (index < currentIndex) {
        --currentIndex;
        currentChanged = true;
    }
    group->removeCheckable(item);

    QQuickContainerPrivate::removeItem(index, item);

    if (currentChanged)
        emit q->currentIndexChanged();
}

QQuickTabBar::QQuickTabBar(QQuickItem *parent) :
    QQuickContainer(*(new QQuickTabBarPrivate), parent)
{
    Q_D(QQuickTabBar);
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);

    d->group = new QQuickExclusiveGroup(this);
    connect(d->group, &QQuickExclusiveGroup::currentChanged, this, &QQuickTabBar::currentItemChanged);
    QObjectPrivate::connect(d->group, &QQuickExclusiveGroup::currentChanged, d, &QQuickTabBarPrivate::updateCurrent);
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

void QQuickTabBar::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTabBar);
    QQuickContainer::geometryChanged(newGeometry, oldGeometry);
    d->updateLayout();
}

QT_END_NAMESPACE
