// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "randomsortmodel.h"
#include <QRandomGenerator>

RandomSortModel::RandomSortModel(QObject* parent):
    QAbstractListModel(parent)
{
    for (int i = 0; i < 10; ++i) {
        mData.append(qMakePair(QString::fromLatin1("Item %1").arg(i), i * 10));
    }
}

QHash<int, QByteArray> RandomSortModel::roleNames() const
{
    QHash<int,QByteArray> roles = QAbstractItemModel::roleNames();
    roles[Qt::UserRole] = "SortRole";
    return roles;
}


int RandomSortModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid())
        return mData.size();

    return 0;
}

QVariant RandomSortModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (index.row() >= mData.size()) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        return QString::fromLatin1("%1 (weight %2)").arg(mData[index.row()].first).arg(mData[index.row()].second);
    } else if (role == Qt::UserRole) {
        return mData[index.row()].second;
    }

    return QVariant();
}

void RandomSortModel::randomize()
{
    const int row = QRandomGenerator::global()->bounded(mData.size());
    int random;
    bool exists = false;
    // Make sure we won't end up with two items with the same weight, as that
    // would make unit-testing much harder
    do {
        exists = false;
        random = QRandomGenerator::global()->bounded(mData.size() * 10);
        QList<QPair<QString, int> >::ConstIterator iter, end;
        for (iter = mData.constBegin(), end = mData.constEnd(); iter != end; ++iter) {
            if ((*iter).second == random) {
                exists = true;
                break;
            }
        }
    } while (exists);
    mData[row].second = random;
    Q_EMIT dataChanged(index(row, 0), index(row, 0));
}
