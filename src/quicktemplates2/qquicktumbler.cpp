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

    \section1 Non-wrapping Tumbler

    The default contentItem of Tumbler is a \l PathView, which wraps when it
    reaches the top and bottom. To achieve a non-wrapping Tumbler, use ListView
    as the contentItem:

    \snippet tst_tumbler.qml contentItem

    \image qtquickcontrols2-tumbler-wrap.gif

    \sa {Customizing Tumbler}, {Input Controls}
*/

class QQuickTumblerPrivate : public QQuickControlPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickTumbler)

public:
    QQuickTumblerPrivate() :
        delegate(nullptr),
        visibleItemCount(3)
    {
    }

    ~QQuickTumblerPrivate()
    {
    }

    QVariant model;
    QQmlComponent *delegate;
    int visibleItemCount;

    void _q_updateItemHeights();
    void _q_updateItemWidths();

    void itemChildAdded(QQuickItem *, QQuickItem *) override;
    void itemChildRemoved(QQuickItem *, QQuickItem *) override;
};

static QList<QQuickItem *> contentItemChildItems(QQuickItem *contentItem)
{
    if (!contentItem)
        return QList<QQuickItem *>();

    // PathView has no contentItem property, but ListView does.
    QQuickFlickable *flickable = qobject_cast<QQuickFlickable *>(contentItem);
    return flickable ? flickable->contentItem()->childItems() : contentItem->childItems();
}

namespace {
    static inline qreal delegateHeight(const QQuickTumbler *tumbler)
    {
        return tumbler->availableHeight() / tumbler->visibleItemCount();
    }

    enum ContentItemType {
        UnsupportedContentItemType,
        PathViewContentItem,
        ListViewContentItem
    };

    static inline QQuickItem *actualContentItem(QQuickItem *rootContentItem, ContentItemType contentType)
    {
        if (contentType == PathViewContentItem)
            return rootContentItem;
        else if (contentType == ListViewContentItem)
            return qobject_cast<QQuickFlickable*>(rootContentItem)->contentItem();

        return nullptr;
    }

    static inline ContentItemType contentItemType(QQuickItem *rootContentItem)
    {
        if (rootContentItem->inherits("QQuickPathView"))
            return PathViewContentItem;
        else if (rootContentItem->inherits("QQuickListView"))
            return ListViewContentItem;

        return UnsupportedContentItemType;
    }

    static inline ContentItemType contentItemTypeFromDelegate(QQuickItem *delegateItem)
    {
        if (delegateItem->parentItem()->inherits("QQuickPathView")) {
            return PathViewContentItem;
        } else if (delegateItem->parentItem()->parentItem()
            && delegateItem->parentItem()->parentItem()->inherits("QQuickListView")) {
            return ListViewContentItem;
        }

        return UnsupportedContentItemType;
    }
}

void QQuickTumblerPrivate::_q_updateItemHeights()
{
    // Can't use our own private padding members here, as the padding property might be set,
    // which doesn't affect them, only their getters.
    Q_Q(const QQuickTumbler);
    const qreal itemHeight = delegateHeight(q);
    const auto items = contentItemChildItems(contentItem);
    for (QQuickItem *childItem : items)
        childItem->setHeight(itemHeight);
}

void QQuickTumblerPrivate::_q_updateItemWidths()
{
    Q_Q(const QQuickTumbler);
    const qreal availableWidth = q->availableWidth();
    const auto items = contentItemChildItems(contentItem);
    for (QQuickItem *childItem : items)
        childItem->setWidth(availableWidth);
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
    return d->contentItem->property("count").toInt();
}

/*!
    \qmlproperty int QtQuick.Controls::Tumbler::currentIndex

    This property holds the index of the current item.
*/
int QQuickTumbler::currentIndex() const
{
    Q_D(const QQuickTumbler);
    return d->contentItem ? d->contentItem->property("currentIndex").toInt() : -1;
}

void QQuickTumbler::setCurrentIndex(int currentIndex)
{
    Q_D(QQuickTumbler);
    d->contentItem->setProperty("currentIndex", currentIndex);
}

/*!
    \qmlproperty Item QtQuick.Controls::Tumbler::currentItem
    \readonly

    This property holds the item at the current index.
*/
QQuickItem *QQuickTumbler::currentItem() const
{
    Q_D(const QQuickTumbler);
    return d->contentItem ? d->contentItem->property("currentItem").value<QQuickItem*>() : nullptr;
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

QQuickTumblerAttached *QQuickTumbler::qmlAttachedProperties(QObject *object)
{
    QQuickItem *delegateItem = qobject_cast<QQuickItem *>(object);
    if (!delegateItem) {
        qWarning() << "Tumbler: attached properties of Tumbler must be accessed from within a delegate item";
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
}

void QQuickTumbler::contentItemChange(QQuickItem *newItem, QQuickItem *oldItem)
{
    Q_D(QQuickTumbler);

    QQuickControl::contentItemChange(newItem, oldItem);

    // Since we use the currentIndex of the contentItem directly, we must
    // ensure that we keep track of the currentIndex so it doesn't get lost
    // between contentItem changes.
    const int previousCurrentIndex = currentIndex();

    if (oldItem) {
        disconnect(oldItem, SIGNAL(currentIndexChanged()), this, SIGNAL(currentIndexChanged()));
        disconnect(oldItem, SIGNAL(currentItemChanged()), this, SIGNAL(currentItemChanged()));
        disconnect(oldItem, SIGNAL(countChanged()), this, SIGNAL(countChanged()));

        ContentItemType oldContentItemType = contentItemType(oldItem);
        QQuickItem *actualOldContentItem = actualContentItem(oldItem, oldContentItemType);
        QQuickItemPrivate *actualContentItemPrivate = QQuickItemPrivate::get(actualOldContentItem);
        actualContentItemPrivate->removeItemChangeListener(d, QQuickItemPrivate::Children);
    }

    if (newItem) {
        ContentItemType contentType = contentItemType(newItem);
        if (contentType == UnsupportedContentItemType) {
            qWarning() << "Tumbler: contentItems other than PathView and ListView are not supported";
            return;
        }

        connect(newItem, SIGNAL(currentIndexChanged()), this, SIGNAL(currentIndexChanged()));
        connect(newItem, SIGNAL(currentItemChanged()), this, SIGNAL(currentItemChanged()));
        connect(newItem, SIGNAL(countChanged()), this, SIGNAL(countChanged()));

        QQuickItem *actualNewContentItem = actualContentItem(newItem, contentType);
        QQuickItemPrivate *actualContentItemPrivate = QQuickItemPrivate::get(actualNewContentItem);
        actualContentItemPrivate->addItemChangeListener(d, QQuickItemPrivate::Children);

        // If the previous currentIndex is -1, it means we had no contentItem previously.
        if (previousCurrentIndex != -1) {
            // Can't call setCurrentIndex here, as contentItemChange() is
            // called *before* the contentItem is set.
            newItem->setProperty("currentIndex", previousCurrentIndex);
        }
    }
}

void QQuickTumbler::keyPressEvent(QKeyEvent *event)
{
    Q_D(QQuickTumbler);

    QQuickControl::keyPressEvent(event);

    if (event->isAutoRepeat())
        return;

    if (event->key() == Qt::Key_Up) {
        QMetaObject::invokeMethod(d->contentItem, "decrementCurrentIndex");
    } else if (event->key() == Qt::Key_Down) {
        QMetaObject::invokeMethod(d->contentItem, "incrementCurrentIndex");
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
            qWarning() << "Tumbler: attached properties must be accessed from within a delegate item that has a parent";
            return;
        }

        QVariant indexContextProperty = qmlContext(delegateItem)->contextProperty(QStringLiteral("index"));
        if (!indexContextProperty.isValid()) {
            qWarning() << "Tumbler: attempting to access attached property on item without an \"index\" property";
            return;
        }

        index = indexContextProperty.toInt();
        const ContentItemType contentItemType = contentItemTypeFromDelegate(delegateItem);
        if (contentItemType == UnsupportedContentItemType)
            return;

        // ListView has an "additional" content item.
        tumbler = qobject_cast<QQuickTumbler* >(contentItemType == PathViewContentItem
            ? delegateItem->parentItem()->parentItem() : delegateItem->parentItem()->parentItem()->parentItem());
        Q_ASSERT(tumbler);
    }

    ~QQuickTumblerAttachedPrivate() {
    }

    void itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void itemChildAdded(QQuickItem *, QQuickItem *) override;
    void itemChildRemoved(QQuickItem *, QQuickItem *) override;

    void _q_calculateDisplacement();

    // The Tumbler that contains the delegate. Required to calculated the displacement.
    QQuickTumbler *tumbler;
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

    const int count = tumbler->count();
    // This can happen in tests, so it may happen in normal usage too.
    if (count == 0)
        return;

    ContentItemType contentType = contentItemType(tumbler->contentItem());
    if (contentType == UnsupportedContentItemType)
        return;

    qreal offset = 0;

    if (contentType == PathViewContentItem) {
        offset = tumbler->contentItem()->property("offset").toReal();

        displacement = count > 1 ? count - index - offset : 0;
        // Don't add 1 if count <= visibleItemCount
        const int visibleItems = tumbler->visibleItemCount();
        int halfVisibleItems = visibleItems / 2 + (visibleItems < count ? 1 : 0);
        if (displacement > halfVisibleItems)
            displacement -= count;
        else if (displacement < -halfVisibleItems)
            displacement += count;
    } else {
        const qreal contentY = tumbler->contentItem()->property("contentY").toReal();
        const qreal delegateH = delegateHeight(tumbler);
        const qreal preferredHighlightBegin = tumbler->contentItem()->property("preferredHighlightBegin").toReal();
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
        QQuickItem *rootContentItem = d->tumbler->contentItem();
        const ContentItemType contentType = contentItemType(rootContentItem);
        QQuickItemPrivate *p = QQuickItemPrivate::get(actualContentItem(rootContentItem, contentType));
        p->addItemChangeListener(d, QQuickItemPrivate::Geometry | QQuickItemPrivate::Children);

        const char *contentItemSignal = contentType == PathViewContentItem
            ? SIGNAL(offsetChanged()) : SIGNAL(contentYChanged());
        connect(d->tumbler->contentItem(), contentItemSignal, this, SLOT(_q_calculateDisplacement()));

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
