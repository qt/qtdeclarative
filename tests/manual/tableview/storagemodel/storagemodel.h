// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Ivan Komissarov
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef STORAGEMODEL_H
#define STORAGEMODEL_H

#include <QAbstractTableModel>
#include <QStorageInfo>

class StorageModel : public QAbstractTableModel
{
    Q_OBJECT
    Q_DISABLE_COPY(StorageModel)
public:
    enum class Column : int {
        RootPath = 0,
        Name,
        Device,
        FileSystemName,
        Free,
        IsReady,
        IsReadOnly,
        IsValid,
        Count
    };
    Q_ENUM(Column)

    enum class Role : int {
        Type = Qt::UserRole + 1,
        Heading,
        Value,
        ValueMax, // If we had ValueMin, it would always be zero in this example
        ValueDisplay,
        ValueMaxDisplay,
        Count
    };
    Q_ENUM(Role)

    enum class Type : int {
        String, // use Qt::DisplayRole
        Value,  // use Role::Value and Role::ValueMax
        Flag,   // use Qt::CheckStateRole
        Count
    };
    Q_ENUM(Type)

    explicit StorageModel(QObject *parent = nullptr);

    int columnCount(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;
    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation = Qt::Horizontal, int role = Qt::DisplayRole) const override;

public slots:
    void refresh();

private:
    QList<QStorageInfo> m_volumes;
};

#endif // STORAGEMODEL_H
