// Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef INNERMODEL_H
#define INNERMODEL_H

#include <QAbstractItemModel>

class ProxyTestInnerModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    ProxyTestInnerModel();
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex & /*parent*/) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_INVOKABLE void doStuff();

private:
    void append(const QString &s);
    void setValue(int i, const QString &s);
    void moveTwoToZero();

private:
    QList<QString> m_values;
};

#endif
