// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:significant reason:default

#include <QtQuickLayouts/private/qquickflexboxlayout_p.h>
#include <QtQuickLayouts/private/qquickflexboxlayoutengine_p.h>
#include <QtQml/qqmlinfo.h>

/*!
    \qmltype FlexboxLayout
    //! \nativetype QQuickFlexboxLayout
    \inherits Item
    \inqmlmodule QtQuick.Layouts
    \ingroup layouts
    \since 6.10
    \preliminary
    \brief The FlexboxLayout QML construct provides a flex layout for the
    quick items.

    The FlexboxLayout enables to layout the quick items in a flexible way as
    similar to that of
    \l {https://www.w3.org/TR/css-flexbox-1/}{CSS Flexible Box Layout}.
    Internally Qt FlexboxLayout uses the \l{qtquick-attribution-yoga.html}{yoga engine}
    to derive the geometry of
    the flex items. The \l {https://www.yogalayout.dev/}{yoga library} is a
    subset of the
    \l {https://www.w3.org/TR/css-flexbox-1/}{CSS Flexible Box Layout}. Thus
    FlexboxLayout can be limited to the feature as supported in the
    \l {https://www.yogalayout.dev/}{yoga library}.

    \note The FlexboxLayout adheres to yoga library version 2.0 for its
    features.

    The items within the FlexboxLayout can be configured with preferred,
    minimum and maximum sizes through the existing layout attached properties.
    For instance, if the item within the FlexboxLayout need to be stretched,
    the layout attached property \l{Layout::fillWidth}{Layout.fillWidth} or
    \l{Layout::fillHeight}{Layout.fillHeight} can be set.

    Items in a FlexboxLayout support these attached properties:
    \list
        \li \l{FlexboxLayout::alignSelf}{FlexboxLayout.alignSelf}
        \li \l{Layout::minimumWidth}{Layout.minimumWidth}
        \li \l{Layout::minimumHeight}{Layout.minimumHeight}
        \li \l{Layout::preferredWidth}{Layout.preferredWidth}
        \li \l{Layout::preferredHeight}{Layout.preferredHeight}
        \li \l{Layout::maximumWidth}{Layout.maximumWidth}
        \li \l{Layout::maximumHeight}{Layout.maximumHeight}
        \li \l{Layout::fillWidth}{Layout.fillWidth}
        \li \l{Layout::fillHeight}{Layout.fillHeight}
        \li \l{Layout::margins}{Layout.margins}
        \li \l{Layout::leftMargin}{Layout.leftMargin}
        \li \l{Layout::rightMargin}{Layout.rightMargin}
        \li \l{Layout::topMargin}{Layout.topMargin}
        \li \l{Layout::bottomMargin}{Layout.bottomMargin}
    \endlist

    Read more about attached properties \l{QML Object Attributes}{here}.
    \sa ColumnLayout
    \sa GridLayout
    \sa RowLayout
    \sa {QtQuick.Controls::StackView}{StackView}
    \sa {Qt Quick Layouts Overview}

    To be able to use this type more efficiently, it is recommended that you
    understand the general mechanism of the Qt Quick Layouts module. Refer to
    \l{Qt Quick Layouts Overview} for more information.

    \section1 Example Usage

    The following snippet shows the minimalistic example of using QML
    FlexboxLayout to arrange the rectangle items in more flexible way

    \snippet layouts/simpleFlexboxLayout.qml layout-definition

    \note This API is considered tech preview and may change or be removed in
    future versions of Qt.
*/

QT_BEGIN_NAMESPACE

class QQuickFlexboxLayoutPrivate : public QQuickLayoutPrivate
{
    Q_DECLARE_PUBLIC(QQuickFlexboxLayout)

public:
    QQuickFlexboxLayoutPrivate() : QQuickLayoutPrivate() {}
    const QQuickFlexboxLayoutEngine& getFlexEngine() const { return m_flexEngine; }

private:
    QQuickFlexboxLayoutEngine m_flexEngine;
    QQuickFlexboxLayout::FlexboxDirection m_direction = QQuickFlexboxLayout::Row;
    QQuickFlexboxLayout::FlexboxWrap m_wrap = QQuickFlexboxLayout::NoWrap;
    QQuickFlexboxLayout::FlexboxAlignment m_alignItems = QQuickFlexboxLayout::AlignStart;
    // Align items within the layout in the multi-line containter (i.e. with
    // wrap enabled) and its aligned to the cross axis of the flexbox layout
    QQuickFlexboxLayout::FlexboxAlignment m_alignContent = QQuickFlexboxLayout::AlignStretch;
    // Align content item in the multi-line containter and its aligned to the
    // main axis of the flexbox layout
    QQuickFlexboxLayout::FlexboxJustify m_justifyContent = QQuickFlexboxLayout::JustifyStart;
    qreal m_gap = 0.;
    qreal m_rowGap = 0.;
    qreal m_columnGap = 0.;
    std::bitset<QQuickFlexboxLayout::GapMax> m_gapBitSet;
};

static QQuickFlexboxLayoutAttached *attachedFlexboxLayoutObject(QQuickItem *item, bool create = false)
{
    return static_cast<QQuickFlexboxLayoutAttached*>(
            qmlAttachedPropertiesObject<QQuickFlexboxLayout>(item, create));
}

QQuickFlexboxLayout::QQuickFlexboxLayout(QQuickItem *parent) :
    QQuickLayout(*new QQuickFlexboxLayoutPrivate, parent)
{
    Q_D(QQuickFlexboxLayout);
    d->m_flexEngine.setFlexboxParentItem(new QQuickFlexboxLayoutItem(this));
}

QQuickFlexboxLayout::~QQuickFlexboxLayout()
{
    Q_D(QQuickFlexboxLayout);
    d->m_flexEngine.clearItems();
}

/*!
    \qmlproperty enumeration FlexboxLayout::direction

    This property holds the item layout direction within the flex box layout
    and it defines the
    \l {https://www.w3.org/TR/css-flexbox-1/#box-model}{main-axis}.

    Possible values:

    \value FlexboxLayout.Row                (default) Items are laid out from
                                            left to right.
    \value FlexboxLayout.RowReversed        Items are laid out from right to
                                            left.
    \value FlexboxLayout.Column             Items are laid out from top to
                                            bottom.
    \value FlexboxLayout.ColumnReversed     Items are laid out from bottom to
                                            top.

    The default value is \c FlexboxLayout.Row.
*/
QQuickFlexboxLayout::FlexboxDirection QQuickFlexboxLayout::direction() const
{
    Q_D(const QQuickFlexboxLayout);
    return d->m_direction;
}

void QQuickFlexboxLayout::setDirection(QQuickFlexboxLayout::FlexboxDirection direction)
{
    Q_D(QQuickFlexboxLayout);
    if (d->m_direction == direction)
        return;
    d->m_direction = direction;
    invalidate();
    emit directionChanged();
}

/*!
    \qmlproperty enumeration FlexboxLayout::wrap

    This property specifies that the items within the flex box layout can wrap
    or not and it happens when the children overflow the size of the flex box
    layout. If the items are wrapped, it will be placed in multiple lines
    depending on overflow condition as stated. Each line takes up the
    maximum size of the item along the
    \l {https://www.w3.org/TR/css-flexbox-1/#box-model}{cross-axis}.

    Possible values:

    \value FlexboxLayout.Wrap           Items are wrapped into multiple lines
                                        within the flex box layout.
    \value FlexboxLayout.NoWrap         (default) Items are not wrapped and
                                        laid out in single line within the
                                        flex box layout.
    \value FlexboxLayout.WrapReverse    Items are wrapped into multiple lines
                                        within the flex box layout in the
                                        reverse direction.

    The default value is \c FlexboxLayout.NoWrap.
*/
QQuickFlexboxLayout::FlexboxWrap QQuickFlexboxLayout::wrap() const
{
    Q_D(const QQuickFlexboxLayout);
    return d->m_wrap;
}

void QQuickFlexboxLayout::setWrap(QQuickFlexboxLayout::FlexboxWrap wrapMode)
{
    Q_D(QQuickFlexboxLayout);
    if (d->m_wrap == wrapMode)
        return;
    d->m_wrap = wrapMode;
    invalidate();
    emit wrapChanged();
}

/*!
    \qmlproperty enumeration FlexboxLayout::alignItems

    This property specifies the alignment of the items within the
    \l {https://www.w3.org/TR/css-flexbox-1/#flex-lines}{flex lines} of the
    flex box layout and its aligned along the
    \l {https://www.w3.org/TR/css-flexbox-1/#box-model}{cross-axis}
    (which is orthogonal to the main-axis, as defined by the property
    \l {direction}). This property can be overridden by the
    items within the flex box layout through the attached property
    \l {FlexboxLayout::alignSelf}{FlexboxLayout.alignSelf}.

    Possible values:

    \value FlexboxLayout.AlignStart         (default) Items are aligned to the
                                            start of the flex box layout
                                            cross-axis.
    \value FlexboxLayout.AlignCenter        Items are aligned along the center
                                            of the flex box layout cross-axis.
    \value FlexboxLayout.AlignEnd           Items are aligned to the end of the
                                            flex box layout cross-axis.

    \note The alignments mentioned in possible values are only applicable for
    the \l{alignItems} property

    The default value is \c FlexboxLayout.AlignStart.
*/
QQuickFlexboxLayout::FlexboxAlignment QQuickFlexboxLayout::alignItems() const
{
    Q_D(const QQuickFlexboxLayout);
    return d->m_alignItems;
}

void QQuickFlexboxLayout::setAlignItems(QQuickFlexboxLayout::FlexboxAlignment alignment)
{
    Q_D(QQuickFlexboxLayout);
    if (alignment >= QQuickFlexboxLayout::AlignStretch || alignment <= QQuickFlexboxLayout::AlignAuto) {
        qWarning("Not applicable for Flexbox layout container");
        return;
    }
    if (d->m_alignItems == alignment)
        return;
    d->m_alignItems = alignment;
    invalidate();
    emit alignItemsChanged();
}

/*!
    \qmlproperty enumeration FlexboxLayout::alignContent

    This property specifies the distribution of the
    \l {https://www.w3.org/TR/css-flexbox-1/#flex-lines}{flex lines} along the
    \l {https://www.w3.org/TR/css-flexbox-1/#box-model}{cross-axis} of the
    flex box layout.

    Possible values:

    \value FlexboxLayout.AlignStart         Flex lines are aligned to
                                            the start of the flex box layout.
    \value FlexboxLayout.AlignCenter        Flex lines are aligned along the
                                            center of the flex box layout.
    \value FlexboxLayout.AlignEnd           Flex lines are aligned to the end
                                            of the flex box layout.
    \value FlexboxLayout.AlignStretch       (default) Flex lines are stretched
                                            according to the height of the flex
                                            box layout.
    \value FlexboxLayout.AlignSpaceBetween  The spaces are evenly distributed
                                            between the lines and no space
                                            along the edge of the flex box
                                            layout.
    \value FlexboxLayout.AlignSpaceAround   The spaces are evenly distributed
                                            between the lines and half-size
                                            space on the edges of the flex box
                                            layout.
    \value FlexboxLayout.AlignSpaceEvenly   The spaces are evenly distributed
                                            between the lines and the edges of
                                            the flex box layout. Not supported
                                            in Qt 6.10.

    The default value is \c FlexboxLayout.AlignStart.
*/
QQuickFlexboxLayout::FlexboxAlignment QQuickFlexboxLayout::alignContent() const
{
    Q_D(const QQuickFlexboxLayout);
    return d->m_alignContent;
}

void QQuickFlexboxLayout::setAlignContent(QQuickFlexboxLayout::FlexboxAlignment alignment)
{
    Q_D(QQuickFlexboxLayout);
    if (alignment == QQuickFlexboxLayout::AlignSpaceEvenly) {
        qmlWarning(this) << "Currently not supported for Flexbox layout container";
        return;
    }
    if (d->m_alignContent == alignment)
        return;
    d->m_alignContent = alignment;
    invalidate();
    emit alignContentChanged();
}

/*!
    \qmlproperty enumeration FlexboxLayout::justifyContent

    This property specifies the distribution of the items along the
    \l {https://www.w3.org/TR/css-flexbox-1/#box-model}{main-axis} of the
    flex box layout.

    Possible values:

    \value FlexboxLayout.JustifyStart           (default) Items are aligned to
                                                the start of the flex box
                                                layout.
    \value FlexboxLayout.JustifyCenter          Items are aligned along the
                                                center of the flex box layout.
    \value FlexboxLayout.JustifyEnd             Items are aligned to the end of
                                                the flex box layout.
    \value FlexboxLayout.JustifySpaceBetween    The spaces are evenly
                                                distributed between the items
                                                and no space along the edges
                                                of the flex box layout.
    \value FlexboxLayout.JustifySpaceAround     The spaces are evenly
                                                distributed between the items
                                                and half-size space on the
                                                edges of the flex box layout.
    \value FlexboxLayout.JustiftSpaceEvenly     The spaces are evenly
                                                distributed between the items
                                                and edges of the flex
                                                box layout.

    The default value is \c FlexboxLayout.JustifyStart.
*/
QQuickFlexboxLayout::FlexboxJustify QQuickFlexboxLayout::justifyContent() const
{
    Q_D(const QQuickFlexboxLayout);
    return d->m_justifyContent;
}

void QQuickFlexboxLayout::setJustifyContent(QQuickFlexboxLayout::FlexboxJustify justifyContent)
{
    Q_D(QQuickFlexboxLayout);
    if (d->m_justifyContent == justifyContent)
        return;
    d->m_justifyContent = justifyContent;
    invalidate();
    emit justifyContentChanged();
}

/*!
    \qmlproperty real FlexboxLayout::gap

    This property holds the amount of space that need to be applied
    to the \l {FlexboxLayout} both along the
    \l {https://www.w3.org/TR/css-align-3/#gaps}{inline axis and block axis}.

    The default value is \c 0.
*/
qreal QQuickFlexboxLayout::gap() const
{
    Q_D(const QQuickFlexboxLayout);
    return d->m_gap;
}

void QQuickFlexboxLayout::setGap(qreal gap)
{
    Q_D(QQuickFlexboxLayout);
    if (d->m_gap == gap)
        return;
    d->m_gap = gap;
    d->m_gapBitSet.set(GapAll);
    invalidate();
    emit gapChanged();
    if (!isGapBitSet(GapRow))
        emit rowGapChanged();
    if (!isGapBitSet(GapColumn))
        emit columnGapChanged();
}

void QQuickFlexboxLayout::resetGap()
{
    Q_D(QQuickFlexboxLayout);
    d->m_gap = 0;
    d->m_gapBitSet.reset(GapAll);
    emit gapChanged();
    if (!isGapBitSet(GapRow))
        emit rowGapChanged();
    if (!isGapBitSet(GapColumn))
        emit columnGapChanged();
}

/*!
    \qmlproperty real FlexboxLayout::rowGap

    This property holds the amount of space that need to be applied to the
    \l {FlexboxLayout} along the
    \l {https://www.w3.org/TR/css-align-3/#gaps}{block axis}. Setting this
    property overrides the \l{gap} value affecting the
    \l {https://www.w3.org/TR/css-align-3/#gaps}{block axis}.

    The default value is \c 0.
*/
qreal QQuickFlexboxLayout::rowGap() const
{
    Q_D(const QQuickFlexboxLayout);
    if (!isGapBitSet(GapRow))
        return d->m_gap;
    return d->m_rowGap;
}

void QQuickFlexboxLayout::setRowGap(qreal gap)
{
    Q_D(QQuickFlexboxLayout);
    if (d->m_rowGap == gap)
        return;
    d->m_rowGap = gap;
    d->m_gapBitSet.set(QQuickFlexboxLayout::GapRow);
    invalidate();
    emit rowGapChanged();
}

void QQuickFlexboxLayout::resetRowGap()
{
    Q_D(QQuickFlexboxLayout);
    d->m_rowGap = 0;
    d->m_gapBitSet.reset(GapRow);
    emit rowGapChanged();
}

/*!
    \qmlproperty real FlexboxLayout::columnGap

    This property holds the amount of space that need to be applied
    to the \l {FlexboxLayout} along the
    \l {https://www.w3.org/TR/css-align-3/#gaps}{inline axis}. Setting this
    property override the \l{gap} value affecting the
    \l {https://www.w3.org/TR/css-align-3/#gaps}{inline axis}.

    The default value is \c 0.
*/
qreal QQuickFlexboxLayout::columnGap() const
{
    Q_D(const QQuickFlexboxLayout);
    if (!isGapBitSet(GapColumn))
        return d->m_gap;
    return d->m_columnGap;
}

void QQuickFlexboxLayout::setColumnGap(qreal gap)
{
    Q_D(QQuickFlexboxLayout);
    if (d->m_columnGap == gap)
        return;
    d->m_columnGap = gap;
    d->m_gapBitSet.set(QQuickFlexboxLayout::GapColumn);
    invalidate();
    emit columnGapChanged();
}

void QQuickFlexboxLayout::resetColumnGap()
{
    Q_D(QQuickFlexboxLayout);
    d->m_columnGap = 0;
    d->m_gapBitSet.reset(GapColumn);
    emit columnGapChanged();
}

void QQuickFlexboxLayout::updateLayoutItems()
{
    Q_D(QQuickFlexboxLayout);
    // Clean all the items in the layout
    d->m_flexEngine.clearItems();
    // Update the parent item properties
    if (auto *flexParentItem = d->m_flexEngine.getFlexboxParentItem()) {
        flexParentItem->setFlexDirection(d->m_direction);
        flexParentItem->setFlexWrap(d->m_wrap);
        flexParentItem->setFlexAlignItemsProperty(d->m_alignItems);
        flexParentItem->setFlexAlignContentProperty(d->m_alignContent);
        flexParentItem->setFlexJustifyContentProperty(d->m_justifyContent);
        if (isGapBitSet(QQuickFlexboxLayout::GapAll))
            flexParentItem->setFlexGap(QQuickFlexboxLayout::GapAll, d->m_gap);
        if (isGapBitSet(QQuickFlexboxLayout::GapRow))
            flexParentItem->setFlexGap(QQuickFlexboxLayout::GapRow, d->m_rowGap);
        if (isGapBitSet(QQuickFlexboxLayout::GapColumn))
            flexParentItem->setFlexGap(QQuickFlexboxLayout::GapColumn, d->m_columnGap);
    }

    // Insert the items in the layout
    const QList<QQuickItem *> items = childItems();
    for (auto *childItem : items) {
        Q_ASSERT(childItem);
        checkAnchors(childItem);
        if (shouldIgnoreItem(childItem))
            continue;
        // Create and set the attached properties of the flex item and add it as child
        auto *flexLayoutItem = new QQuickFlexboxLayoutItem(childItem);
        if (auto *flexItemAttachedProperties = attachedFlexboxLayoutObject(childItem))
            flexLayoutItem->setFlexAlignSelfProperty(flexItemAttachedProperties->alignSelf());
        d->m_flexEngine.insertItem(flexLayoutItem);
    }
}

void QQuickFlexboxLayout::checkAnchors(QQuickItem *item) const
{
    QQuickAnchors *anchors = QQuickItemPrivate::get(item)->_anchors;
    if (anchors && anchors->activeDirections())
        qmlWarning(item) << "Detected anchors on an item that is managed by a layout. This is undefined behavior; use FlexboxLayout alignment properties instead.";
}

void QQuickFlexboxLayout::componentComplete()
{
    QQuickLayout::componentComplete();
    ensureLayoutItemsUpdated(ApplySizeHints);
    if (qobject_cast<QQuickLayout*>(parentItem()))
        return;
    rearrange(QSizeF(width(), height()));
}

void QQuickFlexboxLayout::itemVisibilityChanged(QQuickItem *item)
{
    if (!isReady())
        return;
    invalidate(item);
}

QSizeF QQuickFlexboxLayout::sizeHint(Qt::SizeHint whichSizeHint) const
{
    Q_D(const QQuickFlexboxLayout);
    QSizeF sizeHint =  d->m_flexEngine.sizeHint(whichSizeHint);
    d->m_dirty = false;
    return sizeHint;
}

QQuickFlexboxLayoutAttached *QQuickFlexboxLayout::qmlAttachedProperties(QObject *object)
{
    return new QQuickFlexboxLayoutAttached(object);
}

bool QQuickFlexboxLayout::isGapBitSet(QQuickFlexboxLayout::FlexboxGap gap) const
{
    Q_D(const QQuickFlexboxLayout);
    if (gap < QQuickFlexboxLayout::GapRow || gap > QQuickFlexboxLayout::GapAll)
        return false;
    return d->m_gapBitSet[gap];
}

QQuickItem *QQuickFlexboxLayout::itemAt(int index) const
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

int QQuickFlexboxLayout::itemCount() const
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

void QQuickFlexboxLayout::invalidate(QQuickItem *childItem)
{
    Q_D(QQuickFlexboxLayout);
    d->m_flexEngine.invalidateItemSizeHint(childItem);
    QQuickLayout::invalidate(this);
    if (QQuickLayout *parentLayout = qobject_cast<QQuickLayout *>(parentItem()))
        parentLayout->invalidate(this);
}

void QQuickFlexboxLayout::childItemsChanged()
{
    const int count = itemCount();
    for (int i = 0; i < count; ++i) {
        QQuickItem *child = itemAt(i);
        checkAnchors(child);
    }
}

void QQuickFlexboxLayout::rearrange(const QSizeF &newSize)
{
    Q_D(QQuickFlexboxLayout);
    if (newSize.isNull() || !newSize.isValid())
        return;
    d->m_flexEngine.setGeometries(newSize);
    QQuickLayout::rearrange(newSize);
}

void QQuickFlexboxLayout::itemSiblingOrderChanged(QQuickItem *)
{
    if (!isReady())
        return;
    invalidate();
}

QQuickFlexboxLayoutAttached::QQuickFlexboxLayoutAttached(QObject *object)
{
    auto item = qobject_cast<QQuickItem*>(object);
    if (!item) {
        qmlWarning(object) << "FlexboxLayout attached property must be attached to an object deriving from Item";
        return;
    }
    setParent(object);
    if (auto flexboxLayout = qobject_cast<QQuickFlexboxLayout*>(item->parentItem())) {
        if (!flexboxLayout->isComponentComplete()) {
            // Don't try to get the index if the FlexboxLayout itself hasn't
            // loaded yet.
            return;
        }
        // In case of lazy loading in loader, attachedProperties are created
        // and updated for the object after adding the child object to the
        // stack layout, which leads to entries with same index. Triggering
        // childItemsChanged() resets to right index in the stack layout.
        flexboxLayout->childItemsChanged();
    }
}

/*!
    \qmlattachedproperty enumeration FlexboxLayout::alignSelf

    This attached property allows to align this item in the flex box layout
    along the \l {https://www.w3.org/TR/css-flexbox-1/#box-model}{cross-axis}
    and it overrides the parent flex box layout property
    \l{alignItems}.

    By default, the child item inherit the alignment from the parent and it can
    override the parent flex box layout
    \l{alignItems} property with the values
    \c {FlexboxLayout.AlignStart}, \c {FlexboxLayout.AlignCenter}
    and \c {FlexboxLayout.AlignEnd}.

    The default value is \c {FlexboxLayout.AlignAuto}.
*/
QQuickFlexboxLayout::FlexboxAlignment QQuickFlexboxLayoutAttached::alignSelf() const
{
    return m_alignSelf;
}

void QQuickFlexboxLayoutAttached::setAlignSelf(QQuickFlexboxLayout::FlexboxAlignment alignment)
{
    if (m_alignSelf == alignment)
        return;

    m_alignSelf = alignment;
    const auto *item = qobject_cast<QQuickItem*>(parent());
    if (auto *flexLayout = qobject_cast<QQuickFlexboxLayout *>(item->parent())) {
        auto *priv = dynamic_cast<QQuickFlexboxLayoutPrivate *>(QQuickLayoutPrivate::get(flexLayout));
        auto &flexEngine = priv->getFlexEngine();
        auto *item = qobject_cast<QQuickItem *>(parent());
        if (auto *flexLayoutItem = flexEngine.findFlexboxLayoutItem(item)) {
            flexLayoutItem->setFlexAlignSelfProperty(alignment);
            flexLayout->invalidate();
        }
    }
    emit alignSelfChanged();
}

QT_END_NAMESPACE

#include "moc_qquickflexboxlayout_p.cpp"
