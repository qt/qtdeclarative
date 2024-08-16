// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef TICKMARKCONTAINER_H
#define TICKMARKCONTAINER_H

#include <QQmlComponent>
#include <QQuickItem>

#include "valuerange.h"

QT_BEGIN_NAMESPACE

class TickmarkContainer : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent *delegate READ delegate WRITE setDelegate NOTIFY delegateChanged REQUIRED FINAL)
    Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged REQUIRED FINAL)
    Q_PROPERTY(ValueRange *valueRange READ valueRange WRITE setValueRange NOTIFY valueRangeChanged REQUIRED FINAL)
    Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation NOTIFY orientationChanged REQUIRED FINAL)

    QML_ELEMENT

public:
    TickmarkContainer(QQuickItem *parent = nullptr);
    virtual ~TickmarkContainer() = default;

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    int count() const;
    void setCount(int count);

    ValueRange *valueRange() const;
    void setValueRange(ValueRange *valueRange);

    bool isVertical() const;
    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);

    enum class DelegateItemDirtyFlag {
        NotDirty = 1 << 0,
        FontDirty = 2 << 0,
        ValueDirty = 3 << 0,
        GeometryDirty = 4 << 0,
        // Intentionally not OR'd as it means we need to just recreate every label.
        AllDirty = 5 << 0
    };
    Q_DECLARE_FLAGS(DelegateItemDirtyFlags, DelegateItemDirtyFlag)

    QQuickItem *delegateItemAt(int index) const;

Q_SIGNALS:
    void delegateChanged();
    void countChanged();
    void valueRangeChanged();
    void orientationChanged();

protected:
    void componentComplete() override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;
    void updatePolish() override;

    Q_DISABLE_COPY(TickmarkContainer)

    void destroyDelegateItems();
    qreal spacing() const;
    virtual int totalDelegateItemCount() const;
    virtual qreal valueForDelegateItemAt(int index) const;
    virtual QPointF positionForDelegateItemAt(int index) const;
    virtual void updateSizes(int largestImplicitSize);
    virtual void recreateDelegateItems();

    void makeDirtyAndPolish(DelegateItemDirtyFlags dirtyFlagsToOR);

    QQmlComponent *mDelegate = nullptr;
    // Major tickmark count.
    int mCount = 0;
    ValueRange *mValueRange = nullptr;
    Qt::Orientation mOrientation = Qt::Vertical;
    QList<QQuickItem *> mDelegateItems;
    DelegateItemDirtyFlags mDirtyFlags;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(TickmarkContainer::DelegateItemDirtyFlags)

QT_END_NAMESPACE

#endif // TICKMARKCONTAINER_H
