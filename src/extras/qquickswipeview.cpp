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

#include <QtQuick/private/qquickitem_p.h>
#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuickControls/private/qquickcontainer_p_p.h>
#include <QtQml/private/qqmlobjectmodel_p.h>

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
    QQuickSwipeViewPrivate() : currentIndex(0), model(Q_NULLPTR) { }

    void _q_updateCurrent();

    // TODO: implement the whole list property
    static void contentData_append(QQmlListProperty<QObject> *, QObject *);

    int currentIndex;
    QQmlObjectModel *model;
};

void QQuickSwipeViewPrivate::_q_updateCurrent()
{
    Q_Q(QQuickSwipeView);
    q->setCurrentIndex(contentItem ? contentItem->property("currentIndex").toInt() : -1);
}

void QQuickSwipeViewPrivate::contentData_append(QQmlListProperty<QObject> *prop, QObject *obj)
{
    QQuickSwipeView *view = static_cast<QQuickSwipeView *>(prop->object);
    if (QQuickItem *item = qobject_cast<QQuickItem *>(obj))
        view->addItem(item);
    else
        QQuickItemPrivate::data_append(prop, obj);
}

QQuickSwipeView::QQuickSwipeView(QQuickItem *parent) :
    QQuickContainer(*(new QQuickSwipeViewPrivate), parent)
{
    Q_D(QQuickSwipeView);
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);

    d->model = new QQmlObjectModel(this);
    connect(d->model, &QQmlObjectModel::countChanged, this, &QQuickSwipeView::countChanged);
}

/*!
    \qmlproperty int QtQuickControls2::SwipeView::count

    TODO
*/
int QQuickSwipeView::count() const
{
    Q_D(const QQuickSwipeView);
    return d->model->count();
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
    \qmlproperty model QtQuickControls2::SwipeView::model
    \readonly

    TODO
*/
QVariant QQuickSwipeView::model() const
{
    Q_D(const QQuickSwipeView);
    return QVariant::fromValue(d->model);
}

QQmlListProperty<QObject> QQuickSwipeView::contentData()
{
    Q_D(QQuickSwipeView);
    // TODO: implement the whole list property
    return QQmlListProperty<QObject>(this, d,
                                     QQuickSwipeViewPrivate::contentData_append,
                                     QQuickItemPrivate::data_count,
                                     QQuickItemPrivate::data_at,
                                     QQuickItemPrivate::data_clear);
}

/*!
    \qmlmethod Item QtQuickControls2::SwipeView::itemAt(int index)

    TODO
*/
QQuickItem *QQuickSwipeView::itemAt(int index) const
{
    Q_D(const QQuickSwipeView);
    return qobject_cast<QQuickItem *>(d->model->get(index));
}

/*!
    \qmlmethod void QtQuickControls2::SwipeView::addItem(Item item)

    TODO
*/
void QQuickSwipeView::addItem(QQuickItem *item)
{
    Q_D(QQuickSwipeView);
    d->model->append(item);
}

/*!
    \qmlmethod void QtQuickControls2::SwipeView::insertItem(int index, Item item)

    TODO
*/
void QQuickSwipeView::insertItem(int index, QQuickItem *item)
{
    Q_D(QQuickSwipeView);
    d->model->insert(index, item);
}

/*!
    \qmlmethod void QtQuickControls2::SwipeView::moveItem(int from, int to)

    TODO
*/
void QQuickSwipeView::moveItem(int from, int to)
{
    Q_D(QQuickSwipeView);
    d->model->move(from, to);
}

/*!
    \qmlmethod void QtQuickControls2::SwipeView::removeItem(int index)

    TODO
*/
void QQuickSwipeView::removeItem(int index)
{
    Q_D(QQuickSwipeView);
    d->model->remove(index);
}

void QQuickSwipeView::componentComplete()
{
    Q_D(QQuickSwipeView);
    QQuickContainer::componentComplete();
    d->_q_updateCurrent();
}

void QQuickSwipeView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickSwipeView);
    QQuickContainer::geometryChanged(newGeometry, oldGeometry);

    const int count = d->model->count();
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = qobject_cast<QQuickItem *>(d->model->get(i));
        if (item) {
            QQuickItemPrivate *p = QQuickItemPrivate::get(item);
            if (!p->widthValid) {
                item->setWidth(newGeometry.width());
                p->widthValid = false;
            }
            if (!p->heightValid) {
                item->setHeight(newGeometry.height());
                p->widthValid = false;
            }
        }
    }
}

void QQuickSwipeView::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    if (oldItem)
        disconnect(oldItem, SIGNAL(currentIndexChanged()), this, SLOT(_q_updateCurrent()));
    if (newItem)
        connect(newItem, SIGNAL(currentIndexChanged()), this, SLOT(_q_updateCurrent()));
}

QT_END_NAMESPACE

#include "moc_qquickswipeview_p.cpp"
