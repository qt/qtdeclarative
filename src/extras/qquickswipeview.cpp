/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Extras module of the Qt Toolkit.
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

#include "qquickswipeview_p.h"

#include <QtQuickControls/private/qquickcontainer_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SwipeView
    \inherits Container
    \instantiates QQuickSwipeView
    \inqmlmodule QtQuick.Extras
    \ingroup navigation
    \brief A swipe view control.

    TODO
*/

class QQuickSwipeViewPrivate : public QQuickContainerPrivate
{
    Q_DECLARE_PUBLIC(QQuickSwipeView)

public:
    QQuickSwipeViewPrivate() : currentIndex(-1), updatingCurrent(false) { }

    void resizeItem(QQuickItem *item);
    void resizeItems();
    void _q_updateCurrent();

    void insertItem(int index, QQuickItem *item) Q_DECL_OVERRIDE;
    void moveItem(int from, int to) Q_DECL_OVERRIDE;
    void removeItem(int index, QQuickItem *item) Q_DECL_OVERRIDE;

    int currentIndex;
    bool updatingCurrent;
};

void QQuickSwipeViewPrivate::resizeItems()
{
    Q_Q(QQuickSwipeView);
    const int count = q->count();
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = itemAt(i);
        if (item)
            item->setSize(QSizeF(contentItem->width(), contentItem->height()));
    }
}

void QQuickSwipeViewPrivate::_q_updateCurrent()
{
    Q_Q(QQuickSwipeView);
    if (!updatingCurrent)
        q->setCurrentIndex(contentItem ? contentItem->property("currentIndex").toInt() : -1);
}

void QQuickSwipeViewPrivate::insertItem(int index, QQuickItem *item)
{
    Q_Q(QQuickSwipeView);
    if (q->isComponentComplete())
        item->setSize(QSizeF(contentItem->width(), contentItem->height()));

    QQuickContainerPrivate::insertItem(index, item);

    if (contentModel->count() == 1 && currentIndex == -1)
        q->setCurrentIndex(index);
}

void QQuickSwipeViewPrivate::moveItem(int from, int to)
{
    Q_Q(QQuickSwipeView);
    QQuickContainerPrivate::moveItem(from, to);

    updatingCurrent = true;
    if (from == currentIndex)
        q->setCurrentIndex(to);
    else if (from < currentIndex && to >= currentIndex)
        q->setCurrentIndex(currentIndex - 1);
    else if (from > currentIndex && to <= currentIndex)
        q->setCurrentIndex(currentIndex + 1);
    updatingCurrent = false;
}

void QQuickSwipeViewPrivate::removeItem(int index, QQuickItem *item)
{
    Q_Q(QQuickSwipeView);
    bool currentChanged = false;
    if (index == currentIndex) {
        q->setCurrentIndex(currentIndex - 1);
    } else if (index < currentIndex) {
        --currentIndex;
        currentChanged = true;
    }

    QQuickContainerPrivate::removeItem(index, item);

    if (currentChanged)
        emit q->currentIndexChanged();
}

QQuickSwipeView::QQuickSwipeView(QQuickItem *parent) :
    QQuickContainer(*(new QQuickSwipeViewPrivate), parent)
{
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);
}

/*!
    \qmlproperty int QtQuickControls2::SwipeView::currentIndex

    TODO
*/
int QQuickSwipeView::currentIndex() const
{
    Q_D(const QQuickSwipeView);
    return d->currentIndex;
}

void QQuickSwipeView::setCurrentIndex(int index)
{
    Q_D(QQuickSwipeView);
    if (d->currentIndex != index) {
        d->currentIndex = index;
        emit currentIndexChanged();
    }
}

/*!
    \qmlproperty Item QtQuickControls2::SwipeView::currentItem

    TODO
*/
QQuickItem *QQuickSwipeView::currentItem() const
{
    Q_D(const QQuickSwipeView);
    return itemAt(d->currentIndex);
}

void QQuickSwipeView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickSwipeView);
    QQuickContainer::geometryChanged(newGeometry, oldGeometry);
    d->resizeItems();
}

void QQuickSwipeView::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    QQuickContainer::contentItemChange(newItem, oldItem);
    if (oldItem)
        disconnect(oldItem, SIGNAL(currentIndexChanged()), this, SLOT(_q_updateCurrent()));
    if (newItem)
        connect(newItem, SIGNAL(currentIndexChanged()), this, SLOT(_q_updateCurrent()));
}

QT_END_NAMESPACE

#include "moc_qquickswipeview_p.cpp"
