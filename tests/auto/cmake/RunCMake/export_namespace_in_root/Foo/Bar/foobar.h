// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#pragma once

#include <QtQuick/QQuickPaintedItem>

class Foobar : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
public:
    using QQuickPaintedItem::QQuickPaintedItem;

    void paint(QPainter *painter) override;
};
