/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qquickswipeview_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQuickTemplates2/private/qquickcontainer_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SwipeView
    \inherits Container
    \instantiates QQuickSwipeView
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-navigation
    \ingroup qtquickcontrols2-containers
    \brief Enables the user to navigate pages by swiping sideways.

    SwipeView provides a swipe-based navigation model.

    \image qtquickcontrols2-swipeview-wireframe.png

    SwipeView is populated with a set of pages. One page is visible at a time.
    The user can navigate between the pages by swiping sideways. Notice that
    SwipeView itself is entirely non-visual. It is recommended to combine it
    with PageIndicator, to give the user a visual clue that there are multiple
    pages.

    \snippet qtquickcontrols2-swipeview-indicator.qml 1

    As shown above, SwipeView is typically populated with a static set of
    pages that are defined inline as children of the view. It is also possible
    to \l {Container::addItem()}{add}, \l {Container::insertItem()}{insert},
    \l {Container::moveItem()}{move}, and \l {Container::removeItem()}{remove}
    pages dynamically at run time.

    \note SwipeView takes over the geometry management of items added to the
          view. Using anchors on the items is not supported, and any \c width
          or \c height assignment will be overridden by the view. Notice that
          this only applies to the root of the item. Specifying width and height,
          or using anchors for its children works as expected.

    \sa TabBar, PageIndicator, {Customizing SwipeView}, {Navigation Controls}, {Container Controls}
*/

class QQuickSwipeViewPrivate : public QQuickContainerPrivate
{
    Q_DECLARE_PUBLIC(QQuickSwipeView)

public:
    void resizeItem(QQuickItem *item);
    void resizeItems();

    static QQuickSwipeViewPrivate *get(QQuickSwipeView *view);
};

void QQuickSwipeViewPrivate::resizeItems()
{
    Q_Q(QQuickSwipeView);
    const int count = q->count();
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = itemAt(i);
        if (item) {
            QQuickAnchors *anchors = QQuickItemPrivate::get(item)->_anchors;
            // TODO: expose QQuickAnchorLine so we can test for other conflicting anchors
            if (anchors && (anchors->fill() || anchors->centerIn()) && !item->property("_q_QQuickSwipeView_warned").toBool()) {
                qmlInfo(item) << "SwipeView has detected conflicting anchors. Unable to layout the item.";
                item->setProperty("_q_QQuickSwipeView_warned", true);
            }

            item->setSize(QSizeF(contentItem->width(), contentItem->height()));
        }
    }
}

QQuickSwipeViewPrivate *QQuickSwipeViewPrivate::get(QQuickSwipeView *view)
{
    return view->d_func();
}

QQuickSwipeView::QQuickSwipeView(QQuickItem *parent) :
    QQuickContainer(*(new QQuickSwipeViewPrivate), parent)
{
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);
}

QQuickSwipeViewAttached *QQuickSwipeView::qmlAttachedProperties(QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        qWarning() << "SwipeView: attached properties must be accessed from within a child item";
        return nullptr;
    }

    return new QQuickSwipeViewAttached(item);
}

void QQuickSwipeView::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickSwipeView);
    QQuickContainer::geometryChanged(newGeometry, oldGeometry);
    d->resizeItems();
}

void QQuickSwipeView::itemAdded(int, QQuickItem *item)
{
    Q_D(QQuickSwipeView);
    QQuickItemPrivate::get(item)->setCulled(true); // QTBUG-51078, QTBUG-51669
    if (isComponentComplete())
        item->setSize(QSizeF(d->contentItem->width(), d->contentItem->height()));
}

/*!
    \qmlattachedproperty int QtQuick.Controls::SwipeView::index
    \readonly

    This attached property holds the index of each child item in the SwipeView.

    It is attached to each child item of the SwipeView.
*/

/*!
    \qmlattachedproperty bool QtQuick.Controls::SwipeView::isCurrentItem
    \readonly

    This attached property is \c true if this child is the current item.

    It is attached to each child item of the SwipeView.
*/

/*!
    \qmlattachedproperty SwipeView QtQuick.Controls::SwipeView::view
    \readonly

    This attached property holds the view that manages this child item.

    It is attached to each child item of the SwipeView.
*/

class QQuickSwipeViewAttachedPrivate : public QObjectPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickSwipeViewAttached)
public:
    QQuickSwipeViewAttachedPrivate(QQuickItem *item) :
        item(item),
        swipeView(nullptr),
        index(-1),
        isCurrent(false)
    {
    }

    ~QQuickSwipeViewAttachedPrivate() {
    }

    void updateView(QQuickItem *parent);

    void itemChildAdded(QQuickItem *, QQuickItem *) override;
    void itemChildRemoved(QQuickItem *, QQuickItem *) override;
    void itemParentChanged(QQuickItem *, QQuickItem *) override;
    void itemDestroyed(QQuickItem *) override;

    void updateIndex();
    void updateIsCurrent();

    void setView(QQuickSwipeView *view);
    void setIndex(int i);
    void setIsCurrent(bool current);

    QQuickItem *item;
    QQuickSwipeView *swipeView;
    int index;
    // Better to store this so that we don't need to lump its calculation
    // together with index's calculation, as it would otherwise need to know
    // the old index to know if it should emit the change signal.
    bool isCurrent;
};

void QQuickSwipeViewAttachedPrivate::updateIndex()
{
    setIndex(swipeView ? QQuickSwipeViewPrivate::get(swipeView)->contentModel->indexOf(item, nullptr) : -1);
}

void QQuickSwipeViewAttachedPrivate::updateIsCurrent()
{
    setIsCurrent(swipeView ? swipeView->currentIndex() == index : false);
}

void QQuickSwipeViewAttachedPrivate::setView(QQuickSwipeView *view)
{
    if (view == swipeView)
        return;

    if (swipeView) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(swipeView);
        p->removeItemChangeListener(this, QQuickItemPrivate::Children);

        disconnect(swipeView, &QQuickSwipeView::currentIndexChanged,
            this, &QQuickSwipeViewAttachedPrivate::updateIsCurrent);
        disconnect(swipeView, &QQuickSwipeView::contentChildrenChanged,
            this, &QQuickSwipeViewAttachedPrivate::updateIndex);
    }

    swipeView = view;

    if (swipeView) {
        QQuickItemPrivate *p = QQuickItemPrivate::get(swipeView);
        p->addItemChangeListener(this, QQuickItemPrivate::Children);

        connect(swipeView, &QQuickSwipeView::currentIndexChanged,
            this, &QQuickSwipeViewAttachedPrivate::updateIsCurrent);
        connect(swipeView, &QQuickSwipeView::contentChildrenChanged,
            this, &QQuickSwipeViewAttachedPrivate::updateIndex);
    }

    Q_Q(QQuickSwipeViewAttached);
    emit q->viewChanged();

    updateIndex();
    updateIsCurrent();
}

void QQuickSwipeViewAttachedPrivate::setIsCurrent(bool current)
{
    if (current == isCurrent)
        return;

    isCurrent = current;
    Q_Q(QQuickSwipeViewAttached);
    emit q->isCurrentItemChanged();
}

void QQuickSwipeViewAttachedPrivate::setIndex(int i)
{
    if (i == index)
        return;

    index = i;
    Q_Q(QQuickSwipeViewAttached);
    emit q->indexChanged();
}

void QQuickSwipeViewAttachedPrivate::updateView(QQuickItem *parent)
{
    // parent can be, e.g.:
    // - The contentItem of a ListView (typically the case)
    // - A non-visual or weird type like TestCase, when child items are created from components
    //   wherein the attached properties are used
    // - null, when the item was removed with removeItem()
    QQuickSwipeView *view = nullptr;
    if (parent) {
        view = qobject_cast<QQuickSwipeView*>(parent);
        if (!view) {
            if (parent->parentItem() && parent->parentItem()->property("contentItem").isValid()) {
                // The parent is the contentItem of some kind of view.
                view = qobject_cast<QQuickSwipeView*>(parent->parentItem()->parentItem());
            }
        }
    }

    setView(view);
}

void QQuickSwipeViewAttachedPrivate::itemChildAdded(QQuickItem *, QQuickItem *)
{
    updateIndex();
}

void QQuickSwipeViewAttachedPrivate::itemChildRemoved(QQuickItem *, QQuickItem *)
{
    updateIndex();
}

void QQuickSwipeViewAttachedPrivate::itemParentChanged(QQuickItem *, QQuickItem *parent)
{
    updateView(parent);
}

void QQuickSwipeViewAttachedPrivate::itemDestroyed(QQuickItem *item)
{
    QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Parent | QQuickItemPrivate::Destroyed);
}

QQuickSwipeViewAttached::QQuickSwipeViewAttached(QQuickItem *item) :
    QObject(*(new QQuickSwipeViewAttachedPrivate(item)), item)
{
    Q_D(QQuickSwipeViewAttached);
    if (item->parentItem())
        d->updateView(item->parentItem());

    QQuickItemPrivate *p = QQuickItemPrivate::get(item);
    p->addItemChangeListener(d, QQuickItemPrivate::Parent | QQuickItemPrivate::Destroyed);
}

QQuickSwipeViewAttached::~QQuickSwipeViewAttached()
{
    Q_D(QQuickSwipeViewAttached);
    QQuickItem *item = qobject_cast<QQuickItem *>(parent());
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(d, QQuickItemPrivate::Parent | QQuickItemPrivate::Destroyed);
}

QQuickSwipeView *QQuickSwipeViewAttached::view() const
{
    Q_D(const QQuickSwipeViewAttached);
    return d->swipeView;
}

int QQuickSwipeViewAttached::index() const
{
    Q_D(const QQuickSwipeViewAttached);
    return d->index;
}

bool QQuickSwipeViewAttached::isCurrentItem() const
{
    Q_D(const QQuickSwipeViewAttached);
    return d->swipeView ? d->swipeView->currentIndex() == d->index : false;
}

QT_END_NAMESPACE
