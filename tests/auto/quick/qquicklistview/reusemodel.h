// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef REUSEMODEL_H
#define REUSEMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QStringList>

class ReuseModel : public QAbstractListModel
{
    Q_OBJECT

public:
    ReuseModel(int rowCount, QObject *parent = nullptr)
        : QAbstractListModel(parent)
        , m_rowCount(rowCount)
    {}

    int rowCount(const QModelIndex & = QModelIndex()) const override
    {
        return m_rowCount;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return QVariant();

        switch (role) {
        case Qt::DisplayRole:
            return displayStringForRow(index.row());
        default:
            break;
        }

        return QVariant();
    }

    QString displayStringForRow(int row) const
    {
        return row % 2 == 0 ?
            QStringLiteral("Even%1").arg(row) :
            QStringLiteral("Odd%1").arg(row);
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {Qt::DisplayRole, "display"},
        };
    }

private:
    int m_rowCount;
};

#endif
