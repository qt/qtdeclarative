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

#include "qquicktumbler_p.h"

#include <QtQuick/private/qquickflickable_p.h>
#include <QtQuickTemplates2/private/qquickcontrol_p_p.h>

QT_BEGIN_NAMESPACE

/*!
    \qmltype Tumbler
    \inherits Control
    \instantiates QQuickTumbler
    \inqmlmodule QtQuick.Controls
    \since 5.7
    \ingroup qtquickcontrols2-input
    \brief A spinnable wheel of items that can be selected.

    \code
    Tumbler {
        model: 5
        // ...
    }
    \endcode

    By default, Tumbler wraps when it reaches the top and bottom. To achieve a
    non-wrapping Tumbler, set the \l wrap property to \c false:

    \snippet qtquickcontrols2-tumbler-timePicker.qml tumbler

    \image qtquickcontrols2-tumbler-wrap.gif

    \sa {Customizing Tumbler}, {Input Controls}
*/

class QQuickTumblerPrivate : public QQuickControlPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickTumbler)

public:
    QQuickTumblerPrivate() :
        delegate(nullptr),
        visibleItemCount(3),
        wrap(true),
        view(nullptr),
        viewContentItem(nullptr),
        viewContentItemType(UnsupportedContentItemType),
        currentIndex(-1),
        ignoreCurrentIndexChanges(false)
    {
    }

    ~QQuickTumblerPrivate()
    {
    }

    enum ContentItemType {
        UnsupportedContentItemType,
        PathViewContentItem,
        ListViewContentItem
    };

    QQuickItem *determineViewType(QQuickItem *contentItem);
    void resetViewData();
    QList<QQuickItem *> viewContentItemChildItems() const;

    static QQuickTumblerPrivate *get(QQuickTumbler *tumbler)
    {
        return tumbler->d_func();
    }

    QVariant model;
    QQmlComponent *delegate;
    int visibleItemCount;
    bool wrap;
    QQuickItem *view;
    QQuickItem *viewContentItem;
    ContentItemType viewContentItemType;
    int currentIndex;
    bool ignoreCurrentIndexChanges;

    void _q_updateItemHeights();
    void _q_updateItemWidths();
    void _q_onViewCurrentIndexChanged();

    void disconnectFromView();
    void setupViewData(QQuickItem *newControlContentItem);

    void itemChildAdded(QQuickItem *, QQuickItem *) override;
    void itemChildRemoved(QQuickItem *, QQuickItem *) override;
};

namespace {
    static inline qreal delegateHeight(const QQuickTumbler *tumbler)
    {
        return tumbler->availableHeight() / tumbler->visibleItemCount();
    }
}

/*
    Finds the contentItem of the view that is a child of the control's \a contentItem.
    The type is stored in \a type.
*/
QQuickItem *QQuickTumblerPrivate::determineViewType(QQuickItem *contentItem)
{
    if (contentItem->inherits("QQuickPathView")) {
        view = contentItem;
        viewContentItem = contentItem;
        viewContentItemType = PathViewContentItem;
        return contentItem;
    } else if (contentItem->inherits("QQuickListView")) {
        view = contentItem;
        viewContentItem = qobject_cast<QQuickFlickable*>(contentItem)->contentItem();
        viewContentItemType = ListViewContentItem;
        return contentItem;
    } else {
        const auto childItems = contentItem->childItems();
        for (QQuickItem *childItem : childItems) {
            QQuickItem *item = determineViewType(childItem);
            if (item)
                return item;
        }
    }

    resetViewData();
    return nullptr;
}

void QQuickTumblerPrivate::resetViewData()
{
    view = nullptr;
    viewContentItem = nullptr;
    viewContentItemType = UnsupportedContentItemType;
}

QList<QQuickItem *> QQuickTumblerPrivate::viewContentItemChildItems() const
{
    if (!viewContentItem)
        return QList<QQuickItem *>();

    return viewContentItem->childItems();
}

void QQuickTumblerPrivate::_q_updateItemHeights()
{
    // Can't use our own private padding members here, as the padding property might be set,
    // which doesn't affect them, only their getters.
    Q_Q(const QQuickTumbler);
    const qreal itemHeight = delegateHeight(q);
    const auto items = viewContentItemChildItems();
    for (QQuickItem *childItem : items)
        childItem->setHeight(itemHeight);
}

void QQuickTumblerPrivate::_q_updateItemWidths()
{
    Q_Q(const QQuickTumbler);
    const qreal availableWidth = q->availableWidth();
    const auto items = viewContentItemChildItems();
    for (QQuickItem *childItem : items)
        childItem->setWidth(availableWidth);
}

void QQuickTumblerPrivate::_q_onViewCurrentIndexChanged()
{
    Q_Q(QQuickTumbler);
    if (!ignoreCurrentIndexChanges) {
        Q_ASSERT(view);
        const int oldCurrentIndex = currentIndex;

        currentIndex = view->property("currentIndex").toInt();

        if (oldCurrentIndex != currentIndex)
            emit q->currentIndexChanged();
    }
}

void QQuickTumblerPrivate::itemChildAdded(QQuickItem *, QQuickItem *)
{
    _q_updateItemWidths();
    _q_updateItemHeights();
}

void QQuickTumblerPrivate::itemChildRemoved(QQuickItem *, QQuickItem *)
{
    _q_updateItemWidths();
    _q_updateItemHeights();
}

QQuickTumbler::QQuickTumbler(QQuickItem *parent) :
    QQuickControl(*(new QQuickTumblerPrivate), parent)
{
    setActiveFocusOnTab(true);

    connect(this, SIGNAL(leftPaddingChanged()), this, SLOT(_q_updateItemWidths()));
    connect(this, SIGNAL(rightPaddingChanged()), this, SLOT(_q_updateItemWidths()));
    connect(this, SIGNAL(topPaddingChanged()), this, SLOT(_q_updateItemHeights()));
    connect(this, SIGNAL(bottomPaddingChanged()), this, SLOT(_q_updateItemHeights()));
}

QQuickTumbler::~QQuickTumbler()
{
    Q_D(QQuickTumbler);
    // Ensure that the item change listener is removed.
    d->disconnectFromView();
}

/*!
    \qmlproperty variant QtQuick.Controls::Tumbler::model

    This property holds the model that provides data for this tumbler.
*/
QVariant QQuickTumbler::model() const
{
    Q_D(const QQuickTumbler);
    return d->model;
}

void QQuickTumbler::setModel(const QVariant &model)
{
    Q_D(QQuickTumbler);
    if (model == d->model)
        return;

    d->model = model;
    emit modelChanged();
}

/*!
    \qmlproperty int QtQuick.Controls::Tumbler::count
    \readonly

    This property holds the number of items in the model.
*/
int QQuickTumbler::count() const
{
    Q_D(const QQuickTumbler);
    return d->view ? d->view->property("count").toInt() : 0;
}

/*!
    \qmlproperty int QtQuick.Controls::Tumbler::currentIndex

    This property holds the index of the current item.
*/
int QQuickTumbler::currentIndex() const
{
    Q_D(const QQuickTumbler);
    return d->currentIndex;
}

void QQuickTumbler::setCurrentIndex(int currentIndex)
{
    Q_D(QQuickTumbler);
    if (currentIndex == d->currentIndex)
        return;

    d->currentIndex = currentIndex;

    if (d->view) {
        // The view might not have been created yet, as is the case
        // if you create a Tumbler component and pass e.g. { currentIndex: 2 }
        // to createObject().
        d->ignoreCurrentIndexChanges = true;
        d->view->setProperty("currentIndex", currentIndex);
        d->ignoreCurrentIndexChanges = false;
    }

    emit currentIndexChanged();
}

/*!
    \qmlproperty Item QtQuick.Controls::Tumbler::currentItem
    \readonly

    This property holds the item at the current index.
*/
QQuickItem *QQuickTumbler::currentItem() const
{
    Q_D(const QQuickTumbler);
    return d->view ? d->view->property("currentItem").value<QQuickItem*>() : nullptr;
}

/*!
    \qmlproperty component QtQuick.Controls::Tumbler::delegate

    This property holds the delegate used to display each item.
*/
QQmlComponent *QQuickTumbler::delegate() const
{
    Q_D(const QQuickTumbler);
    return d->delegate;
}

void QQuickTumbler::setDelegate(QQmlComponent *delegate)
{
    Q_D(QQuickTumbler);
    if (delegate == d->delegate)
        return;

    d->delegate = delegate;
    emit delegateChanged();
}

/*!
    \qmlproperty int QtQuick.Controls::Tumbler::visibleItemCount

    This property holds the number of items visible in the tumbler. It must be
    an odd number, as the current item is always vertically centered.
*/
int QQuickTumbler::visibleItemCount() const
{
    Q_D(const QQuickTumbler);
    return d->visibleItemCount;
}

void QQuickTumbler::setVisibleItemCount(int visibleItemCount)
{
    Q_D(QQuickTumbler);
    if (visibleItemCount == d->visibleItemCount)
        return;

    d->visibleItemCount = visibleItemCount;
    d->_q_updateItemHeights();
    emit visibleItemCountChanged();
}

/*!
    \qmlproperty bool QtQuick.Controls::Tumbler::wrap
    \since QtQuick.Controls 2.1

    This property determines whether or not the tumbler wraps around when it
    reaches the top or bottom.

    It is recommended to set this property to \c false when \l count is less than
    \l visibleItemCount, as it is simpler to interact with a non-wrapping Tumbler
    when there are only a few items.
*/
bool QQuickTumbler::wrap() const
{
    Q_D(const QQuickTumbler);
    return d->wrap;
}

void QQuickTumbler::setWrap(bool wrap)
{
    Q_D(QQuickTumbler);
    if (isComponentComplete() && wrap == d->wrap)
        return;

    // Since we use the currentIndex of the contentItem directly, we must
    // ensure that we keep track of the currentIndex so it doesn't get lost
    // between view changes.
    const int oldCurrentIndex = currentIndex();

    d->disconnectFromView();

    d->wrap = wrap;

    // New views will set their currentIndex upon creation, which we'd otherwise
    // take as the correct one, so we must ignore them.
    d->ignoreCurrentIndexChanges = true;

    // This will cause the view to be created if our contentItem is a TumblerView.
    emit wrapChanged();

    d->ignoreCurrentIndexChanges = false;

    // The view should have been created now, so we can start determining its type, etc.
    // If the delegates use attached properties, this will have already been called,
    // in which case it will return early. If the delegate doesn't use attached properties,
    // we need to call it here.
    d->setupViewData(d->contentItem);

    setCurrentIndex(oldCurrentIndex);
}

QQuickTumblerAttached *QQuickTumbler::qmlAttachedProperties(QObject *object)
{
    QQuickItem *delegateItem = qobject_cast<QQuickItem *>(object);
    if (!delegateItem) {
        qWarning() << "Tumbler: attached properties of Tumbler must be accessed through a delegate item";
        return nullptr;
    }

    return new QQuickTumblerAttached(delegateItem);
}

void QQuickTumbler::geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    Q_D(QQuickTumbler);

    QQuickControl::geometryChanged(newGeometry, oldGeometry);

    d->_q_updateItemHeights();

    if (newGeometry.width() != oldGeometry.width())
        d->_q_updateItemWidths();
}

void QQuickTumbler::componentComplete()
{
    Q_D(QQuickTumbler);
    QQuickControl::componentComplete();
    d->_q_updateItemHeights();
    d->_q_updateItemWidths();

    if (!d->view) {
        // We don't want to create a PathView or ListView until we're certain
        // which one we need, and if wrap is not set, it will be the default.
        // We can only know the final value of wrap when componentComplete() is called,
        // so, if the view hasn't already been created, we cause it to be created here.
        emit wrapChanged();
        // Then, we determine the type of view for attached properties, etc.
        d->setupViewData(d->contentItem);
    }
}

void QQuickTumbler::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickTumbler);

    QQuickControl::contentItemChange(newItem, oldItem);

    if (oldItem)
        d->disconnectFromView();

    if (newItem) {
        // We wait until wrap is set to that we know which type of view to create.
        // If we try to set up the view too early, we'll issue warnings about it not existing.
        if (isComponentComplete()) {
            // Make sure we use the new content item and not the current one, as that won't
            // be changed until after contentItemChange() has finished.
            d->setupViewData(newItem);
        }
    }
}

void QQuickTumblerPrivate::disconnectFromView()
{
    Q_Q(QQuickTumbler);
    if (!view && contentItem && !q->isComponentComplete()) {
        // If a custom content item is declared, it can happen that
        // the original contentItem exists without the view etc. having been
        // determined yet, and then this is called when the custom content item
        // is eventually set. In all other cases, a view should exist.
        return;
    }

    Q_ASSERT(view);
    QObject::disconnect(view, SIGNAL(currentIndexChanged()), q, SLOT(_q_onViewCurrentIndexChanged()));
    QObject::disconnect(view, SIGNAL(currentItemChanged()), q, SIGNAL(currentItemChanged()));
    QObject::disconnect(view, SIGNAL(countChanged()), q, SIGNAL(countChanged()));

    QQuickItemPrivate *oldViewContentItemPrivate = QQuickItemPrivate::get(viewContentItem);
    oldViewContentItemPrivate->removeItemChangeListener(this, QQuickItemPrivate::Children);

    resetViewData();
}

void QQuickTumblerPrivate::setupViewData(QQuickItem *newControlContentItem)
{
    // Don't do anything if we've already set up.
    if (view)
        return;

    determineViewType(newControlContentItem);

    if (viewContentItemType == QQuickTumblerPrivate::UnsupportedContentItemType) {
        qWarning() << "Tumbler: contentItem must contain either a PathView or a ListView";
        return;
    }

    Q_Q(QQuickTumbler);
    QObject::connect(view, SIGNAL(currentIndexChanged()), q, SLOT(_q_onViewCurrentIndexChanged()));
    QObject::connect(view, SIGNAL(currentItemChanged()), q, SIGNAL(currentItemChanged()));
    QObject::connect(view, SIGNAL(countChanged()), q, SIGNAL(countChanged()));

    QQuickItemPrivate *viewContentItemPrivate = QQuickItemPrivate::get(viewContentItem);
    viewContentItemPrivate->addItemChangeListener(this, QQuickItemPrivate::Children);

    const int actualViewIndex = view->property("currentIndex").toInt();
    if (actualViewIndex != currentIndex) {
        ignoreCurrentIndexChanges = true;
        view->setProperty("currentIndex", currentIndex);
        ignoreCurrentIndexChanges = false;

        // If we still couldn't set the currentIndex, it's probably out of bounds,
        // in which case we must respect the actual currentIndex.
        if (view->property("currentIndex").toInt() != currentIndex)
            q->setCurrentIndex(actualViewIndex);
    }
}

void QQuickTumbler::keyPressEvent(QKeyEvent *event)
{
    QQuickControl::keyPressEvent(event);

    Q_D(QQuickTumbler);
    if (event->isAutoRepeat() || !d->view)
        return;

    if (event->key() == Qt::Key_Up) {
        QMetaObject::invokeMethod(d->view, "decrementCurrentIndex");
    } else if (event->key() == Qt::Key_Down) {
        QMetaObject::invokeMethod(d->view, "incrementCurrentIndex");
    }
}

class QQuickTumblerAttachedPrivate : public QObjectPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickTumblerAttached)
public:
    QQuickTumblerAttachedPrivate(QQuickItem *delegateItem) :
        tumbler(nullptr),
        index(-1),
        displacement(0)
    {
        if (!delegateItem->parentItem()) {
            qWarning() << "Tumbler: attached properties must be accessed through a delegate item that has a parent";
            return;
        }

        QVariant indexContextProperty = qmlContext(delegateItem)->contextProperty(QStringLiteral("index"));
        if (!indexContextProperty.isValid()) {
            qWarning() << "Tumbler: attempting to access attached property on item without an \"index\" property";
            return;
        }

        index = indexContextProperty.toInt();

        QQuickItem *parentItem = delegateItem;
        while ((parentItem = parentItem->parentItem())) {
            if ((tumbler = qobject_cast<QQuickTumbler*>(parentItem)))
                break;
        }
    }

    ~QQuickTumblerAttachedPrivate() {
    }

    void itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChildAdded(QQuickItem *, QQuickItem *) override;
    void itemChildRemoved(QQuickItem *, QQuickItem *) override;

    void _q_calculateDisplacement();

    // The Tumbler that contains the delegate. Required to calculated the displacement.
    QPointer<QQuickTumbler> tumbler;
    // The index of the delegate. Used to calculate the displacement.
    int index;
    // The displacement for our delegate.
    qreal displacement;
};

void QQuickTumblerAttachedPrivate::itemGeometryChanged(QQuickItem *, const QRectF &, const QRectF &)
{
    _q_calculateDisplacement();
}

void QQuickTumblerAttachedPrivate::itemChildAdded(QQuickItem *, QQuickItem *)
{
    _q_calculateDisplacement();
}

void QQuickTumblerAttachedPrivate::itemChildRemoved(QQuickItem *item, QQuickItem *child)
{
    _q_calculateDisplacement();

    if (parent == child) {
        // The child that was removed from the contentItem was the delegate
        // that our properties are attached to. If we don't remove the change
        // listener, the contentItem will attempt to notify a destroyed
        // listener, causing a crash.

        // item is the "actual content item" of Tumbler's contentItem, i.e. a PathView or ListView.contentItem
        QQuickItemPrivate *p = QQuickItemPrivate::get(item);
        p->removeItemChangeListener(this, QQuickItemPrivate::Geometry | QQuickItemPrivate::Children);
    }
}

void QQuickTumblerAttachedPrivate::_q_calculateDisplacement()
{
    const int previousDisplacement = displacement;
    displacement = 0;

    // Can happen if the attached properties are accessed on the wrong type of item or the tumbler was destroyed.
    if (!tumbler)
        return;

    // Can happen if there is no ListView or PathView within the contentItem.
    QQuickTumblerPrivate *tumblerPrivate = QQuickTumblerPrivate::get(tumbler);
    if (!tumblerPrivate->viewContentItem)
        return;

    const int count = tumbler->count();
    // This can happen in tests, so it may happen in normal usage too.
    if (count == 0)
        return;

    if (tumblerPrivate->viewContentItemType == QQuickTumblerPrivate::PathViewContentItem) {
        const qreal offset = tumblerPrivate->view->property("offset").toReal();

        displacement = count > 1 ? count - index - offset : 0;
        // Don't add 1 if count <= visibleItemCount
        const int visibleItems = tumbler->visibleItemCount();
        const int halfVisibleItems = visibleItems / 2 + (visibleItems < count ? 1 : 0);
        if (displacement > halfVisibleItems)
            displacement -= count;
        else if (displacement < -halfVisibleItems)
            displacement += count;
    } else {
        const qreal contentY = tumblerPrivate->view->property("contentY").toReal();
        const qreal delegateH = delegateHeight(tumbler);
        const qreal preferredHighlightBegin = tumblerPrivate->view->property("preferredHighlightBegin").toReal();
        // Tumbler's displacement goes from negative at the top to positive towards the bottom, so we must switch this around.
        const qreal reverseDisplacement = (contentY + preferredHighlightBegin) / delegateH;
        displacement = reverseDisplacement - index;
    }

    Q_Q(QQuickTumblerAttached);
    if (displacement != previousDisplacement)
        emit q->displacementChanged();
}

QQuickTumblerAttached::QQuickTumblerAttached(QQuickItem *delegateItem) :
    QObject(*(new QQuickTumblerAttachedPrivate(delegateItem)), delegateItem)
{
    Q_D(QQuickTumblerAttached);
    if (d->tumbler) {
        // When the Tumbler is completed, wrapChanged() is emitted to let QQuickTumblerView
        // know that it can create the view. The view itself might instantiate delegates
        // that use attached properties. At this point, setupViewData() hasn't been called yet
        // (it's called on the next line in componentComplete()), so we call it here so that
        // we have access to the view.
        QQuickTumblerPrivate *tumblerPrivate = QQuickTumblerPrivate::get(d->tumbler);
        tumblerPrivate->setupViewData(tumblerPrivate->contentItem);

        if (!tumblerPrivate->viewContentItem)
            return;

        QQuickItemPrivate *p = QQuickItemPrivate::get(tumblerPrivate->viewContentItem);
        p->addItemChangeListener(d, QQuickItemPrivate::Geometry | QQuickItemPrivate::Children);

        const char *contentItemSignal = tumblerPrivate->viewContentItemType == QQuickTumblerPrivate::PathViewContentItem
            ? SIGNAL(offsetChanged()) : SIGNAL(contentYChanged());
        connect(tumblerPrivate->view, contentItemSignal, this, SLOT(_q_calculateDisplacement()));

        d->_q_calculateDisplacement();
    }
}

QQuickTumblerAttached::~QQuickTumblerAttached()
{
}

/*!
    \qmlattachedproperty Tumbler QtQuick.Controls::Tumbler::tumbler
    \readonly

    This attached property holds the tumbler. The property can be attached to
    a tumbler delegate. The value is \c null if the item is not a tumbler delegate.
*/
QQuickTumbler *QQuickTumblerAttached::tumbler() const
{
    Q_D(const QQuickTumblerAttached);
    return d->tumbler;
}

/*!
    \qmlattachedproperty real QtQuick.Controls::Tumbler::displacement
    \readonly

    This attached property holds a value from \c {-visibleItemCount / 2} to
    \c {visibleItemCount / 2}, which represents how far away this item is from
    being the current item, with \c 0 being completely current.

    For example, the item below will be 40% opaque when it is not the current item,
    and transition to 100% opacity when it becomes the current item:

    \code
    delegate: Text {
        text: modelData
        opacity: 0.4 + Math.max(0, 1 - Math.abs(Tumbler.displacement)) * 0.6
    }
    \endcode
*/
qreal QQuickTumblerAttached::displacement() const
{
    Q_D(const QQuickTumblerAttached);
    return d->displacement;
}

QT_END_NAMESPACE

#include "moc_qquicktumbler_p.cpp"
