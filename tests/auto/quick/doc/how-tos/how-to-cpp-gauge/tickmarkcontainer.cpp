// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tickmarkcontainer.h"

#include <QQmlInfo>

QT_BEGIN_NAMESPACE

/*
    This class (and its subclass) creates and position the tickmarks/labels.
    We do this in C++ for the reasons explained below.

    Firstly, we need the labels to start at the bottom (for a
    vertically-oriented gauge). Positioners like Column and Flow cannot do
    this. Instead, we could use a Repeater:

        // Note that we can't just use the count as the model, because that would then require
        // require the delegate to handle this logic, which we don't want because
        // delegates should only be responsible for styling.
        model: [...Array(control.tickmarkCount).keys()].map(value => control.tickmarkCount - 1 - value)

    However, we also need to vertically center each label on its corresponding
    tickmark. That requires access to each delegate in order to position them,
    which we can only do from C++. In Qt Quick Extras, this used to be avoided
    by using Loader (code taken from Base/GaugeStyle.qml):

        y: index * labelDistance - height / 2

    The problem with this is that using a loader for every tickmark and label
    is expensive.

    Another option would be to require the delegate to do the positioning, but
    that's not what we want.
*/

TickmarkContainer::TickmarkContainer(QQuickItem *parent)
    : QQuickItem(parent)
{
}

QQmlComponent *TickmarkContainer::delegate() const
{
    return mDelegate;
}

void TickmarkContainer::setDelegate(QQmlComponent *delegate)
{
    if (mDelegate == delegate)
        return;

    mDelegate = delegate;
    makeDirtyAndPolish(DelegateItemDirtyFlag::AllDirty);
    emit delegateChanged();
}

int TickmarkContainer::count() const
{
    return mCount;
}

void TickmarkContainer::setCount(int count)
{
    if (mCount == count)
        return;

    mCount = count;
    makeDirtyAndPolish(DelegateItemDirtyFlag::AllDirty);
    emit countChanged();
}

ValueRange *TickmarkContainer::valueRange() const
{
    return mValueRange;
}

void TickmarkContainer::setValueRange(ValueRange *valueRange)
{
    if (mValueRange == valueRange)
        return;

    // Should only need to be set once.
    Q_ASSERT(!mValueRange);

    mValueRange = valueRange;

    connect(mValueRange, &ValueRange::constraintsChanged, this, [this]() {
        makeDirtyAndPolish(DelegateItemDirtyFlag::ValueDirty | DelegateItemDirtyFlag::GeometryDirty);
    });

    // Don't need to make the text dirty here, as componentComplete dirties all,
    // and us being a required property guarantees we're set before that is called.
    emit valueRangeChanged();
}

bool TickmarkContainer::isVertical() const
{
    return mOrientation == Qt::Vertical;
}

Qt::Orientation TickmarkContainer::orientation() const
{
    return mOrientation;
}

void TickmarkContainer::setOrientation(Qt::Orientation orientation)
{
    if (mOrientation == orientation)
        return;

    mOrientation = orientation;
    makeDirtyAndPolish(DelegateItemDirtyFlag::ValueDirty | DelegateItemDirtyFlag::GeometryDirty);
    emit orientationChanged();
}

QQuickItem *TickmarkContainer::delegateItemAt(int index) const
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < mDelegateItems.size());
    return mDelegateItems.at(index);
}

void TickmarkContainer::destroyDelegateItems()
{
    for (auto labelIt = mDelegateItems.begin(); labelIt != mDelegateItems.end(); ) {
        auto *label = *labelIt;
        label->setParentItem(nullptr);
        label->deleteLater();
        labelIt = mDelegateItems.erase(labelIt);
    }
}

/*!
    The spacing between tickmarks, not accounting for their size (which is
    handled in positionForDelegateItemAt).
*/
qreal TickmarkContainer::spacing() const
{
    const qreal availableSize = isVertical() ? height() : width();
    return availableSize / (mCount - 1);
}

/*!
    mCount represents the number of major tickmarks, but MinorTickmarkContainer
    needs a different total since its minorCount represents the number of minor
    tickmarks for each major tickmark. This function exists purely to be used
    in recreateDelegateItems so that MinorTickmarkContainer doesn't need to
    reimplement it just to change one line.
*/
int TickmarkContainer::totalDelegateItemCount() const
{
    return mCount;
}

qreal TickmarkContainer::valueForDelegateItemAt(int index) const
{
    const qreal markerPos = index / static_cast<qreal>(mCount - 1);
    return markerPos * (mValueRange->maximumValue() - mValueRange->minimumValue()) + mValueRange->minimumValue();
}

QPointF TickmarkContainer::positionForDelegateItemAt(int index) const
{
    const auto *delegateItem = mDelegateItems.at(index);
    const bool vertical = isVertical();
    // We start from the bottom for vertical orientations.
    if (vertical)
        index = mCount - 1 - index;
    return QPointF(
        vertical ? 0 : index * spacing() - delegateItem->width() / 2,
        vertical ? index * spacing() - delegateItem->height() / 2 : 0);
}

/*!
    \a largestImplicitSize will represent implicitWidth when vertical,
    and implicitHeight when horizontal.
*/
void TickmarkContainer::updateSizes(int largestImplicitSize)
{
    if (isVertical()) {
        setImplicitWidth(largestImplicitSize);

        for (auto *delegateItem : std::as_const(mDelegateItems))
            delegateItem->setWidth(largestImplicitSize);
    } else if (!mDelegateItems.isEmpty()) { // Horizontal
        setImplicitHeight(largestImplicitSize);

        for (auto *delegateItem : std::as_const(mDelegateItems))
            delegateItem->setHeight(largestImplicitSize);
    }
}

void TickmarkContainer::recreateDelegateItems()
{
    destroyDelegateItems();

    const bool vertical = isVertical();

    int largestImplicitSize = 0;

    QQmlContext *ourQmlContext = qmlContext(this);
    for (int i = 0; i < totalDelegateItemCount(); ++i) {
        auto *delegateItem = qobject_cast<QQuickItem *>(mDelegate->beginCreate(ourQmlContext));
        if (!delegateItem) {
            qmlWarning(this) << "Failed to create delegate:\n" << mDelegate->errors();
            destroyDelegateItems();
            return;
        }

        const QVariantMap initialProperties = {{ "parent", QVariant::fromValue(this) }};
        mDelegate->setInitialProperties(delegateItem, initialProperties);

        if (delegateItem->property("index").isValid())
            delegateItem->setProperty("index", i);

        if (delegateItem->property("value").isValid())
            delegateItem->setProperty("value", valueForDelegateItemAt(i));

        mDelegateItems.append(delegateItem);

        mDelegate->completeCreate();

        // The position depends on the size of the label, which we can't know before creating it.
        // Also:
        // - do this after appending the delegate item above since positionForDelegateItemAt
        // requires it being in the list.
        // - do this after component completion because otherwise the implicit size will not have
        // been set yet, and the positioning relies on it.
        delegateItem->setPosition(positionForDelegateItemAt(i));

        if (vertical && delegateItem->implicitWidth() > largestImplicitSize)
            largestImplicitSize = delegateItem->implicitWidth();
        else if (!vertical && delegateItem->implicitHeight() > largestImplicitSize)
            largestImplicitSize = delegateItem->implicitHeight();
    }

    updateSizes(largestImplicitSize);
}

void TickmarkContainer::makeDirtyAndPolish(DelegateItemDirtyFlags dirtyFlagsToOR)
{
    mDirtyFlags |= dirtyFlagsToOR;
    polish();
}

void TickmarkContainer::componentComplete()
{
    QQuickItem::componentComplete();

    makeDirtyAndPolish(DelegateItemDirtyFlag::AllDirty);
}

void TickmarkContainer::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);

    makeDirtyAndPolish(DelegateItemDirtyFlag::GeometryDirty);
}

void TickmarkContainer::updatePolish()
{
    QQuickItem::updatePolish();

    if (mDirtyFlags.testFlag(DelegateItemDirtyFlag::AllDirty)) {
        recreateDelegateItems();
        return;
    }

    Q_ASSERT(!mDirtyFlags.testFlag(DelegateItemDirtyFlag::FontDirty));
    const bool valueDirty = mDirtyFlags.testFlag(DelegateItemDirtyFlag::ValueDirty);
    const bool geometryDirty = mDirtyFlags.testFlag(DelegateItemDirtyFlag::GeometryDirty);
    const bool vertical = isVertical();

    int largestImplicitSize = 0;

    for (int i = 0; i < mDelegateItems.size(); ++i) {
        auto *delegateItem = mDelegateItems.at(i);
        if (valueDirty) {
            const bool setSuccessfully = delegateItem->setProperty("value", valueForDelegateItemAt(i));
            if (!setSuccessfully)
                qmlWarning(this) << "Failed to set value property of label at index" << i;
        }

        if (geometryDirty) {
            delegateItem->setPosition(positionForDelegateItemAt(i));

            if (vertical && delegateItem->implicitWidth() > largestImplicitSize)
                largestImplicitSize = delegateItem->implicitWidth();
            else if (!vertical && delegateItem->implicitHeight() > largestImplicitSize)
                largestImplicitSize = delegateItem->implicitHeight();
        }
    }

    if (geometryDirty)
        updateSizes(largestImplicitSize);
}

QT_END_NAMESPACE
