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

#include "qquicktabbar_p.h"
#include "qquicktabbutton_p.h"
#include "qquickcontainer_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmltype TabBar
    \inherits Container
    \instantiates QQuickTabBar
    \inqmlmodule Qt.labs.controls
    \ingroup qtlabscontrols-navigation
    \ingroup qtlabscontrols-containers
    \brief A tab bar control.

    TabBar provides a tab-based navigation model.

    \image qtlabscontrols-tabbar-wireframe.png

    TabBar is populated with TabButton controls, and can be used together with
    any layout or container control that provides \c currentIndex -property,
    such as \l StackLayout or \l SwipeView

    \snippet qtlabscontrols-tabbar.qml 1

    \labs

    \sa TabButton, {Customizing TabBar}, {Navigation Controls}, {Container Controls}
*/

class QQuickTabBarPrivate : public QQuickContainerPrivate
{
    Q_DECLARE_PUBLIC(QQuickTabBar)

public:
    void updateCurrentItem();
    void updateCurrentIndex();
    void updateLayout();
};

void QQuickTabBarPrivate::updateCurrentItem()
{
    QQuickTabButton *button = qobject_cast<QQuickTabButton *>(contentModel->get(currentIndex));
    if (button)
        button->setChecked(true);
}

void QQuickTabBarPrivate::updateCurrentIndex()
{
    Q_Q(QQuickTabBar);
    QQuickTabButton *button = qobject_cast<QQuickTabButton *>(q->sender());
    if (button && button->isChecked())
        q->setCurrentIndex(contentModel->indexOf(button, Q_NULLPTR));
}

void QQuickTabBarPrivate::updateLayout()
{
    Q_Q(QQuickTabBar);
    const int count = contentModel->count();
    if (count > 0 && contentItem) {
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

QQuickTabBar::QQuickTabBar(QQuickItem *parent) :
    QQuickContainer(*(new QQuickTabBarPrivate), parent)
{
    Q_D(QQuickTabBar);
    setFlag(ItemIsFocusScope);
    QObjectPrivate::connect(this, &QQuickTabBar::currentIndexChanged, d, &QQuickTabBarPrivate::updateCurrentItem);
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
    d->updateCurrentItem();
    d->updateLayout();
}

void QQuickTabBar::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTabBar);
    QQuickContainer::geometryChanged(newGeometry, oldGeometry);
    d->updateLayout();
}

bool QQuickTabBar::isContent(QQuickItem *item) const
{
    return qobject_cast<QQuickTabButton *>(item);
}

void QQuickTabBar::itemAdded(int index, QQuickItem *item)
{
    Q_D(QQuickTabBar);
    Q_UNUSED(index);
    if (QQuickTabButton *button = qobject_cast<QQuickTabButton *>(item))
        QObjectPrivate::connect(button, &QQuickTabButton::checkedChanged, d, &QQuickTabBarPrivate::updateCurrentIndex);
    if (isComponentComplete())
        polish();
}

void QQuickTabBar::itemRemoved(int index, QQuickItem *item)
{
    Q_D(QQuickTabBar);
    Q_UNUSED(index);
    if (QQuickTabButton *button = qobject_cast<QQuickTabButton *>(item))
        QObjectPrivate::disconnect(button, &QQuickTabButton::checkedChanged, d, &QQuickTabBarPrivate::updateCurrentIndex);
    if (isComponentComplete())
        polish();
}

#ifndef QT_NO_ACCESSIBILITY
QAccessible::Role QQuickTabBar::accessibleRole() const
{
    return QAccessible::PageTabList;
}
#endif

QT_END_NAMESPACE
