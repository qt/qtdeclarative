// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef IncrementalModel_H
#define IncrementalModel_H

#include <QAbstractListModel>
#include <QList>
#include <QStringList>

class IncrementalModel : public QAbstractListModel
{
    Q_OBJECT

public:
    IncrementalModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    bool canFetchMore(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;

private:
    QStringList list;
    int count;
};

#endif
