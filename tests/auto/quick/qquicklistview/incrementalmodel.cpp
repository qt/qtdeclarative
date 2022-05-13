// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "incrementalmodel.h"
#include <QGuiApplication>
#include <QDebug>

IncrementalModel::IncrementalModel(QObject *parent)
    : QAbstractListModel(parent), count(0)
{
    for (int i = 0; i < 100; ++i)
        list.append("Item " + QString::number(i));
}

int IncrementalModel::rowCount(const QModelIndex & /* parent */) const
{
    return count;
}

QVariant IncrementalModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= list.size() || index.row() < 0)
        return QVariant();

    if (role == Qt::DisplayRole)
        return list.at(index.row());
    return QVariant();
}

bool IncrementalModel::canFetchMore(const QModelIndex & /* index */) const
{
    if (count < list.size())
        return true;
    else
        return false;
}

void IncrementalModel::fetchMore(const QModelIndex & /* index */)
{
    int remainder = list.size() - count;
    int itemsToFetch = qMin(5, remainder);

    beginInsertRows(QModelIndex(), count, count+itemsToFetch-1);

    count += itemsToFetch;

    endInsertRows();
}
