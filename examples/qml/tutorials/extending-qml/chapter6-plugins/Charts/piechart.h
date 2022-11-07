// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef PIECHART_H
#define PIECHART_H

#include <QtQuick/QQuickItem>

class PieSlice;

class PieChart : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<PieSlice> slices READ slices FINAL)
    Q_PROPERTY(QString name READ name WRITE setName FINAL)
    QML_ELEMENT

public:
    PieChart(QQuickItem *parent = nullptr);

    QString name() const;
    void setName(const QString &name);

    QQmlListProperty<PieSlice> slices();

private:
    static void append_slice(QQmlListProperty<PieSlice> *list, PieSlice *slice);

    QString m_name;
    QList<PieSlice *> m_slices;
};

#endif

