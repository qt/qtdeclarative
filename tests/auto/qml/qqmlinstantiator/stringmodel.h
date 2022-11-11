// Copyright (C) 2016 Dmitrii Kosarev aka Kakadu <kakadu.hafanana@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#ifndef STRINGMODEL_H
#define STRINGMODEL_H

#include <QtCore/QObject>
#include <QtCore/QAbstractItemModel>
#include <QtCore/QDebug>

class StringModel : public QAbstractItemModel
{
    Q_OBJECT
    QVector<QString> items;
    QHash<int, QByteArray> roles;
    QString name;

public:
    explicit StringModel(const QString& name) : QAbstractItemModel(), name(name)
    {
        roles.insert(555, "text");
    }

    void drop(int count)
    {
        beginRemoveRows(QModelIndex(), 0, count-1);
        for (int i=0; i<count; i++)
            items.pop_front();
        endRemoveRows();
    }

    Q_INVOKABLE void add(QString s)
    {
        beginInsertRows(QModelIndex(), 0, 0);
        items.push_front(s);
        endInsertRows();
    }

    int rowCount(const QModelIndex &) const override
    {
        return items.size();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return roles;
    }

    int columnCount(const QModelIndex &) const override
    {
        return 1;
    }

    bool hasChildren(const QModelIndex &) const override
    {
        return rowCount(QModelIndex()) > 0;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent) const override
    {
        Q_UNUSED(column);
        if (row>=0 && row<rowCount(parent))
            return createIndex(row,0);
        else
            return QModelIndex();
    }

    QModelIndex parent(const QModelIndex &) const override
    {
        return QModelIndex();
    }

    QVariant data (const QModelIndex & index, int role) const override
    {
        int row = index.row();
        if ((row<0) || (row>=items.size()))
            return QVariant(QMetaType(QMetaType::UnknownType));

        switch (role) {
        case Qt::DisplayRole:
        case 555:
            return QVariant::fromValue(items.at(row));
        default:
            return QVariant();
        }
    }
};

#endif // STRINGMODEL_H
