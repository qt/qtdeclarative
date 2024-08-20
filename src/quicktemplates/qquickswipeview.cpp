// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickswipeview_p.h"

#include <QtQml/qqmlinfo.h>
#include <QtQuickTemplates2/private/qquickcontainer_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype SwipeView
    \inherits Container
//!     \nativetype QQuickSwipeView
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols-navigation
    \ingroup qtquickcontrols-containers
    \ingroup qtquickcontrols-focusscopes
    \brief Enables the user to navigate pages by swiping sideways.

    SwipeView provides a swipe-based navigation model.

    \image qtquickcontrols-swipeview.gif

    SwipeView is populated with a set of pages. One page is visible at a time.
    The user can navigate between the pages by swiping sideways. Notice that
    SwipeView itself is entirely non-visual. It is recommended to combine it
    with PageIndicator, to give the user a visual clue that there are multiple
    pages.

    \snippet qtquickcontrols-swipeview-indicator.qml 1

    As shown above, SwipeView is typically populated with a static set of
    pages that are defined inline as children of the view. It is also possible
    to \l {Container::addItem()}{add}, \l {Container::insertItem()}{insert},
    \l {Container::moveItem()}{move}, and \l {Container::removeItem()}{remove}
    pages dynamically at run time.

    \include container-currentindex.qdocinc {file} {SwipeView} {TabBar}

    It is generally not advisable to add excessive amounts of pages to a
    SwipeView. However, when the amount of pages grows larger, or individual
    pages are relatively complex, it may be desirable to free up resources by
    unloading pages that are outside the immediate reach of the user.
    The following example presents how to use \l Loader to keep a maximum of
    three pages simultaneously instantiated.

    \code
    SwipeView {
        Repeater {
            model: 6
            Loader {
                active: SwipeView.isCurrentItem || SwipeView.isNextItem || SwipeView.isPreviousItem
                sourceComponent: Text {
                    text: index
                    Component.onCompleted: console.log("created:", index)
                    Component.onDestruction: console.log("destroyed:", index)
                }
            }
        }
    }
    \endcode

    \note SwipeView takes over the geometry management of items added to the
          view. Using anchors on the items is not supported, and any \c width
          or \c height assignment will be overridden by the view. Notice that
          this only applies to the root of the item. Specifying width and height,
          or using anchors for its children works as expected.

    \sa TabBar, PageIndicator, {Customizing SwipeView}, {Navigation Controls}, {Container Controls},
        {Focus Management in Qt Quick Controls}
*/

class QQuickSwipeViewPrivate : public QQuickContainerPrivate
{
    Q_DECLARE_PUBLIC(QQuickSwipeView)

public:
    void resizeItem(int index, QQuickItem *item);
    void resizeItems();

    static QQuickSwipeViewPrivate *get(QQuickSwipeView *view);

    void itemImplicitWidthChanged(QQuickItem *item) override;
    void itemImplicitHeightChanged(QQuickItem *item) override;

    qreal getContentWidth() const override;
    qreal getContentHeight() const override;

    bool interactive = true;
    Qt::Orientation orientation = Qt::Horizontal;
};

class QQuickSwipeViewAttachedPrivate : public QObjectPrivate
{
public:
    Q_DECLARE_PUBLIC(QQuickSwipeViewAttached)

    static QQuickSwipeViewAttachedPrivate *get(QQuickSwipeViewAttached *attached)
    {
        return attached->d_func();
    }

    void update(QQuickSwipeView *newView, int newIndex);
    void updateCurrentIndex();
    void setCurrentIndex(int i);

    QQuickSwipeView *swipeView = nullptr;
    int index = -1;
    int currentIndex = -1;
};

void QQuickSwipeViewPrivate::resizeItem(int index, QQuickItem *item)
{
    QQuickAnchors *anchors = QQuickItemPrivate::get(item)->_anchors;
    // TODO: expose QQuickAnchorLine so we can test for other conflicting anchors
    if (anchors && (anchors->fill() || anchors->centerIn()) && !item->property("_q_QQuickSwipeView_warned").toBool()) {
        qmlWarning(item) << "SwipeView has detected conflicting anchors. Unable to layout the item.";
        item->setProperty("_q_QQuickSwipeView_warned", true);
    }
    if (orientation == Qt::Horizontal)
        item->setPosition({index * (contentItem->width() + spacing), 0});
    else
        item->setPosition({0, index * (contentItem->height() + spacing)});
    item->setSize(QSizeF(contentItem->width(), contentItem->height()));
}

void QQuickSwipeViewPrivate::resizeItems()
{
    Q_Q(QQuickSwipeView);
    const int count = q->count();
    for (int i = 0; i < count; ++i) {
        QQuickItem *item = itemAt(i);
        if (item)
            resizeItem(i, item);
    }
}

QQuickSwipeViewPrivate *QQuickSwipeViewPrivate::get(QQuickSwipeView *view)
{
    return view->d_func();
}

void QQuickSwipeViewPrivate::itemImplicitWidthChanged(QQuickItem *item)
{
    Q_Q(QQuickSwipeView);
    QQuickContainerPrivate::itemImplicitWidthChanged(item);
    if (item == q->currentItem())
        updateImplicitContentWidth();
}

void QQuickSwipeViewPrivate::itemImplicitHeightChanged(QQuickItem *item)
{
    Q_Q(QQuickSwipeView);
    QQuickContainerPrivate::itemImplicitHeightChanged(item);
    if (item == q->currentItem())
        updateImplicitContentHeight();
}

qreal QQuickSwipeViewPrivate::getContentWidth() const
{
    Q_Q(const QQuickSwipeView);
    QQuickItem *currentItem = q->currentItem();
    return currentItem ? currentItem->implicitWidth() : 0;
}

qreal QQuickSwipeViewPrivate::getContentHeight() const
{
    Q_Q(const QQuickSwipeView);
    QQuickItem *currentItem = q->currentItem();
    return currentItem ? currentItem->implicitHeight() : 0;
}

QQuickSwipeView::QQuickSwipeView(QQuickItem *parent)
    : QQuickContainer(*(new QQuickSwipeViewPrivate), parent)
{
    Q_D(QQuickSwipeView);
    d->changeTypes |= QQuickItemPrivate::ImplicitWidth | QQuickItemPrivate::ImplicitHeight;
    setFlag(ItemIsFocusScope);
    setActiveFocusOnTab(true);
    QObjectPrivate::connect(this, &QQuickContainer::currentItemChanged, d, &QQuickControlPrivate::updateImplicitContentSize);
}

/*!
    \since QtQuick.Controls 2.1 (Qt 5.8)
    \qmlproperty bool QtQuick.Controls::SwipeView::interactive

    This property describes whether the user can interact with the SwipeView.
    The user cannot swipe a view that is not interactive.

    The default value is \c true.
*/
bool QQuickSwipeView::isInteractive() const
{
    Q_D(const QQuickSwipeView);
    return d->interactive;
}

void QQuickSwipeView::setInteractive(bool interactive)
{
    Q_D(QQuickSwipeView);
    if (d->interactive == interactive)
        return;

    d->interactive = interactive;
    emit interactiveChanged();
}

/*!
    \since QtQuick.Controls 2.2 (Qt 5.9)
    \qmlproperty enumeration QtQuick.Controls::SwipeView::orientation

    This property holds the orientation.

    Possible values:
    \value Qt.Horizontal Horizontal (default)
    \value Qt.Vertical Vertical

    \sa horizontal, vertical
*/
Qt::Orientation QQuickSwipeView::orientation() const
{
    Q_D(const QQuickSwipeView);
    return d->orientation;
}

void QQuickSwipeView::setOrientation(Qt::Orientation orientation)
{
    Q_D(QQuickSwipeView);
    if (d->orientation == orientation)
        return;

    d->orientation = orientation;
    if (isComponentComplete())
        d->resizeItems();
    emit orientationChanged();
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty bool QtQuick.Controls::SwipeView::horizontal
    \readonly

    This property holds whether the swipe view is horizontal.

    \sa orientation
*/
bool QQuickSwipeView::isHorizontal() const
{
    Q_D(const QQuickSwipeView);
    return d->orientation == Qt::Horizontal;
}

/*!
    \since QtQuick.Controls 2.3 (Qt 5.10)
    \qmlproperty bool QtQuick.Controls::SwipeView::vertical
    \readonly

    This property holds whether the swipe view is vertical.

    \sa orientation
*/
bool QQuickSwipeView::isVertical() const
{
    Q_D(const QQuickSwipeView);
    return d->orientation == Qt::Vertical;
}

QQuickSwipeViewAttached *QQuickSwipeView::qmlAttachedProperties(QObject *object)
{
    return new QQuickSwipeViewAttached(object);
}

void QQuickSwipeView::componentComplete()
{
    Q_D(QQuickSwipeView);
    QQuickContainer::componentComplete();
    d->resizeItems();
}

void QQuickSwipeView::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickSwipeView);
    QQuickContainer::geometryChange(newGeometry, oldGeometry);
    d->resizeItems();
}

void QQuickSwipeView::itemAdded(int index, QQuickItem *item)
{
    Q_D(QQuickSwipeView);
    if (isComponentComplete())
        d->resizeItem(index, item);
    QQuickSwipeViewAttached *attached = qobject_cast<QQuickSwipeViewAttached *>(qmlAttachedPropertiesObject<QQuickSwipeView>(item));
    if (attached)
        QQuickSwipeViewAttachedPrivate::get(attached)->update(this, index);
}

void QQuickSwipeView::itemMoved(int index, QQuickItem *item)
{
    QQuickSwipeViewAttached *attached = qobject_cast<QQuickSwipeViewAttached *>(qmlAttachedPropertiesObject<QQuickSwipeView>(item));
    if (attached)
        QQuickSwipeViewAttachedPrivate::get(attached)->update(this, index);
}

void QQuickSwipeView::itemRemoved(int, QQuickItem *item)
{
    QQuickSwipeViewAttached *attached = qobject_cast<QQuickSwipeViewAttached *>(qmlAttachedPropertiesObject<QQuickSwipeView>(item));
    if (attached)
        QQuickSwipeViewAttachedPrivate::get(attached)->update(nullptr, -1);
}

#if QT_CONFIG(accessibility)
QAccessible::Role QQuickSwipeView::accessibleRole() const
{
    return QAccessible::PageTabList;
}
#endif

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
    \qmlattachedproperty bool QtQuick.Controls::SwipeView::isNextItem
    \since QtQuick.Controls 2.1 (Qt 5.8)
    \readonly

    This attached property is \c true if this child is the next item.

    It is attached to each child item of the SwipeView.
*/

/*!
    \qmlattachedproperty bool QtQuick.Controls::SwipeView::isPreviousItem
    \since QtQuick.Controls 2.1 (Qt 5.8)
    \readonly

    This attached property is \c true if this child is the previous item.

    It is attached to each child item of the SwipeView.
*/

/*!
    \qmlattachedproperty SwipeView QtQuick.Controls::SwipeView::view
    \readonly

    This attached property holds the view that manages this child item.

    It is attached to each child item of the SwipeView.
*/

void QQuickSwipeViewAttachedPrivate::updateCurrentIndex()
{
    setCurrentIndex(swipeView ? swipeView->currentIndex() : -1);
}

void QQuickSwipeViewAttachedPrivate::setCurrentIndex(int i)
{
    if (i == currentIndex)
        return;

    Q_Q(QQuickSwipeViewAttached);
    const bool wasCurrent = q->isCurrentItem();
    const bool wasNext = q->isNextItem();
    const bool wasPrevious = q->isPreviousItem();

    currentIndex = i;
    if (wasCurrent != q->isCurrentItem())
        emit q->isCurrentItemChanged();
    if (wasNext != q->isNextItem())
        emit q->isNextItemChanged();
    if (wasPrevious != q->isPreviousItem())
        emit q->isPreviousItemChanged();
}

void QQuickSwipeViewAttachedPrivate::update(QQuickSwipeView *newView, int newIndex)
{
    Q_Q(QQuickSwipeViewAttached);
    int oldIndex = index;
    QQuickSwipeView *oldView = swipeView;

    index = newIndex;
    swipeView = newView;

    if (oldView != newView) {
        if (oldView) {
            disconnect(oldView, &QQuickSwipeView::currentIndexChanged,
                       this, &QQuickSwipeViewAttachedPrivate::updateCurrentIndex);
        }
        if (newView) {
            connect(newView, &QQuickSwipeView::currentIndexChanged,
                    this, &QQuickSwipeViewAttachedPrivate::updateCurrentIndex);
        }
        emit q->viewChanged();
    }
    if (oldIndex != newIndex)
        emit q->indexChanged();

    updateCurrentIndex();
}

QQuickSwipeViewAttached::QQuickSwipeViewAttached(QObject *parent)
    : QObject(*(new QQuickSwipeViewAttachedPrivate), parent)
{
    if (!qobject_cast<QQuickItem *>(parent))
        qmlWarning(parent) << "SwipeView: attached properties must be accessed from within a child item";
}

int QQuickSwipeViewAttached::index() const
{
    Q_D(const QQuickSwipeViewAttached);
    return d->index;
}

bool QQuickSwipeViewAttached::isCurrentItem() const
{
    Q_D(const QQuickSwipeViewAttached);
    return d->index != -1 && d->currentIndex != -1 && d->index == d->currentIndex;
}

QQuickSwipeView *QQuickSwipeViewAttached::view() const
{
    Q_D(const QQuickSwipeViewAttached);
    return d->swipeView;
}

bool QQuickSwipeViewAttached::isNextItem() const
{
    Q_D(const QQuickSwipeViewAttached);
    return d->index != -1 && d->currentIndex != -1 && d->index == d->currentIndex + 1;
}

bool QQuickSwipeViewAttached::isPreviousItem() const
{
    Q_D(const QQuickSwipeViewAttached);
    return d->index != -1 && d->currentIndex != -1 && d->index == d->currentIndex - 1;
}

QT_END_NAMESPACE

#include "moc_qquickswipeview_p.cpp"
