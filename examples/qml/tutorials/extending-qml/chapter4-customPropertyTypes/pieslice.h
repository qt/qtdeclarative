// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef PIESLICE_H
#define PIESLICE_H

#include <QtQuick/QQuickPaintedItem>
#include <QColor>

//![0]
class PieSlice : public QQuickPaintedItem
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor FINAL)
    QML_ELEMENT

public:
    PieSlice(QQuickItem *parent = nullptr);

    QColor color() const;
    void setColor(const QColor &color);

    void paint(QPainter *painter) override;

private:
    QColor m_color;
};
//![0]

#endif

