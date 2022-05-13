// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef ELLIPSEITEM_H
#define ELLIPSEITEM_H

#include <QQuickPaintedItem>

class EllipseItem : public QQuickPaintedItem
{
    Q_OBJECT
    Q_DISABLE_COPY(EllipseItem)
    QML_NAMED_ELEMENT(EllipseItemCpp)

public:
    EllipseItem(QQuickItem *parent = nullptr);
    ~EllipseItem();
    void paint(QPainter *painter);
};

#endif // ELLIPSEITEM_H
