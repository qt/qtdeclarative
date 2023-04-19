// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qquickstacklayout_p.h"

#include <limits>

#include <QtQml/qqmlinfo.h>

/*!
    \qmltype StackLayout
    //! \instantiates QQuickStackLayout
    \inherits Item
    \inqmlmodule QtQuick.Layouts
    \ingroup layouts
    \brief The StackLayout class provides a stack of items where
    only one item is visible at a time.

    To be able to use this type more efficiently, it is recommended that you
    understand the general mechanism of the Qt Quick Layouts module. Refer to
    \l{Qt Quick Layouts Overview} for more information.

    The current visible item can be modified by setting the \l currentIndex property.
    The index corresponds to the order of the StackLayout's children.

    In contrast to most other layouts, child Items' \l{Layout::fillWidth}{Layout.fillWidth} and \l{Layout::fillHeight}{Layout.fillHeight} properties
    default to \c true. As a consequence, child items are by default filled to match the size of the StackLayout as long as their
    \l{Layout::maximumWidth}{Layout.maximumWidth} or \l{Layout::maximumHeight}{Layout.maximumHeight} does not prevent it.

    Items are added to the layout by reparenting the item to the layout. Similarly, removal is done by reparenting the item from the layout.
    Both of these operations will affect the layout's \l count property.

    The following code will create a StackLayout where only the 'plum' rectangle is visible.
    \code
    StackLayout {
        id: layout
        anchors.fill: parent
        currentIndex: 1
        Rectangle {
            color: 'teal'
            implicitWidth: 200
            implicitHeight: 200
        }
        Rectangle {
            color: 'plum'
            implicitWidth: 300
            implicitHeight: 200
        }
    }
    \endcode

    Items in a StackLayout support these attached properties:
    \list
        \li \l{Layout::minimumWidth}{Layout.minimumWidth}
        \li \l{Layout::minimumHeight}{Layout.minimumHeight}
        \li \l{Layout::preferredWidth}{Layout.preferredWidth}
        \li \l{Layout::preferredHeight}{Layout.preferredHeight}
        \li \l{Layout::maximumWidth}{Layout.maximumWidth}
        \li \l{Layout::maximumHeight}{Layout.maximumHeight}
        \li \l{Layout::fillWidth}{Layout.fillWidth}
        \li \l{Layout::fillHeight}{Layout.fillHeight}
    \endlist

    Read more about attached properties \l{QML Object Attributes}{here}.
    \sa ColumnLayout
    \sa GridLayout
    \sa RowLayout
    \sa {QtQuick.Controls::StackView}{StackView}
    \sa {Qt Quick Layouts Overview}
*/

QT_BEGIN_NAMESPACE

static QQuickStackLayoutAttached *attachedStackLayoutObject(QQuickItem *item, bool create = false)
{
    return static_cast<QQuickStackLayoutAttached*>(
        qmlAttachedPropertiesObject<QQuickStackLayout>(item, create));
}

QQuickStackLayout::QQuickStackLayout(QQuickItem *parent) :
    QQuickLayout(*new QQuickStackLayoutPrivate, parent)
{
}

/*!
    \qmlproperty int StackLayout::count

    This property holds the number of items that belong to the layout.

    Only items that are children of the StackLayout will be candidates for layouting.
*/
int QQuickStackLayout::count() const
{
    Q_D(const QQuickStackLayout);
    return d->count;
}

/*!
    \qmlproperty int StackLayout::currentIndex

    This property holds the index of the child item that is currently visible in the StackLayout.
    By default it will be \c -1 for an empty layout, otherwise the default is \c 0 (referring to the first item).

    Since 6.5, inserting/removing a new Item at an index less than or equal to the current index
    will increment/decrement the current index, but keep the current Item.
*/
int QQuickStackLayout::currentIndex() const
{
    Q_D(const QQuickStackLayout);
    return d->currentIndex;
}

void QQuickStackLayout::setCurrentIndex(int index)
{
    Q_D(QQuickStackLayout);
    if (index == d->currentIndex)
        return;

    QQuickItem *prev = itemAt(d->currentIndex);
    QQuickItem *next = itemAt(index);
    d->currentIndex = index;
    d->explicitCurrentIndex = true;
    if (prev)
        prev->setVisible(false);
    if (next)
        next->setVisible(true);

    if (isComponentComplete()) {
        rearrange(QSizeF(width(), height()));
        emit currentIndexChanged();
    }

    // Update attached properties after emitting currentIndexChanged()
    // to maintain a more sensible emission order.
    if (prev) {
        auto stackLayoutAttached = attachedStackLayoutObject(prev);
        if (stackLayoutAttached)
            stackLayoutAttached->setIsCurrentItem(false);
    }
    if (next) {
        auto stackLayoutAttached = attachedStackLayoutObject(next);
        if (stackLayoutAttached)
            stackLayoutAttached->setIsCurrentItem(true);
    }
}

void QQuickStackLayout::componentComplete()
{
    QQuickLayout::componentComplete();    // will call our geometryChange(), (where isComponentComplete() == true)

    childItemsChanged();
    invalidate();
    ensureLayoutItemsUpdated(ApplySizeHints);

    QQuickItem *par = parentItem();
    if (qobject_cast<QQuickLayout*>(par))
        return;

    rearrange(QSizeF(width(), height()));
}

void QQuickStackLayout::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    QQuickLayout::itemChange(change, value);
    if (!isReady())
        return;

    if (change == ItemChildRemovedChange) {
        QQuickItem *item = value.item;
        auto stackLayoutAttached = attachedStackLayoutObject(item);
        if (stackLayoutAttached) {
            stackLayoutAttached->setLayout(nullptr);
            stackLayoutAttached->setIndex(-1);
            stackLayoutAttached->setIsCurrentItem(false);
        }
        m_cachedItemSizeHints.remove(item);
        childItemsChanged(AdjustCurrentIndex);  // removal; might have to adjust currentIndex
        invalidate();
    } else if (change == ItemChildAddedChange) {
        childItemsChanged();
        invalidate();
    }
}

QSizeF QQuickStackLayout::sizeHint(Qt::SizeHint whichSizeHint) const
{
    Q_D(const QQuickStackLayout);
    QSizeF &askingFor = m_cachedSizeHints[whichSizeHint];
    if (!askingFor.isValid()) {
        QSizeF &minS = m_cachedSizeHints[Qt::MinimumSize];
        QSizeF &prefS = m_cachedSizeHints[Qt::PreferredSize];
        QSizeF &maxS = m_cachedSizeHints[Qt::MaximumSize];

        minS = QSizeF(0,0);
        prefS = QSizeF(0,0);
        maxS = QSizeF(std::numeric_limits<qreal>::infinity(), std::numeric_limits<qreal>::infinity());

        const int count = itemCount();
        for (int i = 0; i < count; ++i) {
            SizeHints &hints = cachedItemSizeHints(i);
            minS = minS.expandedTo(hints.min());
            prefS = prefS.expandedTo(hints.pref());
            //maxS = maxS.boundedTo(hints.max());       // Can be resized to be larger than any of its items.
                                                        // This is the same as QStackLayout does it.
            // Not sure how descent makes sense here...
        }
    }
    d->m_dirty = false;
    return askingFor;
}

int QQuickStackLayout::indexOf(QQuickItem *childItem) const
{
    if (childItem) {
        int indexOfItem = 0;
        const auto items = childItems();
        for (QQuickItem *item : items) {
            if (shouldIgnoreItem(item))
                continue;
            if (childItem == item)
                return indexOfItem;
            ++indexOfItem;
        }
    }
    return -1;
}

QQuickStackLayoutAttached *QQuickStackLayout::qmlAttachedProperties(QObject *object)
{
    return new QQuickStackLayoutAttached(object);
}

QQuickItem *QQuickStackLayout::itemAt(int index) const
{
    const auto items = childItems();
    for (QQuickItem *item : items) {
        if (shouldIgnoreItem(item))
            continue;
        if (index == 0)
            return item;
        --index;
    }
    return nullptr;
}

int QQuickStackLayout::itemCount() const
{
    int count = 0;
    const auto items = childItems();
    for (QQuickItem *item : items) {
        if (shouldIgnoreItem(item))
            continue;
        ++count;
    }
    return count;
}

void QQuickStackLayout::setAlignment(QQuickItem * /*item*/, Qt::Alignment /*align*/)
{
    // ### Do we have to respect alignment?
}

void QQuickStackLayout::invalidate(QQuickItem *childItem)
{
    if (childItem) {
        SizeHints &hints = m_cachedItemSizeHints[childItem];
        hints.min() = QSizeF();
        hints.pref() = QSizeF();
        hints.max() = QSizeF();
    }

    for (int i = 0; i < Qt::NSizeHints; ++i)
        m_cachedSizeHints[i] = QSizeF();
    QQuickLayout::invalidate(this);

    if (QQuickLayout *parentLayout = qobject_cast<QQuickLayout *>(parentItem()))
        parentLayout->invalidate(this);
}

void QQuickStackLayout::childItemsChanged(AdjustCurrentIndexPolicy adjustCurrentIndexPolicy)
{
    Q_D(QQuickStackLayout);
    const int count = itemCount();
    const int oldIndex = d->currentIndex;
    if (!d->explicitCurrentIndex)
        d->currentIndex = (count > 0 ? 0 : -1);

    if (adjustCurrentIndexPolicy == AdjustCurrentIndex) {
        /*
         * If an item is inserted or deleted at an index less than or equal to the current index it
         * will affect the current index, but keep the current item. This is consistent with
         * QStackedLayout, QStackedWidget and TabBar
         *
         * Unless the caller is componentComplete(), we can assume that only one of the children
         * are visible, and we should keep that visible even if the stacking order has changed.
         * This means that if the sibling order has changed (or an item stacked before the current
         * item is added/removed), we must update the currentIndex so that it corresponds with the
         * current visible item.
         */
        if (d->currentIndex < d->count) {
            for (int i = 0; i < count; ++i) {
                QQuickItem *child = itemAt(i);
                if (child->isVisible()) {
                    d->currentIndex = i;
                    break;
                }
            }
        }
    }
    if (d->currentIndex != oldIndex)
        emit currentIndexChanged();

    if (count != d->count) {
        d->count = count;
        emit countChanged();
    }
    for (int i = 0; i < count; ++i) {
        QQuickItem *child = itemAt(i);
        checkAnchors(child);
        child->setVisible(d->currentIndex == i);

        auto stackLayoutAttached = attachedStackLayoutObject(child);
        if (stackLayoutAttached) {
            stackLayoutAttached->setLayout(this);
            stackLayoutAttached->setIndex(i);
            stackLayoutAttached->setIsCurrentItem(d->currentIndex == i);
        }
    }
}

QQuickStackLayout::SizeHints &QQuickStackLayout::cachedItemSizeHints(int index) const
{
    QQuickItem *item = itemAt(index);
    Q_ASSERT(item);
    SizeHints &hints = m_cachedItemSizeHints[item];     // will create an entry if it doesn't exist
    if (!hints.min().isValid())
        QQuickStackLayout::collectItemSizeHints(item, hints.array);
    return hints;
}


void QQuickStackLayout::rearrange(const QSizeF &newSize)
{
    Q_D(QQuickStackLayout);
    if (newSize.isNull() || !newSize.isValid())
        return;

    qCDebug(lcQuickLayouts) << "QQuickStackLayout::rearrange";

    if (d->currentIndex == -1 || d->currentIndex >= m_cachedItemSizeHints.size())
        return;
    QQuickStackLayout::SizeHints &hints = cachedItemSizeHints(d->currentIndex);
    QQuickItem *item = itemAt(d->currentIndex);
    Q_ASSERT(item);
    item->setPosition(QPointF(0,0));    // ### respect alignment?
    const QSizeF oldSize(item->width(), item->height());
    const QSizeF effectiveNewSize = newSize.expandedTo(hints.min()).boundedTo(hints.max());
    item->setSize(effectiveNewSize);
    if (effectiveNewSize == oldSize)
        item->polish();
    QQuickLayout::rearrange(newSize);
}

void QQuickStackLayout::setStretchFactor(QQuickItem * /*item*/, int /*stretchFactor*/, Qt::Orientation /*orient*/)
{
}

void QQuickStackLayout::collectItemSizeHints(QQuickItem *item, QSizeF *sizeHints)
{
    QQuickLayoutAttached *info = nullptr;
    QQuickLayout::effectiveSizeHints_helper(item, sizeHints, &info, true);
    if (!info)
        return;
    if (info->isFillWidthSet() && !info->fillWidth()) {
        const qreal pref = sizeHints[Qt::PreferredSize].width();
        sizeHints[Qt::MinimumSize].setWidth(pref);
        sizeHints[Qt::MaximumSize].setWidth(pref);
    }

    if (info->isFillHeightSet() && !info->fillHeight()) {
        const qreal pref = sizeHints[Qt::PreferredSize].height();
        sizeHints[Qt::MinimumSize].setHeight(pref);
        sizeHints[Qt::MaximumSize].setHeight(pref);
    }
}

bool QQuickStackLayout::shouldIgnoreItem(QQuickItem *item) const
{
    return QQuickItemPrivate::get(item)->isTransparentForPositioner();
}

void QQuickStackLayout::itemSiblingOrderChanged(QQuickItem *)
{
    if (!isReady())
        return;
    childItemsChanged(AdjustCurrentIndex);
    invalidate();
}

QQuickStackLayoutAttached::QQuickStackLayoutAttached(QObject *object)
{
    auto item = qobject_cast<QQuickItem*>(object);
    if (!item) {
        qmlWarning(object) << "StackLayout must be attached to an Item";
        return;
    }

    auto stackLayout = qobject_cast<QQuickStackLayout*>(item->parentItem());
    if (!stackLayout) {
        // It might not be a child of a StackLayout yet, and that's OK.
        // The index will get set by updateLayoutItems() when it's reparented.
        return;
    }

    if (!stackLayout->isComponentComplete()) {
        // Don't try to get the index if the StackLayout itself hasn't loaded yet.
        return;
    }

    // If we got this far, the item was added as a child to the StackLayout after it loaded.
    const int index = stackLayout->indexOf(item);
    setLayout(stackLayout);
    setIndex(index);
    setIsCurrentItem(stackLayout->currentIndex() == index);

    // In case of lazy loading in loader, attachedProperties are created and updated for the
    // object after adding the child object to the stack layout, which leads to entries with
    // same index. Triggering childItemsChanged() resets to right index in the stack layout.
    stackLayout->childItemsChanged();
}

/*!
    \qmlattachedproperty int StackLayout::index

    This attached property holds the index of each child item in the
    \l StackLayout.

    \sa isCurrentItem, layout

    \since QtQuick.Layouts 1.15
*/
int QQuickStackLayoutAttached::index() const
{
    return m_index;
}

void QQuickStackLayoutAttached::setIndex(int index)
{
    if (index == m_index)
        return;

    m_index = index;
    emit indexChanged();
}

/*!
    \qmlattachedproperty bool StackLayout::isCurrentItem

    This attached property is \c true if this child is the current item
    in the \l StackLayout.

    \sa index, layout

    \since QtQuick.Layouts 1.15
*/
bool QQuickStackLayoutAttached::isCurrentItem() const
{
    return m_isCurrentItem;
}

void QQuickStackLayoutAttached::setIsCurrentItem(bool isCurrentItem)
{
    if (isCurrentItem == m_isCurrentItem)
        return;

    m_isCurrentItem = isCurrentItem;
    emit isCurrentItemChanged();
}

/*!
    \qmlattachedproperty StackLayout StackLayout::layout

    This attached property holds the \l StackLayout that manages this child
    item.

    \sa index, isCurrentItem

    \since QtQuick.Layouts 1.15
*/
QQuickStackLayout *QQuickStackLayoutAttached::layout() const
{
    return m_layout;
}

void QQuickStackLayoutAttached::setLayout(QQuickStackLayout *layout)
{
    if (layout == m_layout)
        return;

    m_layout = layout;
    emit layoutChanged();
}

QT_END_NAMESPACE

#include "moc_qquickstacklayout_p.cpp"
