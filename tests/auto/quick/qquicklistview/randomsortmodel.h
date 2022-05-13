// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef RANDOMSORTMODEL_H
#define RANDOMSORTMODEL_H

#include <QAbstractListModel>

class RandomSortModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit RandomSortModel(QObject *parent = nullptr);
    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    void randomize();

  private:
    QList<QPair<QString, int> > mData;
};

#endif // RANDOMSORTMODEL_H
