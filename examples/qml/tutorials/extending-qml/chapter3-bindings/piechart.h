// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef PIECHART_H
#define PIECHART_H

#include <QColor>
#include <QtQuick/QQuickPaintedItem>

//![0]
class PieChart : public QQuickPaintedItem
{
//![0]
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName FINAL)
    QML_ELEMENT

//![1]
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged FINAL)
public:
//![1]

    PieChart(QQuickItem *parent = nullptr);

    QString name() const;
    void setName(const QString &name);

    QColor color() const;
    void setColor(const QColor &color);

    void paint(QPainter *painter) override;

    Q_INVOKABLE void clearChart();

//![2]
signals:
    void colorChanged();
//![2]

private:
    QString m_name;
    QColor m_color;

//![3]
};
//![3]

#endif

