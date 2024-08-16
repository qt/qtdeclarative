// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef MINORTICKMARKCONTAINER_H
#define MINORTICKMARKCONTAINER_H

#include <QQmlComponent>
#include <QQuickItem>

#include "tickmarkcontainer.h"

QT_BEGIN_NAMESPACE

class MinorTickmarkContainer : public TickmarkContainer
{
    Q_OBJECT
    Q_PROPERTY(qreal tickmarkWidth READ tickmarkWidth WRITE setTickmarkWidth NOTIFY tickmarkWidthChanged REQUIRED FINAL)
    Q_PROPERTY(int minorCount READ minorCount WRITE setMinorCount NOTIFY minorCountChanged REQUIRED FINAL)
    QML_ELEMENT

public:
    MinorTickmarkContainer(QQuickItem *parent = nullptr);
    virtual ~MinorTickmarkContainer() = default;

    qreal tickmarkWidth() const;
    void setTickmarkWidth(qreal tickmarkWidth);

    int minorCount() const;
    void setMinorCount(int minorCount);

Q_SIGNALS:
    void tickmarkWidthChanged();
    void minorCountChanged();

private:
    Q_DISABLE_COPY(MinorTickmarkContainer)

    int totalDelegateItemCount() const override;
    qreal valueForDelegateItemAt(int index) const override;
    QPointF positionForDelegateItemAt(int index) const override;

    qreal mTickmarkWidth = 0;
    int mMinorCount = 0;
};

QT_END_NAMESPACE

#endif // MINORTICKMARKCONTAINER_H
