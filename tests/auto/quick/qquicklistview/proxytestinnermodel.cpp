// Copyright (C) 2016 Canonical Limited and/or its subsidiary(-ies).
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "proxytestinnermodel.h"

ProxyTestInnerModel::ProxyTestInnerModel()
{
    append("Adios");
    append("Hola");
    append("Halo");
}

QModelIndex ProxyTestInnerModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid())
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex ProxyTestInnerModel::parent(const QModelIndex & /*parent*/) const
{
    return QModelIndex();
}

int ProxyTestInnerModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_values.size();
}

int ProxyTestInnerModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return 1;
}

QVariant ProxyTestInnerModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    return m_values[index.row()];
}

void ProxyTestInnerModel::append(const QString &s)
{
    beginInsertRows(QModelIndex(), m_values.size(), m_values.size());
    m_values << s;
    endInsertRows();
}

void ProxyTestInnerModel::setValue(int i, const QString &s)
{
    m_values[i] = s;
    Q_EMIT dataChanged(index(i, 0), index(i, 0));
}

void ProxyTestInnerModel::moveTwoToZero()
{
    beginMoveRows(QModelIndex(), 2, 2, QModelIndex(), 0);
    m_values.move(2, 0);
    endMoveRows();
}

void ProxyTestInnerModel::doStuff()
{
    moveTwoToZero();
    setValue(1, "Hilo");
}
