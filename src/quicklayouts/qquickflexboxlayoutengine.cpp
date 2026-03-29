// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQuickLayouts/private/qquickflexboxlayoutengine_p.h>

QT_BEGIN_NAMESPACE

QQuickFlexboxLayoutEngine::QQuickFlexboxLayoutEngine()
{
}

QQuickFlexboxLayoutEngine::~QQuickFlexboxLayoutEngine()
{
    clearItems();
}

void QQuickFlexboxLayoutEngine::setFlexboxParentItem(QQuickFlexboxLayoutItem *item)
{
    Q_ASSERT(item != nullptr);
    if (qobject_cast<QQuickFlexboxLayout *>(item->quickItem())) {
        m_flexboxParentItem = item;
        // Yoga parent item shouldn't have measure function
        if (m_flexboxParentItem->hasMeasureFunc())
            m_flexboxParentItem->resetMeasureFunc();
    }
}

void QQuickFlexboxLayoutEngine::clearItems()
{
    for (auto &flexItem: m_flexLayoutItems)
        delete flexItem;
    m_flexLayoutItems.clear();
    // Clear the size hints as we removed all the items from the flex layout
    for (int hintIndex = 0; hintIndex < Qt::NSizeHints; hintIndex++)
        m_cachedSizeHints[hintIndex] = QSizeF();
}

void QQuickFlexboxLayoutEngine::insertItem(QQuickFlexboxLayoutItem *item)
{
    m_flexboxParentItem->insertChild(item, m_flexLayoutItems.count());
    m_flexLayoutItems.append(item);
}

int QQuickFlexboxLayoutEngine::itemCount() const
{
    return m_flexLayoutItems.count();
}

QQuickItem *QQuickFlexboxLayoutEngine::itemAt(int index) const
{
    if (index < 0 || index >= m_flexLayoutItems.count())
        return nullptr;
    return m_flexLayoutItems.at(index)->quickItem();
}

QQuickFlexboxLayoutItem *QQuickFlexboxLayoutEngine::findFlexboxLayoutItem(QQuickItem *item) const
{
    if (!item || (m_flexLayoutItems.count() <= 0))
        return nullptr;
    auto iterator = std::find_if(m_flexLayoutItems.cbegin(), m_flexLayoutItems.cend(),
                                    [item] (QQuickFlexboxLayoutItem *flexLayoutItem){
                                        return (flexLayoutItem->quickItem() == item);
                                    });
    return (iterator == m_flexLayoutItems.cend()) ? nullptr : *iterator;
}

void QQuickFlexboxLayoutEngine::collectItemSizeHints(QQuickFlexboxLayoutItem *flexItem, QSizeF *sizeHints) const
{
    QQuickLayoutAttached *info = nullptr;
    QQuickLayout::effectiveSizeHints_helper(flexItem->quickItem(), sizeHints, &info, true);

    if (!info)
        return;

    // Set layout margins to the flex item (Layout.margins)
    if (info->isMarginsSet())
        flexItem->setFlexMargin(QQuickFlexboxLayout::EdgeAll, info->margins());
    if (info->isLeftMarginSet())
        flexItem->setFlexMargin(QQuickFlexboxLayout::EdgeLeft, info->leftMargin());
    if (info->isRightMarginSet())
        flexItem->setFlexMargin(QQuickFlexboxLayout::EdgeRight, info->rightMargin());
    if (info->isTopMarginSet())
        flexItem->setFlexMargin(QQuickFlexboxLayout::EdgeTop, info->topMargin());
    if (info->isBottomMarginSet())
        flexItem->setFlexMargin(QQuickFlexboxLayout::EdgeBottom, info->bottomMargin());

    // Set child item to grow, shrink and stretch depending on the layout
    // properties.
    // If Layout.fillWidth or Layout.fillHeight is set as true, then the child
    // item within the layout can grow or shrink (considering the minimum and
    // maximum sizes) along the main axis which depends upon the flex
    // direction.
    // If both Layout.fillWidth and Layout.fillHeight are set as true, then the
    // child item within the layout need to grow or shrink in cross section and
    // it require stretch need to be set for the yoga flex child item.
    if (info->isFillWidthSet() || info->isFillHeightSet()) {
        // Set stretch to child item both width and height
        if (auto *parentLayoutItem = qobject_cast<QQuickFlexboxLayout *>(m_flexboxParentItem->quickItem())) {
            const bool horz = parentLayoutItem->direction() == QQuickFlexboxLayout::Row ||
                              parentLayoutItem->direction() == QQuickFlexboxLayout::RowReverse;
            if (horz) {
                // If the Layout.fillHeight not been set, the preferred height
                // will be set as height
                if (!info->fillHeight())
                    flexItem->setHeight(sizeHints[Qt::PreferredSize].height());
                flexItem->setFlexBasis(sizeHints[Qt::PreferredSize].width(), !info->fillWidth());
                // Set child item to grow on main-axis (i.e. the flex
                // direction)
                flexItem->setItemGrowAlongMainAxis(info->fillWidth() ? 1.0f : 0.0f);
                // Set child item to shrink on main-axis (i.e. the flex
                // direction)
                flexItem->setItemShrinkAlongMainAxis(info->fillWidth() ? 1.0f : 0.0f);
            }
            else {
                // If the Layout.fillWidth not been set, the preferred width
                // will be set as width
                if (!info->fillWidth())
                    flexItem->setWidth(sizeHints[Qt::PreferredSize].width());
                flexItem->setFlexBasis(sizeHints[Qt::PreferredSize].height(), !info->fillHeight());
                // Set child item to grow on main-axis (i.e. the flex
                // direction)
                flexItem->setItemGrowAlongMainAxis(info->fillHeight() ? 1.0f : 0.0f);
                // Set child item to shrink on main-axis (i.e. the flex
                // direction)
                flexItem->setItemShrinkAlongMainAxis(info->fillHeight() ? 1.0f : 0.0f);
            }
            // If the Layout.fillHeight not been set, the preferred height will be
            // set as height in the previous condition. Otherwise (for
            // Layout.fillHeight been set as true), make flex item to AlignStretch.
            // Thus it can also grow vertically.
            // Note: The same applies for Layout.fillWidth to grow horizontally.
            if ((horz && (qt_is_nan(flexItem->size().height()) && info->fillHeight()))
                || (!horz && (qt_is_nan(flexItem->size().width()) && info->fillWidth()))) {
                flexItem->setItemStretchAlongCrossSection();
            } else {
                flexItem->inheritItemStretchAlongCrossSection();
            }
        }
    }
}

SizeHints &QQuickFlexboxLayoutEngine::cachedItemSizeHints(int index) const
{
    QQuickFlexboxLayoutItem *flexBoxLayoutItem = m_flexLayoutItems.at(index);
    Q_ASSERT(flexBoxLayoutItem);
    SizeHints &hints = flexBoxLayoutItem->cachedItemSizeHints();
    if (!hints.min().isValid())
        collectItemSizeHints(flexBoxLayoutItem, hints.array);
    return hints;
}

QSizeF QQuickFlexboxLayoutEngine::sizeHint(Qt::SizeHint whichSizeHint) const
{
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
            auto &flexLayoutItem = m_flexLayoutItems.at(i);
            flexLayoutItem->setMinSize(hints.min());
            if (flexLayoutItem->isFlexBasisUndefined()) {
                // If flex basis is undefined and item is still stretched, it
                // meant the flex child item has a const width or height but
                // want to stretch vertically or horizontally
                if (flexLayoutItem->isItemStreched()) {
                    if (auto *parentLayoutItem = qobject_cast<QQuickFlexboxLayout *>(m_flexboxParentItem->quickItem())) {
                        // Reset the size of the child item if the parent sets
                        // its property 'align-item' to strecth
                        // Note: The child item can also override the parent
                        // align-item property through align-self
                        // (this is FlexboxLayout.alignItem for quick items)
                        flexLayoutItem->resetSize();
                        if (parentLayoutItem->direction() == QQuickFlexboxLayout::Row ||
                            parentLayoutItem->direction() == QQuickFlexboxLayout::RowReverse) {
                            flexLayoutItem->setWidth(hints.pref().width());
                        } else {
                            flexLayoutItem->setHeight(hints.pref().height());
                        }
                    }
                } else {
                    flexLayoutItem->setSize(hints.pref());
                }
            }
            flexLayoutItem->setMaxSize(hints.max());
            // The preferred size, minimum and maximum size of the parent item
            // will be calculated as follows
            // If no wrap enabled in the flex layout:
            //      For flex direction Row or RowReversed:
            //              Parent pref, min and max width:
            //                  Sum of the pref, min and max width of the child
            //                  items
            //              Parent pref, min and max height:
            //                  Max of pref, min and max height of the child
            //                  items
            //      For flex direction Column or ColumnReversed:
            //              Parent pref, min and max width:
            //                  Max of pref, min and max width of the child
            //                  items
            //              Parent pref, min and max height:
            //                  Sum of the pref, min and max height of the
            //                  child items
            // Else if wrap enabled in the flex layout: (either Wrap or
            // WrapReversed)
            //      For flex direction Row or RowReversed or Column or
            //      ColumnReversed:
            //              Parent pref, min, max width/height:
            //                  Sum of the pref, min and max width/height of
            //                  the child items
            if (auto *qFlexLayout = qobject_cast<QQuickFlexboxLayout *>(m_flexboxParentItem->quickItem())) {
                if (qFlexLayout->wrap() == QQuickFlexboxLayout::NoWrap) {
                    if (qFlexLayout->direction() == QQuickFlexboxLayout::Row ||
                        qFlexLayout->direction() == QQuickFlexboxLayout::RowReverse) {
                        // Minimum size
                        minS.setWidth(minS.width() + hints.min().width());
                        minS.setHeight(qMax(minS.height(), hints.min().height()));
                        // Preferred size
                        prefS.setWidth(prefS.width() + hints.pref().width());
                        prefS.setHeight(qMax(prefS.height(), hints.pref().height()));
                        // Maximum size
                        maxS.setWidth(maxS.width() + hints.max().width());
                        maxS.setHeight(qMax(maxS.height(), hints.max().height()));
                    } else if (qFlexLayout->direction() == QQuickFlexboxLayout::Column ||
                               qFlexLayout->direction() == QQuickFlexboxLayout::ColumnReverse) {
                        // Minimum size
                        minS.setWidth(qMax(minS.width(), hints.min().width()));
                        minS.setHeight(minS.height() + hints.min().height());
                        // Preferred size
                        prefS.setWidth(qMax(prefS.width(), hints.pref().width()));
                        prefS.setHeight(prefS.height() + hints.pref().height());
                        // Maximum size
                        maxS.setWidth(qMax(maxS.width(), hints.max().width()));
                        maxS.setHeight(maxS.height() + hints.max().height());
                    }
                } else if (qFlexLayout->wrap() == QQuickFlexboxLayout::Wrap ||
                           qFlexLayout->wrap() == QQuickFlexboxLayout::WrapReverse) {
                    minS += hints.min();
                    prefS += hints.pref();
                    maxS += hints.max();
                }
            }
        }
    }
    return askingFor;
}

void QQuickFlexboxLayoutEngine::invalidateItemSizeHint(QQuickItem *item)
{
    if (auto *flexLayoutItem = findFlexboxLayoutItem(item)) {
        SizeHints &hints = flexLayoutItem->cachedItemSizeHints();
        hints.min() = QSizeF();
        hints.pref() = QSizeF();
        hints.max() = QSizeF();
    }
}

void QQuickFlexboxLayoutEngine::setGeometries(const QSizeF &contentSize)
{
    m_flexboxParentItem->setSize(contentSize);
    m_flexboxParentItem->computeLayout(contentSize);
    for (auto *item : m_flexLayoutItems) {
        item->quickItem()->setPosition(item->position());
        QSizeF oldSize = item->quickItem()->size();
        QSizeF newSize = item->size();
        if (oldSize == newSize) {
            // Enforce rearrange as the size remains the same.
            // This can happen in a case where we add a child item to the layout
            // (which is already a child to a layout)
            if (auto *layout = qobject_cast<QQuickLayout *>(item->quickItem())) {
                if (layout->invalidatedArrangement())
                    layout->rearrange(newSize);
            }
        } else {
            item->quickItem()->setSize(newSize);
        }
    }
}

// TODO: Need to check whether its needed to get the size of the flex item
// through the callback measure function
// QSizeF QQuickFlexboxLayoutItem::getSizeHint(float width,
// YGMeasureMode widthMode, float height, YGMeasureMode heightMode)
// {
//     QSizeF newSize(width, height);
//     switch (widthMode) {
//     case YGMeasureModeAtMost:
//         newSize.setWidth(m_cachedSizeHint.max().width());
//         break;
//     case YGMeasureModeExactly:
//     case YGMeasureModeUndefined:
//         newSize.setWidth(m_cachedSizeHint.pref().width());
//         break;
//     default: break;
//     }
//     switch (heightMode) {
//     case YGMeasureModeAtMost:
//         newSize.setHeight(m_cachedSizeHint.max().height());
//         break;
//     case YGMeasureModeExactly:
//     case YGMeasureModeUndefined:
//         newSize.setHeight(m_cachedSizeHint.pref().height());
//         break;
//     default: break;
//     }
//     return newSize;
// }

// YGSize QQuickFlexboxLayoutItem::measureFunc(YGNodeRef nodeRef, float width,
// YGMeasureMode widthMode, float height, YGMeasureMode heightMode)
// {
//     YGSize defaultSize;
//     auto *layoutItem = static_cast<QQuickFlexboxLayoutItem *>(YGNodeGetContext(nodeRef));
//     if (layoutItem) {
//         QSizeF size = layoutItem->getSizeHint(width, widthMode, height, heightMode);
//         defaultSize.width = (qt_is_nan(size.width()) || qt_is_inf(size.width())) ? YGUndefined : size.width();
//         defaultSize.height = (qt_is_nan(size.height()) || qt_is_inf(size.height())) ? YGUndefined : size.height();
//     }
//     return defaultSize;
// }

QT_END_NAMESPACE
