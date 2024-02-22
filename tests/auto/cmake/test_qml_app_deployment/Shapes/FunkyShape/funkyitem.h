// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#ifndef FunkyItem_H
#define FunkyItem_H

#include <QQuickPaintedItem>

class FunkyItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_DISABLE_COPY(FunkyItem)
    QML_NAMED_ELEMENT(FunkyItemCpp)

public:
    FunkyItem(QQuickItem *parent = nullptr);
    ~FunkyItem();
    void paint(QPainter *painter);
};

#endif // FunkyItem_H
