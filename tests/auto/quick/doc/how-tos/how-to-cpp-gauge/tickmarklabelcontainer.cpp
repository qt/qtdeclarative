// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "tickmarklabelcontainer.h"

#include <QQmlInfo>

QT_BEGIN_NAMESPACE

TickmarkLabelContainer::TickmarkLabelContainer(QQuickItem *parent)
    : TickmarkContainer(parent)
{
}

QFont TickmarkLabelContainer::font() const
{
    return mFont;
}

void TickmarkLabelContainer::setFont(const QFont &font)
{
    if (mFont == font)
        return;

    mFont = font;
    makeDirtyAndPolish(DelegateItemDirtyFlag::FontDirty | DelegateItemDirtyFlag::GeometryDirty);
    emit fontChanged();
}

void TickmarkLabelContainer::resetFont()
{
    setFont(QFont());
}

void TickmarkLabelContainer::recreateDelegateItems()
{
    destroyDelegateItems();

    int largestImplicitWidth = 0;

    QQmlContext *ourQmlContext = qmlContext(this);
    for (int i = 0; i < mCount; ++i) {
        auto *label = qobject_cast<QQuickItem *>(mDelegate->beginCreate(ourQmlContext));
        if (!label) {
            qmlWarning(this) << "Failed to create delegate:\n" << mDelegate->errors();
            destroyDelegateItems();
            return;
        }

        const QVariantMap initialProperties = {
            { "parent", QVariant::fromValue(this) },
            { "value", valueForDelegateItemAt(i) }
        };
        mDelegate->setInitialProperties(label, initialProperties);

        if (label->property("index").isValid())
            label->setProperty("index", i);

        mDelegateItems.append(label);

        mDelegate->completeCreate();

        // The position depends on the size of the label, which we can't know before creating it.
        label->setPosition(positionForDelegateItemAt(i));

        if (label->implicitWidth() > largestImplicitWidth)
            largestImplicitWidth = label->implicitWidth();
    }

    if (isVertical()) {
        setImplicitWidth(largestImplicitWidth);
        for (auto *label : std::as_const(mDelegateItems))
            label->setWidth(largestImplicitWidth);
    } else if (!mDelegateItems.isEmpty()) { // Horizontal
        const auto *firstLabel = mDelegateItems.first();
        // All labels should have the same implicitHeight, so just use the first one.
        setImplicitHeight(firstLabel ? firstLabel->implicitHeight() : 0);
    }
}

void TickmarkLabelContainer::updatePolish()
{
    QQuickItem::updatePolish();

    if (mDirtyFlags.testFlag(DelegateItemDirtyFlag::AllDirty)) {
        recreateDelegateItems();
        return;
    }

    Q_ASSERT(mDelegateItems.size() == mCount);

    const bool fontDirty = mDirtyFlags.testFlag(DelegateItemDirtyFlag::FontDirty);
    const bool valueDirty = mDirtyFlags.testFlag(DelegateItemDirtyFlag::ValueDirty);
    const bool geometryDirty = mDirtyFlags.testFlag(DelegateItemDirtyFlag::GeometryDirty);

    int largestImplicitWidth = 0;

    for (int i = 0; i < mCount; ++i) {
        auto *label = mDelegateItems.at(i);
        if (fontDirty) {
            const bool setSuccessfully = label->setProperty("font", mFont);
            if (!setSuccessfully)
                qmlWarning(this) << "Failed to set font property of label at index" << i;
        }

        if (valueDirty) {
            const bool setSuccessfully = label->setProperty("value", valueForDelegateItemAt(i));
            if (!setSuccessfully)
                qmlWarning(this) << "Failed to set value property of label at index" << i;
        }

        if (geometryDirty) {
            label->setPosition(positionForDelegateItemAt(i));

            if (label->implicitWidth() > largestImplicitWidth)
                largestImplicitWidth = label->implicitWidth();
        }
    }

    if (geometryDirty)
        updateSizes(largestImplicitWidth);
}

QT_END_NAMESPACE
