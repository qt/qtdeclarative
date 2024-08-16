// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "minortickmarkcontainer.h"

#include <QLoggingCategory>
#include <QQmlInfo>

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(positionAt, "qt.quick.tests.gauge.minortickmarkcontainer");

MinorTickmarkContainer::MinorTickmarkContainer(QQuickItem *parent)
    : TickmarkContainer(parent)
{
}

qreal MinorTickmarkContainer::tickmarkWidth() const
{
    return mTickmarkWidth;
}

void MinorTickmarkContainer::setTickmarkWidth(qreal tickmarkWidth)
{
    if (qFuzzyCompare(mTickmarkWidth, tickmarkWidth))
        return;

    mTickmarkWidth = tickmarkWidth;
    makeDirtyAndPolish(DelegateItemDirtyFlag::GeometryDirty);
    emit tickmarkWidthChanged();
}

int MinorTickmarkContainer::minorCount() const
{
    return mMinorCount;
}

void MinorTickmarkContainer::setMinorCount(int minorCount)
{
    if (mMinorCount == minorCount)
        return;

    mMinorCount = minorCount;
    makeDirtyAndPolish(DelegateItemDirtyFlag::AllDirty);
    emit minorCountChanged();
}

int MinorTickmarkContainer::totalDelegateItemCount() const
{
    return (mCount - 1) * mMinorCount;
}

qreal MinorTickmarkContainer::valueForDelegateItemAt(int index) const
{
    const int tickmarkIndex = qFloor(index / mMinorCount);
    const qreal minorTickmarkStepSize = mValueRange->stepSize() / (mMinorCount + 1);
    return index * minorTickmarkStepSize + minorTickmarkStepSize * tickmarkIndex + minorTickmarkStepSize + mValueRange->minimumValue();
}

QPointF MinorTickmarkContainer::positionForDelegateItemAt(int index) const
{
    const auto *delegateItem = mDelegateItems.at(index);
    const bool vertical = isVertical();
    const qreal tickmarkSpacing = spacing();

    // We start from the bottom for vertical orientations.
    if (vertical)
        index = totalDelegateItemCount() - 1 - index;

    // Index within a 0 to 1 range.
    const qreal normalisedPosition = index / static_cast<qreal>(mMinorCount);
    // Position relative to our major tickmark.
    const qreal relativePosition = (index % mMinorCount + 1) * (tickmarkSpacing / (mMinorCount + 1));
    // The position of our major tickmark.
    const qreal clusterOffset = qFloor(normalisedPosition) * tickmarkSpacing;
    // The size of
    const qreal delegateSize = vertical ? delegateItem->height() : delegateItem->width();
    const qreal position = clusterOffset + relativePosition - delegateSize / 2;

    qDebug(positionAt) << "index" << index
        << "normalisedPosition" << normalisedPosition
        << "relativePosition" << relativePosition
        << "clusterOffset" << clusterOffset
        << "totalDelegateItemCount" << totalDelegateItemCount()
        << "minorTickmarkCount" << mMinorCount
        << "tickmarkWidth" << mTickmarkWidth
        << "tickmarkSpacing" << tickmarkSpacing
        << "cw" << width()
        << "ch" << height()
        << "dw" << delegateItem->width()
        << "dh" << delegateItem->height()
        << "position" << position;

    return QPointF(vertical ? 0 : position, vertical ? position : 0);
}

QT_END_NAMESPACE
